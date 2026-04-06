#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>

#include "order.h"
#include "trade.h"
#include "order_book.h"
#include "trade_ring_buffer.h"

namespace shared_data {
    struct MarketState {
        std::atomic<uint64_t> last_price[MAX_SYMBOLS];
    };
}

namespace matching_engine {

    constexpr int group_count = 1;


struct alignas(64) PaddedAtomic {
    std::atomic<uint64_t> value;
};

// PaddedAtomic processed_orders;

template<typename T>
class alignas(64) RingBuffer {
private:
    std::vector<T> buffer;
    size_t capacity;
    std::atomic<uint32_t> head;
    std::atomic<uint32_t> tail;

public:
    explicit RingBuffer(size_t cap) : capacity(cap), head(0), tail(0) {
        buffer.resize(capacity);
    }

    bool push(const T &item) {
        uint32_t cur_tail = tail.load(std::memory_order_relaxed);
        uint32_t next_tail = (cur_tail+1) & (capacity-1);
        if(next_tail == head.load(std::memory_order_acquire)) return false;

        buffer[cur_tail] = item;
        tail.store(next_tail, std::memory_order_release);
        return true;
    }

    bool pop(T &item) {
        uint32_t cur_head = head.load(std::memory_order_relaxed);
        while(cur_head == tail.load(std::memory_order_acquire)) return false;

        item = buffer[cur_head];
        head.store((cur_head+1) & (capacity-1), std::memory_order_release);
        return true;
    }    
};

class GroupProcessor {

    OrderBookManager * bookmanager = nullptr;               // Owned by GroupProcessor
    TradeRingBuffer::trade_ring_buffer trade_buffer;        // Stores the trade ring buffer for this group.
    std::unique_ptr<RingBuffer<Order>> group_queue;         // Queue for incoming orders.
    std::vector<int> order_count;                           // Count of orders received for each symbol
    uint64_t next_trade_id;                                 // ID that will be used for next trade that will be enqueued
    int symbol_count;
    int group_no;
    std::atomic<bool> terminate_flag{false};
    std::atomic<bool> is_running_flag{false};
    shared_data::MarketState* shared_ltp_ptr;

public:
    GroupProcessor(int group_no, size_t queue_capacity, shared_data::MarketState* shared_ptr) :
        group_queue(std::make_unique<RingBuffer<Order>>(queue_capacity)),
        trade_buffer{true, group_no},
        next_trade_id(1),
        group_no(group_no),
        shared_ltp_ptr(shared_ptr) {
    }

    ~GroupProcessor() {
        delete bookmanager;
    }

    // MatchingEngineDispatcher will call this to add the orderbooks into the manager
    OrderBookManager* setup_book_manager(int symbol_count = (1<<SYMBOL_BITS)) {
        bookmanager = new OrderBookManager(symbol_count);
        return bookmanager;
    }

    void start_group_thread(int group_no) {
        // Process orders from group queue 
        Order cur_order;
        // std::vector<int64_t> processing_times;
        // processing_times.reserve(9000000);
        //printf("Group %d started\n",group_no);
        
        is_running_flag.store(true, std::memory_order_release);
        
        while(!terminate_flag.load(std::memory_order_acquire)) {
            if(group_queue->pop(cur_order)) {
                // std::cout << "Processing order for group " << group_no << ": " << cur_order.order_id << std::endl;

                // auto start = std::chrono::high_resolution_clock::now();
                //printf("Order %ld recieved by matching-engine.\n",cur_order.order_id);
                process_order(cur_order);
                // auto end = std::chrono::high_resolution_clock::now();
                // std::cout << "Finished processing order for group " << group_no << ": " << cur_order.order_id << std::endl << std::endl;
                // processing_times.push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count());
                // processed_orders.value.fetch_add(1, std::memory_order_relaxed);

            }
        }
    
        // std::cerr << "Analysing processing times for group " << group_no << std::endl;
        // process_processing_times(processing_times);
    
    }

    void terminate() {
        terminate_flag.store(true, std::memory_order_release);
    }

    bool is_running() {
        return is_running_flag.load(std::memory_order_acquire);
    }


    bool enqueue_order(const Order& order) {
        return group_queue->push(order);
    }

    void process_processing_times(std::vector<int64_t>& times) {
        // Analyse processing times, e.g., calculate average, percentiles, etc.
        int64_t total_time = 0;
        for(int64_t t : times) total_time += t;
        double average_time = static_cast<double>(total_time) / times.size();
        std::cout << "Average processing time: " << average_time << " ns\n";

        // Percentiles
        std::sort(times.begin(), times.end());
        std::cout << "10th percentile: " << times[times.size() / 10] << " ns\n";
        std::cout << "50th percentile: " << times[times.size() / 2] << " ns\n";
        std::cout << "90th percentile: " << times[times.size() * 9 / 10] << " ns\n";
        std::cout << "99th percentile: " << times[times.size() * 99 / 100] << " ns\n";
    }

    void process_buy(Order order) {

        OrderBook &book = bookmanager->get((order.symbol_id) & SYMBOL_MASK);
        if(book.bestsellprice() > order.price) [[likely]] {
            book.addBuyOrder(order);
            //std::cout<<"Order"<<order.order_id<<"added to order book\n";
            return;
        }
        
        uint64_t best_price = book.bestsellprice();
        while(order.quantity > 0 && best_price <= order.price) {
            uint32_t price_index = book.priceToIndex(best_price);
            
            if(book.getpricelevels(price_index).gethead()->order.quantity > order.quantity) {
                
                publish_trade({
                    .trade_id = next_trade_id++,
                    .timestamp_ns = order.timestamp,
                    .price = best_price,
                    .buy_order_id = order.order_id,
                    .sell_order_id = book.getpricelevels(price_index).gethead()->order.order_id,
                    .symbol_id = order.symbol_id,
                    .quantity = order.quantity
                });
                shared_ltp_ptr->last_price[order.symbol_id & SYMBOL_MASK] = order.price;

                uint32_t new_quantity = book.getpricelevels(price_index).gethead()->order.quantity - order.quantity;
                book.modifyQuantity(best_price, new_quantity);
                return;
                
            } else {
                uint32_t traded_quantity = book.getpricelevels(price_index).gethead()->order.quantity;
                publish_trade({
                    .trade_id = next_trade_id++,
                    .timestamp_ns = order.timestamp,
                    .price = best_price,
                    .buy_order_id = order.order_id,
                    .sell_order_id = book.getpricelevels(price_index).gethead()->order.order_id,
                    .symbol_id = order.symbol_id,
                    .quantity = traded_quantity
                });
                shared_ltp_ptr->last_price[order.symbol_id & SYMBOL_MASK] = order.price;
                
                order.quantity -= traded_quantity;
                book.removeSellOrder(best_price);
                best_price = book.bestsellprice();
            }
        }
        if(order.quantity > 0) {
            book.addBuyOrder(order);
            //std::cout<<"Order"<<order.order_id<<"added to order book\n";
        }
    }

    void process_sell(Order order) {
        OrderBook &book = bookmanager->get((order.symbol_id) & SYMBOL_MASK);
        if(book.bestbuyprice() < order.price) [[likely]] {
            book.addSellOrder(order);
            //std::cout<<"Order"<<order.order_id<<"added to order book\n";
            return;
        }

        uint64_t best_price = book.bestbuyprice();
        while(order.quantity > 0 && best_price >= order.price) {
            uint64_t price_index = book.priceToIndex(best_price);

            if(book.getpricelevels(price_index).gethead()->order.quantity > order.quantity) {
            
                publish_trade({
                    .trade_id = next_trade_id++,
                    .timestamp_ns = order.timestamp,
                    .price = best_price,
                    .buy_order_id = book.getpricelevels(price_index).gethead()->order.order_id,
                    .sell_order_id = order.order_id,
                    .symbol_id = order.symbol_id,
                    .quantity = order.quantity
                });
                shared_ltp_ptr->last_price[order.symbol_id & SYMBOL_MASK] = order.price;
            
                uint32_t new_quantity = book.getpricelevels(price_index).gethead()->order.quantity - order.quantity;
                book.modifyQuantity(best_price, new_quantity);
                return;

            } else {
                uint32_t traded_quantity = book.getpricelevels(price_index).gethead()->order.quantity;
                publish_trade({
                    .trade_id = next_trade_id++,
                    .timestamp_ns = order.timestamp,
                    .price = best_price,
                    .buy_order_id = book.getpricelevels(price_index).gethead()->order.order_id,
                    .sell_order_id = order.order_id,
                    .symbol_id = order.symbol_id,
                    .quantity = traded_quantity
                });
                shared_ltp_ptr->last_price[order.symbol_id & SYMBOL_MASK] = order.price;
                
                order.quantity -= traded_quantity;
                book.removeBuyOrder(best_price);
                best_price = book.bestbuyprice();
            }
        }
        if(order.quantity > 0) {
            book.addSellOrder(order);
            //std::cout<<"Order"<<order.order_id<<"added to order book\n";
        }
    }

    void process_order(const Order& order) {
        if(order.type == OrderType::BUY) process_buy(order);
        else process_sell(order);
    }

    void publish_trade(Trade &&trade) {
            //printf("Trade Executed succesfully between %ld and %ld at %ld\n",trade.buy_order_id, trade.sell_order_id, trade.price);
            trade_buffer.add_trade(trade);
    }

};

class MatchingEngineDispatcher {
private:
    std::vector<std::unique_ptr<GroupProcessor>> groups;
    std::atomic<bool> terminate_flag{false};
    std::atomic<bool> is_running_flag{false};
    shared_data::MarketState* shared_ltp_ptr;
    uint64_t lower_limits_cache[MAX_SYMBOLS];
    uint64_t upper_limits_cache[MAX_SYMBOLS];
    int shm_fd;

public:
    MatchingEngineDispatcher(uint64_t capacity) {
        shm_fd = shm_open("/oms_market_data", O_CREAT | O_RDWR, 0666);
        if (ftruncate(shm_fd, sizeof(shared_data::MarketState)) == -1) {
            perror("ftruncate");
        }
        shared_ltp_ptr = (shared_data::MarketState*)mmap(NULL, sizeof(shared_data::MarketState), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
            
        for(int i = 0; i<MAX_SYMBOLS; i++) shared_ltp_ptr->last_price[i] = 0;

        groups.reserve(group_count);
        for(int i=0; i<group_count; i++) {
            groups.emplace_back(std::make_unique<GroupProcessor>(i, capacity, shared_ltp_ptr));
            
        }
    }

    void start_dispatcher() {
        // This would receive orders from EMS via TCP socket
    }

    void terminate() {
        terminate_flag.store(true, std::memory_order_release);
        for(auto &group : groups) {
            group->terminate();
        }
    }

    bool all_groups_running() {
        bool is_all_groups_running = true;
        for(auto &group : groups) {
            is_all_groups_running = is_all_groups_running && group->is_running();
        }
        return is_all_groups_running && is_running_flag.load(std::memory_order_acquire);
    }

    // Used for placing orders
    void dispatch_order(const Order& order) {

        int group = (order.symbol_id >> SYMBOL_BITS);
        //printf("Order %ld recieved\n",order.order_id);
        while(!groups[group]->enqueue_order(order));
        
    }

    void start() { // Dispatcher thread
        std::thread dispatcher_thread(&MatchingEngineDispatcher::start_dispatcher, this);

        std::vector<std::thread> group_threads;
        for (int i = 0; i < group_count; ++i) {
            group_threads.emplace_back(&GroupProcessor::start_group_thread, groups[i].get(), i);
        }

        std::cout << "Dispatcher and group threads started" << std::endl;
        is_running_flag.store(true, std::memory_order_release);
        
        dispatcher_thread.join();
        for (auto& t : group_threads) t.join();

        std::cout << "Dispatcher and group threads finished" << std::endl;

    }

    // One time setup to load order books into the book manager of each group processor.
    // This can be extended to support dynamic addition of order books as well.
    void initialize_engine(uint64_t* lower_limits, uint64_t* upper_limits)
    {
        for (uint32_t symbol_id = 0; symbol_id < MAX_SYMBOLS; symbol_id++) {
            lower_limits_cache[symbol_id] = lower_limits[symbol_id];
            upper_limits_cache[symbol_id] = upper_limits[symbol_id];
        }

        for(auto &group : groups) {
            auto book_manager = group->setup_book_manager();
            for(uint32_t symbol_id = 0; symbol_id < MAX_SYMBOLS; symbol_id++) {
                book_manager->addBook(symbol_id, 10000, lower_limits[symbol_id], upper_limits[symbol_id]);
            }
        }
    }

    uint64_t getUpperLimit(uint32_t symbol_id) const {
        return upper_limits_cache[symbol_id & SYMBOL_MASK];
    }

    uint64_t getLowerLimit(uint32_t symbol_id) const {
        return lower_limits_cache[symbol_id & SYMBOL_MASK];
    }

    ~MatchingEngineDispatcher() {
        munmap(shared_ltp_ptr, sizeof(shared_data::MarketState));
        close(shm_fd);
        shm_unlink("/oms_market_data");
    }
};
}

// int main() {
//     std::cout << "Starting System..." << std::endl;
//     std::cout << "Order size: " << sizeof(matching_engine::Order) << std::endl;
//     matching_engine::MatchingEngineDispatcher dispatcher(10000); // Random value of capacity passed
    
//     std::thread dispatcher_system([&dispatcher]() {
//         dispatcher.start();
//     });
//     dispatcher_system.join();
    
//     return 0;
// }
