//
// Created by junjian LI on 2022/9/7.
//

#pragma once
#include <chrono>
#include <cstdint>
#include <unistd.h>
#include "context/components/id_generator.hpp"
#include "context/runtime.hpp"
#include "context/worker_local_context.hpp"
#include "io/event.hpp"
#include "io/poll.hpp"

namespace coplus {

//extern co_runtime global_runtime;
   struct DelayAwaiter{
        time_t expire_time;
        inline static std::atomic<intptr_t> id{0};
        intptr_t timer_id;
        template<class duration_type,class period>
       explicit DelayAwaiter(std::chrono::duration<duration_type,period> duration)
            :expire_time(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count()),timer_id(id++){}

       template<class duration_type,class period>
       static DelayAwaiter delay(std::chrono::duration<duration_type,period> duration) {
           return DelayAwaiter(duration);
       }

       DelayAwaiter() = delete;
       DelayAwaiter(const DelayAwaiter&) = delete;
       DelayAwaiter(DelayAwaiter&&others) :expire_time(others.expire_time),timer_id(others.timer_id) {}

       void register_event(registry& registry, detail::token_type token, detail::Interest interest,intptr_t task_id) const
       {
           registry.get_selector().register_event(
                   static_cast<uintptr_t>(timer_id),
                   interest,
                   static_cast<int>(expire_time),
                   (void *)task_id);
       }

       void deregister_event(registry& registry, detail::token_type token, detail::Interest interest,intptr_t task_id) const
       {
           registry.get_selector().register_event(
                   static_cast<uintptr_t>(timer_id),
                   interest,
                   static_cast<int>(expire_time),
                   (void *)task_id);
       }

       bool await_ready() {
           //bool r = start_duration.count() + timer.expire_time<=std::chrono::system_clock::now().time_since_epoch().count();
           return false;
       }

       void await_suspend(auto handle)
       {
          current_worker_context
                   .get_poller()
                   .register_event(*this,
                                   this->timer_id,
                                   detail::Interest::TIMER,
                                   current_worker_context.get_current_task_id()
                                   );
       }

       void await_resume() {

       }
   private:
       bool repeat{false};
   };


}
