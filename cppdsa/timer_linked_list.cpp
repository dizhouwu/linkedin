#include <iostream>
#include <chrono>
#include <functional>
#include <thread>
#include <mutex>
#include <random>
#include <atomic>
#include <iomanip>
#include <ctime>
#include <memory>

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
    std::unique_ptr<TimerNode> next;

    TimerNode(TimePoint time, TimerCallback cb) 
        : expirationTime(time), callback(cb), next(nullptr) {}
};

// Timer list manager with thread safety
class TimerList {
public:
    TimerList() : head(nullptr), stop(false) {}

    void addTimer(TimePoint start, int intervalMillis, TimerCallback callback) {
        auto expiration = start + std::chrono::milliseconds(intervalMillis);
        auto newNode = std::make_unique<TimerNode>(expiration, callback);

        std::lock_guard<std::mutex> lock(mtx);
        if (!head || expiration < head->expirationTime) {
            newNode->next = std::move(head);
            head = std::move(newNode);
        } else {
            TimerNode* current = head.get();
            while (current->next && current->next->expirationTime < expiration) {
                current = current->next.get();
            }
            newNode->next = std::move(current->next);
            current->next = std::move(newNode);
        }
    }

    void tick() {
        while (!stop) {
            TimePoint now = Clock::now();
            {
                std::lock_guard<std::mutex> lock(mtx);
                while (head && head->expirationTime <= now) {
                    TimerNode* expired = head.release();
                    expired->callback();
                    head = std::move(expired->next);
                    delete expired;
                }
            }
            std::this_thread::yield();
        }
    }

    void stopTicking() {
        stop = true;
    }

private:
    std::unique_ptr<TimerNode> head;
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
        ohlcvCopy = ohlcv;
        ohlcv = OHLCV();
    }

    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm* now_tm = std::localtime(&now_c);

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

    // Define market start and end times on the current date
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::cout << "Now: " << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M:%S") << std::endl;

    std::tm marketOpenTm = *std::localtime(&now_c);
    marketOpenTm.tm_hour = 22;
    marketOpenTm.tm_min = 22;
    marketOpenTm.tm_sec = 0;
    std::tm marketCloseTm = marketOpenTm;
    marketCloseTm.tm_hour = 23;
    marketCloseTm.tm_min = 0;

    auto marketOpen = std::chrono::system_clock::from_time_t(std::mktime(&marketOpenTm));
    std::time_t marketOpenTime = std::chrono::system_clock::to_time_t(marketOpen);
    std::cout << "Market open: " << std::put_time(std::localtime(&marketOpenTime), "%Y-%m-%d %H:%M:%S") << std::endl;
    auto marketClose = std::chrono::system_clock::from_time_t(std::mktime(&marketCloseTm));
    std::time_t marketCloseTime = std::chrono::system_clock::to_time_t(marketClose);
    std::cout << "Market close: " << std::put_time(std::localtime(&marketCloseTime), "%Y-%m-%d %H:%M:%S") << std::endl;
    // Calculate start and end durations from now, converted to steady_clock
    auto startDuration = std::chrono::duration_cast<std::chrono::milliseconds>(marketOpen - now);
    auto endDuration = std::chrono::duration_cast<std::chrono::milliseconds>(marketClose - now);

    if (endDuration.count() < 0) {
        std::cerr << "Market is closed. Program will exit." << std::endl;
        return 1;
    }

    // Set timers for each second during market hours
    int intervalMillis = 1000;
    for (auto elapsed = startDuration; elapsed < endDuration; elapsed += std::chrono::milliseconds(intervalMillis)) {
        timerList.addTimer(Clock::now() + elapsed, intervalMillis, [&ohlcvData, &ohlcvMutex]() {
            outputOHLCV(ohlcvData, ohlcvMutex);
        });
    }
    std::cout << "timer set" << std::endl;

    // Start the timer processing thread
    std::thread timerThread(&TimerList::tick, &timerList);

    // Simulate receiving data packets in a separate thread
    std::thread dataThread([&ohlcvData, &ohlcvMutex, &running]() {
        while (running) {
            mockProcessPacket(ohlcvData, ohlcvMutex);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    // Run the simulation for the market hours duration
    std::this_thread::sleep_for(endDuration);
    running = false;
    timerList.stopTicking();

    // Clean up
    timerThread.join();
    dataThread.join();

    return 0;
}
