#pragma once


#include "coplus/poll/event.hpp"
#include "coplus/poll/poller.hpp"
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <sys/timerfd.h>
namespace coplus {

    class io_uring_timer : public detail::source_base<selector, io_uring_timer> {
        int timer_fd;
        void register_event_impl(selector& selector, intptr_t taskid) {
            selector.register_event(timer_fd, Interest::READABLE, 0, (void*) taskid);
        }
        void deregister_event_impl(selector& selector) {
            selector.deregister_event(timer_fd, Interest::READABLE);
        }
        friend class detail::source_base<selector, io_uring_timer>;

    public:
        io_uring_timer(selector& selector, int expire_time, bool repeat = false) :
            timer_fd(timerfd_create(CLOCK_MONOTONIC, 0)) {
            set_expire_timeout(2000);
        }

        void set_expire_timeout(int expire_times) {
            struct itimerspec new_value;
            // 第一次超时时间
            new_value.it_value.tv_sec = expire_times / 1000;
            new_value.it_value.tv_nsec = expire_times % 1000 * 1000000;
            new_value.it_interval.tv_sec = new_value.it_value.tv_sec;
            new_value.it_interval.tv_nsec = new_value.it_value.tv_nsec;
            if (timerfd_settime(timer_fd, 0, &new_value, NULL) == -1)
                throw std::runtime_error(std::string("timerfd_settime error") + strerror(errno));
        }

        handle_type get_handle() const {
            return timer_fd;
        }
        token_type get_token() const {
            return timer_fd;
        }
        ~io_uring_timer() {
            close(timer_fd);
        }
    };
}// namespace coplus
