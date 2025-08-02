// src/common/concurrent_queue.h
#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <memory>

template <typename T>
class ConcurrentQueue {
private:
    mutable std::mutex mutex_;
    std::queue<T> queue_;
    std::condition_variable condition_;

public:
    ConcurrentQueue() = default;
    ~ConcurrentQueue() = default;
    
    // Copy constructor
    ConcurrentQueue(const ConcurrentQueue& other) {
        std::lock_guard<std::mutex> lock(other.mutex_);
        queue_ = other.queue_;
    }
    
    // Assignment operator
    ConcurrentQueue& operator=(const ConcurrentQueue& other) {
        if (this != &other) {
            std::lock_guard<std::mutex> lock(mutex_);
            std::lock_guard<std::mutex> other_lock(other.mutex_);
            queue_ = other.queue_;
        }
        return *this;
    }
    
    // Push an element to the queue
    void push(T value) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(std::move(value));
        condition_.notify_one();
    }
    
    // Try to pop an element from the queue (non-blocking)
    bool try_pop(T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return false;
        }
        value = std::move(queue_.front());
        queue_.pop();
        return true;
    }
    
    // Pop an element from the queue (blocking)
    void pop(T& value) {
        std::unique_lock<std::mutex> lock(mutex_);
        condition_.wait(lock, [this] { return !queue_.empty(); });
        value = std::move(queue_.front());
        queue_.pop();
    }
    
    // Check if queue is empty
    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }
    
    // Get queue size
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }
    
    // Clear the queue
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        while (!queue_.empty()) {
            queue_.pop();
        }
    }
};