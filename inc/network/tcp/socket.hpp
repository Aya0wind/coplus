//
// Created by junjian LI on 2023/4/25.
//

#pragma once
#include "network/ip/ipv4.hpp"

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <netinet/in.h>
#include <stdexcept>
#include <string>
#include <sys/_types/_socklen_t.h>
#include <tuple>

namespace coplus {

#ifdef _WIN32
    using socket_t = SOCKET;
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
        [[gnu::always_inline]] static std::tuple<socket_t ,sockaddr,socklen_t> accept(socket_t socket) {
            sockaddr addr{};
            socklen_t len = sizeof(addr);
            socket_t new_socket = ::accept(socket, &addr, &len);
            return {new_socket,addr,len};
        }

        template<class IP>
        [[gnu::always_inline]] static socket_t connect(socket_t socket, const net_address<IP>& address) {
            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port = htons(address.port());
            addr.sin_addr.s_addr = address.ip().bin();
            return ::connect(socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
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
        [[gnu::always_inline]] static std::tuple<socket_t ,sockaddr,socklen_t> accept(socket_t socket) {
            sockaddr addr{};
            socklen_t len = sizeof(addr);
            socket_t new_socket = ::accept(socket, &addr, &len);
            return {new_socket,addr,len};
        }

        template<class IP>
        [[gnu::always_inline]] static socket_t connect(socket_t socket, const net_address<IP>& address) {
            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port = htons(address.port());
            addr.sin_addr.s_addr = address.ip().bin();
            return ::connect(socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
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
#elif __linux__
        using socket_t = int;
        socket_t handle_;
        void set_default_socket_option() const {
            sys_tcp_socket_operation::default_socket_option(handle_);
        }

    public:
        explicit sys_socket(socket_t handle) :
            handle_(handle) {
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
        sys_socket(sys_socket&& other)  noexcept :
            handle_(other.handle_) {
            other.handle_ = -1;
        }
        sys_socket& operator=(sys_socket&& other) noexcept {
            handle_ = other.handle_;
            other.handle_ = -1;
            return *this;
        }
        ~sys_socket() {
            //            if (handle_ != -1)
            //                sys_tcp_socket_operation::close(handle_);
        }

        template<class IP>
        void bind(const net_address<IP>& address) const {
            sys_tcp_socket_operation::bind(handle_, address);
        }
        void listen() const {
            sys_tcp_socket_operation::listen(handle_);
        }
        template<class IP>
        [[nodiscard]] std::tuple<sys_socket,net_address<IP>> accept() const {
            auto [raw_fd,addr,_ ] = sys_tcp_socket_operation::accept(handle_);
            return  sys_socket(raw_fd,net_address<IP>::from_raw((sockaddr_in*)&addr));
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


#elif __APPLE__
        using socket_t = int;
        socket_t handle_;
        void set_default_socket_option() const {
            sys_tcp_socket_operation::default_socket_option(handle_);
        }

    public:
        explicit sys_socket(socket_t handle) :
            handle_(handle) {
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
        sys_socket(sys_socket&& other)  noexcept :
            handle_(other.handle_) {
            other.handle_ = -1;
        }
        sys_socket& operator=(sys_socket&& other) noexcept {
            handle_ = other.handle_;
            other.handle_ = -1;
            return *this;
        }
        ~sys_socket() {
//            if (handle_ != -1)
//                sys_tcp_socket_operation::close(handle_);
        }

        template<class IP>
        void bind(const net_address<IP>& address) const {
            sys_tcp_socket_operation::bind(handle_, address);
        }
        void listen() const {
            sys_tcp_socket_operation::listen(handle_);
        }
        template<class IP>
        [[nodiscard]] std::tuple<sys_socket,net_address<IP>> accept() const {
            auto [raw_fd,addr,_ ] = sys_tcp_socket_operation::accept(handle_);
            if(raw_fd == -1)
                throw std::runtime_error(strerror(errno));
            return {sys_socket(raw_fd),net_address<IP>::from_raw((sockaddr_in*)&addr)};
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

#endif
    };
}// namespace coplus
