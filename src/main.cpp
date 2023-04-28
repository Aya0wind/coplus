
#include "context/runtime.hpp"
#include "coroutine/task.hpp"
#include "network/tcp/socket.hpp"
#include "sources/socket/tcp_stream.hpp"
#include "time/delay.hpp"
using namespace coplus;


task<> server_test() {
    tcp_listener listener(ipv4("0.0.0.0"), 8080);
    while (true) {
        auto stream = co_await listener.accept();
        co_runtime::spawn([ connection(std::move(stream)) ]() -> task<> {
            char buffer[ 4096 ];
            while (true) {
                try {
                    size_t size = co_await connection.read(buffer, sizeof buffer);
                    //std::cout<<"read size:"<<size<<"\n";
                    if (size == 0) {
                        break;
                    }
                    co_await connection.write(buffer, size);
                    //std::cout<<"write size:"<<size<<"\n";
                } catch (std::exception& e) {
                    std::cout << "exception:"<< e.what()<<'\n';
                    break;
                }
            }
        });
    }
}


task<> client_test() {
    while (true) {
        std::cout << "wait for 1000ms\n";
        co_await 1000_ms;
    }
}

int main() {
    //co_runtime::spawn(client_test());
    co_runtime::spawn(server_test());
    co_runtime::run();
}
