//
// Created by junjian LI on 2023/4/25.
//

#pragma once
#include "network/ip/ipv4.hpp"
#include <arpa/inet.h>
#include <cstdint>
#include <cerrno>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
namespace coplus {

#ifdef _WIN32
    using socket_t = SOCKET;
#elif __linux__
    using socket_t = int;
#elif __APPLE__
    using socket_t = int;
#define SYS_CALL(func, ...) [](auto&&... args) {int result__ = func(args...); if(result__<0) throw std::runtime_error(std::string("syscall `"#func"` failed:")+ strerror(errno));else return result__; }(__VA_ARGS__);
    struct [[maybe_unused]] sys_tcp_socket_options {
        [[gnu::always_inline]] static socket_t create() {
            socket_t socket = SYS_CALL(::socket, AF_INET, SOCK_STREAM, IPPROTO_TCP);
            //fcntl(socket, F_SETFL, fcntl(socket, F_GETFL) | O_NONBLOCK);
            return socket;
        }
        [[gnu::always_inline]] static int close(socket_t socket) {
            return SYS_CALL(::close, socket);
        }

        template<class IP>
        [[gnu::always_inline]] static void bind(socket_t socket, const net_address<IP>& address) {
            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port = htons(address.port());
            addr.sin_addr.s_addr = htonl(address.ip().bin());
            SYS_CALL(::bind, socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
        }
        [[gnu::always_inline]] static void listen(socket_t socket) {
            SYS_CALL(::listen, socket, SOMAXCONN);
        }
        [[gnu::always_inline]] static socket_t accept(socket_t socket) {
            return SYS_CALL(::accept, socket, nullptr, nullptr);
        }

        template<class IP>
        [[gnu::always_inline]] static socket_t connect(socket_t socket, const net_address<IP>& address) {
            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port = htons(address.port());
            addr.sin_addr.s_addr = address.ip().bin();
            //inet_pton(AF_INET, address.ip(), &addr.sin_addr);
            return SYS_CALL(::connect, socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
        }
        [[gnu::always_inline]] static size_t read(socket_t socket, char* buffer, size_t size) {
            return SYS_CALL(::read, socket, buffer, size);
        }
        [[gnu::always_inline]] static size_t write(socket_t socket, const char* buffer, size_t size) {
            return SYS_CALL(::write, socket, buffer, size);
        }

        [[gnu::always_inline]] static void set_socket_option(socket_t socket, int level, int option_name, const void* option_value, socklen_t option_len) {
            SYS_CALL(::setsockopt, socket, level, option_name, option_value, option_len);
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
#elif __APPLE__
        using socket_t = int64_t;
        socket_t handle_;
        void set_default_socket_option() const {
            sys_tcp_socket_options::default_socket_option(handle_);
        }

    public:
        explicit sys_socket(socket_t handle)
            : handle_(handle) {
        }
        [[nodiscard]] socket_t raw_fd() const {
            return handle_;
        }
        sys_socket()
            : handle_(sys_tcp_socket_options::create()) {
            set_default_socket_option();
        }
        sys_socket(sys_socket&& other)
            : handle_(other.handle_) {
            other.handle_ = -1;
        }
        sys_socket& operator=(sys_socket&& other) noexcept {
            handle_ = other.handle_;
            other.handle_ = -1;
            return *this;
        }
        ~sys_socket() {
            //if (handle_ != -1) sys_tcp_socket_options::close(handle_);
        }
        sys_socket(const sys_socket&) = delete;
        sys_socket& operator=(const sys_socket&) = delete;
        template<class IP>
        void bind(const net_address<IP>& address) const {
            sys_tcp_socket_options::bind(handle_, address);
        }
        void listen() const {
            sys_tcp_socket_options::listen(handle_);
        }
        [[nodiscard]] sys_socket accept() const {
            return sys_socket(sys_tcp_socket_options::accept(handle_));
        }
        template<class IP>
        void connect(const net_address<IP>& address) {
            sys_tcp_socket_options::connect(handle_, address);
        }
        size_t read(char* buffer, size_t size) const {
            return sys_tcp_socket_options::read(handle_, buffer, size);
        }
        size_t write(const char* buffer, size_t size) const {
            return sys_tcp_socket_options::write(handle_, buffer, size);
        }
        void set_socket_option(int level, int option_name, const void* option_value, socklen_t option_len) const {
            sys_tcp_socket_options::set_socket_option(handle_, level, option_name, option_value, option_len);
        }

#endif
    };
}// namespace coplus
