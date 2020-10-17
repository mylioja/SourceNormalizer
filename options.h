/*
    Command line options handling for source_normalizer

    Copyright (C) 2020  Martti Ylioja

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <set>
#include <string>

//  Command line options
class Options
{
public:
    bool fix() const { return m_fix; }
    bool verbose() const { return m_verbose; }
    bool recursive() const { return m_recursive; }
    int tabsize() const { return m_tabsize; }

    //  Directory names to be skipped when recursing
    //  For example: "bin", "build", etc...
    std::set<std::string> skip() const { return m_skip; }

    //  Index of the first argument after all options were parsed
    int first_argument() const { return m_first_argument; }

    //  Global read only access to the options
    static const Options* get();

private:
    //  Possible return values from parse.
    enum {
        eOK,     // No problems - continue normally
        eDONE,   // All done - exit normally (had --help or --version option)
        eERROR,  // Errors detected and reported - exit with an error code
    };
    int parse(int argc, char** argv, const char* info);

    //  Add a directory name to the list of names to be skipped
    void add_skip(const char* arg);

    bool set_tabsize(const char* arg);

    //  Only main can set the options
    friend int main(int argc, char** argv);

    bool m_fix = false;  // Try to fix errors
    bool m_verbose = false;
    bool m_recursive = false;

    int m_tabsize = 4;

    //  Directory names to be skipped when recursing
    //  For example: "bin", "build", etc...
    std::set<std::string> m_skip;

    //  Index of the first argument after all options were parsed
    int m_first_argument = 0;

};
