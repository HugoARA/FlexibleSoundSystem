#ifndef __GUI_H__
#define __GUI_H__
#pragma once

#include <pthread.h>
#include <list>
#include <string>
#include <mqueue.h>
#include "mixer.h"
#include "midicontroller.h"

#define MQ_CLIENTCMD    "/clientcmd"
#define MQ_MIDIDAEMON   "/mididaemon"

class CMidiController;
class FlexibleSoundSystem;
struct SMidiMsg;

struct SMusic {
    std::string m_path;
    std::string m_artist;
    std::string m_title;
    std::string m_album;
    std::string m_genre;
    std::string m_artwork_path;
    int64_t m_duration;   // seconds
    int64_t m_bitrate;    // kbps
    int m_sample_rate;    // Hz
    int m_channels;
    int m_frame_size;
    int64_t m_nframes;
    // operator
    friend std::ostream& operator<<(std::ostream& os, const SMusic& music);
};

struct SArtwork {
    std::vector<uint8_t> m_raw_buffer;
    int m_width;
    int m_height;
    int m_format;
};

class CGui {
    FlexibleSoundSystem* m_parent;
    std::string m_selected_music;
    CMixer m_mixer;
    std::list<CMidiController*> m_controller;
    // music
    SMusic m_music;
    // artwork
    SArtwork m_artwork;
    // flags
    bool m_music_loaded;
    // message queue objects
    mqd_t m_mq_midicmd;
    mqd_t m_mq_clientcmd;
    mqd_t m_mq_mididaemon;
    // pthread handles
    pthread_t m_checkmidi_handle;
    pthread_t m_processmidi_handle;
    // threads
    static void* tCheckMidi(void*);
    static void* tProcessMidi(void*);
    // setup and destroy ipc methods
    bool createMQueues();
    void destroyMQueues();
    bool setupThreads();
    // init connected midi controllers
    // int setupMidiController();
    void addController(const SMidiStatus&);
    void removeController(const SMidiStatus&);
    void destroyControllers();
    // midi verifications
    void verifyController(const SMidiStatus&);
    // commands to Qt GUI
    void emitSignals(SMidiMsg* option);
public:
    // ctor and dtor
    CGui(FlexibleSoundSystem* parent);
    ~CGui();

    CGui(const CGui&) = delete;
    CGui& operator=(const CGui&) = delete;

    bool isReproducing() { return m_mixer.isReproducing(); }
    bool isMusicLoaded() { return m_music_loaded; }
    bool loadMusic(const std::string&);
    void printMusicInfo();

    void removeMusic();

    const SMusic& getMusicInfo() { return m_music; }
    const SArtwork& getArtwork() { return m_artwork; }

    void startDecoding();

    void playMusic();
    void pauseMusic();
    void stopMusic();
};

#endif // __GUI_H__
