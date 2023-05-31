#ifndef FRONTEND_VIDEOSERVICE_HPP
#define FRONTEND_VIDEOSERVICE_HPP

#include <SDL_render.h>
#include <thread>
#include "av.hpp"

namespace frontend {
    class UI;

    class VideoService {
        public:
            VideoService();

            bool open(const char* url);
            void start(UI& ui);
            void join();
            AVStream& getStream();

            // (Thread-safe) Update SDL texture with the contents of the current video frame.
            void updateSDLTexture(SDL_Texture* tex) const;

        private:
            static void _process(VideoService* self, UI& ui);

        private:
            Frame _frame;
            std::thread _thread;
            mutable std::mutex _frameMutex;
            AVStream _stream;
            bool _running;
    };
}

#endif
