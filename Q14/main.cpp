#include <atomic>
#include <thread>
#include <utility>
#include <array>
#include <vector>
#include <cstdint>
#include <chrono>
#include <iostream>

#if defined(__x86_64__) || defined(_M_X64)
  #include <immintrin.h>
#endif

struct Backoff {
    unsigned count = 0;

    inline void pause() {
        // Cap exponential growth (prevents runaway delays)
        constexpr unsigned MAX_SHIFT = 10;   // 2^10 = 1024 pauses

        unsigned spins = 1u << (count < MAX_SHIFT ? count : MAX_SHIFT);

        // Busy-spin with CPU hint
        for (unsigned i = 0; i < spins; ++i) {
#if defined(__x86_64__) || defined(_M_X64)
            _mm_pause();        // ~10–20 cycles
#elif defined(__aarch64__)
            asm volatile("yield");
#else
            asm volatile("" ::: "memory");
#endif
        }

        // After enough failures, politely yield to OS
        if (count > MAX_SHIFT) {
            std::this_thread::yield();
        }

        ++count;
    }

    inline void reset() {
        count = 0;
    }
};


constexpr int QUEUE_CAPACITY{1023};
constexpr int ORDERS{10'000'000};
constexpr int PTC{4}; // Producer Thread Count
constexpr int CTC{4}; // Consumer thread count

std::array<std::atomic_uint64_t, QUEUE_CAPACITY + 1> circularQueue;
struct alignas(64) paddedAtomicInt{
    std::atomic_uint64_t var;
    paddedAtomicInt(int value):
    var(value)
    {}
};
paddedAtomicInt head{0};
paddedAtomicInt tail{0};
std::atomic<int> producerCount{PTC};

void producer()
{
    Backoff backoff;
    uint64_t local_head{head.var.load(std::memory_order_relaxed)};
    uint64_t local_tail{tail.var.load(std::memory_order_relaxed)};
    int orderCount{0};
    while (orderCount < ORDERS/PTC)
    {
        uint64_t write_index{local_tail & QUEUE_CAPACITY};
        if (local_tail != local_head + 1)
        {
            // we know locally, with the values we had, queue is not full
            if (local_tail == circularQueue[write_index].load(std::memory_order_acquire))
            {
                // first step of verification, whether our sequence and sequence at circularQueue match
                if (tail.var.compare_exchange_strong(local_tail, local_tail + 1, std::memory_order_relaxed))
                {
                    // Our hardware guarantee here. Now even if two competing threads reach here
                    backoff.reset();
                    orderCount++;
                    circularQueue[write_index].store(local_tail + 1, std::memory_order_release);
                }else{
                    backoff.pause();
                }
            }
            else{
                backoff.pause();
            }
        }
        local_tail = tail.var.load(std::memory_order_relaxed);
        local_head = head.var.load(std::memory_order_relaxed);
    }
    producerCount.fetch_sub(1, std::memory_order_release);
}

void consumer()
{
    Backoff backoff;
    uint64_t local_head{head.var.load(std::memory_order_relaxed)};
    uint64_t local_tail{tail.var.load(std::memory_order_relaxed)};
    while (true)
    {
        uint64_t write_index{local_head & QUEUE_CAPACITY};
        if (local_tail != local_head)
        {
            // we know locally, with the values we had, queue is not empty
            if (local_head == circularQueue[write_index].load(std::memory_order_acquire) - 1)
            {
                // first step of verification, whether our sequence and sequence at circularQueue match
                if (head.var.compare_exchange_strong(local_head, local_head + 1, std::memory_order_relaxed))
                {
                    // Our hardware guarantee here. Now even if two competing threads reach here
                    backoff.reset();
                    circularQueue[write_index].store(local_head + QUEUE_CAPACITY + 1, std::memory_order_release); // gonna set some random order_id here
                }else{
                    backoff.pause();
                }
            }else{
                backoff.pause();
            }
        }
        else
        {
            if (producerCount.load(std::memory_order_acquire) == 0)
            {
                break;
            }
        }
        local_tail = tail.var.load(std::memory_order_relaxed);
        local_head = head.var.load(std::memory_order_relaxed);
    }
}

int main()
{
    for (int i = 0; i <= QUEUE_CAPACITY; i++)
    {
        circularQueue[i] = i;
    }
    std::vector<std::thread> producerThreadPool;
    std::vector<std::thread> consumerThreadPool;
    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < PTC; i++)
    {
        producerThreadPool.emplace_back(producer);
    }

    for (int i = 0; i < PTC; i++)
    {
        consumerThreadPool.emplace_back(consumer);
    }
    for (auto &thread : producerThreadPool)
    {
        thread.join();
    }
    for (auto &thread : consumerThreadPool)
    {
        thread.join();
    }
    auto end = std::chrono::steady_clock::now();

    std::cout << "Orders processed: " << tail.var.load() << "in time: " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << ". Throughput = " << static_cast<double>((tail.var.load() / std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()) * 1e6) << "\t \n";
}
