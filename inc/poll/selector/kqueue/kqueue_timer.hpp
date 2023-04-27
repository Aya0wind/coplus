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
        int expire_time;
        selector& attached_selector;
        void register_event_impl(selector& selector, intptr_t task_id) const {
            selector.register_event(timer_fd, detail::Interest::TIMER, expire_time, (void*) task_id);
        }
        void deregister_event_impl(selector& selector) const {
            selector.deregister_event(timer_fd, detail::Interest::TIMER);
        }
        friend class detail::source_base<selector, kqueue_timer>;

    public:
        kqueue_timer(selector& selector, int expire_time, bool repeat = false) :
            timer_fd(static_cast<int>(id_generator::next_id())), attached_selector(selector) {
        }

        void set_expire_timeout(int expire) {
            this->expire_time = expire;
        }

        [[nodiscard]] detail::handle_type get_handle() const {
            return timer_fd;
        }
        kqueue_timer(const kqueue_timer&) = delete;
        kqueue_timer(kqueue_timer&& other) noexcept :
            timer_fd(other.timer_fd), attached_selector(other.attached_selector) {
            other.timer_fd = -1;
        }
    };
}// namespace coplus
