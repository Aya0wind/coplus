//
// Created by junjian LI on 2023/4/6.
//

#pragma once
#include <deque>
#include <thread>
#include <iostream>
#include <unordered_map>
#include <sstream>
#include "coro/task.hpp"
#include "components/concurrent_list.hpp"
#include "components/id_generator.hpp"
#include "io/poll.hpp"
namespace coplus {
    class worker_local_context {

        detail::poll poller;
        size_t runned_task_count{0};
        std::unordered_map<int64_t,task<void>> suspend_tasks;
        std::list<task<void>> ready_task_queue;
        int64_t current_task_id;
        void wake_task(int64_t task_id){
            if(suspend_tasks.contains(task_id)){
                ready_task_queue.emplace_back(std::move(suspend_tasks[task_id]));
                suspend_tasks.erase(task_id);
            }
        }

    public:
        ~worker_local_context(){
            fmt::print("runned task count: {}\n",runned_task_count);
        }
        size_t get_runned_task_count()const{
            return runned_task_count;
        }
        int64_t get_current_task_id()const{
            return current_task_id;
        }
        detail::poll& get_poller(){
            return poller;
        }

        void add_ready_task(task<void> task){
            ready_task_queue.emplace_back(std::move(task));
        }

        void poll_next_task() {
            auto task = std::move(ready_task_queue.front());
            ready_task_queue.pop_front();
            current_task_id = task.get_id();
            task.resume();
            if(!task.is_ready()){
                suspend_tasks.emplace(task.get_id(),std::move(task));
            }
            ++runned_task_count;
        }

        void poll_all_task() {
            while (!ready_task_queue.empty()) {
                poll_next_task();
            }
        }


        size_t all_task_size() {
            return ready_task_queue.size()+suspend_tasks.size();
        }

        size_t suspend_task_size() {
            return suspend_tasks.size();
        }
        size_t ready_task_size() {
            return ready_task_queue.size();
        }

        [[nodiscard]] auto poller_sys_handle()const{
            return poller.get_sys_handle();
        }

        friend class event_loop;
    };

    inline thread_local worker_local_context current_worker_context;

    class event_loop {
        concurrent_list<task<void>> global_task_queue;
        int poller_sys_handle;
        std::atomic<bool>& stop_token;
        std::atomic<size_t> task_size{0};
        public:
            void stop(){
                global_task_queue.down();
            }
            static void wait_for_task(task<void> task){
                auto events_buffer = detail::events(1024);
                while (!task.is_ready()) {
                    int event_size = current_worker_context.get_poller().poll_events(events_buffer,std::chrono::milliseconds(100));
                    if(event_size>0){
                        task.resume();
                    }
                }
            }
            void print_task_size(){
                std::stringstream ss;
                ss<<std::this_thread::get_id();
                fmt::print("thread :{}size:{}\n",ss.str(), global_task_queue.size()+current_worker_context.all_task_size());
            }
            explicit event_loop(std::atomic<bool>& stop_token):stop_token(stop_token){}

            void set_context(worker_local_context* context){
                poller_sys_handle = context->poller_sys_handle();
            }

            int get_poller_sys_handle()const{
                return poller_sys_handle;
            }

            void operator()() {
                auto events_buffer = detail::events(8);
                while (!stop_token) {
                    //print_task_size();
                    //如果等待队列和就绪队列都是空的，代表线程没有任何任务可进行了
                    if(current_worker_context.all_task_size()==0){
                        //阻塞等待调度器下发任务
                        wait_for_global_dispatch();
                        //交换就绪队列和全局任务队列
                        global_task_queue.swap_inner(current_worker_context.ready_task_queue);
                    }
                    //尝试推进所有就绪任务
                    current_worker_context.poll_all_task();
                    //如果就绪队列为空，代表已经没有可以继续推进的任务了，再次尝试从全局任务队列中获取任务
                    if(current_worker_context.ready_task_queue.empty()&&!global_task_queue.empty()){
                        global_task_queue.swap_inner(current_worker_context.ready_task_queue);
                    }
                    //轮询事件
                    int event_size = current_worker_context.poller.poll_events(events_buffer,std::chrono::milliseconds(100));
                    //唤醒监听事件的任务，重新从等待队列加入就绪队列
                    wake_suspend_tasks(events_buffer, event_size);
                }
            }

            void wait_for_global_dispatch(){
                global_task_queue.wait_not_empty();
            }


            void wake_suspend_tasks(detail::events & events,int event_size){
                for(int i=0;i<event_size;i++){
                    if(events[i].filter != EVFILT_USER){
                        auto task_id =  (intptr_t)events[i].udata;
                        current_worker_context.wake_task(task_id);
                    }
                }
            }

            void add_task(task<void> task) {
                global_task_queue.emplace_back(std::move(task));
            }

            //only use at the same thread!!!
            static void add_local_task(task<void> task) {
                current_worker_context.add_ready_task(std::move(task));
            }

            //所有三个队列的任务总数
            size_t all_task_size() {
                return global_task_queue.size()+current_worker_context.all_task_size();
            }

        };


}
