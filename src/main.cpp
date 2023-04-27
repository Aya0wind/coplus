
#include "context/runtime.hpp"
#include "context/worker_thread_context.hpp"
#include "network/tcp/socket.hpp"
#include "network/tcp/tcp_stream.hpp"
#include "poll/poller.hpp"
#include <atomic>
#include <cstddef>
#include <fmt/format.h>
#include <functional>
#include <sstream>
using namespace coplus;

task<> process(char* buffer,size_t size, tcp_stream stream) {
    fmt::print("point 4\n");
    fmt::print("socket:{}\n", stream.raw_fd());
    while (true) {
        fmt::print("point 5\n");
        size_t size = co_await stream.read(buffer, sizeof buffer);
        co_await stream.write(buffer, size);
    }
}


task<> server_test() {
    tcp_listener listener(net_address(ipv4("0.0.0.0"), 8080));
    char buffer[ 1024 ];
    fmt::print("point 1\n");
    while (true) {
        fmt::print("point 2\n");
        co_runtime::print_global_task_queue_size();
        auto stream = co_await listener.accept();
        fmt::print("point 3,{}\n", stream.raw_fd());
        co_runtime::spawn(process(buffer, sizeof buffer, std::move(stream)));
        fmt::print("stop");
    }
}

int main() {
    print_thread_id("main");
    //co_runtime::spawn(client_test());
    co_runtime::spawn(server_test());
    co_runtime::run();
}
