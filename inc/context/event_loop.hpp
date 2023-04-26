//
// Created by junjian LI on 2023/4/26.
//

#pragma once
#include "worker_thread_context.hpp"

namespace coplus {
    class event_loop {
        concurrent_list<task<void>> global_task_queue;
        std::atomic<bool>& stop_token;
        std::atomic<size_t> task_size{0};

    public:
        event_loop(std::atomic<bool>& stop_token) :
            stop_token(stop_token) {
        }
        void stop() {
            global_task_queue.down();
        }
        void run_task_blocking(task<void> task) {
            auto events_buffer = detail::events(32);
            fmt::print("thread tasks:{}\n", global_task_queue.size());
            while (!task.is_ready()) {
                task.resume();
                if (task.is_exception()) {
                    std::rethrow_exception(task.get_exception());
                }
                int event_size = current_worker_context.get_poller().poll_events(events_buffer, std::chrono::milliseconds(100));
                if (event_size > 0) {
                    // if ((events_buffer[ 0 ].flags & EV_ERROR) != 0 && events_buffer[ 0 ].data != 0) {
                    //     fmt::print("error event:{}\n", strerror(events_buffer[ 0 ].data));
                    //     return;
                    // }
                    // else if ((events_buffer[ 0 ].flags & EV_EOF) != 0) {
                    //     return;
                    // }
                }
            }
        }
        void print_task_size() {
            std::stringstream ss;
            ss << std::this_thread::get_id();
            fmt::print("thread :{}size:{}\n", ss.str(), global_task_queue.size() + current_worker_context.all_task_size());
        }


        void operator()() {
            auto events_buffer = detail::events(8);
            while (!stop_token.load(std::memory_order_relaxed)) {
                //print_task_size();
                //如果等待队列和就绪队列都是空的，代表线程没有任何任务可进行了
                //print_thread_id();
                //如果就绪队列为空，代表已经没有可以继续推进的任务了，再次尝试从全局任务队列中获取任务
                if (current_worker_context.ready_task_queue.empty() && !global_task_queue.empty()) {
                    global_task_queue.swap_inner(current_worker_context.ready_task_queue);
                }
                if (current_worker_context.all_task_size() == 0) {
                    //阻塞等待调度器下发任务
                    wait_for_global_dispatch();
                    //交换就绪队列和全局任务队列
                    global_task_queue.swap_inner(current_worker_context.ready_task_queue);
                }
                //尝试推进所有就绪任务
                current_worker_context.poll_all_task();
                //轮询事件
                int event_size = current_worker_context.poller.poll_events(events_buffer, std::chrono::milliseconds(100));
                print_thread_id("sub");
                //唤醒监听事件的任务，重新从等待队列加入就绪队列
                wake_suspend_tasks(events_buffer, event_size);
            }
        }

        void wait_for_global_dispatch() {
            global_task_queue.wait_not_empty();
        }


        void wake_suspend_tasks(detail::events& events, int event_size) {
            for (int i = 0; i < event_size; i++) {
                auto task_id = (intptr_t) events[ i ].data.u64;
                current_worker_context.wake_task(task_id);
            }
        }

        void schedule_task(task<void> task) {
            global_task_queue.emplace_back(std::move(task));
        }

        //only use at the same thread!!!
        static void add_local_task(task<void> task) {
            current_worker_context.add_ready_task(std::move(task));
        }

        //所有三个队列的任务总数
        size_t all_task_size() {
            return global_task_queue.size() + current_worker_context.all_task_size();
        }
    };

}// namespace coplus
