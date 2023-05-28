#ifndef XORG_HPP
#define XORG_HPP

#include <X11/Xlib.h>
#include "input_sender_base.hpp"

namespace input {
    Window findWindowByName(Display* display, Window root, const char* name);

    class InputSender : private IInputSender
    {
        public:
            InputSender();
            ~InputSender() override;

            bool attach(const char* title) final;
            void sendKey(bool pressed, unsigned long key) const override;
            void sendMouse(bool pressed, unsigned int button) const override;
            void sendMouseMove(int x, int y, bool relative) const override;
            void sendMouseWheel(int x, int y) const override;
            void flush() const final;

        private:
            Display* _display;
    };
}

#endif
