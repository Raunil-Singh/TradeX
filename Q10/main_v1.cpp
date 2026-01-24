#include <vector>
#include <thread>
#include <random>
#include <iostream>

constexpr int COUNT{100'000'000};
std::uniform_int_distribution<> intDistr(1,1000);
std::mt19937 rGen(std::random_device{}());
std::vector<int> a(COUNT);
std::vector<int> b(COUNT);
std::vector<int> c(COUNT);

void worker(){
    for(int i = 0; i < COUNT; i++){
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
    std::thread auxThread(worker);
    auxThread.join();
    std::cout<<"Main thread done... \n";
}