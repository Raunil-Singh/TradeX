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


thread_local mt19937 rng(random_device{}());

void work(int min, int max) {
    uniform_int_distribution<int> dist(min, max);
    this_thread::sleep_for(chrono::milliseconds(dist(rng)));
}

class Barrier {
    private:
        mutex m;
        condition_variable cv;
        int count;
        int total_threads;
        int gen;

    public:
        Barrier(int n) : count(0), total_threads(n), gen(0) {}

        void wait() {
            unique_lock<mutex> mylock(m);
            int my_gen = gen;
            count++;
            if(count==total_threads) {
                count = 0;
                gen++;
                cv.notify_all();
            } else {
                cv.wait(mylock, [this, my_gen] {
                    return gen!=my_gen;
                });
            }
        }
};

void worker(int id, Barrier* bar, bool use_bar) {
    work(50,100); // Order Validation
    {
        static mutex print;
        lock_guard<mutex> p(print);
        cout << "Thread id : " << id << " Finished Validation" << endl;
    }
    if(use_bar) bar->wait();

    work(30,80); // Risk Check
    {
        static mutex print;
        lock_guard<mutex> p(print);
        cout << "Thread id : " << id << " Finished Risk Check" << endl;
    }
    if(use_bar) bar->wait();

    work(40,90); // Order Matching
    {
        static mutex print;
        lock_guard<mutex> p(print);
        cout << "Thread id : " << id << " Finished Order Matching" << endl;
    }
    if(use_bar) bar->wait();

    work(20,60); // Trade Reporting
    {
        static mutex print;
        lock_guard<mutex> p(print);
        cout << "Thread id : " << id << " Finished Trade Reporting" << endl;
    }
}


int main() {
    int num_threads = 4;
    Barrier bar(num_threads);
    vector<thread> threads;

    cout << endl << "Running Synchronized (With Barriers)" << endl;
    auto start = chrono::high_resolution_clock::now();
    for(int i=0; i<num_threads; i++) {
        threads.emplace_back(worker, i, &bar, true);
    }
    for(auto &t : threads) t.join();
    auto end = chrono::high_resolution_clock::now();
    cout << "Total Synchronized time = " << chrono::duration_cast<chrono::milliseconds>(end-start).count() << " ms" << endl << endl;

    cout << "Running Non-Synchronized" << endl;
    threads.clear();
    start = chrono::high_resolution_clock::now();
    for(int i=0; i<num_threads; i++) {
        threads.emplace_back(worker, i, &bar, false);
    }
    for(auto &t : threads) t.join();
    end = chrono::high_resolution_clock::now();
    cout << "Total Non-Synchronized time = " << chrono::duration_cast<chrono::milliseconds>(end-start).count() << " ms" << endl << endl;
}