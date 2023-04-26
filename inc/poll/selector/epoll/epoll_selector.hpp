//
// Created by junjian LI on 2023/4/18.
//

#pragma once
#include "../../traits.hpp"
#include "poll/event.hpp"
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/types.h>
#include <unistd.h>
namespace coplus::detail {
    class epoll_selector : public selector_base<epoll_selector> {
        friend class selector_base;
        int epoll_fd;
        int wake_event_fd;
        int epoll_event_register(handle_type fd, Interest interest, int op, void* udata, bool et_mode) const {
            epoll_data data;
            data.ptr = udata;
            event event{.events = 0, .data = data};
            if (interest & Interest::READABLE) {
                event.events |= EPOLLIN;
            }
            if (interest & Interest::WRITEABLE) {
                event.events |= EPOLLOUT;
            }
            if (et_mode) {
                event.events |= EPOLLET;
            }
            return epoll_ctl(this->epoll_fd, op, fd, &event);
        }

        [[nodiscard]] int get_handle_impl() const {
            return epoll_fd;
        }
        static int wake_impl(handle_type sys_handle) {
            //send the stop signal to the selector
            uint64_t one = 1;
            return write(sys_handle, &one, sizeof(one));
        }


        int select_impl(::std::vector<event>& events, ::std::chrono::milliseconds timeout) const {
            return epoll_wait(this->epoll_fd, events.data(), events.size(), timeout.count());
        }

        void register_event_impl(handle_type file_handle, Interest interest, int data, void* udata) const {
            epoll_event_register(file_handle, interest, EPOLL_CTL_ADD, udata, false);
        }

        void deregister_event_impl(handle_type file_handle, Interest interest) const {
            epoll_event_register(file_handle, interest, EPOLL_CTL_DEL, nullptr, false);
        }

    public:
        epoll_selector() :
            epoll_fd(epoll_create(256)), wake_event_fd(eventfd(1, EFD_NONBLOCK)) {
            //register eventfd event
            epoll_event_register(wake_event_fd, Interest::READABLE, EPOLL_CTL_ADD, nullptr, false);
        }
    };
}// namespace coplus::detail
