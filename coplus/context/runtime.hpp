//
// Created by junjian LI on 2022/9/7.
//
#pragma once
#include "coplus/components/mpmc_channel.hpp"
#include "coplus/context/worker_thread_context.hpp"
#include "coplus/coroutine/promise.hpp"
#include "coplus/coroutine/task.hpp"
#include "coplus/poll/event.hpp"
#include <concepts>
#include <map>
#include <queue>
#include <string>
#include <thread>
#include <vector>
namespace coplus {
    namespace detail {

        template<std::invocable Fn>
        auto make_task(Fn fn) -> task<> {
            auto inner_task = std::invoke(std::move(fn));
            co_await inner_task;
            inner_task.detach_handle();
            co_return;
        }


    }// namespace detail


    class event_loop {
        std::atomic<bool>& stop_token;
        std::atomic<size_t> task_size{0};

    public:
        event_loop(std::atomic<bool>& stop_token) :
            stop_token(stop_token) {
        }

        void run_task_blocking(task<void> task) {
            auto events_buffer = std::vector<event>(32);
            while (!task.is_ready()) {
                task.resume();
                if (task.is_exception()) {
                    std::rethrow_exception(task.get_exception());
                }
                int event_size = current_worker_context
                                         .get_poller()
                                         .poll_events(events_buffer, std::chrono::milliseconds(100));
                if (event_size > 0) {
                    wake_suspend_tasks(events_buffer, event_size);
                }
            }
        }

        void operator()();
        static void wake_suspend_tasks(events& events, int event_size) {
            for (int i = 0; i < event_size; i++) {
                if (events[ i ].is_error() || events[ i ].is_read_closed() || events[ i ].is_write_closed()) {
                    continue;
                }
                else if (events[ i ].is_read_closed()) {
                    auto task_id = (intptr_t) events[ i ].get_token();
                    current_worker_context.wake_task(task_id);
                }
                else {
                    auto task_id = (intptr_t) events[ i ].get_token();
                    current_worker_context.wake_task(task_id);
                }
            }
        }
    };
    class co_runtime {
        size_t worker_num;
        std::map<int, std::vector<std::unique_ptr<event_loop>>> worker_event_loops;
        std::vector<std::thread> worker_threads;
        mpmc_channel<task<>> global_task_queue;
        std::atomic<bool> needStop{false};
        event_loop main_loop;

        std::unique_ptr<event_loop> take_min_pressure_event_loop() {
            auto iter = worker_event_loops.begin();
            while (iter->second.empty())
                ++iter;
            auto& minPressure = iter->second;
            auto targetThread = std::move(*minPressure.rbegin());
            minPressure.pop_back();
            return targetThread;
        }

        void start_workers() {
            //initialize threadMap
            auto& event_loops = worker_event_loops[ 0 ];
            event_loops.reserve(worker_num);
            for (int i = 0; i < worker_num; ++i) {
                auto event_loop = std::make_unique<class event_loop>(needStop);
                auto systemThread = ::std::thread([ loop = std::ref(*event_loop) ]() {
                    loop();
                });
                worker_threads.emplace_back(std::move(systemThread));
                event_loops.emplace_back(std::move(event_loop));
            }
        }

    public:
        mpmc_channel<task<>>& get_global_task_queue() {
            return global_task_queue;
        };
        explicit co_runtime(size_t worker = std::thread::hardware_concurrency()) :
            worker_num(worker), main_loop(needStop) {
            start_workers();
        }
        static void print_global_task_queue_size() {
            std::cout << "global_task_queue size: " << get_global_runtime().global_task_queue.size() << std::endl;
        }
        static void run() {
            auto& runtime = get_global_runtime();
            runtime.main_loop();
        }
        size_t get_worker_num() const {
            return worker_num;
        }

        static void block_on(task<void> task) {
            //当前线程等待任务完成
            co_runtime::get_global_runtime().main_loop.run_task_blocking(std::move(task));
        }

        static void spawn(task<void> task) {
            auto& runtime = get_global_runtime();
            //auto target_event_loop = runtime.take_min_pressure_event_loop();
            //runtime.schedule_task_to_event_loop(std::move(task), std::move(target_event_loop));
            runtime.global_task_queue.push_back(std::move(task));
        }

        template<std::invocable Fn>
        static void spawn(Fn fn) {
            auto task = detail::make_task(std::move(fn));
            spawn(std::move(task));
        }


        inline static co_runtime& get_global_runtime() {
            static co_runtime global_runtime;
            return global_runtime;
        }

        void schedule_task(task<void> task) {
            global_task_queue.push_back(std::move(task));
        }


        ~co_runtime() {
            needStop.store(true, std::memory_order_release);
            for (auto&& event_loop: worker_event_loops) {
                for (auto&& event_loop_ptr: event_loop.second) {
                    //event_loop_ptr->stop();
                }
            }
            for (auto& thread: worker_threads) {
                thread.join();
            }
        }
    };

    inline void event_loop::operator()() {
        {
            auto events_buffer = events(8);
            while (!stop_token.load(std::memory_order_relaxed)) {
                //如果就绪队列为空，代表已经没有可以继续推进的任务了，再次尝试从全局任务队列中获取任务
                if (current_worker_context.ready_task_queue.empty()) {
                    auto new_task = co_runtime::get_global_runtime().get_global_task_queue().try_take_front();
                    if (new_task.has_value())
                        current_worker_context.add_ready_task(std::move(new_task.value()));
                }
                //尝试推进所有就绪任务
                current_worker_context.poll_all_task();
                if (!current_worker_context.suspend_tasks.empty()) {
                    int event_size = 0;
                    //尝试尽可能的处理更多的事件
                    event_size = current_worker_context
                                         .get_poller()
                                         .poll_events(events_buffer, std::chrono::milliseconds(50));
                    wake_suspend_tasks(events_buffer, event_size);
                }
            }
        }
    }


}// namespace coplus
