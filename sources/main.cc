//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "app_i18n.h"
#include "ui/main_window.h"
#include "ui/main_component.h"
#include <FL/Fl.H>
#include <FL/Fl_Pixmap.H>
#if !defined(__APPLE__)
#include <getopt.h>
#endif
#include <string>
#if defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

namespace Icon {
#include "../resources/application/FreeMajor.xpm"
};

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
#elif defined(__APPLE__)
static const char *get_locale_path(std::string &buf)
{
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    buf.resize(size);
    _NSGetExecutablePath(&buf[0], &size);
    for (unsigned i = 0; i < 2; ++i) {
        while (!buf.empty() && buf.back() != '/') buf.pop_back();
        while (!buf.empty() && buf.back() == '/') buf.pop_back();
    }
    buf.append("/Resources/locale");
    return buf.c_str();
}
#endif

int main(int argc, char *argv[])
{
#if ENABLE_NLS
    setlocale(LC_ALL, "");
#if !defined(_WIN32) && !defined(__APPLE__)
    const char *locale_path = LOCALE_DIRECTORY "/";
#else
    std::string buf;
    const char *locale_path = get_locale_path(buf);
#endif
    bindtextdomain("FreeMajor", locale_path);
#if defined(__APPLE__)
    bind_textdomain_codeset("FreeMajor", "UTF-8");
#endif
    textdomain("FreeMajor");
#endif

    Fl_Pixmap icon(Icon::FreeMajor);
    Fl_RGB_Image image(&icon);
    Fl_Window::default_icon(&image);

    Main_Window win;

    const char *arg_filename = nullptr;

#if !defined(__APPLE__)
    bool valid_args = true;
    for (int c; valid_args && (c = getopt(argc, argv, "")) != -1;) {
        switch (c) {
        default:
            valid_args = false;
            break;
        }
    }

    switch (argc - optind) {
    case 0:
        break;
    case 1:
        arg_filename = argv[optind];
        break;
    default:
        valid_args = false;
        break;
    }

    if (!valid_args) {
        fprintf(stderr, "%s\n", _("Invalid arguments."));
        return 1;
    }
#endif

    win.show();

    Main_Component &cpt = win.component();
    if (arg_filename)
        cpt.load_bank_file(arg_filename);

    Fl::run();
    return 0;
}
