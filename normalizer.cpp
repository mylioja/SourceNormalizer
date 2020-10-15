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
#include "utf16checker.h"

#include <cctype>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>

namespace {

//  Error bits
enum {
    //  Fixable errors
    err_tabs = 0x0001,                 // Tabs
    err_unusual_whitespace = 0x0002,   // '\v', '\f', etc...
    err_trailing_whitespace = 0x0004,  // Lines with whitespace at end
    err_cr_lf_line_endings = 0x0008,   // Windows "\r\n" line endings
    err_no_lf_at_end = 0x0010,         // No '\n' at end of file
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

    //  An arbitrary character that isn't regarded as whitespace
    constexpr int not_a_space = 0;

    int current = not_a_space;
    int penultimate = not_a_space;
    int antepenultimate = not_a_space;
    int last_character = not_a_space;

    //  The last character should be a line feed.
    if (size > 0 && data[size - 1] != '\n')
    {
        errors |= err_no_lf_at_end;
    }

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
                errors |= err_tabs;
                break;

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


//  Trim trailing spaces from a line of text
void trim(std::string& text)
{
    while (!text.empty() && text.back() == ' ')
    {
        text.resize(text.size() - 1);
    }
}


void append_tab_as_spaces(std::string& text, int tab_width)
{
    int length = int(text.size());
    int tabbed_length = tab_width * ((length + tab_width - 1) / tab_width);
    int count = tabbed_length - length;
    text.append(count, ' ');
}

bool rename_file(const std::string& old_name, const std::string& new_name)
{
    int err = std::rename(old_name.c_str(), new_name.c_str());
    if (!err)
    {
        return true;
    }

    std::string msg = "Could not rename ";
    msg += old_name;
    msg += " to ";
    msg += new_name;

    std::perror(msg.c_str());

    return false;
}

}  //  namespace


void Normalizer::normalize(const char* path, bool fix)
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

    if (fix && is_fixable(m_errors))
    {
        if (fix_the_file(4))
        {
            //  Rename the original to backup
            std::string backup = m_full_name + ".bak~";
            std::remove(backup.c_str());
            if (rename_file(m_full_name, backup))
            {
                //  Rename the temporary to the original
                rename_file(m_temp_name, m_full_name);
            }
        }
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

    m_data.reserve(input.tellg());

    input.seekg(0);
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
        add_error_message("invalid characters");
    }

    if (m_errors & err_tabs)
    {
        add_error_message("tabs");
    }

    if (m_errors & err_unusual_whitespace)
    {
        add_error_message("unusual whitespace");
    }

    if (m_errors & err_trailing_whitespace)
    {
        add_error_message("trailing whitespace");
    }

    if (m_errors & err_cr_lf_line_endings)
    {
        add_error_message("CR-LF line endings");
    }

    if (m_errors & err_no_lf_at_end)
    {
        add_error_message("no line feed at end");
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

    //  Files coming from Windows may have UTF-16 encoding.
    Utf16Checker utf16;
    int err = utf16.check(m_data.data(), size);
    if (err == Utf16Checker::eOK)
    {
        auto& counts = utf16.counts();

        //  Don't accept any weird characters in the ASCII range, and
        //  allow only about 5% non-ascii characters.
        int non_ascii = counts.total_characters - counts.normal_ascii;
        if (counts.weird_ascii == 0 && 20*non_ascii < counts.total_characters)
        {
            return eUTF16;
        }
    }

    int normal = 0;
    int weird = 0;
    for (int ix = 0; ix < size; ++ix)
    {
        int ch = m_data[ix];
        if (isprint(ch) || isspace(ch))
            normal++;
        else
            weird++;
    }

    //  Plenty of weird characters would suggest a binary file
    if (5*weird > normal)
    {
        return eBINARY;
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


//  Fix the fixable issues
bool Normalizer::fix_the_file(int tab_width)
{
    //  Write fixed output to a temporary file
    m_temp_name = m_full_name;
    m_temp_name += ".tmp~";
    std::ofstream output(m_temp_name, std::ofstream::binary | std::ofstream::trunc);
    std::string line;

    const char* cursor = m_data.data();
    const char* end = cursor + data_size();
    while (cursor != end)
    {
        int ch = *cursor++;
        if (!std::isspace(ch))
        {
            line += char(ch);
            continue;
        }

        //  Found some sort of whitespace.
        //  Tab and line feed need special treatment, all the rest are
        //  replaced by ordinary old fashioned space characters ' '.
        switch (ch)
        {
        case '\t':  // tab
            append_tab_as_spaces(line, tab_width);
            break;

        case '\n':  // newline
            //  Remove trailing spaces if any, write the line out,
            //  and begin a new one.
            trim(line);
            output << line << '\n';
            line.clear();
            break;

        default:
            line += ' ';
            break;
        }
    }

    //  If the file didn't end with a line feed,
    //  there might be one last line still pending.
    trim(line);
    if (!line.empty())
    {
        output << line << '\n';
    }

    output.flush();
    return output.good();
}
