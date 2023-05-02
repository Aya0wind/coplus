//
// Created by junjian LI on 2023/4/25.
//

#pragma once
#include "network/ip/ipv4.hpp"
#include "poll/event.hpp"
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <tuple>
namespace coplus {

#ifdef _WIN32
#include <WinSock2.h>
    using socket_t = SOCKET;

    struct [[maybe_unused]] sys_tcp_socket_operation {
        [[gnu::always_inline]] static socket_t create() {
            socket_t socket = ::WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
            return socket;
        }
        [[gnu::always_inline]] static int close(socket_t socket) {
            return ::closesocket(socket);
        }

        template<class IP>
        [[gnu::always_inline]] static int bind(socket_t socket, const net_address<IP>& address) {
            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port = htons(address.port());
            addr.sin_addr.s_addr = htonl(address.ip().bin());
            return ::bind(socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
        }
        [[gnu::always_inline]] static int listen(socket_t socket) {
            return ::listen(socket, SOMAXCONN);
        }
        [[gnu::always_inline]] static std::tuple<socket_t, sockaddr, socklen_t> accept(socket_t socket) {
            sockaddr addr{};
            socklen_t len = sizeof(addr);
            socket_t new_socket = ::accept(socket, &addr, &len);
            return {new_socket, addr, len};
        }

        template<class IP>
        [[gnu::always_inline]] static socket_t connect(socket_t socket, const net_address<IP>& address) {
            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port = htons(address.port());
            addr.sin_addr.s_addr = address.ip().bin();
            return ::connect(socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
        }

        [[gnu::always_inline]] static int read(IOContext* context) {
            return ::WSARecv(context->socket,
                             &context->wsaBuf,
                             context->wsaBuf.len,
                             &context->nBytes,
                             &context->flags,
                             &context->overlapped,
                             nullptr
                             );
        }
        [[gnu::always_inline]] static int write(IOContext* context) {
            return ::WSARecv(context->socket,
                             &context->wsaBuf,
                             context->wsaBuf.len,
                             &context->nBytes,
                             &context->flags,
                             &context->overlapped,
                             nullptr
                             );
        }

        [[gnu::always_inline]] static int set_socket_option(socket_t socket, int level, int option_name, const void* option_value, socklen_t option_len) {
            return ::setsockopt(socket, level, option_name, (const char*)option_value, option_len);
        }
        [[gnu::always_inline]] static void set_non_blocking(socket_t socket) {
            ULONG arg = 1;
            if (SOCKET_ERROR == ::ioctlsocket(socket, FIONBIO, &arg)) {
                shutdown(socket, SD_BOTH);
                closesocket(socket);
                exit(-2);
            }
        }
        static void default_socket_option(socket_t socket) {
            int opt = 1;
            ULONG ul = 1;
            set_non_blocking(socket);
            set_socket_option(socket, SOL_SOCKET, 0, (void*) (&opt), sizeof(opt));
        }
    };
#elif __linux__
#include <arpa/inet.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
    using socket_t = int;
    struct [[maybe_unused]] sys_tcp_socket_operation {
        [[gnu::always_inline]] static socket_t create() {
            socket_t socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            fcntl(socket, F_SETFL, fcntl(socket, F_GETFL) | O_NONBLOCK);
            return socket;
        }
        [[gnu::always_inline]] static int close(socket_t socket) {
            return ::close(socket);
        }

        template<class IP>
        [[gnu::always_inline]] static int bind(socket_t socket, const net_address<IP>& address) {
            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port = htons(address.port());
            addr.sin_addr.s_addr = htonl(address.ip().bin());
            return ::bind(socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
        }
        [[gnu::always_inline]] static int listen(socket_t socket) {
            return ::listen(socket, SOMAXCONN);
        }
        [[gnu::always_inline]] static std::tuple<socket_t, sockaddr, socklen_t> accept(socket_t socket) {
            sockaddr addr{};
            socklen_t len = sizeof(addr);
            socket_t new_socket = ::accept(socket, &addr, &len);
            return {new_socket, addr, len};
        }

        template<class IP>
        [[gnu::always_inline]] static socket_t connect(socket_t socket, const net_address<IP>& address) {
            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port = htons(address.port());
            addr.sin_addr.s_addr = address.ip().bin();
            return ::connect(socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
        }

        [[gnu::always_inline]] static int read_to_end(socket_t socket, char* buffer, size_t size) {
            int read_result = read(socket, buffer, size);
            if(read_result==-1&&errno==EAGAIN)
                return 0;
            if (read_result > 0) {
                int indexer = read_result;
                while (indexer < size) {
                    read_result = read(socket, buffer + indexer, size - indexer);
                    if (read_result > 0) {
                        indexer += read_result;
                    }
                    if(read_result==0)
                        return indexer;
                    if(read_result==-1){
                        if(errno==EAGAIN||errno==EWOULDBLOCK)
                            return indexer;
                        else
                            return -1;
                    }
                }
                return indexer;
            }
            return read_result;
        }

        [[gnu::always_inline]] static int read(socket_t socket, char* buffer, size_t size) {
            return ::read(socket, buffer, size);
        }
        [[gnu::always_inline]] static int write(socket_t socket, const char* buffer, size_t size) {
            return ::write(socket, buffer, size);
        }

        [[gnu::always_inline]] static int set_socket_option(socket_t socket, int level, int option_name, const void* option_value, socklen_t option_len) {
            return ::setsockopt(socket, level, option_name, option_value, option_len);
        }

        static void default_socket_option(socket_t socket) {
            int opt = 1;
            set_socket_option(socket, SOL_SOCKET, 0, (void*) (&opt), sizeof(opt));
        }
    };
#elif __APPLE__
#include <arpa/inet.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
    using socket_t = int;
    struct [[maybe_unused]] sys_tcp_socket_operation {
        [[gnu::always_inline]] static socket_t create() {
            socket_t socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            fcntl(socket, F_SETFL, fcntl(socket, F_GETFL) | O_NONBLOCK);
            return socket;
        }
        [[gnu::always_inline]] static int close(socket_t socket) {
            return ::close(socket);
        }

        template<class IP>
        [[gnu::always_inline]] static int bind(socket_t socket, const net_address<IP>& address) {
            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port = htons(address.port());
            addr.sin_addr.s_addr = htonl(address.ip().bin());
            return ::bind(socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
        }
        [[gnu::always_inline]] static int listen(socket_t socket) {
            return ::listen(socket, SOMAXCONN);
        }
        [[gnu::always_inline]] static std::tuple<socket_t, sockaddr, socklen_t> accept(socket_t socket) {
            sockaddr addr{};
            socklen_t len = sizeof(addr);
            socket_t new_socket = ::accept(socket, &addr, &len);
            return {new_socket, addr, len};
        }

        template<class IP>
        [[gnu::always_inline]] static socket_t connect(socket_t socket, const net_address<IP>& address) {
            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port = htons(address.port());
            addr.sin_addr.s_addr = address.ip().bin();
            return ::connect(socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
        }

        [[gnu::always_inline]] static int read_to_end(socket_t socket, char* buffer, size_t size) {
            int read_result = read(socket, buffer, size);
            if(read_result==-1&&errno==EAGAIN)
                return 0;
            if (read_result > 0) {
                int indexer = read_result;
                while (indexer < size) {
                    read_result = read(socket, buffer + indexer, size - indexer);
                    if (read_result > 0) {
                        indexer += read_result;
                    }
                    if(read_result==0)
                        return indexer;
                    if(read_result==-1){
                        if(errno==EAGAIN||errno==EWOULDBLOCK)
                            return indexer;
                        else
                            return -1;
                    }
                }
                return indexer;
            }
            return read_result;
        }

        [[gnu::always_inline]] static int read(socket_t socket, char* buffer, size_t size) {
            return ::read(socket, buffer, size);
        }

        [[gnu::always_inline]] static int write(socket_t socket, const char* buffer, size_t size) {
            return ::write(socket, buffer, size);
        }

        [[gnu::always_inline]] static int set_socket_option(socket_t socket, int level, int option_name, const void* option_value, socklen_t option_len) {
            return ::setsockopt(socket, level, option_name, option_value, option_len);
        }

        static void default_socket_option(socket_t socket) {
            int opt = 1;
            set_socket_option(socket, SOL_SOCKET, SO_NOSIGPIPE, (void*) (&opt), sizeof(opt));
        }
    };
#endif

    class sys_socket {
#ifdef _WIN32
        using socket_t = SOCKET;
        socket_t handle_;
        void set_default_socket_option() const {
            sys_tcp_socket_operation::default_socket_option(handle_);
        }

    public:
        explicit sys_socket(socket_t handle) :
            handle_(handle) {
            set_default_socket_option();
        }
        sys_socket(const sys_socket&) = delete;
        sys_socket& operator=(const sys_socket&) = delete;
        [[nodiscard]] socket_t raw_handle() const {
            return handle_;
        }
        sys_socket() :
            handle_(sys_tcp_socket_operation::create()) {
            set_default_socket_option();
        }
        sys_socket(sys_socket&& other) noexcept :
            handle_(other.handle_) {
            other.handle_ = -1;
        }
        sys_socket& operator=(sys_socket&& other) noexcept {
            handle_ = other.handle_;
            other.handle_ = -1;
            return *this;
        }
        ~sys_socket() {
            if (handle_ != -1)
                sys_tcp_socket_operation::close(handle_);
        }

        template<class IP>
        void bind(const net_address<IP>& address) const {
            sys_tcp_socket_operation::bind(handle_, address);
        }
        void listen() const {
            sys_tcp_socket_operation::listen(handle_);
        }
        template<class IP>
        [[nodiscard]] std::tuple<sys_socket, net_address<IP>> accept() const {
            auto [ raw_fd, addr, _ ] = sys_tcp_socket_operation::accept(handle_);
            return {sys_socket(raw_fd), net_address<IP>::from_raw((sockaddr_in*) &addr)};
        }
        template<class IP>
        void connect(const net_address<IP>& address) {
            sys_tcp_socket_operation::connect(handle_, address);
        }
        int read(char* buffer, size_t size) const {
            return sys_tcp_socket_operation::read(handle_, buffer, size);
        }
        int write(const char* buffer, size_t size) const {
            return sys_tcp_socket_operation::write(handle_, buffer, size);
        }
        void set_socket_option(int level, int option_name, const void* option_value, socklen_t option_len) const {
            sys_tcp_socket_operation::set_socket_option(handle_, level, option_name, option_value, option_len);
        }

#elif __linux__
        using socket_t = int;
        socket_t handle_;
        void set_default_socket_option() const {
            fcntl(handle_, F_SETFL, fcntl(handle_, F_GETFL) | O_NONBLOCK);
            sys_tcp_socket_operation::default_socket_option(handle_);
        }

    public:
        explicit sys_socket(socket_t handle) :
            handle_(handle) {
            set_default_socket_option();
        }
        sys_socket(const sys_socket&) = delete;
        sys_socket& operator=(const sys_socket&) = delete;
        [[nodiscard]] socket_t raw_fd() const {
            return handle_;
        }
        sys_socket() :
            handle_(sys_tcp_socket_operation::create()) {
            set_default_socket_option();
        }
        sys_socket(sys_socket&& other) noexcept :
            handle_(other.handle_) {
            other.handle_ = -1;
        }
        sys_socket& operator=(sys_socket&& other) noexcept {
            handle_ = other.handle_;
            other.handle_ = -1;
            return *this;
        }
        ~sys_socket() {
            if (handle_ != -1)
                sys_tcp_socket_operation::close(handle_);
        }

        template<class IP>
        void bind(const net_address<IP>& address) const {
            sys_tcp_socket_operation::bind(handle_, address);
        }
        void listen() const {
            sys_tcp_socket_operation::listen(handle_);
        }
        template<class IP>
        [[nodiscard]] std::tuple<sys_socket, net_address<IP>> accept() const {
            auto [ raw_fd, addr, _ ] = sys_tcp_socket_operation::accept(handle_);
            return {sys_socket(raw_fd), net_address<IP>::from_raw((sockaddr_in*) &addr)};
        }
        template<class IP>
        void connect(const net_address<IP>& address) {
            sys_tcp_socket_operation::connect(handle_, address);
        }
        int read(char* buffer, size_t size) const {
            return sys_tcp_socket_operation::read(handle_, buffer, size);
        }
        int write(const char* buffer, size_t size) const {
            return sys_tcp_socket_operation::write(handle_, buffer, size);
        }
        void set_socket_option(int level, int option_name, const void* option_value, socklen_t option_len) const {
            sys_tcp_socket_operation::set_socket_option(handle_, level, option_name, option_value, option_len);
        }

        int read_to_end(char* buffer, size_t size) const {
            return sys_tcp_socket_operation::read_to_end(this->handle_, buffer, size);
        }


#elif __APPLE__
        using socket_t = int;
        socket_t handle_;
        void set_default_socket_option() const {
            fcntl(handle_, F_SETFL, fcntl(handle_, F_GETFL) | O_NONBLOCK);
            sys_tcp_socket_operation::default_socket_option(handle_);
        }

    public:
        explicit sys_socket(socket_t handle) :
            handle_(handle) {
            set_default_socket_option();
        }
        sys_socket(const sys_socket&) = delete;
        sys_socket& operator=(const sys_socket&) = delete;
        [[nodiscard]] socket_t raw_fd() const {
            return handle_;
        }
        sys_socket() :
            handle_(sys_tcp_socket_operation::create()) {
            set_default_socket_option();
        }
        sys_socket(sys_socket&& other) noexcept :
            handle_(other.handle_) {
            other.handle_ = -1;
        }
        sys_socket& operator=(sys_socket&& other) noexcept {
            handle_ = other.handle_;
            other.handle_ = -1;
            return *this;
        }
        ~sys_socket() {
            if (handle_ != -1)
                sys_tcp_socket_operation::close(handle_);
        }

        template<class IP>
        void bind(const net_address<IP>& address) const {
            sys_tcp_socket_operation::bind(handle_, address);
        }
        void listen() const {
            sys_tcp_socket_operation::listen(handle_);
        }
        template<class IP>
        [[nodiscard]] std::tuple<sys_socket, net_address<IP>> accept() const {
            auto [ raw_fd, addr, _ ] = sys_tcp_socket_operation::accept(handle_);
            if (raw_fd == -1)
                throw std::runtime_error(strerror(errno));
            return {sys_socket(raw_fd), net_address<IP>::from_raw((sockaddr_in*) &addr)};
        }
        template<class IP>
        void connect(const net_address<IP>& address) {
            sys_tcp_socket_operation::connect(handle_, address);
        }
        int read(char* buffer, size_t size) const {
            return sys_tcp_socket_operation::read(handle_, buffer, size);
        }
        int write(const char* buffer, size_t size) const {
            int write_result = sys_tcp_socket_operation::write(handle_, buffer, size);
            if (write_result < 0)
                throw std::runtime_error(std::strerror(errno));
            return write_result;
        }
        void set_socket_option(int level, int option_name, const void* option_value, socklen_t option_len) const {
            sys_tcp_socket_operation::set_socket_option(handle_, level, option_name, option_value, option_len);
        }
        int read_to_end(char* buffer, size_t size) const {
            int read_result = sys_tcp_socket_operation::read_to_end(handle_, buffer, size);
            if (read_result < 0)
                throw std::runtime_error(std::string("read error:")+std::strerror(errno));
            return read_result;
        }
#endif
    };
}// namespace coplus
