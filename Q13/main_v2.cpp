#include <random>
#include <map>
#include <thread>
#include <mutex>
#include <vector>
#include <cmath>

constexpr int ORDERS{5'000'000};
constexpr int THREADS{4};
constexpr int SEGMENT_SIZE{ORDERS/THREADS};
enum TYPE{BUY, SELL};


std::uniform_int_distribution<> quantityDistribution(1, 1000);
std::uniform_int_distribution<> buySellDistribution(0, 1);
std::uniform_real_distribution<> priceDistribution(99.00, 101.00);
std::mt19937 rGen(std::random_device{}());


std::vector<int> quantityVector(ORDERS, quantityDistribution(rGen));
std::vector<double> priceVector(ORDERS, priceDistribution(rGen));
std::vector<int> type(ORDERS, buySellDistribution(rGen));

std::map<double, int> globalBuyMap;
std::map<double, int> globalSellMap;

std::mutex buyAndSellMutex;

void worker(int id){
    std::map<double, int> buyOrderMap;
    std::map<double, int> sellOrderMap;
    for(int i = (id * SEGMENT_SIZE); i < (id + 1) * SEGMENT_SIZE; i++){
        double bucket = (static_cast<int>(std::round(priceVector[i] * 100))) / 100.0;
        if(type[i] == BUY){
            buyOrderMap[bucket] += quantityVector[i];
        }
        else{
            sellOrderMap[bucket] += quantityVector[i];
        }
    }
    {
        std::unique_lock<std::mutex> lock(buyAndSellMutex);
        for(auto& [price, quantity]: buyOrderMap){
            globalBuyMap[price] += quantity;
        }
        for(auto& [price, quantity]: sellOrderMap){
            globalSellMap[price] += quantity;
        }
        lock.unlock();
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
