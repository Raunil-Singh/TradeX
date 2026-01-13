#include <thread>
#include <vector>
#include <iostream>

std::vector<int> counters(8, 0);
thread_local int localCounter{0};
void worker(int id){
    for(int i = 0; i < 10'000'000; i++){
        localCounter++;
    }
    counters[id] = localCounter;
}

int main(){
    std::vector<std::thread> workerThreadPool;
    workerThreadPool.reserve(8);
    for(int i = 0; i < 8; i++){
        workerThreadPool.push_back(std::thread(worker, i));
    }

    for(auto& thread: workerThreadPool){
        thread.join();
    }
    int total{};
    for(int i = 0; i < 8; i++){
        std::cout<<"Thread id: "<<i<<", counter: "<<counters[i]<<std::endl;
    }
   
}