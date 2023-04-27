//
// Created by junjian LI on 2023/4/18.
//

#pragma once
#include "./promise.hpp"
namespace coplus {
    template<class return_type>
    using promise = detail::promise<return_type>;
    template<class return_type>
    using co_handle = std::coroutine_handle<promise<return_type>>;
    namespace detail {
        template<class return_type = void>
        struct awaiter_base {
            using promise_type = promise<return_type>;
            co_handle<promise<return_type>> handle;
            explicit awaiter_base(std::coroutine_handle<promise_type> current) noexcept
                :
                handle(current) {
            }

            [[nodiscard]] inline bool await_ready() const noexcept {
                return !handle || handle.done();
            }

            std::coroutine_handle<>
            await_suspend(std::coroutine_handle<> awaiting_coro) noexcept {
                handle.promise().set_continuation(awaiting_coro);
                return handle;
            }

            void await_resume() {
            }
        };

        template<>
        struct awaiter_base<void> {
            using promise_type = promise<>;
            co_handle<void> handle;
            explicit awaiter_base(std::coroutine_handle<promise_type> current) noexcept
                :
                handle(current) {
            }

            inline bool await_ready() const noexcept {
                return !handle || handle.done();
            }

            std::coroutine_handle<>
            await_suspend(std::coroutine_handle<> awaiting_coro) const noexcept {
                handle.promise().set_continuation(awaiting_coro);
                return handle;
            }

            void await_resume() {
            }
        };
    }// namespace detail

}// namespace coplus
