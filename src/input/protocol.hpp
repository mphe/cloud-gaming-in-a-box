#ifndef INPUT_PROTOCOL_HPP
#define INPUT_PROTOCOL_HPP

#include <cstdint>
#include "socket/tcp.hpp"

namespace input {
    // NOTE: We use int32 for everything to have complete control over alignment and endianness.

    struct MouseMotion {
        int32_t x;
        int32_t y;
    };

    struct MouseWheel {
        int32_t x;
        int32_t y;
    };

    struct MouseButton {
        uint32_t button;
        uint32_t pressed;
    };

    struct Key {
        uint32_t key;
        uint32_t pressed;
    };

    enum InputEventType : uint32_t {
        EventMouseMotion,
        EventMouseWheel,
        EventMouseButton,
        EventKey
    };

    struct InputEvent {
        uint32_t type;
        union {
            MouseMotion motion;
            MouseWheel wheel;
            MouseButton button;
            Key key;
        };
    };

    class InputTransmitter
    {
        public:
          InputTransmitter(const TCPSocket& socket);

          void sendMouseButton(uint8_t button, bool pressed) const;
          void sendMouseMotion(int32_t x, int32_t y) const;
          void sendMouseWheel(int32_t x, int32_t y) const;
          void sendKey(uint32_t key, bool pressed) const;
          bool recv(InputEvent* event) const;

        private:
          void _send(const InputEvent& event) const;

        private:
            const TCPSocket& socket;
    };
}

#endif
