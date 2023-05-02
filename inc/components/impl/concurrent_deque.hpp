


#include <algorithm>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <optional>

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
            lock.unlock();
            std::lock_guard<std::mutex> lockm(mutex_);
            auto ret = std::move(queue_.front());
            queue_.pop_front();
            return ret;
        }

        std::optional<T> try_take_front() {
            std::unique_lock<std::mutex> lock(mutex_);
            if (queue_.empty()) {
                cond_.wait_for(lock, std::chrono::milliseconds(50));
                if (queue_.empty()) {
                    return std::nullopt;
                }
            }
            auto ret = std::move(queue_.front());
            queue_.pop_front();
            return ret;
        }


        T take_back() {
            std::unique_lock<std::mutex> lock(mutex_);
            cond_.wait(lock, [ this ] {
                return !queue_.empty() || need_shutdown;
            });
            lock.unlock();
            std::lock_guard<std::mutex> lockm(mutex_);
            auto ret = std::move(queue_.back());
            queue_.pop_back();
            return ret;
        }

        void shutdown() {
            need_shutdown = true;
            cond_.notify_all();
        }

        bool empty() const {
            std::lock_guard<std::mutex> lock(mutex_);
            return queue_.empty();
        }
        size_t size() {
            std::lock_guard<std::mutex> lock(mutex_);
            return queue_.size();
        }
        void clear() {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.clear();
        }

        template<class Fn>
        void for_each(Fn&& fn) {
            std::lock_guard<std::mutex> lock(mutex_);
            std::for_each(queue_.begin(), queue_.end(), fn);
        }

        void wait_not_empty() {
            std::unique_lock<std::mutex> lock(mutex_);
            cond_.wait(lock, [ this ] {
                return !queue_.empty() || need_shutdown;
            });
        }

        template<class... Args>
        void emplace_back(Args&&... args) {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                queue_.emplace_back(std::forward<Args>(args)...);
            }
            cond_.notify_one();
        }
        template<class I>
        void emplace_batch(I begin, I end) {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                queue_.insert(queue_.end(), begin, end);
            }
            cond_.notify_one();
        }

        void push_back(T&& t) {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                queue_.push_back(std::forward<T>(t));
            }
            cond_.notify_all();
        }

        void push_front(T&& t) {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                queue_.push_front(std::forward<T>(t));
            }
        }
    };
}// namespace coplus::detail
