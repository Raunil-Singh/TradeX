#include <iostream>
#include <vector>
#include <thread>

void incrementCounter();

int main()
{
    std::vector<std::thread> vector;
    for (int i = 0; i < 10; i++) {
        vector.push_back(std::thread(incrementCounter));
    }
    for (std::thread& thread : vector) {
        thread.join();
    }
    return 0;
}

void incrementCounter()
{
    long long counter = 0;
    for (int i = 0; i < 1000000000; i++) {
        counter++;
    }
    std::cout << counter;
}