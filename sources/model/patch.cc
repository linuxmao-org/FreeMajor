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
    unsigned lsb = raw_data[6];
    unsigned msb = raw_data[7];
    // fprintf(stderr, "Read M:%u L:%u\n", msb, lsb);
    unsigned value = ((msb & 1) << 7) | (lsb & 127);
    unsigned nth = (int)value - 101;
    nth = ((int)nth < 0) ? 0 : nth;
    nth = (nth > 99) ? 99 : nth;
    return nth;
}

void Patch::patch_number(unsigned nth)
{
    nth = (nth > 99) ? 99 : nth;
    unsigned value = nth + 101;
    raw_data[6] = value & 127;
    raw_data[7] = (value & 128) >> 7;
    // fprintf(stderr, "Write M:%u L:%u\n", raw_data[7], raw_data[6]);
}
