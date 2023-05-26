#ifndef TCP_HPP
#define TCP_HPP

#ifdef __linux__
typedef int SOCKET;
constexpr int SOCKET_ERROR = -1;
constexpr int INVALID_SOCKET = -1;
#elif defined(_WIN32)
#   define NOMINMAX  // Disables min/max macro
#   include <winsock2.h>
#   include <ws2tcpip.h>
#else
#   error "OS not supported!"
#endif


class Socket
{
    public:
        Socket();
        Socket(bool block);
        virtual ~Socket();

        SOCKET handle() const;
        void close();
        void setNagleAlgorithm(bool active) const;

    protected:
        SOCKET _socket;

#ifdef _WIN32
    private:
        // Auto-initializes Winsock
        static class WinsockInitializer
        {
            public:
                WinsockInitializer();
                ~WinsockInitializer();
        } _winsockInitializer;
#endif
};


class TCPSocket : public Socket
{
    friend class TCPListener;

    public:
        bool connect(const char* host, const char* port);
        int send(const char* buffer, unsigned int bufsize) const;
        int recv(char* buffer, unsigned int bufsize) const;
};

#endif
