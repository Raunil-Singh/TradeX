#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <chrono>
#include <atomic>
#include <mutex>
using namespace std;


volatile long long counter = 0;
mutex m;

void unsynchronized(long long itr, bool inc) {
    cout << this_thread::get_id() << endl;
    for(int i=1; i<=itr; i++) {
        if(inc) counter++;
        else counter--;
    }
}

void synchronized(long long itr, bool inc) {
    cout << this_thread::get_id() << endl;
    for(int i=1; i<=itr; i++) {
        m.lock();
        if(inc) counter++;
        else counter--;
        m.unlock();
    }
}

int main() {
    const long long ITR = 500000;
    
    cout << "Starting unsynchronized..." << endl;
    auto start = chrono::high_resolution_clock::now();
    vector<thread> unsync;

    unsync.emplace_back(unsynchronized, ITR, true);
    unsync.emplace_back(unsynchronized, ITR, true);

    unsync.emplace_back(unsynchronized, ITR, false);
    unsync.emplace_back(unsynchronized, ITR, false);

    for(auto &i : unsync) i.join();

    auto end = chrono::high_resolution_clock::now();
    auto unsync_dur = chrono::duration_cast<chrono::microseconds>(end-start);
    cout << "Shared counter value = " << counter << endl;
    cout << "Total execution time = " << unsync_dur.count() << "s" << endl << endl;

    cout << "Starting synchronized..." << endl;
    start = chrono::high_resolution_clock::now();
    counter = 0;
    vector<thread> sync;
    
    sync.emplace_back(synchronized, ITR, true);
    sync.emplace_back(synchronized, ITR, true);

    sync.emplace_back(synchronized, ITR, false);
    sync.emplace_back(synchronized, ITR, false);

    for(auto &i : sync) i.join();

    end = chrono::high_resolution_clock::now();
    auto sync_dur = chrono::duration_cast<chrono::microseconds>(end-start);
    cout << "Shared counter value = " << counter << endl;
    cout << "Total execution time = " << sync_dur.count() << "s" << endl << endl;

}