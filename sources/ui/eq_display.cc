//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "eq_display.h"
#include <FL/fl_draw.H>
#include <algorithm>
#include <math.h>
#include <stdio.h>

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

    fl_color(0x20, 0x4a, 0x87);
    fl_rectf(x, y, w, h);

    fl_color(0x80, 0x80, 0x80);
    fl_line(x, y + h / 2, x + w - 1, y + h / 2);

    for (int f : {10, 100, 1000, 10000}) {
        unsigned xx = x + freq2coord(f, (unsigned)w);
        fl_line(xx, y, xx, y + h - 1);
    }

    fl_font(FL_HELVETICA, 10);
    fl_color(0x00, 0xff, 0x00);

    for (int f : {10, 100, 1000, 10000}) {
        unsigned xx = x + freq2coord(f, (unsigned)w);
        char text[32];
        if (f < 1000)
            sprintf(text, "%d", f);
        else
            sprintf(text, "%dk", f / 1000);
        fl_draw(text, xx, y, 0, 0, FL_ALIGN_LEFT|FL_ALIGN_TOP);
        // fl_draw(text, xx, y + h - 1, 0, 0, FL_ALIGN_LEFT|FL_ALIGN_BOTTOM);
    }

    if (enable_) {
        fl_color(0xff, 0x00, 0x00);
        const double *plotdata = plotdata_.data();

        double py = (y + h / 2) - lround(0.5 * plotdata[0] * h);
        for (unsigned i = 1; i < (unsigned)w; ++i) {
            double oldpy = py;
            py = (y + h / 2) - lround(0.5 * plotdata[i] * h);
            fl_line(x + i - 1, lround(oldpy), x + i, lround(py));
        }
    }

    fl_pop_clip();
}

void Eq_Display::set_bands(bool enable, const Band bands[], unsigned count)
{
    enable_ = enable;
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

    for (unsigned i = 0; i < size; ++i)
        plotdata[i] = 1.0;

    for (unsigned i_band = 0; i_band < num_bands; ++i_band) {
        if (bands[i_band].freq < 0.0)
            continue;

        for (unsigned i = 0; i < size; ++i) {
            double f = coord2freq(i, size);
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

    double fc2 = fcutoff * exp2(0.5 * width);
    double fc1 = fcutoff - (fc2 - fcutoff);

    double x = 2.0 * (freq - fc1) / (fc2 - fc1) - 1.0;
    double g = norm_gauss(x, sigma);

    double gaindb = 20.0 * log10(gain);
    double resultdb = gaindb * g;

    return pow(10.0, 0.05 * resultdb);
}

static const double lx1 = log10(10.0);
static const double lx2 = log10(22000.0);

double Eq_Display::coord2freq(unsigned i, unsigned n)
{
    double r = (double)i / (n - 1);
    return pow(10.0, lx1 + r * (lx2 - lx1));
}

unsigned Eq_Display::freq2coord(double f, unsigned n)
{
    return (n - 1) * (log10(f) - lx1) / (lx2 - lx1);
}
