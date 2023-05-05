//
// Created by junjian LI on 2023/4/25.
//

#pragma once

#include "network/ip/ipv4.hpp"
#ifdef _WIN32
#include "windows_socket.hpp"
#elif __linux__
#include "linux_socket.hpp"
#elif __APPLE__
#include "macos_socket.hpp"
#endif
namespace coplus {
    class sys_socket {
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
        int read(io_event_context& context) const {
            return sys_tcp_socket_operation::read(handle_, context);
        }
        int write(io_event_context& context) const {
            return sys_tcp_socket_operation::write(handle_, context);
        }
        void set_socket_option(int level, int option_name, const void* option_value, socklen_t option_len) const {
            sys_tcp_socket_operation::set_socket_option(handle_, level, option_name, option_value, option_len);
        }
        int read_to_end(io_event_context& context) const {
            return sys_tcp_socket_operation::read_to_end(handle_, context);
        }
    };
}// namespace coplus
