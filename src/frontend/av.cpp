#include <iostream>
#include "av.hpp"

// Based on https://github.com/leandromoreira/ffmpeg-libav-tutorial

// For low latency settings: http://trac.ffmpeg.org/wiki/StreamingGuide

using std::cout;
using std::cerr;
using std::endl;


namespace frontend {
    AVStream::AVStream() : _video(nullptr), _audio(nullptr), _packet(nullptr), _videoIdx(-1), _audioIdx(-1) {
        _formatCtx = avformat_alloc_context();
        _packet = av_packet_alloc();
    }

    AVStream::~AVStream() {
        if (_video)
            avcodec_free_context(&_video);
        if (_audio)
            avcodec_free_context(&_audio);
        avformat_close_input(&_formatCtx);
        avformat_free_context(_formatCtx);
        av_packet_free(&_packet);
    }

    bool AVStream::open(const char *inputPath) {
        _formatCtx->flags = AVFMT_FLAG_NOBUFFER | AVFMT_FLAG_FLUSH_PACKETS;
        AVDictionary *options = nullptr;
        av_dict_set(&options, "protocol_whitelist", "file,udp,rtp", 0);
        av_dict_set(&options, "max_delay", "0", 0);

        // Set socket buffer size to 20MB to allow high quality videos without artifacts.
        av_dict_set(&options, "buffer_size", "20971520", 0);

        if (avformat_open_input(&_formatCtx, inputPath, nullptr, &options) < 0) {
            cerr << "Failed to open " << inputPath << endl;
            if (options)
                av_dict_free(&options);
            return false;
        }

        cout << "Opened input " << inputPath << endl;
        if (options)
            av_dict_free(&options);
        cout << "Format: " << _formatCtx->iformat->name << endl;

        if (avformat_find_stream_info(_formatCtx, nullptr) < 0) {
            cerr << "Cannot find stream info\n";
            return false;
        }

        const AVCodec *codec = nullptr;

        _videoIdx = av_find_best_stream(_formatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0);

        if (_videoIdx < 0) {
            cerr << "Failed to find suitable video stream\n";
        } else {
            AVCodecParameters *params = _formatCtx->streams[_videoIdx]->codecpar;
            cout << "Found video stream:\n";
            cout << "\tCodec: " << codec->long_name << " (" << codec->id << ")" << endl;
            cout << "\tBitrate: " << params->bit_rate << endl;
            cout << "\tResolution: " << params->width << "x" << params->height << endl;
            _video = _create_codec(codec, params);
        }

        _audioIdx = av_find_best_stream(_formatCtx, AVMEDIA_TYPE_AUDIO, -1, -1, &codec, 0);

        if (_audioIdx < 0) {
            cerr << "Failed to find suitable audio stream\n";
        } else {
            AVCodecParameters *params = _formatCtx->streams[_audioIdx]->codecpar;
            cout << "Found audio stream:\n";
            cout << "\tCodec: " << codec->long_name << " (" << codec->id << ")" << endl;
            cout << "\tBitrate: " << params->bit_rate << endl;
            cout << "\tChannels: " << params->ch_layout.nb_channels << endl;
            cout << "\tSample format: " << av_get_sample_fmt_name(static_cast<AVSampleFormat>(params->format)) << endl;
            cout << "\tSample rate: " << params->sample_rate << endl;
            _audio = _create_codec(codec, params);
        }

        return true;
    }

    bool AVStream::readPacket() {
        if (av_read_frame(_formatCtx, _packet) < 0)
            return false;

        if (_packet->stream_index == _videoIdx)
            avcodec_send_packet(_video, _packet);
        else if (_packet->stream_index == _audioIdx)
            avcodec_send_packet(_audio, _packet);

        av_packet_unref(_packet);
        return true;
    }

    bool AVStream::retrieveFrame(AVCodecContext* codec, AVFrame* frame) const {
        return avcodec_receive_frame(codec, frame) != AVERROR_EOF;
    }

    AVCodecContext *AVStream::_create_codec(const AVCodec *codec, const AVCodecParameters *params) {
        AVCodecContext *codecContext = avcodec_alloc_context3(codec);

        if (!codecContext) {
            cerr << "Failed to allocate codec context\n";
            return nullptr;
        }

        if (avcodec_parameters_to_context(codecContext, params) < 0) {
            cerr << "Failed to initialize codec\n";
            avcodec_free_context(&codecContext);
            return nullptr;
        }

        // Set codec to automatically determine how many threads suits best for the decoding job
        codecContext->thread_count = 0;
        codecContext->thread_type = FF_THREAD_SLICE;
        codecContext->flags |= AV_CODEC_FLAG_LOW_DELAY;
        codecContext->flags |= AV_CODEC_FLAG2_FAST;
        codecContext->delay = 0;

        if (params->codec_type == AVMEDIA_TYPE_VIDEO) {
            codecContext->max_b_frames = 0;
        }

        AVDictionary *options = nullptr;
        av_dict_set(&options, "preset", "ultrafast", 0);
        av_dict_set(&options, "tune", "zerolatency", 0);

        if (avcodec_open2(codecContext, codec, nullptr) < 0) {
            cerr << "Failed to open codec\n";
            avcodec_free_context(&codecContext);
            av_dict_free(&options);
            return nullptr;
        }

        av_dict_free(&options);
        return codecContext;
    }

    AVCodecContext* AVStream::audio() {
        return _audio;
    }

    AVCodecContext* AVStream::video() {
        return _video;
    }

    AVFormatContext* AVStream::format() {
        return _formatCtx;
    }


    Frame::Frame() {
        _frame = av_frame_alloc();
    }

    Frame::~Frame() {
        av_frame_free(&_frame);
    }

    AVFrame *Frame::get() {
        return _frame;
    }

    const AVFrame *Frame::get() const {
        return _frame;
    }
} // namespace frontend
