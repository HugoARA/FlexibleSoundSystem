#include "decoder.h"
#include <iostream>
#include <mixer.h>
#include "gui.h"

#include <cstdio>

CDecoder::CDecoder()
        : m_audio_stream_index(-1),
          m_cache_index(0),
          m_audio_format_context(nullptr),
          m_audio_decoder_context(nullptr),
          m_audio_stream(nullptr)
{
}

CDecoder::~CDecoder()
{
    reset();
}

void CDecoder::reset() {
    deallocPackets();
    avcodec_free_context(&m_audio_decoder_context);
    avformat_close_input(&m_audio_format_context);
}

bool CDecoder::allocPackets(int npackets)
{
    m_npackets = npackets;
    m_audio_packet.resize(npackets);
    return true;
}

void CDecoder::deallocPackets() { 
    for (int i = 0; i < m_cache_index; ++i) {
        av_packet_free(&m_audio_packet[i]);
    }
}

bool CDecoder::isPlanar() {
    if (m_audio_decoder_context) {
        const enum AVSampleFormat* sample_format
                = m_audio_decoder_context->codec->sample_fmts;

        while (*sample_format != AV_SAMPLE_FMT_NONE) {
            if (*sample_format == AV_SAMPLE_FMT_S16P) {
                return true;
            }
            ++sample_format;
        }
    }
    return false;
}

void CDecoder::fillMusicInfo(SMusic& music) {
    AVDictionary* metadata = m_audio_format_context->metadata;

    AVDictionaryEntry* dictionary_entry;

    dictionary_entry = av_dict_get(metadata, "artist", NULL, 0);
    if (dictionary_entry != NULL) {
        music.m_artist = dictionary_entry->value;
    }
    dictionary_entry = av_dict_get(metadata, "title", NULL, 0);
    if (dictionary_entry != NULL) {
        music.m_title = dictionary_entry->value;
    }
    dictionary_entry = av_dict_get(metadata, "album", NULL, 0);
    if (dictionary_entry != NULL) {
        music.m_album = dictionary_entry->value;
    }
    dictionary_entry = av_dict_get(metadata, "genre", NULL, 0);
    if (dictionary_entry != NULL) {
        music.m_genre = dictionary_entry->value;
    }
    music.m_duration = m_audio_format_context->duration / AV_TIME_BASE;
    music.m_bitrate = m_audio_stream->codecpar->bit_rate;
    music.m_sample_rate = m_audio_stream->codecpar->sample_rate;
    music.m_channels = m_audio_stream->codecpar->channels;
    music.m_frame_size = m_audio_stream->codecpar->frame_size;
    music.m_nframes = (float)music.m_duration / ((float)SAMPLES_PER_FRAME / (float)music.m_sample_rate);
    music.m_nframes += 100;
}

bool CDecoder::openCodecContext() {
    int stream_index;
    AVCodec* decoder = NULL;
    AVDictionary* opts = NULL;

    /* find stream index */
    stream_index = av_find_best_stream(m_audio_format_context, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
    if (stream_index < 0) {
        std::cerr << "Could not find "
                  << av_get_media_type_string(AVMEDIA_TYPE_AUDIO)
                  << " stream in input file '" << m_filepath << "'" << std::endl;
        return false;
    }
    m_audio_stream_index = stream_index;
    m_audio_stream = m_audio_format_context->streams[stream_index];

    /*find decoder for the stream */
    decoder = avcodec_find_decoder(m_audio_stream->codecpar->codec_id);
    if (decoder == NULL) {
        std::cerr << "Failed to find "
                  << av_get_media_type_string(AVMEDIA_TYPE_AUDIO)
                  << " codec" << std::endl;
        return false;
    }

    /* Allocate a codec context for the decoder */
    m_audio_decoder_context = avcodec_alloc_context3(decoder);
    if (m_audio_decoder_context == NULL) {
        std::cerr << "Failed to allocate the "
                  << av_get_media_type_string(AVMEDIA_TYPE_AUDIO)
                  << " codec context" << std::endl;
        return false;
    }

    /* Copy codec parameters from input stream to output codec context */
    if (avcodec_parameters_to_context(m_audio_decoder_context, m_audio_stream->codecpar) < 0) {
        std::cerr << "Failed to copy "
                  << av_get_media_type_string(AVMEDIA_TYPE_AUDIO)
                  << " codec parameters to decoder context" << std::endl;
        return false;
    }

    /* Init the decoders, with or without reference counting */
    av_dict_set(&opts, "refcounted_frames", "1", 0);
    if ((avcodec_open2(m_audio_decoder_context, decoder, &opts)) < 0) {
        fprintf(stderr, "Failed to open %s codec\n",
                av_get_media_type_string(AVMEDIA_TYPE_AUDIO));
        return false;
    }
    return true;
}

bool CDecoder::setup(SMusic& music) {
    m_filepath = music.m_path;
    /*Create a stream to the input file */
    if(avformat_open_input(&m_audio_format_context, m_filepath.c_str(), NULL, NULL) < 0){
        std::cerr << "Could not open music file " + m_filepath << std::endl;
        return false;
    }
    /* Read the stream information */
    if(avformat_find_stream_info(m_audio_format_context, NULL) < 0){
        std::cerr << "Could not find stream information" << std::endl;
        return false;
    }
    /* get the adequate decoder for the music */
    if(!openCodecContext()) {
        return false;
    }
    // show info on screen about the file
    av_dump_format(m_audio_format_context, 0, m_filepath.c_str(), 0);
    // fill music info
    fillMusicInfo(music);
    /* allocate frames and packets */
    if (!allocPackets(music.m_nframes)) {
        std::cerr << "Couldn't alloc avpackets..." << std::endl;
        return false;
    }
    // create packet
    AVPacket packet;
    av_init_packet(&packet);
    packet.data = NULL;
    packet.size = 0;
    // read all frames from the file
    int i = 0;
    while (av_read_frame(m_audio_format_context, &packet) >= 0) {
        m_audio_packet[i] = av_packet_clone(&packet);
        av_packet_unref(&packet);
        ++i;
    }
    m_cache_index = i;
    music.m_nframes = i;

    return true;
}

int CDecoder::decode(AVFrame** frame, int frame_index) {
    int ret;

    if (m_audio_packet[frame_index]->stream_index == m_audio_stream_index) {
        if (m_audio_packet[frame_index]->size > 0) {
            // send packet to decoder
            ret = avcodec_send_packet
                    (m_audio_decoder_context,
                     m_audio_packet[frame_index]
                    );
            if (ret < 0) {
                // return ret == AVERROR_EOF ? 0 : ret;
                // other error
                std::cerr << "AVERROR: " << ret << std::endl;
                return ret;
            }
            // receive decoded frame
            avcodec_receive_frame(m_audio_decoder_context, *frame);
        }
    } else {
        // not an audio frame
        std::cerr << "not audio frame..." << std::endl;
        return 1;
    }
    // everything went fine, return 0
    return 0;
}

bool CDecoder::setup(SArtwork& artwork) {
    for (unsigned int i = 0; i < m_audio_format_context->nb_streams; i++) {
        if (m_audio_format_context->streams[i]->disposition & AV_DISPOSITION_ATTACHED_PIC) {
            AVCodecParameters* parameters = m_audio_format_context->streams[i]->codecpar;
            AVPacket pkt = m_audio_format_context->streams[i]->attached_pic;

            std::vector<uint8_t> image(pkt.data, pkt.data + pkt.size);
            artwork.m_raw_buffer = image;
            artwork.m_width = parameters->width;
            artwork.m_height = parameters->height;
            artwork.m_format = parameters->format;

            return true;
        }
    }
    return false;
}
