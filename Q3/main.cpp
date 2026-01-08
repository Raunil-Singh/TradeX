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

void readerVersionOne()
{
    std::mt19937 mt(std::random_device{}());
    std::uniform_int_distribution<int> index_dist(0, NUM_PRICES - 1);

    for (int i = 0; i < READ_ITERATIONS; ++i)
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

int main()
{

}