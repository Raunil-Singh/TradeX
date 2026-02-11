#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <string>
#include <vector>
#include <atomic>

#include "order.h"
#include "trade.h"
#include "order_book.h"

namespace matching_engine {
OrderBookManager bookmanager;

template<typename T>
class alignas(64) RingBuffer {
private:
    std::vector<T> buffer;
    size_t capacity;
    std::atomic<uint64_t> head;
    std::atomic<uint64_t> tail;

public:
    explicit RingBuffer(size_t cap) : capacity(cap), head(0), tail(0) {
        buffer.resize(capacity);
    }

    bool push(const T &item) {
        uint64_t cur_tail = tail.load(std::memory_order_relaxed);
        uint64_t next_tail = (cur_tail+1)%capacity;
        if(next_tail == head.load(std::memory_order_acquire)) return false;

        buffer[cur_tail] = item;
        tail.store(next_tail, std::memory_order_release);
        return true;
    }

    bool pop(T &item) {
        uint64_t cur_head = head.load(std::memory_order_relaxed);
        if(cur_head == tail.load(std::memory_order_acquire)) return false;

        item = buffer[cur_head];
        head.store((cur_head+1)%capacity, std::memory_order_release);
        return true;
    }    
};


class MatchingEngineDispatcher {
private:
    const int NUM_GROUPS = 5;
    std::vector<RingBuffer<Order>> group_queues;

public:
    MatchingEngineDispatcher(uint64_t capacity) {
        for(int i=0; i<NUM_GROUPS; i++) {
            group_queues.emplace_back(capacity);
        }
    }

    void start_dispatcher() {
        // This would receive orders from EMS via TCP socket
    }

    void dispatch_order(const Order& order) {
        group_queues[order.symbol_id/1000].push(order);
    }

    void start_group_thread(int group_no) {
        // Process orders from group queue 
        Order cur_order;
        while(true) {
            if(group_queues[group_no].pop(cur_order)) {
                process_order(cur_order);
            }
        }
    }

    void process_order(Order order) {

        if(order.type == OrderType::BUY) {
            OrderBook *book = bookmanager.get((order.symbol_id)%1000);
            if(book->bestsellprice() > order.price) {
                book->addOrder(order);
                return;
            }
            uint64_t current_price = book->bestsellprice();
            while(order.quantity > 0 && current_price <= order.price) {
                uint64_t price_index = book->priceToIndex(current_price);
                if(book->getorderside(price_index) == '0') {
                    current_price++;
                    continue;
                }
                if(book->getpricelevels(price_index)->gethead()->order.quantity > order.quantity) {
                    uint64_t new_quantity = book->getpricelevels(price_index)->gethead()->order.quantity - order.quantity;
                    book->modifyQuantity(current_price, new_quantity);
                    break;
                } else {
                    order.quantity -= book->getpricelevels(price_index)->gethead()->order.quantity;
                    book->removeOrder(current_price);
                }
            }
            if(order.quantity > 0) {
                book->addOrder(order);
            }
        } else {
            OrderBook *book = bookmanager.get((order.symbol_id)%1000);
            if(book->bestbuyprice() < order.price) {
                book->addOrder(order);
                return;
            }
            uint64_t current_price = book->bestbuyprice();
            while(order.quantity > 0 && current_price >= order.price) {
                uint64_t price_index = book->priceToIndex(current_price);
                if(book->getorderside(price_index) == '0') {
                    current_price--;
                    continue;
                }
                if(book->getpricelevels(price_index)->gethead()->order.quantity > order.quantity) {
                    uint64_t new_quantity = book->getpricelevels(price_index)->gethead()->order.quantity - order.quantity;
                    book->modifyQuantity(current_price, new_quantity);
                    break;
                } else {
                    order.quantity -= book->getpricelevels(price_index)->gethead()->order.quantity;
                    book->removeOrder(current_price);
                }
            }
            if(order.quantity > 0) {
                book->addOrder(order);
            }
        }
    }

    void start() { // Dispatcher thread
        std::thread dispatcher_thread(&MatchingEngineDispatcher::start_dispatcher, this);

        std::vector<std::thread> group_threads;
        for (int i = 0; i < NUM_GROUPS; ++i) {
            group_threads.emplace_back(&MatchingEngineDispatcher::start_group_thread, this, i);
        }
        
        dispatcher_thread.join();
        for (auto& t : group_threads) t.join();
    }
};
}

int main() {
    std::cout << "Starting System...";
    std::cout<<sizeof(matching_engine::Order)<<std::endl;
    matching_engine::MatchingEngineDispatcher dispatcher(10000); // Random value of capacity passed
    
    std::thread dispatcher_system([&dispatcher]() {
        dispatcher.start();
    });
    dispatcher_system.join();
    
    return 0;
}

/*

B<Actual Symbol name>

A -> BA -> 26

<character code> * 32 ^ 3 + 
<character code> * 32 ^ 2 + 
<character code> * 32 ^ 1 + 
<character code> * 32 ^ 0 

2^5 = (1<<5)

*/