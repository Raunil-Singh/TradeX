#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <chrono>
using namespace std;

__attribute__((noinline))
void fast_function() {
    volatile long long sum;
    for(int i=1; i<=1000000; i++) {
        sum = i+i;
    }
}

__attribute__((noinline))
void slow_function() {
    volatile long long sum;
    for(int i=1; i<=1000000; i++) {
        sum = (i*10)/7;
    }
}

__attribute__((noinline))
void very_slow_function() {
    volatile long long sum;
    for(int i=1; i<=10000; i++) {
        for(int j=1; j<=10000; j++) {
            sum = i+j;
        }
    }
}

int main() {
    cout << "Starting Profiling benchmark..." << endl;
    fast_function();
    slow_function();
    very_slow_function();
    cout << "Profiling complete." << endl;
    return 0;
}