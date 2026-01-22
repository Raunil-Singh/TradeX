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
#include <bits/stdc++.h>
using namespace std;

const int NUM_ORDERS = 5'000'000;
const int NUM_THREADS = 4;
const double MIN_PRICE = 99.00;
const double MAX_PRICE = 101.00;
const int PRICE_LEVELS = 200; // (101-99) * 100

struct Order {
    double price;
    int quantity;
};

vector<Order> gen() {
    vector<Order> orders(NUM_ORDERS);
    mt19937 rng(42); // With random_device every run is different, so use fixed seed 
    uniform_int_distribution<int> price(0, 199); 
    uniform_int_distribution<int> qty(1, 1000);

    for (int i = 0; i < NUM_ORDERS; i++) {
        orders[i].price = MIN_PRICE + (price(rng) * 0.01);
        orders[i].quantity = qty(rng);
    }
    return orders;
}

long long v1(const vector<Order> &orders) {
    map<double, long long> order_book;
    
    auto start = chrono::high_resolution_clock::now();
    for (const auto &order : orders) {
        order_book[order.price] += order.quantity;
    }
    auto end = chrono::high_resolution_clock::now();
    double time = chrono::duration<double, milli>(end - start).count();
    
    long long total_vol = 0;
    for(auto i : order_book) total_vol += i.second;
    
    cout << "Version 1 : " << endl;
    cout << "Time = " << time << " ms" << " | " << "Throughput = " << (int)(NUM_ORDERS/time) << " orders per ms" << endl;

    return total_vol;
}

void v2_worker(const vector<Order>& orders, int start, int end, map<double, long long>& local_map) {
    for (int i = start; i < end; i++) {
        local_map[orders[i].price] += orders[i].quantity;
    }
}

long long v2_parallel(const vector<Order>& orders) {
    auto start = chrono::high_resolution_clock::now();

    vector<thread> threads;
    vector<map<double, long long>> local_map(NUM_THREADS);
    int block = NUM_ORDERS/NUM_THREADS;

    for (int i = 0; i < NUM_THREADS; i++) {
        int s = i*block;
        int e = (i+1)*block;
        threads.emplace_back(v2_worker, ref(orders), s, e, ref(local_map[i]));
    }
    for (auto& t : threads) t.join();

    map<double, long long> global_book;
    for (int i = 0; i < NUM_THREADS; i++) {
        for (auto j : local_map[i]) {
            global_book[j.first] += j.second;
        }
    }

    auto end = chrono::high_resolution_clock::now();
    double time = chrono::duration<double, milli>(end - start).count();

    long long total_vol = 0;
    for(auto i : global_book) total_vol += i.second;

    cout << "Version 2 : " << endl;
    cout << "Time = " << time << " ms" << " | " << "Throughput = " << (int)(NUM_ORDERS/time) << " orders per ms" << endl;

    return total_vol;
}

atomic<long long> global_counts[PRICE_LEVELS];
void v3_worker(const vector<Order>& orders, int start, int end) {
    long long local_counts[PRICE_LEVELS]; 

    for (int i = start; i < end; i++) {
        int idx = (int)((orders[i].price - MIN_PRICE)*100.0 + 0.5); // +0.5 for rounding off fix
        local_counts[idx] += orders[i].quantity;
    }
    for (int i = 0; i < PRICE_LEVELS; i++) {
        if (local_counts[i] > 0) {
            global_counts[i].fetch_add(local_counts[i], memory_order_relaxed);
        }
    }
}

long long v3_optimized(const vector<Order>& orders) {
    for(int i=0; i<PRICE_LEVELS; i++) global_counts[i] = 0;

    auto start = chrono::high_resolution_clock::now();
    vector<thread> threads;
    int block = NUM_ORDERS/NUM_THREADS;
    for (int i=0; i<NUM_THREADS; i++) {
        int s = i*block;
        int e = (i+1)*block;
        threads.emplace_back(v3_worker, ref(orders), s, e);
    }
    for (auto& t : threads) t.join();
    auto end = chrono::high_resolution_clock::now();
    double time = chrono::duration<double, milli>(end-start).count();

    long long total_vol = 0;
    for(int i=0; i<PRICE_LEVELS; i++) total_vol += global_counts[i];

    cout << "Version 3 : " << endl;
    cout << "Time = " << time << " ms" << " | " << "Throughput = " << (int)(NUM_ORDERS/time) << " orders per ms" << endl;
    return total_vol;
}

int main() {
    auto orders = gen();
    cout << "Running Benchmark..." << endl;

    long long vol1 = v1(orders);
    long long vol2 = v2_parallel(orders);
    long long vol3 = v3_optimized(orders);

    if (vol1==vol2 && vol2==vol3) {
        cout << "Verification: PASS (Total Volume: " << vol1 << ")" << endl;
    } else {
        cout << "Verification: FAIL! Volumes do not match." << endl;
    }

    return 0;
}