#include <SDL2/SDL.h>
#include <condition_variable>
#include <mutex>
#include "network/input.hpp"

// Wait for the next frame and render it immediately. Uses adaptive vsync when available.
// Feels much smoother than VSYNC_METHOD_PREDICT but slightly less responsive.
#define VSYNC_METHOD_ON_FRAME 0

// Measure average vsync interval, then predict when next vsync occurs and render just before it happens.
// This method feels slightly more responsive than VSYNC_METHOD_ON_FRAME but also a lot more choppy.
#define VSYNC_METHOD_PREDICT 1

// Naive - Render and wait for vblank.
// Feels smooth but much less responsive than other methods.
#define VSYNC_METHOD_NAIVE 2

// Vsync method to use. See constants above.
#define VSYNC_METHOD VSYNC_METHOD_ON_FRAME


namespace frontend {
    class VideoService;

    class UI {
        public:
            UI(const input::InputTransmitter& transmitter, VideoService& video, bool vsync);
            ~UI();

            bool init();
            void run();
            // (Thread-safe) Notify the UI that a new frame is ready to be displayed
            void notifyTextureUpdate();

        private:
            // Run like a regular game loop: fetch inputs -> process -> render (wait for vsync).
            // High latency, no tearing.
            void _runSequential();

            // Wait for events and render instantly when a new frame arrives.
            // Low latency but heavy tearing.
            void _runInteractive();

            // Run rendering in a separate thread to allow vsync and immediate input processing.
            // See VSYNC_METHOD above on what rendering technique to use.
            // Medium latency, no tearing.
            void _runThreaded();

            void _processEvent(const SDL_Event& ev);
            void _fetchAndRender();
            static void _renderThread(SDL_GLContext gl, UI& ui);

          private:
            SDL_Window* _window;
            SDL_Renderer* _renderer;
            SDL_Texture* _frame;
            const input::InputTransmitter& _transmitter;
            VideoService& _video;
            SDL_Event _userEvent;

#if VSYNC_METHOD == VSYNC_METHOD_ON_FRAME
            std::mutex _frameMu;
            std::condition_variable _frameCond;
#endif

            bool _running;
            bool _vsync;
    };
}
