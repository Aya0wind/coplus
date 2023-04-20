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
    co_await DelayAwaiter::delay(std::chrono::seconds(1));
    std::cout<<"over"<<'\n';
}


int main()
{
    for (int i = 0; i < 5000; ++i) {
        co_runtime::spawn(test(i));
    }
    co_runtime::block_on(test(20));
}


