#ifndef THREAD_SAFE_QUEUE_H
#define THREAD_SAFE_QUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <string>

class ThreadSafeQueue {
private:
    std::queue<std::string> queue_;
    std::mutex mutex_;
    std::condition_variable cond_var_;

public:
    void push(const std::string& message) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(message);
        
        // Wake up exactly one waiting consumer thread
        cond_var_.notify_one(); 
    }

    std::string wait_and_pop() {
        std::unique_lock<std::mutex> lock(mutex_);
        
        // Yield CPU and sleep until the queue has data
        cond_var_.wait(lock, [this]() { return !queue_.empty(); });
        
        std::string message = queue_.front();
        queue_.pop();
        
        return message;
    }
};

#endif