#include "socket.hpp"
#include <cstring>
#include <cctype>
#include <algorithm>
#include <string>
#include <utility>
#include <cassert>

#ifdef __linux__
#   include <sys/socket.h>
#   include <netinet/tcp.h>
#   include <unistd.h>
#   include <netdb.h>
#   define closesocket(socket) close(socket)
constexpr int INVALID_SOCKET = -1;
#endif

namespace net {
    // General purpose function for setting up TCP/UDP client/server sockets.
    SOCKET setupSocket(SocketType type, bool listen, const char* host, const char* port) {
        SOCKET socket = INVALID_SOCKET;
        addrinfo hint, *ptr = nullptr, *ai = nullptr;
        memset(&hint, 0, sizeof(hint));
        hint.ai_family = AF_INET;

        switch (type) {
            case TCP:
                hint.ai_protocol = IPPROTO_TCP;
                hint.ai_socktype = SOCK_STREAM;
                if (listen)
                    hint.ai_flags = AI_PASSIVE;
                break;

            case UDP:
                hint.ai_protocol = IPPROTO_UDP;
                hint.ai_socktype = SOCK_DGRAM;
                break;

            default:
                assert("Invalid socket type");
        }

        if (getaddrinfo(host, port, &hint, &ai) == 0) {
            for (ptr = ai; ptr != nullptr; ptr = ptr->ai_next) {
                socket = ::socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

                if (socket == INVALID_SOCKET)
                    continue;

                if (listen) {
                    if (::bind(socket, ai->ai_addr, ai->ai_addrlen) == 0)
                        if (type == UDP || ::listen(socket, SOMAXCONN) == 0)
                            break;  // Success
                }
                else if (::connect(socket, ptr->ai_addr, ptr->ai_addrlen) == 0)
                    break;  // Success

                ::closesocket(socket);
                socket = INVALID_SOCKET;
            }
        }

        if (ai != nullptr)
            freeaddrinfo(ai);

        return socket;
    }

    SocketType parseProtocol(std::string str) {
        // Convert to lower-case
        std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return std::tolower(c); });

        if (str == "tcp")
            return TCP;
        else if (str == "udp")
            return UDP;

        return UnsupportedProtocol;
    }

    Socket::Socket() : _socket(INVALID_SOCKET) {}

    Socket::Socket(SOCKET socket) : _socket(socket) {}

    Socket::Socket(Socket&& socket) : Socket() {
        *this = std::move(socket);
    }

    Socket& Socket::operator=(Socket&& other) {
        close();
        _socket = other._socket;
        other._socket = INVALID_SOCKET;
        return *this;
    }

    Socket::~Socket() {
        close();
    }

    SOCKET Socket::handle() const {
        return _socket;
    }

    void Socket::setNagleAlgorithm(bool active) const {
        const char val = !active;  // Invert so that nagle on <=> TCP_NODELAY off
        setsockopt(_socket, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val));
    }

    void Socket::close() {
        if (!isValid())
            return;
        ::closesocket(_socket);
        _socket = INVALID_SOCKET;
    }

    bool Socket::isValid() const {
        return _socket != INVALID_SOCKET;
    }

    bool Socket::connect(SocketType type, const char* host, const char* port) {
        close();
        _socket = setupSocket(type, false, host, port);
        return isValid();
    }

    bool Socket::listen(SocketType type, const char* host, const char* port) {
        close();
        _socket = setupSocket(type, true, host, port);
        return isValid();
    }

    int Socket::send(const char* buffer, unsigned int bufsize, int flags) const {
        return ::send(_socket, buffer, bufsize, flags);
    }

    int Socket::recv(char* buffer, unsigned int bufsize, int flags) const {
        return ::recv(_socket, buffer, bufsize, flags);
    }

    Socket Socket::accept() const {
        return Socket(::accept(_socket, nullptr, nullptr));
    }

#ifdef _WIN32
    Socket::WinsockInitializer::WinsockInitializer() {
        WSADATA data;
        if (WSAStartup(MAKEWORD(2, 2), &data) != 0)
            throw "Winsock not initialized";
    }

    Socket::WinsockInitializer::~WinsockInitializer() {
        WSACleanup();
    }


    Socket::WinsockInitializer Socket::_winsockInitializer;
#endif
}
