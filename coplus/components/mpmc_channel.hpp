#pragma once
#include "coplus/components/impl/concurrent_deque.hpp"
namespace coplus {
    template<class T>
    using mpmc_channel = detail::concurrent_deque<T>;
}
