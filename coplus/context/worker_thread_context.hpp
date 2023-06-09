//
// Created by junjian LI on 2023/4/6.
//

#pragma once
#include "coplus/components/concurrent_list.hpp"
#include "coplus/components/id_generator.hpp"
#include "coplus/coroutine/promise.hpp"
#include "coplus/coroutine/task.hpp"
#include "coplus/poll/event.hpp"
#include "coplus/poll/poller.hpp"
#include "coplus/poll/traits.hpp"
#include "coplus/sources/timer/timer.hpp"
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <deque>
#include <iostream>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <vector>
namespace coplus {
    class worker_thread_context {
        detail::poller poller;
        std::unordered_map<token_type, std::vector<task<void>>> suspend_tasks;
        std::deque<task<void>> ready_task_queue;
        timer_type timer;
        void wake_task(token_type waked_token) {
            if (suspend_tasks.contains(waked_token)) {
                auto waked_tasks = std::move(suspend_tasks[ waked_token ]);
                for (auto& task: waked_tasks) {
                    ready_task_queue.push_back(std::move(task));
                }
                suspend_tasks.erase(waked_token);
            }
            // 虚假事件或过期事件，无视
        }

    public:
        worker_thread_context() :
            timer(poller.get_selector(), 0) {
        }
        timer_type& get_timer() {
            return timer;
        }

        detail::poller& get_poller() {
            return poller;
        }

        void add_ready_task(task<void> task) {
            ready_task_queue.emplace_back(std::move(task));
        }

        void add_suspend_task(token_type token, task<void> task) {
            suspend_tasks[ token ].push_back(std::move(task));
        }

        template<detail::SourceTrait<selector> source_type>
        void register_event(source_type&& s, token_type token) {
            poller.register_event(std::forward<source_type>(s), token);
        }

        template<detail::SourceTrait<selector> source_type>
        void deregister_event(source_type&& s) {
            poller.deregister_event(std::forward<source_type>(s));
        }

        void poll_next_task() {
            auto task = std::move(ready_task_queue.front());
            ready_task_queue.pop_front();
            task.resume();
            if (task.is_exception()) {
                std::rethrow_exception(task.get_exception());
            }
            if (!task.is_ready())
                task.detach_handle();
        }

        void poll_all_task() {
            while (!ready_task_queue.empty()) {
                poll_next_task();
            }
        }

        size_t all_task_size() {
            return ready_task_queue.size() + suspend_tasks.size();
        }

        size_t suspend_task_size() {
            return suspend_tasks.size();
        }
        size_t ready_task_size() {
            return ready_task_queue.size();
        }
        friend class event_loop;
    };

    inline thread_local worker_thread_context current_worker_context;


}// namespace coplus
