//
// Created by junjian LI on 2023/4/18.
//

#pragma once
#include <cstdint>
namespace coplus {
    enum Interest : uint8_t {
        READABLE = 1,
        WRITEABLE = 2,
        AIO = 4,
        TIMER = 8,
        ALL = READABLE | WRITEABLE | AIO | TIMER
    };
    using token_type = uint64_t;
    namespace detail {
        template<class E>
        struct event_base {
            //crtp call derived class method
        public:
            [[nodiscard]] token_type get_token() const {
                return static_cast<const E*>(this)->get_token_impl();
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
    }// namespace detail
}// namespace coplus


#ifdef __linux__
#include <sys/epoll.h>
#include <vector>
namespace coplus::detail {
    using sys_event = struct epoll_event;
    using sys_events = ::std::vector<sys_event>;
    using handle_type = int;
    class epoll_event : public event_base<epoll_event> {
        detail::sys_event sys_event;
        friend class event_base<epoll_event>;
        [[nodiscard]] token_type get_token_impl() const {
            return static_cast<token_type>(sys_event.data.u64);
        }
        [[nodiscard]] bool is_readable_impl() const {
            return sys_event.events == EPOLLIN;
        }
        [[nodiscard]] bool is_writeable_impl() const {
            return sys_event.events == EPOLLOUT;
        }
        [[nodiscard]] bool is_aio_impl() const {
            return false;
        }
        [[nodiscard]] bool is_timer_impl() const {
            return sys_event.events == EPOLLIN;
        }
        [[nodiscard]] bool is_error_impl() const {
            return sys_event.events == EPOLLERR;
        }

        [[nodiscard]] bool is_read_closed_impl() const {
            return sys_event.events == EPOLLHUP;
        }
        [[nodiscard]] bool is_write_closed_impl() const {
            return sys_event.events == EPOLLHUP;
        }
        [[nodiscard]] bool is_priority_impl() const {
            return sys_event.events == EPOLLPRI;
        }

    public:
        explicit epoll_event(detail::sys_event e) :
            sys_event(e){};
        explicit operator detail::sys_event&() {
            return reinterpret_cast<detail::sys_event&>(*this);
        }
        epoll_event() = default;
    };
}// namespace coplus::detail
namespace coplus {
    using event = detail::epoll_event;
    using events = ::std::vector<event>;
}// namespace coplus
#elif __APPLE__
#include <sys/event.h>
#include <vector>
namespace coplus {
    using handle_type = int;
}
namespace coplus::detail {
    using sys_event = struct kevent;
    using sys_events = ::std::vector<sys_event>;

    class kqueue_event : public event_base<kqueue_event> {
        detail::sys_event sys_event;
        friend class event_base<kqueue_event>;
        [[nodiscard]] token_type get_token_impl() const {
            return reinterpret_cast<token_type>(sys_event.udata);
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
            return sys_event.filter == EVFILT_READ && sys_event.flags == EV_EOF;
        }
        [[nodiscard]] bool is_write_closed_impl() const {
            return sys_event.filter == EVFILT_WRITE && sys_event.flags == EV_EOF;
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
#elif _WIN32 || _WIN64
#pragma  comment(lib, "ws2_32.lib")
#pragma  comment(lib, "kernel32.lib")
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <vector>
namespace coplus{
    using handle_type = HANDLE;
    enum IO_EVENT
    {
        IO_ACCEPT,
        IO_RECV,
        IO_SEND
    };
    struct IOContext {
        IOContext(char* buffer,ULONG buffer_size, IO_EVENT type, SOCKET tcp_stream) : type(type), tcp_stream(tcp_stream), wsaBuf{static_cast<ULONG>(buffer_size), buffer}
        {}
        OVERLAPPED overlapped{};
        WSABUF wsaBuf;
        IO_EVENT type;
        SOCKET tcp_stream = INVALID_SOCKET;
        DWORD nBytes = 0;
        ULONG flags = 0;
    };
}

namespace coplus::detail {
    using sys_event = IOContext;
    using sys_events = ::std::vector<sys_event>;
    class iocp_event : public event_base<iocp_event> {
        detail::sys_event sys_event;

        friend class event_base<iocp_event>;
        [[nodiscard]] token_type get_token_impl() const {
            return (token_type)(sys_event.tcp_stream);
        }
        [[nodiscard]] bool is_readable_impl() const {
            return sys_event.type == IO_EVENT::IO_RECV;
        }
        [[nodiscard]] bool is_writeable_impl() const {
            return sys_event.type == IO_EVENT::IO_SEND;
        }
        [[nodiscard]] bool is_aio_impl() const {
            return false;
        }
        [[nodiscard]] bool is_timer_impl() const {
            return false;
        }
        [[nodiscard]] bool is_error_impl() const {
            return false;
        }

        [[nodiscard]] bool is_read_closed_impl() const {
            return sys_event.type==IO_EVENT::IO_RECV&&sys_event.nBytes==0;
        }
        [[nodiscard]] bool is_write_closed_impl() const {
            return sys_event.type==IO_EVENT::IO_SEND&&sys_event.nBytes==0;
        }
        [[nodiscard]] bool is_priority_impl() const {
            return false;
        }

    public:
        explicit iocp_event(detail::sys_event e) :
            sys_event(e){};
        explicit operator detail::sys_event&() {
            return reinterpret_cast<detail::sys_event&>(*this);
        }
        iocp_event() = default;
    };

}// namespace coplus::detail
namespace coplus {
    using event = detail::iocp_event;
    using events = ::std::vector<event>;
}// namespace coplus
#endif
