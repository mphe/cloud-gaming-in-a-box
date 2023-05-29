#ifndef TCP_HPP
#define TCP_HPP

#ifdef __linux__
typedef int SOCKET;
#elif defined(_WIN32)
#   include <winsock2.h>
#   include <ws2tcpip.h>
#else
#   error "OS not supported!"
#endif

namespace net {
    enum SocketType {
        UDP,
        TCP
    };

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

            bool connect(SocketType type, const char* host, const char* port);
            bool listen(SocketType type, const char* host, const char* port);
            int send(const char* buffer, unsigned int bufsize, int flags = 0) const;
            int recv(char* buffer, unsigned int bufsize, int flags = 0) const;
            Socket accept() const;

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
}
#endif
