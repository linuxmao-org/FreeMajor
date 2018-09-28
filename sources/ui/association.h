//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
class Parameter_Access;
class Fl_Group;
class Fl_Widget;
class Patch;

enum Association_Kind {
    Assoc_Undefined,
    Assoc_Dial,
    Assoc_Check,
    Assoc_Choice,
};

enum Association_Flag {
    Assoc_Name_On_Box =    1 << 0,
    Assoc_Value_On_Label = 1 << 1,
    Assoc_Refresh_Full =   1 << 2,
};

struct Association {
    Parameter_Access *access = nullptr;
    Fl_Group *group_box = nullptr;
    Fl_Widget *value_widget = nullptr;
    Association_Kind kind = Assoc_Undefined;
    int flags = 0;
    void update_value(const Patch &pat);
    void update_from_widget(Patch &pat);
};
