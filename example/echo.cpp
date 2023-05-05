
#include "context/runtime.hpp"
#include "coroutine/promise.hpp"
#include "coroutine/task.hpp"
#include "network/tcp/socket.hpp"
#include "sources/tcp_stream/tcp_stream.hpp"
using namespace coplus;

task<> server_test() {
    tcp_listener listener(ipv4("0.0.0.0"), 8082);
    while (true) {
        auto stream = co_await listener.accept();
        co_runtime::spawn([connection = std::move(stream)]()->task<>{
            char buffer[ 4096 ];
            try {
                while (true) {
                    size_t size = co_await connection.read(buffer, sizeof buffer);
                    //std::cout<<"read size:"<<size<<"\n";
                    if (size == 0) {
                        break;
                    }
                    co_await connection.write(buffer, size);
                    //std::cout<<"write size:"<<size<<"\n";
                }
            } catch (std::exception& e) {
                std::cout << "exception:" << e.what() << '\n';
            }
        });
        std::cout<<"created 1"<<'\n';
    }
}


int main() {
    co_runtime::spawn(server_test());
    co_runtime::run();
}
