#include <iostream>
#include <string_view>
#include <string.h>
#include <unistd.h>
#include "socket/tcp.hpp"
#include "event.hpp"

using std::string;
using std::cout;
using std::cerr;
using std::endl;


int main(int argc, char *argv[])
{
    bool isDxGame = false;  // This is just here for future when linux and windows versions are merged
    const char* host = "127.0.0.1";
    const char* port = "9090";
    const char* winTitle = nullptr;

    // Parse args
    if (argc > 1)
        winTitle = argv[1];
    else
    {
        cerr << "Missing window title\n";
        return 1;
    }

    if (argc > 2 && strcmp(argv[2], "game") == 0)
        isDxGame = true;

    if (argc > 4)
        host = argv[4];

    // Attach to target window
    EventProcessor processor(isDxGame);

    if (!processor.attach(winTitle))
    {
        cerr << "Failed to attach to window\n";
        return 1;
    }

    // Init TCP connection
    cout << "Connecting...\n";
    TCPSocket socket;
    socket.setNagleAlgorithm(false);

    if (!socket.connect(host, port)) {
        cerr << "Failed to connect to " << host << ":" << port << endl;
        return 1;
    }

    cout << "Connected\n";

    // Main loop
    char buf[2048];

    while (true) {
        int nrecv = socket.recv(buf, sizeof(buf));

        if (nrecv == 0)
        {
            cerr << "Connection closed\n";
            break;
        }
        else if (nrecv == -1)
        {
            cerr << "An error occurred\n";
            break;
        }
        else if (nrecv == 1 && buf[0] == 0)  // Received ping?
        {
            cout << "ping\n";
            continue;
        }

        processor.processMessage(std::string_view(buf, nrecv));
    }

    socket.close();

    return 0;
}
