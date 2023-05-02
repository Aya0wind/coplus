//
// Created by junjian LI on 2023/4/26.
//

#pragma once
#include "coplus/poll/event.hpp"
#include <chrono>
#include <cstdint>
namespace coplus::detail {

    template<class selector_type>
    concept SelectorTrait = requires(selector_type s) {
        {
            s.select(std::declval<events&>(), std::declval<::std::chrono::milliseconds>())
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
            t.register_event(std::declval<S&>(), std::declval<token_type>())
        } -> std::same_as<void>;
        {
            t.deregister_event(std::declval<S&>())
        } -> std::same_as<void>;
        {
            t.get_handle()
        } -> std::same_as<handle_type>;
    } && SelectorTrait<S>;

    template<class selector_type>
    class selector_base {
    public:
        selector_base() = default;
        [[gnu::always_inline]] [[nodiscard]] handle_type get_handle() const {
            return static_cast<const selector_type*>(this)->get_handle_impl();
        }
        [[gnu::always_inline]] int wake(handle_type sys_handle) {
            return static_cast<const selector_type*>(this)->wake_impl(sys_handle);
        }
        [[gnu::always_inline]] int select(::std::vector<event>& events, ::std::chrono::milliseconds timeout) const {
            return static_cast<const selector_type*>(this)->select_impl(events, timeout);
        }
        [[gnu::always_inline]] void register_event(handle_type file_handle, Interest interest, int data, void* udata) const {
            return static_cast<const selector_type*>(this)->register_event_impl(file_handle, interest, data, udata);
        }
        [[gnu::always_inline]] void deregister_event(handle_type file_handle, Interest interest) const {
            return static_cast<const selector_type*>(this)->deregister_event_impl(file_handle, interest);
        }
    };

    template<SelectorTrait S, class Src>
    class source_base {
    public:
        source_base() = default;
        [[gnu::always_inline]] void register_event(S& s, token_type token) {
            static_cast<Src*>(this)->register_event_impl(s, token);
        }
        [[gnu::always_inline]] void deregister_event(S& s) {
            static_cast<Src*>(this)->deregister_event_impl(s);
        }
    };


}// namespace coplus::detail
