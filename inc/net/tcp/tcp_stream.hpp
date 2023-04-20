//
// Created by junjian LI on 2023/4/18.
//

#pragma once
#include "coro/task.hpp"

namespace cocpp {
    class tcp_stream {
        runtime& _runtime;
    public:
        task<> connect(const std::string& ip, uint16_t port);
        task<size_t> read(char* buffer, size_t size);
        task<size_t> write(const char* buffer, size_t size);
        task<> close();
    };

    class tcp_listener {
        runtime& _runtime;
    public:
        tcp_listener();
        ~tcp_listener();
        tcp_listener(const tcp_listener&) = delete;
        tcp_listener& operator=(const tcp_listener&) = delete;
        tcp_listener(tcp_listener&&) = delete;
        tcp_listener& operator=(tcp_listener&&) = delete;
        task<> bind(const std::string& ip, uint16_t port);
        task<tcp_stream> accept(;
        task<> close();
    };



}