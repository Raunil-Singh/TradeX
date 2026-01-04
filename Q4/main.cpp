#include <thread>
#include <queue>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <iostream>
#include <random>
#include <algorithm>
#include <atomic>

constexpr int MAX_QUEUE_SIZE{20};
constexpr int PRODUCER_THREAD_COUNT{3};
constexpr int CONSUMER_THREAD_COUNT{2};
constexpr int FAST_TIME_WAIT{10};
constexpr int MEDIUM_TIME_WAIT{25};
constexpr int SLOW_TIME_WAIT{50};
constexpr double MAX_PRICE{100'000};
constexpr int MAX_ORDER_SIZE{10'000};
const int array[] = {FAST_TIME_WAIT, MEDIUM_TIME_WAIT, SLOW_TIME_WAIT};

std::uniform_real_distribution<> priceDistribution(1, MAX_PRICE);
std::uniform_int_distribution<> orderSizeDistribution(1, MAX_ORDER_SIZE);

struct Order{
    int orderId;
    double price;
    int quantity;

    Order(std::mt19937& rGen):
    orderId(rGen()),
    price(priceDistribution(rGen)),
    quantity(orderSizeDistribution(rGen))
    {
    }
};

std::queue<Order> orderQueue;
std::mutex orderQueueMutex;
std::condition_variable queueNotEmpty;
std::condition_variable queueNotFull;
std::atomic<bool> shutdown{false};
std::atomic<int> activeProducers{PRODUCER_THREAD_COUNT};
std::vector<int> ordersProduced(PRODUCER_THREAD_COUNT, 0);
std::vector<int> ordersConsumed(CONSUMER_THREAD_COUNT, 0);

int maxQueueSize{0};
void producer(int id, int timeWait){
    int ordersProducedLocal{};
    std::mt19937 rGen{std::random_device{}()};
    while(!shutdown.load()){
        {
            std::unique_lock<std::mutex> lock(orderQueueMutex);
            queueNotFull.wait(lock, []{
                return (orderQueue.size() < MAX_QUEUE_SIZE)||shutdown;
            });
            if(!shutdown.load()){
                orderQueue.push(Order(rGen));
                maxQueueSize = std::max(maxQueueSize, static_cast<int>(orderQueue.size()));
                lock.unlock();
                queueNotEmpty.notify_one();
                ordersProducedLocal++;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(timeWait));
    }
    if(--activeProducers == 0){
        queueNotEmpty.notify_all();
    }
    ordersProduced[id] = ordersProducedLocal;

}
void consumer(int id){
    int ordersConsumedLocal{};
    while(true){
        {
            std::unique_lock<std::mutex> lock(orderQueueMutex);

            queueNotEmpty.wait(lock, []{
                return !(orderQueue.empty())||shutdown;
            });
            if(orderQueue.empty() && shutdown && activeProducers == 0){
                break;
            }
            orderQueue.pop();
            ordersConsumedLocal++;
            lock.unlock();
            queueNotFull.notify_one();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }
    ordersConsumed[id] = ordersConsumedLocal;
}

int main(){
    std::vector<std::thread> consumerThreadPool;
    consumerThreadPool.reserve(CONSUMER_THREAD_COUNT);
    std::vector<std::thread> producerThreadPool;
    producerThreadPool.reserve(PRODUCER_THREAD_COUNT);

    for(int i = 0; i < CONSUMER_THREAD_COUNT; i++){
        consumerThreadPool.push_back(std::thread(consumer, i));
    }

    for(int i = 0; i < PRODUCER_THREAD_COUNT; i++){
        producerThreadPool.push_back(std::thread(producer, i, array[i]));
    }
    std::this_thread::sleep_for(std::chrono::seconds(5));
    shutdown.store(true);
    queueNotFull.notify_all();
    queueNotEmpty.notify_all();
    for(auto& thread: consumerThreadPool){
        thread.join();
    }

    for(auto& thread: producerThreadPool){
        thread.join();
    }

    std::cout<<"After 5 seconds, the results are: \n";
    for(int i = 0; i < PRODUCER_THREAD_COUNT; i++){
        std::cout<<"Thread id :"<<i<<", orders produced: "<<ordersProduced[i]<<std::endl;
    }
    for(int i = 0; i < CONSUMER_THREAD_COUNT; i++){
        std::cout<<"Thread id: "<<i<<", orders consumed: "<<ordersConsumed[i]<<"\n";
    }

    std::cout<<"Max queue size: "<<maxQueueSize<<"\n";
}