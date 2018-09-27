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

P_Equalizer::P_Equalizer()
{
    slots.emplace_back((new PA_Choice(568, 4,
                                      {"40.97", "42.17", "43.40", "44.67", "45.97", "47.32", "48.70", "50.12", "51.58", "53.09", "54.64", "56.23", "57.88", "59.57", "61.31", "63.10", "64.94", "66.83", "68.79", "70.79", "72.86", "74.99", "77.18", "79.43", "81.75", "84.14", "86.60", "89.13", "91.73", "94.41", "97.16", "100.0", "102.9", "105.9", "109.0", "112.2", "115.5", "118.9", "122.3", "125.9", "129.6", "133.4", "137.2", "141.3", "145.4", "149.6", "154.0", "158.5", "163.1", "167.9", "172.8", "177.8", "183.0", "188.4", "193.9", "199.5", "205.4", "211.3", "217.5", "223.9", "230.4", "237.1", "244.1", "251.2", "258.5", "266.1", "273.8", "281.8", "290.1", "298.5", "307.3", "316.2", "325.5", "335.0", "344.7", "354.8", "365.2", "375.8", "386.8", "398.1", "409.7", "421.7", "434.0", "446.7", "459.7", "473.2", "487.0", "501.2", "515.8", "530.9", "546.4", "562.3", "578.8", "595.7", "613.1", "631.0", "649.4", "668.3", "687.9", "707.9", "728.6", "749.9", "771.8", "794.3", "817.5", "841.4", "866.0", "891.3", "917.3", "944.1", "971.6", "1.00k", "1.03k", "1.06k", "1.09k", "1.12k", "1.15k", "1.19k", "1.22k", "1.26k", "1.30k", "1.33k", "1.37k", "1.41k", "1.45k", "1.50k", "1.54k", "1.58k", "1.63k", "1.68k", "1.73k", "1.78k", "1.83k", "1.88k", "1.94k", "2.00k", "2.05k", "2.11k", "2.18k", "2.24k", "2.30k", "2.37k", "2.44k", "2.51k", "2.59k", "2.66k", "2.74k", "2.82k", "2.90k", "2.99k", "3.07k", "3.16k", "3.25k", "3.35k", "3.45k", "3.55k", "3.65k", "3.76k", "3.87k", "3.98k", "4.10k", "4.22k", "4.34k", "4.47k", "4.60k", "4.73k", "4.87k", "5.01k", "5.16k", "5.31k", "5.46k", "5.62k", "5.79k", "5.96k", "6.13k", "6.31k", "6.49k", "6.68k", "6.88k", "7.08k", "7.29k", "7.50k", "7.72k", "7.94k", "8.18k", "8.41k", "8.66k", "8.91k", "9.17k", "9.44k", "9.72k", "10.0k", "10.3k", "10.6k", "10.9k", "11.2k", "11.5k", "11.9k", "12.2k", "12.6k", "13.0k", "13.3k", "13.7k", "14.1k", "14.5k", "15.0k", "15.4k", "15.8k", "16.3k", "16.8k", "17.3k", "17.8k", "18.3k", "18.8k", "19.4k", "20.0k", "Off"},
                                      _("Frequency"), _("Sets the operating frequency for the selected band.")))
                       ->with_offset(25));
    slots.emplace_back((new PA_Choice(580, 4, frequency1().values, frequency1().name, frequency1().description))
                       ->with_offset(frequency1().offset));
    slots.emplace_back((new PA_Choice(592, 4, frequency1().values, frequency1().name, frequency1().description))
                       ->with_offset(frequency1().offset));
    slots.emplace_back((new PA_Integer(572, 4, _("Gain"), _("Gains or attenuates the selected frequency area.")))
                       ->with_min_max(-12, 12));
    slots.emplace_back((new PA_Integer(584, 4, gain1().name, gain1().description))
                       ->with_min_max(gain1().vmin, gain1().vmax));
    slots.emplace_back((new PA_Integer(596, 4, gain1().name, gain1().description))
                       ->with_min_max(gain1().vmin, gain1().vmax));
    slots.emplace_back((new PA_Choice(576, 4,
                                      {"0.2", "0.25", "0.32", "0.4", "0.5", "0.63", "0.8", "1.0", "1.25", "1.6", "2.0", "2.5", "3.2", "4.0"},
                                      _("Width"), _("Width defines the area around the set frequency that the EQ will amplify or attenuate.")))
                       ->with_offset(3));
    slots.emplace_back((new PA_Choice(588, 4, width1().values, width1().name, width1().description))
                       ->with_offset(width1().offset));
    slots.emplace_back((new PA_Choice(600, 4, width1().values, width1().name, width1().description))
                       ->with_offset(width1().offset));
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
