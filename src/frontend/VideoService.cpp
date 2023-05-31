#include "VideoService.hpp"
#include "ui.hpp"

namespace frontend {
    VideoService::VideoService() : _running(false) {}

    bool VideoService::open(const char* url) {
        return _stream.open(url);
    }

    void VideoService::start(UI& ui) {
        _running = true;
        _thread = std::thread(_process, this, std::ref(ui));
    }

    void VideoService::join() {
        _running = false;
        _thread.join();
    }

    AVStream& VideoService::getStream() {
        return _stream;
    }

    void VideoService::updateSDLTexture(SDL_Texture* tex) const {
        std::lock_guard<std::mutex> guard(_frameMutex);
        auto frame = _frame.get();
        SDL_UpdateYUVTexture(tex, nullptr,
                frame->data[0], frame->linesize[0],
                frame->data[1], frame->linesize[1],
                frame->data[2], frame->linesize[2]);
    }


    void VideoService::_process(VideoService* self, UI& ui) {
        AVStream& stream = self->_stream;
        auto video = stream.video();
        auto frame = self->_frame.get();

        while (self->_running) {
            if (!stream.readPacket())
                break;

            {
                std::lock_guard<std::mutex> guard(self->_frameMutex);

                if (!stream.retrieveFrame(video, frame))
                    break;
            }

            // Notify the main loop to refresh
            ui.notifyTextureUpdate();
        }
    }
}
