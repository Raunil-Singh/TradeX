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

const int NUM_THREADS = 4;
const int NUM_ORDERS = 1'000'000;
const int NUM_ITERATIONS = 100;
volatile long long dummy = 0;

struct Order {
    int id;
    double price;
    int quantity;
    char side;
};
vector<Order> AoS_Data(NUM_ORDERS);

struct SoA_Data {
    vector<int> ids;
    vector<double> prices;
    vector<int> qtys;
    vector<char> sides;
};

void aos_scan(const vector<Order> &AoS_Data, int start, int end) {
    long long local_count = 0;
    for (int k = 0; k < NUM_ITERATIONS; k++) {
        for (int i = start; i < end; i++) {
            if (AoS_Data[i].price > 50.0) local_count++;
        }
    }
    dummy = local_count;
}

void soa_scan(const vector<double> &prices, int start, int end) {
    long long local_count = 0;
    for (int k = 0; k < NUM_ITERATIONS; k++) {
        for (int i = start; i < end; i++) {
            if (prices[i] > 50.0) local_count++;
        }
    }
    dummy = local_count;
}

void aos_update(vector<Order> &AoS_Data, int start, int end) {
    for (int k = 0; k < NUM_ITERATIONS; k++) {
        for (int i = start; i < end; i++) {
            AoS_Data[i].price += 0.01*AoS_Data[i].price;
        }
    }
}

void soa_update(vector<double> &prices, int start, int end) {
    for (int k = 0; k < NUM_ITERATIONS; k++) {
        for (int i = start; i < end; i++) {
            prices[i] += 0.01*prices[i];
        }
    }
}

void aos_filter(const vector<Order> &AoS_Data, int start, int end) {
    long long local_count = 0;
    for (int k = 0; k < NUM_ITERATIONS; k++) {
        for (int i = start; i < end; i++) {
            if (AoS_Data[i].price > 50.0 && AoS_Data[i].quantity > 100) local_count++;
        }
    }
    dummy = local_count;
}

void soa_filter(const vector<double> &prices, const vector<int> &qtys, int start, int end) {
    long long local_count = 0;
    for (int k = 0; k < NUM_ITERATIONS; k++) {
        for (int i = start; i < end; i++) {
            if (prices[i] > 50.0 && qtys[i] > 100) local_count++;
        }
    }
    dummy = local_count;
}

void run_aos(vector<Order> &orders) {
    cout << "Array of Structures Results : " << endl;
    int block = NUM_ORDERS/NUM_THREADS;

    auto start = chrono::high_resolution_clock::now();
    vector<thread> t1;
    for(int i=0; i<NUM_THREADS; i++) t1.emplace_back(aos_scan, ref(orders), i*block, (i+1)*block);
    for(auto& t : t1) t.join();
    auto end = chrono::high_resolution_clock::now();
    cout << "SCAN Time: " << chrono::duration_cast<chrono::milliseconds>(end-start).count() << " ms" << endl;

    start = chrono::high_resolution_clock::now();
    vector<thread> t2;
    for(int i=0; i<NUM_THREADS; i++) t2.emplace_back(aos_update, ref(orders), i*block, (i+1)*block);
    for(auto& t : t2) t.join();
    end = chrono::high_resolution_clock::now();
    cout << "UPDATE Time: " << chrono::duration_cast<chrono::milliseconds>(end-start).count() << " ms" << endl;

    start = chrono::high_resolution_clock::now();
    vector<thread> t3;
    for(int i=0; i<NUM_THREADS; i++) t3.emplace_back(aos_filter, ref(orders), i*block, (i+1)*block);
    for(auto& t : t3) t.join();
    end = chrono::high_resolution_clock::now();
    cout << "FILTER Time: " << chrono::duration_cast<chrono::milliseconds>(end-start).count() << " ms" << endl;
    cout << endl;
}

void run_soa(SoA_Data &data) {
    cout << "Structure of Arrays Results : " << endl;
    int block = NUM_ORDERS / NUM_THREADS;

    auto start = chrono::high_resolution_clock::now();
    vector<thread> t1;
    for(int i=0; i<NUM_THREADS; i++) t1.emplace_back(soa_scan, ref(data.prices), i*block, (i+1)*block);
    for(auto& t : t1) t.join();
    auto end = chrono::high_resolution_clock::now();
    cout << "SCAN Time: " << chrono::duration_cast<chrono::milliseconds>(end-start).count() << " ms" << endl;

    start = chrono::high_resolution_clock::now();
    vector<thread> t2;
    for(int i=0; i<NUM_THREADS; i++) t2.emplace_back(soa_update, ref(data.prices), i*block, (i+1)*block);
    for(auto& t : t2) t.join();
    end = chrono::high_resolution_clock::now();
    cout << "UPDATE Time: " << chrono::duration_cast<chrono::milliseconds>(end-start).count() << " ms" << endl;

    start = chrono::high_resolution_clock::now();
    vector<thread> t3;
    for(int i=0; i<NUM_THREADS; i++) t3.emplace_back(soa_filter, ref(data.prices), ref(data.qtys), i*block, (i+1)*block);
    for(auto& t : t3) t.join();
    end = chrono::high_resolution_clock::now();
    cout << "FILTER Time: " << chrono::duration_cast<chrono::milliseconds>(end-start).count() << " ms" << endl;
    cout << endl;
}


int main() {
    SoA_Data soa;
    soa.ids.resize(NUM_ORDERS);
    soa.prices.resize(NUM_ORDERS);
    soa.qtys.resize(NUM_ORDERS);
    soa.sides.resize(NUM_ORDERS);

    for(int i=0; i<NUM_ORDERS; i++) {
        AoS_Data[i] = {i, (double)(i%100), i%500, 'B'};

        soa.ids[i] = i;
        soa.prices[i] = (double)(i%100);
        soa.qtys[i] = i%500;
        soa.sides[i] = 'B';
    }

    run_aos(AoS_Data);
    run_soa(soa);

    return 0;
}