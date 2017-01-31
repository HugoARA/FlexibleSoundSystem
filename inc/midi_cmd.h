#ifndef MIDI_CMD_H
#define MIDI_CMD_H
#pragma once

struct SMidiMsg {
    char m_type;
    SMidiMsg(char type = 'C') : m_type(type) {}
};

struct SMidiCmd : public SMidiMsg {
    char m_cmd;
    SMidiCmd(char type = 'C') : SMidiMsg(type) {}
};

struct SMidiKnob : public SMidiMsg {
    unsigned char m_knob;
    unsigned char m_value;
    SMidiKnob(unsigned char knob, unsigned char value, char cmd = 'K')
        : SMidiMsg(cmd), m_knob(knob), m_value(value) {}
};

#endif // MIDI_CMD_H
