#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <condition_variable>


const int QUEUE_CAPACITY = 1024;
const int TOTAL_ORDERS = 10'000'000;
const int PRODUCER_COUNT = 4;
const int CONSUMER_COUNT = 4;

struct Order {
    int id;
};

template <typename F>
long long measure(F f) {
    auto start = std::chrono::high_resolution_clock::now();
    f();
    auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}

class MutexQueue {
    Order buffer[QUEUE_CAPACITY];
    int head = 0;
    int tail = 0;
    int count = 0;
    std::mutex mtx;
    std::condition_variable not_full;
    std::condition_variable not_empty;

public:
    void push(Order item) {
        std::unique_lock<std::mutex> lock(mtx);
        not_full.wait(lock, [this] 
            { return count < QUEUE_CAPACITY; });
        buffer[tail % QUEUE_CAPACITY] = item;
        tail++;
        count++;
        not_empty.notify_one();
    }

    Order pop() {
        std::unique_lock<std::mutex> lock(mtx);
        not_empty.wait(lock, [this] 
            { return count > 0; });
        Order item = buffer[head % QUEUE_CAPACITY];
        head++;
        count--;
        not_full.notify_one();
        return item;
    }
};

struct Node {
    std::atomic<size_t> sequence;
    Order data;
};

class LockFreeQueue {
    alignas(64) Node buffer[QUEUE_CAPACITY];
    alignas(64) std::atomic<size_t> head_idx;
    alignas(64) std::atomic<size_t> tail_idx;
    const size_t mask = QUEUE_CAPACITY - 1;

public:
    LockFreeQueue() : head_idx(0), tail_idx(0) {
        for (size_t i = 0; i < QUEUE_CAPACITY; i++) {
            buffer[i].sequence.store(i, std::memory_order_relaxed);
        }
    }

    void push(Order item) {
        Node* node;
        size_t pos = tail_idx.load(std::memory_order_relaxed);
        while(true) {
            node = &buffer[pos & mask];
            size_t seq = node->sequence.load(std::memory_order_acquire);
            intptr_t diff = (intptr_t)seq - (intptr_t)pos;

            if (diff == 0) {
                if (tail_idx.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
                    break;
                }
            } else if (diff < 0) {
                pos = tail_idx.load(std::memory_order_relaxed);
            } else {
                pos = tail_idx.load(std::memory_order_relaxed);
            }
            
            std::this_thread::yield();
        }
        node->data = item;
        node->sequence.store(pos + 1, std::memory_order_release);
    }

    Order pop() {
        Node* node;
        size_t pos = head_idx.load(std::memory_order_relaxed);
        for (;;) {
            node = &buffer[pos & mask];
            size_t seq = node->sequence.load(std::memory_order_acquire);
            intptr_t diff = (intptr_t)seq - (intptr_t)(pos + 1);

            if (diff == 0) {
                if (head_idx.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)) {
                    break;
                }
            } else if (diff < 0) {
                pos = head_idx.load(std::memory_order_relaxed);
            } else {
                pos = head_idx.load(std::memory_order_relaxed);
            }
            
            std::this_thread::yield();
        }
        Order item = node->data;
        node->sequence.store(pos + QUEUE_CAPACITY, std::memory_order_release);
        return item;
    }
};

void runMutexVersion() {
    MutexQueue queue;
    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;
    std::atomic<int> consumed_count{0};

    for (int i = 0; i < PRODUCER_COUNT; i++) {
        producers.emplace_back([&queue] {
            for (int j = 0; j < TOTAL_ORDERS / PRODUCER_COUNT; j++) {
                queue.push({1});
            }
        });
    }

    for (int i = 0; i < CONSUMER_COUNT; i++) {
        consumers.emplace_back([&queue, &consumed_count] {
            for (int j = 0; j < TOTAL_ORDERS / CONSUMER_COUNT; j++) {
                queue.pop();
                consumed_count.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    for (auto& t : producers) t.join();
    for (auto& t : consumers) t.join();
}

void runLockFreeVersion() {
    LockFreeQueue queue;
    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;
    std::atomic<int> consumed_count{0};

    for (int i = 0; i < PRODUCER_COUNT; i++) {
        producers.emplace_back([&queue] {
            for (int j = 0; j < TOTAL_ORDERS / PRODUCER_COUNT; j++) {
                queue.push({1});
            }
        });
    }

    for (int i = 0; i < CONSUMER_COUNT; i++) {
        consumers.emplace_back([&queue, &consumed_count] {
            for (int j = 0; j < TOTAL_ORDERS / CONSUMER_COUNT; j++) {
                queue.pop();
                consumed_count.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    for (auto& t : producers) t.join();
    for (auto& t : consumers) t.join();
}

int main() {
    std::cout << "Starting performance comparison..." << std::endl;

    long long mutex_time = measure(runMutexVersion);
    std::cout << "Mutex-based Queue: " << mutex_time << " ms" << std::endl;

    long long lock_free_time = measure(runLockFreeVersion);
    std::cout << "Lock-free Queue: " << lock_free_time << " ms" << std::endl;

    double speedup = (double)mutex_time / lock_free_time;
    double throughput = (TOTAL_ORDERS / (lock_free_time / 1000.0)) / 1'000'000.0;

    std::cout << "\nResults:" << std::endl;
    std::cout << "Speedup: " << speedup << "x" << std::endl;
    std::cout << "Lock-free Throughput: " << throughput << " M orders/sec" << std::endl;

    return 0;
}