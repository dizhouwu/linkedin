#include <iostream>
#include <chrono>
#include <functional>
#include <thread>
#include <mutex>
#include <random>
#include <atomic>
#include <iomanip> // For std::put_time
#include <ctime>   // For std::time_t, std::localtime
#include <memory>  // For std::unique_ptr

using Clock = std::chrono::steady_clock;
using TimePoint = Clock::time_point;
using TimerCallback = std::function<void()>;

// OHLCV data structure
struct OHLCV {
    double open = 0.0;
    double high = 0.0;
    double low = 0.0;
    double close = 0.0;
    uint64_t volume = 0;
};

// Node structure for the linked list
struct TimerNode {
    TimePoint expirationTime;
    TimerCallback callback;
    std::unique_ptr<TimerNode> next; // Use smart pointer here

    TimerNode(TimePoint time, TimerCallback cb) 
        : expirationTime(time), callback(cb), next(nullptr) {}
};

// Timer list manager with thread safety
class TimerList {
public:
    TimerList() : head(nullptr), stop(false) {}

    void addTimer(int delayMillis, TimerCallback callback) {
        TimePoint expiration = Clock::now() + std::chrono::milliseconds(delayMillis);
        auto newNode = std::make_unique<TimerNode>(expiration, callback); // Use smart pointer

        std::lock_guard<std::mutex> lock(mtx);
        if (!head || expiration < head->expirationTime) {
            newNode->next = std::move(head); // Move head to newNode
            head = std::move(newNode);
        } else {
            TimerNode* current = head.get();
            while (current->next && current->next->expirationTime < expiration) {
                current = current->next.get();
            }
            newNode->next = std::move(current->next); // Move next to newNode
            current->next = std::move(newNode);
        }
    }

    void tick() {
        while (!stop) {
            TimePoint now = Clock::now();
            {
                std::lock_guard<std::mutex> lock(mtx);
                while (head && head->expirationTime <= now) {
                    TimerNode* expired = head.release(); // Release the unique_ptr
                    expired->callback();  // Execute the callback
                    head = std::move(expired->next); // Move the next node to head
                    delete expired; // Delete the expired node
                }
            }
            std::this_thread::yield();
        }
    }

    void stopTicking() {
        stop = true;
    }

private:
    std::unique_ptr<TimerNode> head; // Use smart pointer here
    std::mutex mtx;
    bool stop;
};

// Mock packet processing function to simulate incoming market data
void mockProcessPacket(OHLCV &ohlcv, std::mutex &ohlcvMutex) {
    static std::mt19937 rng(static_cast<unsigned>(std::time(nullptr)));
    static std::uniform_real_distribution<double> priceDist(100.0, 200.0);
    static std::uniform_int_distribution<uint64_t> volumeDist(1, 1000);

    double price = priceDist(rng);
    uint64_t volume = volumeDist(rng);

    std::lock_guard<std::mutex> lock(ohlcvMutex);

    if (ohlcv.open == 0.0) ohlcv.open = price;
    ohlcv.high = std::max(ohlcv.high, price);
    ohlcv.low = (ohlcv.low == 0.0) ? price : std::min(ohlcv.low, price);
    ohlcv.close = price;
    ohlcv.volume += volume;
}

// Function to output OHLCV bar
void outputOHLCV(OHLCV &ohlcv, std::mutex &ohlcvMutex) {
    OHLCV ohlcvCopy;

    {
        std::lock_guard<std::mutex> lock(ohlcvMutex);
        ohlcvCopy = ohlcv; // Copy data to minimize lock time
        ohlcv = OHLCV(); // Reset OHLCV for the next period
    }

    // Get current time
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm* now_tm = std::localtime(&now_c);

    // Output the current time and OHLCV data
    std::cout << "[" << std::put_time(now_tm, "%H:%M:%S") << "] "
              << "OHLCV: Open=" << ohlcvCopy.open << ", High=" << ohlcvCopy.high 
              << ", Low=" << ohlcvCopy.low << ", Close=" << ohlcvCopy.close 
              << ", Volume=" << ohlcvCopy.volume << std::endl;
}

int main() {
    TimerList timerList;
    OHLCV ohlcvData;
    std::mutex ohlcvMutex;
    std::atomic<bool> running(true);

    // Add a 1-second timer 10 times
    for (int i = 0; i < 10; ++i) {
        timerList.addTimer(1000 * (i + 1), [&ohlcvData, &ohlcvMutex]() {
            outputOHLCV(ohlcvData, ohlcvMutex);
        });
    }

    // Start the timer processing thread
    std::thread timerThread(&TimerList::tick, &timerList);

    // Simulate receiving data packets in a separate thread
    std::thread dataThread([&ohlcvData, &ohlcvMutex, &running]() {
        while (running) {
            mockProcessPacket(ohlcvData, ohlcvMutex);
            std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Simulate packet arrival rate
        }
    });

    // Run the simulation for a limited time (e.g., 10 seconds)
    std::this_thread::sleep_for(std::chrono::seconds(10));
    running = false;
    timerList.stopTicking();

    // Clean up
    timerThread.join();
    dataThread.join();

    return 0;
}
