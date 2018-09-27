//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "ui/main_window.h"
#include <FL/Fl.H>

int main()
{
    Main_Window win;
    win.show();

    Fl::run();
    return 0;
}
