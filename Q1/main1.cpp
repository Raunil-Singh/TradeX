#include <thread>
#include <iostream>
#include <mutex>
#include <vector>

constexpr int iterCount = 500'000;
int sUCounter{};
int sGCounter{};
std::mutex sGCmutex;
void incrementCounterUnsafe(int iterations){
    for(int i = 0; i < iterations; i++){
        int tmp = sUCounter;
        tmp++;
        sUCounter = tmp;
    }
}
void incrementCounterSafe(int iterations){
    for(int i = 0; i < iterations; i++){
        const std::lock_guard<std::mutex> lock(sGCmutex);
        sGCounter++;
    }
}
void decrementCounterUnsafe(int iterations){
    for(int i = 0; i < iterations; i++){
        int tmp = sUCounter;
        tmp--;
        sUCounter = tmp;
    }
}
void decrementCounterSafe(int iterations){
    for(int i = 0; i < iterations; i++){
        const std::lock_guard<std::mutex> lock(sGCmutex);
        sGCounter--;
    }
}

int main(){
    std::vector<std::thread> threadPool;
    threadPool.reserve(40);
    for(int i = 0; i < 10; i++){
        threadPool.push_back(std::thread(incrementCounterUnsafe, iterCount));
    }
    for(int i = 0; i < 10; i++){
        threadPool.push_back(std::thread(incrementCounterSafe, iterCount));
    }
    for(int i = 0; i < 10; i++){
        threadPool.push_back(std::thread(decrementCounterUnsafe, iterCount));
    }
    for(int i = 0; i < 10; i++){
        threadPool.push_back(std::thread(decrementCounterSafe, iterCount));
    }
    for(auto& thread: threadPool){
        thread.join();
    }
    std::cout<<"Unsafe counter value: "<<sUCounter<<"\n";
    std::cout<<"Safe counter value: "<<sGCounter<<"\n";
    std::cout<<"Main thread terminated... \n";
}


