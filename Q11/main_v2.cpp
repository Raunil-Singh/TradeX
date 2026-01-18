#include <thread>
#include <vector>
#include <random>
#include <iostream>
constexpr int ORDERS{1'000'000};
constexpr int THREADS{4};
constexpr int SEGMENT_SIZE{ORDERS/THREADS};
std::uniform_int_distribution<> orderQuantityDistribution(1, 10000);
std::uniform_real_distribution<> priceDistribution(1, 1000);

std::mt19937 rGen(std::random_device{}());

struct Order{
    int id;
    double price;
    int orderQuantity;

    Order():
    id(rGen()),
    price(priceDistribution(rGen)),
    orderQuantity(orderQuantityDistribution(rGen))
    {
    }
};
//SoA
std::vector<int> id(ORDERS);
std::vector<double> price(ORDERS);
std::vector<int> orderQuantity(ORDERS);
void setup(){
    for(int i = 0; i < ORDERS; i++){
        id[i] = rGen();
        price[i] = priceDistribution(rGen);
        orderQuantity[i] = orderQuantityDistribution(rGen);
    }
}
const int threshold{500};
const int priceThreshold2{700};
const int bulkThreshold{5000};
std::vector<int> thresholdCounter(THREADS);
std::vector<int> bulkCounter(THREADS);
void worker_scan(int id){
    int localThresholdCounter{};
    int localBulkCounter{};
    for(int iter = 0; iter < 1000; iter++){
        for(int i = id * SEGMENT_SIZE; i < (id + 1) * SEGMENT_SIZE; i++){
            if(price[i] > threshold){
                localThresholdCounter++;
            }
            if(price[i] > priceThreshold2 && orderQuantity[i] > bulkThreshold){
                localBulkCounter++;
            }
            price[i] *= 1.01;
        }
        
    }
    thresholdCounter[id] = localThresholdCounter;
}


int main(){
    setup();
    std::vector<std::thread> workerThreadPool;
    workerThreadPool.reserve(THREADS);

    for(int i = 0; i < THREADS; i++){
        workerThreadPool.emplace_back(worker_scan, i);
    }
    for(auto& thread: workerThreadPool){
        thread.join();
    }
    workerThreadPool.clear();

    int aggThresholdCounter{}, aggBulkCounter{};
    for(int i: thresholdCounter){
        aggThresholdCounter += i;
    }
    for(int i: bulkCounter){
        aggBulkCounter += i;
    }
    std::cout<<"Threshold Counter: "<<aggThresholdCounter<<", Bulk Counter: "<<aggBulkCounter<<" \n";
}