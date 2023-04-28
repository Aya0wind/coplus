
#include "context/runtime.hpp"
#include "coroutine/task.hpp"
#include "network/tcp/socket.hpp"
#include "sources/socket/tcp_stream.hpp"
#include "time/delay.hpp"
#include <fmt/format.h>
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
                    if(size==0){
                        break;
                    }
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
        fmt::print("wait for 1000ms\n");
        co_await 1000_ms;
    }
}

int main() {
    //co_runtime::spawn(client_test());
    co_runtime::spawn(server_test());
    co_runtime::run();
}
