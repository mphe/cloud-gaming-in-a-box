#include <iostream>
#include "ui.hpp"
#include "frontend/VideoService.hpp"
#include "frontend/AudioService.hpp"

using std::cout;
using std::cerr;


void help() {
    cout << "Usage: frontend <video filename/URL> <audio filename/URL> <syncinput IP> <syncinput port> <tcp|udp> [mouse-sensitivity] [vsync]\n";
    cout << "Live-streams the given video and audio streams while transmitting inputs to the given syncinput server.";
}


int main(int argc, char *argv[]) {
    if (argc < 6) {
        help();
        cerr << "Missing arguments\n";
        return 1;
    }

    const char* videoURL = argv[1];
    const char* audioURL = argv[2];
    const char* syncinputIP = argv[3];
    const char* syncinputPort = argv[4];
    float mouseSensitivity = 1.0;
    net::SocketType protocol = net::parseProtocol(argv[5]);
    bool useVsync = false;

    if (argc > 6) {
        mouseSensitivity = std::atof(argv[6]);

        if (argc > 7 && strcmp(argv[7], "vsync") == 0) {
            cout << "VSync enabled\n";
            useVsync = true;
        }
    }

    input::InputTransmitter inputTransmitter;
    if (!inputTransmitter.connect(syncinputIP, syncinputPort, protocol))
        return 1;

    frontend::VideoService video;
    if (!video.open(videoURL))
        return 1;

    // Initialize SDL before opening audio device.
    frontend::UI ui(inputTransmitter, video, useVsync);
    if (!ui.init())
        return 1;

    ui.setMouseSensitivity(mouseSensitivity);

    frontend::AudioService audio;
    if (!audio.open(audioURL))
        return 1;

    cout << "Starting video and audio service\n";
    video.start(ui);
    audio.start();

    cout << "Starting main loop\n";
    ui.run();

    cout << "Exiting\n";
    video.join();
    audio.join();

    return 0;
}
