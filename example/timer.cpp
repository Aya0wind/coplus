
#include "coplus/context/runtime.hpp"
#include "coplus/coroutine/task.hpp"
#include "coplus/time/delay.hpp"
using namespace coplus;

task<> client_test() {
    while (true) {
        co_await 1000_ms;
        std::cout << "wait for 1000ms\n";
    }
}
task<> client_test1() {
    while (true) {
        co_await 2000_ms;
        std::cout << "wait for 2000ms\n";
    }
}
task<> client_test2() {
    while (true) {
        co_await 3000_ms;
        std::cout << "wait for 3000ms\n";
    }
}
int main() {
    co_runtime::spawn(client_test());
    co_runtime::spawn(client_test1());
    co_runtime::spawn(client_test2());
    co_runtime::run();
}
