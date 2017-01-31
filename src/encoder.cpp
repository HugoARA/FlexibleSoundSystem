#include "encoder.h"
#include <iostream>

CEncoder::CEncoder()
        : m_audio_encoder_context(nullptr)
{
}

CEncoder::~CEncoder()
{
    reset();
}

void CEncoder::reset() {
    avcodec_free_context(&m_audio_encoder_context);
}

bool CEncoder::setup(AVCodecContext* codec_context,
                     int64_t bitrate, AVCodecID codec_id)
{
    AVCodec* audio_encoder;
    m_bitrate = bitrate;
    m_codec_id = codec_id;
    // Find the respective encoder to the type of music
    audio_encoder = avcodec_find_encoder(codec_id);
    if (audio_encoder == NULL) {
        std::cerr << "Codec not found" << std::endl;
        return false;
    }
    // alloc a codec context for encoder
    m_audio_encoder_context = avcodec_alloc_context3(audio_encoder);
    if (m_audio_encoder_context == NULL) {
        std::cerr << "Could not allocate audio encode context"
                  << std::endl;
        return false;
    }
    // Check if the encode supports PCM Format
    if(!(supports(audio_encoder, AV_SAMPLE_FMT_S16)
            || supports(audio_encoder, AV_SAMPLE_FMT_S16P))) {
        std::cerr << "Encoder does not support s16 and s16p\
                     sample formats..." << std::endl;
        return false;
    }
    // Decoder definition imported to the encoder
    // Use the bitrate = 128000 by default or an user defined if it was written
    m_audio_encoder_context->bit_rate       = bitrate;
    m_audio_encoder_context->sample_rate    = codec_context->sample_rate;
    m_audio_encoder_context->channel_layout = codec_context->channel_layout;
    m_audio_encoder_context->channels       = codec_context->channels;//av_get_channel_layout_nb_channels(codec_context->channel_layout);
    m_audio_encoder_context->sample_fmt     = codec_context->sample_fmt;
    //maybe try to change it to m_audio_decoder_context channels
    //Open the selected codec
    if(avcodec_open2(m_audio_encoder_context, audio_encoder, NULL) < 0){
        std::cerr << "Could not open the codec" << std::endl;
        return false;
    }
    return true;
}

bool CEncoder::encode(AVFrame** frame, AVPacket* packet) {
    int ret;

    ret = avcodec_send_frame
            (m_audio_encoder_context, *frame);
    if (ret < 0) {
        std::cerr << "AVERROR: " << ret << std::endl;
        return false;
    }
    ret = avcodec_receive_packet
            (m_audio_encoder_context, packet);
    if (ret < 0) {
        std::cerr << "AVERROR: " << ret << std::endl;
        return false;
    }
    return true;
}

bool CEncoder::supports(AVCodec* codec, AVSampleFormat sample_fmt)
{
    if (codec != NULL) {
        const AVSampleFormat *p = codec->sample_fmts;

        while (*p != AV_SAMPLE_FMT_NONE) {
            if (*p == sample_fmt)
                return true;
            p++;
        }
    }
    return false;
}
