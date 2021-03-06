// generated by Fast Light User Interface Designer (fluid) version 1.0304

#ifndef patch_chooser_h
#define patch_chooser_h
#include <FL/Fl.H>
#include <FL/Fl_Hold_Browser.H>
#include <stdint.h>
class Patch_Bank;
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>

class Patch_Chooser {
public:
  Patch_Chooser(const Patch_Bank &pbank);
  int show(const char *title, const char *text);
private:
  inline void cb_Cancel_i(Fl_Button*, void*);
  static void cb_Cancel(Fl_Button*, void*);
  inline void cb_OK_i(Fl_Button*, void*);
  static void cb_OK(Fl_Button*, void*);
public:
  Fl_Hold_Browser *br_bank;
  Fl_Box *lbl_text;
private:
  const Patch_Bank *pbank_ = nullptr; 
  Fl_Double_Window *window_ = nullptr; 
  bool accept_ = false; 
};
#endif
