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

        Uint32 flags = SDL_WINDOW_MOUSE_CAPTURE | SDL_WINDOW_FULLSCREEN_DESKTOP;
        // Uint32 flags = SDL_WINDOW_MOUSE_CAPTURE;
        _window = SDL_CreateWindow("Frontend", 0, 0, width, height, flags);

        if (!_window) {
            cerr << "Failed to create window\n";
            return false;
        }

        SDL_SetRelativeMouseMode(SDL_TRUE);

        _renderer = SDL_CreateRenderer(_window, -1, SDL_RENDERER_ACCELERATED | (vsync ? SDL_RENDERER_PRESENTVSYNC : 0));

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

    bool UI::handleInput(const input::InputTransmitter& transmitter) {
        SDL_Event event;
        bool stop = false;

        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_KEYDOWN:
                    if (!event.key.repeat)
                        transmitter.sendKey(event.key.keysym.sym, true);
                    break;

                case SDL_KEYUP:
                    transmitter.sendKey(event.key.keysym.sym, false);
                    break;

                case SDL_MOUSEBUTTONDOWN:
                    transmitter.sendMouseButton(event.button.button, true);
                    break;

                case SDL_MOUSEBUTTONUP:
                    transmitter.sendMouseButton(event.button.button, false);
                    break;

                case SDL_MOUSEMOTION:
                    transmitter.sendMouseMotion(event.motion.xrel, event.motion.yrel);
                    break;

                case SDL_MOUSEWHEEL:
                    transmitter.sendMouseWheel(event.wheel.x, event.wheel.y);
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
