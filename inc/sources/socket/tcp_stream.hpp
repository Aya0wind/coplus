//
// Created by junjian LI on 2023/4/18.
//

#pragma once
#include "context/runtime.hpp"
#include "context/worker_thread_context.hpp"
#include "coroutine/default_awaiter.hpp"
#include "coroutine/task.hpp"
#include "network/ip/ipv4.hpp"
#include "network/tcp/socket.hpp"
#include "poll/event.hpp"
#include "poll/poller.hpp"
#include "poll/traits.hpp"
#include <cerrno>
#include <cstddef>
#include <cstring>
#include <memory>
#include <stdexcept>

namespace coplus {
    template<class IP>
    class socket_read_awaiter : public detail::source_base<selector, socket_read_awaiter<IP>> {
        const sys_socket& _socket;
        char* buffer;
        size_t size;
        friend class detail::source_base<selector, socket_read_awaiter>;
        void register_event_impl(selector& selector, intptr_t task_id) const {
            selector.register_event(_socket.raw_fd(), Interest::READABLE, 0, (void*) task_id);
        }

        void deregister_event_impl(selector& selector) const {
            selector.register_event(_socket.raw_fd(), Interest::READABLE, 0, nullptr);
        }

    public:
        socket_read_awaiter(char* buffer, size_t size, const sys_socket& socket) :
            buffer(buffer), size(size), _socket(socket){};
        socket_read_awaiter(const socket_read_awaiter&) = delete;

        [[nodiscard]] detail::handle_type get_handle() const {
            return _socket.raw_fd();
        }

        bool await_ready() {
            int read_result = _socket.read_to_end(buffer, size);
            return read_result >= 0;
        }

        void await_suspend(co_handle<void> handle) {
            auto& poller = current_worker_context.get_poller();
            poller.register_event(*this, _socket.raw_fd());
            current_worker_context.add_suspend_task(_socket.raw_fd(), handle);
        }

        auto await_resume() {
            auto& poller = current_worker_context.get_poller();
            poller.deregister_event(*this);
            return _socket.read_to_end(buffer, size);
        }
    };
    template<class IP>
    class socket_write_awaiter : public detail::source_base<selector, socket_write_awaiter<IP>> {
        const sys_socket& _socket;
        const char* buffer;
        size_t size;
        size_t current_index = 0;

    public:
        socket_write_awaiter(const char* buffer, size_t size, const sys_socket& socket) :
            buffer(buffer), size(size), _socket(socket){};

        void register_event_impl(selector& selector, intptr_t task_id) const {
            selector.register_event(_socket.raw_fd(), Interest::WRITEABLE, 0, (void*) task_id);
        }

        void deregister_event_impl(selector& selector) const {
            selector.register_event(_socket.raw_fd(), Interest::WRITEABLE, 0, nullptr);
        }

        bool await_ready() {
            int write_result = _socket.write(buffer, size);
            current_index = write_result;
            return write_result == size;
        }
        [[nodiscard]] detail::handle_type get_handle() const {
            return _socket.raw_fd();
        }
        void await_suspend(auto handle) {
            current_worker_context
                    .get_poller()
                    .register_event(*this, _socket.raw_fd());
            current_worker_context.add_suspend_task(_socket.raw_fd(), handle);
        }

        auto await_resume() {
            auto& poller = current_worker_context.get_poller();
            poller.deregister_event(*this);
            int write_result = _socket.write(buffer + current_index, size - current_index);
            return current_index + write_result;
        }
    };

    template<class IP>
    class tcp_stream {
        sys_socket _socket;
        net_address<IP> _address;

    public:
        tcp_stream() = default;
        tcp_stream(sys_socket socket, const net_address<IP>& address) :
            _socket(std::move(socket)), _address(address) {
        }
        tcp_stream(const tcp_stream&) = delete;
        tcp_stream& operator=(const tcp_stream&) = delete;
        tcp_stream(tcp_stream&& other) noexcept = default;
        tcp_stream& operator=(tcp_stream&& other) noexcept {
            std::swap(_socket, other._socket);
            return *this;
        }
        static auto connect(ipv4 ip, uint16_t port);
        [[nodiscard]] auto read(char* buffer, size_t size) const {
            return socket_read_awaiter<IP>(buffer, size, _socket);
        }
        [[nodiscard]] auto write(const char* buffer, size_t size) const {
            return socket_write_awaiter<IP>(buffer, size, _socket);
        }
        [[nodiscard]] socket_t raw_fd() const {
            return _socket.raw_fd();
        }
        friend class socket_read_awaiter<IP>;
        friend class socket_write_awaiter<IP>;
    };

    template<class IP>
    class socket_connect_awaiter : public detail::source_base<selector, socket_connect_awaiter<IP>> {
        sys_socket _socket;
        net_address<IP> _address;

    public:
        socket_connect_awaiter(const socket_connect_awaiter&) = delete;
        socket_connect_awaiter(socket_connect_awaiter&&)  noexcept = default;
        socket_connect_awaiter& operator=(const socket_connect_awaiter&) = delete;
        socket_connect_awaiter& operator=(socket_connect_awaiter&& other) noexcept {
            std::swap(_socket, other._socket);
            return *this;
        }
        explicit socket_connect_awaiter(net_address<IP> address) :
            _address(std::move(address)) {
            _socket.bind(_address);
        }

        void register_event_impl(selector& selector, intptr_t task_id) const {
            selector.register_event(_socket.raw_fd(), Interest::READABLE, 0, (void*) task_id);
        }

        void deregister_event_impl(selector& selector, intptr_t task_id) const {
            selector.register_event(_socket.raw_fd(), Interest::READABLE, 0, (void*) task_id);
        }

        bool await_ready() {
            int connect_result = _socket.connect(this->_address);
            if (connect_result == -1 && errno == EINPROGRESS)
                return false;
            return true;
        }
        [[nodiscard]] detail::handle_type get_handle() const {
            return _socket.raw_fd();
        }

        void await_suspend(auto handle) {
            current_worker_context
                    .get_poller()
                    .register_event(*this, _socket.raw_fd(), Interest::WRITEABLE);
            current_worker_context.add_suspend_task(_socket.raw_fd(), handle);
        }
        tcp_stream<IP> await_resume();
    };
    template<class IP>
    class socket_listen_awaiter : public detail::source_base<selector, socket_listen_awaiter<IP>> {
        sys_socket& _socket;

    public:
        explicit socket_listen_awaiter(sys_socket& socket) :
            _socket(socket) {
        }
        socket_listen_awaiter(const socket_listen_awaiter&) = delete;

        void register_event_impl(selector& selector, intptr_t task_id) const {
            selector.register_event(_socket.raw_fd(), Interest::READABLE, 0, (void*) task_id);
        }

        void deregister_event_impl(selector& selector) const {
            selector.register_event(_socket.raw_fd(), Interest::READABLE, 0, nullptr);
        }

        bool await_ready() {
            return false;
        }

        [[nodiscard]] detail::handle_type get_handle() const {
            return _socket.raw_fd();
        }
        void await_suspend(auto handle) {
            current_worker_context
                    .get_poller()
                    .register_event(*this, _socket.raw_fd());
            current_worker_context.add_suspend_task(_socket.raw_fd(), handle);
        }
        tcp_stream<IP> await_resume() {
            auto&& [ new_socket, addr ] = _socket.accept<IP>();
            auto& poller = current_worker_context.get_poller();
            poller.deregister_event(*this);
            if (new_socket.raw_fd() == -1)
                throw std::runtime_error("accept error");
            return tcp_stream<IP>(std::move(new_socket), std::move(addr));
        }
    };

    template<class IP>
    class tcp_listener {
        sys_socket _socket;

    public:
        explicit tcp_listener(const net_address<IP>& address) {
            _socket.bind(address);
            _socket.listen();
        }
        tcp_listener(const IP& ip, uint16_t port) :
            tcp_listener(net_address<IP>(ip, port)) {
        }
        ~tcp_listener() = default;
        tcp_listener(const tcp_listener&) = delete;
        tcp_listener& operator=(const tcp_listener&) = delete;
        tcp_listener(tcp_listener&&)  noexcept = default;
        tcp_listener& operator=(tcp_listener&& other) noexcept {
            std::swap(_socket, other._socket);
            return *this;
        }
        [[nodiscard]] auto accept();
        friend class socket_listen_awaiter<IP>;
    };
    template<class IP>
    inline auto tcp_stream<IP>::connect(ipv4 ip, uint16_t port) {
        return socket_connect_awaiter(net_address<IP>(ip, port));
    }

    template<class IP>
    inline tcp_stream<IP> socket_connect_awaiter<IP>::await_resume() {
        return tcp_stream(std::move(this->_socket));
    }
    template<class IP>
    inline auto tcp_listener<IP>::accept() {
        return socket_listen_awaiter<IP>(_socket);
    }
}// namespace coplus
