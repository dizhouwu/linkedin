#include <atomic>
#include <vector>
#include <stdexcept>
#include <optional>
#include <iostream>

template<typename T>
class LocklessRingBuffer {
public:
    explicit LocklessRingBuffer(size_t size) : size_(size + 1), buffer_(size + 1), head_(0), tail_(0) {
        if (size_ < 2) { // Adjusted to ensure at least one usable slot
            throw std::invalid_argument("Size must be greater than 0");
        }
    }

    bool push(const T& item) {
        const size_t current_head = head_.load(std::memory_order_relaxed);
        const size_t next_head = (current_head + 1) % size_;

        // Check if the buffer is full
        if (next_head == tail_.load(std::memory_order_acquire)) {
            return false; // Buffer is full
        }

        // Insert the item
        buffer_[current_head] = item;
        head_.store(next_head, std::memory_order_release);
        return true; // Item successfully added
    }

    std::optional<T> pop() {
        const size_t current_tail = tail_.load(std::memory_order_relaxed);

        // Check if the buffer is empty
        if (current_tail == head_.load(std::memory_order_acquire)) {
            return std::nullopt; // Buffer is empty
        }

        // Remove the item
        T item = buffer_[current_tail];
        tail_.store((current_tail + 1) % size_, std::memory_order_release);
        return item; // Return the popped item
    }

    bool is_empty() const {
        return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
    }

    bool is_full() const {
        return (head_.load(std::memory_order_acquire) + 1) % size_ == tail_.load(std::memory_order_acquire);
    }

private:
    std::vector<T> buffer_;
    const size_t size_;
    std::atomic<size_t> head_;
    std::atomic<size_t> tail_;
};
