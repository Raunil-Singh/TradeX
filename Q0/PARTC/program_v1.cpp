#include <iostream>
#include <mutex>
#include <vector>
#include <thread>

void incrementCounter();

long long counter = 0;
std::mutex mutexLock;

int main()
{
    std::vector<std::thread> vector;
    for (int i = 0; i < 1000; i++) {
        vector.push_back(std::thread(incrementCounter));
    }
    for (std::thread& thread : vector) {
        thread.join();
    }
    std::cout << counter;

    return 0;
}

void incrementCounter()
{
    mutexLock.lock();
    for (int i = 0; i < 10000000; i++) {
        counter = counter + i;
    }
    mutexLock.unlock();
}