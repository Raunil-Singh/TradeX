#include <thread>
#include <iostream>
#include <mutex>
#include <chrono>

std::mutex cout_mtx;
const int NUM = 500000;
const int ITER_NUM = 3;   
// Unsynchronized

void increment_unsync(long long &counter)
{
    {
        std::lock_guard<std::mutex> cout_lock(cout_mtx);
        std::cout << "Increment thread ID: " << std::this_thread::get_id();
        std::cout << std::endl;
    }
    for (long long i = 0; i < NUM; i++)
    {
        counter++;
    }
}

void decrement_unsync(long long &counter)
{
    {
        std::lock_guard<std::mutex> cout_lock(cout_mtx);
        std::cout << "Decrement thread ID: " << std::this_thread::get_id();
        std::cout << std::endl;
    }
    for (long long i = 0; i < NUM; i++)
    {
        counter--;
    }
}

void unsynchronized_thread(long long &counter)
{

    std::thread t1(increment_unsync, std::ref(counter));
    std::thread t2(increment_unsync, std::ref(counter));
    std::thread t3(decrement_unsync, std::ref(counter));
    std::thread t4(decrement_unsync, std::ref(counter));

    t1.join();
    t2.join();
    t3.join();
    t4.join();
}

// Synchronized
std::mutex mtx;
void increment_sync(long long &counter)
{
    {
        std::lock_guard<std::mutex> cout_lock(cout_mtx);
        std::cout << "Increment thread ID: " << std::this_thread::get_id();
        std::cout << std::endl;
    }
    for (long long i = 0; i < NUM; i++)
    {
        std::lock_guard<std::mutex> lock(mtx);
        counter++;
    }
}

void decrement_sync(long long &counter)
{
    {
        std::lock_guard<std::mutex> cout_lock(cout_mtx);
        std::cout << "Decrement thread ID: " << std::this_thread::get_id();
        std::cout << std::endl;
    }
    for (long long i = 0; i < NUM; i++)
    {
        std::lock_guard<std::mutex> lock(mtx);
        counter--;
    }
}

void synchronized_thread(long long &counter)
{
    std::thread t1(increment_sync, std::ref(counter));
    std::thread t2(increment_sync, std::ref(counter));
    std::thread t3(decrement_sync, std::ref(counter));
    std::thread t4(decrement_sync, std::ref(counter));

    t1.join();
    t2.join();
    t3.join();
    t4.join();
}

int main()
{
    long long counter = 0;

    std::cout << "=== UNSYNCHRONIZED VERSION ===\n";
    std::cout << "Counter before: " << counter << std::endl;
    long long avg_unsyn_dur = 0;
    for (int i = 0; i < ITER_NUM; i++)
    {
        std::cout<<"RUN "<<i+1<<" : \n";
        counter = 0;
        auto start_unsync = std::chrono::high_resolution_clock::now();
        unsynchronized_thread(counter);
        auto end_unsync = std::chrono::high_resolution_clock::now();
        auto duration_unsync = std::chrono::duration_cast<std::chrono::milliseconds>(end_unsync - start_unsync);
        avg_unsyn_dur += duration_unsync.count();
        std::cout << "Counter after Run "<<i+1<<" : " << counter << std::endl;
    }
    long long duration_unsync = avg_unsyn_dur / 3;
    std::cout << "Average Time taken (unsynchronized): " << duration_unsync << " ms\n";

    counter = 0;

    std::cout << "\n=== SYNCHRONIZED VERSION ===\n";
    std::cout << "Counter before: " << counter << std::endl;
    long long avg_sync_dur = 0;
    for (int i = 0; i < ITER_NUM; i++)
    {
        std::cout<<"RUN "<<i+1<<" :\n";
        counter = 0;
        auto start_sync = std::chrono::high_resolution_clock::now();
        synchronized_thread(counter);
        auto end_sync = std::chrono::high_resolution_clock::now();
        auto duration_sync = std::chrono::duration_cast<std::chrono::milliseconds>(end_sync - start_sync);
        avg_sync_dur += duration_sync.count();
        std::cout << "Counter after Run "<<i+1<<" : " << counter << std::endl;
    }
    long long duration_sync = avg_sync_dur / 3;
    std::cout << "Average Time taken (synchronized): " << duration_sync << " ms\n";

    return 0;
}