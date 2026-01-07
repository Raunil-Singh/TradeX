#include <iostream>
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <functional>
#include <condition_variable>

std::queue<std::function<void()>> tasks;
std::mutex queue_mutex;
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
        std::lock_guard<std::mutex> lock(queue_mutex);
    }
}



int main()
{
    std::vector<std::thread> threads;

    for (int i = 0; i < 4; i++) 
    {
        threads.emplace_back(workerThread);
    }
}