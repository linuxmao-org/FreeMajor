//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "association.h"
#include "model/parameter.h"
#include <FL/Fl_Group.H>
#include <FL/Fl_Dial.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Slider.H>
#include <math.h>

void Association::update_value(const Patch &pat)
{
    int pv = access->get(pat);

    switch (kind) {
    case Assoc_Dial:
        if (Fl_Dial *dl = static_cast<Fl_Dial *>(value_widget))
            dl->value(pv);
        break;
    case Assoc_Check:
        if (Fl_Check_Button *chk = static_cast<Fl_Check_Button *>(value_widget))
            chk->value(pv);
        break;
    case Assoc_Choice:
        if (Fl_Choice *cb = static_cast<Fl_Choice *>(value_widget))
            cb->value(pv);
        break;
    case Assoc_Slider:
        if (Fl_Slider *sl = static_cast<Fl_Slider *>(value_widget))
            sl->value(pv);
        break;
    default:
        break;
    }

    if (group_box && (flags & Assoc_Name_On_Box))
        group_box->copy_label(access->name);
    if (value_widget && (flags & Assoc_Value_On_Label))
        value_widget->copy_label(access->to_string(pv).c_str());
    if(value_update_callback)
        value_update_callback(pv);
}

void Association::update_from_widget(Patch &pat)
{
    if (!value_widget)
        return;

    int new_value;

    switch (kind) {
    case Assoc_Dial: {
        Fl_Dial *dl = static_cast<Fl_Dial *>(value_widget);
        new_value = (int)lround(dl->value());
        break;
    }
    case Assoc_Check: {
        Fl_Check_Button *chk = static_cast<Fl_Check_Button *>(value_widget);
        new_value = chk->value();
        break;
    }
    case Assoc_Choice: {
        Fl_Choice *cb = static_cast<Fl_Choice *>(value_widget);
        new_value = cb->value();
        break;
    }
    case Assoc_Slider: {
        Fl_Slider *sl = static_cast<Fl_Slider *>(value_widget);
        new_value = (int)lround(sl->value());
        break;
    }
    default:
        return;
    }

    access->set(pat, new_value);
    update_value(pat);
}
