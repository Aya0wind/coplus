//
// Created by junjian LI on 2022/9/7.
//

#pragma once

#ifdef __APPLE__
#include "coplus/poll/selector/kqueue/kqueue_timer.hpp"
#include <unistd.h>
namespace coplus {
    using timer_type = coplus::kqueue_timer;
}
#elif _WIN32
#include "coplus/poll/selector/iocp/iocp_timer.hpp"
namespace coplus {
    using timer_type = coplus::iocp_timer;
}
#elif __linux__
#include "coplus/poll/selector/epoll/epoll_timer.hpp"
#include <unistd.h>
namespace coplus {
    using timer_type = coplus::epoll_timer;
}
#endif
