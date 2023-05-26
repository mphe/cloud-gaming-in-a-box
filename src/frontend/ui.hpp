#include <SDL2/SDL.h>

namespace frontend {
    class UI {
        public:
            UI();
            ~UI();

            bool init(int width, int height, bool vsync);
            bool handleInput();
            void renderFrame() const;
            void present() const;
            SDL_Texture *getFrameTexture();

          private:
            SDL_Window* _window;
            SDL_Renderer* _renderer;
            SDL_Texture* _frame;
    };
}
