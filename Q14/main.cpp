#include <iostream>
#include <thread>
#include <chrono>
#include <random>
#include <vector>
#include <atomic>
#include <algorithm>
#include <mutex>
#include <condition_variable>

class Order
{
public:
    int order_id;
    double price;
    int quantity;
    uint64_t sequence_number;

    Order() : order_id(0), price(0.0), quantity(0), sequence_number(0) {}

    Order(int id) : order_id(id)
    {
        thread_local std::mt19937 rng(std::random_device{}());
        std::uniform_real_distribution<double> price_dist(100.0, 200.0);
        std::uniform_int_distribution<int> qty_dist(10, 1000);

        price = price_dist(rng);
        quantity = qty_dist(rng);
        sequence_number = 0;
    }
};

class Queue
{
private:
    static const int CAPACITY = 1024;
    std::atomic<uint64_t> enqueue_attempts;
    std::atomic<uint64_t> enqueue_failures;
    std::atomic<uint64_t> dequeue_attempts;
    std::atomic<uint64_t> dequeue_failures;

    struct Cell
    {
        std::atomic<int> seq;
        Order data;
    };

    alignas(64) Cell buffer[CAPACITY];
    alignas(64) std::atomic<int> head{0};
    alignas(64) std::atomic<int> tail{0};

public:
    Queue()
    {
        for (int i = 0; i < CAPACITY; i++)
        {
            buffer[i].seq.store(i, std::memory_order_relaxed);
        }
    }

    bool try_enqueue(const Order &order)
    {
        int pos = tail.load(std::memory_order_relaxed);
        enqueue_attempts.fetch_add(1,std::memory_order_relaxed);

        while (true)
        {
            Cell &cell = buffer[pos % CAPACITY];
            int seq = cell.seq.load(std::memory_order_acquire);
            int diff = seq - pos;

            if (diff == 0)
            {
                if (tail.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed))
                {
                    break;
                }
            }
            else if (diff < 0)
            {
                enqueue_failures.fetch_add(1, std::memory_order_relaxed);
                return false;
            }
            else
            {
                enqueue_failures.fetch_add(1, std::memory_order_relaxed);   
                pos = tail.load(std::memory_order_relaxed);
            }

            std::this_thread::yield();
        }

        buffer[pos % CAPACITY].data = order;
        buffer[pos % CAPACITY].seq.store(pos + 1, std::memory_order_release);
        return true;
    }

    bool try_dequeue(Order &order)
    {
        dequeue_attempts.fetch_add(1, std::memory_order_relaxed);
        int pos = head.load(std::memory_order_relaxed);

        while (true)
        {
            Cell &cell = buffer[pos % CAPACITY];
            int seq = cell.seq.load(std::memory_order_acquire);
            int diff = seq - (pos + 1);

            if (diff == 0)
            {
                if (head.compare_exchange_weak(pos, pos + 1, std::memory_order_relaxed)){
                    break;
                }
            }
            else if (diff < 0)
            {
                dequeue_failures.fetch_add(1, std::memory_order_relaxed);
                return false;
            }
            else
            {
                dequeue_failures.fetch_add(1, std::memory_order_relaxed);
                pos = head.load(std::memory_order_relaxed);
            }

            std::this_thread::yield();
        }

        order = buffer[pos % CAPACITY].data;
        buffer[pos % CAPACITY].seq.store(pos + CAPACITY, std::memory_order_release);
        return true;
    }

    void print(){
        double enqueue_contention = 100 * enqueue_failures.load()/enqueue_attempts.load();
        double dequeue_contention = 100 * dequeue_failures.load()/dequeue_attempts.load();
        std::cout<<"Enqueue : "<<enqueue_contention<<"% | Dequeue : "<<dequeue_contention<<"%\n";
    }
};


class MutexQueue
{
private:
    static const int CAPACITY = 1024;
    Order buffer[CAPACITY];

    int head = 0;
    int tail = 0;
    int count = 0;

    std::mutex mtx;
    std::condition_variable not_full;
    std::condition_variable not_empty;

    std::atomic<uint64_t> enqueue_attempts{0};
    std::atomic<uint64_t> enqueue_waits{0};
    std::atomic<uint64_t> dequeue_attempts{0};
    std::atomic<uint64_t> dequeue_waits{0};

public:
    void enqueue(const Order &order)
    {
        std::unique_lock<std::mutex> lock(mtx);
        enqueue_attempts++;

        while (count == CAPACITY)
        {
            enqueue_waits++;
            not_full.wait(lock);
        }

        buffer[tail] = order;
        tail = (tail + 1) % CAPACITY;
        count++;

        not_empty.notify_one();
    }

    bool dequeue(Order &order, std::atomic<int> &active_producers)
    {
        std::unique_lock<std::mutex> lock(mtx);
        dequeue_attempts++;

        while (count == 0)
        {
            if (active_producers.load(std::memory_order_acquire) == 0)
            {
                return false;
            }
            dequeue_waits++;
            not_empty.wait(lock);
        }

        order = buffer[head];
        head = (head + 1) % CAPACITY;
        count--;

        not_full.notify_one();
        return true;
    }

    void print()
    {
        double enq_wait_pct =
            100.0 * enqueue_waits.load() / enqueue_attempts.load();
        double deq_wait_pct =
            100.0 * dequeue_waits.load() / dequeue_attempts.load();

        std::cout <<"Enqueue: " << enq_wait_pct << "% | "<< "Dequeue: " << deq_wait_pct << "%\n";
    }
};


const int TOTAL_ORDERS = 10000000;
Queue order_queue;
MutexQueue mutex_queue;

std::atomic<int> active_producers{4};
std::atomic<int> produced_count[4];
std::atomic<int> consumed_count[4];
std::atomic<uint64_t> next_sequence{0};

void producer(int producer_id, int base_order_id)
{
    int order_id = base_order_id;

    for (int i = 0; i < TOTAL_ORDERS / 4; i++)
    {
        Order order(order_id++);
        order.sequence_number = next_sequence.fetch_add(1, std::memory_order_relaxed);

        while (!order_queue.try_enqueue(order))
        {
            std::this_thread::yield();
        }

        produced_count[producer_id]++;
    }

    active_producers.fetch_sub(1, std::memory_order_release);
}

void consumer(int consumer_id)
{
    Order order;

    while (true)
    {
        if (order_queue.try_dequeue(order))
        {
            consumed_count[consumer_id]++;
        }
        else
        {
            if (active_producers.load(std::memory_order_acquire) == 0)
            {
                break;
            }
            std::this_thread::yield();
        }
    }
}

void producer_mutex(int producer_id, int base_order_id)
{
    int order_id = base_order_id;

    for (int i = 0; i < TOTAL_ORDERS / 4; i++)
    {
        Order order(order_id++);
        order.sequence_number = next_sequence.fetch_add(1, std::memory_order_relaxed);

        mutex_queue.enqueue(order);
        produced_count[producer_id]++;
    }

    active_producers.fetch_sub(1, std::memory_order_release);
}

void consumer_mutex(int consumer_id)
{
    Order order;

    while (true)
    {
        if (mutex_queue.dequeue(order, active_producers))
        {
            consumed_count[consumer_id]++;
        }
        else
        {
            break;
        }
    }
}

void verify()
{
    long long produced = 0, consumed = 0;

    for (int i = 0; i < 4; i++)
    {
        produced += produced_count[i];
        consumed += consumed_count[i];
    }

    if (produced == consumed)
    {
        std::cout << "Verification Success!!!!\n";
    }
    else
    {
        std::cout << "Verification Failed!!\n";
    }
}

void reset_counters()
{
    active_producers.store(4);
    next_sequence.store(0);

    for (int i = 0; i < 4; i++)
    {
        produced_count[i].store(0);
        consumed_count[i].store(0);
    }
}

void run_lock_free()
{
    long long avg_time = 0;

    for (int i = 0; i < 3; i++)
    {
        reset_counters();

        auto start = std::chrono::high_resolution_clock::now();

        std::thread p1(producer, 0, 1000);
        std::thread p2(producer, 1, 2000);
        std::thread p3(producer, 2, 3000);
        std::thread p4(producer, 3, 4000);

        std::thread c1(consumer, 0);
        std::thread c2(consumer, 1);
        std::thread c3(consumer, 2);
        std::thread c4(consumer, 3);

        p1.join();
        p2.join();
        p3.join();
        p4.join();

        c1.join();
        c2.join();
        c3.join();
        c4.join();

        auto end = std::chrono::high_resolution_clock::now();
        avg_time += std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    }

    avg_time /= 3;

    std::cout << "Execution Time : " << avg_time << " ms\n";
    verify();
    order_queue.print();
    // for (int i = 0; i < 4; i++)
    // {
    //     std::cout << "Producer " << i << " produced " << produced_count[i] << "\n";
    // }

    // for (int i = 0; i < 4; i++)
    // {
    //     std::cout << "Consumer " << i << " consumed " << consumed_count[i] << "\n";
    // }
}

void run_mutex()
{
    long long avg_time = 0;

    for (int i = 0; i < 3; i++)
    {
        reset_counters();

        auto start = std::chrono::high_resolution_clock::now();

        std::thread p1(producer_mutex, 0, 1000);
        std::thread p2(producer_mutex, 1, 2000);
        std::thread p3(producer_mutex, 2, 3000);
        std::thread p4(producer_mutex, 3, 4000);

        std::thread c1(consumer_mutex, 0);
        std::thread c2(consumer_mutex, 1);
        std::thread c3(consumer_mutex, 2);
        std::thread c4(consumer_mutex, 3);

        p1.join();
        p2.join();
        p3.join();
        p4.join();

        c1.join();
        c2.join();
        c3.join();
        c4.join();

        auto end = std::chrono::high_resolution_clock::now();
        avg_time += std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    }

    avg_time /= 3;

    std::cout << "Execution Time : " << avg_time << " ms\n";
    verify();
    mutex_queue.print();
}

int main()
{
    std::cout<<"LOCK FREE VERSION : \n";
    run_lock_free();
    std::cout<<"\nMUTEX BASED VERSION : \n";
    run_mutex();
    return 0;
}
