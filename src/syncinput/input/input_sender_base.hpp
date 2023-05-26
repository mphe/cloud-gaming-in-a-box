#ifndef INPUT_BASE_HPP
#define INPUT_BASE_HPP

constexpr unsigned int mb_left = 1;
constexpr unsigned int mb_right = 3;

class IInputSender
{
    public:
        virtual ~IInputSender() = default;

        virtual bool attach(const char* title) = 0;
        virtual void sendKey(bool pressed, unsigned long key) const = 0;
        virtual void sendMouse(bool pressed, unsigned int button) const = 0;
        virtual void sendMouseMove(int x, int y, bool relative) const = 0;
        virtual void flush() const = 0;
};

#endif
