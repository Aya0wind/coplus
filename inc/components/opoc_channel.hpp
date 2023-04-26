//
// Created by junjian LI on 2023/4/26.
//

#pragma once

namespace coplus{
    struct opoc_channel{

    };


    namespace detail{
        template<class T>
        struct opoc_channel_context{
            T data;

        }
    }
    template<class T>
    class opoc_channel_sender{

    };
    template<class T>
    class opoc_channel_receiver{

    };
}