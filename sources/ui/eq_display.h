//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <FL/Fl_Group.H>
#include <vector>

class Eq_Display : public Fl_Group {
public:
    Eq_Display(int x, int y, int w, int h, const char *l = nullptr);
    void draw() override;

    struct Band;
    void set_bands(const Band bands[], unsigned count);

    struct Band {
        double freq = 0;
        double gain = 0;
        double width = 0;
    };

private:
    std::vector<Band> bands_;
    std::vector<double> plotdata_;
    void create_plotdata(unsigned size);
    double eval(unsigned band, double freq);
};
