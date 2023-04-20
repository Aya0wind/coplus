//
// Created by junjian LI on 2022/9/8.
//

#pragma once
#include "iomp/"
namespace cocpp {
    class [[nodiscard("Did you forget to co_await?")]] kqueue_awaiter
    {
    public:
        constexpr bool await_ready() const noexcept
        {
            return false;
        }

        void await_suspend(std::coroutine_handle<> current) noexcept
        {
            io_info.handle = current;
            submit();
        }

        int32_t await_resume() const noexcept
        {
            return io_info.result;
        }
        kqueue_awaiter(const kqueue_awaiter&) = delete;
        kqueue_awaiter(kqueue_awaiter&&) = delete;
        kqueue_awaiter& operator=(const kqueue_awaiter&) = delete;
        kqueue_awaiter& operator=(kqueue_awaiter&&) = delete;

    protected:
        friend class lazy_link_io;
        friend struct lazy_link_timeout;
        liburingcxx::SQEntry *sqe;
        task_info io_info;

        inline void submit() noexcept
        {
            worker_meta *const worker = detail::this_thread.worker;
            worker->submit_sqe();
        }

        lazy_awaiter() noexcept : io_info(task_info::task_type::lazy_sqe)
        {
            io_info.tid_hint = detail::this_thread.tid;
            sqe = this_thread.worker->get_free_sqe();
            assert(sqe != nullptr);
            sqe->setData(io_info.as_user_data());
        }
    };
}