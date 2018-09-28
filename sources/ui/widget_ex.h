//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <FL/Fl_Widget.H>

template <class T>
class Fl_Widget_Ex : public T {
public:
    Fl_Widget_Ex(int x, int y, int w, int h, const char *l = nullptr);
    virtual ~Fl_Widget_Ex() {}
    virtual int handle(int event) override;

    void enter_callback(Fl_Callback *pfn, void *user_data);
    void leave_callback(Fl_Callback *pfn, void *user_data);

private:
    Fl_Callback *pfn_enter_ = nullptr;
    void *ud_enter_ = nullptr;
    Fl_Callback *pfn_leave_ = nullptr;
    void *ud_leave_ = nullptr;
};

template <class T>
Fl_Widget_Ex<T>::Fl_Widget_Ex(int x, int y, int w, int h, const char *l)
    : T(x, y, w, h, l)
{
}

template <class T>
int Fl_Widget_Ex<T>::handle(int event)
{
    switch (event) {
    case FL_ENTER:
        if (pfn_enter_)
            pfn_enter_(this, ud_enter_);
        break;
    case FL_LEAVE:
        if (pfn_leave_)
            pfn_leave_(this, ud_leave_);
        break;
    }
    return T::handle(event);
}

template <class T>
void Fl_Widget_Ex<T>::enter_callback(Fl_Callback *pfn, void *user_data)
{
    pfn_enter_ = pfn;
    ud_enter_ = user_data;
}

template <class T>
void Fl_Widget_Ex<T>::leave_callback(Fl_Callback *pfn, void *user_data)
{
    pfn_leave_ = pfn;
    ud_leave_ = user_data;
}
