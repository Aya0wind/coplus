
#include "context/runtime.hpp"
#include "context/worker_thread_context.hpp"
#include "coroutine/task.hpp"
#include "network/tcp/socket.hpp"
#include "network/tcp/tcp_stream.hpp"
#include "poll/poller.hpp"
#include "time/delay.hpp"
#include <atomic>
#include <cstddef>
#include <fmt/format.h>
#include <functional>
#include <sstream>
using namespace coplus;


task<> server_test() {
    tcp_listener listener(net_address(ipv4("0.0.0.0"), 8080));
    char buffer[ 1024 ];
    char* buffer_ptr = buffer;
    while (true) {
        auto stream = co_await listener.accept();
        co_runtime::spawn([ buffer_ptr, connection(std::move(stream)) ]() -> task<> {
            while (true) {
                try {
                    size_t size = co_await connection.read(buffer_ptr, sizeof buffer);
                    co_await connection.write(buffer_ptr, size);
                } catch (std::exception& e) {
                    fmt::print("exception:{}\n", e.what());
                    break;
                }
            }
        });
    }
}


task<> client_test() {
    while (true) {
        co_await 1000_ms;
        fmt::print("wait for 1000ms\n");
    }
}

int main() {
    //co_runtime::spawn(client_test());
    co_runtime::spawn(server_test());
    co_runtime::run();
}
