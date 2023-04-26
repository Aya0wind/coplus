//
// Created by junjian LI on 2023/4/18.
//

#pragma once
#include "poll/event.hpp"
#include <ctime>
#include <iostream>
#include <sys/event.h>
#include <sys/fcntl.h>
#include <unistd.h>

namespace coplus::detail {

    template<class Selector>
    class selector_base{
    public:
        selector_base() = default;
        [[gnu::always_inline]]
        [[nodiscard]] int get_handle() const {
            return static_cast<const Selector*>(this)->get_handle_impl();
        }
        [[gnu::always_inline]]
        int wake(handle_type sys_handle) {
            return static_cast<const Selector*>(this)->wake_impl(sys_handle);
        }
        [[gnu::always_inline]]
        int select(::std::vector<event>& events, ::std::chrono::milliseconds timeout) const {
            return static_cast<const Selector*>(this)->select_impl(events,timeout);
        }
        [[gnu::always_inline]]
        void register_event(handle_type file_handle, Interest interest, int data, void* udata) const {
            return static_cast<const Selector*>(this)->register_event_impl(file_handle,interest,data,udata);
        }
        [[gnu::always_inline]]
        void deregister_event(handle_type file_handle, Interest interest) const {
            return static_cast<const Selector*>(this)->deregister_event_impl(file_handle,interest);
        }
    };




    class kqueue_selector: public selector_base<kqueue_selector>{
        friend class selector_base;
        int kqueue_fd;
        int kevent_register(event* changes, int nchanges, event* events, int nevents, const timespec* timeout) const {
            return kevent(this->kqueue_fd, changes, nchanges, events, nevents, timeout);
        }


        [[nodiscard]] int get_handle_impl() const {
            return kqueue_fd;
        }
        static int wake_impl(handle_type sys_handle) {
            //send the stop signal to the selector
            event ev{};
            EV_SET(&ev, 0, EVFILT_USER, EV_ADD | EV_RECEIPT | EV_CLEAR, NOTE_TRIGGER, 0, nullptr);
            return kevent(sys_handle, &ev, 1, &ev, 1, nullptr);
        }


        int select_impl(::std::vector<event>& events, ::std::chrono::milliseconds timeout) const {
            struct timespec sys_timeout {};
            sys_timeout.tv_sec = 0;
            sys_timeout.tv_nsec = 1000;
            return kevent_register(nullptr, 0, events.data(), static_cast<int>(events.size()), nullptr);// 已经就绪的文件描述符数量
        }

        void register_event_impl(handle_type file_handle, Interest interest, int data, void* udata) const {
            event ev[ 4 ];
            int changes_index = 0;
            auto sys_interest = (decltype(EVFILT_USER)) interest;
            int flags = EV_CLEAR | EV_RECEIPT | EV_ADD;
            if (sys_interest & Interest::READABLE) {
                EV_SET(&ev[ changes_index ], file_handle, EVFILT_READ, flags, 0, data, udata);
                changes_index += 1;
            }
            if (sys_interest & Interest::WRITEABLE) {
                EV_SET(&ev[ changes_index ], file_handle, EVFILT_WRITE, flags, 0, data, udata);
                changes_index += 1;
            }
            if (sys_interest & Interest::AIO) {
                EV_SET(&ev[ changes_index ], file_handle, EVFILT_AIO, flags, 0, data, udata);
                changes_index += 1;
            }
            if (sys_interest & Interest::TIMER) {
                EV_SET(&ev[ changes_index ], file_handle, EVFILT_TIMER, flags, 0, data, udata);
                changes_index += 1;
            }
            kevent_register(ev, changes_index, ev, changes_index, nullptr);
        }

        void deregister_event_impl(handle_type file_handle, Interest interest) const {
            auto flags = EV_DELETE | EV_RECEIPT;
            event ev[ 4 ];
            int changes_index = 0;
            auto sys_interest = (decltype(EVFILT_USER)) interest;
            if (interest & Interest::READABLE) {
                EV_SET(&ev[ 0 ], file_handle, EVFILT_READ, flags, 0, 0, nullptr);
                changes_index += 1;
            }
            if (interest & Interest::WRITEABLE) {
                EV_SET(&ev[ changes_index ], file_handle, EVFILT_READ, flags, 0, 0, nullptr);
                changes_index += 1;
            }
            if (interest & Interest::AIO) {
                EV_SET(&ev[ changes_index ], file_handle, EVFILT_READ, flags, 0, 0, nullptr);
                changes_index += 1;
            }
            if (interest & Interest::TIMER) {
                EV_SET(&ev[ changes_index ], file_handle, EVFILT_READ, flags, 0, 0, nullptr);
                changes_index += 1;
            }
            kevent_register(ev, changes_index, ev, changes_index, nullptr);

        }
    public:
        kqueue_selector()
            : kqueue_fd(kqueue()) {
            fcntl(kqueue_fd, F_SETFD, FD_CLOEXEC);
        }
    };
}// namespace coplus::detail
