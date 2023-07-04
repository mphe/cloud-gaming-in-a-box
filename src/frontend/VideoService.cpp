#include "VideoService.hpp"
#include "ui.hpp"
#include <iostream>

namespace frontend {
    VideoService::VideoService() : _avgFrametimeUs(0.0), _running(false) {}

    bool VideoService::open(const char* url) {
        _stream.format()->max_analyze_duration = INT64_MAX - 1;
        _stream.format()->probesize = INT64_MAX - 1;
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
        using std::chrono::high_resolution_clock;
        using std::chrono::duration_cast;
        using std::chrono::microseconds;

        AVStream& stream = self->_stream;
        auto video = stream.video();
        auto frame = self->_frame.get();

        size_t nFrames = 0;

        while (self->_running) {
            auto begin = high_resolution_clock::now();

            if (!stream.readPacket())
                break;

            {
                std::lock_guard<std::mutex> guard(self->_frameMutex);

                if (!stream.retrieveFrame(video, frame))
                    break;
            }

            auto end = high_resolution_clock::now();
            auto deltaUs = duration_cast<microseconds>(end - begin).count();
            nFrames++;
            self->_avgFrametimeUs += (deltaUs - self->_avgFrametimeUs) / nFrames;

            // Notify the main loop to refresh
            ui.notifyTextureUpdate();
        }
    }

    float VideoService::getAvgFrametime() const {
        return _avgFrametimeUs;
    }
} // namespace frontend
