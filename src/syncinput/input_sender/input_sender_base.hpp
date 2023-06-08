#ifndef INPUT_BASE_HPP
#define INPUT_BASE_HPP

#include <SDL2/SDL_keycode.h>

namespace input {
    class IInputSender
    {
        public:
            virtual ~IInputSender() = default;

            // Attach to window with the given title
            virtual bool attach(const char* title) = 0;

            // Send key event. Expects a platform specific keysym. see also convertSDLKeycode().
            virtual void sendKey(bool pressed, unsigned long key) const = 0;

            // Send a mouse button event
            virtual void sendMouse(bool pressed, unsigned int button) const = 0;

            // Send a mouse move event.
            virtual void sendMouseMove(int x, int y, bool relative) const = 0;

            // Send a mouse wheel event
            virtual void sendMouseWheel(int x, int y) const = 0;

            // Flush the command queue. Required on some platforms.
            virtual void flush() const = 0;

            // Convert an SDL keycode to a platform specific keysym.
            virtual unsigned long convertSDLKeycode(SDL_Keycode keycode) const = 0;
    };
}

#endif
