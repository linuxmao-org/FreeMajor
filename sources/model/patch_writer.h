//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <vector>
#include <stdint.h>
class Patch;
class Patch_Bank;

class Patch_Writer {
public:
    static void save_realmajor_patch(const Patch &pat, std::vector<uint8_t> &data, bool append = false);
    static void save_sysex_patch(const Patch &pat, std::vector<uint8_t> &data, bool append = false);

    static void save_realmajor_bank(const Patch_Bank &pbank, std::vector<uint8_t> &data);
    static void save_sysex_bank(const Patch_Bank &pbank, std::vector<uint8_t> &data);
};
