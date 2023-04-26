//
// Created by junjian LI on 2023/4/19.
//

#pragma once

#include <atomic>
#include <cstdint>
class id_generator {
    inline static std::atomic<intptr_t> id_ = 0;

public:
    static intptr_t next_id() {
        return id_++;
    }
};
