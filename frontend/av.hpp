#include <iostream>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
}

class AVCodec;
class AVCodecParameters;
class AVFormatContext;
class AVCodecContext;


namespace frontend {
    class AVStream {
        public:
            AVStream();
            AVStream(const AVStream &) = delete;
            AVStream(AVStream &&) = delete;
            ~AVStream();

            bool open(const char *inputPath);
            bool readPacket();
            bool retrieveFrame(AVCodecContext* codec, AVFrame* frame) const;
            AVCodecContext *video();
            AVCodecContext *audio();

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
