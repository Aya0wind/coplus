//
// Created by junjian LI on 2023/5/4.
//

#pragma once
#include "coplus/network/ip/ipv4.hpp"
#include <arpa/inet.h>
#include <cstring>
#include <stdexcept>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <tuple>
#include <unistd.h>
namespace coplus {
    using socket_t = int;
    struct io_event_context {
        char* buffer;
        size_t size;
    };

    struct [[maybe_unused]] sys_tcp_socket_operation {
        [[gnu::always_inline]] static socket_t create() {
            socket_t socket = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (socket == -1)
                throw std::runtime_error(std::strerror(errno));
            fcntl(socket, F_SETFL, fcntl(socket, F_GETFL) | O_NONBLOCK);
            return socket;
        }
        [[gnu::always_inline]] static int close(socket_t socket) {
            int close_result = ::close(socket);
            if (close_result == -1)
                throw std::runtime_error(std::strerror(errno));
            return close_result;
        }

        template<class IP>
        [[gnu::always_inline]] static int bind(socket_t socket, const net_address<IP>& address) {
            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port = htons(address.port());
            addr.sin_addr.s_addr = htonl(address.ip().bin());
            int bind_result = ::bind(socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
            if (bind_result == -1)
                throw std::runtime_error(std::strerror(errno));
            return bind_result;
        }
        [[gnu::always_inline]] static int listen(socket_t socket) {
            int listen_result = ::listen(socket, SOMAXCONN);
            if (listen_result == -1)
                throw std::runtime_error(std::strerror(errno));
            return listen_result;
        }
        [[gnu::always_inline]] static std::tuple<socket_t, sockaddr, socklen_t> accept(socket_t socket) {
            sockaddr addr{};
            socklen_t len = sizeof(addr);
            socket_t new_socket = ::accept(socket, &addr, &len);
            if (new_socket == -1)
                throw std::runtime_error(std::strerror(errno));
            return {new_socket, addr, len};
        }

        template<class IP>
        [[gnu::always_inline]] static socket_t connect(socket_t socket, const net_address<IP>& address) {
            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port = htons(address.port());
            addr.sin_addr.s_addr = address.ip().bin();
            int connect_result = ::connect(socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
            if (connect_result == -1 && errno != EINPROGRESS)
                throw std::runtime_error(std::strerror(errno));
            return connect_result;
        }

        [[gnu::always_inline]] static int read_to_end(socket_t socket, io_event_context& context) {
            int read_result = read(socket, context);
            if (read_result == -1 && errno == EAGAIN)
                return 0;
            if (read_result > 0) {
                int indexer = read_result;
                while (indexer < context.size) {
                    read_result = read(socket, context);
                    if (read_result > 0) {
                        indexer += read_result;
                    }
                    if (read_result == 0)
                        return indexer;
                    if (read_result == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                            return indexer;
                        else
                            throw std::runtime_error(std::strerror(errno));
                    }
                }
                return indexer;
            }
            return read_result;
        }

        [[gnu::always_inline]] static int read(socket_t socket, io_event_context& context) {
            int read_result = ::read(socket, context.buffer, context.size);
            if (read_result < 0)
                throw std::runtime_error(std::strerror(errno));
            return read_result;
        }

        [[gnu::always_inline]] static int write(socket_t socket, io_event_context& context) {
            int write_result = ::write(socket, context.buffer, context.size);
            if (write_result < 0)
                throw std::runtime_error(std::strerror(errno));
            return write_result;
        }

        [[gnu::always_inline]] static void set_socket_option(socket_t socket, int level, int option_name, const void* option_value, socklen_t option_len) {
            int set_socket_option_result = ::setsockopt(socket, level, option_name, option_value, option_len);
            if (set_socket_option_result == -1)
                throw std::runtime_error(std::strerror(errno));
        }

        static void default_socket_option(socket_t socket) {
            int opt = 1;
            fcntl(socket, F_SETFL, fcntl(socket, F_GETFL) | O_NONBLOCK);
            set_socket_option(socket, SOL_SOCKET, 0, (void*) (&opt), sizeof(opt));
        }
    };
}// namespace coplus
