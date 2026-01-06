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

struct order {
    long long id;
    int price; 
    int quantity;
};

int const BUFFER_SIZE = 20;
queue<order> buffer;
mutex m;
bool stop = false;
condition_variable notfull;
condition_variable notempty;
long long prod_fast = 0;
long long prod_med = 0;
long long prod_slow = 0;
long long consumed_1 = 0;
long long consumed_2 = 0;
long long c = 1;
long long max_occupancy = 0;

void producer(int delay, long long &counter) {
    while(true) {
        if(stop) break;
        this_thread::sleep_for(chrono::milliseconds(delay));
        unique_lock<mutex> mylock(m);
        notfull.wait(mylock, []{
            return buffer.size()<BUFFER_SIZE || stop;
        });
        if(stop) break;
        order new_order = {c++, 50, 3};
        buffer.push(new_order);
        counter++;
        max_occupancy = max(max_occupancy, (long long)buffer.size());
        notempty.notify_one();
    }
}

void consumer(long long &counter) {
    while(true) {
        unique_lock<mutex> mylock(m);
        notempty.wait(mylock, [] {
            return !buffer.empty() || stop;
        });
        if(buffer.empty() && stop) break;
        order new_order = buffer.front();
        buffer.pop();
        counter++;
        notfull.notify_one();
        mylock.unlock();
        this_thread::sleep_for(chrono::milliseconds(15));
    }
}

int main() {
    cout << "Starting Producer-Consumer bounded buffer..." << endl;

    thread p1(producer, 10, ref(prod_fast));
    thread p2(producer, 25, ref(prod_med));
    thread p3(producer, 50, ref(prod_slow));
    thread c1(consumer, ref(consumed_1));
    thread c2(consumer, ref(consumed_2));

    this_thread::sleep_for(chrono::seconds(5));
    unique_lock<mutex> mylock(m);
    stop = true;
    mylock.unlock();
    notfull.notify_all(); 
    notempty.notify_all();

    p1.join();
    p2.join();
    p3.join();
    c1.join();
    c2.join();

    cout << "Fast Producer: " << prod_fast << endl;
    cout << "Medium Producer: " << prod_med << endl;
    cout << "Slow Producer: " << prod_slow << endl << endl;
    cout << "Consumer 1: " << consumed_1 << endl;
    cout << "Consumer 2: " << consumed_2 << endl << endl;
    cout << "Max buffer occupancy = " << max_occupancy << endl;

    return 0;
}