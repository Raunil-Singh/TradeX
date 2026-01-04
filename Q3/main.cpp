#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <chrono>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <queue>
#include <cstdlib>
#include <shared_mutex>
using namespace std;

vector<double> arr(1000,100.0);
const int READ_ITR = 100000;
const int WRITE_ITR = 10000;

mutex v1;
shared_mutex v2;
shared_mutex v3[10];

atomic<long long> total_exec_time = 0;

int rand_idx() {
    return rand()%1000;
}

void v1_reader() {
    long long sum = 0;
    for(int i=0; i<READ_ITR; i++) {
        int idx = rand_idx();
        auto start = chrono::high_resolution_clock::now();
        unique_lock<mutex> mylock(v1);
        double temp = 0;
        for(int j = 0; j < 100; j++) {
            temp += arr[(idx + j) % 1000] * 0.001;
        }
        sum += temp;
        auto end = chrono::high_resolution_clock::now();
        total_exec_time += chrono::duration_cast<chrono::nanoseconds>(end-start).count();
    } //auto unlock happens here at the end of loop
    auto start = chrono::high_resolution_clock::now();
    int avg = sum/READ_ITR;
    auto end = chrono::high_resolution_clock::now();
    total_exec_time += chrono::duration_cast<chrono::nanoseconds>(end-start).count();
}

void v1_writer() {
    for(int i=0; i<WRITE_ITR; i++) {
        int idx = rand_idx();
        auto start = chrono::high_resolution_clock::now();
        unique_lock<mutex> mylock(v1);
        for(int j = 0; j < 50; j++) {
            arr[(idx + j) % 1000] += 0.001;
        }
        auto end = chrono::high_resolution_clock::now();
        total_exec_time += chrono::duration_cast<chrono::nanoseconds>(end-start).count();
    }
}

void v2_reader() {
    long long sum = 0;
    for(int i=0; i<READ_ITR; i++) {
        int idx = rand_idx();
        auto start = chrono::high_resolution_clock::now();
        shared_lock<shared_mutex> mylock(v2);
        double temp = 0;
        for(int j = 0; j < 100; j++) {
            temp += arr[(idx + j) % 1000] * 0.001;
        }
        sum += temp;
        auto end = chrono::high_resolution_clock::now();
        total_exec_time += chrono::duration_cast<chrono::nanoseconds>(end-start).count();
    }
    auto start = chrono::high_resolution_clock::now();
    int avg = sum/READ_ITR;
    auto end = chrono::high_resolution_clock::now();
    total_exec_time += chrono::duration_cast<chrono::nanoseconds>(end-start).count();
}

void v2_writer() {
    for(int i=0; i<WRITE_ITR; i++) {
        int idx = rand_idx();
        auto start = chrono::high_resolution_clock::now();
        unique_lock<shared_mutex> mylock(v2);
        for(int j = 0; j < 50; j++) {
            arr[(idx + j) % 1000] += 0.001;
        }
        auto end = chrono::high_resolution_clock::now();
        total_exec_time += chrono::duration_cast<chrono::nanoseconds>(end-start).count();
    }
}

void v3_reader() {
    long long sum = 0;
    for(int i=0; i<READ_ITR; i++) {
        int idx = rand_idx();
        int segment = idx/100;
        auto start = chrono::high_resolution_clock::now();
        shared_lock<shared_mutex> mylock(v3[segment]);
        double temp = 0;
        for(int j = 0; j < 100; j++) {
            temp += arr[(idx + j) % 1000] * 0.001;
        }
        sum += temp;
        auto end = chrono::high_resolution_clock::now();
        total_exec_time += chrono::duration_cast<chrono::nanoseconds>(end-start).count();
    }
    auto start = chrono::high_resolution_clock::now();
    int avg = sum/READ_ITR;
    auto end = chrono::high_resolution_clock::now();
    total_exec_time += chrono::duration_cast<chrono::nanoseconds>(end-start).count();
}

void v3_writer() {
    for(int i=0; i<WRITE_ITR; i++) {
        int idx = rand_idx();
        int segment = idx/100;
        auto start = chrono::high_resolution_clock::now();
        unique_lock<shared_mutex> mylock(v3[segment]);
        for(int j = 0; j < 50; j++) {
            arr[(idx + j) % 1000] += 0.001;
        }
        auto end = chrono::high_resolution_clock::now();
        total_exec_time += chrono::duration_cast<chrono::nanoseconds>(end-start).count();
    }
}

int main() {
    long long total = (8*READ_ITR)+(2*WRITE_ITR);

    cout << "Running version 1..." << endl;
    vector<thread> t1;
    auto start = chrono::high_resolution_clock::now();
    for(int i=0; i<8; i++) t1.emplace_back(v1_reader);
    for(int i=0; i<2; i++) t1.emplace_back(v1_writer);
    for(auto &t : t1) t.join();
    auto end = chrono::high_resolution_clock::now();
    cout << "Total time = " << chrono::duration_cast<chrono::milliseconds>(end-start).count() << " ms" << endl;
    cout << "Avg wait time per thread = " << (total_exec_time/total) << " ns" << endl << endl;

    cout << "Running version 2..." << endl;
    total_exec_time = 0;
    vector<thread> t2;
    start = chrono::high_resolution_clock::now();
    for(int i=0; i<8; i++) t2.emplace_back(v2_reader);
    for(int i=0; i<2; i++) t2.emplace_back(v2_writer);
    for(auto &t : t2) t.join();
    end = chrono::high_resolution_clock::now();
    cout << "Total time = " << chrono::duration_cast<chrono::milliseconds>(end-start).count() << " ms" << endl;
    cout << "Avg wait time per thread = " << (total_exec_time/total) << " ns" << endl << endl;

    cout << "Running version 3..." << endl;
    total_exec_time = 0;
    vector<thread> t3;
    start = chrono::high_resolution_clock::now();
    for(int i=0; i<8; i++) t3.emplace_back(v3_reader);
    for(int i=0; i<2; i++) t3.emplace_back(v3_writer);
    for(auto &t : t3) t.join();
    end = chrono::high_resolution_clock::now();
    cout << "Total time = " << chrono::duration_cast<chrono::milliseconds>(end-start).count() << " ms" << endl;
    cout << "Avg wait time per thread = " << (total_exec_time/total) << " ns" << endl << endl;
}