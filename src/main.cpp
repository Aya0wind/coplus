#include "io/poll.hpp"
#include "timer/timer.hpp"
#include <fmt/format.h>
#include <sstream>
using namespace coplus;

//task<> test() {
//    tcp_listener listener;
//    listener.bind("0.0.0.0", 8080);
//    auto stream = co_await listener.accept();
//    char buffer[1024];
//    while (true){
//        size_t size = co_await stream.read(buffer, 1024);
//        co_await stream.write(buffer, size);
//    }
//}
task<> test(int i)
{
    //fmt::print("task:{}\n",i);
    for (int j = 0; j < 5; ++j) {
//        std::stringstream ss;
//        ss<<std::this_thread::get_id();
//        fmt::print("task{} in thread:{}\n",i,ss.str());
        co_await DelayAwaiter::delay(std::chrono::seconds(2));
    }
}

task<> test1(int i)
{
    for (int j = 0; j < 10; ++j){
//        std::stringstream ss;
//        ss<<std::this_thread::get_id();
//        fmt::print("task{} in thread:{}\n",i,ss.str());
        co_await DelayAwaiter::delay(std::chrono::seconds(1));
    }
}




int main()
{
    for (int i = 0; i < 500000; ++i) {
        co_runtime::spawn(test(i));
    }
    co_runtime::block_on(test1(20));
}
