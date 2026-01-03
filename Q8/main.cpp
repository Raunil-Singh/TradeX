#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <chrono>
#include <atomic>

void array_increment(int arr[], int ind){
    for(int i = 0; i<10000000; i++){
        arr[ind]++;
    }
}

struct alignas(64) PaddedInt {
    int value{0};
};

void padded_array_increment(PaddedInt* counter){
    for(int i = 0; i < 10000000; i++){
        counter->value++;
    }
}

void thread_storage_increment(int thread_id, int arr[]){
    thread_local int num;
    num = 0;

    for(int i = 0; i<10000000; i++){
        num++;
    }
    arr[thread_id] = num;
}

struct alignas(64) PaddedAtomic {
    std::atomic<int> value{0};
};

void atomic_increment(PaddedAtomic* counter) {
    for (int i = 0; i < 10000000; ++i) {
        counter->value.fetch_add(1, std::memory_order_relaxed);
    }
}

void shared_array(){
    int arr[8] = {};

    std::vector<std::thread> threads;

    auto start = std::chrono::high_resolution_clock::now();

    for(int i = 0; i<8; i++){
        threads.emplace_back(array_increment, arr, i);
    }

    for(auto &p : threads) p.join();

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    int sum = 0;
    for(int i = 0; i<8; i++) {
        sum+=arr[i];
    }
    std::cout<<"Shared Array\n";
    std::cout<<"Sum : "<<sum<<std::endl;
    std::cout<<"Execution Time : "<<duration.count()<<"ms\n";
}

void padded_array(){
    PaddedInt arr[8];         
    std::vector<std::thread> threads;

    auto start = std::chrono::high_resolution_clock::now();

    for(int i = 0; i<8; i++){
        threads.emplace_back(padded_array_increment, &arr[i]);
    }

    for(auto &p : threads) p.join();

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    int sum = 0;
    for(int i = 0; i<8; i++){
        sum += arr[i].value;
    }
    std::cout<<"Padded Array\n";
    std::cout<<"Sum : "<<sum<<std::endl;
    std::cout<<"Execution Time : "<<duration.count()<<"ms\n";
}

void thread_storage(){
    std::vector<int> counter(8,0);

    std::vector<std::thread> threads;

    auto start = std::chrono::high_resolution_clock::now();

    for(int i = 0; i<8; i++){
        threads.emplace_back(thread_storage_increment, i, counter.data());
    }

    for(auto &p : threads) p.join();

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    int sum = 0;
    for(int i = 0; i<8; i++){
        sum+=counter[i];
    }
    std::cout<<"Thread Local\n";
    std::cout<<"Sum : "<<sum<<std::endl;
    std::cout<<"Execution Time : "<<duration.count()<<"ms\n";
}

void padded_atomic(){
    PaddedAtomic counters[8];           

    std::vector<std::thread> threads;

    auto start = std::chrono::high_resolution_clock::now();

    for(int i = 0; i<8; i++){
        threads.emplace_back(atomic_increment, &counters[i]);
    }

    for(auto &p : threads) p.join();

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    int sum = 0;
    for (int i = 0; i < 8; ++i) {
        sum += counters[i].value.load(std::memory_order_relaxed);
    }
    std::cout<<"Atomic with Padding"<<std::endl;
    std::cout<<"Sum : "<<sum<<std::endl;
    std::cout<<"Execution Time : "<<duration.count()<<"ms\n";
}

int main(){
    shared_array();
    padded_array();
    thread_storage();
    padded_atomic();
}