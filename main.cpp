#include "normalizer.h"

#include <filesystem>
#include <getopt.h>
#include <iostream>
#include <regex>
#include <set>
#include <string>

//  Program name and version
//  Currently as macros to enable compile time concatenation
//  into static const strings.
//
#define PROGRAM_NAME "source_normalizer"
#define PROGRAM_VERSION "1.1"

namespace fs = std::filesystem;

namespace {

    struct option long_options[] =
    {
        {"fix", no_argument, 0, 'f'},
        {"help", no_argument, 0, 'h'},
        {"recursive", no_argument, 0, 'r'},
        {"skip", required_argument, 0, 's'},
        {"verbose", no_argument, 0, 'v'},
        {"version", no_argument, 0, 'V'},
        {0, 0, 0, 0}
    };

    const char short_options[] = "fhrs:vV";

    const char usage_msg[] = "Usage: " PROGRAM_NAME " [option]... path [path]...\n";

    const char help_msg[] =
        "Detect and optionally fix whitespace issues in source files.\n"
        "Example: " PROGRAM_NAME " menu.h main.c\n\n"
        "Options:\n"
        "  -f, --fix        Fix detected easily fixable errors\n"
        "  -h, --help       Display this help text and exit\n"
        "  -r, --recursive  Recurse to subdirectories\n"
        "  -s, --skip       Skip the given subdirectory when recursing\n"
        "  -v, --verbose    Display lots of messages\n"
        "  -V, --version    Display program version and exit\n\n"
        "When path is a directory, and also in recursive mode, only files with\n"
        "extensions .c, .cc, .cpp, .h, and .hpp are examined.\n"
        "If the path is a normal file, it'll be processed regardless of the extension.\n"
        "Without '--fix' option the detected problems are only reported.\n"
        ;

    const char copyright_msg[] =
        "Copyright (C) 2020 Martti Ylioja.\n"
        "This program comes with ABSOLUTELY NO WARRANTY.\n"
        "This is free software, and you are welcome to redistribute it under the terms\n"
        "of GNU GPL version 3 license or later <http://gnu.org/licenses/gpl.html>.\n";

    void emit_version(std::ostream& os)
    {
        os << PROGRAM_NAME " " PROGRAM_VERSION "\n";
        os << copyright_msg;
    }

    void emit_help(std::ostream& os)
    {
        os << usage_msg << help_msg;
    }

    //  Command line options
    struct Options
    {
        bool fix;
        bool verbose;
        bool recursive;
        std::set<std::string> skip;
    };

    Options opts;

    void add_skip(const char* arg)
    {
        //  TODO: Accept a comma separated list of names
        opts.skip.emplace(arg);
    }

    //  Extensions to be accepted as source files
    const auto cpp_regex = std::regex(
        "\\.(h|hpp|c|cc|cpp)",
        std::regex::optimize | std::regex::extended
    );


    //  Return true if the path denotes an uninteresting directory
    //  that should be skipped.
    bool should_be_skipped(const fs::path& path)
    {
        std::string name = path.filename().string();

        //  Skip if the name begins with a '.'
        if (name[0] == '.')
        {
            return true;
        }

        //  Skip if one of the names given in skip options
        if (opts.skip.count(name))
        {
            return true;
        }

        return false;
    }


    bool has_required_extension(const fs::path& path)
    {
        std::string ext = path.extension().string();
        return std::regex_match(ext, cpp_regex);
    }


    void scan_and_process(fs::path& path)
    {
        Normalizer normalizer;
        auto iter = fs::recursive_directory_iterator(path);
        const auto end =  fs::recursive_directory_iterator();
        for (; iter != end; ++iter)
        {
            const auto& entry = *iter;
            if (entry.is_directory())
            {
                const char* prefix = "enter ";
                if (!opts.recursive || should_be_skipped(entry.path()))
                {
                    iter.disable_recursion_pending();
                    prefix = "skip ";
                }

                if (opts.verbose)
                {
                    std::cout << prefix << entry.path() << '\n';
                }

                continue;
            }

            if (entry.is_regular_file())
            {
                bool select = has_required_extension(entry.path());
                if (opts.verbose)
                {
                    std::cout << (select ? "examine " : "skip ") << entry.path() << '\n';
                }

                if (select)
                {
                    std::string full_name = entry.path().string();
                    normalizer.normalize(full_name.c_str());
                }
            }
        }
    }


    void process_file(fs::path& path)
    {
        Normalizer normalizer;
        if (opts.verbose)
        {
            std::cout << "examine " << path << '\n';
        }

        std::string full_name = path.string();
        normalizer.normalize(full_name.c_str());
    }


    bool process(const char* arg)
    {
        try
        {
            fs::path path = fs::canonical(arg);
            fs::directory_entry dir(path);
            if (dir.is_directory())
            {
                scan_and_process(path);
            }
            else if (dir.is_regular_file())
            {
                process_file(path);
            }
        }
        catch(const fs::filesystem_error& err)
        {
            std::cerr << err.what() << '\n';
            return false;
        }

        return true;
    }

}


int main(int argc, char** argv)
{
    //  Emit a short usage message if called without arguments
    if (argc < 2)
    {
        std::cout << usage_msg << "Try '" PROGRAM_NAME " --help' for more information.\n";
        return 0;
    }

    int err = 0;
    int opt_index = 0;
    for (;;)
    {
        int ch = getopt_long(argc, argv, short_options, long_options, &opt_index);
        if (ch == -1)
            break;

        switch (ch)
        {
            case 'f': // fix
                opts.fix = true;
                break;

            case 'h': // help
                emit_help(std::cout);
                return 0;

            case 'r': // recursive
                opts.recursive = true;
                break;

            case 's': // skip
                add_skip(optarg);
                break;

            case 'v': // verbose
                opts.verbose = true;
                break;

            case 'V': // version
                emit_version(std::cout);
                return 0;

            case '?':
                ++err;
                break;
        }
    }

    while (optind < argc)
    {
        bool ok = process(argv[optind]);

        //  Stop at first serious error
        if (!ok)
        {
            break;
        }

        ++optind;
    }

    return err;
}
