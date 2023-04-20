#pragma once
//
// Created by junjian LI on 2022/9/7.
//
#include "coro/task.hpp"
#include "io/poll.hpp"
#include "worker_local_context.hpp"
#include <map>
#include <thread>
namespace coplus {

    class co_runtime {
        size_t worker_num;
        std::map<int, std::vector<std::unique_ptr<event_loop>>> event_loops_map;
        std::vector<std::thread> worker_threads;
        event_loop main_loop;
        std::atomic<bool> needStop{false};

        std::unique_ptr<event_loop> take_min_pressure_event_loop() {
            auto iter = event_loops_map.begin();
            while (iter->second.empty())
                ++iter;
            auto &minPressure = iter->second;
            auto targetThread = std::move(*minPressure.rbegin());
            minPressure.pop_back();
            return targetThread;
        }

        void schedule_task_to_event_loop(task<void> task, std::unique_ptr<event_loop> target_event_loop) {
            if(task.is_ready()){
                return;
            }
            target_event_loop->add_task(std::move(task));
            //获取对应线程的任务数量
            auto context_task_num = target_event_loop->all_task_size();
            //更改线程调度情况
            event_loops_map[static_cast<int>(context_task_num)].emplace_back(std::move(target_event_loop));
        }
    public:
        explicit co_runtime(size_t worker = std::thread::hardware_concurrency()) : worker_num(worker), main_loop(needStop){
            //initialize threadMap
            auto & event_loops = event_loops_map[0];
            event_loops.reserve(worker);
            for (int i = 0; i < worker; ++i) {
                auto event_loop = std::make_unique<class event_loop>(needStop);
                auto systemThread = ::std::thread([loop = std::ref(*event_loop)](){
                    loop.get().set_context(&current_worker_context);
                    loop();
                });
                worker_threads.emplace_back(std::move(systemThread));
                event_loops.emplace_back(std::move(event_loop));
            }
        }

        size_t get_worker_num()const{
            return worker_num;
        }

        static void block_on(task<void> task){
            //当前线程等待任务完成
            event_loop::wait_for_task(std::move(task));
        }

        static void spawn(task<void> task){
            auto& runtime = get_global_runtime();
            auto target_event_loop = runtime.take_min_pressure_event_loop();
            runtime.schedule_task_to_event_loop(std::move(task),std::move(target_event_loop));
        }


        inline static co_runtime& get_global_runtime(){
            static co_runtime global_runtime;
            return global_runtime;
        }

        ~co_runtime(){
            needStop = true;
            for (auto&& event_loop:event_loops_map) {
                for (auto&& event_loop_ptr:event_loop.second){
                    event_loop_ptr->stop();
                }
            }
            for (auto& thread : worker_threads) {
                thread.join();
            }
        }
    };


}// namespace coplus
