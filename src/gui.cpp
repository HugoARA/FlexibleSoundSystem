#include "gui.h"
#include "mixer.h"

#include <QFile>
#include <QMetaObject>
#include "flexiblesoundsystem.h"
#include "midi_cmd.h"

#define MSGSIZE     64

CGui::CGui(FlexibleSoundSystem* parent)
    : m_parent(parent), m_mixer(parent),
      m_music_loaded(false)
{
    if (createMQueues() == false) {
        std::cerr << "couldn't create message queues..."
                  << std::endl;
    } else {
        if (setupThreads() == false) {
            std::cerr << "couldn't create threads..."
                      << std::endl;
        }
    }
    /*int n = setupMidiController();
    std::cout << "initialized " << n
              << "midi controllers at start-up"
              << std::endl;*/
}

CGui::~CGui() {
    destroyMQueues();
    destroyControllers();
}

/*int CGui::setupMidiController() {
    int ret = 0;
    int portcount = CMidiController::getPortCount();
    if (portcount > 0) {
        for (int i = 0; i < portcount; ++i) {
            std::string name = CMidiController::getPortName(i);
            std::cout << "name = " << name << std::endl;
            if (name.find("LPD8") != std::string::npos) { // if device has been found, break from loop
                std::cout << name << std::endl;
                m_controller.push_back(new CAkaiLpd8(i));
                ++ret;
            } else if (name.find("VCI-380") != std::string::npos) {
                std::cout << name << std::endl;
                m_controller.push_back(new CVci380(i));
                ++ret;
            }
        }
    }
    return ret;
}*/

bool CGui::createMQueues() {
    // create, set read only for this scope
    // permissions to read write and execute to user and group
    // default params
    m_mq_midicmd = mq_open(MQ_MIDICMD, O_CREAT | O_RDONLY, S_IRWXU | S_IRWXG, NULL);
    m_mq_clientcmd = mq_open(MQ_CLIENTCMD, O_CREAT | O_WRONLY, S_IRWXU | S_IRWXG, NULL);
    m_mq_mididaemon = mq_open(MQ_MIDIDAEMON, O_RDONLY);

    mq_attr attr;
    mq_getattr(m_mq_mididaemon, &attr);
    std::cout << "messages waiting = " << attr.mq_curmsgs << '\n'
              << "flags = " << attr.mq_flags << '\n'
              << "max messages = " << attr.mq_maxmsg << '\n'
              << "max message size = " << attr.mq_msgsize
              << std::endl;

    // return false if couldn't create or open the message queues.
    return !(m_mq_clientcmd == (mqd_t)-1 ||
            m_mq_midicmd == (mqd_t)-1 ||
            m_mq_mididaemon == (mqd_t)-1);
}

void CGui::destroyMQueues() {
    /* close messages */
    if (mq_close(m_mq_midicmd) == -1) {
        std::cerr << "Error closing midicmd mqueue. ERROR: "
                  << strerror(errno) << std::endl;
    }
    if (mq_close(m_mq_mididaemon) == -1) {
        std::cerr << "Error closing mididaemon mqueue. ERROR: "
                  << strerror(errno) << std::endl;
    }
    if (mq_close(m_mq_clientcmd) == -1) {
        std::cerr << "Error closing clientcmd mqueue. ERROR: "
                  << strerror(errno) << std::endl;
    }
    /* remove from vfs */
    if (mq_unlink(MQ_MIDICMD) == -1) {
        perror("Removing MQueue error.");
    }
    if (mq_unlink(MQ_CLIENTCMD) == -1) {
        perror("Removing clientcmd MQueue error.");
    }
}

bool CGui::setupThreads() {
    int thread_policy;
    pthread_attr_t pthread_attr;
    struct sched_param pthread_param;

    pthread_attr_init(&pthread_attr);
    pthread_attr_getschedpolicy(&pthread_attr, &thread_policy);
    pthread_attr_getschedparam(&pthread_attr, &pthread_param);

    pthread_attr_setschedpolicy (&pthread_attr, SCHED_RR);
    pthread_param.__sched_priority = 8;
    pthread_attr_setschedparam(&pthread_attr, &pthread_param);
    pthread_attr_setinheritsched(&pthread_attr, PTHREAD_EXPLICIT_SCHED);

    int ret;
    /* create and start ProcessMidi thread */
    ret = pthread_create(&m_processmidi_handle, &pthread_attr, tProcessMidi, this);
    if (ret < 0) {
        std::cerr << "Couldn't create thread. ERROR: " << ret << std::endl;
        return false;
    }
    /* create and start CheckMidi thread */
    pthread_param.__sched_priority = 7;
    pthread_attr_setschedparam(&pthread_attr, &pthread_param);
    pthread_attr_setinheritsched(&pthread_attr, PTHREAD_EXPLICIT_SCHED);

    ret = pthread_create(&m_checkmidi_handle, &pthread_attr, tCheckMidi, this);
    if (ret < 0) {
        std::cerr << "Couldn't create thread. ERROR: " << ret << std::endl;
        return false;
    }
    return true;
}

void CGui::addController(const SMidiStatus& midistatus) {
    std::string name(midistatus.name);

    if (name == "LPD8") {
        m_controller.push_back(new CAkaiLpd8(midistatus.port_number));
        std::cout << "added lpd8 midi controller..." << std::endl;
    } else if (name == "VCI-380") {
        m_controller.push_back(new CVci380(midistatus.port_number));
        std::cout << "added vci-380 midi controler..." << std::endl;
    } else {
        std::cout << "unrecognized midi..." << std::endl;
    }
}

void CGui::removeController(const SMidiStatus& midistatus) {
    std::list<CMidiController*>::iterator it;
    for (it = m_controller.begin(); it != m_controller.end();) {
        if ((*it)->getPort() == midistatus.port_number) {
            delete *it;
            it = m_controller.erase(it);
        } else {
            ++it;
        }
    }
}

void CGui::destroyControllers() {
    std::list<CMidiController*>::iterator it;
    for (it = m_controller.begin(); it != m_controller.end();) {
        delete *it;
        it = m_controller.erase(it);
    }
}

void CGui::verifyController(const SMidiStatus& midi_status) {
    switch (midi_status.status) {
    case 'C' :
        // midi connected
        std::cout << "controller added..." << std::endl;
        addController(midi_status);
        break;
    case 'D' :
        // midi disconnected
        std::cout << "controller removed..." << std::endl;
        removeController(midi_status);
        break;
    }
}

bool CGui::loadMusic(const std::string& music) {
    if (m_music_loaded) {
        return false;
    }
    m_music.m_path = music;
    if (m_mixer.setNew(m_music, m_artwork)) {
        m_music_loaded = true;
        return true;
    } else {
        std::cerr << "failed to set new music..." << std::endl;
        return false;
    }
}

void* CGui::tCheckMidi(void* arg) {
    CGui* gui = static_cast<CGui*>(arg);
    ssize_t message_size;
    SMidiStatus mstatus;
    unsigned int priority;

    for (;;) {
        message_size = mq_receive(gui->m_mq_mididaemon, (char*)&mstatus,
                                  8192/*sizeof(SMidiStatus) + 1*/, &priority);
        if (message_size == -1) {
            std::cerr << "Error opening midi daemon mqueue."
                      << strerror(errno)
                      << std::endl;
            continue;   // back to the beggining of loop
        }
        // add or remove controller
        gui->verifyController(mstatus);
    }
}

void* CGui::tProcessMidi(void* arg) {
    CGui* gui = static_cast<CGui*>(arg);
    ssize_t message_size;
    unsigned int priority;
    SMidiMsg midi_rcv_msg;

    for (;;) {
        message_size = mq_receive(gui->m_mq_midicmd, reinterpret_cast<char*>(&midi_rcv_msg),
                                  8192, &priority);
        if (message_size == -1) {
            std::cerr << "Error opening midi daemon mqueue."
                      << strerror(errno)
                      << std::endl;
            continue;   // back to the beggining of loop
        }
        std::cout << "sending signal to gui..." << std::endl;
        gui->emitSignals(&midi_rcv_msg);
    }
}

void CGui::emitSignals(SMidiMsg* midi_msg) {
    if (midi_msg->m_type == 'C') {
        SMidiCmd* midi_cmd = static_cast<SMidiCmd*>(midi_msg);
        switch (midi_cmd->m_cmd) {
        case 'P':
            // play
            QMetaObject::invokeMethod(m_parent, "play", Qt::QueuedConnection);
            break;
        case 'S':
            // stop
            QMetaObject::invokeMethod(m_parent, "stop", Qt::QueuedConnection);
            break;
        case 'C':
            // select
            QMetaObject::invokeMethod(m_parent, "selectItem", Qt::QueuedConnection);
            break;
        case 'U':
            // up arrow
            QMetaObject::invokeMethod(m_parent, "moveCursorUp", Qt::QueuedConnection);
            break;
        case 'D':
            // down arrow
            QMetaObject::invokeMethod(m_parent, "moveCursorDown", Qt::QueuedConnection);
            break;
        case 'T':
            // switch outputs
            m_mixer.toggleOutputMode();
            QMetaObject::invokeMethod(m_parent, "toggleOutputMode", Qt::QueuedConnection);
            break;
        default:
            break;
        }
    } else if (midi_msg->m_type == 'K') {
        SMidiKnob* midi_knob = static_cast<SMidiKnob*>(midi_msg);
        m_mixer.setKnobValue(midi_knob->m_knob, midi_knob->m_value);

        QMetaObject::invokeMethod(m_parent, "updateKnob", Q_ARG(int, midi_knob->m_knob), Q_ARG(int, midi_knob->m_value));
    }
}

void CGui::printMusicInfo() {
    std::cout << m_music << std::endl;
}

void CGui::removeMusic() {
    m_mixer.unloadMusic();
    m_music_loaded = false;
}

void CGui::startDecoding() {
    if (m_mixer.startDecoding()) {
        std::cerr << "decoding thread started..." << std::endl;
    } else {
        std::cerr << "failed to start decoding..." << std::endl;
    }
}

void CGui::playMusic() {
    if (m_music_loaded) {
        m_mixer.start();
        std::cout << "starting to play music..." << std::endl;
    }
}

void CGui::pauseMusic() {
    if (m_music_loaded) {
        m_mixer.pause();
        std::cout << "playback paused..." << std::endl;
    }
}

void CGui::stopMusic() {
    if (m_music_loaded) {
        m_mixer.stop();
        std::cout << "playback stopped..." << std::endl;
    }
}

std::ostream& operator<<(std::ostream &os, const SMusic &music) {
    os << "Duration: " << music.m_duration << "s.\n"
       << "Bit Rate: " << music.m_bitrate / 1000 << "kbps.\n"
       << "Sample Rate: " << music.m_sample_rate << "Hz.\n"
       << "Channels: " << music.m_channels << "\n"
       << "Frame size: " << music.m_frame_size
       << "Total of Frames: " << music.m_nframes;
    return os;
}
