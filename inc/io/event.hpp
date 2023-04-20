//
// Created by junjian LI on 2023/4/18.
//

#pragma once
#include <cstdint>
namespace coplus::detail {
    using token_type = intptr_t;
    enum Interest:uint8_t{
        READABLE = 1,
        WRITEABLE = 2,
        AIO = 4,
        TIMER = 8,
    };

}



#ifdef __linux__
#include "io/event/epoll/event.hpp"
using event =
#elif __APPLE__
#include "io/selector/kqueue/kqueue_event.hpp"
#elif _WIN32
#include "io/event/iocp/event.hpp"
#endif

