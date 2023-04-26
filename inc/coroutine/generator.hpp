//
// Created by junjian LI on 2022/9/8.
//

#pragma once
#include <coroutine>
#include <exception>
namespace cocpp {
    template<class T>
    struct Generator {
        struct promise_type {
            T current_value;
            Generator<T> get_return_object() noexcept {
                return Generator{this};
            }
            auto initial_suspend() {
                return std::suspend_never{};
            }
            auto final_suspend() noexcept {
                return std::suspend_never{};
            }
            void return_void() {
            }
            void unhandled_exception() {
                std::terminate();
            }
            auto yield_value(T value) {
                current_value = value;
                return std::suspend_always{};
            }
        };
        using handle_type = std::coroutine_handle<promise_type>;
        handle_type handle_;
        explicit Generator(handle_type handle) noexcept
            : handle_(handle) {
        }
        Generator(Generator const&) = delete;
        Generator(Generator&& other) noexcept
            : handle_(other.handle_) {
            other.handle_ = nullptr;
        }
        //    Generator& operator=(Generator&& other){
        //        if(this != &other){
        //            if(handle_){
        //                handle_.destroy();
        //            }
        //            handle_ = other.handle_;
        //            other.handle_ = nullptr;
        //        }
        //        return *this;
        //    }
        T next() {
            if (handle_) {
                if (handle_.done()) {
                    handle_.destroy();
                    handle_ = nullptr;
                    return value();
                }
                handle_.resume();
                return value();
            }
            return 0;
        }
        T& value() {
            return handle_.promise().current_value;
        }
    };
}// namespace cocpp
