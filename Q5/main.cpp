#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <mutex>
#include <random>
#include <atomic>

const int PHILOSOPHER_COUNT = 5;
std::timed_mutex fork_timed[PHILOSOPHER_COUNT];
std::mutex fork[PHILOSOPHER_COUNT];
std::mutex fork_resource[PHILOSOPHER_COUNT];
std::vector<std::atomic<int>> eat_count(5);
std::atomic<bool> stop_flag{false};

std::mutex time_mtx;
std::chrono::steady_clock::time_point last_eat_time;

std::vector<std::atomic<long long>> total_wait_ns(PHILOSOPHER_COUNT);


void randomSleep(int min_ms, int max_ms)
{
    static thread_local std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<> dis(min_ms, max_ms);
    std::this_thread::sleep_for(std::chrono::milliseconds(dis(gen)));
}

void philosopher1(int id)
{
    int left_fork = id;
    int right_fork = (id + 1) % 5;

    while (!stop_flag)
    {
        randomSleep(10, 100);

        auto wait_start = std::chrono::steady_clock::now();
        fork[left_fork].lock();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        fork[right_fork].lock();
        auto wait_end = std::chrono::steady_clock::now();
        total_wait_ns[id] += std::chrono::duration_cast<std::chrono::nanoseconds>(wait_end - wait_start).count();

        randomSleep(10, 200);
        eat_count[id]++;
        {
            std::lock_guard<std::mutex> lock(time_mtx);
            last_eat_time = std::chrono::steady_clock::now();
        }

        fork[left_fork].unlock();
        fork[right_fork].unlock();
    }
}

void philosopher2(int id)
{
    int left = id;
    int right = (id + 1) % PHILOSOPHER_COUNT;

    int first = std::min(left, right);
    int second = std::max(left, right);

    while (!stop_flag)
    {
        randomSleep(100, 300);

        auto wait_start = std::chrono::steady_clock::now();

        fork_resource[first].lock();
        fork_resource[second].lock();

        auto wait_end = std::chrono::steady_clock::now();
        total_wait_ns[id] += std::chrono::duration_cast<std::chrono::nanoseconds>(wait_end - wait_start).count();

        randomSleep(100, 200);
        eat_count[id]++;

        {
            std::lock_guard<std::mutex> lock(time_mtx);
            last_eat_time = std::chrono::steady_clock::now();
        }

        fork_resource[second].unlock();
        fork_resource[first].unlock();
    }
}

void philosopher3(int id)
{
    int left = id;
    int right = (id + 1) % PHILOSOPHER_COUNT;

    while (!stop_flag)
    {
        randomSleep(100, 300);

        auto wait_start = std::chrono::steady_clock::now();

        if (!fork_timed[left].try_lock_for(std::chrono::milliseconds(100))){
            continue;
        }

        if (!fork_timed[right].try_lock_for(std::chrono::milliseconds(100)))
        {
            fork_timed[left].unlock();
            continue;
        }

        auto wait_end = std::chrono::steady_clock::now();
        total_wait_ns[id] += std::chrono::duration_cast<std::chrono::nanoseconds>(wait_end - wait_start).count();

        randomSleep(100, 200);
        eat_count[id]++;

        {
            std::lock_guard<std::mutex> lock(time_mtx);
            last_eat_time = std::chrono::steady_clock::now();
        }

        fork_timed[right].unlock();
        fork_timed[left].unlock();
    }
}

void deadlock_detector()
{
    while (!stop_flag)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        std::chrono::steady_clock::time_point last;
        {
            std::lock_guard<std::mutex> lock(time_mtx);
            last = last_eat_time;
        }

        if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - last).count() >= 5)
        {
            stop_flag = true;
            return;
        }
    }
}

void naive_version()
{
    stop_flag = false;
    for (auto &c : eat_count) c = 0;
    for (auto &w : total_wait_ns) w = 0;

    last_eat_time = std::chrono::steady_clock::now();

    std::vector<std::thread> philosophers;
    for (int i = 0; i < 5; ++i)
    {
        philosophers.emplace_back(philosopher1, i);
    }

    std::thread detector(deadlock_detector);

    for (int i = 0; i < 30; i++)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if (stop_flag)
        {
            std::cout << "Deadlock detected (naive version)\n";
            break;
        }
    }

    if (!stop_flag)
    {
        std::cout << "No Deadlock detected (naive version)\n";
        stop_flag = true;
    }
    if (stop_flag)
    {
        detector.join();
        for (auto &p : philosophers)
            p.detach();
        return;
    }
}

void resource_ordering_version()
{
    stop_flag = false;
    for (auto &c : eat_count) c = 0;
    for (auto &w : total_wait_ns) w = 0;

    last_eat_time = std::chrono::steady_clock::now();

    std::vector<std::thread> philosophers;
    for (int i = 0; i < 5; ++i)
    {
        philosophers.emplace_back(philosopher2, i);
    }

    std::thread detector(deadlock_detector);
    for (int i = 0; i < 30; i++)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if (stop_flag)
        {
            std::cout << "Deadlock Detected (resource ordering Version)\n";
        }
    }

    if (!stop_flag)
    {
        std::cout << "No deadlock detected (resources ordering version)\n";
    }
    stop_flag = true;

    detector.join();
    for (auto &p : philosophers)
        p.join();
}

void timeout_based_version()
{
    stop_flag = false;
    for (auto &c : eat_count) c = 0;
    for (auto &w : total_wait_ns) w = 0;

    last_eat_time = std::chrono::steady_clock::now();

    std::vector<std::thread> philosophers;
    for (int i = 0; i < PHILOSOPHER_COUNT; ++i)
        philosophers.emplace_back(philosopher3, i);

    std::thread detector(deadlock_detector);

    for (int i = 0; i < 30 && !stop_flag; ++i)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if (stop_flag)
        {
            std::cout << "Deadlock Detected (timoeout based version)\n";
        }
    }

    if(!stop_flag){
        std::cout << "No deadlock detected (timeout based version)\n";
    }
    stop_flag = true;

    detector.join();
    for (auto &p : philosophers){
        p.join();    
    }
}

void print_stats() {
    for (int i = 0; i < PHILOSOPHER_COUNT; i++) {
        long long eats = eat_count[i];
        double avg_wait_ms = eats == 0 ? 0.0 : (total_wait_ns[i] / 1e6) / eats;
        std::cout << "Philosopher " << i << " | Eats: " << eats << " | Avg wait: " << avg_wait_ms << " ms\n";
    }
}


int main()
{
    naive_version();
    print_stats();
    resource_ordering_version();
    print_stats();
    timeout_based_version();
    print_stats();
    return 0;
}
