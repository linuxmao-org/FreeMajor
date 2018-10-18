//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <FL/Fl_Group.H>
#include <bitset>

class Matrix_Display : public Fl_Group {
public:
    Matrix_Display(int x, int y, int w, int h, const char *l = nullptr);
    void draw() override;

    void clear_matrix();
    void set_matrix(unsigned row, unsigned column, bool value);

private:
    enum { matrix_rows = 4, matrix_columns = 6 };
    void draw_matrix(int x, int y, int w, int h);
    std::bitset<matrix_rows * matrix_columns> enable_;
};
