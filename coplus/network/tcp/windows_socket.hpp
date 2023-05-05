//
// Created by junjian LI on 2023/5/4.
//

#pragma once
#include <WinSock2.h>
namespace coplus {
    using socket_t = SOCKET;
    struct io_event_context {
        char* buffer;
        size_t size;
    };
    struct [[maybe_unused]] sys_tcp_socket_operation {
        [[gnu::always_inline]] static socket_t create() {
            socket_t tcp_stream = ::WSASocketW(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
            return tcp_stream;
        }
        [[gnu::always_inline]] static int close(socket_t tcp_stream) {
            return ::closesocket(tcp_stream);
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

        [[gnu::always_inline]] static int read(IOContext* context) {
            return ::WSARecv(context->tcp_stream,
                             &context->wsaBuf,
                             context->wsaBuf.len,
                             &context->nBytes,
                             &context->flags,
                             &context->overlapped,
                             nullptr);
        }
        [[gnu::always_inline]] static int write(IOContext* context) {
            return ::WSARecv(context->tcp_stream,
                             &context->wsaBuf,
                             context->wsaBuf.len,
                             &context->nBytes,
                             &context->flags,
                             &context->overlapped,
                             nullptr);
        }

        [[gnu::always_inline]] static int set_socket_option(socket_t tcp_stream, int level, int option_name, const void* option_value, socklen_t option_len) {
            return ::setsockopt(tcp_stream, level, option_name, (const char*) option_value, option_len);
        }
        [[gnu::always_inline]] static void set_non_blocking(socket_t tcp_stream) {
            ULONG arg = 1;
            if (SOCKET_ERROR == ::ioctlsocket(tcp_stream, FIONBIO, &arg)) {
                shutdown(tcp_stream, SD_BOTH);
                closesocket(tcp_stream);
                exit(-2);
            }
        }
        static void default_socket_option(socket_t tcp_stream) {
            int opt = 1;
            ULONG ul = 1;
            set_non_blocking(tcp_stream);
            set_socket_option(tcp_stream, SOL_SOCKET, 0, (void*) (&opt), sizeof(opt));
        }
    };


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
    };
}
