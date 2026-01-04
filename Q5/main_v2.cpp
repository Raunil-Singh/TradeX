#include <thread>
#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <iostream>
#include <random>
#include <array>
#include <atomic>

constexpr int PHILOSOPHER_COUNT{5};

std::array<std::mutex, PHILOSOPHER_COUNT> mutexArray;
std::uniform_int_distribution<> thinkDist(100, 300);
std::uniform_int_distribution<> eatDist(100, 200);
std::atomic<bool> shutdown{false};
std::vector<int> philosopherEatCount(PHILOSOPHER_COUNT, 0);
std::vector<std::chrono::steady_clock::duration> averageWaitTime(PHILOSOPHER_COUNT);

void philosopher(int id){
    std::mt19937 rGen{std::random_device{}()};
    std::chrono::steady_clock::time_point waitTimeStart, waitTimeEnd;
    std::chrono::steady_clock::duration durationAverage{};
    //think
    int philosopherEat{};
    while(!shutdown.load(std::memory_order_relaxed)){
        std::this_thread::sleep_for(std::chrono::milliseconds(thinkDist(rGen)));
        waitTimeStart = std::chrono::steady_clock::now();
        {
            if(id%2 == 0){
                //acquired right fork
                std::unique_lock<std::mutex> lockRight(mutexArray[(id + 1) % PHILOSOPHER_COUNT]);
                //acquired left fork
                std::unique_lock<std::mutex> lockLeft(mutexArray[id]);
            }
            
            if(id%2 == 1){
                //acquired left fork
                std::unique_lock<std::mutex> lockLeft(mutexArray[id]);
                //acquired right fork
                std::unique_lock<std::mutex> lockRight(mutexArray[(id + 1) % PHILOSOPHER_COUNT]);
            }
            waitTimeEnd = std::chrono::steady_clock::now();
            philosopherEat++;
            std::this_thread::sleep_for(std::chrono::milliseconds(eatDist(rGen)));
        }
        durationAverage += waitTimeEnd - waitTimeStart;
    }
    durationAverage /= philosopherEat;
    philosopherEatCount[id] = philosopherEat;
    averageWaitTime[id] = durationAverage;
}

int main(){
    std::vector<std::thread> philosophers;
    philosophers.reserve(5);

    for(int i = 0; i < 5; i++){
        philosophers.push_back(std::thread(philosopher, i));
    }
    std::this_thread::sleep_for(std::chrono::seconds(10));
    shutdown.store(true, std::memory_order_relaxed);
    for(auto& thread: philosophers){
        thread.join();
    }
    std::cout<<"Shutting down main thread...\n";
    for(int i = 0; i < PHILOSOPHER_COUNT; i++){
        std::cout<<"Philosopher "<<i<<", ate "<<philosopherEatCount[i]<<", and waited on average: "<<std::chrono::duration_cast<std::chrono::milliseconds>(averageWaitTime[i]).count()<<"ms. \n";
    }
}