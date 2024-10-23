#include <iostream>
#include <utility> // For std::exchange

template<typename T>
class UniquePtr {
public:
    // Constructor
    explicit UniquePtr(T* ptr = nullptr) : ptr_(ptr) {}

    // Destructor
    ~UniquePtr() { delete ptr_; }

    // Move constructor
    UniquePtr(UniquePtr&& other) noexcept : ptr_(other.ptr_) {
        other.ptr_ = nullptr; // Transfer ownership
    }

    // Move assignment operator
    UniquePtr& operator=(UniquePtr&& other) noexcept {
        if (this != &other) {
            delete ptr_; // Clean up current resource
            ptr_ = other.ptr_; // Transfer ownership
            other.ptr_ = nullptr;
        }
        return *this;
    }

    // Delete copy constructor and copy assignment operator
    UniquePtr(const UniquePtr&) = delete;
    UniquePtr& operator=(const UniquePtr&) = delete;

    // Dereference operators
    T& operator*() const { return *ptr_; }
    T* operator->() const { return ptr_; }

    // Get raw pointer
    T* get() const { return ptr_; }

    // Release ownership of the pointer
    T* release() {
        return std::exchange(ptr_, nullptr); // Set ptr_ to nullptr and return the old value
    }

    // Reset the unique_ptr with a new pointer
    void reset(T* ptr = nullptr) {
        delete ptr_; // Clean up existing resource
        ptr_ = ptr;  // Assign new pointer
    }

private:
    T* ptr_; // Raw pointer
};

