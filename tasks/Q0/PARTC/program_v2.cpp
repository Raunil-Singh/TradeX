#include <iostream>
#include <atomic>
#include <vector>
#include <thread>

void incrementCounter();

std::atomic<long long> counter = 0;

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
    for (int i = 0; i < 100000; i++) {
        counter++;
    }
}