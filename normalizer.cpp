//  File contents scanner for source_normalizer
//
//  Copyright (C) 2020  Martti Ylioja
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "normalizer.h"

#include <cctype>
#include <cstring>
#include <fstream>
#include <iostream>

namespace {

//  Error bits
enum {
    //  Fixable errors
    err_unusual_whitespace = 0x0001,   // Tabs, FF, etc...
    err_trailing_whitespace = 0x0002,  // Lines with whitespace at end
    err_cr_lf_line_endings = 0x0004,   // Windows CR-LF line ending
    err_fixable = 0x00ff,
    //
    //  Hopeless errors
    err_invalid_encoding = 0x0100,    // Possibly UTF-16
    err_invalid_characters = 0x0200,  // Strange characters
    err_not_a_text_file = 0x0400,     // Not a text file
    err_hopeless = 0xff00,
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

                    //  The carriage return has already been counted as
                    //  unusual whitespace, so have to adjust the count.
                    //  Only free standing carriage returns are counted as
                    //  whitespace.
                    --unusual_whitespace;

                    //  Make sure the carriage return won't be counted as a
                    //  trailing space
                    penultimate = not_a_space;
                }
                if (isspace(last_character))
                {
                    errors |= err_trailing_whitespace;
                }
                //  Make sure this line feed won't be counted as a trailing
                //  space
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
        text.insert(pos + 1, " and");
    }
}

}  //  namespace

void Normalizer::normalize(const char* path)
{
    m_errors = 0;
    m_full_name.clear();
    m_error_message.clear();
    m_data.clear();
    m_data.reserve(64 * 1024);

    if (!load_file(path))
    {
        return;
    }

    if (!find_errors())
    {
        return;  //  No errors found
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
        int type = classify_invalid();
        switch (type)
        {
        case eBINARY:
            m_errors = err_not_a_text_file;
            add_error_message("binary data. Probably not a text file.");
            break;

        case eUTF16:
            m_errors = err_invalid_encoding;
            add_error_message("invalid encoding. Possibly UTF-16");
            break;

        default:
            break;
        }
    }

    if (m_errors & err_invalid_characters)
    {
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


//  Try to figure out what's up with the invalid characters
int Normalizer::classify_invalid() const
{
    int size = data_size();

    // clang-format off
    // ELF binaries begin with "\x7fELF" and the ELF header is at least 52 bytes long.
    if (size > 50 && std::memcmp(m_data.data(), "\x7f" "ELF", 4) == 0)
    {
        return true;
    }
    // clang-format on

    //  Files coming from Windows may sometimes have UTF-16 encoding.
    //  A leading Byte Order Mark (BOM) is a good sign of an UTF-16 or UCS-2
    //  encoding.
    unsigned bom = m_data[0] | (m_data[1] << 8);
    if (bom == 0xfeff || bom == 0xfffe)
    {
        return true;
    }

    //  Unfortunately the following heuristic doesn't work too well
    //  if the file is really short.
    if (size < 80)
    {
        return eDONT_KNOW;
    }

    int even = 0;
    int odd = 0;
    int printable = 0;
    int unprintable = 0;
    for (int ix = 0; ix < size; ++ix)
    {
        int ch = m_data[ix];
        if (ch == 0)
        {
            if (ix & 0x01)
                ++odd;
            else
                ++even;

            continue;
        }

        if (ch >= ' ' && ch <= '~')
            printable++;
        else
            unprintable++;
    }

    //  Plenty of unprintable characters would suggest a binary file
    if (unprintable > printable/3)
    {
        return eBINARY;
    }

    //  Lots of zeros in either even or odd locations suggests UTF16.
    int smaller = even;
    int bigger = odd;
    if (smaller > bigger)
    {
        smaller = odd;
        bigger = even;
    }

    if (smaller == 0 && bigger > size/3)
    {
        return eUTF16;
    }

    return eDONT_KNOW;
}


void Normalizer::add_error_message(const char* text)
{
    if (!m_error_message.empty())
    {
        m_error_message += ", ";
    }

    m_error_message += text;
}
