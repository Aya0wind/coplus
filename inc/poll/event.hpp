//
// Created by junjian LI on 2023/4/18.
//

#pragma once
#ifdef __linux__
#include "poll/event/epoll/event.hpp"
using event =
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

