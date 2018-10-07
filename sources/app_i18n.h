//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#if defined(ENABLE_GETTEXT)
#include <libintl.h>
#define _(x) ((const char *)gettext(x))
#else
#define _(x) x
#endif
