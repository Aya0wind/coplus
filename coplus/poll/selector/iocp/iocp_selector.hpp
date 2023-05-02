//
// Created by junjian LI on 2023/4/18.
//

#pragma once

namespace coplus::detail {
    static WSAData wsaData = []() {
        WSADATA wsaData;
        int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (iResult != 0) {
            std::cout << std::string("WSAStartup failed: ") + std::to_string(iResult) << '\n';
            exit(1);
        }
        return wsaData;
    }();

    class iocp_selector : public selector_base<iocp_selector> {
        friend class selector_base;
        HANDLE iocp_queue_handle;

        [[nodiscard]] handle_type get_handle_impl() const {
            return iocp_queue_handle;
        }
        static int wake_impl(handle_type sys_handle) {
            //send the stop signal to the selector
        }

        int select_impl(events& events, ::std::chrono::milliseconds timeout) const {
            int completion_key = 0;
            auto& sysEvent = (sys_event&) events[ 0 ];
            return GetQueuedCompletionStatus(iocp_queue_handle,
                                             &sysEvent.nBytes,
                                             (PULONG_PTR) (&sysEvent.flags),
                                             (LPOVERLAPPED*) &sysEvent,
                                             timeout.count());
        }

        void register_event_impl(handle_type file_handle, Interest interest, int data, void* udata) const {
            CreateIoCompletionPort(file_handle,
                                   this->iocp_queue_handle,
                                   0,
                                   1);
        }

        void deregister_event_impl(handle_type file_handle, Interest interest) const {
        }

    public:
        iocp_selector() :
            iocp_queue_handle(CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 1)) {
        }
        ~iocp_selector() {
            CloseHandle(iocp_queue_handle);
        }
    };
}// namespace coplus::detail