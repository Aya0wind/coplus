


#include <condition_variable>
#include <deque>
#include <mutex>

namespace coplus::detail {

    template<class T>
    class concurrent_deque {
        std::deque<T> queue_;
        mutable std::mutex mutex_;
        std::condition_variable cond_;
        bool need_shutdown = false;

    public:
        concurrent_deque() = default;
        concurrent_deque(const concurrent_deque&) = delete;
        concurrent_deque(concurrent_deque&&) = delete;
        concurrent_deque& operator=(const concurrent_deque&) = delete;
        concurrent_deque& operator=(concurrent_deque&&) = delete;
        T take_front() {
            std::unique_lock<std::mutex> lock(mutex_);
            cond_.wait(lock, [ this ] {
                return !queue_.empty() || need_shutdown;
            });
            auto ret = std::move(queue_.front());
            queue_.pop_front();
            return ret;
        }

        T take_back() {
            std::unique_lock<std::mutex> lock(mutex_);
            cond_.wait(lock, [ this ] {
                return !queue_.empty() || need_shutdown;
            });
            auto ret = std::move(queue_.back());
            queue_.pop_back();
            return ret;
        }

        void shutdown() {
            need_shutdown = true;
            cond_.notify_all();
        }

        bool empty() const {
            std::unique_lock<std::mutex> lock(mutex_);
            return queue_.empty();
        }
        size_t size() {
            std::unique_lock<std::mutex> lock(mutex_);
            return queue_.size();
        }
        void clear() {
            std::unique_lock<std::mutex> lock(mutex_);
            queue_.clear();
        }

        template<class Fn>
        void for_each(Fn&& fn) {
            std::unique_lock<std::mutex> lock(mutex_);
            std::for_each(queue_.begin(), queue_.end(), fn);
        }

        void wait_not_empty() {
            std::unique_lock<std::mutex> lock(mutex_);
            cond_.wait(lock, [ this ] {
                return !queue_.empty() || need_shutdown;
            });
        }

        template<class... Args>
        typename std::deque<T>::iterator emplace_back(Args&&... args) {
            std::unique_lock<std::mutex> lock(mutex_);
            queue_.emplace_back(std::forward<Args>(args)...);
            cond_.notify_all();
            return --queue_.end();
        }
        template<class I>
        typename std::deque<T>::iterator emplace_batch(I begin, I end) {
            std::unique_lock<std::mutex> lock(mutex_);
            queue_.insert(queue_.end(), begin, end);
            cond_.notify_all();
            return --queue_.end();
        }

        typename std::deque<T>::iterator push_back(T&& t) {
            std::unique_lock<std::mutex> lock(mutex_);
            queue_.emplace_back(std::move(t));
            cond_.notify_all();
            return --queue_.end();
        }

        typename std::deque<T>::iterator push_front(T&& t) {
            std::unique_lock<std::mutex> lock(mutex_);
            queue_.emplace_front(std::move(t));
            return queue_.begin();
        }
    };
}// namespace coplus::detail
