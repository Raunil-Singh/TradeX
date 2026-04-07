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

   // ---------- Basic resting liquidity ----------
oms::ClientOrder sell_book1{};
sell_book1.price = 10000;
sell_book1.quantity = 100;
sell_book1.type = matching_engine::OrderType::SELL;
sell_book1.execution_type = oms::ClientOrderType::LIMIT;
strcpy(sell_book1.symbol, "AAPL");

oms::ClientOrder sell_book2{};
sell_book2.price = 10100;
sell_book2.quantity = 200;
sell_book2.type = matching_engine::OrderType::SELL;
sell_book2.execution_type = oms::ClientOrderType::LIMIT;
strcpy(sell_book2.symbol, "AAPL");

oms::ClientOrder buy_book1{};
buy_book1.price = 9800;
buy_book1.quantity = 150;
buy_book1.type = matching_engine::OrderType::BUY;
buy_book1.execution_type = oms::ClientOrderType::LIMIT;
strcpy(buy_book1.symbol, "AAPL");

// ---------- Exact limit match ----------
oms::ClientOrder limit_buy_match{};
limit_buy_match.price = 10000;
limit_buy_match.quantity = 50;
limit_buy_match.type = matching_engine::OrderType::BUY;
limit_buy_match.execution_type = oms::ClientOrderType::LIMIT;
strcpy(limit_buy_match.symbol, "AAPL");

// ---------- Partial fill ----------
oms::ClientOrder partial_buy{};
partial_buy.price = 10100;
partial_buy.quantity = 250; // fills 100 + leaves 150
partial_buy.type = matching_engine::OrderType::BUY;
partial_buy.execution_type = oms::ClientOrderType::LIMIT;
strcpy(partial_buy.symbol, "AAPL");

// ---------- Market order tests ----------
oms::ClientOrder market_buy{};
market_buy.quantity = 75;
market_buy.type = matching_engine::OrderType::BUY;
market_buy.execution_type = oms::ClientOrderType::MARKET;
strcpy(market_buy.symbol, "AAPL");

oms::ClientOrder market_sell{};
market_sell.quantity = 60;
market_sell.type = matching_engine::OrderType::SELL;
market_sell.execution_type = oms::ClientOrderType::MARKET;
strcpy(market_sell.symbol, "AAPL");

// ---------- Far-away unmatched order ----------
oms::ClientOrder unmatched_buy{};
unmatched_buy.price = 5000;
unmatched_buy.quantity = 100;
unmatched_buy.type = matching_engine::OrderType::BUY;
unmatched_buy.execution_type = oms::ClientOrderType::LIMIT;
strcpy(unmatched_buy.symbol, "AAPL");

// ---------- Stop-loss SELL trigger ----------
// Buy immediately at market, then place SELL if LTP <= 9500
oms::ClientOrder stop_loss_sell{};
stop_loss_sell.price = 9400;
stop_loss_sell.trigger_price = 9500;
stop_loss_sell.quantity = 40;
stop_loss_sell.type = matching_engine::OrderType::BUY;
stop_loss_sell.execution_type = oms::ClientOrderType::STOP_LOSS;
strcpy(stop_loss_sell.symbol, "AAPL");

// ---------- Stop-loss BUY trigger ----------
// Sell immediately at market, then place BUY if LTP >= 10300
oms::ClientOrder stop_loss_buy{};
stop_loss_buy.price = 10400;
stop_loss_buy.trigger_price = 10300;
stop_loss_buy.quantity = 50;
stop_loss_buy.type = matching_engine::OrderType::SELL;
stop_loss_buy.execution_type = oms::ClientOrderType::STOP_LOSS;
strcpy(stop_loss_buy.symbol, "AAPL");

// ---------- Trigger LTP downward to exactly 9500 ----------
oms::ClientOrder ltp_down_buy{};
ltp_down_buy.price = 9500;
ltp_down_buy.quantity = 100;
ltp_down_buy.type = matching_engine::OrderType::BUY;
ltp_down_buy.execution_type = oms::ClientOrderType::LIMIT;
strcpy(ltp_down_buy.symbol, "AAPL");

oms::ClientOrder ltp_down_sell{};
ltp_down_sell.price = 9500;
ltp_down_sell.quantity = 100;
ltp_down_sell.type = matching_engine::OrderType::SELL;
ltp_down_sell.execution_type = oms::ClientOrderType::LIMIT;
strcpy(ltp_down_sell.symbol, "AAPL");

// ---------- Trigger LTP upward to exactly 10300 ----------
oms::ClientOrder ltp_up_buy{};
ltp_up_buy.price = 10300;
ltp_up_buy.quantity = 100;
ltp_up_buy.type = matching_engine::OrderType::BUY;
ltp_up_buy.execution_type = oms::ClientOrderType::LIMIT;
strcpy(ltp_up_buy.symbol, "AAPL");

oms::ClientOrder ltp_up_sell{};
ltp_up_sell.price = 10300;
ltp_up_sell.quantity = 100;
ltp_up_sell.type = matching_engine::OrderType::SELL;
ltp_up_sell.execution_type = oms::ClientOrderType::LIMIT;
strcpy(ltp_up_sell.symbol, "AAPL");

// ---------- Iceberg SELL ----------
oms::ClientOrder iceberg_sell{};
iceberg_sell.is_active = true;
iceberg_sell.price = 10200;
iceberg_sell.quantity = 240;
iceberg_sell.display_quantity = 60;
iceberg_sell.type = matching_engine::OrderType::SELL;
iceberg_sell.execution_type = oms::ClientOrderType::ICEBERG;
strcpy(iceberg_sell.symbol, "AAPL");

// Buyer that consumes all iceberg slices
oms::ClientOrder iceberg_consumer{};
iceberg_consumer.price = 10200;
iceberg_consumer.quantity = 240;
iceberg_consumer.type = matching_engine::OrderType::BUY;
iceberg_consumer.execution_type = oms::ClientOrderType::LIMIT;
strcpy(iceberg_consumer.symbol, "AAPL");

// ---------- Queue orders in sequence ----------
while(!oms_system.enqueueClientOrder(sell_book1)) {}
while(!oms_system.enqueueClientOrder(sell_book2)) {}
while(!oms_system.enqueueClientOrder(buy_book1)) {}

std::this_thread::sleep_for(std::chrono::milliseconds(50));

while(!oms_system.enqueueClientOrder(limit_buy_match)) {}
while(!oms_system.enqueueClientOrder(partial_buy)) {}
while(!oms_system.enqueueClientOrder(market_buy)) {}

std::this_thread::sleep_for(std::chrono::milliseconds(50));

while(!oms_system.enqueueClientOrder(market_sell)) {}
while(!oms_system.enqueueClientOrder(unmatched_buy)) {}

std::this_thread::sleep_for(std::chrono::milliseconds(50));

while(!oms_system.enqueueClientOrder(stop_loss_sell)) {}
while(!oms_system.enqueueClientOrder(stop_loss_buy)) {}

std::this_thread::sleep_for(std::chrono::milliseconds(50));

while(!oms_system.enqueueClientOrder(ltp_down_buy)) {}
while(!oms_system.enqueueClientOrder(ltp_down_sell)) {}

std::this_thread::sleep_for(std::chrono::milliseconds(50));

while(!oms_system.enqueueClientOrder(ltp_up_buy)) {}
while(!oms_system.enqueueClientOrder(ltp_up_sell)) {}

std::this_thread::sleep_for(std::chrono::milliseconds(50));

while(!oms_system.enqueueClientOrder(iceberg_sell)) {}
std::this_thread::sleep_for(std::chrono::milliseconds(50));
while(!oms_system.enqueueClientOrder(iceberg_consumer)) {}

std::this_thread::sleep_for(std::chrono::seconds(2));
    

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::cout << "Test Complete. Check logs for processing." << std::endl;

    engine.terminate();
    oms_system.stop();
    if(engine_thread.joinable()) engine_thread.join();

    saveEndOfDayCSV("data/symbols.csv", oms_system.shared_memory_ptr, symbol_data);
    return 0;
}