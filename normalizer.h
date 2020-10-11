/*
    File contents scanner for source_normalizer

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

#include <experimental/filesystem>
#include <string>
#include <vector>

class Normalizer
{
public:
    void normalize(const char* path, bool fix);

private:
    using Buffer = std::vector<char>;

    bool load_file(const char* path);

    //  Returns false if no errors found
    bool find_errors();

    int data_size() const { return int(m_data.size()); }

    //  If invalid characters, try to figure out why
    enum { eDONT_KNOW, eBINARY, eUTF16 };
    int classify_invalid() const;

    void add_error_message(const char* text);

    //  Returns true if all OK
    bool make_a_backup() const;

    void fix_the_file(int tab_width) const;

    unsigned m_errors;
    std::string m_full_name;
    std::string m_error_message;
    Buffer m_data;
};
