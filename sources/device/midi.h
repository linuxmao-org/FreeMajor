//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <RtMidi.h>
#include <string>
#include <vector>
#include <memory>
#include <stdint.h>

class Midi_Out {
public:
    static Midi_Out &instance();

    Midi_Out();
    ~Midi_Out();

    bool supports_virtual_port() const;
    std::vector<std::string> get_real_ports();

    void close_port();
    void open_port(unsigned port);

    void send_message(const uint8_t *data, size_t length);

private:
    static void on_midi_error(RtMidiError::Type type, const std::string &text, void *user_data);

    std::unique_ptr<RtMidiOut> client_;
    static std::unique_ptr<Midi_Out> instance_;
};
