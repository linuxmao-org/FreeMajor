//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "app_i18n.h"
#include "ui/main_window.h"
#include <FL/Fl.H>
#include <string>

#if defined(_WIN32)
static const char *get_locale_path(std::string &buf)
{
    buf.resize(PATH_MAX);
    buf.resize(GetModuleFileNameA(nullptr, &buf[0], buf.size()));

    if (buf.empty())
        return nullptr;

    size_t pos = buf.rfind('\\');
    if (pos == buf.npos)
        return nullptr;

    buf.resize(pos + 1);
    buf.append("..\\share\\locale\\");
    return buf.c_str();
}
#endif

int main()
{
#if ENABLE_NLS
    setlocale(LC_ALL, "");
#if !defined(_WIN32)
    const char *locale_path = LOCALE_DIRECTORY "/";
#else
    std::string buf;
    const char *locale_path = get_locale_path(buf);
#endif
    bindtextdomain("FreeMajor", locale_path);
    textdomain("FreeMajor");
#endif

    Main_Window win;
    win.show();

    Fl::run();
    return 0;
}
