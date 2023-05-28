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
        Socket(SOCKET socket);
        Socket(Socket&& socket);
        Socket& operator=(Socket&& socket);
        virtual ~Socket();

        // Delete copy constructor and copy assignment because destructor closes socket.
        Socket(const Socket&) = delete;
        Socket& operator=(const Socket&) = delete;

        SOCKET handle() const;
        void close();
        void setNagleAlgorithm(bool active) const;
        bool isValid() const;

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
    public:
        TCPSocket() = default;
        TCPSocket(SOCKET socket);

        bool connect(const char* host, const char* port);
        int listen(const char* host, const char* port);
        int send(const char* buffer, unsigned int bufsize) const;
        int recv(char* buffer, unsigned int bufsize, int flags = 0) const;
        TCPSocket accept() const;
};

#endif
