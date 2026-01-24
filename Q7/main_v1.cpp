#include <mutex>
#include <thread>
#include <random>
#include <iostream>

constexpr int RANDOM_OPERATION_COUNT{1'000'000};

std::mutex mutexUniv;
std::mt19937 rGen{std::random_device{}()};
std::uniform_int_distribution<> intDistr(1, 100);

int order_count{}, trade_count{}, cancel_count{};

void randomThreadSimulator(){
    for(int i = 0; i < 1'000'000; i++){
        {
            std::lock_guard<std::mutex> lock(mutexUniv);
            order_count += intDistr(rGen);
            trade_count += intDistr(rGen);
            cancel_count += intDistr(rGen);
        }
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