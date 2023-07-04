#include "ui.hpp"
#include <iostream>
#include <chrono>
#include <unistd.h>
#include <syncstream>
#include "VideoService.hpp"

using std::cerr;

namespace frontend {
    UI::UI(const input::InputTransmitter& transmitter, VideoService& video, bool vsync) :
        _mouseSensitivity(1.0), _window(nullptr), _renderer(nullptr), _frame(nullptr),
        _transmitter(transmitter), _video(video), _running(false), _vsync(vsync)
    {}

    UI::~UI() {
        if (_renderer)
            SDL_DestroyRenderer(_renderer);
        if (_window)
            SDL_DestroyWindow(_window);
        if (_frame)
            SDL_DestroyTexture(_frame);
        SDL_Quit();
    }

    bool UI::init() {
        if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
            cerr << "Failed to initialize SDL\n";
            return false;
        }

        int width = _video.getStream().video()->width;
        int height = _video.getStream().video()->height;
        Uint32 flags = SDL_WINDOW_MOUSE_CAPTURE | SDL_WINDOW_FULLSCREEN_DESKTOP;
        _window = SDL_CreateWindow("Frontend", 0, 0, width, height, flags);

        if (!_window) {
            cerr << "Failed to create window\n";
            return false;
        }

        SDL_SetRelativeMouseMode(SDL_TRUE);

        _renderer = SDL_CreateRenderer(_window, -1, SDL_RENDERER_ACCELERATED | (_vsync ? SDL_RENDERER_PRESENTVSYNC : 0));

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

#if VSYNC_METHOD == VSYNC_METHOD_ON_FRAME
        _frameCond.notify_all();
#endif
    }

    void UI::run() {
        _running = true;

        if (_vsync)
            _runThreaded();
        else
            _runInteractive();
    }

    void UI::_runInteractive() {
        SDL_Event event;

        while (_running && SDL_WaitEvent(&event))
            if (event.type == SDL_USEREVENT)
                _fetchAndRender();
            else
                _processEvent(event);
    }

    void UI::_runSequential() {
        SDL_Event event;

        while (_running) {
            while (SDL_PollEvent(&event))
                _processEvent(event);

            _fetchAndRender();
        }
    }

    void UI::_renderThread(SDL_GLContext gl, UI& ui) {
        SDL_GL_MakeCurrent(ui._window, gl);

#if VSYNC_METHOD == VSYNC_METHOD_ON_FRAME
        const auto maxWaitTime = std::chrono::milliseconds(500);

        // Try adaptive Vsync and fallback to regular vsync
        // if (SDL_GL_SetSwapInterval(-1) == -1)
        //    SDL_GL_SetSwapInterval(1);

        std::unique_lock lk(ui._frameMu);
        while (ui._running) {
            ui._frameCond.wait_for(lk, maxWaitTime);
            ui._fetchAndRender();
        }
        lk.unlock();
#elif VSYNC_METHOD == VSYNC_METHOD_PREDICT
        using std::chrono::high_resolution_clock;
        using std::chrono::duration_cast;
        using std::chrono::microseconds;

        float avgUs = 0;
        size_t num = 0;
        constexpr long render_time_us = 2000;
        constexpr size_t max_probes = 240;

        // Measure average vsync interval
        while (ui._running && num < max_probes) {
            auto begin = high_resolution_clock::now();

            ui._fetchAndRender();

            auto end = high_resolution_clock::now();
            auto deltaUs = duration_cast<microseconds>(end - begin).count();
            num++;
            avgUs += (deltaUs - avgUs) / num;
        }

        std::osyncstream(std::cout) << "Average Vsync interval: " << avgUs << "us\n";
        const unsigned int waitUs = avgUs - render_time_us;

        // Try adaptive Vsync and fallback to regular vsync
        // if (SDL_GL_SetSwapInterval(-1) == -1)
        //     SDL_GL_SetSwapInterval(1);

        while (ui._running) {
            usleep(waitUs);
            ui._fetchAndRender();
        }
#elif VSYNC_METHOD == VSYNC_METHOD_NAIVE
        while (ui._running) {
            ui._fetchAndRender();
        }
#endif
    }

    void UI::_runThreaded() {
        SDL_Event event;
        SDL_GLContext gl = SDL_GL_GetCurrentContext();
        SDL_GL_MakeCurrent(_window, nullptr);

        std::thread renderThread(_renderThread, gl, std::ref(*this));

        while (_running && SDL_WaitEvent(&event))
            _processEvent(event);

        renderThread.join();
    }

    void UI::_fetchAndRender() {
        _video.updateSDLTexture(_frame);
        SDL_RenderCopy(_renderer, _frame, nullptr, nullptr);
        SDL_RenderPresent(_renderer);
    }

    void UI::_processEvent(const SDL_Event& event) {
        switch (event.type) {
            case SDL_KEYDOWN:
                if (!event.key.repeat)
                    _transmitter.sendKey(event.key.keysym, true);
                break;

            case SDL_KEYUP:
                _transmitter.sendKey(event.key.keysym, false);
                break;

            case SDL_MOUSEBUTTONDOWN:
                _transmitter.sendMouseButton(event.button.button, true);
                break;

            case SDL_MOUSEBUTTONUP:
                _transmitter.sendMouseButton(event.button.button, false);
                break;

            case SDL_MOUSEMOTION:
                _transmitter.sendMouseMotion(
                        std::round(event.motion.xrel * _mouseSensitivity),
                        std::round(event.motion.yrel * _mouseSensitivity));
                break;

            case SDL_MOUSEWHEEL:
                _transmitter.sendMouseWheel(event.wheel.x, event.wheel.y);
                break;

            case SDL_QUIT:
                _running = false;
                break;

            default:
                break;
        }
    }

    void UI::setMouseSensitivity(float sens) {
        _mouseSensitivity = sens;
    }
} // namespace frontend
