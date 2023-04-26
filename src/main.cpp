
#include "context/runtime.hpp"
#include "context/worker_thread_context.hpp"
#include "network/tcp/tcp_stream.hpp"
#include "poll/poller.hpp"
#include "time/timer.hpp"
#include <atomic>
#include <fmt/format.h>
#include <sstream>
using namespace coplus;

//task<> server_test()
//{
//    tcp_listener listener(net_address(ipv4("0.0.0.0"), 8082));
//    char buffer[ 1024 ];
//    fmt::print("point 1\n");
//    while (true) {
//        fmt::print("point 2\n");
//        auto tcp_stream = co_await listener.accept();
//        fmt::print("point 3\n");
//        auto loop  =  [&buffer,tcp_stream = std::move(tcp_stream)] ()->task<>{
//            fmt::print("point 4\n");
//            while (true) {
//                fmt::print("point 5\n");
//                size_t size = co_await tcp_stream.read(buffer, sizeof buffer);
//                co_await tcp_stream.write(buffer, size);
//            }
//        };
//        current_worker_context.add_ready_task(std::move(co_runtime::make_task(loop)));
//    }
//}
//
task<> client_test() {
    while (true) {
        co_await 1000_ms;
        fmt::print("point 1\n");
    }
}


int main() {
    print_thread_id("main");
    co_runtime::spawn(client_test());
    co_runtime::run();
}
