//
// Created by junjian LI on 2023/4/6.
//

#pragma once
#include "components/concurrent_list.hpp"
#include "components/id_generator.hpp"
#include "coroutine/promise.hpp"
#include "coroutine/task.hpp"
#include "poll/event.hpp"
#include "poll/poller.hpp"
#include "sources/timer/timer.hpp"
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


        class task_destroy_guard{
            task<void> task;
        public:
            task_destroy_guard(::coplus::task<void> task):task(std::move(task)){}
            ~task_destroy_guard(){
                task.destroy();
            }
        };

        void poll_next_task() {
            auto task = std::move(ready_task_queue.front());
            ready_task_queue.pop_front();
            task.resume();
            if (task.is_exception()){
                task_destroy_guard guard(std::move(task));
                std::rethrow_exception(task.get_exception());
            }
            if(task.is_ready())
                task.destroy();
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
