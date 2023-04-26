#pragma once
//
// Created by junjian LI on 2022/9/7.
//
#include "components/mpmc_channel.hpp"
#include "coroutine/task.hpp"
#include "event_loop.hpp"
#include <map>
#include <queue>
#include <thread>
namespace coplus {

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

        mpmc_channel<task<>>& get_global_task_queue() {
            return global_task_queue;
        };

        void schedule_task_to_event_loop(task<void> task, std::unique_ptr<event_loop> target_event_loop) {
            if (task.is_ready()) {
                return;
            }
            target_event_loop->schedule_task(std::move(task));
            //获取对应线程的任务数量
            auto context_task_num = target_event_loop->all_task_size();
            //更改线程调度情况
            //target_event_loop->wake_poller(target_event_loop->get_poller_sys_handle());
            worker_event_loops[ static_cast<int>(context_task_num) ].emplace_back(std::move(target_event_loop));
        }

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
            runtime.schedule_task_to_event_loop(std::move(task), std::move(target_event_loop));
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
                    event_loop_ptr->stop();
                }
            }
            for (auto& thread: worker_threads) {
                thread.join();
            }
        }
    };


}// namespace coplus
