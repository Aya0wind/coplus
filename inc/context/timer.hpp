//
// Created by junjian LI on 2022/9/7.
//

#pragma once
#include "components/id_generator.hpp"
#include "context/runtime.hpp"
#include "context/worker_thread_context.hpp"
#include "poll/poller.hpp"
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <unistd.h>

#ifdef __APPLE__
#include "poll/selector/kqueue/kqueue_timer.hpp"
namespace coplus {
    using timer_type = coplus::kqueue_timer;
}
#elif _WIN32
#include "poll/selector/iocp/iocp_timer.hpp"
namespace coplus {
    using timer_type = coplus::iocp_timer;
}
#elif __linux__
#include "poll/selector/epoll/kqueue_timer.hpp"
namespace coplus {
    using timer_type = coplus::epoll_timer;
}
#endif
