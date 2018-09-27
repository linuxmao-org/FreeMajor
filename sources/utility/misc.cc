//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "misc.h"

bool starts_with(const char *x, const char *s)
{
    size_t nx = strlen(x), ns = strlen(s);
    return nx >= ns && !memcmp(s, x, ns);
}

bool ends_with(const char *x, const char *s)
{
    size_t nx = strlen(x), ns = strlen(s);
    return nx >= ns && !memcmp(s, x + nx - ns, ns);
}

bool read_entire_file(FILE *fh, size_t max_size, std::vector<uint8_t> &data)
{
    struct stat st;
    if (fstat(fileno(fh), &st) != 0)
        return false;

    size_t size = st.st_size;
    if (size > max_size)
        return false;

    data.resize(size);
    rewind(fh);
    return fread(data.data(), 1, size, fh) == size;
}

std::string file_name_extension(const std::string &fn)
{
    size_t n = fn.size();
    for (size_t i = n; i-- > 0;) {
        char c = fn[i];
        if (c == '.')
            return fn.substr(i);
        else if (c == '/')
            break;
#if defined(_WIN32)
        else if (c == '\\')
            break;
#endif
    }
    return std::string();
}

std::string file_name_without_extension(const std::string &fn)
{
    std::string ext = file_name_extension(fn);
    return fn.substr(0, fn.size() - ext.size());
}
