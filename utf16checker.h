/*
    UTF-16 encoding check for source_normalizer

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


class Utf16Checker
{
public:

    //  Possible return values
    enum {
        eOK,       // looks like valid UTF-16
        eSIZE,     // size is negative, zero, or odd
        eINVALID,  // invalid encoding
    };

    //  Size means number of bytes in the data buffer
    int check(const char* data, int size);

    class Counts
    {
    public:
        int normal_ascii;
        int weird_ascii;
        int total_characters;
    };

    const Counts& counts() const { return m_counts; }

private:
    //  Try to find out endianness of the data
    void determine_endiannes();

    //  Get the next 16-bit code unit.
    int next_unit();

    const unsigned char* m_data;
    const unsigned char* m_end;
    bool m_little_endian;

    //  Character classification results
    Counts m_counts;
};

