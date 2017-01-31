#ifndef __SPEAKER_H__
#define __SPEAKER_H__
#pragma once

#include <alsa/asoundlib.h>
#include <string>

#define DEVICE  "default"

class CSpeaker {
    std::string m_device;
    unsigned int m_samplerate;
    unsigned int m_channels;
    bool m_isok;
    int m_nsamples;

    snd_pcm_t* m_pcm;
    snd_pcm_hw_params_t* m_hw_params;
    snd_pcm_access_t m_pcm_access;

    // pcm related function-members
    bool setupPcm();
    bool openPcm();
    bool initHwParams();
    bool setHwParams();
public:
    // ctor and dtor
    CSpeaker(std::string = DEVICE);
    ~CSpeaker();

    void setup(unsigned int, unsigned int, int/*, bool*/);
    void stop();
    bool playInterleaved(const void*, size_t);
    bool playNonInterleaved(void**, size_t);
    bool play(const void*, size_t);

    bool preparePcm(); // not needed
};

#endif // __SPEAKER_H__
