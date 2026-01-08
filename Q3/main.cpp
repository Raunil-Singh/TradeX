#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <random>
#include <atomic>
#include <chrono>
#include <shared_mutex>
#include <array>

const int NUM_PRICES = 1000;
const int NUM_READERS = 8;
const int NUM_WRITERS = 2;

const int READ_ITERATIONS = 100'000;
const int WRITE_ITERATIONS = 10'000;

std::vector<double> prices(NUM_PRICES, 100.0);

std::atomic<long long> total_reads{0};
std::atomic<long long> total_writes{0};

std::atomic<long long> reader_wait_ns{0};
std::atomic<long long> writer_wait_ns{0};

std::mutex mutex_v1;
std::shared_mutex mutex_v2;

void readerVersionOne()
{
    std::mt19937 mt(std::random_device{}());
    std::uniform_int_distribution<int> index_dist(0, NUM_PRICES - 1);

    for (int i = 0; i < READ_ITERATIONS; i++)
    {
        auto wait_start = std::chrono::high_resolution_clock::now();
        std::lock_guard<std::mutex> lock(mutex_v1);
        auto wait_end = std::chrono::high_resolution_clock::now();

        reader_wait_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(wait_end - wait_start).count();

        double value = prices[index_dist(mt)];
        (void)value;

        total_reads++;
    }
}

void writerVersionOne()
{
    std::mt19937 mt(std::random_device{}());
    std::uniform_int_distribution<int> index_distribution(0, NUM_PRICES - 1);
    std::uniform_int_distribution<int> delta(-10, 10);

    for (int i = 0; i < WRITE_ITERATIONS; i++)
    {
        auto wait_start = std::chrono::high_resolution_clock::now();
        std::lock_guard<std::mutex> lock(mutex_v1);
        auto wait_end = std::chrono::high_resolution_clock::now();

        writer_wait_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(wait_end - wait_start).count();
        
        prices[index_distribution(mt)] += delta(mt);
        total_writes++;    
    }
}

void readerVersionTwo() 
{
    std::mt19937 mt(std::random_device{}());
    std::uniform_int_distribution<int> index_distribution(0, NUM_PRICES - 1);

    for (int i = 0; i < READ_ITERATIONS; i++)
    {
        auto wait_start = std::chrono::high_resolution_clock::now();
        std::shared_lock<std::shared_mutex> lock(mutex_v2);
        auto wait_end = std::chrono::high_resolution_clock::now();    
        
        reader_wait_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(wait_end - wait_start).count();
        double value = prices[index_distribution(mt)];
        (void)value;

        total_reads++;
    }
}

void writerVersionTwo()
{
    std::mt19937 mt(std::random_device{}());
    std::uniform_int_distribution<int> index_distribution(0, NUM_PRICES - 1);
    std::uniform_real_distribution<double> delta(-10, 10);

    for (int i = 0; i < WRITE_ITERATIONS; i++)
    {
        auto wait_start = std::chrono::high_resolution_clock::now();
        std::unique_lock<std::shared_mutex> lock(mutex_v2);
        auto wait_end = std::chrono::high_resolution_clock::now();

        writer_wait_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(wait_end - wait_start).count();

        prices[index_distribution(mt)] += delta(mt);
        total_writes++;
    }
}


const int SEGMENTS = 10;
const int SEGMENT_SIZE = NUM_PRICES / SEGMENTS;

std::array<std::shared_mutex, SEGMENTS> segment_mutexes;

void readerVersionThree()
{
    std::mt19937 mt(std::random_device{}());
    std::uniform_int_distribution<int> index_distribution(0, NUM_PRICES - 1);
    
    for (int i = 0; i < READ_ITERATIONS; i++)
    {
        int index = index_distribution(mt);
        int segment = index / SEGMENT_SIZE;

        auto wait_start = std::chrono::high_resolution_clock::now();
        std::shared_lock<std::shared_mutex> lock(segment_mutexes[segment]);
        auto wait_end = std::chrono::high_resolution_clock::now();

        reader_wait_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(wait_end - wait_start).count();

        double value = prices[index];
        (void)value;

        total_reads++;
    }
}

void writerVersionThree()
{
    std::mt19937 mt(std::random_device{}());
    std::uniform_int_distribution<int> index_distribution(0, NUM_PRICES - 1);
    std::uniform_real_distribution<double> delta(-10, 10);

    for (int i = 0; i < WRITE_ITERATIONS; i++)
    {
        int idx = index_distribution(mt);
        int segment = idx / SEGMENT_SIZE;

        auto wait_start = std::chrono::high_resolution_clock::now();
        std::unique_lock<std::shared_mutex> lock(segment_mutexes[segment]);
        auto wait_end = std::chrono::high_resolution_clock::now();

        writer_wait_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(wait_end - wait_start).count();

        prices[idx] += delta(mt);
        total_writes++;
    }
}

template <typename Reader, typename Writer>
void run(Reader reader, Writer writer)
{
    prices.assign(NUM_PRICES, 100.0);
    total_reads = 0;
    total_writes = 0;
    reader_wait_ns = 0;
    writer_wait_ns = 0;

    std::vector<std::thread> threads;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < NUM_READERS; i++) threads.emplace_back(reader);

    for (int i = 0; i < NUM_WRITERS; i++) threads.emplace_back(writer);

    for (auto& t : threads) t.join();

    auto end = std::chrono::high_resolution_clock::now();

    std::cout << "Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms\n";
    std::cout << "Reads: " << total_reads << ", Writes: " << total_writes << "\n";
}


int main()
{
    std::cout << "\n\nv1: ";
    run(readerVersionOne, writerVersionOne);
    std::cout << "\n\nv2: ";
    run(readerVersionOne, writerVersionOne);
    std::cout << "\n\nv3: ";
    run(readerVersionOne, writerVersionOne);
}
