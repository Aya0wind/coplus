//
// Created by junjian LI on 2023/4/18.
//

#pragma once
#include "poll/traits.hpp"
#include <sys/types.h>
#ifdef __linux__

#include <sys/epoll.h>
#include <vector>
namespace coplus::detail {
    using sys_event = struct epoll_event;
    using sys_events = ::std::vector<sys_event>;
}// namespace coplus::detail
#elif __APPLE__
#include <sys/event.h>
#include <vector>
namespace coplus::detail {
    using event = struct kevent;
    using events = ::std::vector<event>;
}// namespace coplus::detail
#elif _WIN32
#include "poll/event/iocp/event.hpp"
#endif
namespace coplus::detail {
    class event {
        Interest interest;
        handle_type handle;
        void* udata;
    };
}// namespace coplus::detail
