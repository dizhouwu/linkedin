#include <memory>
#include <vector>
#include <mutex>
#include <atomic>
#include <iostream>
#include <thread>
#include <chrono>
#include "concurrentqueue.h"

template<typename T>
class Broadcaster {
public:
    class Subscriber {
    private:
        moodycamel::ConcurrentQueue<std::shared_ptr<const T>> queue;
        std::atomic<bool> active{true};
        friend class Broadcaster;

    public:
        bool receive(std::shared_ptr<const T>& item) {
            return queue.try_dequeue(item);
        }

        bool is_active() const {
            return active;
        }
    };

private:
    std::vector<std::weak_ptr<Subscriber>> subscribers;
    std::mutex subscriberMutex;

public:
    void broadcast(const T& item) {
        auto sharedItem = std::make_shared<T>(item);
        std::lock_guard<std::mutex> lock(subscriberMutex);
        for (auto it = subscribers.begin(); it != subscribers.end();) {
            if (auto sub = it->lock()) {
                if (sub->active) {
                    sub->queue.enqueue(sharedItem);
                    ++it;
                } else {
                    it = subscribers.erase(it);
                }
            } else {
                it = subscribers.erase(it);
            }
        }
    }

    std::shared_ptr<Subscriber> subscribe() {
        auto sub = std::make_shared<Subscriber>();
        std::lock_guard<std::mutex> lock(subscriberMutex);
        subscribers.push_back(sub);
        return sub;
    }

    void unsubscribe(const std::shared_ptr<Subscriber>& sub) {
        sub->active = false;
    }
};

std::mutex coutMutex;

void safePrint(const std::string& message) {
    std::lock_guard<std::mutex> lock(coutMutex);
    std::cout << message << std::endl;
}

int main() {
    Broadcaster<int> broadcaster;

    // Consumer 1
    auto sub1 = broadcaster.subscribe();
    std::thread consumer1([sub1]() {
        std::shared_ptr<const int> item;
        while (sub1->is_active()) {
            if (sub1->receive(item)) {
                safePrint("Consumer 1 received: " + std::to_string(*item));
            }
        }
    });

    // Consumer 2
    auto sub2 = broadcaster.subscribe();
    std::thread consumer2([sub2]() {
        std::shared_ptr<const int> item;
        while (sub2->is_active()) {
            if (sub2->receive(item)) {
                safePrint("Consumer 2 received: " + std::to_string(*item));
            }
        }
    });

    // Producer
    for (int i = 0; i < 10; ++i) {
        broadcaster.broadcast(i);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Cleanup
    broadcaster.unsubscribe(sub1);
    broadcaster.unsubscribe(sub2);
    consumer1.join();
    consumer2.join();

    return 0;
}
