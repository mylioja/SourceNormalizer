#include "normalizer.h"

#include <cctype>
#include <fstream>
#include <iostream>


namespace {


    //  Error bits
    enum {
        //  Fixable errors
        err_unusual_whitespace  = 0x0001,  // Tabs, FF, etc...
        err_trailing_whitespace = 0x0002,  // Lines with whitespace at end
        err_cr_lf_line_endings  = 0x0004,  // Windows CR-LF line ending
        err_fixable             = 0x00ff,
        //
        //  Hopeless errors
        err_invalid_encoding    = 0x0100,  // Possibly UTF-16
        err_invalid_characters  = 0x0200,  // Strange characters
        err_hopeless            = 0xff00,
    };


    bool is_fixable(unsigned errors)
    {
        //  Can't fix if any hopeless errors
        if ((errors & err_hopeless) != 0)
        {
            return false;
        }

        //  Must have at least one fixable error
        if ((errors & err_fixable) != 0)
        {
            return true;
        }

        return false;
    }


    unsigned classify_errors(const char* data, int size)
    {
        unsigned errors = 0;

        int unusual_whitespace = 0;

        //  Any arbitrary character that isn't regarded as whitespace
        constexpr int not_a_space = 0;

        int current = not_a_space;
        int penultimate = not_a_space;
        int antepenultimate = not_a_space;
        int last_character = not_a_space;

        const unsigned char* ptr = reinterpret_cast<const unsigned char*>(data);
        while (size--)
        {
            current = *ptr++;

            //  Examine the character more closely if it's outside printable range
            if (current < ' ' || current > '~')
            {
                switch (current)
                {
                case '\n':  //  Line feed (newline)
                    last_character = penultimate;
                    if (penultimate == '\r')
                    {
                        //  Found a CR-LF pair "\r\n"
                        errors |= err_cr_lf_line_endings;

                        //  Adjust the last character on the line
                        last_character = antepenultimate;

                        //  The carriage return has already been counted as unusual whitespace,
                        //  so have to adjust the count. Only free standing carriage returns
                        //  are counted as whitespace.
                        --unusual_whitespace;

                        //  Make sure the carriage return won't be counted as a trailing space
                        penultimate = not_a_space;
                    }
                    if (isspace(last_character))
                    {
                        errors |= err_trailing_whitespace;
                    }
                    //  Make sure this line feed won't be counted as a trailing space
                    current = not_a_space;
                    break;

                case '\t':  //  horizontal tab
                case '\r':  //  carriage return
                case '\v':  //  vertical tab
                case '\f':  //  form feed
                    ++unusual_whitespace;
                    break;

                default:
                    errors |= err_invalid_characters;
                    break;
                }
            }

            antepenultimate = penultimate;
            penultimate = current;
        }

        if (unusual_whitespace)
        {
            errors |= err_unusual_whitespace;
        }

        return errors;
    }


    //  Add an "and" if needed to make the message nicer
    void beautify_message(std::string& text)
    {
        auto pos = text.rfind(',');
        if (pos != std::string::npos)
        {
            text.insert(pos+1, " and");
        }
    }

}   //  namespace


void Normalizer::normalize(const char* path)
{
    m_errors = 0;
    m_full_name.clear();
    m_error_message.clear();
    m_data.clear();
    m_data.reserve(64*1024);

    if (!load_file(path))
    {
        return;
    }

    if (!find_errors())
    {
        return; //  No errors found
    }


    if (is_fixable(m_errors))
    {
        //  TODO: Add code to fix the detected errors
    }
}


bool Normalizer::load_file(const char* path)
{
    m_full_name = path;

    std::ifstream input(m_full_name, std::ifstream::binary | std::ifstream::ate);
    if (!input)
    {
        return false;
    }

    auto pos = input.tellg();
    input.seekg(0);
    m_data.reserve(pos);
    m_data.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());

    return true;
}


bool Normalizer::find_errors()
{
    m_errors = classify_errors(m_data.data(), data_size());
    if (m_errors == 0)
    {
        //  Return false if no errors found
        return false;
    }

    if (m_errors & err_invalid_characters)
    {
        if (is_utf16())
        {
            //  If encoding is wrong, the other errors aren't significant
            m_errors = err_invalid_encoding;
            add_error_message("Invalid encoding. Possibly UTF-16?");
            return true;
        }

        add_error_message("Invalid characters");
    }

    if (m_errors & err_unusual_whitespace)
    {
        add_error_message("Tabs or unusual whitespace");
    }

    if (m_errors & err_trailing_whitespace)
    {
        add_error_message("Trailing whitespace");
    }

    if (m_errors & err_cr_lf_line_endings)
    {
        add_error_message("CR-LF line endings");
    }

    beautify_message(m_error_message);
    std::cerr << "File: " << m_full_name << " has " << m_error_message << '\n';

    return true;
}


//  Files coming from Windows may sometimes have UTF-16 encoding.
//  This is a quick check for it.
bool Normalizer::is_utf16() const
{
    //  A leading Byte Order Mark (BOM) is a good sign of an UTF-16 or UCS-2 encoding.
    unsigned bom = m_data[0] | (m_data[1] << 8);
    if (bom == 0xfeff || bom == 0xfffe)
    {
        return true;
    }

    //  Another sign is lots of zeros in either even or odd locations.
    //  (Because most of the source code is 7-bit ASCII characters)
    int size = data_size();

    //  Unfortunately this heuristic doesn't work too well if the file is really short.
    if (size < 100)
    {
        return false;
    }

    int even = 0;
    int odd = 0;
    for (int ix = 0; ix < size-1; ix += 2)
    {
        even += !m_data[ix];
        odd += !m_data[ix+1];
    }

    if (even < odd)
    {
        even = odd;
    }

    return even > size/3;
}


void Normalizer::add_error_message(const char* text)
{
    if (!m_error_message.empty())
    {
        m_error_message += ", ";
    }

    m_error_message += text;
}
