#include <SDL2/SDL.h>
#include "network/input.hpp"

namespace frontend {
    class UI {
        public:
            UI();
            ~UI();

            bool init(int width, int height, bool vsync);
            bool handleInput(const input::InputTransmitter& transmitter);
            void renderFrame() const;
            void present() const;
            SDL_Texture *getFrameTexture();

          private:
            SDL_Window* _window;
            SDL_Renderer* _renderer;
            SDL_Texture* _frame;
    };
}
