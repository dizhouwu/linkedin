#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include "concurrentqueue.h" // Include Moodycamel's ConcurrentQueue

// Define a structure to represent a price update message.
struct PriceUpdate {
    int symbolId;
    double price;
    long timestamp;
};

// Define the queue with a single producer and multiple consumers.
moodycamel::ConcurrentQueue<PriceUpdate> priceQueue;
std::atomic<bool> stopFlag(false); // To signal consumers to stop

// Producer function
void producer() {
    int symbolId = 1; // Single symbol for simplicity

    while (!stopFlag) {
        PriceUpdate update;
        update.symbolId = symbolId;
        update.price = 100.0 + (rand() % 100) / 100.0; // Random price
        update.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        
        // Push update to the queue
        priceQueue.enqueue(update);
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Simulate data arrival every 10ms
    }

    std::cout << "Producer stopped.\n";
}

// Consumer function
void consumer(int consumerId) {
    while (!stopFlag) {
        PriceUpdate update;
        if (priceQueue.try_dequeue(update)) {
            // Process the price update
            std::cout << "Consumer " << consumerId << " processed update: "
                      << "SymbolId: " << update.symbolId
                      << ", Price: " << update.price
                      << ", Timestamp: " << update.timestamp << "\n";
        } else {
            // No data available, wait a little before retrying
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }

    std::cout << "Consumer " << consumerId << " stopped.\n";
}

int main() {
    // Start the producer thread
    std::thread producerThread(producer);

    // Start multiple consumer threads
    int numConsumers = 4;
    std::vector<std::thread> consumerThreads;
    for (int i = 0; i < numConsumers; ++i) {
        consumerThreads.emplace_back(consumer, i + 1);
    }

    // Run the application for a limited time (e.g., 2 seconds) for this example
    std::this_thread::sleep_for(std::chrono::seconds(2));
    stopFlag = true;

    // Wait for producer and consumers to finish
    producerThread.join();
    for (auto &thread : consumerThreads) {
        thread.join();
    }

    std::cout << "Application stopped.\n";
    return 0;
}
