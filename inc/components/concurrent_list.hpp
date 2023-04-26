//
// Created by junjian LI on 2023/4/19.
//

#pragma once
#include <list>
#include <mutex>
//a list can be access thread safe
template<class T>
class concurrent_list {
    std::list<T> inner_;
    mutable std::mutex mutex_;
    std::condition_variable cond_;
    std::atomic<bool> down_ = false;

public:
    concurrent_list() = default;
    concurrent_list(const concurrent_list&) = delete;
    concurrent_list(concurrent_list&&) = delete;
    concurrent_list& operator=(const concurrent_list&) = delete;
    concurrent_list& operator=(concurrent_list&&) = delete;

    void down() {
        down_ = true;
        cond_.notify_all();
    }
    void swap_inner(std::list<T>& out) {
        std::unique_lock<std::mutex> lock(mutex_);
        out.swap(inner_);
    }

    void wait_not_empty() {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [ this ] {
            return !inner_.empty() || down_;
        });
    }

    template<class... Args>
    typename std::list<T>::iterator emplace_back(Args&&... args) {
        std::unique_lock<std::mutex> lock(mutex_);
        inner_.emplace_back(std::forward<Args>(args)...);
        cond_.notify_all();
        return --inner_.end();
    }
    template<class I>
    typename std::list<T>::iterator emplace_batch(I begin, I end) {
        std::unique_lock<std::mutex> lock(mutex_);
        inner_.insert(inner_.end(), begin, end);
        cond_.notify_all();
        return --inner_.end();
    }

    typename std::list<T>::iterator push_back(T&& t) {
        std::unique_lock<std::mutex> lock(mutex_);
        inner_.emplace_back(std::move(t));
        cond_.notify_all();
        return --inner_.end();
    }

    typename std::list<T>::iterator push_front(T&& t) {
        std::unique_lock<std::mutex> lock(mutex_);
        inner_.emplace_front(std::move(t));
        return inner_.begin();
    }

    T take(typename std::list<T>::iterator it) {
        std::unique_lock<std::mutex> lock(mutex_);
        auto ret = std::move(*it);
        inner_.erase(it);
        return ret;
    }

    T take_back() {
        std::unique_lock<std::mutex> lock(mutex_);
        auto ret = std::move(inner_.back());
        inner_.pop_back();
        return ret;
    }

    T take_back_block() {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [ this ] {
            return !inner_.empty();
        });
        auto ret = std::move(inner_.back());
        inner_.pop_back();
        return ret;
    }

    T take_front() {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [ this ] {
            return !inner_.empty();
        });
        auto ret = std::move(inner_.front());
        inner_.pop_front();
        return ret;
    }

    bool empty() const {
        std::unique_lock<std::mutex> lock(mutex_);
        return inner_.empty();
    }
    size_t size() {
        std::unique_lock<std::mutex> lock(mutex_);
        return inner_.size();
    }
    void clear() {
        std::unique_lock<std::mutex> lock(mutex_);
        inner_.clear();
    }

    template<class Fn>
    void for_each(Fn&& fn) {
        std::unique_lock<std::mutex> lock(mutex_);
        std::for_each(inner_.begin(), inner_.end(), fn);
    }
};
