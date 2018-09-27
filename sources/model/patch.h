//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <string>
#include <bitset>
#include <stdint.h>

class Patch {
public:
    uint8_t raw_data[612];
    static Patch create_empty();
    bool valid() const;
    uint8_t checksum() const;

    std::string name() const;
    void name(const char *name);

    unsigned patch_number() const;
    void patch_number(unsigned nth);
};

class Patch_Bank {
public:
    enum { max_count = 100 };

    Patch slot[max_count];
    std::bitset<max_count> used;
};
