#ifndef __MIDICONTROLLER_H__
#define __MIDICONTROLLER_H__

#include <pthread.h>
#include <mqueue.h>
#include <iostream>
#include <vector>
#include "RtMidi.h"

#define MQ_MIDICMD  "/midicmd"

#define MAX_SLOTS 2

class RtMidiIn;

struct SMidiStatus {
    char status;
    char name[16];
    uint8_t port_number;
};

class CMidiController {
protected:
    std::string m_name;
    int m_port;
    int m_slot;
    static bool slots_used[MAX_SLOTS];
    RtMidiIn* m_controller;
    mqd_t m_mq_midicmd;

    static void midiCallback(double, std::vector<unsigned char>*, void*);
    static void cbError(RtMidiError::Type, const std::string&, void*);
    virtual void cbMidiIn(double, std::vector<unsigned char>*, void*) = 0;
    virtual void parse(std::vector<unsigned char>*) = 0;
private:
    bool m_queue_opened;

    bool openMQueue();
    bool closeMQueue();
    bool openConnection();
public:
    CMidiController() = delete;
    CMidiController(int, std::string);
    virtual ~CMidiController();

    int getPort() { return m_port; }

    bool isConnected() { return m_controller->isPortOpen(); }
    void printRecvCmd(double timestamp, std::vector<unsigned char>* message);
};

class CAkaiLpd8 : public CMidiController {
protected:
    virtual void cbMidiIn(double, std::vector<unsigned char>*, void*);

    virtual void parse(std::vector<unsigned char>*);
public:
    CAkaiLpd8(int port) : CMidiController(port, "LPD8") {}
    virtual ~CAkaiLpd8() {}
};

class CVci380 : public CMidiController {
protected:
    virtual void cbMidiIn(double, std::vector<unsigned char>*, void*);

    virtual void parse(std::vector<unsigned char>*);
public:
    CVci380(int port) : CMidiController(port, "VCI-380") {}
    virtual ~CVci380() {}
};

#endif // __MIDICONTROLLER_H__
