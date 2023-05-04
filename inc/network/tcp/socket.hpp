//
// Created by junjian LI on 2023/4/25.
//

#pragma once
#include "network/ip/ipv4.hpp"
#ifdef _WIN32
#include "windows_socket.hpp"
#elif __linux__
#include "linux_socket.hpp"
#elif __APPLE__
#include "macos_socket.hpp"
#endif