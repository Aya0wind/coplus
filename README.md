# Coplus 👋

>A stackless coroutine library based on c++20, use to implement async network programming and other async I/O operations.

## ✨RoadMap

+ ✅ basic ```task<T>``` future type and promise type.
+ ✅ multi-thread scheduler, based on min heap.
+ ✅ async timer and async delay.
+ ✅️ use kqueue on MacOS.
+ ✅ async socket read and write.
+ ✅️️ use epoll/io uring on linux.
+ ☑️ use iocp on windows.
+ ☑️ async mutex and channel.
+ ☑️ use lock free queue.
+ ☑️ joinable coroutine.

## Example
1. async timer
```c++
task<> timer_test() {
    while (true) {
        std::cout << "wait for 1000ms\n";
        co_await 1000_ms;
    }
}

int main() {
    co_runtime::spawn(timer_test());
    co_runtime::run();
}
```

2. async socket echo server
```c++
task<> server_test() {
    tcp_listener listener(ipv4("0.0.0.0"), 8080);
    while (true) {
        auto stream = co_await listener.accept();
        co_runtime::spawn([ connection(std::move(stream)) ]() -> task<> {
            char buffer[ 4096 ];
            while (true) {
                try {
                    size_t size = co_await connection.read(buffer, sizeof buffer);
                    if (size == 0) {
                        break;
                    }
                    co_await connection.write(buffer, size);
                } catch (std::exception& e) {
                    std::cout << "exception:"<< e.what()<<'\n';
                    break;
                }
            }
        });
    }
}

int main() {
    co_runtime::spawn(server_test());
    co_runtime::run();
}
```