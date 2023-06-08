#include <iostream>
#include <unistd.h>  // sleep
#include "network/input.hpp"
#include "input_sender/input_sender.hpp"

using std::cout;
using std::cerr;
using std::endl;


void help() {
    cout << "Usage: syncinput <window title> <ip> <port> <tcp|udp>\n";
    cout << "Listens on the given IP and port, or localhost:9090 by default, for inputs and sends them to the window with the given title.\n";
}


bool attach(input::InputSender& input, const char* winTitle, int maxTries) {
    for (int i = 0; i < maxTries; ++i) {
        if (input.attach(winTitle))
            return true;
        cout << "Waiting for window with name '" << winTitle << "'...\n";
        sleep(1);
    }
    return false;
}


int main(int argc, char *argv[]) {
    if (argc < 5) {
        help();
        cerr << "Missing arguments\n";
        return 1;
    }

    const char* winTitle = argv[1];
    const char* host = argv[2];
    const char* port = argv[3];
    net::SocketType protocol = net::parseProtocol(argv[4]);

    input::InputTransmitter inputTransmitter;
    if (!inputTransmitter.listen(host, port, protocol)) {
        cerr << "Failed to establish connection\n";
        return 1;
    }

    input::InputSender inputSender;
    if (!attach(inputSender, winTitle, 5)) {
        cerr << "Failed to attach to window\n";
        return 1;
    }

    while (true) {
        input::InputEvent event;

        if (!inputTransmitter.recv(&event))
            break;

        switch (event.type) {
            case input::InputEventType::EventKey:
                // cout << "key received " << event.key.key << " " << event.key.pressed << endl;
                inputSender.sendKey(event.key.pressed, inputSender.convertSDLKeycode(event.key.key));
                break;
            case input::InputEventType::EventMouseButton:
                // cout << "mouse received " << static_cast<int>(event.button.button) << " " << event.button.pressed << endl;
                inputSender.sendMouse(event.button.pressed, event.button.button);
                break;
            case input::InputEventType::EventMouseMotion:
                // cout << "motion received " << event.motion.x << " " << event.motion.y << endl;
                inputSender.sendMouseMove(event.motion.x, event.motion.y, true);
                break;
            case input::InputEventType::EventMouseWheel:
                // cout << "wheel received " << event.wheel.x << " " << event.wheel.y << endl;
                inputSender.sendMouseWheel(event.wheel.x, event.wheel.y);
                break;
            default:
                cerr << "Invalid event type: " << static_cast<int>(event.type) << endl;
                break;
        }

        inputSender.flush();
    }

    return 0;
}
