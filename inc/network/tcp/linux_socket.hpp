//
// Created by junjian LI on 2023/5/4.
//

#pragma once

#include <arpa/inet.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
namespace coplus {

    using socket_t = int;
    struct io_event_context {
        char* buffer;
        size_t size;
    };

    struct [[maybe_unused]] sys_tcp_socket_operation {
        [[gnu::always_inline]] static socket_t create() {
            socket_t tcp_stream = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            fcntl(tcp_stream, F_SETFL, fcntl(tcp_stream, F_GETFL) | O_NONBLOCK);
            return tcp_stream;
        }
        [[gnu::always_inline]] static int close(socket_t tcp_stream) {
            return ::close(tcp_stream);
        }

        template<class IP>
        [[gnu::always_inline]] static int bind(socket_t tcp_stream, const net_address<IP>& address) {
            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port = htons(address.port());
            addr.sin_addr.s_addr = htonl(address.ip().bin());
            return ::bind(tcp_stream, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
        }
        [[gnu::always_inline]] static int listen(socket_t tcp_stream) {
            return ::listen(tcp_stream, SOMAXCONN);
        }
        [[gnu::always_inline]] static std::tuple<socket_t, sockaddr, socklen_t> accept(socket_t tcp_stream) {
            sockaddr addr{};
            socklen_t len = sizeof(addr);
            socket_t new_socket = ::accept(tcp_stream, &addr, &len);
            return {new_socket, addr, len};
        }

        template<class IP>
        [[gnu::always_inline]] static socket_t connect(socket_t tcp_stream, const net_address<IP>& address) {
            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port = htons(address.port());
            addr.sin_addr.s_addr = address.ip().bin();
            return ::connect(tcp_stream, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
        }

        [[gnu::always_inline]] static int read_to_end(socket_t tcp_stream, io_event_context& context) {
            int read_result = read(tcp_stream, context);
            if (read_result == -1 && errno == EAGAIN)
                return 0;
            if (read_result > 0) {
                int indexer = read_result;
                while (indexer < context.size) {
                    read_result = read(tcp_stream, context);
                    if (read_result > 0) {
                        indexer += read_result;
                    }
                    if (read_result == 0)
                        return indexer;
                    if (read_result == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                            return indexer;
                        else
                            return -1;
                    }
                }
                return indexer;
            }
            return read_result;
        }

        [[gnu::always_inline]] static int read(socket_t tcp_stream, io_event_context& context) {
            return ::read(tcp_stream, context.buffer, context.size);
        }

        [[gnu::always_inline]] static int write(socket_t tcp_stream, io_event_context& context) {
            return ::write(tcp_stream, context.buffer, context.size);
        }

        [[gnu::always_inline]] static int set_socket_option(socket_t tcp_stream, int level, int option_name, const void* option_value, socklen_t option_len) {
            return ::setsockopt(tcp_stream, level, option_name, option_value, option_len);
        }

        static void default_socket_option(socket_t tcp_stream) {
            int opt = 1;
            set_socket_option(tcp_stream, SOL_SOCKET, 0, (void*) (&opt), sizeof(opt));
        }
    };
    class sys_socket{
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
            auto [ raw_handle, addr, _ ] = sys_tcp_socket_operation::accept(handle_);
            return {sys_socket(raw_handle), net_address<IP>::from_raw((sockaddr_in*) &addr)};
        }
        template<class IP>
        void connect(const net_address<IP>& address) {
            sys_tcp_socket_operation::connect(handle_, address);
        }
        int read(io_event_context& context) const {
            return sys_tcp_socket_operation::read(handle_, context);
        }
        int write(io_event_context& context) const {
            int write_result = sys_tcp_socket_operation::write(handle_, context);
            if (write_result < 0)
                throw std::runtime_error(std::strerror(errno));
            return write_result;
        }
        void set_socket_option(int level, int option_name, const void* option_value, socklen_t option_len) const {
            sys_tcp_socket_operation::set_socket_option(handle_, level, option_name, option_value, option_len);
        }
        int read_to_end(io_event_context& context) const {
            int read_result = sys_tcp_socket_operation::read_to_end(handle_,context);
            if (read_result < 0)
                throw std::runtime_error(std::string("read error:")+std::strerror(errno));
            return read_result;
        }
    };
}