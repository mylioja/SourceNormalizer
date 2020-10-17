//  Command line options handling for source_normalizer
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

#include "options.h"

#include <getopt.h>
#include <cstring>
#include <iostream>

namespace {

// clang-format off
struct option long_options[] =
{
    {"extension", required_argument, 0, 'e'},
    {"fix", no_argument, 0, 'f'},
    {"help", no_argument, 0, 'h'},
    {"recursive", no_argument, 0, 'r'},
    {"skip", required_argument, 0, 's'},
    {"tabsize", required_argument, 0, 't'},
    {"verbose", no_argument, 0, 'v'},
    {"version", no_argument, 0, 'V'},
    {0, 0, 0, 0}
};
// clang-format on

const char short_options[] = "e:fhrs:t:vV";

const char usage_msg[] = "Usage: $(NAME) [option]... path [path]...\n";

const char help_msg[] =
    "Detect and optionally fix whitespace issues in source files.\n"
    "Example: $(NAME) -rv -s bin .\n\n"
    "Options:\n"
    "  -e, --extension=ext[,ext]... Extensions to be treated as source files\n"
    "  -f, --fix        Fix detected easily fixable errors\n"
    "  -h, --help       Display this help text and exit\n"
    "  -r, --recursive  Recurse to subdirectories\n"
    "  -s, --skip=name[,name]... Subdirectories to skip when recursing\n"
    "  -t, --tabsize=n  Set the tab size (default is 4)\n"
    "  -v, --verbose    Display lots of messages\n"
    "  -V, --version    Display program version and exit\n\n"
    "If no extensions were given, the following are assumed: c,cc,cpp,h,hpp\n"
    "When path is a directory, and also in recursive mode, only files with\n"
    "the chosen extensions are examined.\n"
    "If the path is a normal file, it'll be processed regardless of the extension.\n"
    "Without the '--fix' option, detected problems are reported but not fixed.\n"
    "Recursion always skips subdirectories with names having a leading period.\n";

const char copyright_msg[] =
    "$(COPYRIGHT)\n"
    "This program comes with ABSOLUTELY NO WARRANTY.\n"
    "This is free software, and you can redistribute it under the terms of\n"
    "GNU GPL version 3 license or later <http://gnu.org/licenses/gpl.html>.\n";


//  Find a key string within a text.
//  Returns a pointer to the end of the text if the key isn't found.
const char* find_str(const char* text, const char* key)
{
    const char* found = std::strstr(text, key);
    if (!found)
    {
        found = text + std::strlen(text);
    }

    return found;
}

//  Emit the value of a substitution variable.
//
//  WARNING: There's no error checking!
//
//  No check for a possibly missing end parenthesis ')'.
//  No check for not finding the substitution variable.
//  No check for not finding a terminating newline at the end of the value.
//
const char* emit_variable_value(std::ostream& os, const char* info, const char* key_begin)
{
    //  Variable names are delimited by '\n' and ": ", so adjust accordingly
    const char* key_end = find_str(key_begin, ")");
    std::string key(key_begin + 1, key_end);
    key[0] = '\n';
    key += ": ";

    const char* found = find_str(info, key.c_str());
    found += key.size();
    const char* end = find_str(found, "\n");
    os.write(found, end - found);

    return key_end + 1;
}

//  Emit a templated message
void emit_message(std::ostream& os, const char* info, const char* msg)
{
    const char* cursor = msg;
    while (*cursor)
    {
        //  The next substution variable or end of message
        const char* next = find_str(cursor, "$(");
        os.write(cursor, next - cursor);
        if (*next)
        {
            next = emit_variable_value(os, info, next);
        }

        cursor = next;
    }
}

void emit_version(std::ostream& os, const char* info)
{
    emit_message(os, info, "$(NAME) $(VERSION)\n");
    emit_message(os, info, copyright_msg);
}

void emit_help(std::ostream& os, const char* info)
{
    emit_message(os, info, usage_msg);
    emit_message(os, info, help_msg);
}

void emit_short_help(std::ostream& os, const char* info)
{
    emit_message(os, info, usage_msg);
    emit_message(os, info, "Try '$(NAME) --help' for more information.\n");
}


class SuboptionTokenizer
{
public:
    SuboptionTokenizer(const char* text) : m_text(text) { }

    std::string& next()
    {
        //  Skip leading delimiters (and empty items)
        constexpr const char* delimiters = ", ";
        m_text += strspn(m_text, delimiters);
        size_t size = strcspn(m_text, delimiters);
        m_token.assign(m_text, m_text+size);
        m_text += size;
        return m_token;
    }

private:
    const char* m_text;
    std::string m_token;
};


Options* the_options;

}  // namespace


const Options* Options::get()
{
    return the_options;
}


void Options::add_skip(const char* arg)
{
    SuboptionTokenizer tokenizer(arg);
    for (std::string& name = tokenizer.next(); !name.empty(); name = tokenizer.next())
    {
        m_skip.insert(name);
    }
}


void Options::add_extension(const char* arg)
{
    SuboptionTokenizer tokenizer(arg);
    for (std::string& ext = tokenizer.next(); !ext.empty(); ext = tokenizer.next())
    {
        if (ext[0] != '.')
        {
            ext.insert(ext.begin(), '.');
        }

        m_extensions.insert(ext);
    }
}


int Options::parse(int argc, char** argv, const char* info)
{
    the_options = this;

    int err = 0;
    int opt_index = 0;
    for (;;)
    {
        int ch = getopt_long(argc, argv, short_options, long_options, &opt_index);
        if (ch == -1)
            break;

        switch (ch)
        {
        case 'e':  // extension
            add_extension(optarg);
            break;

        case 'f':  // fix
            m_fix = true;
            break;

        case 'h':  // help
            emit_help(std::cout, info);
            return eDONE;

        case 'r':  // recursive
            m_recursive = true;
            break;

        case 's':  // skip
            add_skip(optarg);
            break;

        case 't':  // tabsize
            if (!set_tabsize(optarg))
            {
                ++err;
            }
            break;

        case 'v':  // verbose
            m_verbose = true;
            break;

        case 'V':  // version
            emit_version(std::cout, info);
            return eDONE;

        case '?':
            ++err;
            break;
        }
    }

    m_first_argument = optind;

    //  Emit a short usage message if there were errors, or
    //  if the program was called without any options or arguments.
    if (err || argc < 2)
    {
        emit_short_help(std::cerr, info);
        return eERROR;
    }

    //  If no extension given, use the default values
    if (m_extensions.empty())
    {
        add_extension("c,cc,cpp,h,hpp");
    }

    return eOK;
}


bool Options::set_tabsize(const char* arg)
{
    int size = std::atoi(arg);

    //  Small sanity check
    if (size < 1 || size > 100)
    {
        std::cerr << "Error: Strange tab size argument \"" << arg << "\"\n";
        return false;
    }

    m_tabsize = size;
    return true;
}


bool Options::should_be_skipped(const std::string& name) const
{
    //  Skip if the name begins with a '.'
    if (name[0] == '.')
    {
        return true;
    }

    //  Skip if it's one of the names given in skip options
    if (m_skip.count(name))
    {
        return true;
    }

    return false;
}


bool Options::is_source_extension(const std::string& extension) const
{
    return m_extensions.count(extension);
}
