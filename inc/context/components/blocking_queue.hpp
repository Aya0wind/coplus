//
// Created by junjian LI on 2023/4/11.
//

#pragma once
#include <deque>

template<class T>
class blocking_queue {
    std::deque<T> queue;
    std::condition_variable cv;
    mutable std::mutex mutex;
    bool isStop = false;
public:
    blocking_queue() = default;
    blocking_queue(const blocking_queue&) = delete;

    template<class ...Args>
    void emplace(Args&&... args){
        std::unique_lock<std::mutex> lock(mutex);
        queue.emplace_back(std::forward<Args>(args)...);
    }

    template<Iter<T> I>
    void emplace_batch(I begin, I end){
        std::unique_lock<std::mutex> lock(mutex);
        queue.insert(queue.end(), begin, end);
        lock.unlock();
        cv.notify_all();
    }

    void push_back(T&& t){
        std::unique_lock<std::mutex> lock(mutex);
        queue.emplace_back(std::move(t));
        lock.unlock();
        cv.notify_all();
    }

    T pop_front(){
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [this](){return !queue.empty();});
        auto ret = std::move(queue.front());
        queue.pop_front();
        cv.notify_all();
        return ret;
    }

    bool empty() const{
        std::unique_lock<std::mutex> lock(mutex);
        return queue.empty();
    }

    size_t size(){
        std::unique_lock<std::mutex> lock(mutex);
        return queue.size();
    }

};


