#include <thread>
#include <vector>
#include <iostream>
struct alignas(64) paddedInt{
    int value;
    char padding[64 - sizeof(int)];
    paddedInt():
    value(0)
    {}
};
std::vector<paddedInt> counters(8);

void worker(int id){
    for(int i = 0; i < 10'000'000; i++){
        counters[id].value++;
    }
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

    for(int i = 0; i < 8; i++){
        std::cout<<"Thread id: "<<i<<", counter: "<<counters[i].value<<std::endl;
    }
}