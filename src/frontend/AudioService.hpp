#ifndef FRONTEND_AUDIOSERVICE_HPP
#define FRONTEND_AUDIOSERVICE_HPP

#include <SDL_audio.h>
#include <thread>
#include "av.hpp"

namespace frontend {
    class AudioService {
        public:
            AudioService();

            bool open(const char* url);
            void start();
            void join();

        private:
            static void _process(AudioService* self);

        private:
            SDL_AudioDeviceID _audioDev;
            Frame _frame;
            std::thread _thread;
            AVStream _stream;
            bool _running;
    };
}

#endif
