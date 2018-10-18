//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#if ENABLE_NLS
#include <gettext.h>
#define _(x) ((const char *)gettext(x))
#define _P(c, x) ((const char *)pgettext(c, x))
#else
#define _(x) x
#define _P(c, x) x
#endif

// Locale
#include <locale.h>
#if defined(__APPLE__)
#include <xlocale.h>
#endif

#if defined(_WIN32)
typedef _locale_t locale_t;
inline void freelocale(locale_t x) { _free_locale(x); }
inline locale_t createlocale(int cat, const char *loc) { return _create_locale(cat, loc); }
#else
inline locale_t createlocale(int cat, const char *loc) { return newlocale(cat, loc, nullptr); }
#endif

// Locale RAII
#include <memory>
#include <type_traits>

struct Locale_Deleter {
    void operator()(locale_t x) const { freelocale(x); }
};
typedef std::unique_ptr<std::remove_pointer<locale_t>::type, Locale_Deleter> locale_u;

// Locale-independent formatting
#include <stdio.h>
#include <stdarg.h>


#if !defined(_WIN32)
int vsscanf_l(const char *str, const char *format, locale_t locale, va_list ap);
int sscanf_l(const char *str, const char *format, locale_t locale, ...);
#else
#define vsscanf_l _vsscanf_l
#define sscanf_l _sscanf_l
#endif

int vsscanf_lc(const char *str, const char *format, const char *locale, va_list ap);
int sscanf_lc(const char *str, const char *format, const char *locale, ...);
