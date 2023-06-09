//
// Created by junjian LI on 2023/4/18.
//

#pragma once

#include "coplus/poll/event.hpp"
#include "coplus/poll/traits.hpp"

#ifdef __linux__
#include "coplus/poll/selector/epoll/epoll_selector.hpp"
using selector = coplus::detail::epoll_selector;
#elif __APPLE__
#include "coplus/poll/selector/kqueue/kqueue_selector.hpp"
using selector = ::coplus::detail::kqueue_selector;
#elif _WIN32
#include "coplus/poll/selector/iocp/iocp_selector.hpp"
using selector = coplus::detail::iocp_selector;
#endif


namespace coplus::detail {

    class poller {
        selector inner_selector;

    public:
        poller() = default;
        poller(const poller&) = delete;
        poller(poller&&) = delete;
        template<class duration_type, class period>
        int poll_events(events& events, ::std::chrono::duration<duration_type, period> timeout) {
            return inner_selector.select(events, timeout);
        }
        void wake_poller() {
            auto handle = inner_selector.get_handle();
            inner_selector.wake(handle);
        }

        [[nodiscard]] const selector& get_selector() const {
            return inner_selector;
        }

        [[nodiscard]] selector& get_selector() {
            return inner_selector;
        }

        template<SourceTrait<selector> source_type>
        void register_event(source_type& source, token_type token) {
            source.register_event(inner_selector, token);
        }

        template<SourceTrait<selector> source_type>
        void deregister_event(source_type& source) {
            source.deregister_event(inner_selector);
        }
    };


}// namespace coplus::detail
