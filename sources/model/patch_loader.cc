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

bool Patch_Loader::load_realmajor_patch(const uint8_t *data, size_t length, Patch &pat, const uint8_t **endp)
{
    const uint8_t *startp = (const uint8_t *)memchr(data, '[', length);
    if (!startp)
        return false;
    length -= startp - data + 1;
    data = startp + 1;

    const uint8_t *end = (const uint8_t *)memchr(data, ']', length);
    if (end)
        length = end - data;
    if (endp)
        *endp = end + 1;

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
    return load_sysex_patch(sysex.data(), sysex.size(), pat, nullptr, validate_checksum);
}

bool Patch_Loader::load_sysex_patch(const uint8_t *data, size_t length, Patch &pat, const uint8_t **endp, bool validate_checksum)
{
    const uint8_t *startp = (const uint8_t *)memchr(data, 0xf0, length);
    if (!startp)
        return false;
    length -= startp - data;
    data = startp;

    const uint8_t *end = (const uint8_t *)memchr(data + 1, 0xf7, length - 1);
    if (!end)
        return false;
    if (endp)
        *endp = end + 1;
    length = end - data;

    if (length < 614)
        return false;

    Patch tmp;
    memcpy(tmp.raw_data, data + 1, 612);

    bool checksum_ok = data[613] == tmp.checksum();
    // fprintf(stderr, "Checksum %s\n", checksum_ok ? "good" : "bad");

    if (validate_checksum && !checksum_ok)
        return false;

    pat = tmp;
    return true;
}

bool Patch_Loader::load_realmajor_bank(const uint8_t *data, size_t length, Patch_Bank &pbank)
{
    const uint8_t *curp = data;
    Patch_Bank pbank_tmp;
    Patch pat_tmp;
    size_t count = 0;

    while (load_realmajor_patch(curp, data + length - curp, pat_tmp, &curp)) {
        unsigned patchno = pat_tmp.patch_number();
        count += !pbank_tmp.used[patchno];
        pbank_tmp.slot[patchno] = pat_tmp;
        pbank_tmp.used[patchno] = true;
    }

    if (count == 0)
        return false;

    pbank = pbank_tmp;
    return true;
}

bool Patch_Loader::load_sysex_bank(const uint8_t *data, size_t length, Patch_Bank &pbank, bool validate_checksum)
{
    const uint8_t *curp = data;
    Patch_Bank pbank_tmp;
    Patch pat_tmp;
    size_t count = 0;

    while (load_sysex_patch(curp, data + length - curp, pat_tmp, &curp, validate_checksum)) {
        unsigned patchno = pat_tmp.patch_number();
        count += !pbank_tmp.used[patchno];
        pbank_tmp.slot[patchno] = pat_tmp;
        pbank_tmp.used[patchno] = true;
    }

    if (count == 0)
        return false;

    pbank = pbank_tmp;
    return true;
}
