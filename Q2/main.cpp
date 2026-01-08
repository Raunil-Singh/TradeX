#include <iostream>
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <functional>
#include <condition_variable>
#include <chrono>
#include <random>

std::queue<std::function<void()>> tasks;
std::mutex queue_mutex;
std::mutex print_lock;
std::condition_variable condition;
bool stop = false;


void workerThread()
{
    while (true)
    {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queue_mutex);

            condition.wait(lock, [] {
                return stop || !tasks.empty();
            });

            if (stop && tasks.empty()) return;

            task = std::move(tasks.front());
            tasks.pop();
        }
        task();
    }
}

void submitTask(std::function<void()> task)
{
    {
        std::lock_guard<std::mutex> guard(queue_mutex);
        tasks.push(std::move(task));
    }
    condition.notify_one();
}

void orderTask(int task_id)
{
    std::mt19937 mt(task_id);
    std::uniform_int_distribution<int> dist(1,100);

    long long sum = 0;
    for (int i = 0; i < 100'000; i++)
    {
        sum += dist(mt);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    std::lock_guard<std::mutex> print_guard(print_lock);
    std::cout << "Task " << task_id << " processed by thread " << std::this_thread::get_id() << ", result = " << sum << "\n";
}


int main()
{
    std::vector<std::thread> threads;

    for (int i = 0; i < 4; i++) 
    {
        threads.emplace_back(workerThread);
    }

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 50; i++)
    {
        submitTask([i] {
            orderTask(i);
        });
    }

    {
        std::lock_guard<std::mutex> guard(queue_mutex);
        stop = true;
    }

    condition.notify_all();

    for (auto& thread : threads)
    {
        thread.join();
    }

    auto end = std::chrono::high_resolution_clock::now();

    std::cout << "time taken by thread pool: " << std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count() << "ms\n";
}