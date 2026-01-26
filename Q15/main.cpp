#include <string_view>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <iostream>
#include <map>
#include <queue>
#include <condition_variable>
#include <array>
#include <chrono>
#include <string>
#include <fstream>
#include <charconv>
#include <regex>
#include <unordered_set>
#include <random>


/* structure: 2 parsers, 4 validations, 4 processors. 
Implement back pressure in validation stage and parser(don't we get natural back pressure thru sleep).
Have a blocking queue in between parse and validation.
Have a lockless mpmc between validation and processor(thinking of trying both a single mpmc lockless queue, as well as a shard based spmc queue lockless or a work stealing based spmc queue lockless)
*/
namespace constants{
    const std::string fileName = "file.txt";
    constexpr int boundedQueueSize{1<<18};
    constexpr int boundedQueueSizeMod{boundedQueueSize - 1};
    constexpr int lockFreeQueueSize{1<<10};
    constexpr int lockFreeQueueSizeMod{lockFreeQueueSize - 1};
    constexpr int orderCount{20'000'000};
    constexpr int parserThreads{2};
    constexpr int validatorThreads{4};
    constexpr int processorThreads{4};
    const std::regex symbolMatch("^[A-Z]{1,5}$");
    const std::unordered_set<std::string> allowedSymbols{"AAPL", "MSFT", "GOOGL", "AMZN", "TSLA", "META", "NVDA", "AMD", "INTC", "IBM"};
    const std::array<std::string, 10> allowedSymbolsNames{"AAPL", "MSFT", "GOOGL", "AMZN", "TSLA", "META", "NVDA", "AMD", "INTC", "IBM"};
}
namespace enumGenerators{
    const char* typeGenerator(int num){
        switch(num){
            case Order::Type::ORDER:
                return "ORDER";
            case Order::Type::MODIFY:
                return "MODIFY";
            case Order::Type::CANCEL:
                return "CANCEL";
        }
    }

    const char* sideGenerator(int num){
        switch(num){
            case Order::Side::BUY:
                return "BUY";
            case Order::Side::SELL:
                return "SELL";
        }
    }
}

struct Order {
    enum Type { ORDER, CANCEL, MODIFY };
    enum Side { BUY, SELL };
    Type type;
    Side side;
    std::string symbol;
    double price;
    int quantity;
    long order_id;
    bool is_valid;  // Set by validation stage
    
};

std::atomic_bool shutdown{};
std::atomic_int orderGenerationCounter{};
std::atomic_int completed{};

constexpr double INVALID_RATE = 0.05;

// Valid bounds
constexpr double PRICE_LO = 0.01;
constexpr double PRICE_HI = 500.0;

constexpr int QTY_LO = 1;
constexpr int QTY_HI = 10'000;

constexpr int ID_LO = 10'000'000;
constexpr int ID_HI = 99'999'999;

static const std::array<std::string, 10> valid_symbols = {
    "AAPL","MSFT","GOOGL","AMZN","TSLA",
    "META","NVDA","AMD","INTC","IBM"
};

static const std::array<std::string, 5> invalid_symbols = {
    "aapl", "APPLEE", "AAPL1", "GOOG$", "TESLA"
};
class Backoff {
public:
    Backoff() : failures_(0) {}

    // Call when a lock-free operation fails
    void on_failure() {
        ++failures_;

        if (failures_ < SPIN_LIMIT) {
            // Busy spin (very short)
            cpu_relax();
        }
        else if (failures_ < YIELD_LIMIT) {
            // Hint to CPU / pipeline
            cpu_relax();
            cpu_relax();
        }
        else if (failures_ < SLEEP_LIMIT) {
            // Yield to scheduler, stay runnable
            std::this_thread::yield();
        }
        else {
            // Rare, last resort: tiny sleep
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
    }

    // Call when an operation succeeds
    void on_success() {
        failures_ = 0;
    }

private:
    static constexpr uint32_t SPIN_LIMIT  = 8;
    static constexpr uint32_t YIELD_LIMIT = 32;
    static constexpr uint32_t SLEEP_LIMIT = 64;

    uint32_t failures_;

    static inline void cpu_relax() {
#if defined(__x86_64__) || defined(__i386__)
        __builtin_ia32_pause();
#elif defined(__aarch64__) || defined(__arm__)
        __asm__ volatile("yield");
#else
        // Fallback: empty loop
#endif
    }
};

class Generator{
    private: 
        std::mt19937 rGen;
        std::uniform_int_distribution<> typeDistr_;
        std::uniform_int_distribution<> sideDistr_;
        std::uniform_int_distribution<> quantityDistr_;
        std::uniform_real_distribution<> priceDistr_;
        std::uniform_int_distribution<> idDistr_;
        std::uniform_int_distribution<> symbolDistr_;
        std::uniform_real_distribution<> prob;
        boundedQueue<std::string> queueRef_;

    public:
        void generatePush();
        std::string orderGeneration();
};

//This is done, but needs to be tweaked for better readability and shit, remove ai, make mine
class OrderGenerator {
    std::mt19937_64 rng;
    std::uniform_real_distribution<double> prob{0.0, 1.0};
    boundedQueue<std::string>& queueRef_;
    

public:
    inline static std::atomic_int counter{1};
    OrderGenerator(boundedQueue<std::string>& queueRef) : rng(std::random_device{}()), queueRef_(queueRef) {
        std::thread thread{generateAndPush};
        thread.join();
    }
    std::string generate();
    void generateAndPush();
};

void OrderGenerator::generateAndPush(){
    while(!shutdown.load(std::memory_order_relaxed)){
        queueRef_.push(std::move(generate));
        orderGenerationCounter.fetch_add(1, std::memory_order_relaxed);
    }
    counter.fetch_sub(1, std::memory_order_relaxed);
}
std::string OrderGenerator::generate() {
        bool invalid_mode = (prob(rng) < INVALID_RATE);

        // ---- PRICE ----
        std::uniform_real_distribution<double> price_dist(
            invalid_mode ? PRICE_LO - 300.0 : PRICE_LO,
            invalid_mode ? PRICE_HI + 300.0 : PRICE_HI
        );

        // ---- QUANTITY ----
        std::uniform_int_distribution<int> qty_dist(
            invalid_mode ? QTY_LO - 5000 : QTY_LO,
            invalid_mode ? QTY_HI + 5000 : QTY_HI
        );

        // ---- ORDER ID ----
        std::uniform_int_distribution<int> id_dist(
            invalid_mode ? ID_LO - 5'000'000 : ID_LO,
            invalid_mode ? ID_HI + 5'000'000 : ID_HI
        );

        // ---- SYMBOL ----
        std::uniform_int_distribution<int> sym_dist(
            0,
            invalid_mode
                ? valid_symbols.size() + invalid_symbols.size() - 1
                : valid_symbols.size() - 1
        );

        const char* type = "ORDER";
        const char* side = (rng() & 1) ? "BUY" : "SELL";

        int sym_idx = sym_dist(rng);
        const std::string& symbol =
            (sym_idx < (int)valid_symbols.size())
                ? valid_symbols[sym_idx]
                : invalid_symbols[sym_idx - valid_symbols.size()];

        double price = price_dist(rng);
        int quantity = qty_dist(rng);
        int order_id = id_dist(rng);

        std::ostringstream oss;
        oss << type << '|'
            << side << '|'
            << symbol << '|'
            << price << '|'
            << quantity << '|'
            << order_id;

        return oss.str();
    }


//Queue between parser and validator, locked by mutex. 
template <typename T>
class boundedQueue{
    private:
        typedef size_t counter;
        typedef size_t indexPos;
        counter pushCounter{};
        counter popCounter{};
        std::mutex qMutex{};
        indexPos head{};
        indexPos tail{};
        std::array<T, constants::boundedQueueSize> queue;
        std::condition_variable queueFull_;
        std::condition_variable queueEmpty_;
        bool isEmpty();
        bool isFull();
    
    public:
        size_t size(){
            return pushCounter - popCounter;
        }
        bool push(T&&);  //called only if mutex has been acquired.
        bool pop(T&); //called only if mutex has been acquired.
};

template <typename T>
bool inline boundedQueue<T>::isEmpty(){
    return (this->head == this->tail);
}

template <typename T>
bool inline boundedQueue<T>::isFull(){
    return (((this->tail + 1) & constants::boundedQueueSizeMod) == this->head);
}

template <typename T>
bool boundedQueue<T>::push(T&& order){
    {
        std::unique_lock<std::mutex> lock(this->qMutex);
        queueFull_.wait(lock, [this]{
            return !this->isFull();
        });

        queue[tail] = order;
        queueEmpty_.notify_one();
        tail = (tail + 1) & constants::boundedQueueSizeMod;
    }
    return true;
}

template <typename T>
bool boundedQueue<T>::pop(T& order){
    {
        std::unique_lock<std::mutex> lock(this->qMutex);
        queueEmpty_.wait(lock, [this]{
            return !this->isEmpty();
        });

        order = std::move(queue[head]);
        head = (head + 1) & constants::boundedQueueSizeMod;
        queueFull_.notify_one();
    }
    return true;
}

template <typename T>
class lockFreeQueue{
    private:
        typedef uint64_t indexPositionReturn;
        typedef uint64_t sequenceNumberReturn;
        typedef std::atomic_uint64_t indexPosition;
        typedef std::atomic_uint64_t sequenceNumber;
        typedef std::atomic_size_t counter;
        typedef size_t counterReturn;
        indexPosition head{};
        indexPosition tail{};
        alignas(64) counter pushCounter;
        alignas(64) counter popCounter;
        alignas(64) counter orderCounter;
        std::array<std::pair<T, sequenceNumber>, constants::lockFreeQueueSize> queue;

    public:
        counterReturn fetchPushCounter();
        counterReturn fetchPopCounter();
        bool pop(T& out);
        bool push(T&&);
        lockFreeQueue(){
            for(int i = 0; i < constants::lockFreeQueueSize; i++){
                queue[i].second.store(i, std::memory_order_relaxed);
            }
        }
};

template <typename T>
bool lockFreeQueue<T>::push(T&& data){
    lockFreeQueue::indexPositionReturn local_head = this->head.load(std::memory_order_relaxed);
    lockFreeQueue::indexPositionReturn local_tail = this->tail.load(std::memory_order_relaxed);
    if(!(local_tail + 1 == local_head)){
        //we know by our info queue's not full
        lockFreeQueue::indexPositionReturn indexPos = local_tail & constants::lockFreeQueueSizeMod;
        lockFreeQueue::sequenceNumberReturn sequenceNumberLocal = this->queue[tail].second.load(std::memory_order_acquire);
        if(sequenceNumberLocal == local_tail){
            //acquired correct number
            if(this->tail.compare_exchange_strong(local_tail, local_tail + 1, std::memory_order_relaxed)){
                //cas worked, now we change state
                this->queue[local_tail].first = data;
                orderCounter.fetch_add(1, std::memory_order_relaxed);
                pushCounter.fetch_add(1, std::memory_order_relaxed);
                this->queue[local_tail].second.store(sequenceNumberLocal + 1, std::memory_order_release);
                return true;
            }
            else{
                return false;
            }
        }
        else{
            return false;
        }
    }else{
        return false;
    }
}

template <typename T>
bool lockFreeQueue<T>::pop(T& out){
    lockFreeQueue::indexPositionReturn local_head = this->head.load(std::memory_order_relaxed);
    lockFreeQueue::indexPositionReturn local_tail = this->tail.load(std::memory_order_relaxed);
    if(!(local_tail == local_head)){
        //we know, locally atleast, tail != head
        lockFreeQueue::indexPositionReturn indexPos = local_head & constants::lockFreeQueueSizeMod;
        lockFreeQueue::sequenceNumberReturn localSequenceNumber = this->queue[local_head].second.load(std::memory_order_acquire);
        if(local_head + 1 == localSequenceNumber){
            //we're holding the correct ticket
            if(this->head.compare_exchange_strong(local_head, local_head +1 , std::memory_order_relaxed)){
                //we've achieved successful CAS
                out = std::move(this->queue[local_head].first);
                this->queue[local_head].second.store(local_head + constants::lockFreeQueueSize, std::memory_order_release);
                popCounter.fetch_add(1, std::memory_order_relaxed);
                return true;
            }
            
        }
    }
    return false;
}

template <typename T>
lockFreeQueue<T>::counterReturn lockFreeQueue<T>::fetchPopCounter(){
    return this->popCounter.load(std::memory_order_relaxed);
}

template <typename T>
lockFreeQueue<T>::counterReturn lockFreeQueue<T>::fetchPushCounter(){
    return this->pushCounter.load(std::memory_order_relaxed);
}

class Parser{
    private:
        enum parse_result {SUCCESS, FAILURE}; //static by nature along with typedefs? ans: probably yes
        std::ifstream file_;
        std::vector<std::thread> pool_;
        const int SEGMENT_SIZE{constants::orderCount / constants::parserThreads};
        boundedQueue<std::string>& queueRefString_;
        lockFreeQueue<Order>& queueRefOrder_;
        Backoff backoff_;
        StageMetrics stm_;
    public:
        inline static std::atomic_int count = constants::parserThreads;
        Parser() = delete;
        Parser(boundedQueue<std::string>& queueRefString, lockFreeQueue<Order>& queueRefOrder):
        queueRefString_(queueRefString),
        queueRefOrder_(queueRefOrder)
       {
            pool_.reserve(constants::parserThreads);
            for(int i = 0; i < constants::parserThreads; i++){
                pool_.emplace_back(worker);
            }
       }
       void worker(); // active logic
       bool parseAndReturn(std::string_view, Order&);
       void rejoin(); // rejoins the threads based on some condition
};

void Parser::worker(){
    std::string fileLine;
    while(!shutdown.load(std::memory_order_relaxed)||queueRefString_.size()!=0||OrderGenerator::counter != 0){
        if(this->queueRefString_.pop(fileLine)){
            
            Order order_;
            if(parseAndReturn(fileLine, order_)){
                while(!queueRefOrder_.push(std::move(order_))){
                    this->backoff_.on_failure();
                }
                this->backoff_.on_success();
            }else{
                //need to log message at stage metrics
            }
        }
        //shutdown still required
    }
    Parser::count.fetch_sub(1, std::memory_order_relaxed);
}

void Parser::rejoin(){//Should some of these methods be static
    for(auto& thread: this->pool_){
        thread.join();
    }
}

bool Parser::parseAndReturn(std::string_view s, Order& order){
    int pos{};
    size_t field_start[6];
    size_t field_len[6];
    int field_start_ = 0;
    int field_idx{};
    for(int i = 0; i <= s.size(); i++){
        if(s[i] == '|' || i == s.size()){
            if(field_idx >= 6){
                return Parser::parse_result::FAILURE;
            }
            field_start[field_idx] = field_start_;
            field_len[field_idx] = i - field_start_;
            field_start_ = i + 1;
            field_idx++;
        }
    }
    if(field_idx != 6){
        return Parser::parse_result::FAILURE;
    }
    switch (field_len[0]){
        case 5:
            order.type = Order::Type::ORDER;
            break;
        case 6:
            if(s.substr(field_start[0], field_start[1]).compare("MODIFY")){
                order.type = Order::Type::MODIFY;
            }else{
                order.type = Order::Type::CANCEL;
            }
    }
    switch(field_len[1]){
        case 3:
            order.side = Order::Side::BUY;
            break;
        case 4:
            order.side = Order::Side::SELL;
    }
    order.symbol = std::string(s.substr(field_start[2], field_start[2] + field_len[2])); //is std::move encompassing this redundant because the object created by the constructor is already an rvalue? Does std::string operator = have a move overload, Ans: YES
    std::from_chars(s.data() + field_start[3], s.data() + field_start[3] + field_len[3], order.price);
    std::from_chars(s.data() + field_start[4], s.data() + field_start[4] + field_len[4], order.quantity);
    return Parser::parse_result::SUCCESS;
}

class Validator{
    private:
        lockFreeQueue<Order>& queueRefParsed_;
        lockFreeQueue<Order>& queueRefValidated_;
        std::vector<std::thread> pool_;
        Backoff backoff_;
    
    public:
        inline static std::atomic_int count = constants::validatorThreads;
        Validator(lockFreeQueue<Order>& queueRefParsed, lockFreeQueue<Order>& queueRefValidated):
        queueRefParsed_(queueRefParsed), 
        queueRefValidated_(queueRefValidated)
        {
            pool_.reserve(constants::validatorThreads);
            for(int i = 0; i < constants::validatorThreads; i++){
                pool_.emplace_back(worker);
            }
        }
        void worker();
        void rejoin();
        
};
void Validator::worker(){
//still havent implemented shutdown; Need to get rid of magic numbers
    while(!(shutdown.load(std::memory_order_relaxed)||Parser::count != 0||(queueRefParsed_.fetchPushCounter() - queueRefParsed_.fetchPopCounter() != 0))){
        Order order;
        if(this->queueRefParsed_.pop(order)){
            this->backoff_.on_success();
            auto price = order.price;
            auto quantity = order.quantity;
            auto id = order.order_id;
            if(price < 0.0 || price > 1'000'000){
                order.is_valid = false;
            }
            else if(quantity < 0 || quantity > 10'000){
                order.is_valid = false;
            }
            else if(price * quantity > 1'000'000){
                order.is_valid = false;
            }
            else if(!(((order.side == Order::Side::BUY && price <= 500.0)||order.side == Order::Side::SELL && price >= 0.01))){
                order.is_valid = false;
            }
            else if(constants::allowedSymbols.find(order.symbol) == constants::allowedSymbols.end()){
                order.is_valid = false;
            }
            else{
                order.is_valid = true;
            }
            while(!this->queueRefValidated_.push(std::move(order))){
                this->backoff_.on_failure();
            }
            this->backoff_.on_success();
        }
    }
    Validator::count.fetch_sub(1, std::memory_order_relaxed);
}

void Validator::rejoin(){
    for(auto& thread: pool_){
        thread.join();
    }
}

class Processor{
    private:
        std::vector<std::thread> pool_;
        lockFreeQueue<Order>& queueref_;
        std::map<double, int> buyBook;
        std::map<double, int> sellBook;
        std::mutex bookMutex;
        Backoff bckoff_;
        static std::atomic_int validOrderProcessed;
        static std::atomic_int invalidOrderProcessed;
        static std::atomic_int match;
    public:
        inline static std::atomic_int count = constants::processorThreads;
        Processor(lockFreeQueue<Order>& queueref):
        queueref_(queueref){
            for(int i = 0; i < 4; i++){
                pool_.emplace_back(pushAndProcess);
            }
        }
        void pushAndProcess();
        void rejoin();
};

void Processor::rejoin(){
    for(auto& thread: pool_){
        thread.join();
    }
}
void Processor::pushAndProcess(){
    Order order;
    while(!shutdown.load(std::memory_order_relaxed)||(Validator::count==0)||(queueref_.fetchPushCounter() - queueref_.fetchPopCounter() == 0)){
        if(queueref_.pop(order)){
            bckoff_.on_success();
            {
                std::unique_lock<std::mutex> lock(this->bookMutex);
                if(order.is_valid){
                    validOrderProcessed.fetch_add(1, std::memory_order_relaxed);
                    if(order.side == Order::Side::BUY){
                        buyBook[order.price] += order.quantity;
                    }
                    else{
                        sellBook[order.price] += order.quantity;
                    }
                }else{
                    invalidOrderProcessed.fetch_add(1, std::memory_order_relaxed);
                }
                auto bestBuyPrice = buyBook.rbegin();
                auto bestSellPrice = sellBook.begin();
                if((*bestBuyPrice).first >= (*bestSellPrice).first){
                    match.fetch_add(1, std::memory_order_relaxed);
                    buyBook.erase(std::prev(bestBuyPrice.base()));
                    sellBook.erase(bestSellPrice);
                }
            }
        }
        else{
            bckoff_.on_failure();
        }
    }
}

class StageMetrics{
    public:
        void logError();
};



int main(){
    boundedQueue<std::string> queue1_b;
    lockFreeQueue<Order> queue1_lf;
    lockFreeQueue<Order> queue2_lf;
    OrderGenerator og = OrderGenerator(queue1_b);
    while(orderGenerationCounter.load(std::memory_order_relaxed) < 100,000){
        
    }
    auto start = std::chrono::steady_clock::now();
    Parser p = Parser(queue1_b, queue1_lf);
    Validator v = Validator(queue1_lf, queue2_lf);
    Processor pp = Processor(queue2_lf);
    std::this_thread::sleep_for(std::chrono::seconds(30));
    shutdown.store(true, std::memory_order_relaxed);
    p.rejoin();
    v.rejoin();
    pp.rejoin();
    auto end = std::chrono::steady_clock::now();
    std::cout<<"Thoroughput: "<<orderGenerationCounter<<" in 30s, rate: "<<(static_cast<double>(orderGenerationCounter))/std::chrono::duration_cast<std::chrono::seconds>(end - start).count()<<" orders/sec. \n";
}