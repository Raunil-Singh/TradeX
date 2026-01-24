#include <vector>
#include <thread>
#include <random>
#include <iostream>

constexpr int COUNT{100'000'000};
constexpr int SEGMENT{COUNT / 4};

std::uniform_int_distribution<> intDistr(1,1000);
std::mt19937 rGen(std::random_device{}());
std::vector<int> a(COUNT);
std::vector<int> b(COUNT);
std::vector<int> c(COUNT);

void worker(int id, const int* __restrict a, const int* __restrict b, int* __restrict c){
    #pragma omp simd
    for(int i = id * SEGMENT; i < (id + 1) * SEGMENT; i++){
        c[i] = a[i] + b[i];
    }
}

int main(){
    

    //initializing a and b
    for(int i = 0; i < COUNT; i++){
        a[i] = rGen();
        b[i] = rGen();
    }

    //calling aux thread to do operations
    std::vector<std::thread> auxThread;
    auxThread.reserve(4);
    for(int i = 0; i < 4; i++){
        auxThread.push_back(std::thread(worker, i, a.data(), b.data(), c.data()));
    }
    for(auto& thread: auxThread){
        thread.join();
    }
    std::cout<<"Main thread done... \n";
}