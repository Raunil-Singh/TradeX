#include "oms.h"
#include <iostream>
#include <assert.h>
#include "symbol_loader.h"
#include "market_state.h"

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

    oms::OrderManagementSystem oms_system(&engine, 1024);
    
    std::thread engine_thread([&engine]() { engine.start(); });
    oms_system.start();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // ---------- GROUP 0: AAPL (Symbol ID 1) ----------
    //LIMIT TEST
    oms::ClientOrder aapl_sell{};
    aapl_sell.price = 10000;
    aapl_sell.quantity = 100;
    aapl_sell.type = matching_engine::OrderType::SELL;
    aapl_sell.execution_type = oms::ClientOrderType::LIMIT;
    strcpy(aapl_sell.symbol, "AAPL"); // Mapping must ensure ID 1

    oms::ClientOrder aapl_buy{};
    aapl_buy.price = 10000;
    aapl_buy.quantity = 50;
    aapl_buy.type = matching_engine::OrderType::BUY;
    aapl_buy.execution_type = oms::ClientOrderType::LIMIT;
    strcpy(aapl_buy.symbol, "AAPL");

    oms::ClientOrder aapl_buy2{};
    aapl_buy2.price = 10000;
    aapl_buy2.quantity = 50;
    aapl_buy2.type = matching_engine::OrderType::BUY;
    aapl_buy2.execution_type = oms::ClientOrderType::LIMIT;
    strcpy(aapl_buy2.symbol, "AAPL");

    //STOP LOSS TEST
    oms::ClientOrder aapl_sell_l{};
    aapl_sell_l.price = 10000;
    aapl_sell_l.quantity = 100;
    aapl_sell_l.type = matching_engine::OrderType::SELL;
    aapl_sell_l.execution_type = oms::ClientOrderType::LIMIT;
    strcpy(aapl_sell_l.symbol, "AAPL"); // Mapping must ensure ID 1

    oms::ClientOrder aapl_sl_b{};
    aapl_sl_b.price = 9400;
    aapl_sl_b.trigger_price = 9500;
    aapl_sl_b.type = matching_engine::OrderType::BUY;
    aapl_sl_b.execution_type = oms::ClientOrderType::STOP_LOSS;
    aapl_sl_b.quantity = 100;
    strcpy(aapl_sl_b.symbol,"AAPL");

    oms::ClientOrder aapl_sell_l2{};
    aapl_sell_l2.price = 9500;
    aapl_sell_l2.quantity = 100;
    aapl_sell_l2.type = matching_engine::OrderType::SELL;
    aapl_sell_l2.execution_type = oms::ClientOrderType::LIMIT;
    strcpy(aapl_sell_l2.symbol, "AAPL"); // Mapping must ensure ID 1

    oms::ClientOrder aapl_buy_b2{};
    aapl_buy_b2.price = 9500;
    aapl_buy_b2.quantity = 100;
    aapl_buy_b2.type = matching_engine::OrderType::BUY;
    aapl_buy_b2.execution_type = oms::ClientOrderType::LIMIT;
    strcpy(aapl_buy_b2.symbol, "AAPL");
    
    oms::ClientOrder aapl_buy_b3{};
    aapl_buy_b3.price = 9400;
    aapl_buy_b3.quantity = 100;
    aapl_buy_b3.type = matching_engine::OrderType::BUY;
    aapl_buy_b3.execution_type = oms::ClientOrderType::LIMIT;
    strcpy(aapl_buy_b3.symbol, "AAPL");

    //ICEBERG TEST
    oms::ClientOrder aapl_ib_b{};
    aapl_ib_b.price = 10000;
    aapl_ib_b.type = matching_engine::OrderType::BUY;
    aapl_ib_b.execution_type = oms::ClientOrderType::LIMIT;
    aapl_ib_b.quantity = 100;
    strcpy(aapl_ib_b.symbol,"AAPL");

    oms::ClientOrder aapl_ib{};
    aapl_ib.price = 10000;
    aapl_ib.quantity = 100;
    aapl_ib.type = matching_engine::OrderType::SELL;
    aapl_ib.execution_type = oms::ClientOrderType::ICEBERG;
    aapl_ib.display_quantity = 20;
    aapl_ib.is_active = true;
    strcpy(aapl_ib.symbol, "AAPL");
    

    //---------- GROUP 1: TSLA (Symbol ID 1025) ----------
    //LIMIT TEST
    oms::ClientOrder tsla_sell{};
    tsla_sell.price = 20000;
    tsla_sell.quantity = 100;
    tsla_sell.type = matching_engine::OrderType::SELL;
    tsla_sell.execution_type = oms::ClientOrderType::LIMIT;
    strcpy(tsla_sell.symbol, "TSLA"); // Mapping must ensure ID 1025

    oms::ClientOrder tsla_buy{};
    tsla_buy.price = 20000;
    tsla_buy.quantity = 50;
    tsla_buy.type = matching_engine::OrderType::BUY;
    tsla_buy.execution_type = oms::ClientOrderType::LIMIT;
    strcpy(tsla_buy.symbol, "TSLA");

    oms::ClientOrder tsla_buy2{};
    tsla_buy2.price = 20000;
    tsla_buy2.quantity = 50;
    tsla_buy2.type = matching_engine::OrderType::BUY;
    tsla_buy2.execution_type = oms::ClientOrderType::LIMIT;
    strcpy(tsla_buy2.symbol, "TSLA");

    //STOP LOSS TEST
    oms::ClientOrder tsla_sell_l{};
    tsla_sell_l.price = 10000;
    tsla_sell_l.quantity = 100;
    tsla_sell_l.type = matching_engine::OrderType::SELL;
    tsla_sell_l.execution_type = oms::ClientOrderType::LIMIT;
    strcpy(tsla_sell_l.symbol, "TSLA"); // Mapping must ensure ID 1

    oms::ClientOrder tsla_sl_b{};
    tsla_sl_b.price = 9400;
    tsla_sl_b.trigger_price = 9500;
    tsla_sl_b.type = matching_engine::OrderType::BUY;
    tsla_sl_b.execution_type = oms::ClientOrderType::STOP_LOSS;
    tsla_sl_b.quantity = 100;
    strcpy(tsla_sl_b.symbol,"TSLA");

    oms::ClientOrder tsla_sell_l2{};
    tsla_sell_l2.price = 9500;
    tsla_sell_l2.quantity = 100;
    tsla_sell_l2.type = matching_engine::OrderType::SELL;
    tsla_sell_l2.execution_type = oms::ClientOrderType::LIMIT;
    strcpy(tsla_sell_l2.symbol, "TSLA"); // Mapping must ensure ID 1

    oms::ClientOrder tsla_buy_b2{};
    tsla_buy_b2.price = 9500;
    tsla_buy_b2.quantity = 100;
    tsla_buy_b2.type = matching_engine::OrderType::BUY;
    tsla_buy_b2.execution_type = oms::ClientOrderType::LIMIT;
    strcpy(tsla_buy_b2.symbol, "TSLA");
    
    oms::ClientOrder tsla_buy_b3{};
    tsla_buy_b3.price = 9400;
    tsla_buy_b3.quantity = 100;
    tsla_buy_b3.type = matching_engine::OrderType::BUY;
    tsla_buy_b3.execution_type = oms::ClientOrderType::LIMIT;
    strcpy(tsla_buy_b3.symbol, "TSLA");

    //ICEBERG TEST
    oms::ClientOrder tsla_ib_b{};
    tsla_ib_b.price = 10000;
    tsla_ib_b.type = matching_engine::OrderType::BUY;
    tsla_ib_b.execution_type = oms::ClientOrderType::LIMIT;
    tsla_ib_b.quantity = 100;
    strcpy(tsla_ib_b.symbol,"TSLA");

    oms::ClientOrder tsla_ib{};
    tsla_ib.price = 10000;
    tsla_ib.quantity = 100;
    tsla_ib.type = matching_engine::OrderType::SELL;
    tsla_ib.execution_type = oms::ClientOrderType::ICEBERG;
    tsla_ib.display_quantity = 20;
    tsla_ib.is_active = true;
    strcpy(tsla_ib.symbol, "TSLA");
    

    //std::cout << "Dispatching orders to Group 0 (AAPL) and Group 1 (TSLA)..." << std::endl;

      // Queue orders interleaved to see threads working
       while(!oms_system.enqueueClientOrder(aapl_sell)) {} // Group 0
       while(!oms_system.enqueueClientOrder(tsla_sell)) {} // Group 1
    
       std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    while(!oms_system.enqueueClientOrder(aapl_buy)) {}  // Group 0 trade
   while(!oms_system.enqueueClientOrder(tsla_buy)) {}  // Group 1 trade
    while(!oms_system.enqueueClientOrder(aapl_buy2)) {}  // Group 0 trade
   while(!oms_system.enqueueClientOrder(tsla_buy2)) {}  // Group 1 trade

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    while(!oms_system.enqueueClientOrder(aapl_sell_l)) {}
    while(!oms_system.enqueueClientOrder(aapl_sl_b)) {}
    while(!oms_system.enqueueClientOrder(aapl_sell_l2)) {}
    while(!oms_system.enqueueClientOrder(aapl_buy_b2)) {}
    while(!oms_system.enqueueClientOrder(aapl_buy_b3)) {}

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    while(!oms_system.enqueueClientOrder(tsla_sell_l)) {}
    while(!oms_system.enqueueClientOrder(tsla_sl_b)) {}
    while(!oms_system.enqueueClientOrder(tsla_sell_l2)) {}
    while(!oms_system.enqueueClientOrder(tsla_buy_b2)) {}
    while(!oms_system.enqueueClientOrder(tsla_buy_b3)) {}

    while(!oms_system.enqueueClientOrder(aapl_ib_b)){};
    while(!oms_system.enqueueClientOrder(tsla_ib_b)){};
    while(!oms_system.enqueueClientOrder(aapl_ib)){};
    while(!oms_system.enqueueClientOrder(tsla_ib)){};

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::cout << "Test Complete. Check logs for processing." << std::endl;

    engine.terminate();
    oms_system.stop();
    if(engine_thread.joinable()) engine_thread.join();

    saveEndOfDayCSV("data/symbols.csv", oms_system.shared_memory_ptr, symbol_data);
    return 0;
}
