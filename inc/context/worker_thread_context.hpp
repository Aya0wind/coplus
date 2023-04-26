//
// Created by junjian LI on 2023/4/6.
//

#pragma once
#include "components/concurrent_list.hpp"
#include "components/id_generator.hpp"
#include "coroutine/task.hpp"
#include "poll/poller.hpp"
#include <atomic>
#include <deque>
#include <iostream>
#include <sstream>
#include <thread>
#include <unordered_map>
namespace coplus {
    class worker_thread_context {
        detail::poller poller;
        std::unordered_map<int64_t, task<void>> suspend_tasks;
        std::list<task<void>> ready_task_queue;
        int64_t current_task_id{0};
        void wake_task(int64_t task_id) {
            if (suspend_tasks.contains(task_id)) {
                ready_task_queue.emplace_back(std::move(suspend_tasks[ task_id ]));
                suspend_tasks.erase(task_id);
            }
            else {
                fmt::print("task_id: {} not found\n", task_id);
            }
        }

    public:
        [[nodiscard]] int64_t get_current_task_id() const {
            return current_task_id;
        }
        detail::poller& get_poller() {
            return poller;
        }

        void add_ready_task(task<void> task) {
            ready_task_queue.emplace_back(std::move(task));
        }

        void poll_next_task() {
            auto task = std::move(ready_task_queue.front());
            ready_task_queue.pop_front();
            current_task_id = task.get_id();
            task.resume();
            if (task.is_exception())
                std::rethrow_exception(task.get_exception());
            if (!task.is_ready()) {
                suspend_tasks.emplace(task.get_id(), std::move(task));
            }
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

        [[nodiscard]] auto poller_sys_handle() const {
            return poller.get_selector().get_handle();
        }

        friend class event_loop;
    };

    inline thread_local worker_thread_context current_worker_context;



}// namespace coplus
