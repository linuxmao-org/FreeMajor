//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "patch.h"
#include <string.h>

Patch Patch::create_empty()
{
    Patch pat;
    memset(&pat.raw_data, 0, sizeof(pat.raw_data));
    const uint8_t head_data[] = { 0, 32, 31, 0, 72, 32 };
    memcpy(&pat.raw_data, head_data, sizeof(head_data));
    pat.name("New patch");
    return pat;
}

bool Patch::valid() const
{
    for (uint8_t byte : raw_data) {
        if (byte >= 128)
            return false;
    }
    return true;
}

uint8_t Patch::checksum() const
{
    unsigned sum = 0;
    for (unsigned i = 28; i < 612; ++i)
        sum = (sum + raw_data[i]) & 127;
    return sum;
}

std::string Patch::name() const
{
    const char *name_start = (const char *)&raw_data[8];
    const char *name_end = (const char *)&raw_data[8 + 20];

    while (name_end > name_start && (name_end[-1] == ' ' || name_end[-1] == '\0'))
        --name_end;

    return std::string(name_start, name_end);
}

void Patch::name(const char *name)
{
    memset(&raw_data[8], ' ', 20);
    memcpy(&raw_data[8], name, strnlen(name, 20));
}

unsigned Patch::patch_number() const
{
    uint8_t lsb = raw_data[6];
    uint8_t msb = raw_data[7];
    return (lsb > 100) ? (lsb - 100) : (lsb + 28);
}

void Patch::patch_number(unsigned nth)
{
    raw_data[6] = (nth < 27) ? (nth + 100) : (nth - 28);
    raw_data[7] = (nth > 27) ? 1 : 0;  // select user bank
}
