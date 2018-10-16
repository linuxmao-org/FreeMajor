//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "app_i18n.h"
#include "ui/main_window.h"
#include <FL/Fl.H>

int main()
{
#if ENABLE_NLS
    setlocale(LC_ALL, "");
    bindtextdomain("gmajctl", LOCALE_DIRECTORY "/");
    textdomain("gmajctl");
#endif

    Main_Window win;
    win.show();

    Fl::run();
    return 0;
}
