#include <vector>
#include <queue>
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <random>
#include <chrono>

const int MOD = 1e9 + 7;

std::queue<int> taskQueue;
std::mutex q_mutex;
std::condition_variable cvQEmpty;

bool done = false;
void workerThread(){
    std::mt19937 rGenerator{std::random_device{}()};
    while(true){
        int task_id{};
        {
            std::unique_lock<std::mutex> lock(q_mutex);

            cvQEmpty.wait(lock, []{
                return !taskQueue.empty()||done;
            });
            if(taskQueue.empty() && done){
                break;
            }
            task_id = taskQueue.front();
            taskQueue.pop();
        }
        volatile long long sum{};

        for(int i = 0; i < 100'000; i++){
            sum += rGenerator();
            sum = sum % MOD;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        std::cout<<"Work done in thread: "<<std::this_thread::get_id()<<", task id: "<<task_id<<", is = "<<sum<<"\n";
    }
}

int main(){
    std::vector<std::thread> workerThreadPool;
    workerThreadPool.reserve(4);
    for(int i = 0; i < 4; i++){
        workerThreadPool.push_back(std::thread(workerThread));
    }
    for(int i = 0; i < 50; i++)
    {
        {
        std::lock_guard<std::mutex> lock(q_mutex);
        taskQueue.push(i + 1);
        }
        cvQEmpty.notify_one();
    }
    {
        std::lock_guard<std::mutex> lock(q_mutex);
        done = true;
    }
    cvQEmpty.notify_all();
    for(auto& thread: workerThreadPool){
        thread.join();
    }
    std::cout<<"exited main thread\n";
}