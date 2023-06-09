//
// Created by junjian LI on 2023/4/18.
//

#pragma once
#include "coplus/poll/traits.hpp"
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
            sys_event sys_event{.events = 0, .data = data};
            if (interest & Interest::READABLE) {
                sys_event.events |= EPOLLIN;
            }
            if (interest & Interest::WRITEABLE) {
                sys_event.events |= EPOLLOUT;
            }
            if (et_mode) {
                sys_event.events |= EPOLLET;
            }
            // sys_event.events |= EPOLLONESHOT;
            sys_event.events |= EPOLLRDHUP;
            sys_event.events |= EPOLLHUP;
            return epoll_ctl(this->epoll_fd, op, fd, &sys_event);
        }

        [[nodiscard]] int get_handle_impl() const {
            return epoll_fd;
        }
        static int wake_impl(handle_type sys_handle) {
            //send the stop signal to the selector
            // uint64_t one = 1;
            // return write(sys_handle, &one, sizeof(one));
            return 0;
        }


        int select_impl(events& sys_events, ::std::chrono::milliseconds timeout) const {
            return epoll_wait(this->epoll_fd, (::epoll_event*) (sys_events.data()), sys_events.size(), timeout.count());
        }

        void register_event_impl(handle_type file_handle, Interest interest, int data, void* udata) const {
            epoll_event_register(file_handle, interest, EPOLL_CTL_ADD, udata, true);
        }

        void deregister_event_impl(handle_type file_handle, Interest interest) const {
            epoll_event_register(file_handle, interest, EPOLL_CTL_DEL, nullptr, true);
        }

    public:
        epoll_selector() :
            epoll_fd(epoll_create(256)), wake_event_fd(eventfd(1, EFD_NONBLOCK)) {
            //register eventfd sys_event
            //epoll_event_register(wake_event_fd, Interest::READABLE, EPOLL_CTL_ADD, nullptr, false);
        }
        ~epoll_selector() {
            close(epoll_fd);
            close(wake_event_fd);
        }
    };
}// namespace coplus::detail
