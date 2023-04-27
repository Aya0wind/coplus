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
namespace coplus {

    struct DelayAwaiter {
        timer_type timer;

        template<class duration_type, class period>
        static DelayAwaiter delay(std::chrono::duration<duration_type, period> duration) {
            return DelayAwaiter(duration);
        }

        DelayAwaiter() = delete;
        DelayAwaiter(const DelayAwaiter&) = delete;
        DelayAwaiter(std::chrono::milliseconds timeout, bool repeat = false) :
            timer(timeout, repeat) {
        }

        bool await_ready() {
            //bool r = start_duration.count() + time.expire_time<=std::chrono::system_clock::now().time_since_epoch().count();
            return false;
        }

        void await_suspend(auto handle) {
            auto& selector = current_worker_context.get_poller().get_selector();
            timer.register_event(selector, current_worker_context.get_current_task_id());
        }

        void await_resume() {
            auto& selector = current_worker_context.get_poller().get_selector();
            timer.deregister_event(selector);
        }

    private:
        bool repeat{false};
    };

    inline DelayAwaiter operator""_us(unsigned long long us) {
        return DelayAwaiter::delay(std::chrono::microseconds(us));
    }

    inline DelayAwaiter operator""_ns(unsigned long long ns) {
        return DelayAwaiter::delay(std::chrono::nanoseconds(ns));
    }

    inline DelayAwaiter operator""_ms(unsigned long long ms) {
        return DelayAwaiter::delay(std::chrono::milliseconds(ms));
    }

    inline DelayAwaiter operator""_s(unsigned long long s) {
        return DelayAwaiter::delay(std::chrono::seconds(s));
    }

    inline DelayAwaiter operator""_min(unsigned long long m) {
        return DelayAwaiter::delay(std::chrono::minutes(m));
    }

    inline DelayAwaiter operator""_hour(unsigned long long h) {
        return DelayAwaiter::delay(std::chrono::hours(h));
    }

    inline DelayAwaiter operator""_day(unsigned long long d) {
        return DelayAwaiter::delay(std::chrono::hours(d * 24));
    }

}// namespace coplus
