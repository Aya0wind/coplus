#pragma once

#include "components/id_generator.hpp"
#include "poll/poller.hpp"
#include <chrono>
#include <cmath>
#include <cstdint>
#include <stdexcept>
namespace coplus {

    class kqueue_timer : public detail::source_base<selector, kqueue_timer> {
        int timer_fd;
        int expire_times;
        void register_event_impl(selector& selector, intptr_t task_id) const {
            selector.register_event(timer_fd, detail::Interest::TIMER, expire_times, (void*) task_id);
        }
        void deregister_event_impl(selector& selector) const {
            selector.deregister_event(timer_fd, detail::Interest::TIMER);
        }
        friend class detail::source_base<selector, kqueue_timer>;

    public:
        template<class Duration, class Period>
        kqueue_timer(std::chrono::duration<Duration, Period> timeout, bool repeat) :
            timer_fd(static_cast<int>(id_generator::next_id())),
            expire_times(static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count())) {
        }

        [[nodiscard]] detail::handle_type get_handle() const {
            return timer_fd;
        }
    };
}// namespace coplus
