//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "eq_display.h"
#include <FL/fl_draw.H>
#include <algorithm>
#include <math.h>

Eq_Display::Eq_Display(int x, int y, int w, int h, const char *l)
    : Fl_Group(x, y, w, h, l)
{
}

void Eq_Display::draw()
{
    Fl_Group::draw();

    int x = this->x(), y = this->y();
    int w = this->w(), h = this->h();

    x += 1; y += 1;
    w -= 2; h -= 2;

    if (w <= 0)
        return;

    create_plotdata(w);

    fl_push_clip(x, y, w, h);

    fl_color(0x00, 0x00, 0x00);
    fl_rectf(x, y, w, h);

    fl_color(0xff, 0x00, 0x00);
    const double *plotdata = plotdata_.data();

    double py = (y + h / 2) - lround(0.5 * plotdata[0] * h);
    for (unsigned i = 1; i < (unsigned)w; ++i) {
        double oldpy = py;
        py = (y + h / 2) - lround(0.5 * plotdata[i] * h);
        fl_line(x + i - 1, lround(oldpy), x + i, lround(py));
    }

    fl_pop_clip();
}

void Eq_Display::set_bands(const Band bands[], unsigned count)
{
    bands_.assign(bands, bands + count);
    plotdata_.clear();
    redraw();
}

static double gauss(double x, double s)
{
    auto sqr = [](double x) -> double { return x * x; };
    return (1.0 / (s * sqrt(2 * M_PI))) * exp(-0.5 * sqr(x / s));
}

static double norm_gauss(double x, double s)
{
    return gauss(x, s) / gauss(0, s);
}

static constexpr double sigma = 0.85;

void Eq_Display::create_plotdata(unsigned size)
{
    std::vector<double> &plotdata = plotdata_;
    plotdata.resize(size);

    const Band *bands = bands_.data();
    unsigned num_bands = bands_.size();

    auto to_frequency =
        [](double r) -> double {
            const double lx1 = log10(10.0), lx2 = log10(20000.0);
            return pow(10.0, lx1 + r * (lx2 - lx1));
        };

    for (unsigned i = 0; i < size; ++i)
        plotdata[i] = 1.0;

    for (unsigned i_band = 0; i_band < num_bands; ++i_band) {
        if (bands[i_band].freq < 0.0)
            continue;

        for (unsigned i = 0; i < size; ++i) {
            double f = to_frequency((double)i / (size - 1));
            double v = eval(i_band, f);
            plotdata[i] *= v;
        }
    }

    for (unsigned i = 0; i < size; ++i) {
        double db = 20.0 * log10(plotdata[i]);
        plotdata[i] = db * (1.0 / 12.0);
    }
}

double Eq_Display::eval(unsigned band, double freq)
{
    if (band >= bands_.size())
        return 1.0;

    double fcutoff = bands_[band].freq;
    if (fcutoff < 0.0)
        return 1.0;

    double gain = bands_[band].gain;
    double width = bands_[band].width;

    double fc2 = fcutoff * exp2(+width);
    double fc1 = fcutoff - (fc2 - fcutoff);

    double x = 2.0 * (freq - fc1) / (fc2 - fc1) - 1.0;
    double g = norm_gauss(x, sigma);

    double gaindb = 20.0 * log10(gain);
    double resultdb = std::abs(gaindb) * g;
    resultdb = (gaindb < 0) ? -resultdb : +resultdb;

    return pow(10.0, 0.05 * resultdb);
}
