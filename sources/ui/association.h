//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <list>
class Parameter_Access;
class Fl_Group;
class Fl_Widget;
class Patch;

enum Association_Kind {
    Assoc_Undefined,
    Assoc_Dial,
    Assoc_Check,
    Assoc_Choice,
    Assoc_Slider,
};

enum Association_Flag {
    Assoc_Refresh_Full =   1 << 0,
};

struct Association {
    Parameter_Access *access = nullptr;
    Fl_Group *group_box = nullptr;
    Fl_Widget *value_widget = nullptr;
    Association_Kind kind = Assoc_Undefined;
    int flags = 0;
    std::list<Fl_Widget *> value_labels;
    std::list<Fl_Widget *> name_labels;
    void update_value(const Patch &pat);
    void update_from_widget(Patch &pat);
};
