//  UTF-16 encoding check for source_normalizer
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

#include "utf16checker.h"

namespace {

constexpr int ascii_del_character = 0x7f;
constexpr int min_high_surrogate = 0xD800;
constexpr int min_low_surrogate = 0xDC00;
constexpr int max_low_surrogate = 0xDFFF;

static const int END_SENTINEL = -1;

//  Code unit types
enum {
    UNICODE_CHARACTER,
    HIGH_SURROGATE,
    LOW_SURROGATE,
};

//  Determine the type of a code unit
int unit_type(int code)
{
    if (code < min_high_surrogate)
    {
        //  from 0x0000 to 0xD7FF
        return UNICODE_CHARACTER;
    }

    if (code < min_low_surrogate)
    {
        //  from 0xD800 to 0xDBFF
        return HIGH_SURROGATE;
    }

    if (code <= max_low_surrogate)
    {
        //  from 0xDC00 to 0xDFFF
        return LOW_SURROGATE;
    }

    //  from 0xE000 to 0xFFFF
    return UNICODE_CHARACTER;
}


bool is_normal_ascii(int code)
{
    //  Reject if the code is outside reasonable range
    if (code > '~' || code < '\t')
    {
        return false;
    }

    //  Usual printable characters are fine
    if (code >= ' ')
    {
        return true;
    }

    //  All whitespace is fine, even the vertical tab and form feed.
    //  Note that values less than '\t' have already been rejected.
    if (code <= '\r')
    {
        return true;
    }

    return false;
}


//  Get a little endian code unit
int get_le_unit(const unsigned char* ptr)
{
    return ptr[0] | (ptr[1] << 8);
}


//  Get a big endian code unit
int get_be_unit(const unsigned char* ptr)
{
    return (ptr[0] << 8) | ptr[1];
}

}  // namespace


int Utf16Checker::check(const char* data, int size)
{
    //  Error if size is too small or not even
    if (size < 2 || (size & 0x0001))
    {
        return eSIZE;
    }

    m_data = reinterpret_cast<const unsigned char*>(data);
    m_end = m_data + size;

    m_counts.normal_ascii = 0;
    m_counts.weird_ascii = 0;
    m_counts.total_characters = 0;

    determine_endiannes();

    int previous_type = UNICODE_CHARACTER;
    for (int unit = next_unit(); unit != END_SENTINEL; unit = next_unit())
    {
        int type = unit_type(unit);
        switch (type)
        {
        case UNICODE_CHARACTER:
            //  A lonely surrogate isn't allowed, so check
            //  if there was one just before.
            if (previous_type != UNICODE_CHARACTER)
            {
                return eINVALID;
            }

            if (unit <= ascii_del_character)
            {
                //  If ASCII, count it as either normal or weird
                if (is_normal_ascii(unit))
                    ++m_counts.normal_ascii;
                else
                    ++m_counts.weird_ascii;
            }

            ++m_counts.total_characters;
            break;

        case HIGH_SURROGATE:
            //  Can't have two high surrogates in a row
            if (previous_type == HIGH_SURROGATE)
            {
                return eINVALID;
            }
            break;

        case LOW_SURROGATE:
            //  A low surrogate is valid only after a high surrogate
            if (previous_type != HIGH_SURROGATE)
            {
                return eINVALID;
            }

            //  Count the surrogate pair as one character, and accordingly
            //  change the current type to UNICODE_CHARACTER.
            //  All possible encoded values are technically valid, so there's
            //  no need to decode or examine the actual code point.
            ++m_counts.total_characters;
            type = UNICODE_CHARACTER;
            break;
        }

        previous_type = type;
    }

    //  Error if the last code unit was a lonely surrogate
    if (previous_type != UNICODE_CHARACTER)
    {
        return eINVALID;
    }

    return eOK;
}


void Utf16Checker::determine_endiannes()
{
    //  UTF-16 encoded texts tend to come from Windows,
    //  so usually they are little endian.
    m_little_endian = true;

    //  Might begin with a Byte Order Mark (BOM)
    if (m_data[0] == 0xff && m_data[1] == 0xfe)
    {
        //  Data is little endian.
        m_data += 2;  // Skip the BOM
        return;
    }

    if (m_data[0] == 0xfe && m_data[1] == 0xff)
    {
        //  Data is big endian.
        m_little_endian = false;
        m_data += 2;  // Skip the BOM
        return;
    }

    //  Examine some text and choose the endianness
    //  that produces more ASCII characters.
    constexpr int MAX_UNITS_TO_EXAMINE = 1000;

    int le = 0;
    int be = 0;
    const unsigned char* ptr = m_data;
    const unsigned char* end = ptr + 2 * MAX_UNITS_TO_EXAMINE;
    if (end > m_end)
    {
        end = m_end;
    }

    while (ptr != end)
    {
        le += is_normal_ascii(get_le_unit(ptr));
        be += is_normal_ascii(get_be_unit(ptr));
        ptr += 2;
    }

    //  Change the endianness if big endian looks better
    if (be > le)
    {
        m_little_endian = false;
    }
}


//  Returns END_SENTINEL if no input available.
//
int Utf16Checker::next_unit()
{
    int unit = END_SENTINEL;
    if (m_data != m_end)
    {
        if (m_little_endian)
            unit = get_le_unit(m_data);
        else
            unit = get_be_unit(m_data);

        m_data += 2;
    }

    return unit;
}
