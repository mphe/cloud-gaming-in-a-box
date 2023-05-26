#ifndef XORG_HPP
#define XORG_HPP

#include <X11/Xlib.h>
#include "input_sender_base.hpp"

Window findWindowByName(Display* display, Window root, const char* name);

// Does not rely on the XTest interface but generates XEvents directly.
// NOTE: sendMouseMove() does not work as expected. Mouse can be moved, but it might not register in
// the target application, e.g. a game.
// Use InputSenderXTest instead in those cases.
class InputSenderXEvent : private IInputSender
{
    public:
        InputSenderXEvent();
        ~InputSenderXEvent() override;

        bool attach(const char* title) final;
        void sendKey(bool pressed, unsigned long key) const override;
        void sendMouse(bool pressed, unsigned int button) const override;
        void sendMouseMove(int x, int y, bool relative) const override;
        void flush() const final;

    private:
        Display* _display;
        Window _window;
        Window _root;
};


class InputSenderXTest : private IInputSender
{
    public:
        InputSenderXTest();
        ~InputSenderXTest() override;

        bool attach(const char* title) final;
        void sendKey(bool pressed, unsigned long key) const override;
        void sendMouse(bool pressed, unsigned int button) const override;
        void sendMouseMove(int x, int y, bool relative) const override;
        void flush() const final;

    private:
        Display* _display;
};

#endif
