#include <iostream>
#include <thread>
#include <atomic>

int shared_data = 0; // change to atomic to eliminate data race

void increment() {
    for (int i = 0; i < 100000; ++i) {
        ++shared_data;  // This will cause a data race when accessed by multiple threads
    }
}

int main() {
    std::thread t1(increment);
    std::thread t2(increment);

    t1.join();
    t2.join();

    std::cout << "Final value of shared_data: " << shared_data << std::endl;
    return 0;
}
