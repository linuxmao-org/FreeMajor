//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "midi_out_queue.h"
#include "device/midi.h"
#include <FL/Fl.H>

void Midi_Out_Queue::enqueue_message(
    const uint8_t *msg, unsigned length, double interval_after)
{
    Event evt;
    evt.message.assign(msg, msg + length);
    evt.interval_after = interval_after;
    eventq_.push_back(std::move(evt));

    if (!Fl::has_timeout(&on_timeout, this))
        Fl::add_timeout(0.0, &on_timeout, this);
}

void Midi_Out_Queue::cancel_all_sysex()
{
    eventq_.remove_if([](const Event &evt) -> bool {
                          size_t size = evt.message.size();
                          return size >= 2 &&
                              evt.message[0] == 0xf0 &&
                              evt.message[size - 1] == 0xf7;
                      });
}

void Midi_Out_Queue::on_timeout(void *user_data)
{
    Midi_Out_Queue *self = (Midi_Out_Queue *)user_data;

    if (!self->eventq_.empty()) {
        Event evt = std::move(self->eventq_.front());
        self->eventq_.pop_front();

        Midi_Out &out = Midi_Out::instance();
        out.send_message(evt.message.data(), evt.message.size());

        Fl::add_timeout(evt.interval_after, &on_timeout, self);
    }
}
