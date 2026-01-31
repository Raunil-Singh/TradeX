#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <chrono>
#include <atomic>
#include <mutex>
#include <random>
#include <condition_variable>
#include <functional>
#include <queue>
#include <future>
#include <cstdlib>
#include <shared_mutex>
#include <algorithm>
using namespace std;

const int QUEUE_CAPACITY = 1024;
const int TOTAL_ORDERS = 10'000'000;
const int NUM_PRODUCERS = 4;
const int NUM_CONSUMERS = 4;

struct Order {
    int id;
};

class Queue {
private:
    struct Node {
        Order order;
        atomic<int> seq{0}; //Sequence number for this slot
    };
    vector<Node> buffer;
    atomic<int> head{0};
    atomic<int> tail{0};
    static constexpr int INDEX_MASK = QUEUE_CAPACITY - 1; //Bitmask for fast modulo since QUEUE_CAPACITY is power of 2
    int index(int pos) {
        return INDEX_MASK & pos;
    }

public:
    Queue() : buffer(QUEUE_CAPACITY) {
        for (int i = 0; i < QUEUE_CAPACITY; i++) {
            buffer[i].seq.store(i, memory_order_relaxed);
        }
    }
    bool try_enqueue(const Order& order) {
        int current_tail;
        while (true) {
            current_tail = tail.load(memory_order_relaxed);
            int idx = index(current_tail);
            int slot_seq = buffer[idx].seq.load(memory_order_acquire);
            int diff = slot_seq - current_tail;
            if (diff == 0) {
                if (tail.compare_exchange_weak(current_tail, current_tail + 1)) {
                    buffer[idx].order = order;
                    buffer[idx].seq.store(current_tail + 1, memory_order_release);
                    return true;
                }
            } else if (diff < 0) {
                // Slot is still full (consumer hasn't read it yet)
                // Queue is full
                return false;
            } else {
                // diff > 0: Tail moved forward, retry with new value
            }
        }
    }
    bool try_dequeue(Order& order) {
        int current_head;
        while (true) {
            current_head = head.load(memory_order_relaxed);
            int idx = index(current_head);
            int slot_seq = buffer[idx].seq.load(memory_order_acquire);
            int diff = slot_seq - (current_head + 1);
            if (diff == 0) {
                if (head.compare_exchange_weak(current_head, current_head + 1)) {
                    order = buffer[idx].order;
                    buffer[idx].seq.store(current_head + QUEUE_CAPACITY, memory_order_release);
                    return true;
                }
            } else if (diff < 0) {
                // Slot is empty (producer hasn't written yet)
                // Queue is empty
                return false;
            } else {
                // diff > 0: Head moved forward, retry with new value
            }
        }
    }
    int size() {
        int t = tail.load(memory_order_relaxed);
        int h = head.load(memory_order_relaxed);
        return t - h;
    }
};

class MutexQueue {
private:
    queue<Order> q;
    mutex mtx;
    
public:
    bool try_enqueue(const Order& order) {
        lock_guard<mutex> lock(mtx);
        if (q.size() >= QUEUE_CAPACITY) {
            return false;
        }
        q.push(order);
        return true;
    }
    bool try_dequeue(Order& order) {
        lock_guard<mutex> lock(mtx);
        if (q.empty()) {
            return false;
        }
        order = q.front();
        q.pop();
        return true;
    }
    int size() {
        lock_guard<mutex> lock(mtx);
        return q.size();
    }
};

template<typename QueueType>
void producer(QueueType& queue, int producer_id, int num_orders, atomic<int>& produced_count, atomic<int>& retry_count) {
    int produced = 0;
    int retries = 0;
    int backoff = 1;
    
    while (produced < num_orders) {
        Order order;
        order.id = producer_id * num_orders + produced;
        
        if (queue.try_enqueue(order)) {
            produced++;
            produced_count.fetch_add(1, memory_order_relaxed);
            backoff = 1; 
        } else {
            retries++;
            retry_count.fetch_add(1, memory_order_relaxed);
            for (int i = 0; i < backoff; i++) {
                asm volatile("yield");
            }
            backoff = min(backoff * 2, 1024); 
        }
    }
}

template<typename QueueType>
void consumer(QueueType& queue, atomic<int>& consumed_count,atomic<bool>& done, vector<int>& received_ids) {
    int backoff = 1;
    while (!done.load(memory_order_relaxed) || queue.size() > 0) {
        Order order;
        if (queue.try_dequeue(order)) {
            consumed_count.fetch_add(1, memory_order_relaxed);
            received_ids.push_back(order.id);
            backoff = 1;
        } else {
            for (int i = 0; i < backoff; i++) {
                asm volatile("yield");
            }
            backoff = min(backoff * 2, 1024);
        }
    }
}

template<typename QueueType>
void run_benchmark(const string& name) {
    cout << "Running " << name << endl;
    QueueType queue;
    atomic<int> produced_count{0};
    atomic<int> consumed_count{0};
    atomic<int> retry_count{0};
    atomic<bool> done{false};
    
    int orders_per_producer = TOTAL_ORDERS / NUM_PRODUCERS;
    vector<vector<int>> consumer_ids(NUM_CONSUMERS);
    
    auto start = chrono::high_resolution_clock::now();
    
    vector<thread> producers;
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        producers.emplace_back(producer<QueueType>, ref(queue), i,orders_per_producer, ref(produced_count), ref(retry_count));
    }
    
    vector<thread> consumers;
    for (int i = 0; i < NUM_CONSUMERS; i++) {
        consumers.emplace_back(consumer<QueueType>, ref(queue), ref(consumed_count), ref(done), ref(consumer_ids[i]));
    }

    for (auto& t : producers) t.join();
    done.store(true, memory_order_release);
    for (auto& t : consumers) t.join();
    
    auto end = chrono::high_resolution_clock::now();
    double time_ms = chrono::duration<double, milli>(end - start).count();
    
    vector<int> all_ids;
    for (const auto& vec : consumer_ids) {
        all_ids.insert(all_ids.end(), vec.begin(), vec.end());
    }
    sort(all_ids.begin(), all_ids.end());
    
    bool no_duplicate_ids = (adjacent_find(all_ids.begin(), all_ids.end()) == all_ids.end());
    bool correct_count = (all_ids.size() == TOTAL_ORDERS);
    
    cout << "Time: " << time_ms << " ms" << endl;
    
    cout << "Verification: ";
    cout << (no_duplicate_ids && correct_count ? "PASS" : "FAIL") << endl;
}

int main() {
    cout << "Running benchmark..." << endl;
    
    run_benchmark<MutexQueue>("Mutex-Based Queue");
    run_benchmark<Queue>("Lock-Free Queue");
    
    return 0;
}
