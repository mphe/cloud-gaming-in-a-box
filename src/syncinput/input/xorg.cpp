#include "xorg.hpp"
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#include <string.h>

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


InputSenderXEvent::InputSenderXEvent()
    : _window(BadWindow)
{
    _display = XOpenDisplay(nullptr);
    _root = DefaultRootWindow(_display);
}

InputSenderXEvent::~InputSenderXEvent()
{
    XCloseDisplay(_display);
}

bool InputSenderXEvent::attach(const char* title)
{
    _window = findWindowByName(_display, _root, title);
    return _window != BadWindow;
};

void InputSenderXEvent::sendKey(bool pressed, KeySym key) const
{
    auto keycode = XKeysymToKeycode(_display, key);
    if (keycode == NoSymbol)
        return;

    auto mask = pressed ? KeyPressMask : KeyReleaseMask;
    auto type = pressed ? KeyPress : KeyRelease;

    XEvent event;
    event.type = type;
    event.xkey = XKeyEvent {
        .type = type,
        .serial = 0,
        .send_event = True,
        .display = _display,
        .window = _window,
        .root = _root,
        .subwindow = None,
        .time = CurrentTime,
        .x = 1,
        .y = 1,
        .x_root = 1,
        .y_root = 1,
        .state = 0,
        .keycode = keycode,
        .same_screen = True
    };

    XSendEvent(_display, _window, False, mask, &event);
}

void InputSenderXEvent::sendMouse(bool pressed, unsigned int button) const
{
    auto mask = pressed ? ButtonPressMask : ButtonReleaseMask;
    auto type = pressed ? ButtonPress : ButtonRelease;

    XEvent event;
    event.type = type;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
    event.xbutton = XButtonEvent {
        .type = type,
        .serial = 0,
        .send_event = True,
        .display = _display,
        .window = _window,
        .time = CurrentTime,
        .button = button,
        .same_screen = True
    };
#pragma GCC diagnostic pop

    XQueryPointer(_display, _window, &event.xbutton.root, &event.xbutton.subwindow,
            &event.xbutton.x_root, &event.xbutton.y_root, &event.xbutton.x, &event.xbutton.y,
            &event.xbutton.state);

    XSendEvent(_display, _window, False, mask, &event);
}

void InputSenderXEvent::sendMouseMove(int x, int y, bool relative) const
{
    XEvent event;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
    event.type = MotionNotify;
    event.xmotion = XMotionEvent {
        .type = MotionNotify,
        .serial = 0,
        .send_event = True,
        .display = _display,
        .window = _window,
        .time = CurrentTime,
        // .x = x,
        // .y = y,
        // .x_root = x,
        // .y_root = y,
        // .state = 0,
        .is_hint = NotifyNormal,
        .same_screen = True
    };
#pragma GCC diagnostic pop

    Window root, child;
    XQueryPointer(_display, _window, &root, &child, &event.xmotion.x_root, &event.xmotion.y_root,
            &event.xmotion.x, &event.xmotion.y, &event.xmotion.state);

    // Convert to relative movement
    if (!relative)
    {
        x = x - event.xmotion.x;
        y = y - event.xmotion.y;
    }

    // Do relative movement
    event.xmotion.x += x;
    event.xmotion.y += y;
    event.xmotion.x_root += x;
    event.xmotion.y_root += y;

    event.xmotion.state |= PointerMotionMask;
    XSendEvent(_display, _window, False, event.xmotion.state, &event);
}

void InputSenderXEvent::flush() const
{
    XFlush(_display);
}



InputSenderXTest::InputSenderXTest()
{
    _display = XOpenDisplay(nullptr);
}

InputSenderXTest::~InputSenderXTest()
{
    XCloseDisplay(_display);
}

void InputSenderXTest::sendKey(bool pressed, KeySym key) const
{
    auto keycode = XKeysymToKeycode(_display, key);
    if (keycode == NoSymbol)
        return;
    XTestFakeKeyEvent(_display, keycode, pressed, CurrentTime);
}

void InputSenderXTest::sendMouse(bool pressed, unsigned int button) const
{
    XTestFakeButtonEvent(_display, button, pressed, CurrentTime);
}

void InputSenderXTest::sendMouseMove(int x, int y, bool relative) const
{
    if (relative)
        XTestFakeRelativeMotionEvent(_display, x, y, CurrentTime);
    else
        XTestFakeMotionEvent(_display, 0, x, y, CurrentTime);
}

void InputSenderXTest::flush() const
{
    XFlush(_display);
}

bool InputSenderXTest::attach([[maybe_unused]] const char* title)
{
    return true;
};
