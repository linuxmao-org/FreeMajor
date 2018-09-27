//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "parameter.h"
#include "patch.h"
#include "app_i18n.h"

static int load_int(const uint8_t *src, unsigned size)
{
    int v = 0;
    bool negative = src[size - 1] == 7;
    for (int i = 0; i < size - 1; ++i) {
        int byte = src[i];
        if (negative)
            byte = (byte != 127) ? (byte -= 128) : 0;
        v += byte * (1 << (7 * i));
    }
    return v;
}

static void store_int(uint8_t *dst, unsigned size, int value)
{
    bool negative = value < 0;
    int absv = std::abs(value);
    dst[size - 1] = negative ? 7 : 0;

    for (int i = 0; i < size - 1; ++i) {
        int divisor = 1 << (7 * i);
        dst[i] = (absv >= divisor) ?
            (127 & (negative ? (128 - absv / divisor) : (absv / divisor))) :
            (negative ? 127 : 0);
    }
}

///

std::string Parameter_Access::to_string(int value) const
{
    return std::to_string(value);
}

PA_Integer *PA_Integer::with_min_max(int vmin, int vmax)
{
    this->vmin = vmin;
    this->vmax = vmax;
    return this;
}

int PA_Integer::get(const Patch &pat) const
{
    int v = load_int(&pat.raw_data[index], size);
    return clamp(v);
}

void PA_Integer::set(Patch &pat, int value)
{
    int v = clamp(value);
    store_int(&pat.raw_data[index], size, v);
}

int PA_Integer::clamp(int value) const
{
    value = (value < vmin) ? vmin : value;
    value = (value > vmax) ? vmax : value;
    return value;
}

PA_Boolean *PA_Boolean::with_inversion()
{
    inverted = true;
    return this;
}

int PA_Boolean::get(const Patch &pat) const
{
    bool v = load_int(&pat.raw_data[index], size) != 0;
    return v ^ inverted;
}

void PA_Boolean::set(Patch &pat, int value)
{
    store_int(&pat.raw_data[index], size, (bool)value ^ inverted);
}

std::string PA_Boolean::to_string(int value) const
{
    return value ? "on" : "off";
}

PA_Choice *PA_Choice::with_offset(int offset)
{
    this->offset = offset;
    return this;
}

int PA_Choice::get(const Patch &pat) const
{
    int v = load_int(&pat.raw_data[index], size);
    return clamp(v - offset);
}

void PA_Choice::set(Patch &pat, int value)
{
    int v = clamp(value + offset);
    store_int(&pat.raw_data[index], size, v);
}

int PA_Choice::clamp(int value) const
{
    int vmin = min(), vmax = max();
    value = (value < vmin) ? vmin : value;
    value = (value > vmax) ? vmax : value;
    return value;
}

std::string PA_Choice::to_string(int value) const
{
    return values[clamp(value)];
}

///

P_Compressor::P_Compressor()
{
    slots.emplace_back((new PA_Integer(164, 4, _("Threshold"), _("When the signal is above the set Threshold point the Compressor is activated and the gain of any signal above the Threshold point is processed according to the Ratio, Attack and Release.")))
                       ->with_min_max(-40, 40));
    slots.emplace_back((new PA_Choice(168, 4,
                                      {_("Off"), "1.12:1", "1.25:1", "1.40:1", "1.60:1", "1.80:1", "2.0:1", "2.5:1", "3.2:1", "4.0:1", "5.6:1", "8.0:1", "16:1", "32:1", "64:1", _("Inf:1")},
                                      _("Ratio"), _("The Ratio setting determines how hard the signal is compressed."))));
    // TODO check times for correct values
    slots.emplace_back((new PA_Choice(172, 4,
                                      {"1.0", "1.4", "2.0", "3.0", "5.0", "7.0", "10", "14", "20", "30", "50", "70"},
                                      _("Attack"), _("The Attack time is the response time of the Compressor. The shorter the attack time the sooner the Compressor will reach the specified Ratio after the signal rises above the Threshold.")))
                       ->with_offset(3));
    slots.emplace_back((new PA_Choice(176, 4,
                                      {"50", "70", "100", "140", "200", "300", "500", "700", "1000", "1400", "2000"},
                                      _("Release"), _("The Release time is the time it takes for the Compressor to release the gain reduction of the signal after the Input signal drops below the Threshold point again.")))
                       ->with_offset(3));
    slots.emplace_back((new PA_Integer(180, 4, _("Gain"), _("Use this Gain parameter to compensate for the level changes caused by the applied compression.")))
                       ->with_min_max(-6, 6));
}

P_General::P_General()
{
    slots.emplace_back((new PA_Boolean(224, 4, _("Enable"), _("Enable the compressor")))
                       ->with_inversion());
    slots.emplace_back((new PA_Boolean(288, 4, _("Enable"), _("Enable the filter")))
                       ->with_inversion());
    slots.emplace_back((new PA_Boolean(352, 4, _("Enable"), _("Enable the pitch")))
                       ->with_inversion());
    slots.emplace_back((new PA_Boolean(416, 4, _("Enable"), _("Enable the chorus/flanger")))
                       ->with_inversion());
    slots.emplace_back((new PA_Boolean(480, 4, _("Enable"), _("Enable the delay")))
                       ->with_inversion());
    slots.emplace_back((new PA_Boolean(544, 4, _("Enable"), _("Enable the reverb")))
                       ->with_inversion());
    slots.emplace_back((new PA_Boolean(564, 4, _("Enable"), _("Enable the equalizer"))));
    slots.emplace_back((new PA_Boolean(608, 4, _("Enable"), _("Enable the noise gate")))
                       ->with_inversion());

    slots.emplace_back((new PA_Choice(228, 4,
                                      {_("Auto filter"), _("Resonance filter"), _("Vintage phaser"), _("Smooth phaser"), _("Tremolo"), _("Phaser")},
                                      _("Filter type"), _("Type of filter effect"))));
    slots.emplace_back((new PA_Choice(292, 4,
                                      {_("Detune"), _("Whammy"), _("Octaver"), _("Pitch shifter")},
                                      _("Pitch type"), _("Type of pitch effect"))));
    slots.emplace_back((new PA_Choice(356, 4,
                                      {_("Classic chorus"), _("Advanced chorus"), _("Classic flanger"), _("Advanced flanger"), _("Vibrato")},
                                      _("Modulation type"), _("Type of modulation effect"))));
    slots.emplace_back((new PA_Choice(420, 4,
                                      {_("Ping Pong"), _("Dynamic"), _("Dual")},
                                      _("Delay type"), _("Type of delay effect"))));
    slots.emplace_back((new PA_Choice(484, 4,
                                      {_("Spring"), _("Hall"), _("Room"), _("Plate")},
                                      _("Reverb type"), _("Type of reverb effect"))));
}
