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

const int N = 100'000'000;

void sequential(const vector<int> &A, const vector<int> &B, vector<int> &C) {
    for(int i=0; i<N; i++) {
        C[i] = A[i] + B[i];
    }
}

void parallel(const vector<int> &A, const vector<int> &B, vector<int> &C, int start, int end) {
    for(int i=start; i<end; i++) {
        C[i] = A[i] + B[i];
    }
}

void add_parallel(const vector<int> &A, const vector<int> &B, vector<int> &C) {
    int n_threads = 4;
    vector<thread> threads;
    int blocks = N/n_threads;
    
    for(int i=0; i<n_threads; i++) {
        int start = i*blocks;
        int end = (i+1)*blocks;
        threads.emplace_back(parallel, cref(A), cref(B), ref(C), start, end);
    }
    for(auto &t : threads) t.join();
}

void parallel_unrolling(const vector<int> &A, const vector<int> &B, vector<int> &C, int start, int end) {
    for(int i=start; i<end; i+=4) {
        C[i] = A[i] + B[i];
        C[i+1] = A[i+1] + B[i+1];
        C[i+2] = A[i+2] + B[i+2];
        C[i+3] = A[i+3] + B[i+3];
    }
}

void add_parallel_unrolling(const vector<int> &A, const vector<int> &B, vector<int> &C) {
    int n_threads = 4;
    vector<thread> threads;
    int blocks = N/n_threads;
    
    for(int i=0; i<n_threads; i++) {
        int start = i*blocks;
        int end = (i+1)*blocks;
        threads.emplace_back(parallel_unrolling, cref(A), cref(B), ref(C), start, end);
    }
    for(auto &t : threads) t.join();
}

void parallel_hints(const vector<int> &A, const vector<int> &B, vector<int> &C, int start, int end) {
    #pragma GCC ivdep
    for(int i=start; i<end; i++) {
        C[i] = A[i] + B[i];
    }
}

void add_parallel_hints(const vector<int> &A, const vector<int> &B, vector<int> &C) {
    int n_threads = 4;
    vector<thread> threads;
    int blocks = N/n_threads;
    
    for(int i=0; i<n_threads; i++) {
        int start = i*blocks;
        int end = (i+1)*blocks;
        threads.emplace_back(parallel_hints, cref(A), cref(B), ref(C), start, end);
    }
    for(auto &t : threads) t.join();
}

template <typename fdt> 
void run(string name, fdt func, const vector<int> &A, const vector<int> &B, vector<int> &C) {
    fill(C.begin(), C.end(), 0);
    
    auto start = chrono::high_resolution_clock::now();
    func(A,B,C);
    auto end = chrono::high_resolution_clock::now();

    cout << name << " time = " << chrono::duration_cast<chrono::milliseconds>(end-start).count() << " ms" << endl;
}

int main() {
    vector<int> A(N, 1);
    vector<int> B(N, 2);
    vector<int> C(N, 0);

    cout << "Running Benchmark..." << endl;

    run("1. Sequential", sequential, A, B, C);
    run("2. Parallel", add_parallel, A, B, C);
    run("3. Parallel+Unrolling", add_parallel_unrolling, A, B, C);
    run("4. Parallel+Hints", add_parallel_hints, A, B, C);
}