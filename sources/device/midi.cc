//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "midi.h"
#include "app_i18n.h"
#include <stdio.h>

std::unique_ptr<Midi_Out> Midi_Out::instance_;

Midi_Out &Midi_Out::instance()
{
    if (instance_)
        return *instance_;

    Midi_Out *out = new Midi_Out;
    instance_.reset(out);
    return *out;
}

Midi_Out::Midi_Out()
{
    RtMidiOut *client = new RtMidiOut(
        RtMidi::UNSPECIFIED, _("TC G-Major Editor"));
    client_.reset(client);
    client->setErrorCallback(&on_midi_error, this);
}

Midi_Out::~Midi_Out()
{
}

RtMidi::Api Midi_Out::current_api() const
{
    return client_->getCurrentApi();
}

void Midi_Out::switch_api(RtMidi::Api api)
{
    if (api == client_->getCurrentApi())
        return;

    RtMidiOut *client = new RtMidiOut(api, _("TC G-Major Editor"));
    client_.reset(client);
    client->setErrorCallback(&on_midi_error, this);
    has_open_port_ = false;
}

bool Midi_Out::supports_virtual_port() const
{
    RtMidiOut &client = *client_;
    switch (client.getCurrentApi()) {
    case RtMidi::MACOSX_CORE: case RtMidi::LINUX_ALSA: case RtMidi::UNIX_JACK:
        return true;
    default:
        return false;
    }
}

std::vector<std::string> Midi_Out::get_real_ports()
{
    RtMidiOut &client = *client_;
    unsigned count = client.getPortCount();

    std::vector<std::string> ports;
    ports.reserve(count);

    for (unsigned i = 0; i < count; ++i)
        ports.push_back(client.getPortName(i));
    return ports;
}

void Midi_Out::close_port()
{
    RtMidiOut &client = *client_;
    if (has_open_port_) {
        client.closePort();
        has_open_port_ = false;
    }
}

void Midi_Out::open_port(unsigned port)
{
    RtMidiOut &client = *client_;
    close_port();

    std::string name = _("MIDI out");
    if (port == ~0u) {
        client.openVirtualPort(name);
        has_open_port_ = true;
    }
    else {
        client.openPort(port, name);
        has_open_port_ = client.isPortOpen();
    }
}

void Midi_Out::send_message(const uint8_t *data, size_t length)
{
    RtMidiOut &client = *client_;
    if (has_open_port_)
        client.sendMessage(data, length);
}

void Midi_Out::on_midi_error(RtMidiError::Type type, const std::string &text, void *user_data)
{
    fprintf(stderr, "[Midi Out] %s\n", text.c_str());
}
