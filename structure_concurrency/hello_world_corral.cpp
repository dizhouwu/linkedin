#include <iostream>

#include "../third_party/corral/asio.h"
#include "../third_party/corral/corral.h"


using namespace std::chrono_literals;

boost::asio::io_service io_service;

corral::Task<void> greet(std::string name) {
    std::cout << "Hello..." << std::endl;
    co_await corral::sleepFor(io_service, 1s);
    std::cout << "..." << name << std::endl;
}

corral::Task<void> greetThings() {
    co_await corral::allOf(greet("world"), greet("corral"),
                           greet("coroutines"));
}

int main(int argc, char** argv) {
    corral::run(io_service, greetThings());
}