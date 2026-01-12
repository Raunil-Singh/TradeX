#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>

const int THREAD_COUNT = 8;
const int ITERATIONS = 10'000'000;
const int CACHE_LINE_SIZE = 64;


template <typename F>
long long measure(F f)
{
    auto start = std::chrono::high_resolution_clock::now();
    f();
    auto end = std::chrono::high_resolution_clock::now();

    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}


int counters_shared[THREAD_COUNT] = {};

void resetShared()
{
    for (int i = 0; i < THREAD_COUNT; ++i)
        counters_shared[i] = 0;
}

void sharedWorker(int thread_id)
{
    for (int i = 0; i < ITERATIONS; ++i)
        counters_shared[thread_id]++;
}

long long runShared()
{
    std::vector<std::thread> threads;

    return measure([&] {
        for (int i = 0; i < THREAD_COUNT; ++i)
            threads.emplace_back(sharedWorker, i);

        for (auto& t : threads)
            t.join();
    });
}

long long sumShared()
{
    long long total = 0;
    for (int i = 0; i < THREAD_COUNT; ++i)
        total += counters_shared[i];
    return total;
}


struct PaddedCounter
{
    alignas(CACHE_LINE_SIZE) int value;
};

PaddedCounter counters_padded[THREAD_COUNT];

void resetPadded()
{
    for (int i = 0; i < THREAD_COUNT; ++i)
        counters_padded[i].value = 0;
}

void paddedWorker(int thread_id)
{
    for (int i = 0; i < ITERATIONS; ++i)
        counters_padded[thread_id].value++;
}

long long runPadded()
{
    std::vector<std::thread> threads;

    return measure([&] {
        for (int i = 0; i < THREAD_COUNT; ++i)
            threads.emplace_back(paddedWorker, i);

        for (auto& t : threads)
            t.join();
    });
}

long long sumPadded()
{
    long long total = 0;
    for (int i = 0; i < THREAD_COUNT; ++i)
        total += counters_padded[i].value;
    return total;
}


thread_local int counter_tls = 0;

void tlsWorker(int thread_id, std::vector<int>& results)
{
    counter_tls = 0;

    for (int i = 0; i < ITERATIONS; ++i)
        counter_tls++;

    results[thread_id] = counter_tls;
}

long long runTLS(std::vector<int>& results)
{
    std::vector<std::thread> threads;

    return measure([&] {
        for (int i = 0; i < THREAD_COUNT; ++i)
            threads.emplace_back(tlsWorker, i, std::ref(results));

        for (auto& t : threads)
            t.join();
    });
}

long long sumTLS(const std::vector<int>& results)
{
    long long total = 0;
    for (int v : results)
        total += v;
    return total;
}



struct AtomicPaddedCounter
{
    alignas(CACHE_LINE_SIZE) std::atomic<int> value;
};

AtomicPaddedCounter counters_atomic[THREAD_COUNT];

void resetAtomic()
{
    for (int i = 0; i < THREAD_COUNT; ++i)
        counters_atomic[i].value.store(0, std::memory_order_relaxed);
}

void atomicWorker(int thread_id)
{
    for (int i = 0; i < ITERATIONS; ++i)
        counters_atomic[thread_id].value.fetch_add(
            1, std::memory_order_relaxed);
}

long long runAtomic()
{
    std::vector<std::thread> threads;

    return measure([&] {
        for (int i = 0; i < THREAD_COUNT; ++i)
            threads.emplace_back(atomicWorker, i);

        for (auto& t : threads)
            t.join();
    });
}

long long sumAtomic()
{
    long long total = 0;
    for (int i = 0; i < THREAD_COUNT; ++i)
        total += counters_atomic[i].value.load(
            std::memory_order_relaxed);
    return total;
}


int main()
{
    std::cout << "\nResult:\n";

    resetShared();
    long long t1 = runShared();
    std::cout << "Shared array time: " << t1 << " ms\n";
    std::cout << "Total: " << sumShared() << "\n\n";

    resetPadded();
    long long t2 = runPadded();
    std::cout << "Padded array time: " << t2 << " ms\n";
    std::cout << "Total: " << sumPadded() << "\n\n";

    std::vector<int> tls_results(THREAD_COUNT);
    long long t3 = runTLS(tls_results);
    std::cout << "Thread-local time: " << t3 << " ms\n";
    std::cout << "Total: " << sumTLS(tls_results) << "\n\n";

    resetAtomic();
    long long t4 = runAtomic();
    std::cout << "Atomic padded time: " << t4 << " ms\n";
    std::cout << "Total: " << sumAtomic() << "\n\n";
}
