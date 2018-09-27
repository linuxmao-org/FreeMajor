//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <FL/Fl_Double_Window.H>
#include <memory>
class Main_Component;

class Main_Window : public Fl_Double_Window {
public:
    Main_Window();
    ~Main_Window();

private:
    std::unique_ptr<Main_Component> component_;
};
