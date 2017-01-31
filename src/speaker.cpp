#include "speaker.h"
#include <iostream>

#define     SRATE               44100U
#define     CHANNELS            2
#define     SAMPLES_PER_FRAME   1152

CSpeaker::CSpeaker(std::string device)
        : m_device(device),
          m_samplerate(SRATE),
          m_channels(CHANNELS),
          m_isok(false),
          m_nsamples(SAMPLES_PER_FRAME),
          m_pcm_access(SND_PCM_ACCESS_RW_INTERLEAVED)
{
    if (setupPcm() == false) {
        std::cerr << "couldn't setup pcm sound card..." << std::endl;
    }
}

CSpeaker::~CSpeaker() {
    snd_pcm_hw_params_free(m_hw_params);
    snd_pcm_close(m_pcm);
}

bool CSpeaker::setupPcm() {
    if (openPcm() == false) {
        return false;
    }
    if (initHwParams() == false) {
        return false;
    }
    return true;
}

bool CSpeaker::initHwParams() {
    int err_snd;
    /* alloc hw params */
    err_snd = snd_pcm_hw_params_malloc(&m_hw_params);
    if (err_snd < 0) {
        std::cerr << "cannot allocate hardware parameter structure. "
                  << snd_strerror(err_snd) << std::endl;
        return false;
    }
    /* initialize hw params */
    err_snd = snd_pcm_hw_params_any(m_pcm, m_hw_params);
    if (err_snd < 0) {
        std::cerr << "cannot initialize hardware parameter structure. "
                  << snd_strerror(err_snd) << std::endl;
        return false;
    }
    return true;
}

bool CSpeaker::openPcm() {
    int err_snd;
    /* open pcm sound card */
    err_snd = snd_pcm_open(&m_pcm, m_device.c_str(), SND_PCM_STREAM_PLAYBACK, 0);
    if (err_snd < 0) {
        std::cerr << "cannot open audio device " + m_device
                  << "" << std::endl;
        return false;
    }
    return true;
}

bool CSpeaker::setHwParams() {
    int err_snd;
    /* set pcm access */
    err_snd = snd_pcm_hw_params_set_access(m_pcm, m_hw_params, m_pcm_access);
    if (err_snd < 0) {
        std::cerr << "cannot set access type. "
                 << snd_strerror(err_snd) << std::endl;
        return false;
    }
    /* set playback format */
    err_snd = snd_pcm_hw_params_set_format(m_pcm, m_hw_params, SND_PCM_FORMAT_S16_LE);
    if (err_snd < 0) {
        std::cerr << "cannot set format. "
                  << snd_strerror(err_snd) << std::endl;
        return false;
    }
    /* set pcm sampling rate */
    err_snd = snd_pcm_hw_params_set_rate_near(
            m_pcm, m_hw_params, &m_samplerate, NULL);
    if (err_snd < 0) {
        std::cerr << "cannot set sample rate. "
                  << snd_strerror(err_snd) << std::endl;
        return false;
    }
    /* set pcm output number of channels */
    err_snd = snd_pcm_hw_params_set_channels(m_pcm, m_hw_params, m_channels);
    if ((err_snd) < 0) {
        std::cerr << "cannot set channel count. "
                  << snd_strerror(err_snd) << std::endl;
        return false;
    }
    /* set pcm buffer size */
    err_snd = snd_pcm_hw_params_set_buffer_size(m_pcm, m_hw_params, m_nsamples * m_channels);
    if (err_snd < 0) {
        std::cerr << "cannot set buffer size... "
                  << snd_strerror(err_snd) << std::endl;
    }
    /* set pcm buffer period size */
    err_snd = snd_pcm_hw_params_set_period_size(m_pcm, m_hw_params, m_nsamples, 0);
    if (err_snd < 0) {
        std:: cerr << "cannot set period size... "
                   << snd_strerror(err_snd) << std::endl;
    }
    /* comit hw params */
    err_snd = snd_pcm_hw_params(m_pcm, m_hw_params);
    if (err_snd < 0) {
        std::cerr << "cannot set parameters. "
                  << snd_strerror(err_snd) << std::endl;
        return false;
    }
    return true;
}

bool CSpeaker::preparePcm() {
    int err_snd;

    if ((err_snd = snd_pcm_prepare(m_pcm)) < 0) {
        std::cerr << "cannot prepare audio interface for use. "
                  << snd_strerror(err_snd) << std::endl;
        return false;
    }

    return true;
}

bool CSpeaker::playInterleaved(const void* buf, size_t size) {
    if (m_isok) {
        snd_pcm_writei(m_pcm, buf, size);
        return true;
    }
    return false;
}

bool CSpeaker::playNonInterleaved(void** buf, size_t size) {
    if (m_isok) {
        snd_pcm_writen(m_pcm, buf, size);
        return true;
    }
    return false;
}

bool CSpeaker::play(const void* buf, size_t size) {
    return playInterleaved(buf, size);
}

void CSpeaker::stop() {
    /* Stop PCM device and drop pending frames */
    snd_pcm_drop(m_pcm);
    /* Stop PCM device after pending frames have been played */
    snd_pcm_drain(m_pcm);
}

void CSpeaker::setup(
        unsigned int samplerate,
        unsigned int channels,
        int nsamples/*,
        bool is_planar*/)
{
//    m_pcm_access = is_planar
//            ? SND_PCM_ACCESS_RW_NONINTERLEAVED
//            : SND_PCM_ACCESS_RW_INTERLEAVED;
    m_samplerate = samplerate;
    m_channels = channels;
    m_nsamples = nsamples;
    if (setHwParams() == false) {
        return;
    }
    m_isok = true;
}
