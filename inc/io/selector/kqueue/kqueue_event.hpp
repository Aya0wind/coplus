//
// Created by junjian LI on 2023/4/19.
//

#pragma once
#include <vector>
#include <sys/event.h>
namespace coplus::detail{
    using event = struct kevent;
    using events = ::std::vector<event>;
}

