//
// Created by junjian LI on 2023/5/4.
//

#pragma once
#include "network/tcp/socket.hpp"
namespace coplus {
    template<class IP>
    class socket_read_awaiter : public detail::source_base<selector, socket_read_awaiter<IP>> {
        const sys_socket& _socket;
        io_event_context _context;
        bool already_read = false;
        friend class detail::source_base<selector, socket_read_awaiter>;
        void register_event_impl(selector& selector, intptr_t task_id) const {
            selector.register_event(_socket.raw_handle(), Interest::READABLE, 0, (void*) task_id);
        }

        void deregister_event_impl(selector& selector) const {
            //selector.deregister_event(_socket.raw_fd(), Interest::READABLE);
        }

    public:
        socket_read_awaiter(char* buffer, size_t size, const sys_socket& socket) :
            _context{buffer,size}, _socket(socket){};
        socket_read_awaiter(const socket_read_awaiter&) = delete;

        [[nodiscard]] handle_type get_handle() const {
            return _socket.raw_handle();
        }

        bool await_ready() {
            int read_result = _socket.read_to_end(_context);
            already_read = read_result>0;
            if(already_read)
                _context.size = read_result;
            return already_read;
        }

        void await_suspend(co_handle<void> handle) {
            auto& poller = current_worker_context.get_poller();
            poller.register_event(*this, _socket.raw_handle());
            current_worker_context.add_suspend_task(_socket.raw_handle(), task<void>(handle));
        }

        auto await_resume() {
            if(already_read){
                return static_cast<int>(_context.size);
            }
            auto& poller = current_worker_context.get_poller();
            poller.deregister_event(*this);
            int read_result = _socket.read_to_end(_context);
            return read_result;
        }
    };
    template<class IP>
    class socket_write_awaiter : public detail::source_base<selector, socket_write_awaiter<IP>> {
        const sys_socket& _socket;
        io_event_context _context;
        size_t current_index = 0;
    public:
        socket_write_awaiter(const char* buffer, size_t size, const sys_socket& socket) :
            _context{const_cast<char*>(buffer),size}, _socket(socket){};

        void register_event_impl(selector& selector, intptr_t task_id) const {
            selector.register_event(_socket.raw_handle(), Interest::WRITEABLE, 0, (void*) task_id);
        }

        void deregister_event_impl(selector& selector) const {
            //selector.deregister_event(_socket.raw_fd(), Interest::WRITEABLE);
        }

        bool await_ready() {
            int write_result = _socket.write(_context);
            current_index = write_result;
            return write_result == _context.size;
        }
        [[nodiscard]] handle_type get_handle() const {
            return _socket.raw_handle();
        }
        void await_suspend(auto handle) {
            current_worker_context
                    .get_poller()
                    .register_event(*this, _socket.raw_handle());
            current_worker_context.add_suspend_task(_socket.raw_handle(), task<void>(handle));
        }

        auto await_resume() {
            auto& poller = current_worker_context.get_poller();
            poller.deregister_event(*this);
            auto next_context = io_event_context{_context.buffer + current_index, _context.size - current_index};
            int write_result = _socket.write(next_context);
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
        [[nodiscard]] socket_t raw_handle() const {
            return _socket.raw_handle();
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
            selector.register_event(_socket.raw_handle(), Interest::READABLE, 0, (void*) task_id);
        }

        void deregister_event_impl(selector& selector, intptr_t task_id) const {
            //selector.deregister_event(_socket.raw_fd(), Interest::READABLE);
        }

        bool await_ready() {
            int connect_result = _socket.connect(this->_address);
            return !(connect_result == -1 && errno == EINPROGRESS);
        }
        [[nodiscard]] handle_type get_handle() const {
            return _socket.raw_handle();
        }

        void await_suspend(auto handle) {
            current_worker_context
                    .get_poller()
                    .register_event(*this, _socket.raw_handle(), Interest::WRITEABLE);
            current_worker_context.add_suspend_task(_socket.raw_handle(), task<void>(handle));
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
            selector.register_event(_socket.raw_handle(), Interest::READABLE, 0, (void*) task_id);
        }

        void deregister_event_impl(selector& selector) const {
            //selector.deregister_event(_socket.raw_fd(), Interest::READABLE);
        }

        bool await_ready() {
            return false;
        }

        [[nodiscard]] handle_type get_handle() const {
            return _socket.raw_handle();
        }
        void await_suspend(auto handle) {
            current_worker_context
                    .get_poller()
                    .register_event(*this, _socket.raw_handle());
            current_worker_context.add_suspend_task(_socket.raw_handle(), task<void>(handle));
        }
        tcp_stream<IP> await_resume() {
            auto&& [ new_socket, addr ] = _socket.accept<IP>();
            auto& poller = current_worker_context.get_poller();
            poller.deregister_event(*this);
            if (new_socket.raw_handle() == -1)
                throw std::runtime_error(std::string ("accept error:")+strerror(errno));
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