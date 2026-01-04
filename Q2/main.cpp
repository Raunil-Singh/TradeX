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
using namespace std;


queue <function<void()>> tasks;
mutex m;
condition_variable cv;
bool stop = false;

void worker(int id) {
    while(true) {
        function<void()> current_task;
        unique_lock<mutex> mylock(m);
        cv.wait(mylock, []{
            return (!tasks.empty() || stop);
        });
        if(stop && tasks.empty()) {
            mylock.unlock();
            return;
        }
        current_task = tasks.front();
        tasks.pop();
        mylock.unlock();
        current_task();
    }
}

void task(int i) {
    long long sum = 0;
    for(int i=0; i<100000; i++) {
        int n = rand()%101; //Generates a random number between 0 and 100
        sum += n;
    }
    this_thread::sleep_for(chrono::milliseconds(10));
    mutex print;
    print.lock();
    cout << "Task ID = " << i << ", Processing thread ID = " << this_thread::get_id() << ", Computed Result = " << sum << endl;
}

int main() {
    vector <thread> threads;
    cout << "Starting thread pools..." << endl;
    auto start = chrono::high_resolution_clock::now();
    for(int i=0; i<4; i++) {
        threads.emplace_back(worker, i);
    }

    cout << "Adding 50 tasks..." << endl;

    for(int i=0; i<50; i++) {
        unique_lock<mutex> mylock(m);
        tasks.push(bind(task, i));
        mylock.unlock();
        cv.notify_one();
    }

    mutex m;
    m.lock();
    stop = true;
    m.unlock();
    cv.notify_all();
    for(auto &t : threads) t.join();
    auto end = chrono::high_resolution_clock::now();
    auto dur = chrono::duration_cast<chrono::milliseconds>(end-start);
    cout << " \n Thread pool execution time = " << dur.count() << endl;

    cout << "Starting sequential process..." << endl;
    start = chrono::high_resolution_clock::now();
    for(int i=0; i<50; i++) {
        long long sum = 0;
        for(int i=0; i<100000; i++) {
            int n = rand()%101;
            sum += n;
        }
        this_thread::sleep_for(chrono::milliseconds(10));
    }
    end = chrono::high_resolution_clock::now();
    dur = chrono::duration_cast<chrono::milliseconds>(end-start);
    cout << " \n Sequential execution time = " << dur.count() << endl;

    return 0;
}