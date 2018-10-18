//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "matrix_display.h"
#include "app_i18n.h"
#include <FL/fl_draw.H>

Matrix_Display::Matrix_Display(int x, int y, int w, int h, const char *l)
    : Fl_Group(x, y, w, h, l)
{
}

void Matrix_Display::draw()
{
    Fl_Group::draw();

    int x = this->x(), y = this->y();
    int w = this->w(), h = this->h();

    x += 1; y += 1;
    w -= 2; h -= 2;

    fl_push_clip(x, y, w, h);

    fl_color(0x20, 0x4a, 0x87);
    fl_rectf(x, y, w, h);

    {
        int py = y;
        int ph = 16;
        y += ph;
        h -= ph;
        fl_font(FL_HELVETICA, 10);
        fl_color(0xcc, 0xe5, 0xff);
        fl_draw("MATRIX", x, py, w, ph, FL_ALIGN_CENTER|FL_ALIGN_TOP);
    }

    {
        int px = x;
        int pw = 24;
        x += pw;
        w -= pw;
        int padx = 1;
        int boxh = h / matrix_rows;
        px += padx;
        pw -= padx;
        for (int i = 0; i < matrix_rows; ++i) {
            int py = y + i * boxh + boxh / 2;
            fl_line(((i == 0) ? px : (px + pw / 2)), py,
                    px + pw - 2 * padx, py);
        }
        fl_font(FL_HELVETICA, 9);
        fl_color(0xcc, 0xe5, 0xff);
        fl_draw("IN", px, y + boxh / 2, pw, 0, FL_ALIGN_CENTER|FL_ALIGN_BOTTOM);
        fl_line(px + pw / 2, y + boxh / 2,
                px + pw / 2, y + (matrix_rows - 1) * boxh + boxh / 2);
    }

    {
        int pw = 24;
        int px = x + w - pw;
        w -= pw;
        int padx = 1;
        int boxh = h / matrix_rows;
        px += padx;
        pw -= padx;
        for (int i = 0; i < matrix_rows; ++i) {
            int py = y + i * boxh + boxh / 2;
            fl_line(px, py,
                    ((i == 0) ? (px + pw) : (px + pw / 2)), py);
        }
        fl_font(FL_HELVETICA, 9);
        fl_color(0xcc, 0xe5, 0xff);
        fl_draw("OUT", px, y + boxh / 2, pw, 0, FL_ALIGN_CENTER|FL_ALIGN_BOTTOM);
        fl_line(px + pw / 2, y + boxh / 2,
                px + pw / 2, y + (matrix_rows - 1) * boxh + boxh / 2);
    }

    draw_matrix(x, y, w, h);

    fl_pop_clip();
}

void Matrix_Display::draw_matrix(int x, int y, int w, int h)
{
    int boxw = w / matrix_columns;
    int boxh = h / matrix_rows;
    int offx = (w - matrix_columns * boxw) / 2;
    int offy = (h - matrix_rows * boxh) / 2;
    int padx = 1;
    int pady = 1;

    for (int row = 0; row < matrix_rows; ++row) {
        for (int column = 0; column < matrix_columns; ++column) {
            int boxx = x + offx + boxw * column;
            int boxy = y + offy + boxh * row;

            size_t index = row * matrix_columns + column;
            if (enable_[index])
                fl_color(0xff, 0x00, 0x00);
            else
                fl_color(0x99, 0x00, 0x00);
            fl_rectf(boxx + padx, boxy + pady, boxw - 2 * padx, boxh - 2 * pady);
        }
    }
}

void Matrix_Display::clear_matrix()
{
    enable_.reset();
    redraw();
}

void Matrix_Display::set_matrix(unsigned row, unsigned column, bool value)
{
    size_t index = row * matrix_columns + column;

    if (enable_[index] == value)
        return;

    enable_[index] = value;
    redraw();
}
