//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <string>
#include <vector>
#include <memory>
class Patch;

enum Parameter_Type {
    PT_Integer, PT_Boolean, PT_Choice, PT_Bits,
};

class Parameter_Access {
public:
    Parameter_Access(const char *name, const char *description)
        : name(name), description(description) {}
    virtual ~Parameter_Access() {}

    virtual Parameter_Type type() const = 0;
    virtual int get(const Patch &pat) const = 0;
    virtual void set(Patch &pat, int value) = 0;
    virtual int min() const = 0;
    virtual int max() const = 0;
    virtual std::string to_string(int value) const;

    const char *name = nullptr;
    const char *description = nullptr;
};

class Parameter_Collection {
public:
    virtual ~Parameter_Collection() {}
    std::vector<std::unique_ptr<Parameter_Access>> slots;
};

#define DEFPARAMETER(i, t, x)                                   \
    t &x() { return static_cast<t &>(*this->slots[(i)]); }

///

class PA_Integer : public Parameter_Access {
public:
    explicit PA_Integer(unsigned index, unsigned size, const char *name, const char *description)
        : Parameter_Access(name, description), index(index), size(size) {}
    PA_Integer *with_min_max(int vmin, int vmax);

    virtual Parameter_Type type() const override { return PT_Integer; }
    virtual int get(const Patch &pat) const override;
    virtual void set(Patch &pat, int value) override;
    int min() const override { return vmin; }
    int max() const override { return vmax; }
    int clamp(int value) const;

    unsigned index = 0;
    unsigned size = 0;
    int vmin = 0;
    int vmax = 1;
};

class PA_Boolean : public Parameter_Access {
public:
    explicit PA_Boolean(unsigned index, unsigned size, const char *name, const char *description)
        : Parameter_Access(name, description), index(index), size(size) {}
    PA_Boolean *with_inversion();

    Parameter_Type type() const override { return PT_Boolean; }
    int get(const Patch &pat) const override;
    void set(Patch &pat, int value) override;
    int min() const override { return 0; }
    int max() const override { return 1; }
    std::string to_string(int value) const override;

    unsigned index = 0;
    unsigned size = 0;
    bool inverted = false;
};

class PA_Choice : public Parameter_Access {
public:
    explicit PA_Choice(unsigned index, unsigned size, std::vector<const char *> values, const char *name, const char *description)
        : Parameter_Access(name, description),
          values(std::move(values)), index(index), size(size) {}
    PA_Choice *with_offset(int offset);

    Parameter_Type type() const override { return PT_Choice; }
    int get(const Patch &pat) const override;
    void set(Patch &pat, int value) override;
    int min() const override { return 0; }
    int max() const override { return values.size() - 1; }
    int clamp(int value) const;
    std::string to_string(int value) const override;

    unsigned index = 0;
    unsigned size = 0;
    std::vector<const char *> values;
    int offset = 0;
};

class PA_Bits : public Parameter_Access {
public:
    explicit PA_Bits(unsigned index, unsigned size, unsigned bit_offset, unsigned bit_size, const char *name, const char *description)
        : Parameter_Access(name, description),
          bit_offset(bit_offset), bit_size(bit_size), index(index), size(size) {}
    PA_Bits *with_min_max(int vmin, int vmax);

    virtual Parameter_Type type() const override { return PT_Bits; }
    int get(const Patch &pat) const override;
    void set(Patch &pat, int value) override;
    int min() const override { return vmin; }
    int max() const override { return vmax; }
    int clamp(int value) const;

    unsigned bit_offset = 0;
    unsigned bit_size = 0;
    unsigned index = 0;
    unsigned size = 0;
    int vmin = 0;
    int vmax = 1;
};

///

class Polymorphic_Parameter_Collection {
public:
    explicit Polymorphic_Parameter_Collection(const PA_Choice &tag)
        : tag(tag) {}
    virtual Parameter_Collection &dispatch(const Patch &pat) = 0;

    const PA_Choice &tag;
};

///

class P_Compressor : public Parameter_Collection {
public:
    P_Compressor();

    DEFPARAMETER(0, PA_Integer, threshold)
    DEFPARAMETER(1, PA_Choice, ratio)
    DEFPARAMETER(2, PA_Choice, attack)
    DEFPARAMETER(3, PA_Choice, release)
    DEFPARAMETER(4, PA_Integer, gain)
};

class P_Equalizer : public Parameter_Collection {
public:
    P_Equalizer();

    DEFPARAMETER(0, PA_Choice, frequency1)
    DEFPARAMETER(1, PA_Choice, frequency2)
    DEFPARAMETER(2, PA_Choice, frequency3)
    DEFPARAMETER(3, PA_Integer, gain1)
    DEFPARAMETER(4, PA_Integer, gain2)
    DEFPARAMETER(5, PA_Integer, gain3)
    DEFPARAMETER(6, PA_Choice, width1)
    DEFPARAMETER(7, PA_Choice, width2)
    DEFPARAMETER(8, PA_Choice, width3)
};

class P_Noise_Gate : public Parameter_Collection {
public:
    P_Noise_Gate();

    DEFPARAMETER(0, PA_Choice, mode);
    DEFPARAMETER(1, PA_Integer, threshold);
    DEFPARAMETER(2, PA_Integer, max_damping);
    DEFPARAMETER(3, PA_Integer, release);
};

class P_Reverb : public Parameter_Collection {
public:
    P_Reverb();

    DEFPARAMETER(0, PA_Integer, decay)
    DEFPARAMETER(1, PA_Integer, pre_delay)
    DEFPARAMETER(2, PA_Choice, shape)
    DEFPARAMETER(3, PA_Choice, size)
    DEFPARAMETER(4, PA_Choice, hi_color)
    DEFPARAMETER(5, PA_Integer, hi_factor)
    DEFPARAMETER(6, PA_Choice, lo_color)
    DEFPARAMETER(7, PA_Integer, lo_factor)
    DEFPARAMETER(8, PA_Integer, room_level)
    DEFPARAMETER(9, PA_Integer, reverb_level)
    DEFPARAMETER(10, PA_Integer, diffuse)
    DEFPARAMETER(11, PA_Integer, mix)
    DEFPARAMETER(12, PA_Integer, out_level)
};

class P_Pitch : public Polymorphic_Parameter_Collection {
public:
    explicit P_Pitch(const PA_Choice &tag);

    Parameter_Collection &dispatch(const Patch &pat) override;

    class Detune : public Parameter_Collection {
    public:
        Detune();

        DEFPARAMETER(0, PA_Integer, voice1);
        DEFPARAMETER(1, PA_Integer, voice2);
        DEFPARAMETER(2, PA_Integer, delay1);
        DEFPARAMETER(3, PA_Integer, delay2);
        DEFPARAMETER(4, PA_Integer, mix);
        DEFPARAMETER(5, PA_Integer, out_level);
    } detune;

    class Whammy : public Parameter_Collection {
    public:
        Whammy();

        DEFPARAMETER(0, PA_Integer, pitch);
        DEFPARAMETER(1, PA_Choice, direction);
        DEFPARAMETER(2, PA_Choice, range);
        DEFPARAMETER(3, PA_Integer, out_level);
    } whammy;

    class Octaver : public Parameter_Collection {
    public:
        Octaver();

        DEFPARAMETER(0, PA_Choice, direction);
        DEFPARAMETER(1, PA_Choice, range);
        DEFPARAMETER(2, PA_Integer, mix);
        DEFPARAMETER(3, PA_Integer, out_level);
    } octaver;

    class Shifter : public Parameter_Collection {
    public:
        Shifter();

        DEFPARAMETER(0, PA_Integer, voice1);
        DEFPARAMETER(1, PA_Integer, voice2);
        DEFPARAMETER(2, PA_Integer, pan1);
        DEFPARAMETER(3, PA_Integer, pan2);
        DEFPARAMETER(4, PA_Integer, delay1);
        DEFPARAMETER(5, PA_Integer, delay2);
        DEFPARAMETER(6, PA_Integer, feedback1);
        DEFPARAMETER(7, PA_Integer, feedback2);
        DEFPARAMETER(8, PA_Integer, level1);
        DEFPARAMETER(9, PA_Integer, level2);
        DEFPARAMETER(10, PA_Integer, mix);
        DEFPARAMETER(11, PA_Integer, out_level);
    } shifter;
};

class P_Delay : public Polymorphic_Parameter_Collection {
public:
    explicit P_Delay(const PA_Choice &tag);

    Parameter_Collection &dispatch(const Patch &pat) override;

    class Ping_Pong : public Parameter_Collection {
    public:
        Ping_Pong();

        DEFPARAMETER(0, PA_Integer, delay)
        DEFPARAMETER(1, PA_Choice, tempo)
        DEFPARAMETER(2, PA_Integer, width)
        DEFPARAMETER(3, PA_Integer, feedback)
        DEFPARAMETER(4, PA_Choice, fb_hi_cut)
        DEFPARAMETER(5, PA_Choice, fb_lo_cut)
        DEFPARAMETER(6, PA_Integer, mix)
        DEFPARAMETER(7, PA_Integer, out_level)
    } ping_pong;

    class Dynamic : public Parameter_Collection {
    public:
        Dynamic();

        DEFPARAMETER(0, PA_Integer, delay)
        DEFPARAMETER(1, PA_Choice, tempo)
        DEFPARAMETER(2, PA_Integer, feedback)
        DEFPARAMETER(3, PA_Choice, fb_hi_cut)
        DEFPARAMETER(4, PA_Choice, fb_lo_cut)
        DEFPARAMETER(5, PA_Integer, offset)
        DEFPARAMETER(6, PA_Integer, sensitivity)
        DEFPARAMETER(7, PA_Integer, damping)
        DEFPARAMETER(8, PA_Choice, release)
        DEFPARAMETER(9, PA_Integer, mix)
        DEFPARAMETER(10, PA_Integer, out_level)
    } dynamic;

    class Dual : public Parameter_Collection {
    public:
        Dual();

        DEFPARAMETER(0, PA_Integer, delay1)
        DEFPARAMETER(1, PA_Integer, delay2)
        DEFPARAMETER(2, PA_Choice, tempo1)
        DEFPARAMETER(3, PA_Choice, tempo2)
        DEFPARAMETER(4, PA_Integer, feedback1)
        DEFPARAMETER(5, PA_Integer, feedback2)
        DEFPARAMETER(6, PA_Choice, fb_hi_cut)
        DEFPARAMETER(7, PA_Choice, fb_lo_cut)
        DEFPARAMETER(8, PA_Integer, pan1)
        DEFPARAMETER(9, PA_Integer, pan2)
        DEFPARAMETER(10, PA_Integer, mix)
        DEFPARAMETER(11, PA_Integer, out_level)
    } dual;
};

class P_Filter : public Polymorphic_Parameter_Collection {
public:
    explicit P_Filter(const PA_Choice &tag);

    Parameter_Collection &dispatch(const Patch &pat) override;

    class Auto_Resonance : public Parameter_Collection {
    public:
        Auto_Resonance();

        DEFPARAMETER(0, PA_Choice, order)
        DEFPARAMETER(1, PA_Integer, sensitivity)
        DEFPARAMETER(2, PA_Choice, response)
        DEFPARAMETER(3, PA_Choice, frequency_max)
        DEFPARAMETER(4, PA_Integer, mix)
        DEFPARAMETER(4, PA_Integer, out_level)
    } auto_resonance;

    class Resonance : public Parameter_Collection {
    public:
        Resonance();

        DEFPARAMETER(0, PA_Choice, order)
        DEFPARAMETER(1, PA_Choice, hi_cut)
        DEFPARAMETER(2, PA_Integer, hi_resonance)
        DEFPARAMETER(3, PA_Integer, mix)
        DEFPARAMETER(4, PA_Integer, out_level)
    } resonance;

    class Vintage_Phaser : public Parameter_Collection {
    public:
        Vintage_Phaser();

        DEFPARAMETER(0, PA_Choice, speed)
        DEFPARAMETER(1, PA_Integer, depth)
        DEFPARAMETER(2, PA_Choice, tempo)
        DEFPARAMETER(3, PA_Integer, feedback)
        DEFPARAMETER(4, PA_Choice, range)
        DEFPARAMETER(5, PA_Boolean, phase_reverse)
        DEFPARAMETER(6, PA_Integer, mix)
        DEFPARAMETER(7, PA_Integer, out_level)
    } vintage_phaser;

    class Smooth_Phaser : public Parameter_Collection {
    public:
        Smooth_Phaser();

        DEFPARAMETER(0, PA_Choice, speed)
        DEFPARAMETER(1, PA_Integer, depth)
        DEFPARAMETER(2, PA_Choice, tempo)
        DEFPARAMETER(3, PA_Integer, feedback)
        DEFPARAMETER(4, PA_Choice, range)
        DEFPARAMETER(5, PA_Boolean, phase_reverse)
        DEFPARAMETER(6, PA_Integer, mix)
        DEFPARAMETER(7, PA_Integer, out_level)
    } smooth_phaser;

    class Tremolo : public Parameter_Collection {
    public:
        Tremolo();

        DEFPARAMETER(0, PA_Choice, speed)
        DEFPARAMETER(1, PA_Integer, depth)
        DEFPARAMETER(2, PA_Choice, tempo)
        DEFPARAMETER(3, PA_Integer, lfo_pulse_width)
        DEFPARAMETER(4, PA_Choice, hi_cut)
        DEFPARAMETER(5, PA_Choice, type)
        DEFPARAMETER(6, PA_Integer, out_level)
    } tremolo;

    class Panner : public Parameter_Collection {
    public:
        Panner();

        DEFPARAMETER(0, PA_Choice, speed)
        DEFPARAMETER(1, PA_Integer, width)
        DEFPARAMETER(2, PA_Choice, tempo)
        DEFPARAMETER(3, PA_Integer, out_level)
    } panner;
};

class P_Modulation : public Polymorphic_Parameter_Collection {
public:
    explicit P_Modulation(const PA_Choice &tag);

    Parameter_Collection &dispatch(const Patch &pat) override;

    class Classic_Chorus : public Parameter_Collection {
    public:
        Classic_Chorus();

        DEFPARAMETER(0, PA_Choice, speed)
        DEFPARAMETER(1, PA_Integer, depth)
        DEFPARAMETER(2, PA_Choice, tempo)
        DEFPARAMETER(3, PA_Choice, hi_cut)
        DEFPARAMETER(4, PA_Integer, mix)
        DEFPARAMETER(5, PA_Integer, out_level)
    } classic_chorus;

    class Advanced_Chorus : public Parameter_Collection {
    public:
        Advanced_Chorus();

        DEFPARAMETER(0, PA_Choice, speed)
        DEFPARAMETER(1, PA_Integer, depth)
        DEFPARAMETER(2, PA_Choice, tempo)
        DEFPARAMETER(3, PA_Choice, hi_cut)
        DEFPARAMETER(4, PA_Integer, delay)
        DEFPARAMETER(5, PA_Choice, gold_ratio)
        DEFPARAMETER(6, PA_Choice, phase_reverse)
        DEFPARAMETER(7, PA_Integer, mix)
        DEFPARAMETER(8, PA_Integer, out_level)
    } advanced_chorus;

    class Classic_Flanger : public Parameter_Collection {
    public:
        Classic_Flanger();

        DEFPARAMETER(0, PA_Choice, speed)
        DEFPARAMETER(1, PA_Integer, depth)
        DEFPARAMETER(2, PA_Choice, tempo)
        DEFPARAMETER(3, PA_Choice, hi_cut)
        DEFPARAMETER(4, PA_Integer, feedback)
        DEFPARAMETER(5, PA_Choice, fb_hi_cut)
        DEFPARAMETER(6, PA_Integer, mix)
        DEFPARAMETER(7, PA_Integer, out_level)
    } classic_flanger;

    class Advanced_Flanger : public Parameter_Collection {
    public:
        Advanced_Flanger();

        DEFPARAMETER(0, PA_Choice, speed)
        DEFPARAMETER(1, PA_Integer, depth)
        DEFPARAMETER(2, PA_Choice, tempo)
        DEFPARAMETER(3, PA_Choice, hi_cut)
        DEFPARAMETER(4, PA_Integer, feedback)
        DEFPARAMETER(5, PA_Choice, fb_hi_cut)
        DEFPARAMETER(6, PA_Integer, delay)
        DEFPARAMETER(7, PA_Choice, gold_ratio)
        DEFPARAMETER(8, PA_Choice, phase_reverse)
        DEFPARAMETER(9, PA_Integer, mix)
        DEFPARAMETER(10, PA_Integer, out_level)
    } advanced_flanger;

    class Vibrato : public Parameter_Collection {
    public:
        Vibrato();

        DEFPARAMETER(0, PA_Choice, speed)
        DEFPARAMETER(1, PA_Integer, depth)
        DEFPARAMETER(2, PA_Choice, tempo)
        DEFPARAMETER(3, PA_Choice, hi_cut)
        DEFPARAMETER(4, PA_Integer, out_level)
    } vibrato;
};

class P_General : public Parameter_Collection {
public:
    P_General();

    DEFPARAMETER(0, PA_Boolean, enable_compressor)
    DEFPARAMETER(1, PA_Boolean, enable_filter)
    DEFPARAMETER(2, PA_Boolean, enable_pitch)
    DEFPARAMETER(3, PA_Boolean, enable_modulator)
    DEFPARAMETER(4, PA_Boolean, enable_delay)
    DEFPARAMETER(5, PA_Boolean, enable_reverb)
    DEFPARAMETER(6, PA_Boolean, enable_equalizer)
    DEFPARAMETER(7, PA_Boolean, enable_noisegate)

    DEFPARAMETER(8, PA_Choice, type_filter)
    DEFPARAMETER(9, PA_Choice, type_pitch)
    DEFPARAMETER(10, PA_Choice, type_modulation)
    DEFPARAMETER(11, PA_Choice, type_delay)
    DEFPARAMETER(12, PA_Choice, type_reverb)

    DEFPARAMETER(13, PA_Integer, tap_tempo)
    DEFPARAMETER(14, PA_Bits, relay1)
    DEFPARAMETER(15, PA_Bits, relay2)

    P_Compressor compressor;
    P_Equalizer equalizer;
    P_Noise_Gate noise_gate;
    P_Reverb reverb;
    std::unique_ptr<P_Pitch> pitch;
    std::unique_ptr<P_Delay> delay;
    std::unique_ptr<P_Filter> filter;
    std::unique_ptr<P_Modulation> modulation;
};

#undef DEFPARAMETER
