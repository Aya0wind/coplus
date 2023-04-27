//
// Created by junjian LI on 2023/4/18.
//

#pragma once
#include "../../traits.hpp"
#include "poll/event.hpp"
#include <ctime>
#include <iostream>
#include <sys/event.h>
#include <sys/fcntl.h>
#include <unistd.h>
namespace coplus::detail {

    class kqueue_selector : public selector_base<kqueue_selector> {
        friend class selector_base;
        int kqueue_fd;
        int kevent_register(sys_event* changes, int nchanges, sys_event* events, int nevents, const timespec* timeout) const {
            return kevent(this->kqueue_fd, changes, nchanges, events, nevents, timeout);
        }


        [[nodiscard]] int get_handle_impl() const {
            return kqueue_fd;
        }
        static int wake_impl(handle_type sys_handle) {
            //send the stop signal to the selector
            sys_event ev{};
            EV_SET(&ev, 0, EVFILT_USER, EV_ADD | EV_RECEIPT | EV_CLEAR, NOTE_TRIGGER, 0, nullptr);
            return kevent(sys_handle, &ev, 1, &ev, 1, nullptr);
        }


        int select_impl(events& events, ::std::chrono::milliseconds timeout) const {
            struct timespec sys_timeout {};
            sys_timeout.tv_sec = 0;
            sys_timeout.tv_nsec = (timeout.count() % 1000) * 1000000;
            return kevent_register(nullptr, 0, (struct kevent*) events.data(), static_cast<int>(events.size()), nullptr);// 已经就绪的文件描述符数量
        }

        void register_event_impl(handle_type file_handle, Interest interest, int data, void* udata) const {
            sys_event ev[ 4 ];
            int changes_index = 0;
            int flags =  EV_ADD | EV_CLEAR | EV_RECEIPT;
            if (interest & Interest::READABLE) {
                EV_SET(&ev[ changes_index ], file_handle, EVFILT_READ, flags, 0, data, udata);
                changes_index += 1;
            }
            if (interest & Interest::WRITEABLE) {
                EV_SET(&ev[ changes_index ], file_handle, EVFILT_WRITE, flags, 0, data, udata);
                changes_index += 1;
            }
            if (interest & Interest::AIO) {
                EV_SET(&ev[ changes_index ], file_handle, EVFILT_AIO, flags, 0, data, udata);
                changes_index += 1;
            }
            if (interest & Interest::TIMER) {
                EV_SET(&ev[ changes_index ], file_handle, EVFILT_TIMER, flags, 0, data, udata);
                changes_index += 1;
            }
            kevent_register(ev, changes_index, ev, changes_index, nullptr);
        }

        void deregister_event_impl(handle_type file_handle, Interest interest) const {
            int flags = EV_DELETE | EV_CLEAR;
            sys_event ev[ 4 ];
            int changes_index = 0;
            if (interest & Interest::READABLE) {
                EV_SET(&ev[ changes_index ], file_handle, EVFILT_READ, flags, 0, 0, nullptr);
                changes_index += 1;
            }
            if (interest & Interest::WRITEABLE) {
                EV_SET(&ev[ changes_index ], file_handle, EVFILT_WRITE, flags, 0, 0, nullptr);
                changes_index += 1;
            }
            if (interest & Interest::AIO) {
                EV_SET(&ev[ changes_index ], file_handle, EVFILT_AIO, flags, 0, 0, nullptr);
                changes_index += 1;
            }
            if (interest & Interest::TIMER) {
                EV_SET(&ev[ changes_index ], file_handle, EVFILT_TIMER, flags, 0, 0, nullptr);
                changes_index += 1;
            }
            kevent_register(ev, changes_index, ev, changes_index, nullptr);
        }

    public:
        kqueue_selector() :
            kqueue_fd(kqueue()) {
            //fcntl(kqueue_fd, F_SETFD, FD_CLOEXEC);
        }
        ~kqueue_selector() {
            close(kqueue_fd);
        }
    };
}// namespace coplus::detail
