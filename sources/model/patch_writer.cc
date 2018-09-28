//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "patch_writer.h"
#include "patch.h"
#include "utility/misc.h"
#include <FL/fl_utf8.h>
#include <string.h>
#include <stdio.h>

void Patch_Writer::save_realmajor_patch(const Patch &pat, std::vector<uint8_t> &data)
{
    data.clear();

    const char *start = "[240";
    std::copy(start, start + 4, std::back_inserter(data));

    for (uint8_t byte : pat.raw_data) {
        char str[8];
        sprintf(str, ", %u", byte);
        std::copy(str, str + strlen(str), std::back_inserter(data));
    }
    for (unsigned byte : {(unsigned)pat.checksum(), 0xf7u, 0u}) {
        char str[8];
        sprintf(str, ", %u", byte);
        std::copy(str, str + strlen(str), std::back_inserter(data));
    }

    data.push_back(']');
}

void Patch_Writer::save_sysex_patch(const Patch &pat, std::vector<uint8_t> &data)
{
    data.clear();
    data.reserve(615);
    data.push_back(0xf0);
    for (uint8_t byte : pat.raw_data)
        data.push_back(byte);
    data.push_back(pat.checksum());
    data.push_back(0xf7);
}
