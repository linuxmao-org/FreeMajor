//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "modifiers_editor.h"

Fl_Group *Modifiers_Editor::box_from_coords(int row, int column)
{
    if (row < 0 || column < 0 || row >= rows || column >= columns)
        return nullptr;

    Fl_Group *boxes[rows][columns] = {
        {mod_A1, mod_A2, mod_A3, mod_A4, mod_A5, mod_A6, mod_A7, mod_A8, mod_A9, mod_A10},
        {mod_B1, mod_B2, mod_B3, mod_B4, mod_B5, mod_B6, mod_B7, mod_B8, mod_B9, mod_B10},
        {mod_C1, mod_C2, mod_C3, mod_C4, mod_C5, mod_C6, mod_C7, mod_C8, mod_C9, mod_C10},
        {mod_D1, mod_D2, mod_D3, mod_D4, mod_D5, mod_D6, mod_D7, mod_D8, mod_D9, mod_D10},
        {mod_E1, mod_E2, mod_E3, mod_E4, mod_E5, mod_E6, mod_E7, mod_E8, mod_E9, mod_E10},
    };
    return boxes[row][column];
}

Fl_Box *Modifiers_Editor::label_for_row(int row)
{
    if (row < 0 || row >= rows)
        return nullptr;

    Fl_Box *labels[rows] = {
        lbl_A, lbl_B, lbl_C, lbl_D, lbl_E,
    };
    return labels[row];
}
