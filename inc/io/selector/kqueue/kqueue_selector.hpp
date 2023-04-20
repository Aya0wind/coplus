//
// Created by junjian LI on 2023/4/18.
//

#pragma once
#include <ctime>
#include <iostream>
#include <sys/event.h>
#include <unistd.h>
#include "io/event.hpp"
#include "kqueue_event.hpp"

namespace coplus::detail {
    class kqueue_selector {
        int kqueue_fd;
        events changes;
        void add_event(uintptr_t fd, int filter, int flags, int fflags, int data, void *udata)
        {
            event ev{};
            EV_SET(&ev, fd, filter, flags, fflags, data, udata);
            changes.push_back(ev);
        }
    public:
        [[nodiscard]] int get_fd() const{
            return kqueue_fd;
        }

        static void stop_wait_signal(int sys_handle) {
            //send the stop signal to the selector
            //event ev{};
            //EV_SET(&ev,sys_handle, EVFILT_USER, 0, NOTE_TRIGGER, 0, nullptr);
            //kevent(sys_handle, &ev, 1, nullptr, 0, nullptr);
        }

        kqueue_selector() : kqueue_fd(kqueue()) {
            event ev{};
            EV_SET(&ev, kqueue_fd, EVFILT_USER, EV_ADD|EV_CLEAR, NOTE_TRIGGER, 0, nullptr);
            kevent(kqueue_fd, &ev, 1, nullptr, 0, nullptr);
        }
        int select(::std::vector<event>& events, ::std::chrono::milliseconds timeout)
        {
            struct timespec sys_timeout{};
            sys_timeout.tv_sec = 0;
            sys_timeout.tv_nsec = 1000;
            return kevent(kqueue_fd,
                          changes.data(),
                          changes.size(),
                          reinterpret_cast<event*>(events.data()),
                          events.size(),
                          nullptr);// 已经就绪的文件描述符数量
        }

       void register_event(uintptr_t fd, Interest interest,int data,void* udata)
       {
           int sys_interest = 0;
           if (interest & Interest::READABLE) {
               sys_interest |= EVFILT_READ;
           }
           if (interest & Interest::WRITEABLE) {
               sys_interest |= EVFILT_WRITE;
           }
           if (interest & Interest::AIO) {
               sys_interest |= EVFILT_AIO;
           }
           if(interest & Interest::TIMER){
               sys_interest |= EVFILT_TIMER;
           }
           add_event(fd, sys_interest, EV_ADD, 0, data, udata);
       }
    };
}// namespace coplus::detail
