#ifndef __EQUALIZER_H__
#define __EQUALIZER_H__

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavutil/imgutils.h"
#include "libavutil/samplefmt.h"
#include "libavformat/avformat.h"
#include "libavutil/timestamp.h"
#include "libavutil/dict.h"
#include "libavutil/frame.h"
#include "libavformat/avformat.h"
#include "libavutil/dict.h"
}

#include <semaphore.h>
#include <pthread.h>
#include <iostream>
#include <vector>
#include "decoder.h"
#include "encoder.h"
#include "speaker.h"
#include "streamer.h"
#include "knobs.h"

#define SAMPLES_PER_FRAME   1152
#define N_KNOBS             6

class FlexibleSoundSystem;
class CSpeaker;
struct SMusic;
struct SArtwork;

class CMixer {
    FlexibleSoundSystem* m_parent;
    CSpeaker m_speaker;
    CDecoder m_decoder;
    CEncoder m_encoder;
    CStreamer m_streamer;
    CKnobs m_knob[N_KNOBS];
    // frames
    std::vector<AVFrame*> m_audio_frame;     // array of frames
    uint8_t* m_raw_audio;
    int m_nframes;
    // frame index
    int m_frame_index;
    // cached frame index
    int m_cache_index;
    // reproduction
    bool m_playing;
    bool m_decoding;
    bool m_mode_stream;
    // frames methods
    bool allocFrames(int);
    void deallocFrames();
    // raw audio data
    void allocRawFrames(int);
    void deallocRawFrames();
    // audio output methods
    void outputMusic();
    void streamMusic();
    void filterMusic();
    // thread handles
    pthread_t m_audiodecode_handle;
    pthread_t m_audiooutput_handle;
    // threads functions
    static void* tAudioDecode(void*);
    static void* tAudioOutput(void*);
    /* method to setup threads */
    bool setupThreads();
    // mutexes
    pthread_mutex_t m_mtx_playing;
    pthread_mutex_t m_mtx_decoding;
    pthread_mutex_t m_mtx_frame_index;
    pthread_mutex_t m_mtx_mode_stream;
    // semaphores
    sem_t m_sem_decode_to_audioop;
    // setup semaphores
    bool initSem();
    void destroySem();
    bool checkReturn(int);
    // decoding flag access methods
    bool isDecoding();
    void setDecoding(bool);
    int incrementFrameIndex();

    void cacheFrame(AVFrame*);

    void setReproducing(bool);

    void resetFrameIndex() { setFrameIndex(0); }

    void stopDecoding() { setDecoding(false); }
    bool isStreaming();
    void setStreaming(bool);
public:
    // ctor and dtor
    CMixer(FlexibleSoundSystem* parent);
    ~CMixer();

    CMixer(const CMixer&) = delete;
    CMixer& operator=(const CMixer&) = delete;
    // thread methods
    bool startDecoding();
    // reproduction methods
    bool start();
    void pause();
    void stop();
    // setup methods
    bool setNew(SMusic&, SArtwork&);
    void unloadMusic();

    void setFrameIndex(int);
    int getFrameIndex();

    bool isReproducing();
    void toggleOutputMode();

    void setKnobValue(int knob, int value)
    { m_knob[knob - 1].setValue(value); }
};

#endif	//__EQUALIZER__
