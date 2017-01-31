#include "mixer.h"

#include "speaker.h"
#include "gui.h"

#include <QMetaObject>
#include "flexiblesoundsystem.h"

CMixer::CMixer(FlexibleSoundSystem* parent)
    : m_parent(parent),
      m_raw_audio(nullptr), m_frame_index(0),
      m_cache_index(0), m_playing(false),
      m_decoding(false), m_mode_stream(false),
      m_mtx_playing(PTHREAD_MUTEX_INITIALIZER),
      m_mtx_decoding(PTHREAD_MUTEX_INITIALIZER),
      m_mtx_frame_index(PTHREAD_MUTEX_INITIALIZER),
      m_mtx_mode_stream(PTHREAD_MUTEX_INITIALIZER)
{
    if (initSem() == true) {
        av_register_all();
        avcodec_register_all();
    }
}

CMixer::~CMixer() {
    stop();
    unloadMusic();
    destroySem();
}

bool CMixer::initSem() {
    int ret;
    ret = sem_init(&m_sem_decode_to_audioop, 0, 0);
    if (checkReturn(ret) == false) {
        return false;
    }
    return true;
}

void CMixer::destroySem() {
    sem_destroy(&m_sem_decode_to_audioop);
}

bool CMixer::checkReturn(int ret) {
    if (ret != 0) {
        int err = errno;
        std::cerr << strerror(err) << std::endl;
        return false;
    }
    return true;
}

bool CMixer::allocFrames(int nframes) {
    m_nframes = nframes;
    m_audio_frame.resize(m_nframes);
    return true;
}

void CMixer::deallocFrames() {
    for (int i = 0; i < m_cache_index; ++i) {
        av_frame_free(&m_audio_frame[i]);
    }
    m_cache_index = 0;
}

void CMixer::allocRawFrames(int frame_size) {
    m_raw_audio = new uint8_t[frame_size];
}

void CMixer::deallocRawFrames() {
    delete[] m_raw_audio;
}

void CMixer::setFrameIndex(int index) {
    if (index > m_nframes || index < 0) {
        return;
    }
    pthread_mutex_lock(&m_mtx_frame_index);
    m_frame_index = index;
    pthread_mutex_unlock(&m_mtx_frame_index);
}

int CMixer::getFrameIndex() {
    int ret;
    pthread_mutex_lock(&m_mtx_frame_index);
    ret = m_frame_index;
    pthread_mutex_unlock(&m_mtx_frame_index);
    return ret;
}

int CMixer::incrementFrameIndex() {
    int ret;
    pthread_mutex_lock(&m_mtx_frame_index);
    ret = ++m_frame_index;
    pthread_mutex_unlock(&m_mtx_frame_index);
    return ret;
}

bool CMixer::isReproducing() {
    bool ret;
    pthread_mutex_lock(&m_mtx_playing);
    ret = m_playing;
    pthread_mutex_unlock(&m_mtx_playing);
    return ret;
}

void CMixer::setReproducing(bool mode) {
    pthread_mutex_lock(&m_mtx_playing);
    m_playing = mode;
    pthread_mutex_unlock(&m_mtx_playing);
}

bool CMixer::isStreaming() {
    bool ret;
    pthread_mutex_lock(&m_mtx_mode_stream);
    ret = m_mode_stream;
    pthread_mutex_unlock(&m_mtx_mode_stream);
    return ret;
}

void CMixer::setStreaming(bool mode) {
    pthread_mutex_lock(&m_mtx_mode_stream);
    m_mode_stream = mode;
    pthread_mutex_unlock(&m_mtx_mode_stream);
}

void CMixer::toggleOutputMode() {
    if (isStreaming()) {
        setStreaming(false);
    } else {
        setStreaming(true);
    }
}

bool CMixer::setNew(SMusic& music, SArtwork& artwork) {
    // setup decoder to this music
    if (m_decoder.setup(music) == false) {
        std::cerr << "couldn't setup music decoder..."
                  << std::endl;
        return false;
    }
    // retrieve artwork
    if (m_decoder.setup(artwork) == false) {
        std::cerr << "couldn't setup artwork..." << std::endl;
        // no need to return false
    }
    if (m_encoder.setup(m_decoder.getContext()) == false) {
        std::cerr << "couldn't setup encoder..."
                  << std::endl;
        return false;
    }
    // alloc necessary frames to store the music
    if (!allocFrames(music.m_nframes)) {
        std::cerr << "Couldn't alloc avframes..." << std::endl;
        return false;
    }
    allocRawFrames(m_decoder.getFrameSize() * m_decoder.getNChannels() * 2);
    // setup speaker params
    m_speaker.setup(music.m_sample_rate, music.m_channels, music.m_frame_size/*, m_decoder.isPlanar()*/);
    return true;
}

void CMixer::outputMusic() {
    int frame_index = getFrameIndex();
    int size = m_audio_frame[frame_index]->buf[0]->size;
    int index = 0;

    // populate raw audio data
    for (int i = 0; i < size; i += m_decoder.getNChannels()) {
        for (int j = 0; j < 2; ++j) {
            m_raw_audio[index++] = m_audio_frame[frame_index]->buf[j]->data[i];
            m_raw_audio[index++] = m_audio_frame[frame_index]->buf[j]->data[i + 1];
        }
    }
    if (m_speaker.play(m_raw_audio, m_decoder.getFrameSize())) {
        std::cout << "raw audio frame sent..." << std::endl;
    }
}

void CMixer::streamMusic() {
    AVPacket packet;
    int frame_index = getFrameIndex();

    av_init_packet(&packet);
    packet.data = NULL;
    packet.size = 0;

    m_encoder.encode(&m_audio_frame[frame_index], &packet);
    if (m_streamer.transmit(packet.data, packet.size) == true) {
        std::cout << "frame sent successfully" << std::endl;
    }

}

void CMixer::filterMusic() {

}

void CMixer::unloadMusic() {
    int ret;

    stopDecoding();
    ret = pthread_join(m_audiodecode_handle, NULL);
    if (ret < 0) {
        std::cerr << "error joining decode thread..."
            << strerror(ret) << std::endl;
    }

    deallocFrames();
    deallocRawFrames();
    m_decoder.reset();
    m_encoder.reset();
}

bool CMixer::isDecoding() {
    bool ret;
    pthread_mutex_lock(&m_mtx_decoding);
    ret = m_decoding;
    pthread_mutex_unlock(&m_mtx_decoding);
    return ret;
}

void CMixer::setDecoding(bool mode) {
    pthread_mutex_lock(&m_mtx_decoding);
    m_decoding = mode;
    pthread_mutex_unlock(&m_mtx_decoding);
}

bool CMixer::startDecoding() {
    if (isDecoding() == false) {
        int thread_policy;
        pthread_attr_t pthread_attr;
        struct sched_param pthread_param;

        pthread_attr_init(&pthread_attr);
        pthread_attr_getschedpolicy(&pthread_attr, &thread_policy);
        pthread_attr_getschedparam(&pthread_attr, &pthread_param);

        pthread_attr_setschedpolicy (&pthread_attr, SCHED_RR);
        pthread_param.__sched_priority = 6;
        pthread_attr_setschedparam(&pthread_attr, &pthread_param);
        pthread_attr_setinheritsched(&pthread_attr, PTHREAD_EXPLICIT_SCHED);

        int ret;
        /* create and start AudioDecode thread */
        ret = pthread_create(&m_audiodecode_handle, &pthread_attr, tAudioDecode, this);
        if (ret < 0) {
            std::cerr << "Couldn't create thread. ERROR: " << ret << std::endl;
            return false;
        }
        return true;
    } else {
        std::cerr << "stop the decoding thread first..." << std::endl;
        return false;
    }
}

void CMixer::cacheFrame(AVFrame* frame) {
    m_audio_frame[m_cache_index++] = av_frame_clone(frame);
}

bool CMixer::start() {
    // create and start audio reproduction thread
    int thread_policy;
    pthread_attr_t pthread_attr;
    struct sched_param pthread_param;

    pthread_attr_init(&pthread_attr);
    pthread_attr_getschedpolicy(&pthread_attr, &thread_policy);
    pthread_attr_getschedparam(&pthread_attr, &pthread_param);

    /* create and start AudioOutput thread */
    pthread_attr_setschedpolicy (&pthread_attr, SCHED_RR);
    pthread_param.__sched_priority = 10;
    pthread_attr_setschedparam(&pthread_attr, &pthread_param);
    pthread_attr_setinheritsched(&pthread_attr, PTHREAD_EXPLICIT_SCHED);

    // flag reproduction activation
    setReproducing(true);

    int ret;
    ret = pthread_create(&m_audiooutput_handle, &pthread_attr, tAudioOutput, this);
    if (ret < 0) {
        setReproducing(false);
        std::cerr << "Couldn't create thread. ERROR: " << ret << std::endl;
        return false;
    }
    return true;
}

void CMixer::pause() {
    int ret;

    setReproducing(false);

    ret = pthread_join(m_audiooutput_handle, NULL);
    if (ret < 0) {
        std::cerr << "error joining decode thread..."
            << strerror(ret) << std::endl;
    }
}

void CMixer::stop() {
    pause();
    resetFrameIndex();
}

void* CMixer::tAudioDecode(void* arg) {
    CMixer* const mixer = static_cast<CMixer* const>(arg);
    AVFrame* frame;
    int max_frames = mixer->m_nframes;
    int ret;

    mixer->setDecoding(true);
    for (int i = 0; i < max_frames; ) {
        if (mixer->isDecoding() == false) {
            break;
        }
        frame = av_frame_alloc();
        /* decode music frame */
        ret = mixer->m_decoder.decode(&frame, i);
        switch (ret) {
        case 0:
            mixer->cacheFrame(frame);
            /* let audio output thread do the job */
            if (sem_post(&mixer->m_sem_decode_to_audioop) != 0) {
                // handle error
            }
        case 1:
            // not audio frame
            ++i;
            break;
        default:
            // averror
            break;
        }
        av_frame_free(&frame);
    }
    pthread_exit(NULL);
}

void* CMixer::tAudioOutput(void* arg) {
    CMixer* const mixer = static_cast<CMixer* const>(arg);
    int max_frames = mixer->m_nframes;
    int i = mixer->getFrameIndex();

    mixer->m_speaker.preparePcm();
    while (i < max_frames) {
        if (mixer->isReproducing() == false) {
            break;
        }
        // take the semaphore to start reproduction
        if (!sem_wait(&mixer->m_sem_decode_to_audioop)) {
            mixer->filterMusic();
            // output music
            if (mixer->isStreaming()) {
                mixer->streamMusic();
            } else {
                mixer->outputMusic();
            }
            i = mixer->incrementFrameIndex();
        }
    }
    mixer->m_speaker.stop();
    QMetaObject::invokeMethod(mixer->m_parent, "handleStop");
    pthread_exit(NULL);
}
