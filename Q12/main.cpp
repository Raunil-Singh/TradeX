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

const int NUM_POS = 10000;
const int NUM_ITR = 100;

struct Position {
    double price;
    double quantity;
    double strike_price;
    double volatility;
    double time_to_maturity;
    double risk_free_rate;
    bool is_complex; 
};

mutex m;
double total = 0.0;

// Cumulative Distribution Function
double cdf(double value) {
    return 0.5 * erfc(-value * M_SQRT1_2);
}

double black_scholes(double S, double K, double T, double r, double sigma) {
    double d1 = (log(S / K) + (r + 0.5 * sigma * sigma) * T) / (sigma * sqrt(T));
    double d2 = d1 - sigma * sqrt(T);
    return S * cdf(d1) - K * exp(-r * T) * cdf(d2);
}

void worker(const vector<Position> &position, int start, int end) {
    double sum = 0.0;
    for (int k=0; k<NUM_ITR; k++) {
        for (int i=start; i<end; i++) {
            if (position[i].is_complex) {
                sum += black_scholes(position[i].price, position[i].strike_price, position[i].time_to_maturity, position[i].risk_free_rate, position[i].volatility)*position[i].quantity;
            } else {
                sum += position[i].price * position[i].quantity;
            }
        }
    }
    lock_guard<mutex> lock(m);
    total += sum;
}

double run(const vector<Position> &position, int num_threads) {
    total = 0.0;
    
    int block = NUM_POS/num_threads;
    vector<thread> threads;
    auto start = chrono::high_resolution_clock::now();

    for (int i = 0; i < num_threads; i++) {
        int start = i*block;
        int end = (i + 1)*block;
        threads.emplace_back(worker, ref(position), start, end);
    }
    for (auto &t : threads) t.join();

    auto end = chrono::high_resolution_clock::now();
    return chrono::duration_cast<chrono::duration<double, milli>>(end-start).count();
}

double amdahl(double p, int n) {
    // Speedup = 1 / ((1-P) + P/N)
    return 1.0/((1.0 - p)+(p/(double)n));
}

int main() {
    vector<Position> position(NUM_POS);
    for(int i=0; i<NUM_POS; i++) {
        position[i].price = double(i%100);
        position[i].quantity = i%10;
        position[i].strike_price = 100.0;
        position[i].volatility = 0.2;
        position[i].time_to_maturity = 1.0;
        position[i].risk_free_rate = 0.05;
        if(i%5==0) {
            position[i].is_complex = true;
        } else position[i].is_complex = false;
    }

    cout << "Running Benchmark..." << endl;
    double base_time = 0.0;
    vector<int> thread_count = {1,2,4,8,16};
    for(int i : thread_count) {
        double time = run(position, i);
        if(i==1) base_time = time;
        double speedup = base_time/time;
        double efficiency = speedup/i;
        double amdahl_speedup = amdahl(0.95, i);
        cout << "No. of threads = " << i << " | " << "Time = " << time << " ms | " << "Speedup = " << speedup << " | " << "Efficiency = " << efficiency << " | " << "Amdahl Speedup = " << amdahl_speedup << endl;
    }

    return 0;
}