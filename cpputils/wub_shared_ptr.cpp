#include <iostream>

template<typename T>
class SharedPtr {
public:
    // Constructor
    explicit SharedPtr(T* ptr = nullptr) : ptr_(ptr), ref_count_(ptr ? new int(1) : nullptr) {}

    // Copy constructor
    SharedPtr(const SharedPtr& other) noexcept : ptr_(other.ptr_), ref_count_(other.ref_count_) {
        if (ref_count_) {
            ++(*ref_count_); // Increment reference count
        }
    }

    // Move constructor
    SharedPtr(SharedPtr&& other) noexcept : ptr_(other.ptr_), ref_count_(other.ref_count_) {
        other.ptr_ = nullptr;
        other.ref_count_ = nullptr;
    }

    // Copy assignment operator
    SharedPtr& operator=(const SharedPtr& other) noexcept {
        if (this != &other) {
            release(); // Decrement the current ref count and delete if necessary

            ptr_ = other.ptr_;
            ref_count_ = other.ref_count_;
            if (ref_count_) {
                ++(*ref_count_); // Increment new reference count
            }
        }
        return *this;
    }

    // Move assignment operator
    SharedPtr& operator=(SharedPtr&& other) noexcept {
        if (this != &other) {
            release(); // Release current ownership

            ptr_ = other.ptr_;
            ref_count_ = other.ref_count_;
            other.ptr_ = nullptr;
            other.ref_count_ = nullptr;
        }
        return *this;
    }

    // Destructor
    ~SharedPtr() { release(); }

    // Dereference operators
    T& operator*() const { return *ptr_; }
    T* operator->() const { return ptr_; }

    // Get raw pointer
    T* get() const { return ptr_; }

    // Check if it's the only owner
    bool unique() const { return ref_count_ && *ref_count_ == 1; }

    // Get the use count
    int use_count() const { return ref_count_ ? *ref_count_ : 0; }


    // Helper function to release ownership
    void release() {
        if (ref_count_) {
            if (--(*ref_count_) == 0) { // Decrement ref count and check if it's zero
                delete ptr_;
                delete ref_count_;
            }
        }
        ptr_ = nullptr;
        ref_count_ = nullptr;
    }

private:
    T* ptr_;          // Raw pointer to the managed object
    int* ref_count_;  // Pointer to the reference count


};
