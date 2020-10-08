#include "normalizer.h"

#include <cctype>
#include <fstream>
#include <iostream>


namespace {


    //  Error bits
    enum {
        err_unusual_whitespace  = 0x0001,  // Tabs, FF, etc...
        err_trailing_whitespace = 0x0002,  // Lines with whitespace at end
        err_cr_lf_line_endings  = 0x0004,  // The dreaded windows line ending
        err_invalid_encoding    = 0x0008,  // Possibly UTF-16
        err_invalid_characters  = 0x0010,  // Strange stuff
    };


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


    void add_and_if_needed(std::string& text)
    {
        auto pos = text.find(',');
        if (pos != std::string::npos)
        {
            text.insert(pos+1, " and");
        }
    }

}   //  namespace


void Normalizer::normalize(const std::experimental::filesystem::path& path)
{
    m_err_bits = 0;
    m_full_name.clear();
    m_errors.clear();
    m_data.clear();
    m_data.reserve(64*1024);

    if (!load_file(path))
    {
        return;
    }

    m_err_bits = classify_errors(m_data.data(), data_size());
    if (!m_err_bits)
    {
        return;
    }

    if (m_err_bits & err_invalid_characters)
    {
        if (is_utf16())
        {
            //  If encoding is wrong, the other errors are meaningless
            m_err_bits = err_invalid_encoding;
            add_error("Invalid encoding. Possibly UTF-16?");
            return;
        }

        add_error("Invalid characters");
    }

    if (m_err_bits & err_unusual_whitespace)
    {
        add_error("Tabs or unusual whitespace");
    }

    if (m_err_bits & err_trailing_whitespace)
    {
        add_error("Trailing whitespace");
    }

    if (m_err_bits & err_cr_lf_line_endings)
    {
        add_error("CR-LF line endings");
    }

    if (!m_errors.empty())
    {
        add_and_if_needed(m_errors);
        std::cerr << "File: " << m_full_name << " has " << m_errors << '\n';
    }
}


bool Normalizer::load_file(const std::experimental::filesystem::path& path)
{
    m_full_name = std::experimental::filesystem::canonical(path).string();

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


//  Files coming from Windows may sometimes be UTF-16. This is a quick check for it.
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


void Normalizer::add_error(const char* text)
{
    if (!m_errors.empty())
    {
        m_errors += ", ";
    }

    m_errors += text;
}
