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
#ifdef __linux__
#include "poll/selector/epoll/epoll_timer.hpp"
#endif
namespace coplus {

    struct DelayAwaiter {
        epoll_timer timer;

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
            //current_worker_context.get_poller().deregister_event(*this, timer_id, detail::Interest::TIMER);
        }

    private:
        bool repeat{false};
    };

    inline DelayAwaiter operator""_ms(unsigned long long ms) {
        return DelayAwaiter::delay(std::chrono::milliseconds(ms));
    }

}// namespace coplus
