//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "patch_loader.h"
#include "patch.h"
#include "utility/misc.h"
#include <FL/fl_utf8.h>
#include <string>
#include <string.h>
#include <stdio.h>

bool Patch_Loader::load_realmajor_patch(const uint8_t *data, size_t length, Patch &pat)
{
    const uint8_t *startp = (const uint8_t *)memchr(data, '[', length);
    if (!startp)
        return false;
    length -= startp - data + 1;
    data = startp + 1;

    if (const uint8_t *endp = (const uint8_t *)memchr(data, ']', length))
        length = endp - data;

    std::string input((const char *)data, length);
    std::vector<uint8_t> sysex;
    sysex.reserve(1024);

    for (size_t i = 0; i < length; ++i) {
        unsigned number;
        unsigned bytecount;
        if (sscanf(&input.c_str()[i], "%u%n", &number, &bytecount) == 1) {
            if (number >= 256)
                return false;
            sysex.push_back((uint8_t)number);
            i += bytecount - 1;
        }
    }

    bool validate_checksum = false;
    return load_sysex_patch(sysex.data(), sysex.size(), pat, validate_checksum);
}

bool Patch_Loader::load_sysex_patch(const uint8_t *data, size_t length, Patch &pat, bool validate_checksum)
{
    const uint8_t *startp = (const uint8_t *)memchr(data, 0xf0, length);
    if (!startp)
        return false;
    length -= startp - data;
    data = startp;

    const uint8_t *endp = (const uint8_t *)memchr(data + 1, 0xf7, length - 1);
    if (!endp)
        return false;
    length = endp - data;

    if (length < 614)
        return false;

    Patch tmp;
    memcpy(tmp.raw_data, data + 1, 612);

    if (validate_checksum) {
        if (data[613] != tmp.checksum())
            return false;
    }

    pat = tmp;
    return true;
}

bool Patch_Loader::load_patch_file(const char *path, Patch &pat)
{
    FILE_u fh(fl_fopen(path, "rb"));
    std::vector<uint8_t> data;

    if (!read_entire_file(fh.get(), 1 << 20, data))
        return false;

    if (ends_with(path, ".syx"))
        return load_sysex_patch(data.data(), data.size(), pat);

    return load_realmajor_patch(data.data(), data.size(), pat);
}
