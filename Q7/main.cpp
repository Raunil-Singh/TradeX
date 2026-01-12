#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <atomic>
#include <random>
#include <chrono>


const int THREAD_COUNT = 8;
const int EVENTS_PER_THREAD = 1'000'000;


enum EventType
{
    ORDER = 0,
    TRADE = 1,
    CANCEL = 2
};


int order_count_int = 0;
int trade_count_int = 0;
int cancel_count_int = 0;

std::atomic<int> order_count_atomic{0};
std::atomic<int> trade_count_atomic{0};
std::atomic<int> cancel_count_atomic{0};

std::mutex global_mutex;

std::mutex order_mutex;
std::mutex trade_mutex;
std::mutex cancel_mutex;


template <typename F>
long long measure(F f)
{
    auto start = std::chrono::high_resolution_clock::now();
    f();
    auto end = std::chrono::high_resolution_clock::now();

    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}


void singleMutex()
{
    std::mt19937 mt(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, 2);

    for (int i = 0; i < EVENTS_PER_THREAD; ++i)
    {
        int event = dist(mt);

        std::lock_guard<std::mutex> lock(global_mutex);

        if (event == ORDER)
            order_count_int++;

        else if (event == TRADE)
            trade_count_int++;
            
        else
            cancel_count_int++;
    }
}


void multipleMutexes()
{
    std::mt19937 mt(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, 2);

    for (int i = 0; i < EVENTS_PER_THREAD; ++i)
    {
        int event = dist(mt);

        if (event == ORDER)
        {
            std::lock_guard<std::mutex> lock(order_mutex);
            order_count_int++;
        }
        else if (event == TRADE)
        {
            std::lock_guard<std::mutex> lock(trade_mutex);
            trade_count_int++;
        }
        else
        {
            std::lock_guard<std::mutex> lock(cancel_mutex);
            cancel_count_int++;
        }
    }
}


void atomic()
{
    std::mt19937 mt(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, 2);

    for (int i = 0; i < EVENTS_PER_THREAD; ++i)
    {
        int event = dist(mt);

        if (event == ORDER)
            order_count_atomic++;

        else if (event == TRADE)
            trade_count_atomic++;

        else
            cancel_count_atomic++;
    }
}


void atomicRelaxed()
{
    std::mt19937 mt(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, 2);

    for (int i = 0; i < EVENTS_PER_THREAD; ++i)
    {
        int event = dist(mt);

        if (event == ORDER)
            order_count_atomic.fetch_add(1, std::memory_order_relaxed);

        else if (event == TRADE)
            trade_count_atomic.fetch_add(1, std::memory_order_relaxed);

        else
            cancel_count_atomic.fetch_add(1, std::memory_order_relaxed);
    }
}


template <typename Worker>
void run(const std::string& name, Worker worker, bool use_atomic)
{
    order_count_int = 0;
    trade_count_int = 0;
    cancel_count_int = 0;

    order_count_atomic = 0;
    trade_count_atomic = 0;
    cancel_count_atomic = 0;

    std::vector<std::thread> threads;

    long long time_ms = measure([&] {
        for (int i = 0; i < THREAD_COUNT; ++i)
            threads.emplace_back(worker);

        for (auto& t : threads)
            t.join();
    });

    long long total = use_atomic
                    ? order_count_atomic + trade_count_atomic + cancel_count_atomic
                    : order_count_int + trade_count_int + cancel_count_int;

    double seconds = time_ms / 1000.0;
    double throughput = (THREAD_COUNT * EVENTS_PER_THREAD) / seconds;

    std::cout << "\n" << name << "\n";
    std::cout << "Time:       " << time_ms << " ms\n";
    std::cout << "Throughput: " << throughput << " ops/sec\n";
    std::cout << "Total ops:  " << total << "\n";
}


int main()
{
    std::cout << "\nResult:\n";

    run("Single mutex", singleMutex, false);

    run("Mutex for each counter", multipleMutexes, false);

    run("Atomic", atomic, true);

    run("Atomic with memory_order_relaxed", atomicRelaxed, true);

    std::cout << "\nExpected total ops: " << THREAD_COUNT * EVENTS_PER_THREAD << "\n\n";

    return 0;
}
