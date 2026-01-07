#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <functional>
#include <chrono>

constexpr int ITERATIONS = 500'000;
constexpr int NUM_THREADS = 4;

void incrementCounterAsync(int& counter, int iterations)
{
    std::cout << "Thread " << std::this_thread::get_id()
              << " incrementing (unsynchronized)\n";

    for (int i = 0; i < iterations; ++i)
    {
        counter++;
    }
}

void decrementCounterAsync(int& counter, int iterations)
{
    std::cout << "Thread " << std::this_thread::get_id() << " decrementing (unsynchronized)\n";

    for (int i = 0; i < iterations; ++i)
    {
        counter--;
    }
}

void incrementCounterSync(int& counter, std::mutex& mutex, int iterations)
{
    std::cout << "Thread " << std::this_thread::get_id() << " incrementing (synchronized)\n";

    for (int i = 0; i < iterations; ++i)
    {
        std::lock_guard<std::mutex> lock(mutex);
        counter++;
    }
}

void decrementCounterSync(int& counter, std::mutex& mutex, int iterations)
{
    std::cout << "Thread " << std::this_thread::get_id() << " decrementing (synchronized)\n";

    for (int i = 0; i < iterations; ++i)
    {
        std::lock_guard<std::mutex> lock(mutex);
        counter--;
    }
}

void unsynchronized()
{
    std::cout << "\nUnsynchronized Version: \n\n";

    int shared_counter = 0;
    std::vector<std::thread> threads;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 2; i++)
    {
        threads.emplace_back(incrementCounterAsync, std::ref(shared_counter), ITERATIONS);
    }
    for (int i = 0; i < 2; i++)
    {
        threads.emplace_back(decrementCounterAsync, std::ref(shared_counter), ITERATIONS);
    }

    for (std::thread& t : threads)
    {
        t.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "Final counter value: " << shared_counter << "\n";
    std::cout << "Execution time: " << duration.count() << " μs\n";
}

void synchronized()
{
    std::cout << "\nSynchronized Version: \n\n";

    int shared_counter = 0;
    std::mutex mutex;
    std::vector<std::thread> threads;

    auto start = std::chrono::high_resolution_clock::now();

    threads.emplace_back(incrementCounterSync, std::ref(shared_counter), std::ref(mutex), ITERATIONS);
    threads.emplace_back(incrementCounterSync, std::ref(shared_counter), std::ref(mutex), ITERATIONS);
    threads.emplace_back(decrementCounterSync, std::ref(shared_counter), std::ref(mutex), ITERATIONS);
    threads.emplace_back(decrementCounterSync, std::ref(shared_counter), std::ref(mutex), ITERATIONS);

    for (std::thread& t : threads)
    {
        t.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    std::cout << "Final counter value: " << shared_counter << "\n";
    std::cout << "Execution time: " << duration.count() << " μs\n";
}

int main()
{
    unsynchronized();
    synchronized();
}
