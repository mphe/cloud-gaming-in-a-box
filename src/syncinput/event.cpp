#include "event.hpp"
#include <iostream>
#include <string>
#include <string_view>
#include <unistd.h>
#include <vector>
#include <charconv>

using std::cout;
using std::endl;

typedef unsigned char byte;

constexpr byte no_interaction = 0;
constexpr byte is_pressed = 1;
constexpr byte is_released = 2;


// Fast string split that doesn't require instancing a stringstream and string instances.
// Copied from https://old.reddit.com/r/Cplusplus/comments/gnc9rz/fast_string_split/
std::vector<std::string_view> split(std::string_view str, std::string_view delimeters)
{
    std::vector<std::string_view> res;
    res.reserve(str.length() / 2);

    const char* ptr = str.data();
    size_t size = 0;

    for (const char c : str)
    {
        for (const char d : delimeters)
        {
            if (c == d)
            {
                res.emplace_back(ptr, size);
                ptr += size + 1;
                size = 0;
                goto next;
            }
        }
        ++size;
next: continue;
    }

    if (size)
        res.emplace_back(ptr, size);
    return res;
}


EventProcessor::EventProcessor(bool isDxGame) :
    _isDxGame(isDxGame)
{}

bool EventProcessor::attach(const char* winTitle, int maxTries)
{
    for (int i = 0; i < maxTries; ++i)
    {
        if (_input.attach(winTitle))
            return true;
        cout << "Waiting for window with name '" << winTitle << "'...\n";
        sleep(1);
    }
    return false;
}

void EventProcessor::processMessage(std::string_view message) const
{
    auto lines = split(message, "|");
    cout << "Received: " << message << endl;
    for (auto line : lines)
        if (!line.empty())
            _processEvent(line);
    _input.flush();
    cout << "---------\n";
}

void EventProcessor::_processEvent(std::string_view ev) const
{
    cout << "Event: " << ev << endl;
    auto data = ev.substr(1);

    if (ev[0] == 'K')
        _processKey(data);
    else if (ev[0] == 'M')
        _processMouse(data);
}

void EventProcessor::_processKey(std::string_view ev) const
{
    byte key;
    byte state;

    auto lines = split(ev, ",");
    std::from_chars(lines[0].data(), lines[0].data() + lines[0].size(), key);
    std::from_chars(lines[1].data(), lines[1].data() + lines[1].size(), state);

    cout << "Key: " << int(key) << " state: " << int(state) << endl;

    _input.sendKey(state == is_pressed, key);
}

void EventProcessor::_processMouse(std::string_view ev) const
{
    byte isLeft;
    byte state;
    float x;
    float y;
    float relwidth;
    float relheight;

    auto lines = split(ev, ",");
    std::from_chars(lines[0].data(), lines[0].data() + lines[0].size(), isLeft);
    std::from_chars(lines[1].data(), lines[1].data() + lines[1].size(), state);
    std::from_chars(lines[2].data(), lines[2].data() + lines[2].size(), x);
    std::from_chars(lines[3].data(), lines[3].data() + lines[3].size(), y);
    std::from_chars(lines[4].data(), lines[4].data() + lines[4].size(), relwidth);
    std::from_chars(lines[5].data(), lines[5].data() + lines[5].size(), relheight);

    cout << "isLeft: " << bool(isLeft) << " state: " << int(state) << " x: " << x << " y: " << y << " relwidth: " << relwidth << " relheight: " << relheight << endl;

    if (state == no_interaction)  // Only move
        _input.sendMouseMove(x, y, true);
    else
        _input.sendMouse(state == is_pressed, isLeft ? mb_left : mb_right);
}
