//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <vector>
#include <list>
#include <stdint.h>

class Midi_Out_Queue {
public:
    void enqueue_message(const uint8_t *msg, unsigned length, double interval_after);
    void cancel_all_sysex();

private:
    struct Event {
        std::vector<uint8_t> message;
        double interval_after = 0;
    };
    std::list<Event> eventq_;

    static void on_timeout(void *user_data);
};
