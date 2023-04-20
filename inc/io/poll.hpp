//
// Created by junjian LI on 2023/4/18.
//

#pragma once


#include "event.hpp"
#include <functional>
#include <mutex>
namespace coplus {
    class waker;
}

namespace coplus::detail {

    template<class T>
    concept RegistryTrait = requires(T t) {
        {
            t.get_selector()
        } -> std::convertible_to<std::reference_wrapper<typename T::selector_type>>;
    };

    template<class T,class R>
    concept SourceTrait = requires(T t) {
        {
            t.register_event(std::declval<R&>(), std::declval<token_type>(), std::declval<Interest>(), std::declval<intptr_t>())
        } -> std::same_as<void>;
    }&&RegistryTrait<R>;

//    template<class S>
//    class source_template {
//        S source;
//    public:
//        using source_type = S;
//        S& get_source(){
//            return source;
//        }
//        void register_event(registry& registry, token_type token, Interest interest,intptr_t task_id) const
//        {
//            source.register_event(registry,token, interest,task_id);
//        }
//    };


    template<class S>
    class registry_template{
        S  selector_;
    public:
        using selector_type = S;
        S& get_selector();
        [[nodiscard]] int get_sys_handle() const;
    };

}
    
#ifdef __linux__
#include "io/selector/epoll/epoll_selector.hpp"
    using selector = coplus::detail::epoll_selector;
#elif __APPLE__
#include "io/selector/kqueue/kqueue_selector.hpp"
    using selector = ::coplus::detail::kqueue_selector;
    using registry = ::coplus::detail::registry_template<selector>;
#elif _WIN32
#include "io/registries/iocp/iocp_selector.hpp"
    using selector = coplus::detail::iocp_selector;
#endif

namespace coplus::detail {

    template<>
    class registry_template<kqueue_selector>{
        kqueue_selector  selector_;
    public:
        using selector_type = kqueue_selector;
        kqueue_selector& get_selector(){
            return selector_;
        }
        [[nodiscard]] int get_sys_handle() const
        {
            return selector_.get_fd();
        }
    };



    class poll {
        registry inner_registry;
    public:
        template<class duration_type, class period>
        int poll_events(events& events, ::std::chrono::duration<duration_type, period> timeout)
        {
            return inner_registry.get_selector().select(events, timeout);
        }
        static void stop_wait(int sys_handle)
        {
            registry::selector_type::stop_wait_signal(sys_handle);
        }
        registry& get_registry()
        {
            return inner_registry;
        }
        auto get_sys_handle() const
        {
            return inner_registry.get_sys_handle();
        }

        template<SourceTrait<registry> S>
        void register_event(S&& source,token_type token, Interest interest,intptr_t task_id)
        {
            source.register_event(inner_registry,token, interest,task_id);
        }

        template<SourceTrait<registry> S>
        void deregister_event(S&& source,token_type token, Interest interest,intptr_t task_id)
        {
            source.register_event(inner_registry,token, interest,task_id);
        }

    };



}// namespace coplus::detail
