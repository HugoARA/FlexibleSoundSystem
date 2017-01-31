#ifndef __ENCODER_H__
#define __ENCODER_H__
#pragma once

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavutil/frame.h"
#include "libavformat/avformat.h"
}

class CEncoder {
    int64_t m_bitrate;
    AVCodecID m_codec_id;
    // Store the information about the encode process
    AVCodecContext* m_audio_encoder_context;

    bool supports(AVCodec* codec, AVSampleFormat sample_fmt);
public:
    CEncoder();
    ~CEncoder();
    bool setup(AVCodecContext* codec_context,
               int64_t bitrate = 128000,
               AVCodecID codec_id = AV_CODEC_ID_MP3);
    // Encode the frame based on the codec and the bitrate
    bool encode(AVFrame** frame, AVPacket* packet);
    void reset();
};

#endif //__ENCODER_H__
