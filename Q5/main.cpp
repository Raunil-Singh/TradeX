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

void naiveTrial(int id, std::atomic<bool>& stop)
{
    int left = id;
    int right = (id + 1) % PHILOSOPHER_COUNT;

    while (!stop)
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

bool deadlock_detected(const std::vector<std::atomic<int>>& counts, const std::vector<int>& last_counts)
{
    for (int i = 0; i < PHILOSOPHER_COUNT; i++)
    {
        if (counts[i] != last_counts[i])
            return false;
    }
    return true;
}

void orderedTrial(int philosopher_id, std::atomic<bool>& stop)
{
    int left_fork = philosopher_id;
    int right_fork = (philosopher_id + 1) % PHILOSOPHER_COUNT;

    while (!stop)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(random_between(100, 300)));

        auto wait_start = std::chrono::high_resolution_clock::now();

        if (philosopher_id % 2 == 0)
        {
            forks[right_fork].lock();
            forks[left_fork].lock();
        }
        else
        {
            forks[left_fork].lock();
            forks[right_fork].lock();
        }

        auto wait_end = std::chrono::high_resolution_clock::now();

        wait_time_ns[philosopher_id] += std::chrono::duration_cast<std::chrono::nanoseconds>(wait_end - wait_start).count();

        std::this_thread::sleep_for(std::chrono::milliseconds(random_between(100, 200)));

        eat_count[philosopher_id]++;

        forks[left_fork].unlock();
        forks[right_fork].unlock();
    }
}

std::vector<std::timed_mutex> timed_forks(PHILOSOPHER_COUNT);

void timeoutTrial(int id, std::atomic<bool>& stop)
{
    int left = id;
    int right = (id + 1) % PHILOSOPHER_COUNT;

    while (!stop)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(random_between(100, 300)));

        auto wait_start = std::chrono::high_resolution_clock::now();

        if (!timed_forks[left].try_lock_for(std::chrono::milliseconds(100))) continue;

        if (!timed_forks[right].try_lock_for(std::chrono::milliseconds(100)))
        {
            timed_forks[left].unlock();
            continue;
        }

        auto wait_end = std::chrono::high_resolution_clock::now();

        wait_time_ns[id] += std::chrono::duration_cast<std::chrono::nanoseconds>(wait_end - wait_start).count();

        std::this_thread::sleep_for(std::chrono::milliseconds(random_between(100, 200)));

        eat_count[id]++;

        timed_forks[right].unlock();
        timed_forks[left].unlock();
    }
}

void resetVariables()
{
    for (int i = 0; i < PHILOSOPHER_COUNT; i++)
    {
        eat_count[i].store(0);
        wait_time_ns[i].store(0);
    }
}

int main()
{
    {
        std::atomic<bool> stop{false};
        std::vector<std::thread> philosophers;

        for (int i = 0; i < PHILOSOPHER_COUNT; i++)
            philosophers.emplace_back(
                naiveTrial, i, std::ref(stop));

        std::vector<int> last_counts(PHILOSOPHER_COUNT);

        for (int i = 0; i < PHILOSOPHER_COUNT; i++)
            last_counts[i] = eat_count[i];

        std::this_thread::sleep_for(std::chrono::seconds(5));

        stop = true;
        if (deadlock_detected(eat_count, last_counts))
        {
            std::cout << "\nNaive version deadlocked\n\n";
            for (auto& t : philosophers) t.detach();
        }
        else
        {
            for (auto& t : philosophers) t.join();

            std::cout << "Naive version results:\n";
            for (int i = 0; i < PHILOSOPHER_COUNT; i++)
                std::cout << "Philosopher " << i << " ate " << eat_count[i] << "\n";
            std::cout << "\n";
        }

    }

    resetVariables();

    {
        std::atomic<bool> stop{false};
        std::vector<std::thread> philosophers;

        for (int i = 0; i < PHILOSOPHER_COUNT; i++)
            philosophers.emplace_back(orderedTrial, i, std::ref(stop));

        std::this_thread::sleep_for(std::chrono::seconds(RUN_TIME_SECONDS));

        stop = true;

        for (auto& t : philosophers) t.join();

        std::cout << "Resource ordering results:\n";
        for (int i = 0; i < PHILOSOPHER_COUNT; i++)
            std::cout << "Philosopher " << i << " ate " << eat_count[i] << "\n";
        std::cout << "\n";
    }

    resetVariables();

    {
        std::atomic<bool> stop{false};
        std::vector<std::thread> philosophers;

        for (int i = 0; i < PHILOSOPHER_COUNT; i++)
            philosophers.emplace_back(timeoutTrial, i, std::ref(stop));

        std::this_thread::sleep_for(std::chrono::seconds(RUN_TIME_SECONDS));

        stop = true;

        for (auto& t : philosophers) t.join();

        std::cout << "Timeout-based results:\n";
        for (int i = 0; i < PHILOSOPHER_COUNT; i++) std::cout << "Philosopher " << i << " ate " << eat_count[i] << "\n";
        std::cout << "\n";
    }
}
