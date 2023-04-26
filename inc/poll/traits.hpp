//
// Created by junjian LI on 2023/4/26.
//

#pragma once
#include "event.hpp"
namespace coplus::detail{
    enum Interest : uint8_t {
        READABLE = 1,
        WRITEABLE = 2,
        AIO = 4,
        TIMER = 8,
        ALL = READABLE | WRITEABLE | AIO | TIMER
    };
    using token_type = intptr_t;
    using handle_type = int;

    template<class Selector>
    concept SelectorTrait = requires(Selector s) {
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

    template<class T,class S>
    concept SourceTrait = requires(T t) {
        {
            t.register_event(std::declval<S&>(), std::declval<token_type>(), std::declval<Interest>(), std::declval<intptr_t>())
        } -> std::same_as<void>;
        {
            t.deregister_event(std::declval<S&>(), std::declval<token_type>(), std::declval<Interest>())
        } -> std::same_as<void>;
    }&&SelectorTrait<S>;
}