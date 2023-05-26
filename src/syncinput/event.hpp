#ifndef EVENT_HPP
#define EVENT_HPP

#include <string_view>
#include "input/input.hpp"

class EventProcessor
{
    public:
        EventProcessor(bool isDxGame);

        bool attach(const char* winTitle, int maxTries = 5);
        void processMessage(std::string_view message) const;

    private:
        void _processEvent(std::string_view ev) const;
        void _processMouse(std::string_view ev) const;
        void _processKey(std::string_view ev) const;

    private:
        bool _isDxGame;
        InputSender _input;
};

#endif
