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

void worker(int id){
    int i = id* SEGMENT;
    for(; (i + 3) < (id + 1) * SEGMENT; i+=4){
        c[i]     = a[i]     + b[i];
        c[i + 1] = a[i + 1] + b[i + 1];
        c[i + 2] = a[i + 2] + b[i + 2];
        c[i + 3] = a[i + 3] + b[i + 3];
    }

    for(; i < (id+1)*SEGMENT; i++){
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
        auxThread.push_back(std::thread(worker, i));
    }
    for(auto& thread: auxThread){
        thread.join();
    }
    std::cout<<"Main thread done... \n";
}