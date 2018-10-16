//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "parameter.h"
#include "patch.h"
#include "app_i18n.h"
#include <assert.h>

static int32_t load_int24(const uint8_t *src)
{
    uint32_t value24 = 0;
    for (unsigned i = 0; i < 4; ++i)
        value24 |= (src[i] & 127) << (7 * i);

    uint32_t result;
    if (value24 & (1u << 23))
        result = value24 | ((1u << 8) - 1) << 24;
    else
        result = value24;

    return result;
}

static void store_int24(uint8_t *dst, int32_t value)
{
    uint32_t value24;
    if (value >= 0)
        value24 = (uint32_t)value;
    else
        value24 = ((uint32_t)value & ((1u << 23) - 1)) | (1u << 23);

    for (unsigned i = 0; i < 4; ++i)
        dst[i] = (value24 >> (7 * i)) & 127;
}

static int32_t load_int(const uint8_t *src, unsigned size)
{
    switch (size) {
    case 4:  // sign-extended 24 bit integer storage, LSB first
        return load_int24(src);
    default:
        assert(false); abort();
    }
}

static void store_int(uint8_t *dst, unsigned size, int32_t v)
{
    switch (size) {
    case 4:  // sign-extended 24 bit integer storage, LSB first
        store_int24(dst, v); break;
    default:
        assert(false); abort();
    }
}

static std::vector<bool> read_bits7(const uint8_t *src, unsigned size)
{
    std::vector<bool> bits(7 * size);
    for (unsigned i_byte = 0; i_byte < size; ++i_byte) {
        for (unsigned i_bit = 0; i_bit < 7; ++i_bit)
            bits[7 * i_byte + i_bit] = (src[i_byte] & (1 << i_bit)) != 0;
    }
    return bits;
}

static void write_bits7(uint8_t *dst, unsigned size, const std::vector<bool> &bits)
{
    for (unsigned i_byte = 0; i_byte < size; ++i_byte) {
        unsigned b = dst[i_byte];
        for (unsigned i_bit = 0; i_bit < 7; ++i_bit) {
            b &= ~(1u << i_bit);
            b |= (unsigned)bits[7 * i_byte + i_bit] << i_bit;
        }
        dst[i_byte] = (uint8_t)b;
    }
}

///

Parameter_Access *Parameter_Access::with_string_fn(std::function<std::string(int)> fn)
{
    to_string_fn = std::move(fn);
    return this;
}

Parameter_Access *Parameter_Access::with_position(Parameter_Position pos)
{
    position = pos;
    return this;
}

Parameter_Access *Parameter_Access::with_modifier_at(unsigned index)
{
    std::unique_ptr<Parameter_Modifiers> modifiers(new Parameter_Modifiers);
    modifiers->assignment.reset((new PA_Bits(index, 4,
                                             0, 3,
                                             _P("Modifier|", "Assignment"), _("Assignment")))
                                ->with_min_max(0, 4)
                                ->with_string_fn([](int v) -> std::string {
                                                     if (v == 0)
                                                         return "Off";
                                                     else
                                                         return "M" + std::to_string(v);
                                                 }));
    modifiers->min.reset((new PA_Bits(index, 4,
                                      3, 7,
                                      _P("Modifier|", "Minimum"), _("Minimum")))
                         ->with_min_max(0, 100));
    modifiers->mid.reset((new PA_Bits(index, 4,
                                      10, 7,
                                      _P("Modifier|", "Middle"), _("Middle")))
                         ->with_min_max(0, 100));
    modifiers->max.reset((new PA_Bits(index, 4,
                                      17, 7,
                                      _P("Modifier|", "Maximum"), _("Maximum")))
                         ->with_min_max(0, 100));
    this->modifiers = std::move(modifiers);
    return this;
}

std::string Parameter_Access::to_string(int value) const
{
    if (to_string_fn)
        return to_string_fn(value);
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
    if (to_string_fn)
        return to_string_fn(value);
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
    int v = clamp(value) + offset;
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
    if (to_string_fn)
        return to_string_fn(value);
    return values[clamp(value)];
}

PA_Bits *PA_Bits::with_min_max(int vmin, int vmax)
{
    this->vmin = vmin;
    this->vmax = vmax;
    return this;
}

PA_Bits *PA_Bits::with_offset(int offset)
{
    this->offset = offset;
    return this;
}

int PA_Bits::get(const Patch &pat) const
{
    std::vector<bool> bits = read_bits7(&pat.raw_data[index], size);
    int v = 0;
    for (unsigned i = 0; i < bit_size; ++i)
        v |= (unsigned)bits[bit_offset + i] << i;
    return clamp(v - offset);
}

void PA_Bits::set(Patch &pat, int value)
{
    int v = clamp(value) + offset;
    std::vector<bool> bits = read_bits7(&pat.raw_data[index], size);
    for (unsigned i = 0; i < bit_size; ++i)
        bits[bit_offset + i] = (v & (1u << i)) != 0;
    write_bits7(&pat.raw_data[index], size, bits);
}

int PA_Bits::clamp(int value) const
{
    value = (value < vmin) ? vmin : value;
    value = (value > vmax) ? vmax : value;
    return value;
}

///

P_Compressor::P_Compressor()
{
    slots.emplace_back((new PA_Integer(164, 4, _("Threshold"), _("When the signal is above the set Threshold point the Compressor is activated and the gain of any signal above the Threshold point is processed according to the Ratio, Attack and Release.")))
                       ->with_min_max(-40, 40));
    slots.emplace_back((new PA_Choice(168, 4,
                                      {_("Off"), "1.12:1", "1.25:1", "1.40:1", "1.60:1", "1.80:1", "2.0:1", "2.5:1", "3.2:1", "4.0:1", "5.6:1", "8.0:1", "16:1", "32:1", "64:1", _("Inf:1")},
                                      _("Ratio"), _("The Ratio setting determines how hard the signal is compressed."))));
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

P_Noise_Gate::P_Noise_Gate()
{
    slots.emplace_back((new PA_Choice(548, 4,
                                      {_P("Noise Gate|Mode|", "Soft"), _P("Noise Gate|Mode|", "Hard")},
                                      _("Mode"), _("General overall mode that determines how fast the Noise Gate should attenuate/dampen the signal when below Threshold."))));
    slots.emplace_back((new PA_Integer(552, 4, _("Threshold"), _("The Threshold point determines at what point the Noise Gate should start to dampen the signal.")))
                       ->with_min_max(-60, 0));
    slots.emplace_back((new PA_Integer(556, 4, _("Max Damp"/*Max Damping*/), _("The parameter determines how hard the signal should be attenuated when below the set Threshold.")))
                       ->with_min_max(0, 90));
    slots.emplace_back((new PA_Integer(560, 4, _("Release"), _("The Release parameter determines how fast the signal is released when the Input signal rises above the Threshold point.")))
                       ->with_min_max(3, 200));
}

P_Reverb::P_Reverb()
{
    slots.emplace_back((new PA_Integer(488, 4, _("Decay"), _("The Decay parameter determines the length of the Reverb Diffuse field.")))
                       ->with_min_max(1, 200));
    slots.emplace_back((new PA_Integer(492, 4, _("Pre Delay"), _("A short Delay placed between the direct signal and the Reverb Diffuse field.")))
                       ->with_min_max(0, 100));
    slots.emplace_back((new PA_Choice(496, 4,
                                      {_P("Reverb|Shape|", "Round"), _P("Reverb|Shape|", "Curved"), _P("Reverb|Shape|", "Square")},
                                      _("Shape"), _("Shape"))));
    slots.emplace_back((new PA_Choice(500, 4,
                                      {_P("Reverb|Size|", "Box"), _P("Reverb|Size|", "Tiny"), _P("Reverb|Size|", "Small"), _P("Reverb|Size|", "Medium"), _P("Reverb|Size|", "Large"), _P("Reverb|Size|", "Ex Large"), _P("Reverb|Size|", "Grand"), _P("Reverb|Size|", "Huge")},
                                      _("Size"), _("The Size parameter defines the size of the Early Reflection pattern used."))));
    slots.emplace_back((new PA_Choice(504, 4,
                                      {_P("Reverb|Hi Color|", "Wool"), _P("Reverb|Hi Color|", "Warm"), _P("Reverb|Hi Color|", "Real"), _P("Reverb|Hi Color|", "Clear"), _P("Reverb|Hi Color|", "Bright"), _P("Reverb|Hi Color|", "Crisp"), _P("Reverb|Hi Color|", "Grand")},
                                      _("Hi color"), _("7 different Hi Colors can be selected."))));
    slots.emplace_back((new PA_Integer(508, 4, _("Hi factor"), _("Adds or substracts the selected Hi Color type.")))
                       ->with_min_max(-25, 25));
    slots.emplace_back((new PA_Choice(512, 4,
                                      {_P("Reverb|Lo Color|", "Thick"), _P("Reverb|Lo Color|", "Round"), _P("Reverb|Lo Color|", "Real"), _P("Reverb|Lo Color|", "Light"), _P("Reverb|Lo Color|", "Tight"), _P("Reverb|Lo Color|", "Thin"), _P("Reverb|Lo Color|", "No Base")},
                                      _("Lo color"), _("7 different Lo Colors can be selected."))));
    slots.emplace_back((new PA_Integer(516, 4, _("Lo factor"), _("Adds or substracts the selected Lo Color type.")))
                       ->with_min_max(-25, 25));
    slots.emplace_back((new PA_Integer(520, 4, _("Room level"), _("This parameter adjusts the Reverb Diffuse field level. Lowering the Reverb Level will give you a more ambient sound, since the Early Reflection patterns will become more obvious.")))
                       ->with_min_max(-100, 0));
    slots.emplace_back((new PA_Integer(524, 4, _("Rev level"/*Reverb level*/), _("The level of the Early Reflections.")))
                       ->with_min_max(-100, 0));
    slots.emplace_back((new PA_Integer(528, 4, _("Diffuse"), _("Allows fine-tuning of the density of the Reverb Diffuse field.")))
                       ->with_min_max(-25, 25));
    slots.emplace_back((new PA_Integer(532, 4, _("Mix"), _("Sets the relation between the dry signal and the applied effect in this block.")))
                       ->with_min_max(0, 100)
                       ->with_position(PP_Back)
                       ->with_modifier_at(148));
    slots.emplace_back((new PA_Integer(536, 4, _("Out level"), _("Sets the overall Output level of this block.")))
                       ->with_min_max(-100, 0)
                       ->with_position(PP_Back)
                       ->with_modifier_at(152));
}

P_Pitch::P_Pitch(const PA_Choice &tag)
    : Polymorphic_Parameter_Collection(tag)
{
}

Parameter_Collection &P_Pitch::dispatch(const Patch &pat)
{
    switch (tag.get(pat)) {
    default:
        assert(false);
    case 0:
        return detune;
    case 1:
        return whammy;
    case 2:
        return octaver;
    case 3:
        return shifter;
    }
}

P_Pitch::Detune::Detune()
{
    slots.emplace_back((new PA_Integer(296, 4, _("Voice 1"), _("Offsets the first Voice in the Detune block.")))
                       ->with_min_max(-100, 100));
    slots.emplace_back((new PA_Integer(300, 4, _("Voice 2"), _("Offsets the second Voice in the Detune block.")))
                       ->with_min_max(-100, 100));
    slots.emplace_back((new PA_Integer(312, 4, _("Delay 1"), _("Specifies the Delay on the first voice.")))
                       ->with_min_max(0, 50));
    slots.emplace_back((new PA_Integer(316, 4, _("Delay 2"), _("Specifies the Delay on the second voice.")))
                       ->with_min_max(0, 50));
    slots.emplace_back((new PA_Integer(340, 4, _("Mix"), _("Sets the relation between the dry signal and the applied effect in this block.")))
                       ->with_min_max(0, 100)
                       ->with_position(PP_Back)
                       ->with_modifier_at(48));
    slots.emplace_back((new PA_Integer(344, 4, _("Out level"), _("Sets the overall Output level of this block.")))
                       ->with_min_max(-100, 0)
                       ->with_position(PP_Back)
                       ->with_modifier_at(52));
}

P_Pitch::Whammy::Whammy()
{
    slots.emplace_back((new PA_Integer(328, 4, _("Pitch"), _("This parameter sets the mix between the dry and processed signal. If e.g. set to 100%, no direct guitar tone will be heard - only the processed \"pitched\" tone.")))
                       ->with_min_max(0, 100)
                       ->with_modifier_at(48));
    slots.emplace_back((new PA_Choice(332, 4,
                                      {_P("Pitch|Direction|", "Down"), _P("Pitch|Direction|", "Up")},
                                      _("Direction"), _("This parameter determines whether the attached Expression pedal should increase or decrease Pitch when moved either direction."))));
    slots.emplace_back((new PA_Choice(336, 4,
                                      {_P("Pitch|Range|", "1 oct"), _P("Pitch|Range|", "2 oct")},
                                      _("Range"), _("Selects how the Whammy block will pitch your tone.")))
                       ->with_offset(1));
    slots.emplace_back((new PA_Integer(344, 4, _("Out level"), _("Sets the overall Output level of this block.")))
                       ->with_min_max(-100, 0)
                       ->with_position(PP_Back)
                       ->with_modifier_at(52));
}

P_Pitch::Octaver::Octaver()
{
    slots.emplace_back((new PA_Choice(332, 4,
                                      {_P("Pitch|Direction|", "Down"), _P("Pitch|Direction|", "Up")},
                                      _("Direction"), _("Direction."))));
    slots.emplace_back((new PA_Choice(336, 4,
                                      {_P("Pitch|Range|", "1 oct"), _P("Pitch|Range|", "2 oct")},
                                      _("Range"), _("Range.")))
                       ->with_offset(1));
    slots.emplace_back((new PA_Integer(340, 4, _("Mix"), _("Sets the relation between the dry signal and the applied effect in this block.")))
                       ->with_min_max(0, 100)
                       ->with_position(PP_Back)
                       ->with_modifier_at(48));
    slots.emplace_back((new PA_Integer(344, 4, _("Out level"), _("Sets the overall Output level of this block.")))
                       ->with_min_max(-100, 0)
                       ->with_position(PP_Back)
                       ->with_modifier_at(52));
}

P_Pitch::Shifter::Shifter()
{
    slots.emplace_back((new PA_Integer(296, 4, _("Voice 1"), _("Specifies the Pitch of the first Voice. As 100 cent equals 1 semitone you can select a pitch freely between one octave below the Input Pitch to one octave above.")))
                       ->with_min_max(-2400, 2400)
                       ->with_modifier_at(48));
    slots.emplace_back((new PA_Integer(300, 4, _("Voice 2"), _("Specifies the Pitch of the second Voice. As 100 cent equals 1 semitone you can select a pitch freely between one octave below the Input Pitch to one octave above.")))
                       ->with_min_max(-2400, 2400)
                       ->with_modifier_at(52));
    slots.emplace_back((new PA_Integer(304, 4, _("Pan 1"), _("Pan parameter for the first voice.")))
                       ->with_min_max(-50, 50)
                       ->with_modifier_at(56));
    slots.emplace_back((new PA_Integer(308, 4, _("Pan 2"), _("Pan parameter for the second voice.")))
                       ->with_min_max(-50, 50)
                       ->with_modifier_at(60));
    slots.emplace_back((new PA_Integer(312, 4, _("Delay 1"), _("Sets the delay time for the first voice.")))
                       ->with_min_max(0, 350));
    slots.emplace_back((new PA_Integer(316, 4, _("Delay 2"), _("Sets the delay time for the second voice.")))
                       ->with_min_max(0, 350));
    slots.emplace_back((new PA_Integer(320, 4, _("Feedback 1"), _("Determines how many repetitions there will be on the Delay of the first voice.")))
                       ->with_min_max(0, 100)
                       ->with_modifier_at(64));
    slots.emplace_back((new PA_Integer(324, 4, _("Feedback 2"), _("Determines how many repetitions there will be on the Delay of the second voice.")))
                       ->with_min_max(0, 100)
                       ->with_modifier_at(68));
    slots.emplace_back((new PA_Integer(328, 4, _("Level 1"), _("Sets the level for Voice 1.")))
                       ->with_min_max(-100, 0));
    slots.emplace_back((new PA_Integer(332, 4, _("Level 2"), _("Sets the level for Voice 2.")))
                       ->with_min_max(-100, 0));
    slots.emplace_back((new PA_Integer(340, 4, _("Mix"), _("Sets the relation between the dry signal and the applied effect in this block.")))
                       ->with_min_max(0, 100)
                       ->with_position(PP_Back)
                       ->with_modifier_at(72));
    slots.emplace_back((new PA_Integer(344, 4, _("Out level"), _("Sets the overall Output level of this block.")))
                       ->with_min_max(-100, 0)
                       ->with_position(PP_Back)
                       ->with_modifier_at(76));
}

P_Delay::P_Delay(const PA_Choice &tag)
    : Polymorphic_Parameter_Collection(tag)
{
}

Parameter_Collection &P_Delay::dispatch(const Patch &pat)
{
    switch (tag.get(pat)) {
    default:
        assert(false);
    case 0:
        return ping_pong;
    case 1:
        return dynamic;
    case 2:
        return dual;
    }
}

P_Delay::Ping_Pong::Ping_Pong()
{
    slots.emplace_back((new PA_Integer(424, 4, _("Delay"/*Delay Time*/), _("The time between the repetitions.")))
                       ->with_min_max(0, 1800)
                       ->with_modifier_at(108));
    slots.emplace_back((new PA_Choice(432, 4,
                                      {_P("*|Tempo|", "Ignored"), _P("*|Tempo|", "1"), _P("*|Tempo|", "1/2D"), _P("*|Tempo|", "1/2"), _P("*|Tempo|", "1/2T"), _P("*|Tempo|", "1/4D"), _P("*|Tempo|", "1/4"), _P("*|Tempo|", "1/4T"), _P("*|Tempo|", "1/8D"), _P("*|Tempo|", "1/8"), _P("*|Tempo|", "1/8T"), _P("*|Tempo|", "1/16D"), _P("*|Tempo|", "1/16"), _P("*|Tempo|", "1/16T"), _P("*|Tempo|", "1/32D"), _P("*|Tempo|", "1/32"), _P("*|Tempo|", "1/32T")},
                                      _("Tempo"), _("The Tempo parameter sets the relationship to the global Tempo."))));
    slots.emplace_back((new PA_Integer(436, 4, _("Width"), _("The Width parameter determines whether the Left or Right repetitions are panned 100% or not.")))
                       ->with_min_max(0, 100));
    slots.emplace_back((new PA_Integer(440, 4, _("Feedback"), _("Determines how many repetitions there will be.")))
                       ->with_min_max(0, 100)
                       ->with_modifier_at(112));
    slots.emplace_back((new PA_Choice(448, 4,
                                      {"2.00k", "2.24k", "2.51k", "2.82k", "3.16k", "3.55k", "3.98k", "4.47k", "5.01k", "5.62k", "6.31k", "7.08k", "7.94k", "8.91k", "10.0k", "11.2k", "12.6k", "14.1k", "15.8k", "17.8k", _("Off")},
                                      _("FB Hi cut"), _("Attenuates the frequencies above the set frequency thereby giving you a more analog Delay sound that in many cases will blend better in the overall sound.")))
                       ->with_modifier_at(116));
    slots.emplace_back((new PA_Choice(452, 4,
                                      {_("Off"), "22.39", "25.12", "28.18", "31.62", "35.48", "39.81", "44.67", "50.12", "56.23", "63.10", "70.79", "79.43", "89.13", "100.0", "112.2", "125.9", "141.3", "158.5", "177.8", "199.5", "223.9", "251.2", "281.8", "316.2", "354.8", "398.1", "446.7", "501.2", "562.3", "631.0", "707.9", "794.3", "891.3", "1.00k", "1.12k", "1.26k", "1.41k", "1.58k", "1.78k", "2.00k"},
                                      _("FB Lo cut"), _("Attenuates the frequencies below the set frequency.")))
                       ->with_modifier_at(120));
    slots.emplace_back((new PA_Integer(472, 4, _("Mix"), _("Sets the relation between the dry signal and the applied effect in this block.")))
                       ->with_min_max(0, 100)
                       ->with_position(PP_Back)
                       ->with_modifier_at(124));
    slots.emplace_back((new PA_Integer(476, 4, _("Out level"), _("Sets the overall Output level of this block.")))
                       ->with_min_max(-100, 0)
                       ->with_position(PP_Back)
                       ->with_modifier_at(128));
}

P_Delay::Dynamic::Dynamic()
{
    slots.emplace_back((new PA_Integer(424, 4, _("Delay"/*Delay Time*/), _("The time between the repetitions.")))
                       ->with_min_max(0, 1800)
                       ->with_modifier_at(108));
    slots.emplace_back((new PA_Choice(432, 4,
                                      {_P("*|Tempo|", "Ignored"), _P("*|Tempo|", "1"), _P("*|Tempo|", "1/2D"), _P("*|Tempo|", "1/2"), _P("*|Tempo|", "1/2T"), _P("*|Tempo|", "1/4D"), _P("*|Tempo|", "1/4"), _P("*|Tempo|", "1/4T"), _P("*|Tempo|", "1/8D"), _P("*|Tempo|", "1/8"), _P("*|Tempo|", "1/8T"), _P("*|Tempo|", "1/16D"), _P("*|Tempo|", "1/16"), _P("*|Tempo|", "1/16T"), _P("*|Tempo|", "1/32D"), _P("*|Tempo|", "1/32"), _P("*|Tempo|", "1/32T")},
                                      _("Tempo"), _("The Tempo parameter sets the relationship to the global Tempo."))));
    slots.emplace_back((new PA_Integer(440, 4, _("Feedback"), _("Determines how many repetitions there will be.")))
                       ->with_min_max(0, 100)
                       ->with_modifier_at(112));
    slots.emplace_back((new PA_Choice(448, 4,
                                      {"2.00k", "2.24k", "2.51k", "2.82k", "3.16k", "3.55k", "3.98k", "4.47k", "5.01k", "5.62k", "6.31k", "7.08k", "7.94k", "8.91k", "10.0k", "11.2k", "12.6k", "14.1k", "15.8k", "17.8k", _("Off")},
                                      _("FB Hi cut"), _("Attenuates the frequencies above the set frequency thereby giving you a more analog Delay sound that in many cases will blend better in the overall sound.")))
                       ->with_modifier_at(116));
    slots.emplace_back((new PA_Choice(452, 4,
                                      {_("Off"), "22.39", "25.12", "28.18", "31.62", "35.48", "39.81", "44.67", "50.12", "56.23", "63.10", "70.79", "79.43", "89.13", "100.0", "112.2", "125.9", "141.3", "158.5", "177.8", "199.5", "223.9", "251.2", "281.8", "316.2", "354.8", "398.1", "446.7", "501.2", "562.3", "631.0", "707.9", "794.3", "891.3", "1.00k", "1.12k", "1.26k", "1.41k", "1.58k", "1.78k", "2.00k"},
                                      _("FB Lo cut"), _("Attenuates the frequencies below the set frequency.")))
                       ->with_modifier_at(120));
    slots.emplace_back((new PA_Integer(456, 4, _("Offset R"), _("Offsets the Delay repeats in the Right channel only. For a true wide stereo Delay the Delay in the two channels should not appear at the exact same time.")))
                       ->with_min_max(-200, 200));
    slots.emplace_back((new PA_Integer(460, 4, _("Sensitivity"), _("With this parameter you control how sensitive the \"ducking\" or dampening function of the Delay repeats should be according to the signal present on the Input.")))
                       ->with_min_max(-50, 0));
    slots.emplace_back((new PA_Integer(464, 4, _("Damping"), _("This parameter controls the actual attenuation of the Delay while Input is present.")))
                       ->with_min_max(0, 100));
    slots.emplace_back((new PA_Choice(468, 4,
                                      {"20", "30", "50", "70", "100", "140", "200", "300", "500", "700", "1000"},
                                      _("Release"), _("A parameter relative to a Compressor release.")))
                       ->with_offset(3));
    slots.emplace_back((new PA_Integer(472, 4, _("Mix"), _("Sets the relation between the dry signal and the applied effect in this block.")))
                       ->with_min_max(0, 100)
                       ->with_position(PP_Back)
                       ->with_modifier_at(124));
    slots.emplace_back((new PA_Integer(476, 4, _("Out level"), _("Sets the overall Output level of this block.")))
                       ->with_min_max(-100, 0)
                       ->with_position(PP_Back)
                       ->with_modifier_at(128));
}

P_Delay::Dual::Dual()
{
    slots.emplace_back((new PA_Integer(424, 4, _("Delay 1"/*Delay Time 1*/), _("Sets the Delay Time of the first Delay Line.")))
                       ->with_min_max(0, 1800)
                       ->with_modifier_at(108));
    slots.emplace_back((new PA_Integer(428, 4, _("Delay 2"/*Delay Time 2*/), _("Sets the Delay Time of the second Delay Line.")))
                       ->with_min_max(0, 1800)
                       ->with_modifier_at(112));
    slots.emplace_back((new PA_Choice(432, 4,
                                      {_P("*|Tempo|", "Ignored"), _P("*|Tempo|", "1"), _P("*|Tempo|", "1/2D"), _P("*|Tempo|", "1/2"), _P("*|Tempo|", "1/2T"), _P("*|Tempo|", "1/4D"), _P("*|Tempo|", "1/4"), _P("*|Tempo|", "1/4T"), _P("*|Tempo|", "1/8D"), _P("*|Tempo|", "1/8"), _P("*|Tempo|", "1/8T"), _P("*|Tempo|", "1/16D"), _P("*|Tempo|", "1/16"), _P("*|Tempo|", "1/16T"), _P("*|Tempo|", "1/32D"), _P("*|Tempo|", "1/32"), _P("*|Tempo|", "1/32T")},
                                      _("Tempo 1"), _("The Tempo parameter sets the relationship to the global Tempo."))));
    slots.emplace_back((new PA_Choice(436, 4,
                                      {_P("*|Tempo|", "Ignored"), _P("*|Tempo|", "1"), _P("*|Tempo|", "1/2D"), _P("*|Tempo|", "1/2"), _P("*|Tempo|", "1/2T"), _P("*|Tempo|", "1/4D"), _P("*|Tempo|", "1/4"), _P("*|Tempo|", "1/4T"), _P("*|Tempo|", "1/8D"), _P("*|Tempo|", "1/8"), _P("*|Tempo|", "1/8T"), _P("*|Tempo|", "1/16D"), _P("*|Tempo|", "1/16"), _P("*|Tempo|", "1/16T"), _P("*|Tempo|", "1/32D"), _P("*|Tempo|", "1/32"), _P("*|Tempo|", "1/32T")},
                                      _("Tempo 2"), _("The Tempo parameter sets the relationship to the global Tempo."))));
    slots.emplace_back((new PA_Integer(440, 4, _("Feedback 1"), _("Determines the number of repetitions of the Delay of the first Delay Line.")))
                       ->with_min_max(0, 100)
                       ->with_modifier_at(116));
    slots.emplace_back((new PA_Integer(444, 4, _("Feedback 2"), _("Determines the number of repetitions of the Delay of the second Delay Line.")))
                       ->with_min_max(0, 100)
                       ->with_modifier_at(120));
    slots.emplace_back((new PA_Choice(448, 4,
                                      {"2.00k", "2.24k", "2.51k", "2.82k", "3.16k", "3.55k", "3.98k", "4.47k", "5.01k", "5.62k", "6.31k", "7.08k", "7.94k", "8.91k", "10.0k", "11.2k", "12.6k", "14.1k", "15.8k", "17.8k", _("Off")},
                                      _("FB Hi cut"), _("Attenuates the frequencies above the set frequency thereby giving you a more analog Delay sound that in many cases will blend better in the overall sound.")))
                       ->with_modifier_at(124));
    slots.emplace_back((new PA_Choice(452, 4,
                                      {_("Off"), "22.39", "25.12", "28.18", "31.62", "35.48", "39.81", "44.67", "50.12", "56.23", "63.10", "70.79", "79.43", "89.13", "100.0", "112.2", "125.9", "141.3", "158.5", "177.8", "199.5", "223.9", "251.2", "281.8", "316.2", "354.8", "398.1", "446.7", "501.2", "562.3", "631.0", "707.9", "794.3", "891.3", "1.00k", "1.12k", "1.26k", "1.41k", "1.58k", "1.78k", "2.00k"},
                                      _("FB Lo cut"), _("Attenuates the frequencies below the set frequency.")))
                       ->with_modifier_at(128));
    slots.emplace_back((new PA_Integer(456, 4, _("Pan 1"), _("Pans the Delay repetitions of the first Delay Line.")))
                       ->with_min_max(-50, 50)
                       ->with_modifier_at(132));
    slots.emplace_back((new PA_Integer(460, 4, _("Pan 2"), _("Pans the Delay repetitions of the second Delay Line.")))
                       ->with_min_max(-50, 50)
                       ->with_modifier_at(136));
    slots.emplace_back((new PA_Integer(472, 4, _("Mix"), _("Sets the relation between the dry signal and the applied effect in this block.")))
                       ->with_min_max(0, 100)
                       ->with_position(PP_Back)
                       ->with_modifier_at(140));
    slots.emplace_back((new PA_Integer(476, 4, _("Out level"), _("Sets the overall Output level of this block.")))
                       ->with_min_max(-100, 0)
                       ->with_position(PP_Back)
                       ->with_modifier_at(144));
}

P_Filter::P_Filter(const PA_Choice &tag)
    : Polymorphic_Parameter_Collection(tag)
{
}

Parameter_Collection &P_Filter::dispatch(const Patch &pat)
{
    switch (tag.get(pat)) {
    default:
        assert(false);
    case 0:
        return auto_resonance;
    case 1:
        return resonance;
    case 2:
        return vintage_phaser;
    case 3:
        return smooth_phaser;
    case 4:
        return tremolo;
    case 5:
        return panner;
    }
}

P_Filter::Auto_Resonance::Auto_Resonance()
{
    slots.emplace_back((new PA_Choice(232, 4,
                                      {_P("Filter|Resonance|Order|", "2nd"), _P("Filter|Resonance|Order|", "4th")},
                                      _("Order"), _("The Order parameter of the resonance filters changes the steepness of the filters. 2nd order filters are 12dB/Octave while 4th order filters are 24dB/Octave."))));
    slots.emplace_back((new PA_Integer(236, 4, _("Sensitivity"), _("Sets the sensitivity according to the Input you are feeding the unit.")))
                       ->with_min_max(0, 10));
    slots.emplace_back((new PA_Choice(240, 4,
                                      {_P("Filter|Resonance|Response|", "Slow"), _P("Filter|Resonance|Response|", "Medium"), _P("Filter|Resonance|Response|", "Fast")},
                                      _("Response"), _("Determines whether the sweep through a frequency range will be performed fast or slow."))));
    slots.emplace_back((new PA_Choice(252, 4,
                                      {"1.00k", "1.12k", "1.26k", "1.41k", "1.58k", "1.78k", "2.00k", "2.24k", "2.51k", "2.82k", "3.16k", "3.55k", "3.98k", "4.47k", "5.01k", "5.62k", "6.31k", "7.08k", "7.94k", "8.91k", "10.0k"},
                                      _("Freq Max"/*Frequency Max*/), _("Limits the frequency range in which the sweep will be performed."))));
    slots.emplace_back((new PA_Integer(280, 4, _("Mix"), _("Sets the relation between the dry signal and the applied effect in this block.")))
                       ->with_min_max(0, 100)
                       ->with_position(PP_Back)
                       ->with_modifier_at(28));
    slots.emplace_back((new PA_Integer(284, 4, _("Out level"), _("Sets the overall Output level of this block.")))
                       ->with_min_max(-100, 0)
                       ->with_position(PP_Back)
                       ->with_modifier_at(32));
}

P_Filter::Resonance::Resonance()
{
    slots.emplace_back((new PA_Choice(232, 4,
                                      {_P("Filter|Resonance|Order|", "2nd"), _P("Filter|Resonance|Order|", "4th")},
                                      _("Order"), _("The Order parameter of the resonance filters changes the steepness of the filters. 2nd order filters are 12dB/Octave while 4th order filters are 24dB/Octave."))));
    slots.emplace_back((new PA_Choice(272, 4,
                                      {"158.5", "177.8", "199.5", "223.9", "251.2", "281.8", "316.2", "354.8", "398.1", "446.7", "501.2", "562.3", "631.0", "707.9", "794.3", "891.3", "1.00k", "1.12k", "1.26k", "1.41k", "1.58k", "1.78k", "2.00k", "2.24k", "2.51k", "2.82k", "3.16k", "3.55k", "3.98k", "4.47k", "5.01k", "5.62k", "6.31k", "7.08k", "7.94k", "8.91k", "10.0k", "11.2k", "12.6k", "14.1k"},
                                      _("Hi Cut"), _("Determines the frequency above which the Hi Cut filter will attenuate the high-end frequencies of the generated effect.")))
                       ->with_modifier_at(28));
    slots.emplace_back((new PA_Integer(276, 4, _("Hi Reso"/*Hi Resonance*/), _("Sets the amount of Resonance in the Hi Cut filter.")))
                       ->with_min_max(0, 100)
                       ->with_modifier_at(32));
    slots.emplace_back((new PA_Integer(280, 4, _("Mix"), _("Sets the relation between the dry signal and the applied effect in this block.")))
                       ->with_min_max(0, 100)
                       ->with_position(PP_Back)
                       ->with_modifier_at(36));
    slots.emplace_back((new PA_Integer(284, 4, _("Out level"), _("Sets the overall Output level of this block.")))
                       ->with_min_max(-100, 0)
                       ->with_position(PP_Back)
                       ->with_modifier_at(40));
}

P_Filter::Vintage_Phaser::Vintage_Phaser()
{
    slots.emplace_back((new PA_Choice(244, 4,
                                      {".050", ".052", ".053", ".055", ".056", ".058", ".060", ".061", ".063", ".065", ".067", ".069", ".071", ".073", ".075", ".077", ".079", ".082", ".084", ".087", ".089", ".092", ".094", ".097", ".100", ".103", ".106", ".109", ".112", ".115", ".119", ".122", ".126", ".130", ".133", ".137", ".141", ".145", ".150", ".154", ".158", ".163", ".168", ".173", ".178", ".183", ".188", ".194", ".200", ".205", ".211", ".218", ".224", ".230", ".237", ".244", ".251", ".259", ".266", ".274", ".282", ".290", ".299", ".307", ".316", ".325", ".335", ".345", ".355", ".365", ".376", ".387", ".398", ".410", ".422", ".434", ".447", ".460", ".473", ".487", ".501", ".516", ".531", ".546", ".562", ".579", ".596", ".613", ".631", ".649", ".668", ".688", ".708", ".729", ".750", ".772", ".794", ".818", ".841", ".866", ".891", ".917", ".944", ".972", "1.00", "1.03", "1.06", "1.09", "1.12", "1.15", "1.19", "1.22", "1.26", "1.30", "1.33", "1.37", "1.41", "1.45", "1.50", "1.54", "1.58", "1.63", "1.68", "1.73", "1.78", "1.83", "1.88", "1.94", "2.00", "2.05", "2.11", "2.18", "2.24", "2.30", "2.37", "2.44", "2.51", "2.59", "2.66", "2.74", "2.82", "2.90", "2.99", "3.07", "3.16", "3.25", "3.35", "3.45", "3.55", "3.65", "3.76", "3.87", "3.98", "4.10", "4.22", "4.34", "4.47", "4.60", "4.73", "4.87", "5.01", "5.16", "5.31", "5.46", "5.62", "5.79", "5.96", "6.13", "6.31", "6.49", "6.68", "6.88", "7.08", "7.29", "7.50", "7.72", "7.94", "8.18", "8.41", "8.66", "8.91", "9.17", "9.44", "9.72", "10.00", "10.29", "10.59", "10.90", "11.22", "11.55", "11.89", "12.23", "12.59", "12.96", "13.34", "13.72", "14.13", "14.54", "14.96", "15.40", "15.85", "16.31", "16.79", "17.28", "17.78", "18.30", "18.84", "19.39", "19.95"},
                                      _("Speed"), _("Controls the Speed of the Phaser.")))
                       ->with_modifier_at(28));
    slots.emplace_back((new PA_Integer(248, 4, _("Depth"), _("Controls the Depth of the Phaser.")))
                       ->with_min_max(0, 100)
                       ->with_modifier_at(32));
    slots.emplace_back((new PA_Choice(252, 4,
                                      {_P("*|Tempo|", "Ignored"), _P("*|Tempo|", "1"), _P("*|Tempo|", "1/2D"), _P("*|Tempo|", "1/2"), _P("*|Tempo|", "1/2T"), _P("*|Tempo|", "1/4D"), _P("*|Tempo|", "1/4"), _P("*|Tempo|", "1/4T"), _P("*|Tempo|", "1/8D"), _P("*|Tempo|", "1/8"), _P("*|Tempo|", "1/8T"), _P("*|Tempo|", "1/16D"), _P("*|Tempo|", "1/16"), _P("*|Tempo|", "1/16T"), _P("*|Tempo|", "1/32D"), _P("*|Tempo|", "1/32"), _P("*|Tempo|", "1/32T")},
                                      _("Tempo"), _("The Tempo parameter sets the relationship to the global Tempo."))));
    slots.emplace_back((new PA_Integer(260, 4, _("Feedback"), _("Controls the amount of feedback in the Phaser. Setting this parameter to \"-100\" reverses the phase of the signal that is fed back to the algorithm Input.")))
                       ->with_min_max(-100, 100)
                       ->with_modifier_at(36));
    slots.emplace_back((new PA_Choice(264, 4,
                                      {_P("Filter|Phaser|Range|", "Low"), _P("Filter|Phaser|Range|", "High")},
                                      _("Range"), _("Determines whether the phasing effect should be mainly on the high- of low-end frequencies."))));
    slots.emplace_back((new PA_Boolean(268, 4, _("Phase Rev"/*Phase Reverse*/), _("An LFO phase change that causes a small Delay in one of the waveform starting points. When applied, the Left and Right outputs will start the current waveform at two different points giving you a more extreme wide spread phasing effect.")))
                       ->with_inversion());
    slots.emplace_back((new PA_Integer(280, 4, _("Mix"), _("Sets the relation between the dry signal and the applied effect in this block.")))
                       ->with_min_max(0, 100)
                       ->with_position(PP_Back)
                       ->with_modifier_at(40));
    slots.emplace_back((new PA_Integer(284, 4, _("Out level"), _("Sets the overall Output level of this block.")))
                       ->with_min_max(-100, 0)
                       ->with_position(PP_Back)
                       ->with_modifier_at(44));
}

P_Filter::Smooth_Phaser::Smooth_Phaser()
{
    slots.emplace_back((new PA_Choice(244, 4,
                                      {".050", ".052", ".053", ".055", ".056", ".058", ".060", ".061", ".063", ".065", ".067", ".069", ".071", ".073", ".075", ".077", ".079", ".082", ".084", ".087", ".089", ".092", ".094", ".097", ".100", ".103", ".106", ".109", ".112", ".115", ".119", ".122", ".126", ".130", ".133", ".137", ".141", ".145", ".150", ".154", ".158", ".163", ".168", ".173", ".178", ".183", ".188", ".194", ".200", ".205", ".211", ".218", ".224", ".230", ".237", ".244", ".251", ".259", ".266", ".274", ".282", ".290", ".299", ".307", ".316", ".325", ".335", ".345", ".355", ".365", ".376", ".387", ".398", ".410", ".422", ".434", ".447", ".460", ".473", ".487", ".501", ".516", ".531", ".546", ".562", ".579", ".596", ".613", ".631", ".649", ".668", ".688", ".708", ".729", ".750", ".772", ".794", ".818", ".841", ".866", ".891", ".917", ".944", ".972", "1.00", "1.03", "1.06", "1.09", "1.12", "1.15", "1.19", "1.22", "1.26", "1.30", "1.33", "1.37", "1.41", "1.45", "1.50", "1.54", "1.58", "1.63", "1.68", "1.73", "1.78", "1.83", "1.88", "1.94", "2.00", "2.05", "2.11", "2.18", "2.24", "2.30", "2.37", "2.44", "2.51", "2.59", "2.66", "2.74", "2.82", "2.90", "2.99", "3.07", "3.16", "3.25", "3.35", "3.45", "3.55", "3.65", "3.76", "3.87", "3.98", "4.10", "4.22", "4.34", "4.47", "4.60", "4.73", "4.87", "5.01", "5.16", "5.31", "5.46", "5.62", "5.79", "5.96", "6.13", "6.31", "6.49", "6.68", "6.88", "7.08", "7.29", "7.50", "7.72", "7.94", "8.18", "8.41", "8.66", "8.91", "9.17", "9.44", "9.72", "10.00", "10.29", "10.59", "10.90", "11.22", "11.55", "11.89", "12.23", "12.59", "12.96", "13.34", "13.72", "14.13", "14.54", "14.96", "15.40", "15.85", "16.31", "16.79", "17.28", "17.78", "18.30", "18.84", "19.39", "19.95"},
                                      _("Speed"), _("Controls the Speed of the Phaser.")))
                       ->with_modifier_at(28));
    slots.emplace_back((new PA_Integer(248, 4, _("Depth"), _("Controls the Depth of the Phaser.")))
                       ->with_min_max(0, 100)
                       ->with_modifier_at(32));
    slots.emplace_back((new PA_Choice(252, 4,
                                      {_P("*|Tempo|", "Ignored"), _P("*|Tempo|", "1"), _P("*|Tempo|", "1/2D"), _P("*|Tempo|", "1/2"), _P("*|Tempo|", "1/2T"), _P("*|Tempo|", "1/4D"), _P("*|Tempo|", "1/4"), _P("*|Tempo|", "1/4T"), _P("*|Tempo|", "1/8D"), _P("*|Tempo|", "1/8"), _P("*|Tempo|", "1/8T"), _P("*|Tempo|", "1/16D"), _P("*|Tempo|", "1/16"), _P("*|Tempo|", "1/16T"), _P("*|Tempo|", "1/32D"), _P("*|Tempo|", "1/32"), _P("*|Tempo|", "1/32T")},
                                      _("Tempo"), _("The Tempo parameter sets the relationship to the global Tempo."))));
    slots.emplace_back((new PA_Integer(260, 4, _("Feedback"), _("Controls the amount of feedback in the Phaser. Setting this parameter to \"-100\" reverses the phase of the signal that is fed back to the algorithm Input.")))
                       ->with_min_max(-100, 100)
                       ->with_modifier_at(36));
    slots.emplace_back((new PA_Choice(264, 4,
                                      {_P("Filter|Phaser|Range|", "Low"), _P("Filter|Phaser|Range|", "High")},
                                      _("Range"), _("Determines whether the phasing effect should be mainly on the high- of low-end frequencies."))));
    slots.emplace_back((new PA_Boolean(268, 4, _("Phase Rev"/*Phase Reverse*/), _("An LFO phase change that causes a small Delay in one of the waveform starting points. When applied, the Left and Right outputs will start the current waveform at two different points giving you a more extreme wide spread phasing effect.")))
                       ->with_inversion());
    slots.emplace_back((new PA_Integer(280, 4, _("Mix"), _("Sets the relation between the dry signal and the applied effect in this block.")))
                       ->with_min_max(0, 100)
                       ->with_position(PP_Back)
                       ->with_modifier_at(40));
    slots.emplace_back((new PA_Integer(284, 4, _("Out level"), _("Sets the overall Output level of this block.")))
                       ->with_min_max(-100, 0)
                       ->with_position(PP_Back)
                       ->with_modifier_at(44));
}

P_Filter::Tremolo::Tremolo()
{
    slots.emplace_back((new PA_Choice(244, 4,
                                      {".050", ".052", ".053", ".055", ".056", ".058", ".060", ".061", ".063", ".065", ".067", ".069", ".071", ".073", ".075", ".077", ".079", ".082", ".084", ".087", ".089", ".092", ".094", ".097", ".100", ".103", ".106", ".109", ".112", ".115", ".119", ".122", ".126", ".130", ".133", ".137", ".141", ".145", ".150", ".154", ".158", ".163", ".168", ".173", ".178", ".183", ".188", ".194", ".200", ".205", ".211", ".218", ".224", ".230", ".237", ".244", ".251", ".259", ".266", ".274", ".282", ".290", ".299", ".307", ".316", ".325", ".335", ".345", ".355", ".365", ".376", ".387", ".398", ".410", ".422", ".434", ".447", ".460", ".473", ".487", ".501", ".516", ".531", ".546", ".562", ".579", ".596", ".613", ".631", ".649", ".668", ".688", ".708", ".729", ".750", ".772", ".794", ".818", ".841", ".866", ".891", ".917", ".944", ".972", "1.00", "1.03", "1.06", "1.09", "1.12", "1.15", "1.19", "1.22", "1.26", "1.30", "1.33", "1.37", "1.41", "1.45", "1.50", "1.54", "1.58", "1.63", "1.68", "1.73", "1.78", "1.83", "1.88", "1.94", "2.00", "2.05", "2.11", "2.18", "2.24", "2.30", "2.37", "2.44", "2.51", "2.59", "2.66", "2.74", "2.82", "2.90", "2.99", "3.07", "3.16", "3.25", "3.35", "3.45", "3.55", "3.65", "3.76", "3.87", "3.98", "4.10", "4.22", "4.34", "4.47", "4.60", "4.73", "4.87", "5.01", "5.16", "5.31", "5.46", "5.62", "5.79", "5.96", "6.13", "6.31", "6.49", "6.68", "6.88", "7.08", "7.29", "7.50", "7.72", "7.94", "8.18", "8.41", "8.66", "8.91", "9.17", "9.44", "9.72", "10.00", "10.29", "10.59", "10.90", "11.22", "11.55", "11.89", "12.23", "12.59", "12.96", "13.34", "13.72", "14.13", "14.54", "14.96", "15.40", "15.85", "16.31", "16.79", "17.28", "17.78", "18.30", "18.84", "19.39", "19.95"},
                                      _("Speed"), _("Sets the Speed of the Tremolo.")))
                       ->with_modifier_at(28));
    slots.emplace_back((new PA_Integer(248, 4, _("Depth"), _("Controls the Depth of the Phaser.")))
                       ->with_min_max(0, 100)
                       ->with_modifier_at(32));
    slots.emplace_back((new PA_Choice(252, 4,
                                      {_P("*|Tempo|", "Ignored"), _P("*|Tempo|", "1"), _P("*|Tempo|", "1/2D"), _P("*|Tempo|", "1/2"), _P("*|Tempo|", "1/2T"), _P("*|Tempo|", "1/4D"), _P("*|Tempo|", "1/4"), _P("*|Tempo|", "1/4T"), _P("*|Tempo|", "1/8D"), _P("*|Tempo|", "1/8"), _P("*|Tempo|", "1/8T"), _P("*|Tempo|", "1/16D"), _P("*|Tempo|", "1/16"), _P("*|Tempo|", "1/16T"), _P("*|Tempo|", "1/32D"), _P("*|Tempo|", "1/32"), _P("*|Tempo|", "1/32T")},
                                      _("Tempo"), _("The Tempo parameter sets the relationship to the global Tempo."))));
    slots.emplace_back((new PA_Integer(260, 4, _("LFO P Width"/*LFO Pulse Width*/), _("Controls the division of the upper and the lower part of the current waveform, e.g. if Pulse Width is set to 75%, the upper half of the waveform will be on for 75% of the time.")))
                       ->with_min_max(0, 100));
    slots.emplace_back((new PA_Choice(272, 4,
                                      {"19.95", "22.39", "25.12", "28.18", "31.62", "35.48", "39.81", "44.67", "50.12", "56.23", "63.10", "70.79", "79.43", "89.13", "100.0", "112.2", "125.9", "141.3", "158.5", "177.8", "199.5", "223.9", "251.2", "281.8", "316.2", "354.8", "398.1", "446.7", "501.2", "562.3", "631.0", "707.9", "794.3", "891.3", "1.00k", "1.12k", "1.26k", "1.41k", "1.58k", "1.78k", "2.00k", "2.24k", "2.51k", "2.82k", "3.16k", "3.55k", "3.98k", "4.47k", "5.01k", "5.62k", "6.31k", "7.08k", "7.94k", "8.91k", "10.0k", "11.2k", "12.6k", "14.1k", "15.8k", "17.8k", _("Off")},
                                      _("Hi Cut"), _("Attenuates the high frequencies of the Tremolo effect. Use the Hi Cut filter to create a less dominant Tremolo effect while keeping the Depth.")))
                       ->with_modifier_at(36));
    slots.emplace_back((new PA_Choice(256, 4,
                                      {_P("Filter|Tremolo|Type|", "Soft"), _P("Filter|Tremolo|Type|", "Hard")},
                                      _("Type"), _("Two variations of the steepness of the Tremolo Curve are available. Listen and select."))));
    slots.emplace_back((new PA_Integer(284, 4, _("Out level"), _("Sets the overall Output level of this block.")))
                       ->with_min_max(-100, 0)
                       ->with_position(PP_Back)
                       ->with_modifier_at(40));
}

P_Filter::Panner::Panner()
{
    slots.emplace_back((new PA_Choice(244, 4,
                                      {".050", ".052", ".053", ".055", ".056", ".058", ".060", ".061", ".063", ".065", ".067", ".069", ".071", ".073", ".075", ".077", ".079", ".082", ".084", ".087", ".089", ".092", ".094", ".097", ".100", ".103", ".106", ".109", ".112", ".115", ".119", ".122", ".126", ".130", ".133", ".137", ".141", ".145", ".150", ".154", ".158", ".163", ".168", ".173", ".178", ".183", ".188", ".194", ".200", ".205", ".211", ".218", ".224", ".230", ".237", ".244", ".251", ".259", ".266", ".274", ".282", ".290", ".299", ".307", ".316", ".325", ".335", ".345", ".355", ".365", ".376", ".387", ".398", ".410", ".422", ".434", ".447", ".460", ".473", ".487", ".501", ".516", ".531", ".546", ".562", ".579", ".596", ".613", ".631", ".649", ".668", ".688", ".708", ".729", ".750", ".772", ".794", ".818", ".841", ".866", ".891", ".917", ".944", ".972", "1.00", "1.03", "1.06", "1.09", "1.12", "1.15", "1.19", "1.22", "1.26", "1.30", "1.33", "1.37", "1.41", "1.45", "1.50", "1.54", "1.58", "1.63", "1.68", "1.73", "1.78", "1.83", "1.88", "1.94", "2.00", "2.05", "2.11", "2.18", "2.24", "2.30", "2.37", "2.44", "2.51", "2.59", "2.66", "2.74", "2.82", "2.90", "2.99", "3.07", "3.16", "3.25", "3.35", "3.45", "3.55", "3.65", "3.76", "3.87", "3.98", "4.10", "4.22", "4.34", "4.47", "4.60", "4.73", "4.87", "5.01", "5.16", "5.31", "5.46", "5.62", "5.79", "5.96", "6.13", "6.31", "6.49", "6.68", "6.88", "7.08", "7.29", "7.50", "7.72", "7.94", "8.18", "8.41", "8.66", "8.91", "9.17", "9.44", "9.72", "10.00", "10.29", "10.59", "10.90", "11.22", "11.55", "11.89", "12.23", "12.59", "12.96", "13.34", "13.72", "14.13", "14.54", "14.96", "15.40", "15.85", "16.31", "16.79", "17.28", "17.78", "18.30", "18.84", "19.39", "19.95"},
                                      _("Speed"), _("Sets the Speed of the Panning.")))
                       ->with_modifier_at(28));
    slots.emplace_back((new PA_Integer(248, 4, _("Width"), _("A 100% setting will sweep the signal completely from the Left to the Right. Very often a more subtle setting will be more applicable and blend better with the overall sound.")))
                       ->with_min_max(0, 100)
                       ->with_modifier_at(32));
    slots.emplace_back((new PA_Choice(252, 4,
                                      {_P("*|Tempo|", "Ignored"), _P("*|Tempo|", "1"), _P("*|Tempo|", "1/2D"), _P("*|Tempo|", "1/2"), _P("*|Tempo|", "1/2T"), _P("*|Tempo|", "1/4D"), _P("*|Tempo|", "1/4"), _P("*|Tempo|", "1/4T"), _P("*|Tempo|", "1/8D"), _P("*|Tempo|", "1/8"), _P("*|Tempo|", "1/8T"), _P("*|Tempo|", "1/16D"), _P("*|Tempo|", "1/16"), _P("*|Tempo|", "1/16T"), _P("*|Tempo|", "1/32D"), _P("*|Tempo|", "1/32"), _P("*|Tempo|", "1/32T")},
                                      _("Tempo"), _("The Tempo parameter sets the relationship to the global Tempo."))));
    slots.emplace_back((new PA_Integer(284, 4, _("Out level"), _("Sets the overall Output level of this block.")))
                       ->with_min_max(-100, 0)
                       ->with_position(PP_Back)
                       ->with_modifier_at(36));
}

P_Modulation::P_Modulation(const PA_Choice &tag)
    : Polymorphic_Parameter_Collection(tag)
{
}

Parameter_Collection &P_Modulation::dispatch(const Patch &pat)
{
    switch (tag.get(pat)) {
    default:
        assert(false);
    case 0:
        return classic_chorus;
    case 1:
        return advanced_chorus;
    case 2:
        return classic_flanger;
    case 3:
        return advanced_flanger;
    case 4:
        return vibrato;
    }
}

P_Modulation::Classic_Chorus::Classic_Chorus()
{
    slots.emplace_back((new PA_Choice(360, 4,
                                      {".050", ".052", ".053", ".055", ".056", ".058", ".060", ".061", ".063", ".065", ".067", ".069", ".071", ".073", ".075", ".077", ".079", ".082", ".084", ".087", ".089", ".092", ".094", ".097", ".100", ".103", ".106", ".109", ".112", ".115", ".119", ".122", ".126", ".130", ".133", ".137", ".141", ".145", ".150", ".154", ".158", ".163", ".168", ".173", ".178", ".183", ".188", ".194", ".200", ".205", ".211", ".218", ".224", ".230", ".237", ".244", ".251", ".259", ".266", ".274", ".282", ".290", ".299", ".307", ".316", ".325", ".335", ".345", ".355", ".365", ".376", ".387", ".398", ".410", ".422", ".434", ".447", ".460", ".473", ".487", ".501", ".516", ".531", ".546", ".562", ".579", ".596", ".613", ".631", ".649", ".668", ".688", ".708", ".729", ".750", ".772", ".794", ".818", ".841", ".866", ".891", ".917", ".944", ".972", "1.00", "1.03", "1.06", "1.09", "1.12", "1.15", "1.19", "1.22", "1.26", "1.30", "1.33", "1.37", "1.41", "1.45", "1.50", "1.54", "1.58", "1.63", "1.68", "1.73", "1.78", "1.83", "1.88", "1.94", "2.00", "2.05", "2.11", "2.18", "2.24", "2.30", "2.37", "2.44", "2.51", "2.59", "2.66", "2.74", "2.82", "2.90", "2.99", "3.07", "3.16", "3.25", "3.35", "3.45", "3.55", "3.65", "3.76", "3.87", "3.98", "4.10", "4.22", "4.34", "4.47", "4.60", "4.73", "4.87", "5.01", "5.16", "5.31", "5.46", "5.62", "5.79", "5.96", "6.13", "6.31", "6.49", "6.68", "6.88", "7.08", "7.29", "7.50", "7.72", "7.94", "8.18", "8.41", "8.66", "8.91", "9.17", "9.44", "9.72", "10.00", "10.29", "10.59", "10.90", "11.22", "11.55", "11.89", "12.23", "12.59", "12.96", "13.34", "13.72", "14.13", "14.54", "14.96", "15.40", "15.85", "16.31", "16.79", "17.28", "17.78", "18.30", "18.84", "19.39", "19.95"},
                                      _("Speed"), _("The Speed of the Chorus, also known as Rate.")))
                       ->with_modifier_at(80));
    slots.emplace_back((new PA_Integer(364, 4, _("Depth"), _("The Depth parameter specifies the intensity of the Chorus effect - the value represents the amount of modulation.")))
                       ->with_min_max(0, 100)
                       ->with_modifier_at(84));
    slots.emplace_back((new PA_Choice(368, 4,
                                      {_P("*|Tempo|", "Ignored"), _P("*|Tempo|", "1"), _P("*|Tempo|", "1/2D"), _P("*|Tempo|", "1/2"), _P("*|Tempo|", "1/2T"), _P("*|Tempo|", "1/4D"), _P("*|Tempo|", "1/4"), _P("*|Tempo|", "1/4T"), _P("*|Tempo|", "1/8D"), _P("*|Tempo|", "1/8"), _P("*|Tempo|", "1/8T"), _P("*|Tempo|", "1/16D"), _P("*|Tempo|", "1/16"), _P("*|Tempo|", "1/16T"), _P("*|Tempo|", "1/32D"), _P("*|Tempo|", "1/32"), _P("*|Tempo|", "1/32T")},
                                      _("Tempo"), _("The Tempo parameter sets the relationship to the global Tempo."))));
    slots.emplace_back((new PA_Choice(372, 4,
                                      {"19.95", "22.39", "25.12", "28.18", "31.62", "35.48", "39.81", "44.67", "50.12", "56.23", "63.10", "70.79", "79.43", "89.13", "100.0", "112.2", "125.9", "141.3", "158.5", "177.8", "199.5", "223.9", "251.2", "281.8", "316.2", "354.8", "398.1", "446.7", "501.2", "562.3", "631.0", "707.9", "794.3", "891.3", "1.00k", "1.12k", "1.26k", "1.41k", "1.58k", "1.78k", "2.00k", "2.24k", "2.51k", "2.82k", "3.16k", "3.55k", "3.98k", "4.47k", "5.01k", "5.62k", "6.31k", "7.08k", "7.94k", "8.91k", "10.0k", "11.2k", "12.6k", "14.1k", "15.8k", "17.8k", _("Off")},
                                      _("Hi Cut"), _("Reduces the high-end frequencies in the Chorus effect. Try using the Hi Cut parameter as an option if you feel the Chorus effect is too dominant in your sound and turning down the Mix or Out level doesn't give you the dampening of the Chorus effect you are looking for.")))
                       ->with_modifier_at(88));
    slots.emplace_back((new PA_Integer(396, 4, _("Mix"), _("Sets the relation between the dry signal and the applied effect in this block.")))
                       ->with_min_max(0, 100)
                       ->with_position(PP_Back)
                       ->with_modifier_at(92));
    slots.emplace_back((new PA_Integer(400, 4, _("Out level"), _("Sets the overall Output level of this block.")))
                       ->with_min_max(-100, 0)
                       ->with_position(PP_Back)
                       ->with_modifier_at(96));
}

P_Modulation::Advanced_Chorus::Advanced_Chorus()
{
    slots.emplace_back((new PA_Choice(360, 4,
                                      {".050", ".052", ".053", ".055", ".056", ".058", ".060", ".061", ".063", ".065", ".067", ".069", ".071", ".073", ".075", ".077", ".079", ".082", ".084", ".087", ".089", ".092", ".094", ".097", ".100", ".103", ".106", ".109", ".112", ".115", ".119", ".122", ".126", ".130", ".133", ".137", ".141", ".145", ".150", ".154", ".158", ".163", ".168", ".173", ".178", ".183", ".188", ".194", ".200", ".205", ".211", ".218", ".224", ".230", ".237", ".244", ".251", ".259", ".266", ".274", ".282", ".290", ".299", ".307", ".316", ".325", ".335", ".345", ".355", ".365", ".376", ".387", ".398", ".410", ".422", ".434", ".447", ".460", ".473", ".487", ".501", ".516", ".531", ".546", ".562", ".579", ".596", ".613", ".631", ".649", ".668", ".688", ".708", ".729", ".750", ".772", ".794", ".818", ".841", ".866", ".891", ".917", ".944", ".972", "1.00", "1.03", "1.06", "1.09", "1.12", "1.15", "1.19", "1.22", "1.26", "1.30", "1.33", "1.37", "1.41", "1.45", "1.50", "1.54", "1.58", "1.63", "1.68", "1.73", "1.78", "1.83", "1.88", "1.94", "2.00", "2.05", "2.11", "2.18", "2.24", "2.30", "2.37", "2.44", "2.51", "2.59", "2.66", "2.74", "2.82", "2.90", "2.99", "3.07", "3.16", "3.25", "3.35", "3.45", "3.55", "3.65", "3.76", "3.87", "3.98", "4.10", "4.22", "4.34", "4.47", "4.60", "4.73", "4.87", "5.01", "5.16", "5.31", "5.46", "5.62", "5.79", "5.96", "6.13", "6.31", "6.49", "6.68", "6.88", "7.08", "7.29", "7.50", "7.72", "7.94", "8.18", "8.41", "8.66", "8.91", "9.17", "9.44", "9.72", "10.00", "10.29", "10.59", "10.90", "11.22", "11.55", "11.89", "12.23", "12.59", "12.96", "13.34", "13.72", "14.13", "14.54", "14.96", "15.40", "15.85", "16.31", "16.79", "17.28", "17.78", "18.30", "18.84", "19.39", "19.95"},
                                      _("Speed"), _("The Speed of the Chorus, also known as Rate.")))
                       ->with_modifier_at(80));
    slots.emplace_back((new PA_Integer(364, 4, _("Depth"), _("The Depth parameter specifies the intensity of the Chorus effect - the value represents the amount of modulation.")))
                       ->with_min_max(0, 100)
                       ->with_modifier_at(84));
    slots.emplace_back((new PA_Choice(368, 4,
                                      {_P("*|Tempo|", "Ignored"), _P("*|Tempo|", "1"), _P("*|Tempo|", "1/2D"), _P("*|Tempo|", "1/2"), _P("*|Tempo|", "1/2T"), _P("*|Tempo|", "1/4D"), _P("*|Tempo|", "1/4"), _P("*|Tempo|", "1/4T"), _P("*|Tempo|", "1/8D"), _P("*|Tempo|", "1/8"), _P("*|Tempo|", "1/8T"), _P("*|Tempo|", "1/16D"), _P("*|Tempo|", "1/16"), _P("*|Tempo|", "1/16T"), _P("*|Tempo|", "1/32D"), _P("*|Tempo|", "1/32"), _P("*|Tempo|", "1/32T")},
                                      _("Tempo"), _("The Tempo parameter sets the relationship to the global Tempo."))));
    slots.emplace_back((new PA_Choice(372, 4,
                                      {"19.95", "22.39", "25.12", "28.18", "31.62", "35.48", "39.81", "44.67", "50.12", "56.23", "63.10", "70.79", "79.43", "89.13", "100.0", "112.2", "125.9", "141.3", "158.5", "177.8", "199.5", "223.9", "251.2", "281.8", "316.2", "354.8", "398.1", "446.7", "501.2", "562.3", "631.0", "707.9", "794.3", "891.3", "1.00k", "1.12k", "1.26k", "1.41k", "1.58k", "1.78k", "2.00k", "2.24k", "2.51k", "2.82k", "3.16k", "3.55k", "3.98k", "4.47k", "5.01k", "5.62k", "6.31k", "7.08k", "7.94k", "8.91k", "10.0k", "11.2k", "12.6k", "14.1k", "15.8k", "17.8k", _("Off")},
                                      _("Hi Cut"), _("Reduces the high-end frequencies in the Chorus effect. Try using the Hi Cut parameter as an option if you feel the Chorus effect is too dominant in your sound and turning down the Mix or Out level doesn't give you the dampening of the Chorus effect you are looking for.")))
                       ->with_modifier_at(88));
    slots.emplace_back((new PA_Integer(384, 4, _("Delay"), _("Chorus is basically a Delay being modulated by an LFO. This parameter makes it possible to change the length of that Delay. A typical chorus uses Delays at approximately 10 ms.")))
                       ->with_min_max(1, 500));
    slots.emplace_back((new PA_Choice(388, 4,
                                      {_("Off"), _("On")},
                                      _("Gold Ratio"), _("When Speed is increased the Depth must be decreased to achieve the same amount of perceived Chorusing effect. When Golden Ratio is \"On\" this value is automatically calculated."))));
    slots.emplace_back((new PA_Choice(392, 4,
                                      {_("Off"), _("On")},
                                      _("Phase Rev"/*Phase Reverse*/), _("Reverses the processed Chorus signal in the right channel. This gives a very wide Chorus effect and a less defined sound."))));
    slots.emplace_back((new PA_Integer(396, 4, _("Mix"), _("Sets the relation between the dry signal and the applied effect in this block.")))
                       ->with_min_max(0, 100)
                       ->with_position(PP_Back)
                       ->with_modifier_at(92));
    slots.emplace_back((new PA_Integer(400, 4, _("Out level"), _("Sets the overall Output level of this block.")))
                       ->with_min_max(-100, 0)
                       ->with_position(PP_Back)
                       ->with_modifier_at(96));
}

P_Modulation::Classic_Flanger::Classic_Flanger()
{
    slots.emplace_back((new PA_Choice(360, 4,
                                      {".050", ".052", ".053", ".055", ".056", ".058", ".060", ".061", ".063", ".065", ".067", ".069", ".071", ".073", ".075", ".077", ".079", ".082", ".084", ".087", ".089", ".092", ".094", ".097", ".100", ".103", ".106", ".109", ".112", ".115", ".119", ".122", ".126", ".130", ".133", ".137", ".141", ".145", ".150", ".154", ".158", ".163", ".168", ".173", ".178", ".183", ".188", ".194", ".200", ".205", ".211", ".218", ".224", ".230", ".237", ".244", ".251", ".259", ".266", ".274", ".282", ".290", ".299", ".307", ".316", ".325", ".335", ".345", ".355", ".365", ".376", ".387", ".398", ".410", ".422", ".434", ".447", ".460", ".473", ".487", ".501", ".516", ".531", ".546", ".562", ".579", ".596", ".613", ".631", ".649", ".668", ".688", ".708", ".729", ".750", ".772", ".794", ".818", ".841", ".866", ".891", ".917", ".944", ".972", "1.00", "1.03", "1.06", "1.09", "1.12", "1.15", "1.19", "1.22", "1.26", "1.30", "1.33", "1.37", "1.41", "1.45", "1.50", "1.54", "1.58", "1.63", "1.68", "1.73", "1.78", "1.83", "1.88", "1.94", "2.00", "2.05", "2.11", "2.18", "2.24", "2.30", "2.37", "2.44", "2.51", "2.59", "2.66", "2.74", "2.82", "2.90", "2.99", "3.07", "3.16", "3.25", "3.35", "3.45", "3.55", "3.65", "3.76", "3.87", "3.98", "4.10", "4.22", "4.34", "4.47", "4.60", "4.73", "4.87", "5.01", "5.16", "5.31", "5.46", "5.62", "5.79", "5.96", "6.13", "6.31", "6.49", "6.68", "6.88", "7.08", "7.29", "7.50", "7.72", "7.94", "8.18", "8.41", "8.66", "8.91", "9.17", "9.44", "9.72", "10.00", "10.29", "10.59", "10.90", "11.22", "11.55", "11.89", "12.23", "12.59", "12.96", "13.34", "13.72", "14.13", "14.54", "14.96", "15.40", "15.85", "16.31", "16.79", "17.28", "17.78", "18.30", "18.84", "19.39", "19.95"},
                                      _("Speed"), _("The Speed of the Flanger, also known as Rate.")))
                       ->with_modifier_at(80));
    slots.emplace_back((new PA_Integer(364, 4, _("Depth"), _("Adjusts the Depth of the Flanger, also known as Intensity. The value represents the amount of modulation applied.")))
                       ->with_min_max(0, 100)
                       ->with_modifier_at(84));
    slots.emplace_back((new PA_Choice(368, 4,
                                      {_P("*|Tempo|", "Ignored"), _P("*|Tempo|", "1"), _P("*|Tempo|", "1/2D"), _P("*|Tempo|", "1/2"), _P("*|Tempo|", "1/2T"), _P("*|Tempo|", "1/4D"), _P("*|Tempo|", "1/4"), _P("*|Tempo|", "1/4T"), _P("*|Tempo|", "1/8D"), _P("*|Tempo|", "1/8"), _P("*|Tempo|", "1/8T"), _P("*|Tempo|", "1/16D"), _P("*|Tempo|", "1/16"), _P("*|Tempo|", "1/16T"), _P("*|Tempo|", "1/32D"), _P("*|Tempo|", "1/32"), _P("*|Tempo|", "1/32T")},
                                      _("Tempo"), _("The Tempo parameter sets the relationship to the global Tempo."))));
    slots.emplace_back((new PA_Choice(372, 4,
                                      {"19.95", "22.39", "25.12", "28.18", "31.62", "35.48", "39.81", "44.67", "50.12", "56.23", "63.10", "70.79", "79.43", "89.13", "100.0", "112.2", "125.9", "141.3", "158.5", "177.8", "199.5", "223.9", "251.2", "281.8", "316.2", "354.8", "398.1", "446.7", "501.2", "562.3", "631.0", "707.9", "794.3", "891.3", "1.00k", "1.12k", "1.26k", "1.41k", "1.58k", "1.78k", "2.00k", "2.24k", "2.51k", "2.82k", "3.16k", "3.55k", "3.98k", "4.47k", "5.01k", "5.62k", "6.31k", "7.08k", "7.94k", "8.91k", "10.0k", "11.2k", "12.6k", "14.1k", "15.8k", "17.8k", _("Off")},
                                      _("Hi Cut"), _("Reduces the high-end frequencies in the Flanger effect. Try using the Hi Cut parameter as an option if you feel the Flanger effect is too dominant in your sound and turning down the Mix or Out level doesn't give you the dampening of the Flanger effect you are looking for.")))
                       ->with_modifier_at(88));
    slots.emplace_back((new PA_Integer(376, 4, _("Feedback"), _("Controls the amount of Feedback/Resonance of the short modulated Delay that causes the Flange effect.")))
                       ->with_min_max(-100, 100)
                       ->with_modifier_at(92));
    slots.emplace_back((new PA_Choice(380, 4,
                                      {"19.95", "22.39", "25.12", "28.18", "31.62", "35.48", "39.81", "44.67", "50.12", "56.23", "63.10", "70.79", "79.43", "89.13", "100.0", "112.2", "125.9", "141.3", "158.5", "177.8", "199.5", "223.9", "251.2", "281.8", "316.2", "354.8", "398.1", "446.7", "501.2", "562.3", "631.0", "707.9", "794.3", "891.3", "1.00k", "1.12k", "1.26k", "1.41k", "1.58k", "1.78k", "2.00k", "2.24k", "2.51k", "2.82k", "3.16k", "3.55k", "3.98k", "4.47k", "5.01k", "5.62k", "6.31k", "7.08k", "7.94k", "8.91k", "10.0k", "11.2k", "12.6k", "14.1k", "15.8k", "17.8k", _("Off")},
                                      _("FB Hi Cut"), _("A parameter than can attenuate the high-end frequencies of the resonance created via the Feedback parameter.")))
                       ->with_modifier_at(96));
    slots.emplace_back((new PA_Integer(396, 4, _("Mix"), _("Sets the relation between the dry signal and the applied effect in this block.")))
                       ->with_min_max(0, 100)
                       ->with_position(PP_Back)
                       ->with_modifier_at(100));
    slots.emplace_back((new PA_Integer(400, 4, _("Out level"), _("Sets the overall Output level of this block.")))
                       ->with_min_max(-100, 0)
                       ->with_position(PP_Back)
                       ->with_modifier_at(104));
}

P_Modulation::Advanced_Flanger::Advanced_Flanger()
{
    slots.emplace_back((new PA_Choice(360, 4,
                                      {".050", ".052", ".053", ".055", ".056", ".058", ".060", ".061", ".063", ".065", ".067", ".069", ".071", ".073", ".075", ".077", ".079", ".082", ".084", ".087", ".089", ".092", ".094", ".097", ".100", ".103", ".106", ".109", ".112", ".115", ".119", ".122", ".126", ".130", ".133", ".137", ".141", ".145", ".150", ".154", ".158", ".163", ".168", ".173", ".178", ".183", ".188", ".194", ".200", ".205", ".211", ".218", ".224", ".230", ".237", ".244", ".251", ".259", ".266", ".274", ".282", ".290", ".299", ".307", ".316", ".325", ".335", ".345", ".355", ".365", ".376", ".387", ".398", ".410", ".422", ".434", ".447", ".460", ".473", ".487", ".501", ".516", ".531", ".546", ".562", ".579", ".596", ".613", ".631", ".649", ".668", ".688", ".708", ".729", ".750", ".772", ".794", ".818", ".841", ".866", ".891", ".917", ".944", ".972", "1.00", "1.03", "1.06", "1.09", "1.12", "1.15", "1.19", "1.22", "1.26", "1.30", "1.33", "1.37", "1.41", "1.45", "1.50", "1.54", "1.58", "1.63", "1.68", "1.73", "1.78", "1.83", "1.88", "1.94", "2.00", "2.05", "2.11", "2.18", "2.24", "2.30", "2.37", "2.44", "2.51", "2.59", "2.66", "2.74", "2.82", "2.90", "2.99", "3.07", "3.16", "3.25", "3.35", "3.45", "3.55", "3.65", "3.76", "3.87", "3.98", "4.10", "4.22", "4.34", "4.47", "4.60", "4.73", "4.87", "5.01", "5.16", "5.31", "5.46", "5.62", "5.79", "5.96", "6.13", "6.31", "6.49", "6.68", "6.88", "7.08", "7.29", "7.50", "7.72", "7.94", "8.18", "8.41", "8.66", "8.91", "9.17", "9.44", "9.72", "10.00", "10.29", "10.59", "10.90", "11.22", "11.55", "11.89", "12.23", "12.59", "12.96", "13.34", "13.72", "14.13", "14.54", "14.96", "15.40", "15.85", "16.31", "16.79", "17.28", "17.78", "18.30", "18.84", "19.39", "19.95"},
                                      _("Speed"), _("The Speed of the Flanger, also known as Rate.")))
                       ->with_modifier_at(80));
    slots.emplace_back((new PA_Integer(364, 4, _("Depth"), _("Adjusts the Depth of the Flanger, also known as Intensity. The value represents the amount of modulation applied.")))
                       ->with_min_max(0, 100)
                       ->with_modifier_at(84));
    slots.emplace_back((new PA_Choice(368, 4,
                                      {_P("*|Tempo|", "Ignored"), _P("*|Tempo|", "1"), _P("*|Tempo|", "1/2D"), _P("*|Tempo|", "1/2"), _P("*|Tempo|", "1/2T"), _P("*|Tempo|", "1/4D"), _P("*|Tempo|", "1/4"), _P("*|Tempo|", "1/4T"), _P("*|Tempo|", "1/8D"), _P("*|Tempo|", "1/8"), _P("*|Tempo|", "1/8T"), _P("*|Tempo|", "1/16D"), _P("*|Tempo|", "1/16"), _P("*|Tempo|", "1/16T"), _P("*|Tempo|", "1/32D"), _P("*|Tempo|", "1/32"), _P("*|Tempo|", "1/32T")},
                                      _("Tempo"), _("The Tempo parameter sets the relationship to the global Tempo."))));
    slots.emplace_back((new PA_Choice(372, 4,
                                      {"19.95", "22.39", "25.12", "28.18", "31.62", "35.48", "39.81", "44.67", "50.12", "56.23", "63.10", "70.79", "79.43", "89.13", "100.0", "112.2", "125.9", "141.3", "158.5", "177.8", "199.5", "223.9", "251.2", "281.8", "316.2", "354.8", "398.1", "446.7", "501.2", "562.3", "631.0", "707.9", "794.3", "891.3", "1.00k", "1.12k", "1.26k", "1.41k", "1.58k", "1.78k", "2.00k", "2.24k", "2.51k", "2.82k", "3.16k", "3.55k", "3.98k", "4.47k", "5.01k", "5.62k", "6.31k", "7.08k", "7.94k", "8.91k", "10.0k", "11.2k", "12.6k", "14.1k", "15.8k", "17.8k", _("Off")},
                                      _("Hi Cut"), _("Reduces the high-end frequencies in the Flanger effect. Try using the Hi Cut parameter as an option if you feel the Flanger effect is too dominant in your sound and turning down the Mix or Out level doesn't give you the dampening of the Flanger effect you are looking for.")))
                       ->with_modifier_at(88));
    slots.emplace_back((new PA_Integer(376, 4, _("Feedback"), _("Controls the amount of Feedback/Resonance of the short modulated Delay that causes the Flange effect.")))
                       ->with_min_max(-100, 100)
                       ->with_modifier_at(92));
    slots.emplace_back((new PA_Choice(380, 4,
                                      {"19.95", "22.39", "25.12", "28.18", "31.62", "35.48", "39.81", "44.67", "50.12", "56.23", "63.10", "70.79", "79.43", "89.13", "100.0", "112.2", "125.9", "141.3", "158.5", "177.8", "199.5", "223.9", "251.2", "281.8", "316.2", "354.8", "398.1", "446.7", "501.2", "562.3", "631.0", "707.9", "794.3", "891.3", "1.00k", "1.12k", "1.26k", "1.41k", "1.58k", "1.78k", "2.00k", "2.24k", "2.51k", "2.82k", "3.16k", "3.55k", "3.98k", "4.47k", "5.01k", "5.62k", "6.31k", "7.08k", "7.94k", "8.91k", "10.0k", "11.2k", "12.6k", "14.1k", "15.8k", "17.8k", _("Off")},
                                      _("FB Hi Cut"), _("A parameter than can attenuate the high-end frequencies of the resonance created via the Feedback parameter.")))
                       ->with_modifier_at(96));
    slots.emplace_back((new PA_Integer(384, 4, _("Delay"), _("Flanger is basically a Delay being modulated by an LFO. This parameter makes it possible to change the length of that Delay. A typical flanger uses Delays at approximately 5 ms.")))
                       ->with_min_max(1, 500));
    slots.emplace_back((new PA_Choice(388, 4,
                                      {_("Off"), _("On")},
                                      _("Gold Ratio"), _("When Speed is increased the Depth must be decreased to achieve the same amount of perceived Flanging effect. When Golden Ratio is \"On\" this value is automatically calculated."))));
    slots.emplace_back((new PA_Choice(392, 4,
                                      {_("Off"), _("On")},
                                      _("Phase Rev"/*Phase Reverse*/), _("Reverses the processed Flanger signal in the right channel. This gives a very wide Flanger effect and a less defined sound."))));
    slots.emplace_back((new PA_Integer(396, 4, _("Mix"), _("Sets the relation between the dry signal and the applied effect in this block.")))
                       ->with_min_max(0, 100)
                       ->with_position(PP_Back)
                       ->with_modifier_at(100));
    slots.emplace_back((new PA_Integer(400, 4, _("Out level"), _("Sets the overall Output level of this block.")))
                       ->with_min_max(-100, 0)
                       ->with_position(PP_Back)
                       ->with_modifier_at(104));
}

P_Modulation::Vibrato::Vibrato()
{
    slots.emplace_back((new PA_Choice(360, 4,
                                      {".050", ".052", ".053", ".055", ".056", ".058", ".060", ".061", ".063", ".065", ".067", ".069", ".071", ".073", ".075", ".077", ".079", ".082", ".084", ".087", ".089", ".092", ".094", ".097", ".100", ".103", ".106", ".109", ".112", ".115", ".119", ".122", ".126", ".130", ".133", ".137", ".141", ".145", ".150", ".154", ".158", ".163", ".168", ".173", ".178", ".183", ".188", ".194", ".200", ".205", ".211", ".218", ".224", ".230", ".237", ".244", ".251", ".259", ".266", ".274", ".282", ".290", ".299", ".307", ".316", ".325", ".335", ".345", ".355", ".365", ".376", ".387", ".398", ".410", ".422", ".434", ".447", ".460", ".473", ".487", ".501", ".516", ".531", ".546", ".562", ".579", ".596", ".613", ".631", ".649", ".668", ".688", ".708", ".729", ".750", ".772", ".794", ".818", ".841", ".866", ".891", ".917", ".944", ".972", "1.00", "1.03", "1.06", "1.09", "1.12", "1.15", "1.19", "1.22", "1.26", "1.30", "1.33", "1.37", "1.41", "1.45", "1.50", "1.54", "1.58", "1.63", "1.68", "1.73", "1.78", "1.83", "1.88", "1.94", "2.00", "2.05", "2.11", "2.18", "2.24", "2.30", "2.37", "2.44", "2.51", "2.59", "2.66", "2.74", "2.82", "2.90", "2.99", "3.07", "3.16", "3.25", "3.35", "3.45", "3.55", "3.65", "3.76", "3.87", "3.98", "4.10", "4.22", "4.34", "4.47", "4.60", "4.73", "4.87", "5.01", "5.16", "5.31", "5.46", "5.62", "5.79", "5.96", "6.13", "6.31", "6.49", "6.68", "6.88", "7.08", "7.29", "7.50", "7.72", "7.94", "8.18", "8.41", "8.66", "8.91", "9.17", "9.44", "9.72", "10.00", "10.29", "10.59", "10.90", "11.22", "11.55", "11.89", "12.23", "12.59", "12.96", "13.34", "13.72", "14.13", "14.54", "14.96", "15.40", "15.85", "16.31", "16.79", "17.28", "17.78", "18.30", "18.84", "19.39", "19.95"},
                                      _("Speed"), _("The Speed of the Vibrato, also known as Rate.")))
                       ->with_modifier_at(80));
    slots.emplace_back((new PA_Integer(364, 4, _("Depth"), _("The amount of Pitch modulation applied.")))
                       ->with_min_max(0, 100)
                       ->with_modifier_at(84));
    slots.emplace_back((new PA_Choice(368, 4,
                                      {_P("*|Tempo|", "Ignored"), _P("*|Tempo|", "1"), _P("*|Tempo|", "1/2D"), _P("*|Tempo|", "1/2"), _P("*|Tempo|", "1/2T"), _P("*|Tempo|", "1/4D"), _P("*|Tempo|", "1/4"), _P("*|Tempo|", "1/4T"), _P("*|Tempo|", "1/8D"), _P("*|Tempo|", "1/8"), _P("*|Tempo|", "1/8T"), _P("*|Tempo|", "1/16D"), _P("*|Tempo|", "1/16"), _P("*|Tempo|", "1/16T"), _P("*|Tempo|", "1/32D"), _P("*|Tempo|", "1/32"), _P("*|Tempo|", "1/32T")},
                                      _("Tempo"), _("The Tempo parameter sets the relationship to the global Tempo."))));
    slots.emplace_back((new PA_Choice(372, 4,
                                      {"19.95", "22.39", "25.12", "28.18", "31.62", "35.48", "39.81", "44.67", "50.12", "56.23", "63.10", "70.79", "79.43", "89.13", "100.0", "112.2", "125.9", "141.3", "158.5", "177.8", "199.5", "223.9", "251.2", "281.8", "316.2", "354.8", "398.1", "446.7", "501.2", "562.3", "631.0", "707.9", "794.3", "891.3", "1.00k", "1.12k", "1.26k", "1.41k", "1.58k", "1.78k", "2.00k", "2.24k", "2.51k", "2.82k", "3.16k", "3.55k", "3.98k", "4.47k", "5.01k", "5.62k", "6.31k", "7.08k", "7.94k", "8.91k", "10.0k", "11.2k", "12.6k", "14.1k", "15.8k", "17.8k", _("Off")},
                                      _("Hi Cut"), _("Determines the frequency above which the Hi Cut filter will attenuate the high-end frequencies of the generated effect. Hi Cut filters can be used to give a less dominant effect even at high mix levels.")))
                       ->with_modifier_at(88));
    slots.emplace_back((new PA_Integer(400, 4, _("Out level"), _("Sets the overall Output level of this block.")))
                       ->with_min_max(-100, 0)
                       ->with_position(PP_Back)
                       ->with_modifier_at(92));
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
                                      {_("Auto filter"), _("Resonance filter"), _("Vintage phaser"), _("Smooth phaser"), _("Tremolo"), _("Panner")},
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

    slots.emplace_back((new PA_Integer(160, 4,
                                      _("Tap tempo"), _("Sets the Tap Tempo.")))
                       ->with_min_max(100, 3000));

    slots.emplace_back((new PA_Bits(156, 4,
                                    9, 1,
                                    _("Relay 1"), _("Relay 1")))
                       ->with_min_max(0, 1));
    slots.emplace_back((new PA_Bits(156, 4,
                                    10, 1,
                                    _("Relay 2"), _("Relay 2")))
                       ->with_min_max(0, 1));
    slots.emplace_back((new PA_Bits(156, 4,
                                    0, 2,
                                    _("Routing"), _("Routing : Serial is routing the effects blocks in serial. Semi parallel is routing delay and reverb in parallel. Parallel is routing pitch, Chorus flanger, delay and reverb.")))
                       ->with_min_max(0, 2)
                       ->with_string_fn([](int v) -> std::string {
                                            switch (v) {
                                            case 0: return _("Serial");
                                            case 1: return _("Semi Parallel");
                                            case 2: return _("Parallel");
                                            default: assert(false); return "";
                                            }
                                        }));
    slots.emplace_back((new PA_Bits(156, 4,
                                    2, 7,
                                    _("Out level"), _("Sets preset output level.")))
                       ->with_min_max(-100, 0)
                       ->with_offset(100));

    pitch.reset(new P_Pitch(type_pitch()));
    delay.reset(new P_Delay(type_delay()));
    filter.reset(new P_Filter(type_filter()));
    modulation.reset(new P_Modulation(type_modulation()));
}
