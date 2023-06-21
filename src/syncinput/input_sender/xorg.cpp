#include "xorg.hpp"
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#include <cmath>
#include <string.h>
#include <iostream>

namespace input {
    Window findWindowByName(Display* display, Window root, const char* name)
    {
        unsigned int nchildren;
        Window* children;
        Window root_return, parent_return;
        Window found = BadWindow;

        XQueryTree(display, root, &root_return, &parent_return, &children, &nchildren);

        for (size_t i = 0; i < nchildren; ++i)
        {
            Window win = children[i];
            char* title = nullptr;
            XFetchName(display, win, &title);

            if (title != nullptr)
            {
                if (strcmp(name, title) == 0)
                {
                    found = win;
                    break;
                }
                XFree(title);
            }

            found = findWindowByName(display, win, name);
            if (found != BadWindow)
                break;
        }

        if (children)
            XFree(children);

        return found;
    }


    InputSender::InputSender()
    {
        _display = XOpenDisplay(nullptr);
    }

    InputSender::~InputSender()
    {
        XCloseDisplay(_display);
    }

    void InputSender::sendKey(bool pressed, KeySym key) const
    {
        auto keycode = XKeysymToKeycode(_display, key);

        if (keycode == NoSymbol)
            std::cerr << "Unknown key: " << key << std::endl;
        else
            XTestFakeKeyEvent(_display, keycode, pressed, CurrentTime);
    }

    void InputSender::sendMouse(bool pressed, unsigned int button) const
    {
        XTestFakeButtonEvent(_display, button, pressed, CurrentTime);
    }

    void InputSender::sendMouseMove(int x, int y, bool relative) const
    {
        if (relative)
            XTestFakeRelativeMotionEvent(_display, x, y, CurrentTime);
        else
            XTestFakeMotionEvent(_display, 0, x, y, CurrentTime);
    }

    void InputSender::sendMouseWheel([[maybe_unused]] int x, int y) const
    {
        for (int i = 0; i < std::abs(y); ++i)
        {
            if (y < 0)
            {
                sendMouse(true, Button5);
                sendMouse(false, Button5);
            }
            else if (y > 0)
            {
                sendMouse(true, Button4);
                sendMouse(false, Button4);
            }
        }
    }

    void InputSender::flush() const
    {
        XFlush(_display);
    }

    bool InputSender::attach([[maybe_unused]] const char* title)
    {
        return true;
    };

    unsigned long InputSender::convertSDLKeycode(SDL_Keycode keycode) const
    {
        switch (keycode)
        {
            case SDLK_TAB:
                return XK_Tab;

            case SDLK_BACKQUOTE:
                return XK_dead_circumflex;

            case SDLK_UP:
                return XK_Up;

            case SDLK_DOWN:
                return XK_Down;

            case SDLK_LEFT:
                return XK_Left;

            case SDLK_RIGHT:
                return XK_Right;

            case SDLK_F1:
                return XK_F1;

            case SDLK_F2:
                return XK_F2;

            case SDLK_F3:
                return XK_F3;

            case SDLK_F4:
                return XK_F4;

            case SDLK_F5:
                return XK_F5;

            case SDLK_F6:
                return XK_F6;

            case SDLK_F7:
                return XK_F7;

            case SDLK_F8:
                return XK_F8;

            case SDLK_F9:
                return XK_F9;

            case SDLK_F10:
                return XK_F10;

            case SDLK_F11:
                return XK_F11;

            case SDLK_F12:
                return XK_F12;

            case SDLK_DELETE:
                return XK_Delete;

            case SDLK_LCTRL:
                return XK_Control_L;

            case SDLK_RCTRL:
                return XK_Control_R;

            case SDLK_CAPSLOCK:
                return XK_Caps_Lock;

            case SDLK_LSHIFT:
                return XK_Shift_L;

            case SDLK_RSHIFT:
                return XK_Shift_R;

            case SDLK_LALT:
                return XK_Alt_L;

            case SDLK_RALT:
                return XK_ISO_Level3_Shift;

            case SDLK_RETURN:
                return XK_Return;

            case SDLK_ESCAPE:
                return XK_Escape;

            case SDLK_BACKSPACE:
                return XK_BackSpace;

            default:
                return keycode;
        }
    }
}
