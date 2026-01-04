#include <thread>
#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <iostream>
#include <random>

constexpr int PHILOSOPHER_COUNT{5};

std::array<std::mutex, PHILOSOPHER_COUNT> mutexArray;
std::uniform_int_distribution<> thinkDist(0, 0);
std::uniform_int_distribution<> eatDist(0, 0);
std::atomic<bool> shutdown{false};

void philosopher(int id){
    std::mt19937 rGen{std::random_device{}()};
    //think
    while(!shutdown.load(std::memory_order_relaxed)){
        std::this_thread::sleep_for(std::chrono::milliseconds(thinkDist(rGen)));
        {
            //acquired left fork
            std::unique_lock<std::mutex> lockLeft(mutexArray[id]);
            //acquired right fork
            std::unique_lock<std::mutex> lockRight(mutexArray[(id + 1) % PHILOSOPHER_COUNT]);
            std::this_thread::sleep_for(std::chrono::milliseconds(eatDist(rGen)));
        }
        std::cout<<"Philosopher "<<id<<" is done with eating.\n";
    }
}

int main(){
    std::vector<std::thread> philosophers;
    philosophers.reserve(5);

    for(int i = 0; i < 5; i++){
        philosophers.push_back(std::thread(philosopher, i));
    }
    std::this_thread::sleep_for(std::chrono::seconds(30));
    shutdown.store(true, std::memory_order_relaxed);
    for(auto& thread: philosophers){
        thread.join();
    }
    std::cout<<"Shutting down main thread...\n";
}