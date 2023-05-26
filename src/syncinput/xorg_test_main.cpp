#include <iostream>
#include <X11/keysym.h>
#include <unistd.h>
#include "input/xorg.hpp"
#include <X11/extensions/XTest.h>

using namespace std;


int main(int argc, char *argv[])
{
    const char* searchTitle = nullptr;

    if (argc > 1)
        searchTitle = argv[1];
    else
    {
        cout << "Missing window title\n";
        return 1;
    }

    Display* display = XOpenDisplay(nullptr);

    while (true)
    {
        XTestFakeRelativeMotionEvent(display, 10, 10, CurrentTime);
        XFlush(display);
        usleep(500000);
        XTestFakeRelativeMotionEvent(display, -10, -10, CurrentTime);
        XFlush(display);
        usleep(500000);
    }

    return 0;

    InputSenderXEvent sender;

    if (sender.attach(searchTitle))
        cout << "Found: " << searchTitle << endl;
    else
    {
        cout << "Couldn't find window\n";
        return 1;
    }

    Window target = findWindowByName(display, DefaultRootWindow(display), searchTitle);
    XSetInputFocus(display, target, RevertToParent, CurrentTime);
    // XSetInputFocus(display, DefaultRootWindow(display), RevertToParent, CurrentTime);

    for (int i = 0; i < 10; ++i)
    {
        XWarpPointer(display, None, None, 0, 0, 0, 0, 100, 100);
        // XWarpPointer(display, DefaultRootWindow(display), None, 0, 0, 0, 0, 100, 100);
        XFlush(display);
        // sender.sendMouseMove(1000, 1000, True);
        // sender.flush();
        sleep(1);
        // usleep(100000);
    }

    return 0;

    sender.sendKey(true, XK_A);
    sender.flush();
    sleep(1);
    sender.sendKey(false, XK_A);
    sender.flush();

    sender.sendKey(true, XK_D);
    sender.flush();
    sleep(1);
    sender.sendKey(false, XK_D);
    sender.flush();

    for (int i = 0; i < 5; ++i)
    {
        sender.sendMouse(true, Button1);
        sender.flush();
        usleep(20000);
        sender.sendMouse(false, Button1);
        sender.flush();
        usleep(500000);
    }

    sender.sendMouse(true, Button2);
    sender.sendKey(true, XK_space);
    sender.sendKey(true, XK_W);
    sender.flush();
    usleep(20000);
    sender.sendMouse(false, Button2);
    sender.flush();
    usleep(2000000);
    sender.sendKey(false, XK_space);
    sender.sendKey(false, XK_W);
    sender.flush();

    XCloseDisplay(display);

    return 0;
}
