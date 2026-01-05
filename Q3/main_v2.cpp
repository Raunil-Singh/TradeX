#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <random>
#include <shared_mutex>
#include <chrono>
constexpr int EXEC_TIME{5};
constexpr int ARRAY_SIZE{1000};
constexpr int READS{100'000};
constexpr int WRITES{10'000};

std::vector<double> priceArray(ARRAY_SIZE, 100.0);
std::shared_mutex priceArrayMutex;
std::atomic<bool> shutdown{false};

std::uniform_int_distribution<> randomArrayIndice(0, ARRAY_SIZE - 1);
std::uniform_real_distribution<> randomDistrubtion(-10.0, 10.0);
std::vector<double> averageTable(8);
std::vector<std::chrono::steady_clock::duration> averageWaitTimeReader(8);
std::vector<std::chrono::steady_clock::duration> averageWaitTimeWriter(2);

void readerThread(int id){
    std::mt19937 rGen{std::random_device{}()};
    std::chrono::steady_clock::time_point waitTimeStart, waitTimeEnd;
    std::chrono::steady_clock::duration avgWaitTime{};
    while(!shutdown.load(std::memory_order_relaxed)){
        double avg{};
        for(int i = 0; i < READS; i++){
            waitTimeStart = std::chrono::steady_clock::now();
            {
                std::shared_lock<std::shared_mutex> shareLock(priceArrayMutex);
                waitTimeEnd = std::chrono::steady_clock::now();
                avg += priceArray[randomArrayIndice(rGen)];
            }
            avgWaitTime += waitTimeEnd - waitTimeStart;
        }
        averageWaitTimeReader[id] = avgWaitTime/READS;
        averageTable[id] = avg/READS;
    }
}

void writerThread(int id){
    std::mt19937 rGen{std::random_device{}()};
    std::chrono::steady_clock::time_point waitTimeStart, waitTimeEnd;
    std::chrono::steady_clock::duration avgWaitTime{};
    while(!shutdown.load(std::memory_order_relaxed)){
        for(int i = 0; i < WRITES; i++){
            waitTimeStart = std::chrono::steady_clock::now();
            {
                std::lock_guard<std::shared_mutex> exLock(priceArrayMutex);
                waitTimeEnd = std::chrono::steady_clock::now();
                priceArray[randomArrayIndice(rGen)] += randomDistrubtion(rGen);
            }
            avgWaitTime += waitTimeEnd - waitTimeStart;
        }
        averageWaitTimeWriter[id] = avgWaitTime/WRITES;
    }
}
int main(){
    std::vector<std::thread> readerThreadPool;
    readerThreadPool.reserve(8);
    std::vector<std::thread> writerThreadPool;
    writerThreadPool.reserve(2);
    for(int i = 0; i < 8; i++){
        readerThreadPool.push_back(std::thread(readerThread, i));
    }
    for(int i = 0; i < 2; i++){
        writerThreadPool.push_back(std::thread(writerThread, i));
    }
    std::this_thread::sleep_for(std::chrono::seconds(EXEC_TIME));
    shutdown.store(true, std::memory_order_relaxed);
    for(auto& thread: readerThreadPool){
        thread.join();
    }
    for(auto& thread:writerThreadPool){
        thread.join();
    }
    double avgFromTable{};
    double trueAvg{};
    for(auto& val: averageTable){
        avgFromTable += val;
    }
    for(auto& val: priceArray){
        trueAvg += val;
    }
    avgFromTable /= 8;
    trueAvg /= 1000;
    
    std::cout<<"Main released after "<<EXEC_TIME<<", final average: "<<trueAvg<<", and from threads: "<<avgFromTable<<".\n";
    for(int i = 0; i < 8; i++){
        std::cout<<"Reader "<<i<<" average wait time: "<<std::chrono::duration_cast<std::chrono::microseconds>(averageWaitTimeReader[i]).count()<<" \n";
    }
    for(int i = 0; i < 2; i++){
        std::cout<<"Writer "<<i<<" average wait time: "<<std::chrono::duration_cast<std::chrono::microseconds>(averageWaitTimeWriter[i]).count()<<" \n";
    }
}