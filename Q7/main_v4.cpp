#include <mutex>
#include <thread>
#include <random>
#include <iostream>
#include <atomic>

constexpr int RANDOM_OPERATION_COUNT{1'000'000};

std::mutex mutexUniv;

std::atomic<int> order_count{}, trade_count{}, cancel_count{};

void randomThreadSimulator(){

    std::mt19937 rGen{std::random_device{}()};
    std::uniform_int_distribution<> intDistr(1, 100);
    for(int i = 0; i < 1'000'000; i++){
        order_count.fetch_add(intDistr(rGen), std::memory_order_relaxed);
        trade_count.fetch_add(intDistr(rGen), std::memory_order_relaxed);
        cancel_count.fetch_add(intDistr(rGen), std::memory_order_relaxed);        
    }
}

int main(){
    std::vector<std::thread> threadPool;
    threadPool.reserve(8);
    for(int i = 0; i < 8; i++){
        threadPool.push_back(std::thread(randomThreadSimulator));
    }

    for(auto& thread: threadPool){
        thread.join();
    }
    std::cout<<"Order Count: "<<order_count<<std::endl;
    std::cout<<"Cancel Count: "<<cancel_count<<std::endl;
    std::cout<<"Trade Count: "<<trade_count<<std::endl;
}