#include <SDL2/SDL.h>
#include "network/input.hpp"

namespace frontend {
    class VideoService;

    class UI {
        public:
            UI();
            ~UI();

            bool init(int width, int height, bool vsync);
            void run(const input::InputTransmitter& transmitter, const VideoService& video);
            // (Thread-safe) Notify the UI that a new frame is ready to be displayed
            void notifyTextureUpdate();

          private:
            SDL_Window* _window;
            SDL_Renderer* _renderer;
            SDL_Texture* _frame;
            SDL_Event _userEvent;
    };
}
