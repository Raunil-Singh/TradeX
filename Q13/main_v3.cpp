#include <random>
#include <thread>
#include <vector>
#include <array>
#include <cmath>
#include <atomic>
#include <algorithm>

constexpr int ORDERS{5'000'000};
constexpr int THREADS{4};
constexpr int SEGMENT_SIZE{ORDERS/THREADS};
constexpr int BUCKETS{200};
enum TYPE{BUY, SELL};


std::uniform_int_distribution<> quantityDistribution(1, 1000);
std::uniform_int_distribution<> buySellDistribution(0, 1);
std::uniform_real_distribution<> priceDistribution(99.00, 101.00);
std::mt19937 rGen(std::random_device{}());


std::vector<int> quantityVector(ORDERS, quantityDistribution(rGen));
std::vector<double> priceVector(ORDERS, priceDistribution(rGen));
std::vector<int> type(ORDERS, buySellDistribution(rGen));

std::array<std::atomic<int>, BUCKETS> globalBuyArray{};
std::array<std::atomic<int>, BUCKETS> globalSellArray{};

void worker(int id){
    std::array<int, BUCKETS> buyOrderArray{};
    std::array<int, BUCKETS> sellOrderArray{};
    for(int i = (id * SEGMENT_SIZE); i < (id + 1) * SEGMENT_SIZE; i++){
        int bucket = static_cast<int>(std::round((priceVector[i] - 99.0) * 100));
        bucket = std::min(std::max(bucket, 0), BUCKETS - 1);
        if(type[i] == BUY){
            buyOrderArray[bucket] += quantityVector[i];
        }
        else{
            sellOrderArray[bucket] += quantityVector[i];
        }
    }
    for(int i = 0; i < BUCKETS; i++){
        globalBuyArray[i].fetch_add(buyOrderArray[i], std::memory_order_relaxed);
    }
    for(int i = 0; i < BUCKETS; i++){
        globalSellArray[i].fetch_add(sellOrderArray[i], std::memory_order_relaxed);
    }
    
}

int main(){
    std::vector<std::thread> workerThreadPool;
    workerThreadPool.reserve(THREADS);
    for(int i = 0; i < THREADS; i++){
        workerThreadPool.emplace_back(worker, i);
    }
    
    for(auto& thread: workerThreadPool){
        thread.join();
    }
}
