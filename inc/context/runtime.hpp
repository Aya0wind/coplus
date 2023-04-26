#pragma once
//
// Created by junjian LI on 2022/9/7.
//
#include "components/mpmc_channel.hpp"
#include "context/worker_thread_context.hpp"
#include "coroutine/task.hpp"
#include "poll/event.hpp"
#include <map>
#include <queue>
#include <thread>
namespace coplus {
    class event_loop {
        std::atomic<bool>& stop_token;
        std::atomic<size_t> task_size{0};

    public:
        event_loop(std::atomic<bool>& stop_token) :
            stop_token(stop_token) {
        }

        void run_task_blocking(task<void> task) {
            auto events_buffer = detail::sys_events(32);
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

        void operator()();
        void wake_suspend_tasks(detail::sys_events& events, int event_size) {
            for (int i = 0; i < event_size; i++) {
                auto task_id = (intptr_t) events[ i ].data.u64;
                current_worker_context.wake_task(task_id);
            }
        }

        //only use at the same thread!!!
        static void add_local_task(task<void> task) {
            current_worker_context.add_ready_task(std::move(task));
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


        // void schedule_task_to_event_loop(task<void> task, std::unique_ptr<event_loop> target_event_loop) {
        //     if (task.is_ready()) {
        //         return;
        //     }
        //     target_event_loop->schedule_task(std::move(task));
        //     //获取对应线程的任务数量
        //     auto context_task_num = target_event_loop->all_task_size();
        //     //更改线程调度情况
        //     //target_event_loop->wake_poller(target_event_loop->get_poller_sys_handle());
        //     worker_event_loops[ static_cast<int>(context_task_num) ].emplace_back(std::move(target_event_loop));
        // }

        void start_wokers() {
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
        explicit co_runtime(size_t worker = 2) :
            worker_num(worker), main_loop(needStop) {
        }
        static void run() {
            auto& runtime = get_global_runtime();
            runtime.start_wokers();
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
            auto target_event_loop = runtime.take_min_pressure_event_loop();
            //runtime.schedule_task_to_event_loop(std::move(task), std::move(target_event_loop));
            runtime.global_task_queue.push_back(std::move(task));
        }

        template<std::invocable Fn>
        static task<> make_task(Fn&& fn) {
            std::invoke(std::forward<Fn>(fn));
            co_return;
        }

        template<std::invocable Fn>
        static void spawn(Fn&& fn) {
            auto task = make_task(std::forward<Fn>(fn));
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
            auto events_buffer = detail::sys_events(8);
            while (!stop_token.load(std::memory_order_relaxed)) {
                //print_task_size();
                //如果等待队列和就绪队列都是空的，代表线程没有任何任务可进行了
                //print_thread_id();
                //如果就绪队列为空，代表已经没有可以继续推进的任务了，再次尝试从全局任务队列中获取任务
                if (current_worker_context.ready_task_queue.empty()) {
                    auto new_task = co_runtime::get_global_runtime().get_global_task_queue().take_front();
                    current_worker_context.add_ready_task(std::move(new_task));
                }

                //尝试推进所有就绪任务
                current_worker_context.poll_all_task();
                //轮询事件
                int event_size = current_worker_context.poller.poll_events(events_buffer, std::chrono::milliseconds(100));
                //唤醒监听事件的任务，重新从等待队列加入就绪队列
                wake_suspend_tasks(events_buffer, event_size);
            }
        }
    }


}// namespace coplus
