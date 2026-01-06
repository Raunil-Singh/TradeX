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
#include <algorithm>
using namespace std;
using Clock = chrono::high_resolution_clock;

const int N_PHIL = 5;
const int RUN_TIME = 30;
timed_mutex forks[N_PHIL];
atomic<bool> stop = false;
Clock::time_point last_eat;
vector<long long> eat_counts(N_PHIL);
vector<long long> total_wait_time(N_PHIL);
int MODE;

void think() {
    int dur = 100+(rand()%201);
    this_thread::sleep_for(chrono::milliseconds(dur));
}

void eat(int id) {
    int dur = 100+(rand()%101);
    this_thread::sleep_for(chrono::milliseconds(dur));
    last_eat = Clock::now();
    eat_counts[id]++;
}

void phil(int id) {
    int left = id;
    int right = (id+1)%N_PHIL;
    while(!stop) {
        think();
        auto start = Clock::now();
        if(MODE==0) {
            forks[left].lock();
            this_thread::sleep_for(chrono::milliseconds(150)); //Forcing Deadlock
            forks[right].lock();
            eat(id);
            forks[right].unlock();
            forks[left].unlock();
            auto end = Clock::now();
            total_wait_time[id] += chrono::duration_cast<chrono::milliseconds>(end-start).count();
        } else if(MODE==1) {
            if(id%2==0) {
                forks[right].lock();
                forks[left].lock();
            } else {
                forks[left].lock();
                forks[right].lock();
            }
            eat(id);
            forks[right].unlock();
            forks[left].unlock();
            auto end = Clock::now();
            total_wait_time[id] += chrono::duration_cast<chrono::milliseconds>(end-start).count();
        } else {
            if(forks[left].try_lock_for(chrono::milliseconds(100))) {
                if(forks[right].try_lock_for(chrono::milliseconds(100))) {
                    eat(id);
                    forks[right].unlock();
                }
                forks[left].unlock();
                auto end = Clock::now();
                total_wait_time[id] += chrono::duration_cast<chrono::milliseconds>(end-start).count();
            } else {
                this_thread::sleep_for(chrono::milliseconds(50));
            }
        }
    }
}

void check_deadlock() {
    while(!stop) {
        this_thread::sleep_for(chrono::seconds(1));
        auto now = Clock::now();
        auto gap = now-last_eat;
        if(gap>chrono::seconds(5)) {
            cout << "Deadlock Detected" << endl;
            exit(1);
        }
    }
}

int main(int argc, char* argv[]) {
    if(argc < 2) {
        cout << "Error : Enter 1, 2 or 3" << endl;
    }
    MODE = stoi(argv[1]);

    last_eat = Clock::now();
    thread check(check_deadlock);
    check.detach();
    
    vector<thread> threads;
    for(int i=0; i<N_PHIL; i++) {
        threads.emplace_back(thread(phil,i));
    }
    this_thread::sleep_for(chrono::seconds(30));
    stop = true;
    for(auto &t : threads) t.join();
    cout << "Final Result : " << endl;
    long long total = 0;
    for(int i=0; i<N_PHIL; i++) {
        cout << "Philosopher " << i << ": " << eat_counts[i] << " meals"  << ", Average Wait Time = " << (total_wait_time[i]/eat_counts[i]) << "ms" << endl;
    }
    return 0;
}