#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <chrono>
#include <atomic>
#include <mutex>
using namespace std;

mutex m;
volatile long long counterm = 0;
volatile atomic<int> countera = 0;

void version_1() {
    for(int i=1; i<=10000000; i++) {
        m.lock();
        counterm++;
        m.unlock();
    }
}

void version_2() {
    for(int i=1; i<=10000000; i++) {
        countera++;
    }
}

void version_3(long long *count) {
    long long local_count = 0;
    for(int i=1; i<=10000000; i++) {
        local_count++;
    }
    *count = local_count;
}

int main(int argc, char* argv[]) {
    if(argc<2) {
        cout << "Error : Enter valid mode(1, 2 or 3)" << endl;
        return 1;
    }
    int mode = stoi(argv[1]);
    vector<thread> threads;
    if(mode==1) {
        for(int i=0; i<4; i++) {
            threads.emplace_back(version_1);
        }
        for(auto &i : threads) {
            i.join();
        }
        cout << counterm << endl;
    } else if(mode==2) {
        for(int i=0; i<4; i++) {
            threads.emplace_back(version_2);
        }
        for(auto &i : threads) {
            i.join();
        }
        cout << countera << endl;
    } else if(mode==3) {
        vector<long long> counters(4);
        for(int i=0; i<4; i++) {
            threads.emplace_back(version_3, &counters[i]);
        }
        for(auto &i : threads) {
            i.join();
        }
        long long count = 0;
        for(int i=0; i<4; i++) {
            count += counters[i];
        }
        cout << count << endl;
    }
    return 0;
}