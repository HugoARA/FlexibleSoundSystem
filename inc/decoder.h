#ifndef __DECODER_H__
#define __DECODER_H__
#pragma once

extern "C"
{
    #include "libavcodec/avcodec.h"
    #include "libavutil/imgutils.h"
    #include "libavutil/samplefmt.h"
    #include "libavformat/avformat.h"
    #include "libavutil/timestamp.h"
    #include "libavutil/dict.h"
    #include "libavutil/frame.h"
}

#include <string>
#include <vector>

struct SMusic;
struct SArtwork;

class CDecoder {
private:
    int m_audio_stream_index;
    int m_npackets;
    int m_cache_index;
    AVFormatContext* m_audio_format_context;     // pointer to retrieve music information
    AVCodecContext* m_audio_decoder_context;
    AVStream* m_audio_stream;
    std::vector<AVPacket*> m_audio_packet;                   // array of packets
    std::string m_filepath;

    bool openCodecContext();
    bool allocPackets(int);
    void deallocPackets();
    void fillMusicInfo(SMusic&);
public:
    CDecoder();
    ~CDecoder();
    bool setup(SMusic& music);
    bool setup(SArtwork& artwork);
    AVCodecContext* getContext() { return m_audio_decoder_context; }
    int decode(AVFrame** frame, int frame_index);
    void reset();

    int getNChannels() { return m_audio_stream->codecpar->channels; }
    int getFrameSize() { return m_audio_stream->codecpar->frame_size; }
    bool isPlanar();
};

#endif // __DECODER_H__
