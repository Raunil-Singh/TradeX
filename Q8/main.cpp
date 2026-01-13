#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <chrono>
#include <atomic>
#include <mutex>
#include <random>
#include <condition_variable>
#include <functional>
#include <queue>
#include <future>
#include <cstdlib>
#include <shared_mutex>
#include <algorithm>
using namespace std;


const int N_ITRS = 10000000;
volatile int counters1[8];
mutex m;
long long thread_local_total = 0;

struct padding {
    int val;
    char padding[60];
};
padding counters2[8];

struct atomic_padding {
    atomic<int> val;
    char padding[60];
};
atomic_padding counters3[8];

void shared_array(int i) {
    for(int j=0; j<N_ITRS; j++) {
        counters1[i]++;
    }
}

void padded_array(int i) {
    for(int j=0; j<N_ITRS; j++) {
        counters2[i].val++;
    }
}

void thread__local() {
    long long temp = 0;
    for(int i=0; i<N_ITRS; i++) temp++;
    lock_guard<mutex> mylock(m);
    thread_local_total += temp;
}

void atomic_with_padding(int i) {
    for(int j=0; j<N_ITRS; j++) {
        counters3[i].val++;
    }
}

int main() {

    vector<thread> t1;
    auto start = chrono::high_resolution_clock::now();
    for(int i=0; i<8; i++) t1.emplace_back(thread(shared_array, i));
    for(auto &t : t1) t.join();
    auto end = chrono::high_resolution_clock::now();
    long long total_sum = 0;
    for(int i=0; i<8; i++) total_sum += counters1[i];
    cout << "Shared array : " << endl;
    cout << "Total sum = " << total_sum << " | " << "Execution time = " << chrono::duration<double, milli>(end-start).count() << " ms" << endl;

    vector<thread> t2;
    start = chrono::high_resolution_clock::now();
    for(int i=0; i<8; i++) t2.emplace_back(thread(padded_array, i));
    for(auto &t : t2) t.join();
    end = chrono::high_resolution_clock::now();
    total_sum = 0;
    for(int i=0; i<8; i++) total_sum += counters2[i].val;
    cout << "Padded array : " << endl;
    cout << "Total sum = " << total_sum << " | " << "Execution time = " << chrono::duration<double, milli>(end-start).count() << " ms" << endl;

    vector<thread> t3;
    start = chrono::high_resolution_clock::now();
    for(int i=0; i<8; i++) t3.emplace_back(thread(thread__local));
    for(auto &t : t3) t.join();
    end = chrono::high_resolution_clock::now();
    cout << "Thread local : " << endl;
    cout << "Total sum = " << thread_local_total << " | " << "Execution time = " << chrono::duration<double, milli>(end-start).count() << " ms" << endl;

    vector<thread> t4;
    start = chrono::high_resolution_clock::now();
    for(int i=0; i<8; i++) t4.emplace_back(thread(atomic_with_padding, i));
    for(auto &t : t4) t.join();
    end = chrono::high_resolution_clock::now();
    total_sum = 0;
    for(int i=0; i<8; i++) total_sum += counters3[i].val;
    cout << "Atomic with padding array : " << endl;
    cout << "Total sum = " << total_sum << " | " << "Execution time = " << chrono::duration<double, milli>(end-start).count() << " ms" << endl;

    return 0;
}