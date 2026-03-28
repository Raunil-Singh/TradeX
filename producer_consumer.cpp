#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <string>
#include <vector>
#include <atomic>
#include <chrono>

#include "order.h"
#include "trade.h"
#include "order_book.h"
#include "trade_ring_buffer.h"

namespace shared_data {
    struct MarketState {
        uint64_t last_price[1000]; 
    };
}

namespace matching_engine {
OrderBookManager bookmanager;

std::atomic<uint64_t> next_trade_id{1};

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
    const int NUM_GROUPS = 2;
    std::vector<RingBuffer<Order>> group_queues;
    std::vector<TradeRingBuffer::trade_ring_buffer*> trade_buffers;             // One ring buffer per group_queue
    shared_data::MarketState* shared_ltp_ptr;

public:
    MatchingEngineDispatcher(uint64_t capacity) {
        for(int i=0; i<NUM_GROUPS; i++) {
            group_queues.emplace_back(capacity);    
            std::string shm_name = "/Trade_Ring_Buffer_" + std::to_string(i);   // Creating filenames      
            trade_buffers.push_back(new TradeRingBuffer::trade_ring_buffer(true, i));
        }

        int shm_fd = shm_open("/oms_market_data", O_CREAT | O_RDWR, 0666);
        ftruncate(shm_fd, sizeof(shared_data::MarketState));
        shared_ltp_ptr = (shared_data::MarketState*)mmap(NULL, sizeof(shared_data::MarketState), 
                          PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
        
        // Zero out prices initially
        for(int i=0; i<1000; i++) shared_ltp_ptr->last_price[i] = 0;
    }

    ~MatchingEngineDispatcher() {
        for(auto* t : trade_buffers) {
            delete t;
        }
    }

    void dispatch_order(const Order& order) {
        group_queues[order.symbol_id/1000].push(order);
    }

    void start_group_thread(int group_no) {
        // Process orders from group queue 
        Order cur_order;
        while(true) {
            if(group_queues[group_no].pop(cur_order)) {
                process_order(cur_order, group_no);
            }
        }
    }

    void publish_trade(int group_no, uint64_t buy_order_id, uint64_t sell_order_id, uint32_t symbol_id, uint64_t price, int32_t quantity) {
        Trade t;
        t.trade_id = next_trade_id.fetch_add(1, std::memory_order_relaxed);
        t.buy_order_id = buy_order_id;
        t.sell_order_id = sell_order_id;
        t.symbol_id = symbol_id;
        t.price = price;
        t.quantity = quantity;
        t.timestamp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        trade_buffers[group_no]->add_trade(t);
        shared_ltp_ptr->last_price[symbol_id % 1000] = price;
    }

    void process_order(Order order, int group_no) {
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
                OrderNode* best_sell = book->getpricelevels(price_index)->gethead();
                if(best_sell->order.quantity > order.quantity) {
                    uint64_t fill_qty = order.quantity;
                    publish_trade(group_no, order.order_id, best_sell->order.order_id, order.symbol_id, current_price, fill_qty);
                    uint64_t new_quantity = best_sell->order.quantity - order.quantity;
                    book->modifyQuantity(current_price, new_quantity);
                    break;
                } else {
                    uint64_t fill_qty = best_sell->order.quantity;
                    publish_trade(group_no, order.order_id, best_sell->order.order_id, order.symbol_id, current_price, fill_qty);
                    order.quantity -= fill_qty;
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
                OrderNode* best_buy = book->getpricelevels(price_index)->gethead();
                if(book->getpricelevels(price_index)->gethead()->order.quantity > order.quantity) {
                    uint64_t fill_qty = order.quantity;
                    publish_trade(group_no, best_buy->order.order_id, order.order_id, order.symbol_id, current_price, fill_qty);
                    uint64_t new_quantity = best_buy->order.quantity - order.quantity;
                    order.quantity = 0;
                    break;
                } else {
                    uint64_t fill_qty = best_buy->order.quantity;
                    publish_trade(group_no, best_buy->order.order_id, order.order_id, order.symbol_id, current_price, fill_qty);
                    order.quantity -= fill_qty;
                    book->removeOrder(current_price);
                }
            }
            if(order.quantity > 0) {
                book->addOrder(order);
            }
        }
    }

    void start() { // Dispatcher thread
        std::vector<std::thread> group_threads;
        for (int i = 0; i < NUM_GROUPS; ++i) {
            group_threads.emplace_back(&MatchingEngineDispatcher::start_group_thread, this, i);
        }
    
        for (auto& t : group_threads) t.join();
    }
};
}

int main() {
    std::cout << "Starting System..." << std::endl;
    std::cout << "Order size: " << sizeof(matching_engine::Order) << std::endl;
    matching_engine::MatchingEngineDispatcher dispatcher(10000); // Random value of capacity passed
    
    std::thread dispatcher_system([&dispatcher]() {
        dispatcher.start();
    });
    dispatcher_system.join();
    
    return 0;
}
