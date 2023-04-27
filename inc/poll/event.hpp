//
// Created by junjian LI on 2023/4/18.
//

#pragma once
namespace coplus::detail {
    enum Interest : uint8_t {
        READABLE = 1,
        WRITEABLE = 2,
        AIO = 4,
        TIMER = 8,
        ALL = READABLE | WRITEABLE | AIO | TIMER
    };
    template<class E>
    struct event_base {
        //crtp call derived class method
    public:
        [[nodiscard]] intptr_t get_task_id() const {
            return static_cast<const E*>(this)->get_task_id_impl();
        }
        [[nodiscard]] bool is_readable() const {
            return static_cast<const E*>(this)->is_readable_impl();
        }
        [[nodiscard]] bool is_writeable() const {
            return static_cast<const E*>(this)->is_writeable_impl();
        }
        [[nodiscard]] bool is_aio() const {
            return static_cast<const E*>(this)->is_aio_impl();
        }
        [[nodiscard]] bool is_timer() const {
            return static_cast<const E*>(this)->is_timer_impl();
        }
        [[nodiscard]] bool is_error() const {
            return static_cast<const E*>(this)->is_error_impl();
        }
        [[nodiscard]] bool is_read_closed() const {
            return static_cast<const E*>(this)->is_read_closed_impl();
        }
        [[nodiscard]] bool is_write_closed() const {
            return static_cast<const E*>(this)->is_write_closed_impl();
        }
        [[nodiscard]] bool is_priority() const {
            return static_cast<const E*>(this)->is_priority_impl();
        }
    };

}// namespace coplus::detail


#ifdef __linux__
#include <sys/epoll.h>
#include <vector>
namespace coplus::detail {
    using sys_event = struct epoll_event;
    using sys_events = ::std::vector<sys_event>;
}// namespace coplus::detail
#elif __APPLE__
#include <sys/event.h>
#include <vector>
namespace coplus::detail {
    using sys_event = struct kevent;
    using sys_events = ::std::vector<sys_event>;
    using handle_type = int;

    class kqueue_event : public event_base<kqueue_event> {
        detail::sys_event sys_event;
        friend class event_base<kqueue_event>;
        [[nodiscard]] intptr_t get_task_id_impl() const {
            return reinterpret_cast<intptr_t>(sys_event.udata);
        }
        [[nodiscard]] bool is_readable_impl() const {
            return sys_event.filter == EVFILT_READ || sys_event.filter == EVFILT_USER;
        }
        [[nodiscard]] bool is_writeable_impl() const {
            return sys_event.filter == EVFILT_WRITE;
        }
        [[nodiscard]] bool is_aio_impl() const {
            return sys_event.filter == EVFILT_AIO;
        }
        [[nodiscard]] bool is_timer_impl() const {
            return sys_event.filter == EVFILT_TIMER;
        }
        [[nodiscard]] bool is_error_impl() const {
            return sys_event.flags == EV_ERROR || sys_event.flags == EV_EOF && sys_event.fflags != 0;
        }

        [[nodiscard]] bool is_read_closed_impl() const {
            return sys_event.filter == EVFILT_READ&&sys_event.flags == EV_EOF;
        }
        [[nodiscard]] bool is_write_closed_impl() const {
            return sys_event.filter == EVFILT_WRITE&& sys_event.flags == EV_EOF;
        }
        [[nodiscard]] bool is_priority_impl() const {
            return false;
        }

    public:
        explicit kqueue_event(detail::sys_event e) :
            sys_event(e){};
        explicit operator detail::sys_event&() {
            return reinterpret_cast<detail::sys_event&>(*this);
        }
        kqueue_event() = default;
    };

}// namespace coplus::detail
namespace coplus {
    using event = detail::kqueue_event;
    using events = ::std::vector<event>;
}// namespace coplus
#elif _WIN32
#endif
