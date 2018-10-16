//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "app_i18n.h"
#include <stdexcept>

#if !defined(_WIN32)
int vsscanf_l(const char *str, const char *format, locale_t locale, va_list ap)
{
    locale_t oldloc = uselocale(locale);
    if (!oldloc)
        throw std::runtime_error("cannot set the thread locale");
    int ret = vsscanf(str, format, ap);
    if (!uselocale(oldloc))
        throw std::runtime_error("cannot restore the thread locale");
    return ret;
}

int sscanf_l(const char *str, const char *format, locale_t locale, ...)
{
    va_list ap;
    va_start(ap, locale);
    int ret = vsscanf_l(str, format, locale, ap);
    va_end(ap);
    return ret;
}
#endif
