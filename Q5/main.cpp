#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <random>
#include <chrono>
#include <atomic>

const int PHILOSOPHER_COUNT = 5;
const int RUN_TIME_SECONDS = 30;

std::vector<std::mutex> forks(PHILOSOPHER_COUNT);

std::vector<std::atomic<int>> eat_count(PHILOSOPHER_COUNT);
std::vector<std::atomic<long long>> wait_time_ns(PHILOSOPHER_COUNT);

int random_between(int low, int high)
{
    static thread_local std::mt19937 mt(std::random_device{}());
    std::uniform_int_distribution<int> dist(low, high);
    return dist(mt);
}

void naive(int id, std::atomic<bool>& stop_flag)
{
    int left = id;
    int right = (id + 1) % PHILOSOPHER_COUNT;

    while (!stop_flag)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(random_between(100, 300)));

        auto wait_start = std::chrono::high_resolution_clock::now();

        forks[left].lock();
        forks[right].lock();

        auto wait_end = std::chrono::high_resolution_clock::now();

        wait_time_ns[id] += std::chrono::duration_cast<std::chrono::nanoseconds>(wait_end - wait_start).count();

        std::this_thread::sleep_for(std::chrono::milliseconds(random_between(100, 200)));

        eat_count[id]++;

        forks[right].unlock();
        forks[left].unlock();
    }
}

bool deadlock_detected(
    const std::vector<std::atomic<int>>& counts,
    const std::vector<int>& last_counts)
{
    for (int i = 0; i < PHILOSOPHER_COUNT; i++)
    {
        if (counts[i] != last_counts[i])
            return false;
    }
    return true;
}
