
#include "context/runtime.hpp"
#include "coroutine/task.hpp"
#include "time/delay.hpp"
using namespace coplus;

task<> client_test() {
    while (true) {
        std::cout << "wait for 1000ms\n";
        co_await 1000_ms;
    }
}

int main() {
    co_runtime::spawn(client_test());
    co_runtime::run();
}
