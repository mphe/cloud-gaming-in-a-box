#include "protocol.hpp"
#include <cstring>
#include <iostream>

// htonl
#ifdef __linux__
#	include <netinet/in.h>
#elif _WIN32
#	include <WinSock2.h>
#endif

using std::cout;
using std::cerr;
using std::endl;

namespace input {
    InputTransmitter::InputTransmitter(const net::Socket& socket) : socket(socket) {}

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

    void InputTransmitter::sendKey(uint32_t key, bool pressed) const {
        _send(InputEvent {
            .type = htonl(EventKey),
            .key = Key {
                .key = htonl(key),
                .pressed = htonl(pressed)
            }
        });
    }

    void InputTransmitter::_send(const InputEvent& event) const {
        if (socket.send(reinterpret_cast<const char *>(&event), sizeof(InputEvent)) == -1)
            cerr << "An error occurred during send: " << errno << " " << std::strerror(errno) << endl;

    }

    bool InputTransmitter::recv(InputEvent* event) const {
        int nrecv = socket.recv(reinterpret_cast<char*>(event), sizeof(InputEvent), MSG_WAITALL);

        if (nrecv == 0) {
            cout << "Connection closed\n";
            return false;
        }
        else if (nrecv == -1) {
            cerr << "An error occurred during recv: " << errno << " " << std::strerror(errno) << endl;
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
} // namespace input
