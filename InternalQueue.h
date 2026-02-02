// InternalQueue.h
#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>
#include "Trade.h"

class InternalQueue {
private:
    std::queue<Trade> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    bool m_shutdown{false};

public:
    void push(Trade obj) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_queue.push(std::move(obj));
        }
        m_cv.notify_one();
    }

    Trade pop() {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [this] { return !m_queue.empty() || m_shutdown; });
        
        
        Trade temp = std::move(m_queue.front());
        m_queue.pop();
        return temp;
    }

    bool empty() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

    void shutdown() {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_shutdown = true;
        }
        m_cv.notify_all();
    }
};