//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "main_window.h"
#include "main_component.h"

Main_Window::Main_Window()
    : Fl_Double_Window(1000, 840)
{
    component_.reset(new Main_Component(0, 0, w(), h()));
}

Main_Window::~Main_Window()
{
}
