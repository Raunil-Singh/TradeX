#include "oms.h"
#include <iostream>
#include <assert.h>
#include "symbol_loader.h"
#include "market_state.h"

int main() {
    std::cout << "--- HFT System Integration Test ---" << std::endl;

    matching_engine::MatchingEngineDispatcher engine(1024);
    
    uint64_t lower[MAX_SYMBOLS] = {0};
    uint64_t upper[MAX_SYMBOLS] = {0};
    std::vector<SymbolInfo> symbol_data = loadSymbolCSV("symbols.csv");
    for (const auto& s : symbol_data) {
        lower[s.symbol_id] = s.lower_limit;
        upper[s.symbol_id] = s.upper_limit;
    }
    
    engine.initialize_engine(lower, upper);

    oms::OrderManagementSystem oms_system(&engine, 1024);
    
    std::thread engine_thread([&engine]() { engine.start(); });
    oms_system.start();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    //limit test
    // oms::ClientOrder test_order1;
    // test_order1.client_order_id = 101;
    // test_order1.price = 15000; // 150.00
    // test_order1.quantity = 10;
    // test_order1.type = matching_engine::OrderType::BUY;
    // test_order1.execution_type = oms::ClientOrderType::LIMIT;
    // strcpy(test_order1.symbol, "AAPL");

    // oms::ClientOrder test_order2;
    // test_order2.client_order_id = 102;
    // test_order2.price = 15000; // 150.00
    // test_order2.quantity = 10;
    // test_order2.type = matching_engine::OrderType::SELL;
    // test_order2.execution_type = oms::ClientOrderType::LIMIT;
    // strcpy(test_order2.symbol, "AAPL");

    //market test
    // oms::ClientOrder test_order1;
    // test_order1.client_order_id = 101;
    // test_order1.price = 15000; // 150.00
    // test_order1.quantity = 10;
    // test_order1.type = matching_engine::OrderType::BUY;
    // test_order1.execution_type = oms::ClientOrderType::LIMIT;
    // strcpy(test_order1.symbol, "AAPL");

    // oms::ClientOrder test_order2;
    // test_order2.client_order_id = 102;
    // test_order2.quantity = 10;
    // test_order2.type = matching_engine::OrderType::SELL;
    // test_order2.execution_type = oms::ClientOrderType::MARKET;
    // strcpy(test_order2.symbol, "AAPL");

    //stop loss test
    oms::ClientOrder buy_resting{};
    buy_resting.client_order_id = 101;
    buy_resting.price = 10000;
    buy_resting.type = matching_engine::OrderType::BUY;
    buy_resting.quantity = 100;
    buy_resting.execution_type = oms::ClientOrderType::LIMIT;
    strcpy(buy_resting.symbol, "AAPL");

    oms::ClientOrder sell_resting{};
    sell_resting.client_order_id = 102;
    sell_resting.price = 10000;
    sell_resting.type = matching_engine::OrderType::SELL;
    sell_resting.quantity = 100;
    sell_resting.execution_type = oms::ClientOrderType::LIMIT;
    strcpy(sell_resting.symbol, "AAPL");

    oms::ClientOrder sell_resting2{};
    sell_resting2.client_order_id = 102;
    sell_resting2.price = 10000;
    sell_resting2.type = matching_engine::OrderType::SELL;
    sell_resting2.quantity = 100;
    sell_resting2.execution_type = oms::ClientOrderType::LIMIT;
    strcpy(sell_resting2.symbol, "AAPL");

    oms::ClientOrder stop_loss{};
    stop_loss.client_order_id = 103;
    stop_loss.price = 9400;
    stop_loss.trigger_price = 9500;
    stop_loss.type = matching_engine::OrderType::BUY;
    stop_loss.quantity = 50;
    stop_loss.execution_type = oms::ClientOrderType::STOP_LOSS;
    strcpy(stop_loss.symbol, "AAPL");

    oms::ClientOrder trigger_buy{};
    trigger_buy.client_order_id = 104;
    trigger_buy.price = 9500;
    trigger_buy.type = matching_engine::OrderType::BUY;
    trigger_buy.quantity = 100;
    trigger_buy.execution_type = oms::ClientOrderType::LIMIT;
    strcpy(trigger_buy.symbol, "AAPL");

    oms::ClientOrder trigger_sell{};
    trigger_sell.client_order_id = 105;
    trigger_sell.price = 9500;
    trigger_sell.type = matching_engine::OrderType::SELL;
    trigger_sell.quantity = 100;
    trigger_sell.execution_type = oms::ClientOrderType::LIMIT;
    strcpy(trigger_sell.symbol, "AAPL");

    oms::ClientOrder trigger_buy2{};
    trigger_buy2.client_order_id = 104;
    trigger_buy2.price = 9400;
    trigger_buy2.type = matching_engine::OrderType::BUY;
    trigger_buy2.quantity = 100;
    trigger_buy2.execution_type = oms::ClientOrderType::LIMIT;
    strcpy(trigger_buy2.symbol, "AAPL");

    //ice berg
    // oms::ClientOrder buy_resting{};
    // buy_resting.client_order_id = 101;
    // buy_resting.price = 10000;
    // buy_resting.type = matching_engine::OrderType::BUY;
    // buy_resting.quantity = 200;
    // buy_resting.execution_type = oms::ClientOrderType::LIMIT;
    // strcpy(buy_resting.symbol, "AAPL");

    // oms::ClientOrder sell_resting{};
    // sell_resting.client_order_id = 102;
    // sell_resting.is_active = true;
    // sell_resting.price = 10000;
    // sell_resting.type = matching_engine::OrderType::SELL;
    // sell_resting.quantity = 200;
    // sell_resting.display_quantity = 50;
    // sell_resting.execution_type = oms::ClientOrderType::ICEBERG;
    // strcpy(sell_resting.symbol, "AAPL");

    //std::cout << "Dispatching test order for AAPL..." << std::endl;
    while(!oms_system.enqueueClientOrder(buy_resting)) {}
    while(!oms_system.enqueueClientOrder(sell_resting)) {}
    while(!oms_system.enqueueClientOrder(sell_resting2)) {}
    while(!oms_system.enqueueClientOrder(stop_loss)) {}
    while(!oms_system.enqueueClientOrder(trigger_buy)) {}
    while(!oms_system.enqueueClientOrder(trigger_sell)) {}
    while(!oms_system.enqueueClientOrder(trigger_buy2)) {}
    

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::cout << "Test Complete. Check logs for processing." << std::endl;

    engine.terminate();
    oms_system.stop();
    if(engine_thread.joinable()) engine_thread.join();

    saveEndOfDayCSV("symbols.csv", oms_system.shared_memory_ptr, symbol_data);
    return 0;
}