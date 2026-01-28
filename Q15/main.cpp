#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <string>
#include <sstream>
#include <regex>
#include <map>
#include <atomic>
#include <random>
#include <chrono>

const int QUEUE_CAPACITY = 100;
const int TOTAL_MESSAGES = 1'000'000; 

struct Order {
    enum Type { ORDER, CANCEL, MODIFY };
    enum Side { BUY, SELL };
    Type type;
    Side side;
    std::string symbol;
    double price;
    int quantity;
    long order_id;
    bool is_valid;
    bool is_poison_pill = false;
};

template <typename T>
class BoundedQueue {
    std::queue<T> queue_data;
    std::mutex mtx;
    std::condition_variable not_full;
    std::condition_variable not_empty;
    size_t capacity;

public:
    BoundedQueue(size_t cap) : capacity(cap) {}

    void push(T item) {
        std::unique_lock<std::mutex> lock(mtx);
        not_full.wait(lock, [this] { return queue_data.size() < capacity; });
        queue_data.push(std::move(item));
        not_empty.notify_one();
    }

    T pop() {
        std::unique_lock<std::mutex> lock(mtx);
        not_empty.wait(lock, [this] { return !queue_data.empty(); });
        T item = std::move(queue_data.front());
        queue_data.pop();
        not_full.notify_one();
        return item;
    }
};

void simulateWork(int micros) {
    auto start = std::chrono::high_resolution_clock::now();
    while (std::chrono::duration_cast<std::chrono::microseconds>(
               std::chrono::high_resolution_clock::now() - start).count() < micros);
}

void parserWorker(BoundedQueue<std::string>& in_q, BoundedQueue<Order>& out_q) {
    while (true) {
        std::string raw = in_q.pop();
        if (raw == "STOP") {
            out_q.push({Order::ORDER, Order::BUY, "", 0, 0, 0, false, true});
            break;
        }

        std::vector<std::string> fields;
        std::stringstream ss(raw);
        std::string segment;
        while (std::getline(ss, segment, '|')) fields.push_back(segment);

        if (fields.size() != 6) continue;

        try {
            Order ord;
            ord.type = (fields[0] == "ORDER") ? Order::ORDER : (fields[0] == "CANCEL" ? Order::CANCEL : Order::MODIFY);
            ord.side = (fields[1] == "BUY") ? Order::BUY : Order::SELL;
            ord.symbol = fields[2];
            ord.price = std::stod(fields[3]);
            ord.quantity = std::stoi(fields[4]);
            ord.order_id = std::stol(fields[5]);
            ord.is_valid = true;
            
            simulateWork(2); 
            out_q.push(ord);
        } catch (...) { continue; }
    }
}

void validatorWorker(BoundedQueue<Order>& in_q, BoundedQueue<Order>& out_q) {
    const std::vector<std::string> whitelist = {"AAPL", "MSFT", "GOOGL", "AMZN", "TSLA", "META", "NVDA", "AMD", "INTC", "IBM"};
    std::regex symbol_regex("^[A-Z]{1,5}$");

    while (true) {
        Order ord = in_q.pop();
        if (ord.is_poison_pill) {
            out_q.push(ord);
            break;
        }

        bool valid = true;
        if (ord.price <= 0 || ord.price >= 1000000) valid = false;
        if (ord.quantity <= 0 || ord.quantity > 10000) valid = false;
        if (!std::regex_match(ord.symbol, symbol_regex)) valid = false;
        
        bool in_list = false;
        for (const auto& s : whitelist) if (s == ord.symbol) in_list = true;
        if (!in_list) valid = false;

        if (ord.side == Order::BUY && ord.price > 500.0) valid = false;
        if (ord.price * ord.quantity > 1000000) valid = false;

        ord.is_valid = valid;
        simulateWork(3);
        out_q.push(ord);
    }
}

void processorWorker(BoundedQueue<Order>& in_q, std::atomic<long>& trades, std::atomic<long>& volume) {
    std::map<double, int> buy_book;
    std::map<double, int> sell_book;

    while (true) {
        Order ord = in_q.pop();
        if (ord.is_poison_pill) break;
        if (!ord.is_valid) continue;

        if (ord.side == Order::BUY) buy_book[ord.price] += ord.quantity;
        else sell_book[ord.price] += ord.quantity;

        if (!buy_book.empty() && !sell_book.empty()) {
            auto best_buy = buy_book.rbegin();
            auto best_sell = sell_book.begin();

            if (best_buy->first >= best_sell->first) {
                int match_qty = std::min(best_buy->second, best_sell->second);
                trades.fetch_add(1, std::memory_order_relaxed);
                volume.fetch_add(match_qty, std::memory_order_relaxed);
                
                best_buy->second -= match_qty;
                best_sell->second -= match_qty;
                
                if (best_buy->second == 0) buy_book.erase(std::prev(buy_book.end()));
                if (best_sell->second == 0) sell_book.erase(sell_book.begin());
            }
        }
        simulateWork(7);
    }
}

std::string generateRaw() {
    static std::vector<std::string> syms = {"AAPL", "MSFT", "GOOGL", "AMZN", "TSLA"};
    static std::mt19937 rng(std::random_device{}());
    
    std::uniform_int_distribution<int> sym_dist(0, 4);
    std::uniform_int_distribution<int> side_dist(0, 1);

    std::uniform_real_distribution<double> price_dist(145.0, 155.0); 
    
    std::string side = (side_dist(rng) == 0) ? "BUY" : "SELL";
    std::string symbol = syms[sym_dist(rng)];
    
    return "ORDER|" + side + "|" + symbol + "|" + 
           std::to_string(price_dist(rng)) + "|100|12345678";
}

int main() {
    BoundedQueue<std::string> parse_q(QUEUE_CAPACITY);
    BoundedQueue<Order> validate_q(QUEUE_CAPACITY);
    BoundedQueue<Order> process_q(QUEUE_CAPACITY);

    std::atomic<long> total_trades{0};
    std::atomic<long> total_volume{0};

    std::vector<std::thread> parsers;
    for (int i = 0; i < 10; i++) parsers.emplace_back(parserWorker, std::ref(parse_q), std::ref(validate_q));

    std::vector<std::thread> validators;
    for (int i = 0; i < 8; i++) validators.emplace_back(validatorWorker, std::ref(validate_q), std::ref(process_q));

    std::vector<std::thread> processors;
    for (int i = 0; i < 4; i++) processors.emplace_back(processorWorker, std::ref(process_q), std::ref(total_trades), std::ref(total_volume));

    auto start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < TOTAL_MESSAGES; i++) parse_q.push(generateRaw());
    for (int i = 0; i < 10; i++) parse_q.push("STOP");

    for (auto& t : parsers) t.join();
    for (auto& t : validators) t.join();
    for (auto& t : processors) t.join();

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    std::cout << "Processed " << TOTAL_MESSAGES << " messages in " << duration << " ms" << std::endl;
    std::cout << "Trades: " << total_trades << ", Volume: " << total_volume << std::endl;

    return 0;
}