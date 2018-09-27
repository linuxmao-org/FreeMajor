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
    PT_Integer, PT_Boolean, PT_Choice,
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

    P_Compressor compressor;
};

#undef DEFPARAMETER
