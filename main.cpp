//  The main program for source_normalizer
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

#include "file_scanner.h"
#include "options.h"

#include <iostream>
#include <string>

//  If build system doesn't provide a timestamp, use this instead
#ifndef BUILD_DATETIME
#define BUILD_DATETIME __DATE__ " " __TIME__
#endif

//  Release builds set these variables to indicate status of the workspace.
//  Set them to default values for builds that don't provide the info.
//
#ifndef GIT_REVISION
#define GIT_REVISION "unknown"
#endif

#ifndef GIT_STATUS
#define GIT_STATUS "unknown"
#endif

//  This string contains some basic info about the program.
//
//  The text is used by options parser for simple variable substitution,
//  but it should also look nice and informative in a core dump, or in
//  a hex dump of the executable binary image.
//
//  Variable names are delimited by line feed '\n' and ": ".
//  Any text after the ": " up to the next line feed '\n' is the value.
//
const char program_info[] =
    "\n"
    "NAME: source_normalizer\n"
    "VERSION: 1.4\n"
    "COPYRIGHT: Copyright (C) 2020 Martti Ylioja\n"
    "SPDX-License-Identifier: GPL-3.0-or-later\n"
    "BUILD_DATETIME: " BUILD_DATETIME "\n"
    "GIT_REVISION: " GIT_REVISION "\n"
    "GIT_STATUS: " GIT_STATUS "\n";


int main(int argc, char** argv)
{
    Options options;
    int err = options.parse(argc, argv, program_info);
    switch (err)
    {
    case Options::eDONE:
        return 0;  //  All done (was maybe --help or --version)

    case Options::eERROR:  // Error with the command line
        return 1;

    case Options::eOK:  // No errors
        break;
    }

    err = 0;
    for (int arg_ix = options.first_argument(); arg_ix < argc; ++arg_ix)
    {
        const char* arg = argv[arg_ix];
        bool ok = FileScanner::process(arg);

        //  Stop at the first serious error
        if (!ok)
        {
            err = 2;
            break;
        }
    }

    return err;
}
