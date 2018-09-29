//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "main_component.h"
#include "patch_chooser.h"
#include "widget_ex.h"
#include "association.h"
#include "midi_out_queue.h"
#include "app_i18n.h"
#include "model/patch.h"
#include "model/patch_loader.h"
#include "model/patch_writer.h"
#include "model/parameter.h"
#include "device/midi.h"
#include "utility/misc.h"
#include <FL/Fl_Dial.H>
#include <FL/Fl_Native_File_Chooser.H>
#include <algorithm>
#include <assert.h>

static constexpr double sysex_send_interval = 0.100;

void Main_Component::init()
{
    reset_description_text();

    Patch_Bank *pbank = new Patch_Bank;
    pbank_.reset(pbank);

    P_General *pgen = new P_General;
    pgen_.reset(pgen);

    midi_out_q_.reset(new Midi_Out_Queue);
    update_midi_outs();

    txt_patch_name->when(FL_WHEN_CHANGED);

    set_nth_patch(0, Patch::create_empty());
    set_patch_number(0);
}

Main_Component::~Main_Component()
{
}

void Main_Component::reset_description_text()
{
    txt_description->label(_("TC Electronic G-Major controller, © 2018\n"
                             "Free and open source software controller by\n"
                             "Jean Pierre Cimalando & Julien Taverna"));
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
    a->value_update_callback = [this](int v) {
                                   lbl_tap_tempo->copy_label((std::to_string(v) + " ms").c_str());
                               };

    setup_checkbox(chk_relay1, pgen.relay1());
    setup_checkbox(chk_relay2, pgen.relay2());
    setup_choice(cb_routing, pgen.routing());

    a = setup_slider(sl_out_level, pgen.out_level());
    a->value_update_callback = [this](int v) {
                                   lbl_out_level->copy_label((std::to_string(v) + " dB").c_str());
                               };

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

    for (const auto &a : assoc_) {
        if (Fl_Widget *w = a->value_widget)
            w->callback(&on_edited_parameter, this);
        a->update_value(pat);
    }
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
            switch (p->position) {
            case PP_Front:
                box_alloc[i] = *box_frontp++; break;
            case PP_Back:
                box_alloc[i] = *--box_backp; break;
            default:
                assert(false); abort();
            }
        }

        for (size_t i = 0; i < slot_count; ++i) {
            std::unique_ptr<Association> a(new Association);
            a->flags = Assoc_Name_On_Box|Assoc_Value_On_Label;

            Parameter_Access *pa = pc.slots[i].get();
            a->access = pa;

            Fl_Group_Ex *box = box_alloc[i];
            a->group_box = box;
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

void Main_Component::update_midi_outs()
{
    Midi_Out &mido = Midi_Out::instance();
    ch_midi_out->clear();
    if (mido.supports_virtual_port())
        ch_midi_out->add(_("Virtual port"));

    std::vector<std::string> out_ports = mido.get_real_ports();
    for (size_t i = 0, n = out_ports.size(); i < n; ++i)
        ch_midi_out->add(out_ports[i].c_str());
}

void Main_Component::on_changed_midi_out()
{
    int value = ch_midi_out->value();
    Midi_Out &mido = Midi_Out::instance();
    if (mido.supports_virtual_port())
        --value;  // virtual port is the first entry

    mido.open_port(value);
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
    Patch pat;
    if (!Patch_Loader::load_patch_file(filename, pat)) {
        fl_message_title(_("Error"));
        fl_alert(_("Could not load the patch file."));
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
        if (fl_choice(_("The file already exists. Replace it?"), _("Yes"), _("No"), nullptr))
            return;
    }

    FILE_u fh(fl_fopen(filename.c_str(), "wb"));
    if (fwrite(data.data(), 1, data.size(), fh.get()) != data.size()) {
        fl_unlink(filename.c_str());
        fl_message_title(_("Error"));
        fl_alert(_("Could not save the patch file."));
        return;
    }
}

void Main_Component::on_clicked_change()
{
    unsigned patchno = get_patch_number();
    if (patchno == ~0u)
        return;

    uint8_t msg[2] = {0xc0, (uint8_t)patchno};
    midi_out_q_->enqueue_message(msg, sizeof(msg), 0.0);
}

void Main_Component::on_clicked_new()
{
    Patch_Chooser p_chooser(*pbank_);
    unsigned patchno = p_chooser.show(_("Create into..."), _("Select the patch number:"));
    if ((int)patchno == -1)
        return;

    Patch_Bank &pbank = *pbank_;
    pbank.slot[patchno] = Patch::create_empty();
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
    if (fl_choice(_("Delete the current patch?"), _("Yes"), _("No"), nullptr))
        return;

    Patch_Bank &pbank = *pbank_;
    pbank.used[patchno] = false;

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

    uint8_t pgm_chg_msg[2] = {0xc0, (uint8_t)patchno};
    midi_out_q_->enqueue_message(pgm_chg_msg, sizeof(pgm_chg_msg), 0.0);

    midi_out_q_->enqueue_message(message.data(), message.size(), sysex_send_interval);
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
