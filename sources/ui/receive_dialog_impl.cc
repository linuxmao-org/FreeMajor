//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "receive_dialog.h"
#include "model/patch.h"
#include "model/patch_loader.h"
#include "device/midi.h"
#include <FL/Fl.H>

static constexpr double update_tick_interval = 0.1;

Receive_Dialog::Receive_Dialog(Patch_Bank &pbank)
    : pbank_(&pbank)
{
}

void Receive_Dialog::begin_receive()
{
    Patch_Bank &pbank = *pbank_;

    pbank = Patch_Bank();
    rx_programs_ = 0;
    rx_messages_ = 0;

    Fl::add_timeout(update_tick_interval, &on_update_tick, this);

    Midi_Interface &mi = Midi_Interface::instance();
    mi.install_input_handler(&on_midi_input, this);
}

void Receive_Dialog::end_receive()
{
    Midi_Interface &mi = Midi_Interface::instance();
    mi.uninstall_input_handler(&on_midi_input, this);

    Fl::remove_timeout(&on_update_tick, this);
}

void Receive_Dialog::on_midi_input(const uint8_t *msg, size_t len, void *user_data)
{
    Receive_Dialog *self = reinterpret_cast<Receive_Dialog *>(user_data);
    Patch_Bank &pbank = *self->pbank_;

    Patch patch;
    if (!Patch_Loader::load_sysex_patch(msg, len, patch))
        return;

    unsigned patchno = patch.patch_number();
    if (!pbank.used.test(patchno)) {
        pbank.used.set(patchno);
        ++self->rx_programs_;
    }

    // ensure having patch number in valid range
    patch.patch_number(patchno);
    pbank.slot[patchno] = patch;

    ++self->rx_messages_;
    std::atomic_thread_fence(std::memory_order_release);
}

void Receive_Dialog::on_update_tick(void *user_data)
{
    Receive_Dialog *self = reinterpret_cast<Receive_Dialog *>(user_data);

    std::atomic_thread_fence(std::memory_order_acquire);
    self->val_rx_programs->value(self->rx_programs_);
    self->val_rx_messages->value(self->rx_messages_);

    Fl::repeat_timeout(update_tick_interval, &on_update_tick, user_data);
}
