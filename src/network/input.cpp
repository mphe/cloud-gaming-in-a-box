#include "input.hpp"
#include <cstring>
#include <iostream>
#include <unistd.h>

// htonl
#ifdef __linux__
#	include <netinet/in.h>
#elif _WIN32
#	include <WinSock2.h>
#endif

using std::cout;
using std::cerr;
using std::endl;

void cerrWithErrno(const char* text) {
    cerr << text << errno << " " << std::strerror(errno) << endl;
}

namespace input {
    void InputTransmitter::sendMouseButton(uint8_t button, bool pressed) const {
        _send(InputEvent {
            .type = htonl(EventMouseButton),
            .button = MouseButton {
                .button = htonl(button),
                .pressed = htonl(pressed)
            }
        });
    }

    void InputTransmitter::sendMouseMotion(int32_t x, int32_t y) const {
        _send(InputEvent {
            .type = htonl(EventMouseMotion),
            .motion = MouseMotion {
                .x = static_cast<int32_t>(htonl(x)),
                .y = static_cast<int32_t>(htonl(y))
            }
        });
    }

    void InputTransmitter::sendMouseWheel(int32_t x, int32_t y) const {
        _send(InputEvent {
            .type = htonl(EventMouseWheel),
            .wheel = MouseWheel {
                .x = static_cast<int32_t>(htonl(x)),
                .y = static_cast<int32_t>(htonl(y))
            }
        });
    }

    void InputTransmitter::sendKey(const SDL_Keysym& key, bool pressed) const {
        // Apparently this key does not produce a meaningful keysym, so we handle it manually.
        auto sym = (key.scancode == SDL_SCANCODE_GRAVE) ? SDLK_BACKQUOTE : key.sym;

        _send(InputEvent {
            .type = htonl(EventKey),
            .key = Key {
                .key = static_cast<int32_t>(htonl(sym)),
                .pressed = htonl(pressed)
            }
        });
    }

    void InputTransmitter::_send(const InputEvent& event) const {
        if (_socket.send(reinterpret_cast<const char *>(&event), sizeof(InputEvent)) == -1)
            cerrWithErrno("An error occurred during send: ");
    }

    bool InputTransmitter::recv(InputEvent* event) const {
        int nrecv = _socket.recv(reinterpret_cast<char*>(event), sizeof(InputEvent), MSG_WAITALL);

        if (nrecv == 0) {
            cout << "Connection closed\n";
            return false;
        }
        else if (nrecv == -1) {
            cerrWithErrno("An error occurred during recv: ");
            return false;
        }

        event->type = ntohl(event->type);

        switch (event->type) {
            case InputEventType::EventMouseMotion:
                event->motion.x = ntohl(event->motion.x);
                event->motion.y = ntohl(event->motion.y);
                break;

            case InputEventType::EventMouseButton:
                event->button.button = ntohl(event->button.button);
                event->button.pressed = ntohl(event->button.pressed);
                break;

            case InputEventType::EventMouseWheel:
                event->wheel.x = ntohl(event->wheel.x);
                event->wheel.y = ntohl(event->wheel.y);
                break;

            case InputEventType::EventKey:
                event->key.key = ntohl(event->key.key);
                event->key.pressed = ntohl(event->key.pressed);
                break;
        }

        return true;
    }

    bool InputTransmitter::connect(const char* host, const char* port, net::SocketType type, int maxTries) {
        cout << "Connecting to " << host << ":" << port << "..." << endl;

        for (int i = 0; i < maxTries; ++i) {
            if (_socket.connect(type, host, port)) {
                _socket.setNagleAlgorithm(false);
                cout << "Connection established to syncinput\n";
                return true;
            }

            cerr << "Failed to connect. Retrying in 1s.\n";
            sleep(1);
        }

        cerr << "Failed to establish connection\n";
        return false;
    }

    bool InputTransmitter::listen(const char* host, const char* port, net::SocketType type) {
        cout << "Starting listener on " << host << ":" << port << "..." << endl;

        net::Socket listener;
        if (!listener.listen(type, host, port)) {
            cerrWithErrno("Failed to start listener: ");
            return false;
        }

        if (type == net::UDP) {
            _socket = std::move(listener);
            return true;
        }

        // TCP
        do {
            cout << "Waiting for clients...\n";
            _socket = listener.accept();
            cout << "Client accepted\n";
        } while (!_socket.isValid());

        cout << "Connection established to frontend, closing listener\n";
        listener.close();
        _socket.setNagleAlgorithm(false);
        return true;
    }
} // namespace input
