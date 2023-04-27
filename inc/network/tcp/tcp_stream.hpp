//
// Created by junjian LI on 2023/4/18.
//

#pragma once
#include "context/runtime.hpp"
#include "coroutine/task.hpp"
#include "poll/poller.hpp"
#include "poll/traits.hpp"
#include "socket.hpp"
#include <memory>
namespace coplus {

    class socket_read_awaiter : public detail::source_base<selector, socket_read_awaiter> {
        const sys_socket& _socket;
        char* buffer;
        size_t size;
        friend class detail::source_base<selector, socket_read_awaiter>;
        void register_event_impl(selector& selector, intptr_t task_id) const {
            selector.register_event(_socket.raw_fd(), detail::Interest::READABLE, 0, (void*) task_id);
        }

        void deregister_event_impl(selector& selector) const {
            selector.register_event(_socket.raw_fd(), detail::Interest::READABLE, 0, nullptr);
        }

    public:
        socket_read_awaiter(char* buffer, size_t size, const sys_socket& socket) :
            buffer(buffer), size(size), _socket(socket){};
        socket_read_awaiter(const socket_read_awaiter&) = delete;

        detail::handle_type get_handle() const {
            return _socket.raw_fd();
        }

        bool await_ready() {
            return false;
        }

        void await_suspend(auto handle) {
            auto& poller = current_worker_context.get_poller();
            poller.register_event(*this, current_worker_context.get_current_task_id());
        }

        auto await_resume() {
            auto& poller = current_worker_context.get_poller();
            poller.deregister_event(*this);
            return _socket.read(buffer, size);
        }
        friend class tcp_stream;
    };

    class socket_write_awaiter : public detail::source_base<selector, socket_write_awaiter> {
        const sys_socket& _socket;
        const char* buffer;
        size_t size;

    public:
        socket_write_awaiter(const char* buffer, size_t size, const sys_socket& socket) :
            buffer(buffer), size(size), _socket(socket){};

        void register_event_impl(selector& selector, intptr_t task_id) const {
            selector.register_event(_socket.raw_fd(), detail::Interest::WRITEABLE, 0, (void*) task_id);
        }

        void deregister_event_impl(selector& selector) const {
            selector.register_event(_socket.raw_fd(), detail::Interest::WRITEABLE, 0, nullptr);
        }

        bool await_ready() {
            return false;
        }
        detail::handle_type get_handle() const {
            return _socket.raw_fd();
        }
        void await_suspend(auto handle) {
            current_worker_context
                    .get_poller()
                    .register_event(*this, current_worker_context.get_current_task_id());
        }

        auto await_resume() {
            auto& poller = current_worker_context.get_poller();
            poller.deregister_event(*this);
            return _socket.write(buffer, size);
        }
    };
    class tcp_stream {
        sys_socket _socket;

    public:
        tcp_stream() = default;
        tcp_stream(sys_socket socket) :_socket(std::move(socket)) {}
        tcp_stream(const tcp_stream&) = delete;
        tcp_stream& operator=(const tcp_stream&) = delete;
        tcp_stream(tcp_stream&& other) noexcept = default;
        tcp_stream& operator=(tcp_stream&& other) noexcept{
            std::swap(_socket, other._socket);
            return *this;
        }

        static auto connect(ipv4 ip, uint16_t port);
        [[nodiscard]] auto read(char* buffer, size_t size) const {
            return socket_read_awaiter(buffer, size, _socket);
        }
        [[nodiscard]] auto write(const char* buffer, size_t size) const {
            return socket_write_awaiter(buffer, size, _socket);
        }
        [[nodiscard]] socket_t raw_fd() const{
            return _socket.raw_fd();
        }
        friend class socket_listen_awaiter;
        friend class socket_read_awaiter;
        friend class socket_write_awaiter;
    };

    template<class IP>
    class socket_connect_awaiter : public detail::source_base<selector, socket_connect_awaiter<IP>> {
        sys_socket _socket;
        net_address<IP> _address;

    public:
        socket_connect_awaiter(const socket_connect_awaiter&) = delete;
        socket_connect_awaiter(socket_connect_awaiter&&) = default;
        socket_connect_awaiter& operator=(const socket_connect_awaiter&) = delete;
        socket_connect_awaiter& operator=(socket_connect_awaiter&&other) {
            std::swap(_socket, other._socket);
            return *this;
        }
        explicit socket_connect_awaiter(net_address<IP> address) :
            _address(std::move(address)) {
            _socket.bind(_address);
        }

        void register_event_impl(selector& selector, intptr_t task_id) const {
            selector.register_event(_socket.raw_fd(), detail::Interest::READABLE, 0, (void*) task_id);
        }

        void deregister_event_impl(selector& selector, intptr_t task_id) const {
            selector.register_event(_socket.raw_fd(), detail::Interest::READABLE, 0, (void*) task_id);
        }

        bool await_ready() {
            return false;
        }
        detail::handle_type get_handle() const {
            return _socket.raw_fd();
        }

        void await_suspend(auto handle) {
            _socket.connect(this->_address);
            current_worker_context
                    .get_poller()
                    .register_event(*this,
                                    this->_socket.raw_fd(),
                                    detail::Interest::WRITEABLE,
                                    current_worker_context.get_current_task_id());
        }
        tcp_stream await_resume();
    };

    class socket_listen_awaiter : public detail::source_base<selector, socket_listen_awaiter> {
        sys_socket& _socket;
    public:
        explicit socket_listen_awaiter(sys_socket& socket) :
            _socket(socket) {
        }
        socket_listen_awaiter(const socket_listen_awaiter&) = delete;

        void register_event_impl(selector& selector, intptr_t task_id) const {
            selector.register_event(_socket.raw_fd(), detail::Interest::READABLE, 0, (void*) task_id);
        }

        void deregister_event_impl(selector& selector) const {
            selector.register_event(_socket.raw_fd(), detail::Interest::READABLE, 0, nullptr);
        }

        bool await_ready() {
            return false;
        }

        detail::handle_type get_handle() const {
            return _socket.raw_fd();
        }
        void await_suspend(auto handle) {
            current_worker_context
                    .get_poller()
                    .register_event(*this, current_worker_context.get_current_task_id());
        }
        tcp_stream await_resume() {
            return {_socket.accept()};
        }
    };

    class tcp_listener {
        sys_socket _socket;

    public:
        template<class IP>
        explicit tcp_listener(const net_address<IP>& address) {
            _socket.bind(address);
            _socket.listen();
        }
        ~tcp_listener() = default;
        tcp_listener(const tcp_listener&) = delete;
        tcp_listener& operator=(const tcp_listener&) = delete;
        tcp_listener(tcp_listener&&) = default;
        tcp_listener& operator=(tcp_listener&& other){
            std::swap(_socket, other._socket);
            return *this;
        }
        [[nodiscard]] auto accept();
        friend class socket_listen_awaiter;
    };

    inline auto tcp_stream::connect(ipv4 ip, uint16_t port) {
        return socket_connect_awaiter(net_address<ipv4>(ip, port));
    }

    template<class IP>
    inline tcp_stream socket_connect_awaiter<IP>::await_resume() {
        return tcp_stream(std::move(this->_socket));
    }
    inline auto tcp_listener::accept() {
        return socket_listen_awaiter(_socket);
    }
}// namespace coplus
