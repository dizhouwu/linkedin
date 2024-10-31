#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <memory> // For std::shared_ptr
#include "concurrentqueue.h" // Include Moodycamel's ConcurrentQueue

// Define a structure to represent a price update message.
struct PriceUpdate {
    int symbolId;
    double price;
    long timestamp;
};

// Each consumer has its own queue with shared pointers to PriceUpdate.
std::vector<moodycamel::ConcurrentQueue<std::shared_ptr<PriceUpdate>>> consumerQueues;
std::atomic<bool> stopFlag(false); // To signal consumers to stop

// Producer function
void producer(int numConsumers) {
    int symbolId = 1; // Single symbol for simplicity

    while (!stopFlag) {
        // Create a shared pointer to a new PriceUpdate instance
        auto update = std::make_shared<PriceUpdate>();
        update->symbolId = symbolId;
        update->price = 100.0 + (rand() % 100) / 100.0; // Random price
        update->timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        
        // Push the shared pointer to each consumer's queue
        for (int i = 0; i < numConsumers; ++i) {
            consumerQueues[i].enqueue(update);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Simulate data arrival every 10ms
    }

    std::cout << "Producer stopped.\n";
}

// Consumer function
void consumer(int consumerId) {
    while (!stopFlag) {
        std::shared_ptr<PriceUpdate> update;
        if (consumerQueues[consumerId].try_dequeue(update)) {
            // Process the price update
            std::cout << "Consumer " << consumerId + 1 << " processed update: "
                      << "SymbolId: " << update->symbolId
                      << ", Price: " << update->price
                      << ", Timestamp: " << update->timestamp << "\n";
        } else {
            // No data available, wait a little before retrying
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    }

    std::cout << "Consumer " << consumerId + 1 << " stopped.\n";
}

int main() {
    // Define the number of consumers
    int numConsumers = 4;

    // Initialize a queue for each consumer
    consumerQueues.resize(numConsumers);

    // Start the producer thread
    std::thread producerThread(producer, numConsumers);

    // Start multiple consumer threads
    std::vector<std::thread> consumerThreads;
    for (int i = 0; i < numConsumers; ++i) {
        consumerThreads.emplace_back(consumer, i);
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
