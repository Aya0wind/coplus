//
// Created by junjian LI on 2022/9/7.
//

#pragma once

#include <concepts>
#include <coroutine>
#include <cstdint>
#include <exception>
#include <utility>
#include <variant>

namespace coplus {
    template<class T = void>
    class task;
    namespace detail {
        //指示promise返回的类型，可能是异常或者返回值
        template<class T>
        using promise_result_type = std::variant<T, std::exception_ptr>;

        struct promise_base//协程必须带一个promise_type的内部类，可以用using或者typedef定义
        {
            /*
                协程初始化完调用此方法
                返回 stdsuspend_never{} 表示不切出协程（立即执行的意思）
                返回 stdsuspend_always{} 表示切出协程（不要立即执行）
                这两个类在 co_await 的时候解释
            */
            constexpr auto initial_suspend() {
                return std::suspend_always{};//惰性执行，创建时即挂起
            }
            //协程恢复后继续执行剩下的逻辑
            struct final_awaiter {
                constexpr bool await_ready() const noexcept {
                    return false;
                }

                template<std::derived_from<promise_base> promise_type>
                std::coroutine_handle<>
                await_suspend(std::coroutine_handle<promise_type> current) noexcept {
                    return current.promise().get_continuation();//继续执行剩下的逻辑
                }

                // Won't be resumed anyway
                constexpr void await_resume() const noexcept {
                }
            };
            /*
                在方法返回后，上下文环境销毁前调用此方法，返回值同 initial_suspend
            */
            constexpr final_awaiter final_suspend() noexcept {
                return {};
            }//立即销毁
            auto set_continuation(std::coroutine_handle<> coro_continuation) {
                this->continuation = coro_continuation;
            }
            auto get_continuation() {
                return continuation;
            }

        private:
            friend task<>;
            std::coroutine_handle<> continuation{std::noop_coroutine()};//返回的上一级协程
        };


        template<class T = void>
        struct promise : promise_base {
            task<T> get_return_object() noexcept;
            /*
            //在使用 co_return 返回时调用此方法 参数为返回的值
            */
            template<typename Value>
                requires std::convertible_to<Value&&, T>
            void return_value(Value&& result) noexcept(std::is_nothrow_constructible_v<T, Value&&>) {
                promise_result = std::forward<Value>(result);
            }

            /*
                调用co_yeild时调用此方法 参数是co_yield的值
            */
            template<typename Value>
                requires std::convertible_to<Value&&, T>
            auto yield_value(Value&& result) noexcept(std::is_nothrow_constructible_v<T, Value&&>) {
                promise_result = std::forward<Value>(result);
                return std::suspend_always{};
            }

            /*协程方法抛出没有处理的错误调用此方法*/
            void unhandled_exception() {
                promise_result = std::current_exception();
            }

            bool is_exception() {
                return promise_result.index() == 1;
            }

        private:
            friend task<T>;
            promise_result_type<T> promise_result;
        };

        template<>
        struct promise<void> : promise_base {
            task<> get_return_object() noexcept;
            /*
            //在使用 co_return 返回时调用此方法 参数为返回的值
            */
            void return_void() {
            }

            /*协程方法抛出没有处理的错误调用此方法*/
            void unhandled_exception() {
                exceptionPtr = std::current_exception();
            }
            bool is_exception() {
                return exceptionPtr != nullptr;
            }

            std::exception_ptr get_exception() {
                return exceptionPtr;
            }

        private:
            friend task<>;
            std::exception_ptr exceptionPtr;
        };
    }// namespace detail
}// namespace coplus