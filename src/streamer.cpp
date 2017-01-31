#include "streamer.h"

#include <jrtplib3/rtpipv4address.h>
#include <jrtplib3/rtperrors.h>
#include <iostream>

CStreamer::CStreamer(uint16_t port_base, double timestamp_unit, uint8_t payload)
    : m_timestamp_unit(timestamp_unit), m_port_base(port_base),
      m_payload(payload), m_running(false)
{
    int status;

    m_session_params.SetOwnTimestampUnit(m_timestamp_unit);
    m_transmission_params.SetPortbase(m_port_base);

    status = Create(m_session_params, &m_transmission_params);

    if (checkError(status)) {
        std::cerr << "session not created..." << std::endl;
    } else {
        m_running = true;
        uint8_t localip[] = {127, 0, 0, 1};
        jrtplib::RTPIPv4Address addr(localip, 9000);
        addClient(addr);
        SetDefaultPayloadType(m_payload);
        SetDefaultMark(false);
        SetDefaultTimestampIncrement(1152);
    }
}

CStreamer::~CStreamer()
{  }

bool CStreamer::addClient(uint32_t ip, uint16_t port) {
    int status;
    jrtplib::RTPIPv4Address address(ip, ntohl(port));

    status = AddDestination(address);
    if (checkError(status)) {
        std::cerr << "couldn't add new client..." << std::endl;
        return false;
    }
    return true;
}

bool CStreamer::addClient(const jrtplib::RTPIPv4Address& address) {
    int status;

    status = AddDestination(address);
    if (checkError(status)) {
        std::cerr << "couldn't add new client..." << std::endl;
        return false;
    }
    return true;
}

bool CStreamer::checkError(int rtperr) {
    if (rtperr < 0) {
        std::cerr << "ERROR: " << jrtplib::RTPGetErrorString(rtperr) << std::endl;
        return true;
    }
    return false;
}

bool CStreamer::transmit(const void *buf, size_t size) {
    if (m_running == false) {
        return false;
    } else {
        jrtplib::RTPTime time(0.026);
        int status;
        status = SendPacket(buf, size);
        if (checkError(status)) {
            std::cerr << "error transmitting packet..." << std::endl;
            return false;
        }
        jrtplib::RTPTime::Wait(time);
        return true;
    }
}
