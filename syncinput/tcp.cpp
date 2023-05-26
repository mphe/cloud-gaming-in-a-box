#include "tcp.hpp"
#include <cstring>

#ifdef __linux__
#	include <sys/socket.h>
#	include <netinet/tcp.h>
#	include <unistd.h>
#	include <netdb.h>
#	define closesocket(socket) close(socket)
#endif

Socket::Socket() : _socket(INVALID_SOCKET) { }

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
    if (_socket == INVALID_SOCKET)
        return;
    ::closesocket(_socket);
    _socket = INVALID_SOCKET;
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


bool TCPSocket::connect(const char* host, const char* port)
{
    close();

    addrinfo hint, *ptr = NULL, *ai = NULL;
    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_INET;
    hint.ai_protocol = IPPROTO_TCP;
    hint.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host, port, &hint, &ai) == 0)
    {
        for (ptr = ai; ptr != NULL; ptr = ptr->ai_next)
        {
            _socket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);

            if (_socket == INVALID_SOCKET)
                continue;

            if (::connect(_socket, ptr->ai_addr, ptr->ai_addrlen) == 0)
                break;  // Success
            else
                close();
        }
    }

    if (ai != NULL)
        freeaddrinfo(ai);

    return _socket != INVALID_SOCKET;
}


int TCPSocket::send(const char* buffer, unsigned int bufsize) const
{
    return ::send(_socket, buffer, bufsize, 0);
}


int TCPSocket::recv(char* buffer, unsigned int bufsize) const
{
    return ::recv(_socket, buffer, bufsize, 0);
}
