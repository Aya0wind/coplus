//
// Created by junjian LI on 2022/9/7.
//

#pragma once

#include "components/id_generator.hpp"
#include "default_awaiter.hpp"
#include <cassert>
#include <cstdint>
#include <fmt/format.h>
#include <functional>
#include <sstream>
namespace coplus {
    void print_thread_id(std::string thread) {
        std::stringstream ss;
        ss << std::this_thread::get_id();
        fmt::print("thread:{}, id: {}\n", thread, ss.str());
    }
    template<class return_type>
    struct task {
        using promise_type = promise<return_type>;

        explicit task(co_handle<promise_type> handle) :
            handle(handle), id(id_generator::next_id()) {
        }
        task(task const&) = delete;
        task() :
            handle(nullptr), id(id_generator::next_id()) {
        }
        task(task&& other) noexcept
            :
            handle(other.handle),
            id(other.id) {
            other.handle = nullptr;
        }
        ~task() {
            if (handle) {
                handle.destroy();
            }
        }
        void resume() {
            handle.resume();
        }

        detail::promise_result_type<return_type>& getPromiseValueRef() {
            return handle.promise().promise_result;
        }

        return_type& getReturnValueRef() {
            assert(this->is_ready() && "coroutine is not ready");
            return std::get<0>(this->getPromiseValueRef());
        }
        return_type takeReturnValue() {
            assert(this->is_ready() && "coroutine is not ready");
            return std::get<0>(std::move(handle.promise().promise_result));
        }

        [[nodiscard]] bool is_ready() const noexcept {
            return !handle || handle.done();
        }

        [[nodiscard]] bool is_exception() const noexcept {
            return handle.promise().is_exception();
        }

        /**
         * @brief wait for the task<> to complete, and get the ref of the result
         */
        auto operator co_await() const& noexcept {
            struct awaiter : detail::awaiter_base<return_type> {
                using detail::awaiter_base<return_type>::awaiter_base;

                decltype(auto) await_resume() {
                    // if (!this->handle) [[unlikely]]
                    //     throw std::logic_error("broken_promise");
                    assert(this->handle && "broken_promise");

                    return std::get<0>(this->getPromiseValue());
                }
            };

            return awaiter{handle};
        }

        /**
         * @brief wait for the task<> to complete, and get the rvalue ref of the
         * result
         */
        auto operator co_await() const&& noexcept {
            struct awaiter : detail::awaiter_base<return_type> {
                using detail::awaiter_base<return_type>::awaiter_base;

                decltype(auto) await_resume() {
                    // if (!this->handle) [[unlikely]]
                    //     throw std::logic_error("broken_promise");
                    assert(this->handle && "broken_promise");
                    return std::get<0>(this->handle.promise().promise_result);
                }
            };

            return awaiter{handle};
        }

        /**
         * @brief wait for the task<> to complete, but do not get the result
         */
        auto when_ready() const noexcept {
            struct awaiter : detail::awaiter_base<return_type> {
                using detail::awaiter_base<return_type>::awaiter_base;

                constexpr void await_resume() const noexcept {
                }
            };

            return awaiter{handle};
        }
        task<return_type>& operator=(const task<return_type>&) = delete;
        task<return_type>& operator=(task<return_type>&& s) noexcept {
            handle = s.handle;
            id = s.id;
            s.handle = nullptr;
            return *this;
        }
        [[nodiscard]] intptr_t get_id() const {
            return id;
        }

    private:
        co_handle<promise_type> handle;
        intptr_t id;
    };
    template<>
    struct task<void> {
        using promise_type = detail::promise<>;
        using co_handle = std::coroutine_handle<promise_type>;
        task() :
            handle(nullptr), id(id_generator::next_id()) {
        }
        task(task const&) = delete;
        explicit task(co_handle handle) :
            handle(handle), id(id_generator::next_id()) {
        }
        task(task&& other) noexcept
            :
            handle(other.handle),
            id(other.id) {
            other.handle = nullptr;
        }
        void resume() {
            handle.resume();
        }
        ~task() {
            if (handle) {
                handle.destroy();
            }
        }
        bool is_ready() const noexcept {
            return !handle || handle.done();
        }

        bool is_exception() const noexcept {
            return handle.promise().is_exception();
        }
        /**
         * @brief wait for the task<> to complete, and get the ref of the result
         */
        auto operator co_await() const& noexcept {
            struct awaiter : detail::awaiter_base<> {
                using detail::awaiter_base<>::awaiter_base;

                void await_resume() {
                    assert(this->handle && "broken_promise");
                }
            };
            return awaiter{handle};
        }

        /**
         * @brief wait for the task<> to complete, and get the rvalue ref of the
         * result
         */
        auto operator co_await() const&& noexcept {
            struct awaiter : detail::awaiter_base<> {
                using detail::awaiter_base<>::awaiter_base;

                void await_resume() {
                    assert(this->handle && "broken_promise");
                }
            };

            return awaiter{handle};
        }

        /**
         * @brief wait for the task<> to complete, but do not get the result
         */
        auto when_ready() const noexcept {
            struct awaiter : detail::awaiter_base<> {
                using detail::awaiter_base<>::awaiter_base;

                constexpr void await_resume() const noexcept {
                }
            };
            return awaiter{handle};
        }
        task<>& operator=(const task<>&) = delete;
        task<>& operator=(task<>&& s) noexcept {
            handle = s.handle;
            id = s.id;
            s.handle = nullptr;
            return *this;
        }
        co_handle get_handle() {
            return handle;
        }
        [[nodiscard]] intptr_t get_id() const {
            return id;
        }

        [[nodiscard]] std::exception_ptr get_exception() const noexcept {
            return handle.promise().get_exception();
        }

    private:
        co_handle handle;
        intptr_t id;
    };
    namespace detail {
        template<class T>
        inline task<T> promise<T>::get_return_object() noexcept {
            return task<T>{std::coroutine_handle<promise<T>>::from_promise(*this)};
        }

        inline task<> promise<>::get_return_object() noexcept {
            return task<>{std::coroutine_handle<promise<>>::from_promise(*this)};
        }

    }// namespace detail
}// namespace coplus
