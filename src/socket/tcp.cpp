#include "tcp.hpp"
#include <cstring>
#include <utility>

#ifdef __linux__
#	include <sys/socket.h>
#	include <netinet/tcp.h>
#	include <unistd.h>
#	include <netdb.h>
#	define closesocket(socket) close(socket)
#endif

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

Socket::~Socket()
{
    close();
}


SOCKET Socket::handle() const
{
    return _socket;
}


void Socket::setNagleAlgorithm(bool active) const
{
    const char val = !active;  // Invert so that nagle on <=> TCP_NODELAY off
    setsockopt(_socket, IPPROTO_TCP, TCP_NODELAY, &val, sizeof(val));
}


void Socket::close()
{
    if (!isValid())
        return;
    ::closesocket(_socket);
    _socket = INVALID_SOCKET;
}

bool Socket::isValid() const
{
    return _socket != INVALID_SOCKET;
}

#ifdef _WIN32
Socket::WinsockInitializer::WinsockInitializer()
{
    WSADATA data;
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0)
        throw "Winsock not initialized";
}

Socket::WinsockInitializer::~WinsockInitializer()
{
    WSACleanup();
}


Socket::WinsockInitializer Socket::_winsockInitializer;
#endif


TCPSocket::TCPSocket(SOCKET socket) : Socket(socket) {}

bool TCPSocket::connect(const char* host, const char* port)
{
    close();

    addrinfo hint, *ptr = nullptr, *ai = nullptr;
    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_INET;
    hint.ai_protocol = IPPROTO_TCP;
    hint.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, port, &hint, &ai) == 0)
    {
        for (ptr = ai; ptr != nullptr; ptr = ptr->ai_next)
        {
            _socket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

            if (!isValid())
                continue;

            if (::connect(_socket, ptr->ai_addr, ptr->ai_addrlen) == 0)
                break;  // Success

            close();
        }
    }

    if (ai != nullptr)
        freeaddrinfo(ai);

    return isValid();
}

int TCPSocket::listen(const char* host, const char* port)
{
    close();

    addrinfo hint, *ptr = nullptr, *ai = nullptr;
    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_INET;
    hint.ai_protocol = IPPROTO_TCP;
    hint.ai_socktype = SOCK_STREAM;
    hint.ai_flags = AI_PASSIVE;

    if (getaddrinfo(host, port, &hint, &ai) == 0)
    {
        for (ptr = ai; ptr != nullptr; ptr = ptr->ai_next)
        {
            _socket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

            if (!isValid())
                continue;

            if (bind(_socket, ai->ai_addr, ai->ai_addrlen) == 0 && ::listen(_socket, SOMAXCONN) == 0)
                break;  // Success

            close();
        }
    }

    if (ai != nullptr)
        freeaddrinfo(ai);

    return _socket != INVALID_SOCKET;
}


int TCPSocket::send(const char* buffer, unsigned int bufsize) const
{
    return ::send(_socket, buffer, bufsize, 0);
}


int TCPSocket::recv(char* buffer, unsigned int bufsize, int flags) const
{
    return ::recv(_socket, buffer, bufsize, flags);
}

TCPSocket TCPSocket::accept() const
{
    return TCPSocket(::accept(_socket, nullptr, nullptr));
}
