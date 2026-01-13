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


const int N_THREADS = 8;
const int N_ITRS = 1000000;
thread_local mt19937 rng(random_device{}());
thread_local uniform_int_distribution<int> dist(0,2);
long long total_ops = N_THREADS*N_ITRS;

struct mutex_counts {
    long long order_count = 0;
    long long trade_count = 0;
    long long cancel_count = 0;
    mutex m;

    void update() {
        int c = dist(rng);
        lock_guard<mutex> mylock(m);
        if(c==0) order_count++;
        else if (c==1) trade_count++;
        else cancel_count++;
    }
};

struct per_counter {
    long long order_count = 0;
    long long trade_count = 0;
    long long cancel_count = 0;
    mutex m1,m2,m3;

    void update() {
        int c = dist(rng);
        if(c==0) {
            {lock_guard<mutex> l1(m1); order_count++;};
        }
        else if(c==1) {
            {lock_guard<mutex> l2(m2); trade_count++;}; 
        }
        else {lock_guard<mutex> l3(m3); cancel_count++;};
    }
};

struct atomic_counts {
    atomic<long long> order_count = 0;
    atomic<long long> trade_count = 0;
    atomic<long long> cancel_count = 0;

    void update() {
        int c = dist(rng);
        if(c==0) order_count++;
        else if(c==1) trade_count++;
        else cancel_count++;
    }
};

struct atomic_relaxed {
    atomic<long long> order_count = 0;
    atomic<long long> trade_count = 0;
    atomic<long long> cancel_count = 0;

    void update() {
        int c = dist(rng);
        if(c==0) order_count.fetch_add(1,memory_order_relaxed);
        else if(c==0)trade_count.fetch_add(1,memory_order_relaxed);
        else cancel_count.fetch_add(1,memory_order_relaxed);
    }
};

template <typename T>
void test(string output) {
    T count;
    vector<thread> threads;

    auto start = chrono::high_resolution_clock::now();
    for(int i=0; i<N_THREADS; i++) {
        threads.emplace_back([&count]() {
            for(int j=0; j<N_ITRS; j++) {
                count.update();
            }
        });
    }
    for(auto &t : threads) t.join();

    // "Mutex Based Time = "
    auto end = chrono::high_resolution_clock::now();
    double dur = chrono::duration_cast<chrono::milliseconds>(end-start).count();
    long long actual_sum = count.order_count + count.trade_count + count.cancel_count;
    double throughput = total_ops/dur;

    cout << output << chrono::duration_cast<chrono::milliseconds>(end-start).count() << " ms" << " | " << "Throughput = " << 
        throughput << " | " << "Actual Sum = " << actual_sum <<  endl;
}

int main() {
    cout << "Running Benchmarks... " << endl;

    test<mutex_counts>("Mutex Based Time = ");
    test<per_counter>("Pre Counter Time = ");
    test<atomic_counts>("Standard Atomic Time = ");
    test<atomic_relaxed>("Atomic Relaxed Time = ");
}