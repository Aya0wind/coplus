//
// Created by junjian LI on 2023/4/25.
//

#pragma once
#ifdef __linux__
#include "coplus/network/ip/ipv4/posix_ipv4.hpp"
#elif __APPLE__
#include "coplus/network/ip/ipv4/unix_ipv4.hpp"
#elif _WIN32
#include "coplus/network/ip/ipv4/windows_ipv4.hpp"
#endif
