#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <string>
#include <vector>
#include <atomic>

#include <order.h>
#include <trade.h>
#include <order_book.h>

template<typename T>
class ThreadSafeQueue {
private:
    std::queue<T> queue;
    std::mutex m_queue;
    std::condition_variable cv;

public:
    void push(T item) {
        {
            std::lock_guard<std::mutex> lock(m_queue);
            queue.push(std::move(item));
        }
        cv.notify_one();
    }

    T front() {
        std::lock_guard<std::mutex> lock(m_queue);
        return queue.front();
    }

    bool pop(T& item, int timeout_ms = 100) {
        std::unique_lock<std::mutex> lock(m_queue);
        if (cv.wait_for(lock, std::chrono::milliseconds(timeout_ms),[this]{
                return !this->queue.empty();
            })) {
            item = std::move(queue.front());
            queue.pop();
            return true;
        }
        return false;
    }

    size_t size() {
        std::lock_guard<std::mutex> lock(m_queue);
        return queue.size();
    }
};

class SymbolGroupMapping {
private:
    std::unordered_map<uint64_t, std::pair<int, OrderBook*>> symbol_to_group; // [Symbol_id -> (Group number, Orderbook)]
    std::mutex m_sgm;

public:
    void load_mapping(const std::string& file) {
        // Load from file thats generated based on volume analysis
    }

    std::pair<int, OrderBook*> get_group(const uint64_t& symbol_id) {
        std::lock_guard<std::mutex> lock(m_sgm);
        auto temp = symbol_to_group[symbol_id]; 
        return temp;
    }
};

class MatchingEngineDispatcher {
private:
    const int NUM_GROUPS = 5;
    std::vector<ThreadSafeQueue<std::pair<Order, OrderBook*>>> group_queues;
    SymbolGroupMapping symbol_mapping;

public:
    MatchingEngineDispatcher() : group_queues(NUM_GROUPS) {
        symbol_mapping.load_mapping("file_name");
    }

    void start_dispatcher() {
        // This would receive orders from EMS via TCP socket
    }

    void dispatch_order(const Order& order) {
        std::pair<int, OrderBook*> group_id = symbol_mapping.get_group(order.symbol_id);
        group_queues[group_id.first].push({order, group_id.second});
    }

    void start_group_thread(int group_no) {
        // Process orders from group queue 
        std::pair<Order, OrderBook*> cur_order = group_queues[group_no].front();
        process_order(cur_order);
    }

    void process_order(std::pair<Order, OrderBook*> order) {
        // Matching logic and orderbook instances
        /*
            if(order.type==0) {
                if(orderbook.best_sell_price > order.price) return 0;
                double start_price = orderbook.best_sell_price;
                while(order.quantity>0 && start_price <= buy_price) {
                    if(orderbook[start_price].empty()) {
                        start_price++;
                        continue;
                    }
                    if(start_price.quantity > order.quantity) {
                        // Inititalise trade
                        buffer.push(trade);
                        orderbook[sell_price].modify_order();
                        orderbook[buy_price].remove_order();
                        break;
                    } else if (start_price.quantity > 0) {
                        // Inititalise trade
                        buffer.push(trade);
                        orderbook[sell_price].remove_order();
                        orderbook[buy_price].modify_order();
                    } else {
                        map[symbol_id].push(order)          // Map of [symbol_id, orderbook]
                        break;
                    }
                    start_price++;
                }
                if(orderbook[start_price].empty()) {
                    while(!orderbook[start_price].empty()) start_price++;
                }
            }

        */
    }

    void start() { // Dispatcher thread
        std::thread dispatcher_thread(&MatchingEngineDispatcher::start_dispatcher, this);

        std::vector<std::thread> group_threads;
        for (int i = 1; i <= NUM_GROUPS; ++i) {
            group_threads.emplace_back(&MatchingEngineDispatcher::start_group_thread, this, i);
        }
        
        dispatcher_thread.join();
        for (auto& t : group_threads) t.join();
    }
};

int main() {
    std::cout << "Starting System...";
    
    MatchingEngineDispatcher dispatcher;
    
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