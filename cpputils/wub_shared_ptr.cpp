#include <iostream>
#include <atomic>
#include <utility> 

template<typename T>
class SharedPtr {
public:
    // Constructor
    explicit SharedPtr(T* ptr = nullptr) 
        : ptr_(ptr), ref_count_(ptr ? new std::atomic<int>(1) : nullptr) {}

    // Copy constructor
    SharedPtr(const SharedPtr& other) noexcept 
        : ptr_(other.ptr_), ref_count_(other.ref_count_) {
        if (ref_count_) {
            ref_count_->fetch_add(1, std::memory_order_relaxed); // Atomic increment
        }
    }

    // Move constructor
    SharedPtr(SharedPtr&& other) noexcept 
        : ptr_(other.ptr_), ref_count_(other.ref_count_) {
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
                ref_count_->fetch_add(1, std::memory_order_relaxed); // Atomic increment
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
    bool unique() const { return ref_count_ && ref_count_->load(std::memory_order_relaxed) == 1; }

    // Get the use count
    int use_count() const { return ref_count_ ? ref_count_->load(std::memory_order_relaxed) : 0; }

    // Helper function to release ownership
    void release() {
        if (ref_count_) {
            if (ref_count_->fetch_sub(1, std::memory_order_acq_rel) == 1) { // Atomic decrement
                delete ptr_;
                delete ref_count_;
            }
        }
        ptr_ = nullptr;
        ref_count_ = nullptr;
    }

private:
    T* ptr_;                      // Raw pointer to the managed object
    std::atomic<int>* ref_count_; // Atomic reference count pointer
};



/*************  ✨ Codeium Command ⭐  *************/
/**
 * my_make_shared is a helper function to create a SharedPtr object.
 * It uses placement new to construct the object and wrap it in a SharedPtr.
 * This function is similar to std::make_shared, but it does not use the atomic
 * reference count optimization.
 *
 * @param args The arguments to the constructor of type T.
 * @return A SharedPtr object that manages the constructed object.
 */
/******  39c38dc7-06b3-4e60-a7f3-b5201295282d  *******/
template<typename T, typename... Args>
SharedPtr<T> my_make_shared(Args&&... args) {
    T* ptr = new T(std::forward<Args>(args)...);

    return SharedPtr<T>(ptr);
}