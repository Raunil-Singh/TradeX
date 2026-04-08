#include "oms.h"
#include <iostream>
#include <assert.h>
#include "symbol_loader.h"
#include "market_state.h"
#include <iomanip>
#include <chrono>

#define NUM_ORDERS 900000

int main() {
    std::cout << "--- HFT System Integration Test ---" << std::endl;

    matching_engine::MatchingEngineDispatcher engine(1024);
    
    uint64_t lower[TOTAL_SYMBOLS] = {0};
    uint64_t upper[TOTAL_SYMBOLS] = {0};
    std::vector<SymbolInfo> symbol_data = loadSymbolCSV("data/symbols.csv");
    if (symbol_data.empty()) {
        std::cerr << "ERROR: CSV not found at data/symbols.csv! Current path: ";
        return 1;
    }
    for (const auto& s : symbol_data) {
        lower[s.symbol_id] = s.lower_limit;
        upper[s.symbol_id] = s.upper_limit;
    }
    
    engine.initialize_engine(lower, upper);

    
    oms::OrderManagementSystem oms_system(&engine, 16384);
    
    std::thread engine_thread([&engine]() { engine.start(); });
    oms_system.start();

    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    
    std::cout << "Starting Benchmark: " << NUM_ORDERS << " orders (AAPL)" << std::endl;

    std::vector<oms::ClientOrder> orders(NUM_ORDERS, oms::ClientOrder{});
    for (int i = 0; i < NUM_ORDERS; ++i) {
        strcpy(orders[i].symbol, "AAPL");
        orders[i].quantity = (rand() % 100) + 1;
        orders[i].price = 10000 + (rand() % 100);
        orders[i].type = (i % 2 == 0) ? matching_engine::OrderType::BUY : matching_engine::OrderType::SELL;
        orders[i].execution_type = oms::ClientOrderType::LIMIT;
    }

    auto start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < NUM_ORDERS; ++i) {
        while (!oms_system.enqueueClientOrder(orders[i])) {
            std::this_thread::yield();
        }
    }

    auto end_enqueue = std::chrono::high_resolution_clock::now();
    
    auto end_processing = std::chrono::high_resolution_clock::now();

    // Metrics Calculation
    double enqueue_duration = std::chrono::duration<double>(end_enqueue - start_time).count();
    double total_duration = std::chrono::duration<double>(end_processing - start_time).count();


    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        break; 
    }

    std::cout << std::fixed << std::setprecision(4);
    std::cout << "\nBenchmark Results for 9M Orders:" << std::endl;
    std::cout << "---------------------------------" << std::endl;
    std::cout << "Enqueue Time:      " << enqueue_duration << " s" << std::endl;
    std::cout << "Enqueue Throughput: " << (NUM_ORDERS / enqueue_duration) / 1e6 << " M orders/sec" << std::endl;
    std::cout << "Total Time:        " << total_duration << " s" << std::endl;
    std::cout << "Overall Throughput: " << (NUM_ORDERS / total_duration) / 1e6 << " M orders/sec" << std::endl;

    engine.terminate();
    oms_system.stop();
    if(engine_thread.joinable()) engine_thread.join();

    saveEndOfDayCSV("data/symbols.csv", oms_system.shared_memory_ptr, symbol_data);
    return 0;
}
