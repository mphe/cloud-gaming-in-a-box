#include "ui.hpp"
#include <iostream>
#include "VideoService.hpp"

using std::cerr;

namespace frontend {
    UI::UI() : _window(nullptr), _renderer(nullptr), _frame(nullptr) {}

    UI::~UI() {
        if (_renderer)
            SDL_DestroyRenderer(_renderer);
        if (_window)
            SDL_DestroyWindow(_window);
        if (_frame)
            SDL_DestroyTexture(_frame);
        SDL_Quit();
    }

    bool UI::init(int width, int height, bool vsync) {
        if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
            cerr << "Failed to initialize SDL\n";
            return false;
        }

        Uint32 flags = SDL_WINDOW_MOUSE_CAPTURE | SDL_WINDOW_FULLSCREEN_DESKTOP;
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

        // Setup user event
        SDL_zero(_userEvent);
        // It should suffice to use a generic user event
        // _userEvent.type = SDL_RegisterEvents(1);
        _userEvent.type = SDL_USEREVENT;

        return true;
    }

    void UI::notifyTextureUpdate() {
        SDL_PushEvent(&_userEvent);
    }

    void UI::run(const input::InputTransmitter& transmitter, const VideoService& video) {
        SDL_Event event;
        bool running = true;

        while (running && SDL_WaitEvent(&event)) {
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
                    running = false;
                    break;

                case SDL_USEREVENT:
                    video.updateSDLTexture(_frame);
                    SDL_RenderCopy(_renderer, _frame, nullptr, nullptr);
                    SDL_RenderPresent(_renderer);
                    break;

                default:
                    break;
            }
        }
    }
} // namespace frontend
