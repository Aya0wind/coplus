
#include "context/runtime.hpp"
#include "time/delay.hpp"
#include "context/worker_thread_context.hpp"
#include "network/tcp/tcp_stream.hpp"
#include "poll/poller.hpp"
#include <atomic>
#include <fmt/format.h>
#include <sstream>
using namespace coplus;

task<> server_test()
{
    tcp_listener listener(net_address(ipv4("0.0.0.0"), 8080));
    char buffer[ 1024 ];
    fmt::print("point 1\n");
    while (true) {
        fmt::print("point 2\n");
        co_runtime::print_global_task_queue_size();
        auto tcp_stream = co_await listener.accept();
        fmt::print("point 3\n");
        co_runtime::spawn([&buffer,connection = std::move(tcp_stream)] ()->task<>{
            fmt::print("point 4\n");
            fmt::print("socket:{}\n",connection.raw_fd());
            while (true) {
                fmt::print("point 5\n");
                size_t size = co_await connection.read(buffer, sizeof buffer);
                co_await connection.write(buffer, size);
            }
        });
        fmt::print("stop");
    }
}

task<> client_test() {
    while (true){
        co_await 1000_ms;
        co_runtime::print_global_task_queue_size();
    }
}


int main() {
    print_thread_id("main");
    //co_runtime::spawn(client_test());
    co_runtime::spawn(server_test());
    co_runtime::run();
}
