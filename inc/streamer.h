#ifndef STREAMER_H
#define STREAMER_H
#pragma once

#include <jrtplib3/rtpsession.h>
#include <jrtplib3/rtpudpv4transmitter.h>
#include <jrtplib3/rtpsessionparams.h>

class CStreamer : public jrtplib::RTPSession {
    const double m_timestamp_unit;
    const uint16_t m_port_base;
    const uint8_t m_payload;
    bool m_running;

    jrtplib::RTPUDPv4TransmissionParams m_transmission_params;
    jrtplib::RTPSessionParams m_session_params;
public:
    CStreamer(uint16_t port_base = 8000, double timestamp_unit = 1 / 90000, uint8_t payload = 14);
    virtual ~CStreamer();

    bool addClient(uint32_t ip, uint16_t port);
    bool addClient(const jrtplib::RTPIPv4Address&);
    bool transmit(const void* buf, size_t size);
    bool checkError(int);
};

#endif // STREAMER_H
