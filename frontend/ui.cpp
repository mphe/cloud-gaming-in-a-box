#include "ui.hpp"
#include <iostream>

using std::cerr;

namespace frontend {
    UI::UI() : _window(nullptr), _renderer(nullptr), _frame(nullptr) {}

    UI::~UI() {
        if (_renderer)
            SDL_DestroyRenderer(_renderer);
        if (_window)
            SDL_DestroyWindow(_window);
        SDL_Quit();
    }

    bool UI::init(int width, int height, bool vsync) {
        if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
            cerr << "Failed to initialize SDL\n";
            return false;
        }

        Uint32 flags = vsync ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;
        _window = SDL_CreateWindow("Frontend", 0, 0, width, height, flags);

        if (!_window) {
            cerr << "Failed to create window\n";
            return false;
        }

        _renderer = SDL_CreateRenderer(_window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

        if (!_renderer) {
            cerr << "Failed to create renderer\n";
            return false;
        }

        _frame = SDL_CreateTexture(_renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, width, height);

        if (!_frame) {
            cerr << "Failed to create frame texture\n";
            return false;
        }

        SDL_SetRenderDrawColor(_renderer, 0, 0, 0, 255);
        SDL_RenderClear(_renderer);
        SDL_RenderCopy(_renderer, _frame, nullptr, nullptr);

        return true;
    }

    bool UI::handleInput() {
        SDL_Event event;
        bool stop = false;

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_ESCAPE)
                        stop = true;
                    break;

                case SDL_QUIT:
                    stop = true;
                    break;

                default:
                    break;
            }
        }

        return !stop;
    }

    void UI::renderFrame() const {
        SDL_RenderCopy(_renderer, _frame, nullptr, nullptr);
    }

    void UI::present() const {
        SDL_RenderPresent(_renderer);
    }

    SDL_Texture *UI::getFrameTexture() {
        return _frame;
    }
} // namespace frontend
