#pragma once


#include "poll/poller.hpp"
#include <chrono>
#include <cstdint>
#include <stdexcept>
#include <sys/timerfd.h>
namespace coplus {

    class epoll_timer : public detail::source_base<selector, epoll_timer> {
        int timer_fd;
        void register_event_impl(selector& selector, intptr_t taskid) {
            selector.register_event(timer_fd, detail::Interest::READABLE, 0, (void*) taskid);
        }
        void deregister_event_impl(selector& selector) {
            selector.deregister_event(timer_fd, detail::Interest::READABLE);
        }
        friend class detail::source_base<selector, epoll_timer>;

    public:
        epoll_timer(std::chrono::milliseconds timeout, bool repeat) :
            timer_fd(timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC)) {
            struct itimerspec new_value;
            // 第一次超时时间
            new_value.it_value.tv_sec = timeout.count() / 1000;
            new_value.it_value.tv_nsec = timeout.count() % 1000 * 1000000;
            if (repeat) {
                // 设置超时间隔
                new_value.it_interval.tv_sec = timeout.count() / 1000;
                new_value.it_interval.tv_nsec = timeout.count() % 1000 * 1000000;
            }
            // 设置第一次超时时间和超时间隔
            if (timerfd_settime(timer_fd, TFD_TIMER_ABSTIME, &new_value, NULL) == -1)
                throw std::runtime_error("timerfd_settime error");
        }

        detail::handle_type get_handle() const {
            return timer_fd;
        }
    };
}// namespace coplus
