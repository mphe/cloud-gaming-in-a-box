#ifndef FRONTEND_AV_HPP
#define FRONTEND_AV_HPP

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}


namespace frontend {
    // Wrapper around AVFrame with automatic allocation and deallocation.
    class Frame {
        public:
          Frame();
          ~Frame();

          AVFrame *get();
          const AVFrame *get() const;

        private:
          AVFrame *_frame;
    };

    class AVStream {
        public:
            AVStream();
            AVStream(const AVStream &) = delete;
            AVStream(AVStream &&) = delete;
            ~AVStream();

            bool open(const char *inputPath);
            bool readPacket();
            bool retrieveFrame(AVCodecContext* codec, AVFrame* frame) const;
            AVCodecContext* video();
            AVCodecContext* audio();
            AVFormatContext* format();

          private:
            static AVCodecContext *_create_codec(const AVCodec *codec, const AVCodecParameters *params);

        private:
            AVFormatContext* _formatCtx;
            AVCodecContext* _video;
            AVCodecContext* _audio;
            AVPacket* _packet;
            int _videoIdx;
            int _audioIdx;
    };
}

#endif
