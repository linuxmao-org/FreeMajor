//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "main_component.h"
#include "patch_chooser.h"
#include "eq_display.h"
#include "matrix_display.h"
#include "modifiers_editor.h"
#include "singlemod_editor.h"
#include "receive_dialog.h"
#include "widget_ex.h"
#include "association.h"
#include "midi_out_queue.h"
#include "app_i18n.h"
#include "model/patch.h"
#include "model/patch_loader.h"
#include "model/patch_writer.h"
#include "model/parameter.h"
#include "device/midi.h"
#include "device/midi_apis.h"
#include "utility/misc.h"
#include <FL/Fl_Dial.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <FL/Fl_Double_Window.H>
#include <FL/fl_ask.H>
#include <array>
#include <algorithm>
#include <math.h>
#include <assert.h>

static constexpr double sysex_send_interval = 0.100;

void Main_Component::init()
{
    reset_description_text();

    Fl_Double_Window *win_modifiers = new Fl_Double_Window(810, 635);
    win_modifiers_.reset(win_modifiers);
    win_modifiers->label(_("Modifiers"));

    win_modifiers->begin();
    Modifiers_Editor *edt_modifiers = new Modifiers_Editor(0, 0, win_modifiers->w(), win_modifiers->h());
    edt_modifiers_ = edt_modifiers;
    win_modifiers->end();

    Patch_Bank *pbank = new Patch_Bank;
    pbank_.reset(pbank);

    P_General *pgen = new P_General;
    pgen_.reset(pgen);

    Midi_Interface &mi = Midi_Interface::instance();
    size_t num_midi_apis = midi_api_count();
    if (compiled_midi_api_count() == 1)
        ch_midi_interface->hide();
    else {
        for (size_t i = 0; i < num_midi_apis; ++i) {
            if (!is_compiled_midi_api((RtMidi::Api)i))
                continue;
            const char *name = midi_api_name((RtMidi::Api)i);
            ch_midi_interface->add(name, 0, nullptr);
        }
        ch_midi_interface->value(compiled_midi_api_index(mi.current_api()));
    }
    after_changed_midi_interface();

    midi_out_q_.reset(new Midi_Out_Queue);

    txt_patch_name->when(FL_WHEN_CHANGED);

    set_nth_patch(0, Patch::create_empty());
    set_patch_number(0);
}

Main_Component::~Main_Component()
{
}

void Main_Component::reset_description_text()
{
    std::string text = _(u8"TC Electronic G-Major controller\n"
                           "Version %v, © 2018-2026\n"
                           "\n"
                           "Free and open source software controller by\n"
                           "Jean Pierre Cimalando & Julien Taverna");

    size_t vpos = text.find("%v");
    if (vpos != text.npos)
        text.replace(vpos, 2, PROJECT_VERSION);

    txt_description->copy_label(text.c_str());
}

unsigned Main_Component::get_patch_number() const
{
    int value = br_bank->value();
    if (value == 0)
        return ~0u;
    return (unsigned)(uintptr_t)br_bank->data(value);
}

void Main_Component::set_patch_number(unsigned no)
{
    if (no == get_patch_number()) {
        refresh_patch_display();
        return;
    }

    Fl_Browser &br = *br_bank;

    if (no == ~0u) {
        br.value(0);
        return;
    }

    unsigned nth = 0;
    for (unsigned i = 1, n = br.size(); nth == 0 && i <= n; ++i) {
        if ((uintptr_t)br.data(i) == no)
            nth = i;
    }

    br.value(nth);
    refresh_patch_display();
}

void Main_Component::set_nth_patch(unsigned nth, const Patch &pat)
{
    if (nth >= Patch_Bank::max_count)
        return;

    Patch_Bank &pbank = *pbank_;
    pbank.slot[nth] = pat;
    pbank.slot[nth].patch_number(nth);
    pbank.used.set(nth);

    unsigned patchno = get_patch_number();
    refresh_bank_browser();
    set_patch_number(patchno);
}

void Main_Component::load_bank_file(const char *filename, int format)
{
    std::vector<uint8_t> filedata;
    FILE_u fh(fl_fopen(filename, "rb"));
    if (!fh || !read_entire_file(fh.get(), 1 << 20, filedata)) {
        fl_message_title(_("Error"));
        fl_alert("%s", _("Could not read the bank file."));
        return;
    }
    fh.reset();

    if (format == -1) {
        const char *ext = fl_filename_ext(filename);
        ext = ext ? ext : "";
        if (!strcmp(ext, ".realmajor") || !strcmp(ext, ".realpatch"))
            format = Bank_Format::RealMajor;
        else if (!strcmp(ext, ".syx"))
            format = Bank_Format::SystemExclusive;
    }

    bool loaded = false;
    switch (format) {
    case Bank_Format::RealMajor:
        loaded = Patch_Loader::load_realmajor_bank(filedata.data(), filedata.size(), *pbank_);
        break;
    case Bank_Format::SystemExclusive:
        loaded = Patch_Loader::load_sysex_bank(filedata.data(), filedata.size(), *pbank_);
        break;
    }

    if (!loaded) {
        fl_message_title(_("Error"));
        fl_alert("%s", _("Could not load the bank file."));
        return;
    }

    for (unsigned i = 0; i < Patch_Bank::max_count; ++i) {
        if (pbank_->used[i])
            pbank_->slot[i].patch_number(i);
    }

    refresh_bank_browser();
    refresh_patch_display();
}

void Main_Component::refresh_bank_browser()
{
    Fl_Browser &br = *br_bank;
    Patch_Bank &pbank = *pbank_;

    br.clear();
    for (unsigned i = 0; i < Patch_Bank::max_count; ++i) {
        if (pbank.used[i]) {
            std::string text = std::to_string(i + 1) + " - " + pbank.slot[i].name();
            br.add(text.c_str(), (void *)(uintptr_t)i);
        }
    }
}

void Main_Component::refresh_patch_display()
{
    unsigned patchno = get_patch_number();
    if (patchno == ~0u)
        return;
    const Patch &pat = pbank_->slot[patchno];

    txt_patch_name->value(pat.name().c_str());

    P_General &pgen = *pgen_;
    chk_compressor->value(pgen.enable_compressor().get(pat));
    chk_filter->value(pgen.enable_filter().get(pat));
    chk_pitch->value(pgen.enable_pitch().get(pat));
    chk_chorus->value(pgen.enable_modulator().get(pat));
    chk_delay->value(pgen.enable_delay().get(pat));
    chk_reverb->value(pgen.enable_reverb().get(pat));
    chk_equalizer->value(pgen.enable_equalizer().get(pat));
    chk_noise_gate->value(pgen.enable_noisegate().get(pat));

    assoc_.clear();
    assoc_entered_.clear();

    Association *a;

    a = setup_slider(sl_tap_tempo, pgen.tap_tempo());
    a->value_labels.push_back(lbl_tap_tempo);

    setup_checkbox(chk_relay1, pgen.relay1());
    setup_checkbox(chk_relay2, pgen.relay2());
    setup_choice(cb_routing, pgen.routing());

    a = setup_slider(sl_out_level, pgen.out_level());
    a->value_labels.push_back(lbl_out_level);

    setup_checkbox(chk_compressor, pgen.enable_compressor(), Assoc_Refresh_Full);
    setup_checkbox(chk_filter, pgen.enable_filter(), Assoc_Refresh_Full);
    setup_checkbox(chk_pitch, pgen.enable_pitch(), Assoc_Refresh_Full);
    setup_checkbox(chk_chorus, pgen.enable_modulator(), Assoc_Refresh_Full);
    setup_checkbox(chk_delay, pgen.enable_delay(), Assoc_Refresh_Full);
    setup_checkbox(chk_reverb, pgen.enable_reverb(), Assoc_Refresh_Full);
    setup_checkbox(chk_equalizer, pgen.enable_equalizer(), Assoc_Refresh_Full);
    setup_checkbox(chk_noise_gate, pgen.enable_noisegate(), Assoc_Refresh_Full);

    setup_choice(cb_filter, pgen.type_filter(), Assoc_Refresh_Full);
    setup_choice(cb_pitch, pgen.type_pitch(), Assoc_Refresh_Full);
    setup_choice(cb_chorus, pgen.type_modulation(), Assoc_Refresh_Full);
    setup_choice(cb_delay, pgen.type_delay(), Assoc_Refresh_Full);
    setup_choice(cb_reverb, pgen.type_reverb(), Assoc_Refresh_Full);

    std::array<Fl_Group_Ex *, 6> box_cpr
        {{ box_cpr1, box_cpr2, box_cpr3, box_cpr4, box_cpr5, box_cpr6 }};
    setup_boxes(pgen.enable_compressor().get(pat), pgen.compressor, box_cpr.data(), box_cpr.size());

    std::array<Fl_Group_Ex *, 9> box_eq
        {{ box_eq1, box_eq2, box_eq3, box_eq4, box_eq5, box_eq6, box_eq7, box_eq8, box_eq9 }};
    setup_boxes(pgen.enable_equalizer().get(pat), pgen.equalizer, box_eq.data(), box_eq.size());

    std::array<Fl_Group_Ex *, 6> box_ng
        {{ box_ng1, box_ng2, box_ng3, box_ng4, box_ng5, box_ng6 }};
    setup_boxes(pgen.enable_noisegate().get(pat), pgen.noise_gate, box_ng.data(), box_ng.size());

    std::array<Fl_Group_Ex *, 14> box_rev
        {{ box_rev1, box_rev2, box_rev3, box_rev4, box_rev5, box_rev6, box_rev7, box_rev8, box_rev9, box_rev10, box_rev11, box_rev12, box_rev13, box_rev14 }};
    setup_boxes(pgen.enable_reverb().get(pat), pgen.reverb, box_rev.data(), box_rev.size());

    std::array<Fl_Group_Ex *, 14> box_pit
        {{ box_pit1, box_pit2, box_pit3, box_pit4, box_pit5, box_pit6, box_pit7, box_pit8, box_pit9, box_pit10, box_pit11, box_pit12, box_pit13, box_pit14 }};
    setup_boxes(pgen.enable_pitch().get(pat), pgen.pitch->dispatch(pat), box_pit.data(), box_pit.size());

    std::array<Fl_Group_Ex *, 14> box_del
        {{ box_del1, box_del2, box_del3, box_del4, box_del5, box_del6, box_del7, box_del8, box_del9, box_del10, box_del11, box_del12, box_del13, box_del14 }};
    setup_boxes(pgen.enable_delay().get(pat), pgen.delay->dispatch(pat), box_del.data(), box_del.size());

    std::array<Fl_Group_Ex *, 14> box_flt
        {{ box_flt1, box_flt2, box_flt3, box_flt4, box_flt5, box_flt6, box_flt7, box_flt8, box_flt9, box_flt10, box_flt11, box_flt12, box_flt13, box_flt14 }};
    setup_boxes(pgen.enable_filter().get(pat), pgen.filter->dispatch(pat), box_flt.data(), box_flt.size());

    std::array<Fl_Group_Ex *, 14> box_cho
        {{ box_cho1, box_cho2, box_cho3, box_cho4, box_cho5, box_cho6, box_cho7, box_cho8, box_cho9, box_cho10, box_cho11, box_cho12, box_cho13, box_cho14 }};
    setup_boxes(pgen.enable_modulator().get(pat), pgen.modulation->dispatch(pat), box_cho.data(), box_cho.size());

    setup_modifier_row(_("Filter"), pgen.enable_filter().get(pat), 0, pgen.filter->dispatch(pat));
    setup_modifier_row(_("Pitch"), pgen.enable_pitch().get(pat), 1, pgen.pitch->dispatch(pat));
    setup_modifier_row(_("Chorus/Flanger"), pgen.enable_modulator().get(pat), 2, pgen.modulation->dispatch(pat));
    setup_modifier_row(_("Delay"), pgen.enable_delay().get(pat), 3, pgen.delay->dispatch(pat));
    setup_modifier_row(_("Reverb"), pgen.enable_reverb().get(pat), 4, pgen.reverb);

    for (const auto &a : assoc_) {
        if (Fl_Widget *w = a->value_widget)
            w->callback(&on_edited_parameter, this);
        a->update_value(pat);
    }

    update_eq_display();
    update_matrix_display();
}

Association *Main_Component::setup_slider(Fl_Slider_Ex *sl, Parameter_Access &p, int flags)
{
    std::unique_ptr<Association> a(new Association);
    a->access = &p;
    a->value_widget = sl;
    a->kind = Assoc_Slider;
    a->flags |= flags;

    sl->range(p.min(), p.max());
    sl->step(1);
    sl->enter_callback(&on_enter_parameter_control, this);
    sl->leave_callback(&on_leave_parameter_control, this);

    assoc_.emplace_back(a.get());
    return a.release();
}

void Main_Component::setup_checkbox(Fl_Check_Button_Ex *chk, Parameter_Access &p, int flags)
{
    std::unique_ptr<Association> a(new Association);
    a->access = &p;
    a->value_widget = chk;
    a->kind = Assoc_Check;
    a->flags |= flags;

    chk->enter_callback(&on_enter_parameter_control, this);
    chk->leave_callback(&on_leave_parameter_control, this);

    assoc_.push_back(std::move(a));
}

void Main_Component::setup_choice(Fl_Choice_Ex *cb, Parameter_Access &p, int flags)
{
    std::unique_ptr<Association> a(new Association);

    cb->clear();

    if (p.type() == PT_Choice) {
        for (const std::string &value : static_cast<PA_Choice &>(p).values)
            cb->add(value.c_str(), 0, nullptr);
    }
    else {
        for (int i1 = p.min(), i2 = p.max(); i1 <= i2; ++i1)
            cb->add(p.to_string(i1).c_str(), 0, nullptr);
    }

    a->access = &p;
    a->value_widget = cb;
    a->kind = Assoc_Choice;
    a->flags |= flags;

    cb->enter_callback(&on_enter_parameter_control, this);
    cb->leave_callback(&on_leave_parameter_control, this);

    assoc_.push_back(std::move(a));
}

void Main_Component::setup_boxes(bool enable, const Parameter_Collection &pc, Fl_Group_Ex *boxes[], unsigned nboxes)
{
    for (unsigned i = 0; i < nboxes; ++i) {
        Fl_Group_Ex *box = boxes[i];
        box->clear();
        box->labeltype(FL_NO_LABEL);
    }
    if (enable) {
        size_t slot_count = pc.slots.size();
        Fl_Group_Ex **box_frontp = boxes;
        Fl_Group_Ex **box_backp = boxes + nboxes;

        std::unique_ptr<Fl_Group_Ex *[]> box_alloc(new Fl_Group_Ex *[nboxes]); 
        for (size_t i = 0; i < slot_count; ++i) {
            Parameter_Access *p = pc.slots[i].get();
            if (p->position == PP_Front)
                box_alloc[i] = *box_frontp++;
        }
        for (size_t i = slot_count; i-- > 0;) {
            Parameter_Access *p = pc.slots[i].get();
            if (p->position != PP_Front)
                box_alloc[i] = *--box_backp;
        }

        for (size_t i = 0; i < slot_count; ++i) {
            std::unique_ptr<Association> a(new Association);

            Parameter_Access *pa = pc.slots[i].get();
            a->access = pa;

            Fl_Group_Ex *box = box_alloc[i];
            a->group_box = box;
            a->name_labels.push_back(box);
            int bx = box->x(), by = box->y(), bw = box->w(), bh = box->h();

            int padding = 16;
            int wx = bx + padding, wy = by + padding;
            int ww = bw - 2 * padding, wh = bh - 2 * padding;

            box->labeltype(FL_NORMAL_LABEL);
            box->labelsize(9);
            box->align(FL_ALIGN_TOP|FL_ALIGN_INSIDE);

            box->begin();
            Fl_Dial_Ex *dial = new Fl_Dial_Ex(wx, wy, ww, wh);
            a->value_widget = dial;
            a->kind = Assoc_Dial;
            dial->range(pa->min(), pa->max());
            dial->labeltype(FL_NORMAL_LABEL);
            dial->labelsize(9);
            dial->align(FL_ALIGN_BOTTOM);
            dial->step(1);
            box->end();

            a->value_labels.push_back(dial);

            box->enter_callback(&on_enter_parameter_control, this);
            box->leave_callback(&on_leave_parameter_control, this);
            dial->enter_callback(&on_enter_parameter_control, this);
            dial->leave_callback(&on_leave_parameter_control, this);

            assoc_.push_back(std::move(a));
        }
    }
    for (unsigned i = 0; i < nboxes; ++i) {
        Fl_Group_Ex *box = boxes[i];
        box->redraw();
    }
}

void Main_Component::setup_modifier_row(const char *title, bool enable, int row, Parameter_Collection &pc)
{
    Modifiers_Editor *edt = edt_modifiers_;

    for (unsigned i = 0, n = edt->columns; i < n; ++i) {
        Fl_Group *box = edt->box_from_coords(row, i);
        box->label("");
        box->clear();
    }

    edt->label_for_row(row)->copy_label(title);

    if (enable) {
        std::vector<Parameter_Access *> slots;
        slots.reserve(pc.slots.size());
        for (size_t i = 0, n = pc.slots.size(); i < n; ++i) {
            if (pc.slots[i]->modifiers)
                slots.push_back(pc.slots[i].get());
        }

        size_t slot_count = slots.size();
        std::unique_ptr<Fl_Group *[]> box_alloc(new Fl_Group *[slot_count]); 
        for (size_t i = 0, n = slot_count, c = 0; i < n; ++i) {
            Parameter_Access *p = slots[i];
            if (p->position == PP_Front)
                box_alloc[i] = edt->box_from_coords(row, c++);
        }
        for (size_t i = slot_count, c = edt->columns; i-- > 0;) {
            Parameter_Access *p = slots[i];
            if (p->position != PP_Front)
                box_alloc[i] = edt->box_from_coords(row, --c);
        }

        for (size_t i = 0; i < slot_count; ++i) {
            Parameter_Access *pa = slots[i];
            Parameter_Modifiers *pm = pa->modifiers.get();

            Fl_Group *box = box_alloc[i];
            int bx = box->x(), by = box->y(), bw = box->w(), bh = box->h();

            box->label(pa->name);
            box->begin();
            Single_Mod_Editor *me = new Single_Mod_Editor(0, 0, bw, bh);
            me->position(bx, by);

            Fl_Slider *valuators[4] = {
                me->sl_assignment, me->sl_min, me->sl_mid, me->sl_max
            };
            Fl_Box *value_labels[4] = {
                me->lbl_assignment, me->lbl_min, me->lbl_mid, me->lbl_max
            };
            Parameter_Access *parameters[4] = {
                pm->assignment.get(), pm->min.get(), pm->mid.get(), pm->max.get()
            };

            for (unsigned i = 0; i < 4; ++i) {
                std::unique_ptr<Association> a(new Association);
                Fl_Slider *sl = valuators[i];
                Parameter_Access *pa = parameters[i];
                sl->range(pa->max(), pa->min());
                sl->step(1);
                a->access = pa;
                a->group_box = box;
                a->value_widget = sl;
                a->kind = Assoc_Slider;
                a->value_labels.push_back(value_labels[i]);
                assoc_.push_back(std::move(a));
            }

            box->end();
        }
    }

    for (unsigned i = 0, n = edt->columns; i < n; ++i) {
        Fl_Group *box = edt->box_from_coords(row, i);
        box->redraw();
    }

    edt->redraw();
}

void Main_Component::on_changed_midi_interface()
{
    unsigned value = ch_midi_interface->value();
    if ((int)value == -1)
        return;

    Midi_Interface &mi = Midi_Interface::instance();
    RtMidi::Api api = compiled_midi_api_by_index(value);

    mi.switch_api(api);
    after_changed_midi_interface();
}

void Main_Component::after_changed_midi_interface()
{
    Midi_Interface &mi = Midi_Interface::instance();

    if (mi.supports_virtual_port()) {
        mi.open_output_port(~0u);
        mi.open_input_port(~0u);
        lbl_midi_out->label(_("Virtual port"));
        lbl_midi_in->label(_("Virtual port"));
    }
    else {
        lbl_midi_out->label("");
        lbl_midi_in->label("");
    }
}

void Main_Component::on_change_midi_out()
{
    int x = btn_midi_out->x();
    int y = btn_midi_out->y() + btn_midi_out->h();

    Midi_Interface &mi = Midi_Interface::instance();
    std::vector<Fl_Menu_Item> menu_list;

    if (mi.supports_virtual_port())
        menu_list.push_back(Fl_Menu_Item{_("Virtual port"), 0, nullptr, (void *)~(uintptr_t)0, FL_MENU_DIVIDER});

    std::vector<std::string> out_ports = mi.get_real_output_ports();
    for (size_t i = 0, n = out_ports.size(); i < n; ++i)
        menu_list.push_back(Fl_Menu_Item{out_ports[i].c_str(), 0, nullptr, (void *)(uintptr_t)i});
    menu_list.push_back(Fl_Menu_Item{nullptr});

    for (Fl_Menu_Item &item : menu_list)
        item.labelsize(12);

    const Fl_Menu_Item *choice = menu_list[0].popup(x, y);
    if (!choice)
        return;

    unsigned port = (unsigned)(uintptr_t)choice->user_data();
    mi.open_output_port(port);
    lbl_midi_out->copy_label(choice->label());
}

void Main_Component::on_change_midi_in()
{
    int x = btn_midi_in->x();
    int y = btn_midi_in->y() + btn_midi_in->h();

    Midi_Interface &mi = Midi_Interface::instance();
    std::vector<Fl_Menu_Item> menu_list;

    if (mi.supports_virtual_port())
        menu_list.push_back(Fl_Menu_Item{_("Virtual port"), 0, nullptr, (void *)~(uintptr_t)0, FL_MENU_DIVIDER});

    std::vector<std::string> in_ports = mi.get_real_input_ports();
    for (size_t i = 0, n = in_ports.size(); i < n; ++i)
        menu_list.push_back(Fl_Menu_Item{in_ports[i].c_str(), 0, nullptr, (void *)(uintptr_t)i});
    menu_list.push_back(Fl_Menu_Item{nullptr});

    for (Fl_Menu_Item &item : menu_list)
        item.labelsize(12);

    const Fl_Menu_Item *choice = menu_list[0].popup(x, y);
    if (!choice)
        return;

    unsigned port = (unsigned)(uintptr_t)choice->user_data();
    mi.open_input_port(port);
    lbl_midi_in->copy_label(choice->label());
}

void Main_Component::on_selected_patch()
{
    unsigned patchno = get_patch_number();
    if (patchno == ~0u)
        return;

    refresh_patch_display();

    if (chk_realtime->value())
        on_clicked_change();
}

void Main_Component::on_clicked_import()
{
    Fl_Native_File_Chooser f_chooser(Fl_Native_File_Chooser::BROWSE_FILE);
    f_chooser.title(_("Import..."));
    f_chooser.filter(_("Real Major patch\t*.realpatch\n"
                       "Sysex patch\t*.syx"));

    if (f_chooser.show() != 0)
        return;

    const char *filename = f_chooser.filename();
    std::vector<uint8_t> filedata;
    FILE_u fh(fl_fopen(filename, "rb"));
    if (!fh || !read_entire_file(fh.get(), 1 << 20, filedata)) {
        fl_message_title(_("Error"));
        fl_alert("%s", _("Could not read the patch file."));
        return;
    }
    fh.reset();

    Patch pat;
    bool loaded = false;
    switch (f_chooser.filter_value()) {
    case 0:
        loaded = Patch_Loader::load_realmajor_patch(filedata.data(), filedata.size(), pat);
        break;
    case 1:
        loaded = Patch_Loader::load_sysex_patch(filedata.data(), filedata.size(), pat);
        break;
    }

    if (!loaded) {
        fl_message_title(_("Error"));
        fl_alert("%s", _("Could not load the patch file."));
        return;
    }

    Patch_Chooser p_chooser(*pbank_);
    unsigned patchno = p_chooser.show(_("Import into..."), _("Select the patch number:"));
    if ((int)patchno == -1)
        return;

    set_nth_patch(patchno, pat);
}

void Main_Component::on_clicked_export()
{
    unsigned patchno = get_patch_number();
    if (patchno == ~0u)
        return;
    const Patch &pat = pbank_->slot[patchno];

    Fl_Native_File_Chooser chooser(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
    chooser.title(_("Export..."));
    chooser.filter(_("Real Major patch\t*.realpatch\n"
                     "Sysex patch\t*.syx"));

    if (chooser.show() != 0)
        return;

    std::string filename = chooser.filename();
    std::vector<uint8_t> data;
    switch (chooser.filter_value()) {
    case 0:
        Patch_Writer::save_realmajor_patch(pat, data);
        if (file_name_extension(filename).empty())
            filename += ".realpatch";
        break;
    case 1:
        Patch_Writer::save_sysex_patch(pat, data);
        if (file_name_extension(filename).empty())
            filename += ".syx";
        break;
    }

    if (fl_access(filename.c_str(), 0) == 0) {
        fl_message_title(_("Confirm overwrite"));
        if (fl_choice("%s", _("No"), _("Yes"), nullptr, _("The file already exists. Replace it?")) != 1)
            return;
    }

    FILE_u fh(fl_fopen(filename.c_str(), "wb"));
    if (fwrite(data.data(), 1, data.size(), fh.get()) != data.size()) {
        fl_unlink(filename.c_str());
        fl_message_title(_("Error"));
        fl_alert("%s", _("Could not save the patch file."));
        return;
    }
}

void Main_Component::on_clicked_change()
{
    unsigned patchno = get_patch_number();
    if (patchno == ~0u)
        return;

    if (false) {
        uint8_t bank_chg_msg[3] = {0xb0, 0x00, 0x01};
        midi_out_q_->enqueue_message(bank_chg_msg, sizeof(bank_chg_msg), 0.0);
    }

    uint8_t pgm_chg_msg[2] = {0xc0, (uint8_t)patchno};
    midi_out_q_->enqueue_message(pgm_chg_msg, sizeof(pgm_chg_msg), 0.0);
}

void Main_Component::on_clicked_load()
{
    Fl_Native_File_Chooser f_chooser(Fl_Native_File_Chooser::BROWSE_FILE);
    f_chooser.title(_("Load..."));
    f_chooser.filter(_("Real Major bank\t*.realmajor\n"
                       "Sysex bank\t*.syx"));

    if (f_chooser.show() != 0)
        return;

    int format = -1;
    switch (f_chooser.filter_value()) {
    case 0:
        format = Bank_Format::RealMajor;
        break;
    case 1:
        format = Bank_Format::SystemExclusive;
        break;
    }

    return load_bank_file(f_chooser.filename(), format);
}

void Main_Component::on_clicked_save()
{
    Fl_Native_File_Chooser chooser(Fl_Native_File_Chooser::BROWSE_SAVE_FILE);
    chooser.title(_("Save..."));
    chooser.filter(_("Real Major bank\t*.realmajor\n"
                     "Sysex bank\t*.syx"));

    if (chooser.show() != 0)
        return;

    std::string filename = chooser.filename();
    std::vector<uint8_t> data;
    switch (chooser.filter_value()) {
    case 0:
        Patch_Writer::save_realmajor_bank(*pbank_, data);
        if (file_name_extension(filename).empty())
            filename += ".realmajor";
        break;
    case 1:
        Patch_Writer::save_sysex_bank(*pbank_, data);
        if (file_name_extension(filename).empty())
            filename += ".syx";
        break;
    }

    if (fl_access(filename.c_str(), 0) == 0) {
        fl_message_title(_("Confirm overwrite"));
        if (fl_choice("%s", _("No"), _("Yes"), nullptr, _("The file already exists. Replace it?")) != 1)
            return;
    }

    FILE_u fh(fl_fopen(filename.c_str(), "wb"));
    if (fwrite(data.data(), 1, data.size(), fh.get()) != data.size()) {
        fl_unlink(filename.c_str());
        fl_message_title(_("Error"));
        fl_alert("%s", _("Could not save the bank file."));
        return;
    }
}

void Main_Component::on_clicked_new()
{
    Patch_Chooser p_chooser(*pbank_);
    unsigned patchno = p_chooser.show(_("Create into..."), _("Select the patch number:"));
    if ((int)patchno == -1)
        return;

    Patch_Bank &pbank = *pbank_;
    pbank.slot[patchno] = Patch::create_empty();
    pbank.slot[patchno].patch_number(patchno);
    pbank.used[patchno] = true;

    refresh_bank_browser();
    set_patch_number(patchno);
}

void Main_Component::on_clicked_copy()
{
    unsigned src_patchno = get_patch_number();
    if (src_patchno == ~0u)
        return;

    Patch_Chooser p_chooser(*pbank_);
    unsigned dst_patchno = p_chooser.show(_("Copy into..."), _("Select the patch number:"));
    if ((int)dst_patchno == -1)
        return;

    Patch_Bank &pbank = *pbank_;
    pbank.slot[dst_patchno] = pbank.slot[src_patchno];
    pbank.used[dst_patchno] = true;

    refresh_bank_browser();
    set_patch_number(src_patchno);
}

void Main_Component::on_clicked_delete()
{
    unsigned patchno = get_patch_number();
    if (patchno == ~0u)
        return;

    fl_message_title(_("Confirm delete"));
    if (fl_choice("%s", _("No"), _("Yes"), nullptr, _("Delete the current patch?")) != 1)
        return;

    Patch_Bank &pbank = *pbank_;
    pbank.used[patchno] = false;

    refresh_bank_browser();
    refresh_patch_display();
}

void Main_Component::on_clicked_receive()
{
    Patch_Bank pbank;

    Receive_Dialog dlg(pbank);
    if (dlg.show(_("Receive")) == -1)
        return;

    *pbank_ = pbank;
    refresh_bank_browser();
    refresh_patch_display();
}

void Main_Component::on_clicked_send()
{
    unsigned patchno = get_patch_number();
    if (patchno == ~0u)
        return;
    const Patch &pat = pbank_->slot[patchno];

    std::vector<uint8_t> message;
    Patch_Writer::save_sysex_patch(pat, message);

    midi_out_q_->cancel_all_sysex();

    if (false) {
        uint8_t bank_chg_msg[3] = {0xb0, 0x00, 0x01};
        midi_out_q_->enqueue_message(bank_chg_msg, sizeof(bank_chg_msg), 0.0);
    }

    uint8_t pgm_chg_msg[2] = {0xc0, (uint8_t)patchno};
    midi_out_q_->enqueue_message(pgm_chg_msg, sizeof(pgm_chg_msg), 0.0);

    midi_out_q_->enqueue_message(message.data(), message.size(), sysex_send_interval);
}

void Main_Component::on_clicked_modifiers()
{
    Fl_Double_Window &win = *win_modifiers_;
    win.show();
}

void Main_Component::on_edited_patch_name()
{
    unsigned patchno = get_patch_number();
    if (patchno == ~0u)
        return;
    Patch &pat = pbank_->slot[patchno];

    pat.name(txt_patch_name->value());
    refresh_bank_browser();
    set_patch_number(patchno);

    if (chk_realtime->value())
        on_clicked_send();
}

void Main_Component::on_edited_parameter(Fl_Widget *w, void *user_data)
{
    Main_Component *self = (Main_Component *)user_data;

    unsigned patchno = self->get_patch_number();
    if (patchno == ~0u)
        return;

    Patch &pat = self->pbank_->slot[patchno];
    P_General &pgen = *self->pgen_;

    Association *a = nullptr;
    for (unsigned i = 0, n = self->assoc_.size(); i < n && !a; ++i) {
        Association *cur = self->assoc_[i].get();
        if (cur->value_widget == w)
            a = cur;
    }

    if (!a) {
        assert(false);
        return;
    }

    a->update_from_widget(pat);

    if (a->flags & Assoc_Refresh_Full)
        self->refresh_patch_display();
    else if (pgen.equalizer.contains(*a->access))
        self->update_eq_display();
    else if (a->access == &pgen.routing())
        self->update_matrix_display();

    if (self->chk_realtime->value())
        self->on_clicked_send();
}

void Main_Component::on_enter_parameter_control(Fl_Widget *w, void *user_data)
{
    Main_Component *self = (Main_Component *)user_data;

    Association *a = nullptr;
    for (unsigned i = 0, n = self->assoc_.size(); i < n && !a; ++i) {
        Association *cur = self->assoc_[i].get();
        if (cur->value_widget == w)
            a = cur;
        else if (cur->group_box == w)
            a = cur;
    }

    if (!a)
        return;

    self->txt_description->copy_label(a->access->description);
    self->assoc_entered_.push_front(a);
}

void Main_Component::on_leave_parameter_control(Fl_Widget *w, void *user_data)
{
    Main_Component *self = (Main_Component *)user_data;

    Association *a = nullptr;
    for (unsigned i = 0, n = self->assoc_.size(); i < n && !a; ++i) {
        Association *cur = self->assoc_[i].get();
        if (cur->value_widget == w)
            a = cur;
        else if (cur->group_box == w)
            a = cur;
    }

    if (!a)
        return;

    auto &assoc_entered = self->assoc_entered_;

    auto it = std::find(assoc_entered.begin(), assoc_entered.end(), a);
    if (it != assoc_entered.end())
        assoc_entered.erase(it);

    if (assoc_entered.empty())
        self->reset_description_text();
    else
        self->txt_description->copy_label(assoc_entered.front()->access->description);
}

void Main_Component::update_eq_display()
{
    unsigned patchno = get_patch_number();
    if (patchno == ~0u)
        return;

    Patch &pat = pbank_->slot[patchno];
    P_General &pgen = *pgen_;
    P_Equalizer &peq = pgen.equalizer;

    Eq_Display::Band bands[3];
    bool peq_enable = pgen.enable_equalizer().get(pat) != 0;
    PA_Choice *peq_frequency[3] = {&peq.frequency1(), &peq.frequency2(), &peq.frequency3()};
    PA_Integer *peq_gain[3] = {&peq.gain1(), &peq.gain2(), &peq.gain3()};
    PA_Choice *peq_width[3] = {&peq.width1(), &peq.width2(), &peq.width3()};

    auto frequency = [](const std::string &x) -> double {
                         double v = 0.0;
                         unsigned count = 0;
                         if (sscanf_lc(x.c_str(), "%lf%n", "C", &v, &count) != 1) {
                             assert(false);
                             abort();
                         }
                         if (x.size() > count && x[count] == 'k') {
                             v *= 1000;
                             ++count;
                         }
                         if (x.size() != count) {
                             assert(false);
                             abort();
                         }
                         return v;
                     };
    auto width = [](const std::string &x) -> double {
                     double v = 0.0;
                     unsigned count = 0;
                     if (sscanf_lc(x.c_str(), "%lf%n", "C", &v, &count) != 1 || x.size() != count) {
                         assert(false);
                         abort();
                     }
                     return v;
                 };

    for (unsigned i = 0; i < 3; ++i) {
        PA_Choice *pf = peq_frequency[i];
        PA_Integer *pg = peq_gain[i];
        PA_Choice *pw = peq_width[i];
        unsigned idf = pf->clamp(pf->get(pat));
        unsigned idw = pw->clamp(pw->get(pat));
        bands[i].freq = (idf + 1 == pf->values.size()) ? -1.0 : frequency(pf->values[idf]);
        bands[i].gain = pow(10.0, 0.05 * pg->get(pat));
        bands[i].width = width(pw->values[idw]);
    }

    d_eq->set_bands(peq_enable, bands, 3);
}

void Main_Component::update_matrix_display()
{
    unsigned patchno = get_patch_number();
    if (patchno == ~0u)
        return;

    Patch &pat = pbank_->slot[patchno];
    P_General &pgen = *pgen_;

    int routing = pgen.routing().get(pat);
    Matrix_Display *d_matrix = this->d_matrix;
    d_matrix->clear_matrix();

    switch (routing) {
    default:
        assert(false);
    case 0:
        for (unsigned column = 0; column < 6; ++column)
            d_matrix->set_matrix(0, column, true);
        break;
    case 1:
        for (unsigned column = 0; column < 5; ++column)
            d_matrix->set_matrix(0, column, true);
        d_matrix->set_matrix(1, 4, true);
        break;
    case 2:
        for (unsigned column = 0; column < 3; ++column)
            d_matrix->set_matrix(0, column, true);
        for (unsigned row = 1; row < 4; ++row)
            d_matrix->set_matrix(row, 2, true);
        break;
    }
}
