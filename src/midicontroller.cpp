#include "midicontroller.h"
#include <cstring>
#include <new>
#include "midi_cmd.h"

bool CMidiController::slots_used[2] = { false };

CMidiController::CMidiController(int port, std::string name) :
        m_name(name), m_port(port),
        m_controller(nullptr), m_queue_opened(false)
{
    if (openMQueue() == false) {
        std::cerr << "couldn't open midicontroller message queues..."
                  << std::endl;
    } else {
        m_queue_opened = true;

        m_controller = new(std::nothrow) RtMidiIn();
        if (m_controller == nullptr) {
            std::cerr << "couldn't create midi_in object..."
                      << std::endl;
        } else {
            if (openConnection() == true) {
                std::cout << "connection opened at port n: "
                          << m_port << " Name = " << m_name
                          << std::endl;
            }
        }
    }
}

CMidiController::~CMidiController() {
    if (m_controller) {
        if (m_controller->isPortOpen()) {
            // free a slot
            slots_used[m_slot] = false;
            m_controller->cancelCallback();
            m_controller->closePort();
        }
        delete m_controller;
    }
    if (m_queue_opened == true) {
        closeMQueue();
    }
}

void CMidiController::midiCallback(
        double timestamp,
        std::vector<unsigned char>* message,
        void* userdata)
{
    static_cast<CMidiController*>(userdata)->cbMidiIn(timestamp, message, userdata);
}

void CMidiController::cbError(RtMidiError::Type type, const std::string& errortext, void* userdata) {
    // remove warnings
    (void)type;
    (void)userdata;

    std::cerr << "Error: " << errortext << std::endl;
}

bool CMidiController::openMQueue() {
    // S_IRWXU -> write, read and execute permission to user
    // S_IRWXG -> ""     ""   ""  ""      ""         "" group
    // last parameter NULL -> default mqueue options
    m_mq_midicmd = mq_open(MQ_MIDICMD, O_WRONLY);
    return (mqd_t)-1 != m_mq_midicmd;
}

bool CMidiController::closeMQueue() {
    if (mq_close(m_mq_midicmd) == -1) {
        std::cerr << "Error closing mqueue. ERROR: " << strerror(errno) << std::endl;
        return false;
    }
    return true;
}

bool CMidiController::openConnection() {
    for (int i = 0; i < MAX_SLOTS; ++i) {
        if (slots_used[i] == false) {
            m_slot = i;

            m_controller->setCallback(midiCallback, this);
            m_controller->setErrorCallback(cbError);
            m_controller->ignoreTypes(false, false, false);
            m_controller->openPort(m_slot + 1, m_name);

            if (m_controller->isPortOpen()) {
                slots_used[i] = true;
                return true;
            } else {
                std::cerr << "couldn't open port..." << std::endl;
                return false;
            }
        }
    }
    std::cerr << "no slots available..." << std::endl;
    return false;
}

void CMidiController::printRecvCmd(double timestamp,
                                   std::vector<unsigned char>* message)
{
    int nBytes;
    nBytes = message->size();
    for (int i = 0; i < nBytes; ++i) {
        std::cout << "Byte" << i << " = " << (int) (*message)[i] << ", ";
    }
    if (nBytes > 0) {
        std::cout << "stamp = " << timestamp << std::endl;
    }
}

void CAkaiLpd8::cbMidiIn(double timestamp,
                         std::vector<unsigned char>* message,
                         void* userdata)
{
    (void)userdata;
    printRecvCmd(timestamp, message);
    parse(message);
}

void CAkaiLpd8::parse(std::vector<unsigned char>* message) {
    if ((*message)[0] == 0xC0) {
        SMidiCmd midi_cmd('C');

        switch ((*message)[1]) {
        case 0x00:
            // down arrow
            midi_cmd.m_cmd = 'D';
            break;
        case 0x01:
            // select
            midi_cmd.m_cmd = 'C';
            break;
        case 0x02:
            // stop
            midi_cmd.m_cmd = 'S';
            break;
        case 0x03:
            // play pause
            midi_cmd.m_cmd = 'P';
            break;
        case 0x04:
            // up arrow
            midi_cmd.m_cmd = 'U';
            break;
        case 0x05:
            // toggle output
            midi_cmd.m_cmd = 'T';
            break;
        default:
            return;
        }
        if (mq_send(m_mq_midicmd, reinterpret_cast<char*>(&midi_cmd), sizeof(SMidiCmd), 1) == -1) {
            std::cerr << "couldn't send cmd in message queue..." << std::endl;
        }
    } else if ((*message)[0] == 0xB0) {
        SMidiKnob midi_knob((*message)[1], (*message)[2]);

        if (mq_send(m_mq_midicmd, reinterpret_cast<char*>(&midi_knob), sizeof(SMidiKnob), 1) == -1) {
            std::cerr << "couldn't send knob cmd in message queue..." << std::endl;
        }
    }
}

void CVci380::cbMidiIn(double timestamp,
                       std::vector<unsigned char>* message,
                       void* userdata)
{
    (void)userdata;
    printRecvCmd(timestamp, message);
}

void CVci380::parse(std::vector<unsigned char>* message) {
    (void)message;
}
