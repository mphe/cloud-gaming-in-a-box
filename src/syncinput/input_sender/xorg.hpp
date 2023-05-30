#ifndef XORG_HPP
#define XORG_HPP

#include <X11/Xlib.h>
#include "input_sender_base.hpp"

namespace input {
    Window findWindowByName(Display* display, Window root, const char* name);

    class InputSender final : private IInputSender
    {
        public:
            InputSender();
            ~InputSender() final;

            bool attach(const char* title) final;
            void sendKey(bool pressed, unsigned long key) const final;
            void sendMouse(bool pressed, unsigned int button) const final;
            void sendMouseMove(int x, int y, bool relative) const final;
            void sendMouseWheel(int x, int y) const final;
            void flush() const final;

        private:
            Display* _display;
    };
}

#endif
