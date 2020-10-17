//  File and directory scanning for source_normalizer
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
#include "options.h"

#include <filesystem>
#include <iostream>
#include <regex>
#include <set>
#include <string>

namespace fs = std::filesystem;

namespace {

//  Extensions accepted as source files
const auto cpp_regex =
    std::regex("\\.(h|hpp|c|cc|cpp)", std::regex::optimize | std::regex::extended);


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
    if (Options::get()->skip().count(name))
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

    const Options* opts = Options::get();
    bool fix = opts->fix();
    bool verbose = opts->verbose();
    bool recursive = opts->recursive();
    int tabsize = opts->tabsize();

    auto iter = fs::recursive_directory_iterator(path);
    const auto end = fs::recursive_directory_iterator();
    for (; iter != end; ++iter)
    {
        const auto& entry = *iter;
        if (entry.is_directory())
        {
            const char* prefix = "enter ";
            if (!recursive || should_be_skipped(entry.path()))
            {
                iter.disable_recursion_pending();
                prefix = "skip ";
            }

            if (verbose)
            {
                std::cout << prefix << entry.path() << '\n';
            }

            continue;
        }

        if (entry.is_regular_file())
        {
            bool select = has_required_extension(entry.path());
            if (verbose)
            {
                std::cout << (select ? "examine " : "skip ") << entry.path() << '\n';
            }

            if (select)
            {
                std::string full_name = entry.path().string();
                normalizer.normalize(full_name.c_str(), tabsize, fix);
            }
        }
    }
}


void process_file(fs::path& path)
{
    Normalizer normalizer;
    if (Options::get()->verbose())
    {
        std::cout << "examine " << path << '\n';
    }

    const Options* opts = Options::get();
    std::string full_name = path.string();
    normalizer.normalize(full_name.c_str(), opts->tabsize(), opts->fix());
}

}  // namespace


namespace FileScanner {


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
    catch (const fs::filesystem_error& err)
    {
        std::cerr << err.what() << '\n';
        return false;
    }

    return true;
}

}
