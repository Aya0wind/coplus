
#include "context/runtime.hpp"
#include "context/worker_thread_context.hpp"
#include "network/tcp/socket.hpp"
#include "network/tcp/tcp_stream.hpp"
#include "poll/poller.hpp"
#include <atomic>
#include <fmt/format.h>
#include <sstream>
using namespace coplus;

task<> server_test() {
    tcp_listener listener(net_address(ipv4("0.0.0.0"), 8080));
    char buffer[ 1024 ];
    fmt::print("point 1\n");
    while (true) {
        fmt::print("point 2\n");
        co_runtime::print_global_task_queue_size();
        auto stream = co_await listener.accept();
        fmt::print("point 3,{}\n", stream.raw_fd());
        co_runtime::spawn([ &buffer, connection = stream.raw_fd() ]() -> task<> {
            fmt::print("point 4\n");
            fmt::print("socket:{}\n", connection);
            auto con = tcp_stream(sys_socket(connection));
            while (true) {
                fmt::print("point 5\n");
                size_t size = co_await con.read(buffer, sizeof buffer);
                co_await con.write(buffer, size);
            }
        });
        fmt::print("stop");
    }
}

int main() {
    print_thread_id("main");
    //co_runtime::spawn(client_test());
    co_runtime::spawn(server_test());
    co_runtime::run();
}
