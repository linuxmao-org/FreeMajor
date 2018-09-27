//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
class Parameter_Access;
class Fl_Group;
class Fl_Widget;

struct Association {
    Parameter_Access *access = nullptr;
    Fl_Group *group_box = nullptr;
    Fl_Widget *value_widget = nullptr;
};
