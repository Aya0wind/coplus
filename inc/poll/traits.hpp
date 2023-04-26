//
// Created by junjian LI on 2023/4/26.
//

#pragma once
#include "event.hpp"
#include <chrono>
#include <cstdint>
namespace coplus::detail {
    enum Interest : uint8_t {
        READABLE = 1,
        WRITEABLE = 2,
        AIO = 4,
        TIMER = 8,
        ALL = READABLE | WRITEABLE | AIO | TIMER
    };
    using handle_type = int;

    template<class Selector>
    concept SelectorTrait = requires(Selector s) {
        {
            s.select(std::declval<sys_events&>(), std::declval<::std::chrono::milliseconds>())
            } -> std::same_as<int>;
        {
            s.register_event(std::declval<handle_type>(), std::declval<Interest>(), std::declval<int>(), std::declval<void*>())
            } -> std::same_as<void>;
        {
            s.deregister_event(std::declval<handle_type>(), std::declval<Interest>())
            } -> std::same_as<void>;
        {
            s.wake(std::declval<handle_type>())
            } -> std::same_as<int>;
        {
            s.get_handle()
            } -> std::same_as<handle_type>;
    };

    template<class T, class S>
    concept SourceTrait = requires(T t) {
        {
            t.register_event(std::declval<S&>(), std::declval<intptr_t>())
            } -> std::same_as<void>;
        {
            t.deregister_event(std::declval<S&>())
            } -> std::same_as<void>;
        {
            t.get_handle()
            } -> std::same_as<handle_type>;
    }
    &&SelectorTrait<S>;

    template<class SelectorImpl>
    class selector_base {
    public:
        selector_base() = default;
        [[gnu::always_inline]] [[nodiscard]] int get_handle() const {
            return static_cast<const SelectorImpl*>(this)->get_handle_impl();
        }
        [[gnu::always_inline]] int wake(handle_type sys_handle) {
            return static_cast<const SelectorImpl*>(this)->wake_impl(sys_handle);
        }
        [[gnu::always_inline]] int select(::std::vector<sys_event>& events, ::std::chrono::milliseconds timeout) const {
            return static_cast<const SelectorImpl*>(this)->select_impl(events, timeout);
        }
        [[gnu::always_inline]] void register_event(handle_type file_handle, Interest interest, int data, void* udata) const {
            return static_cast<const SelectorImpl*>(this)->register_event_impl(file_handle, interest, data, udata);
        }
        [[gnu::always_inline]] void deregister_event(handle_type file_handle, Interest interest) const {
            return static_cast<const SelectorImpl*>(this)->deregister_event_impl(file_handle, interest);
        }
    };

    template<SelectorTrait Selector, class SourceImpl>
    class source_base {
    public:
        source_base() = default;
        [[gnu::always_inline]] void register_event(Selector& s, intptr_t taskid) {
            return static_cast<SourceImpl*>(this)->register_event_impl(s, taskid);
        }
        [[gnu::always_inline]] void deregister_event(Selector& s) {
            return static_cast<SourceImpl*>(this)->deregister_event_impl(s);
        }
    };


}// namespace coplus::detail
