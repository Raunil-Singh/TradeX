#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <random>
#include <sstream>
#include <map>
#include <regex>
#include <iomanip>
#include <algorithm>

struct Order {
    enum Type { ORDER = 0, CANCEL = 1, MODIFY = 2 };
    enum Side { BUY = 0, SELL = 1 };
    
    Type type;
    Side side;
    std::string symbol;
    double price;
    int quantity;
    long order_id;
    bool is_valid;
    
    Order() : type(ORDER), side(BUY), price(0), quantity(0), order_id(0), is_valid(false) {}
};

class StringQueue {
private:
    std::queue<std::string> queue_;
    mutable std::mutex mutex_;  
    std::condition_variable cv_not_full_;
    std::condition_variable cv_not_empty_;
    size_t capacity_;
    bool shutdown_;
    std::atomic<size_t> total_enqueued_{0};
    std::atomic<size_t> total_dequeued_{0};
    
public:
    explicit StringQueue(size_t capacity) : capacity_(capacity), shutdown_(false) {}
    
    bool push(const std::string& item) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_not_full_.wait(lock, [this] { 
            return queue_.size() < capacity_ || shutdown_; 
        });
        
        if (shutdown_) return false;
        
        queue_.push(item);
        total_enqueued_++;
        cv_not_empty_.notify_one();
        return true;
    }
    
    bool pop(std::string& item) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_not_empty_.wait(lock, [this] { 
            return !queue_.empty() || shutdown_; 
        });
        
        if (queue_.empty() && shutdown_) return false;
        
        item = queue_.front();
        queue_.pop();
        total_dequeued_++;
        cv_not_full_.notify_one();
        return true;
    }
    
    void shutdown() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            shutdown_ = true;
        }
        cv_not_empty_.notify_all();
        cv_not_full_.notify_all();
    }
    
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }
    
    size_t total_enqueued() const { return total_enqueued_.load(); }
    size_t total_dequeued() const { return total_dequeued_.load(); }
};

class OrderQueue {
private:
    std::queue<Order> queue_;
    mutable std::mutex mutex_;  
    std::condition_variable cv_not_full_;
    std::condition_variable cv_not_empty_;
    size_t capacity_;
    bool shutdown_;
    std::atomic<size_t> total_enqueued_{0};
    std::atomic<size_t> total_dequeued_{0};
    
public:
    explicit OrderQueue(size_t capacity) : capacity_(capacity), shutdown_(false) {}
    
    bool push(const Order& item) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_not_full_.wait(lock, [this] { 
            return queue_.size() < capacity_ || shutdown_; 
        });
        
        if (shutdown_) return false;
        
        queue_.push(item);
        total_enqueued_++;
        cv_not_empty_.notify_one();
        return true;
    }
    
    bool pop(Order& item) {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_not_empty_.wait(lock, [this] { 
            return !queue_.empty() || shutdown_; 
        });
        
        if (queue_.empty() && shutdown_) return false;
        
        item = queue_.front();
        queue_.pop();
        total_dequeued_++;
        cv_not_full_.notify_one();
        return true;
    }
    
    void shutdown() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            shutdown_ = true;
        }
        cv_not_empty_.notify_all();
        cv_not_full_.notify_all();
    }
    
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }
    
    size_t total_enqueued() const { return total_enqueued_.load(); }
    size_t total_dequeued() const { return total_dequeued_.load(); }
};


class StageMetrics {
public:
    std::atomic<size_t> processed{0};
    std::atomic<size_t> errors{0};
    std::atomic<size_t> total_processing_time_us{0};
    std::chrono::steady_clock::time_point start_time;
    
    StageMetrics() : start_time(std::chrono::steady_clock::now()) {}
    
    void record_processing(size_t time_us) {
        processed++;
        total_processing_time_us += time_us;
    }
    
    void record_error() {
        errors++;
    }
    
    double get_throughput() const {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();
        return duration > 0 ? processed.load() / (double)duration : 0;
    }
    
    double get_avg_processing_time_us() const {
        size_t p = processed.load();
        return p > 0 ? total_processing_time_us.load() / (double)p : 0;
    }
};


class ParseStage {
private:
    StringQueue& input_queue_;
    OrderQueue& output_queue_;
    StageMetrics metrics_;
    std::vector<std::thread> threads_;
    

    void worker() {
        std::string raw_message;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> sleep_dist(1, 3);
        
        while (input_queue_.pop(raw_message)) {
            auto start = std::chrono::steady_clock::now();
            
            Order order;
            if (parse_message(raw_message, order)) {
                output_queue_.push(order);
            } else {
                metrics_.record_error();
            }
            
            std::this_thread::sleep_for(std::chrono::microseconds(sleep_dist(gen)));
            
            auto end = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
            metrics_.record_processing(duration);
        }
    }
    
    bool parse_message(const std::string& raw, Order& order) {
        try {
            std::vector<std::string> fields;
            std::stringstream ss(raw);
            std::string field;
            
            while (std::getline(ss, field, '|')) {
                fields.push_back(field);
            }
            
            if (fields.size() != 6) return false;
            
            if (fields[0] == "ORDER") order.type = Order::ORDER;
            else if (fields[0] == "CANCEL") order.type = Order::CANCEL;
            else if (fields[0] == "MODIFY") order.type = Order::MODIFY;
            else return false;
            
            if (fields[1] == "BUY") order.side = Order::BUY;
            else if (fields[1] == "SELL") order.side = Order::SELL;
            else return false;
            
            order.symbol = fields[2];
            order.price = std::stod(fields[3]);
            order.quantity = std::stoi(fields[4]);
            order.order_id = std::stol(fields[5]);
            order.is_valid = true; 
            
            return true;
        } catch (...) {
            return false;
        }
    }
    
public:
    ParseStage(StringQueue& input, OrderQueue& output, int num_threads = 10) : input_queue_(input), output_queue_(output) {
        for (int i = 0; i < num_threads; ++i) {
            threads_.emplace_back(&ParseStage::worker, this);
        }
    }
    
    void join() {
        for (auto& t : threads_) {
            if (t.joinable()) t.join();
        }
        output_queue_.shutdown();
    }
    
    const StageMetrics& get_metrics() const { return metrics_; }
};


class ValidateStage {
private:
    OrderQueue& input_queue_;
    OrderQueue& output_queue_;
    StageMetrics metrics_;
    std::vector<std::thread> threads_;
    
    const std::vector<std::string> allowed_symbols_ = {"AAPL", "MSFT", "GOOGL", "AMZN", "TSLA", "META", "NVDA", "AMD", "INTC", "IBM"};
    
    void worker() {
        Order order;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> sleep_dist(2, 5);
        
        while (input_queue_.pop(order)) {
            auto start = std::chrono::steady_clock::now();
            
            validate_order(order);
            output_queue_.push(order);
            
            if (!order.is_valid) {
                metrics_.record_error();
            }
            
            std::this_thread::sleep_for(std::chrono::microseconds(sleep_dist(gen)));
            
            auto end = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
            metrics_.record_processing(duration);
        }
    }
    
    void validate_order(Order& order) {
        if (order.price <= 0.0 || order.price >= 1000000.0) {
            order.is_valid = false;
            return;
        }
        
        if (order.quantity <= 0 || order.quantity > 10000) {
            order.is_valid = false;
            return;
        }
        
        std::regex symbol_regex("^[A-Z]{1,5}$");
        if (!std::regex_match(order.symbol, symbol_regex)) {
            order.is_valid = false;
            return;
        }
        
        if (order.order_id <= 0) {
            order.is_valid = false;
            return;
        }
        
        if (order.price * order.quantity > 1000000.0) {
            order.is_valid = false;
            return;
        }
        
        if (order.side == Order::BUY && order.price > 500.0) {
            order.is_valid = false;
            return;
        }
        if (order.side == Order::SELL && order.price < 0.01) {
            order.is_valid = false;
            return;
        }

        if (std::find(allowed_symbols_.begin(), allowed_symbols_.end(), 
                     order.symbol) == allowed_symbols_.end()) {
            order.is_valid = false;
            return;
        }
    }
    
public:
    ValidateStage(OrderQueue& input, OrderQueue& output, int num_threads = 8) : input_queue_(input), output_queue_(output) {
        
        for (int i = 0; i < num_threads; ++i) {
            threads_.emplace_back(&ValidateStage::worker, this);
        }
    }
    
    void join() {
        for (auto& t : threads_) {
            if (t.joinable()) t.join();
        }
        output_queue_.shutdown();
    }
    
    const StageMetrics& get_metrics() const { return metrics_; }
};


class ProcessStage {
private:
    OrderQueue& input_queue_;
    StageMetrics metrics_;
    std::vector<std::thread> threads_;
    
    std::mutex book_mutex_;
    std::map<double, int> buy_book_;
    std::map<double, int> sell_book_;
    
    std::atomic<size_t> valid_orders_{0};
    std::atomic<size_t> invalid_orders_{0};
    std::atomic<size_t> matches_{0};
    std::atomic<long long> total_volume_{0};
    
    void worker() {
        Order order;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> sleep_dist(5, 10);
        
        while (input_queue_.pop(order)) {
            auto start = std::chrono::steady_clock::now();
            
            if (order.is_valid) {
                process_order(order);
                valid_orders_++;
            } else {
                invalid_orders_++;
            }
            
            std::this_thread::sleep_for(std::chrono::microseconds(sleep_dist(gen)));
            
            auto end = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
            metrics_.record_processing(duration);
        }
    }
    
    void process_order(const Order& order) {
        std::lock_guard<std::mutex> lock(book_mutex_);
        
        if (order.side == Order::BUY) {
            buy_book_[order.price] += order.quantity;
        } else {
            sell_book_[order.price] += order.quantity;
        }
        
        while (!buy_book_.empty() && !sell_book_.empty()) {
            auto best_buy = buy_book_.rbegin();
            auto best_sell = sell_book_.begin();
            
            if (best_buy->first >= best_sell->first) {
                int match_qty = std::min(best_buy->second, best_sell->second);
                
                best_buy->second -= match_qty;
                best_sell->second -= match_qty;
                
                matches_++;
                total_volume_ += match_qty;
                
                if (best_buy->second == 0) {
                    buy_book_.erase(best_buy->first);
                }
                if (best_sell->second == 0) {
                    sell_book_.erase(best_sell);
                }
            } else {
                break;
            }
        }
    }
    
public:
    ProcessStage(OrderQueue& input, int num_threads = 4)
        : input_queue_(input) {
        
        for (int i = 0; i < num_threads; ++i) {
            threads_.emplace_back(&ProcessStage::worker, this);
        }
    }
    
    void join() {
        for (auto& t : threads_) {
            if (t.joinable()) t.join();
        }
    }
    
    const StageMetrics& get_metrics() const { return metrics_; }
    
    void print_stats() const {
        std::cout << "\n=== Process Stage Statistics ===\n";
        std::cout << "Valid orders processed: " << valid_orders_ << "\n";
        std::cout << "Invalid orders rejected: " << invalid_orders_ << "\n";
        std::cout << "Successful matches: " << matches_ << "\n";
        std::cout << "Total volume traded: " << total_volume_ << "\n";
    }
};


class MessageGenerator {
private:
    std::vector<std::string> symbols_ = {"AAPL", "MSFT", "GOOGL", "AMZN", "TSLA", "META", "NVDA", "AMD", "INTC", "IBM"};
    std::random_device rd_;
    std::mt19937 gen_;
    
public:
    MessageGenerator() : gen_(rd_()) {}
    
    std::string generate(bool valid = true) {
        std::uniform_int_distribution<> type_dist(0, 2);
        std::uniform_int_distribution<> side_dist(0, 1);
        std::uniform_int_distribution<> symbol_dist(0, symbols_.size() - 1);
        std::uniform_real_distribution<> price_dist(10.0, 400.0);
        std::uniform_int_distribution<> qty_dist(10, 1000);
        std::uniform_int_distribution<> id_dist(10000000, 99999999);
        
        std::stringstream ss;
        
        int type = type_dist(gen_);
        ss << (type == 0 ? "ORDER" : (type == 1 ? "CANCEL" : "MODIFY")) << "|";
        
        ss << (side_dist(gen_) == 0 ? "BUY" : "SELL") << "|";
        
        ss << symbols_[symbol_dist(gen_)] << "|";
        
        if (!valid && std::uniform_real_distribution<>(0, 1)(gen_) < 0.5) {
            ss << -price_dist(gen_) << "|"; 
        } else {
            ss << std::fixed << std::setprecision(2) << price_dist(gen_) << "|";
        }
        
        ss << qty_dist(gen_) << "|";
        
        ss << id_dist(gen_);
        
        return ss.str();
    }
};


int main() {
    const size_t NUM_MESSAGES = 1000000; 
    const size_t QUEUE_CAPACITY = 100;
    const double INVALID_RATE = 0.05; 
    
    StringQueue input_queue(QUEUE_CAPACITY);
    OrderQueue parse_to_validate(QUEUE_CAPACITY);
    OrderQueue validate_to_process(QUEUE_CAPACITY);
    
    auto pipeline_start = std::chrono::steady_clock::now();
    
    ParseStage parse_stage(input_queue, parse_to_validate, 10);
    ValidateStage validate_stage(parse_to_validate, validate_to_process, 8);
    ProcessStage process_stage(validate_to_process, 4);
    
    std::thread producer([&]() {
        MessageGenerator gen;
        std::random_device rd;
        std::mt19937 rng(rd());
        std::uniform_real_distribution<> dist(0.0, 1.0);
        
        for (size_t i = 0; i < NUM_MESSAGES; ++i) {
            bool valid = dist(rng) >= INVALID_RATE;
            input_queue.push(gen.generate(valid));
        }
        
        input_queue.shutdown();
    });
    
    producer.join();
    parse_stage.join();
    validate_stage.join();
    process_stage.join();
    
    auto pipeline_end = std::chrono::steady_clock::now();
    auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        pipeline_end - pipeline_start).count();
    
    std::cout << "\n=== Pipeline Performance ===\n";
    std::cout << "Total time: " << total_time << " ms\n";
    std::cout << "Throughput: " << (NUM_MESSAGES * 1000.0 / total_time) << " messages/sec\n\n";
    
    std::cout << "Parse Stage:\n";
    std::cout << "Processed: " << parse_stage.get_metrics().processed << "\n";
    std::cout << "Avg time: " << parse_stage.get_metrics().get_avg_processing_time_us()<< "μs\n\n";
    
    std::cout << "Validate Stage:\n";
    std::cout << "Processed: " << validate_stage.get_metrics().processed << "\n";
    std::cout << "Errors: " << validate_stage.get_metrics().errors << "\n";
    std::cout << "Avg time: " << validate_stage.get_metrics().get_avg_processing_time_us() << "μs\n\n";
    
    std::cout << "Process Stage:\n";
    std::cout << "Processed: " << process_stage.get_metrics().processed << "\n";
    std::cout << "Avg time: " << process_stage.get_metrics().get_avg_processing_time_us()<< "μs\n";
    
    process_stage.print_stats();
    
    std::cout << "\n=== Queue Statistics ===\n";
    std::cout << "Input queue: " << input_queue.total_enqueued()<< " enqueued, " << input_queue.total_dequeued() << " dequeued\n";
    std::cout << "Parse→Validate: " << parse_to_validate.total_enqueued()<< " enqueued, " << parse_to_validate.total_dequeued() << " dequeued\n";
    std::cout << "Validate→Process: " << validate_to_process.total_enqueued()<< " enqueued, " << validate_to_process.total_dequeued() << " dequeued\n";
    
    return 0;
}