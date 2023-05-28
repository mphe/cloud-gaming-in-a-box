#include "xorg.hpp"
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#include <cmath>
#include <string.h>
#include <SDL2/SDL_keycode.h>

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
        // These don't map directly to xlib button values, so we handle them manually.
        if (key == SDLK_RETURN)
            key = XK_Return;
        else if (key == SDLK_ESCAPE)
            key = XK_Escape;
        else if (key == SDLK_BACKSPACE)
            key = XK_BackSpace;

        auto keycode = XKeysymToKeycode(_display, key);

        if (keycode != NoSymbol)
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
}
