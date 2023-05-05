//
// Created by junjian LI on 2023/4/27.
//

#pragma once
#include "coplus/context/worker_thread_context.hpp"
#include "coplus/coroutine/promise.hpp"
#include "coplus/coroutine/task_awaiter.hpp"
#include "coplus/poll/event.hpp"
#include <chrono>

namespace coplus {
    struct DelayAwaiter {
        int expire_times;
        template<class duration_type, class period>
        static DelayAwaiter delay(std::chrono::duration<duration_type, period> duration) {
            return DelayAwaiter(duration);
        }

        DelayAwaiter() = delete;
        DelayAwaiter(const DelayAwaiter&) = delete;
        template<class duration_type, class period>
        explicit DelayAwaiter(std::chrono::duration<duration_type, period> timeout, bool repeat = false) :
            expire_times(std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count()),
            repeat(repeat) {
        }

        bool await_ready() {
            return false;
        }

        void await_suspend(auto handle) {
            auto& timer = current_worker_context.get_timer();
            token_type timer_token = timer.get_token();
            timer.set_expire_timeout(expire_times);
            auto& selector = current_worker_context.get_poller().get_selector();
            timer.register_event(selector, timer_token);
            current_worker_context.add_suspend_task(timer_token, task<>(handle));
        }

        void await_resume() {
            auto& timer = current_worker_context.get_timer();
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
