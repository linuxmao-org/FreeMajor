//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <string>
#include <vector>
#include <memory>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

// stream RAII
struct FILE_Deleter {
    void operator()(FILE *x) const { fclose(x); }
};
typedef std::unique_ptr<FILE, FILE_Deleter> FILE_u;

// prefix/suffix check
bool starts_with(const char *x, const char *s);
bool ends_with(const char *x, const char *s);

// whole file I/O
bool read_entire_file(FILE *fh, size_t max_size, std::vector<uint8_t> &data);

// file names
std::string file_name_extension(const std::string &fn);
std::string file_name_without_extension(const std::string &fn);
