#pragma once

#include "components/id_generator.hpp"
#include "poll/event.hpp"
#include "poll/poller.hpp"
#include <chrono>
#include <cmath>
#include <cstdint>
#include <stdexcept>
namespace coplus {

    class iocp_timer : public detail::source_base<selector, iocp_timer> {
        handle_type timer_fd;
        token_type token;
        int expire_time{0};
        selector& attached_selector;
        void register_event_impl(selector& selector, token_type token) const {
            //selector.register_event(timer_fd, Interest::TIMER, expire_time, (void*) token);
        }
        void deregister_event_impl(selector& selector) const {
            //selector.deregister_event(timer_fd, detail::Interest::TIMER);
        }
        friend class detail::source_base<selector, iocp_timer>;

    public:
        iocp_timer(selector& selector, token_type token, bool repeat = false) :
            attached_selector(selector), token(token) {
        }

        void set_expire_timeout(int expire) {
            this->expire_time = expire;
        }

        [[nodiscard]] handle_type get_handle() const {
            return timer_fd;
        }

        token_type get_token() const {
            return token;
        }
        iocp_timer(const iocp_timer&) = delete;
        iocp_timer(iocp_timer&& other) noexcept :
            timer_fd(other.timer_fd), attached_selector(other.attached_selector), token(other.token) {
            //other.timer_fd = -1;
        }
        ~iocp_timer() {
            deregister_event(attached_selector);
        }
    };
}// namespace coplus
