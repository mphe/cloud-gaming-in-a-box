#include <iostream>
#include <unistd.h>  // sleep
#include "input/protocol.hpp"
#include "socket/tcp.hpp"
#include "input_sender/input_sender.hpp"

using std::cout;
using std::cerr;
using std::endl;


void help() {
    cout << "Usage: syncinput <window title> [ip] [port]\n";
    cout << "Listens on the given IP and port, or localhost:9090 by default, for inputs and sends them to the window with the given title.\n";
}


bool attach(input::InputSender& input, const char* winTitle, int maxTries) {
    for (int i = 0; i < maxTries; ++i)
    {
        if (input.attach(winTitle))
            return true;
        cout << "Waiting for window with name '" << winTitle << "'...\n";
        sleep(1);
    }
    return false;
}


TCPSocket establishConnection(const char* host, const char* port) {
    TCPSocket listener, client;
    listener.listen(host, port);
    cout << "Listening on " << host << ":" << port << "..." << endl;

    do {
        client = listener.accept();
        cout << "Client accepted\n";
    } while (!client.isValid());

    listener.close();
    client.setNagleAlgorithm(false);
    cout << "Connection established, closing listener\n";
    return client;
}


int main(int argc, char *argv[]) {
    const char* host = "127.0.0.1";
    const char* port = "9090";

    // Parse args
    if (argc < 2) {
        help();
        cerr << "Missing window title\n";
        return 1;
    }

    if (argc > 2)
        host = argv[2];

    if (argc > 3)
        port = argv[3];

    TCPSocket client = establishConnection(host, port);
    input::InputTransmitter inputTransmitter(client);

    input::InputSender inputSender;
    if (!attach(inputSender, argv[1], 5))
    {
        cerr << "Failed to attach to window\n";
        return 1;
    }

    while (client.isValid()) {
        input::InputEvent event;

        if (!inputTransmitter.recv(&event))
            break;

        switch (event.type) {
            case input::InputEventType::EventKey:
                // cout << "key received " << event.key.key << " " << event.key.pressed << endl;
                inputSender.sendKey(event.key.pressed, event.key.key);
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
        }

        inputSender.flush();
    }

    return 0;
}