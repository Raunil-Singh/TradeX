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

std::array<std::timed_mutex, PHILOSOPHER_COUNT> mutexArray;
std::uniform_int_distribution<> thinkDist(100, 300);
std::uniform_int_distribution<> eatDist(100, 200);
std::atomic<bool> shutdown{false};
std::vector<int> philosopherEatCount(PHILOSOPHER_COUNT, 0);
std::vector<std::chrono::steady_clock::duration> averageWaitTime(PHILOSOPHER_COUNT);

void philosopher(int id){
    std::mt19937 rGen{std::random_device{}()};
    int philosopherEat{};
    std::chrono::steady_clock::duration durationAverage{};
    std::chrono::steady_clock::time_point waitTimeStart, waitTimeEnd;
    //think
    while(!shutdown.load()){
        std::this_thread::sleep_for(std::chrono::milliseconds(thinkDist(rGen)));
        {
            waitTimeStart = std::chrono::steady_clock::now();
            while(true){
                //if lock acquired
                
                if(mutexArray[id].try_lock_for(std::chrono::milliseconds(100))){
                    if(mutexArray[(id + 1) % PHILOSOPHER_COUNT].try_lock_for(std::chrono::milliseconds(100))){
                        //right lock acquired
                        waitTimeEnd = std::chrono::steady_clock::now();
                        std::this_thread::sleep_for(std::chrono::milliseconds(eatDist(rGen)));
                        philosopherEat++;
                        mutexArray[(id + 1) % PHILOSOPHER_COUNT].unlock();
                        mutexArray[id].unlock();
                        break;
                    }
                    mutexArray[id].unlock();
                }
            }
            
        }
        durationAverage += (waitTimeEnd) - (waitTimeStart);
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
    std::this_thread::sleep_for(std::chrono::seconds(30));
    shutdown.store(true);
    for(auto& thread: philosophers){
        thread.join();
    }
    std::cout<<"Shutting down main thread...\n";
    for(int i = 0; i < PHILOSOPHER_COUNT; i++){
        std::cout<<"Philosopher "<<i<<", ate "<<philosopherEatCount[i]<<", and waited on average: "<<std::chrono::duration_cast<std::chrono::milliseconds>(averageWaitTime[i]).count()<<"ms. \n";
    }
}