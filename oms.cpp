#include "oms.h"
#include <chrono>
#include <iostream>
#include <thread>
#include <algorithm>

namespace oms {

OrderManagementSystem::OrderManagementSystem(matching_engine::MatchingEngineDispatcher* eng, size_t capacity) : incoming_orders(capacity), engine(eng) {
    int shm_fd = shm_open("/oms_market_data", O_RDONLY, 0666);
    if (shm_fd != -1) {
        shared_memory_ptr = (MarketData*)mmap(NULL, sizeof(uint64_t) * MAX_SYMBOLS, PROT_READ, MAP_SHARED, shm_fd, 0);
    }
    for(int i = 0; i < NUM_GROUPS; i++) {
        // Reading from multiple ring buffers that the matching engine pushes into 
    }
}

OrderManagementSystem::~OrderManagementSystem() {
    stop();
    for(auto* t : trade_consumers) {
        delete t;
    }
}

uint64_t OrderManagementSystem::getCurrentTimestamp() {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
           std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

int OrderManagementSystem::find_id(const std::string& symbol) {
    uint32_t hash = 0;

    for(int i = 0; i<4; ++i){
        uint32_t char_val = 0;
        if(i<symbol.length()){
            char_val = (uint32_t)(toupper(symbol[i]) - 'A' + 1);
        }
        hash = (hash<<5) | char_val;
    }
    
    if (hash >= 880000) return -1; 

    return symbolLookupTable[hash];
}

void OrderManagementSystem::start() {
    oms_thread = std::thread(&OrderManagementSystem::listenForClientOrder, this);
}

void OrderManagementSystem::stop() {
    shutdown.store(true, std::memory_order_release);
    if(oms_thread.joinable()) {
        oms_thread.join();
    }
}

void OrderManagementSystem::sendToEngine(const InternalOrder& ord) {
    engine->dispatch_order(ord);
}

void OrderManagementSystem::listenForClientOrder() {
    ClientOrder new_order;

    while(!shutdown.load(std::memory_order_relaxed)) {
        InternalOrder out;
        while(incoming_orders.pop(new_order)) {
            //ICEBERG
            if (new_order.execution_type == matching_engine::OrderExecutionType::ICEBERG) {
                // Friend's Iceberg Logic
                active_icebergs[new_order.client_order_id] = new_order;
                send_slice(new_order.client_order_id);
            }
            //STOP LOSS
            else if (new_order.execution_type == matching_engine::OrderExecutionType::STOP_LOSS) {
                registerStopLoss(new_order); 
            } 
            //MARKET
            else if (new_order.execution_type == matching_engine::OrderExecutionType::MARKET) {
                out.execution_type = matching_engine::OrderExecutionType::MARKET;
                out.order_id = new_order.client_order_id;
                out.quantity = new_order.quantity;
                out.symbol_id = find_id(new_order.symbol);
                out.timestamp = getCurrentTimestamp();
                out.trader_id = new_order.trader_id;
                out.type = new_order.type;
                out.price = (new_order.type == matching_engine::OrderType::BUY) ? TICK_MAX_PRICE : TICK_MIN_PRICE;
                sendToEngine(out); 
            }
            //LIMIT
            else {
                out.execution_type = matching_engine::OrderExecutionType::LIMIT;
                out.order_id = new_order.client_order_id;
                out.quantity = new_order.quantity;
                out.symbol_id = find_id(new_order.symbol);
                out.timestamp = getCurrentTimestamp();
                out.trader_id = new_order.trader_id;
                out.type = new_order.type;
                out.price = new_order.price;
                sendToEngine(out); 
            }
        }

        //checks for changes in last traded price and triggers SL
        if(shared_memory_ptr){
        for (uint32_t i=0; i<1000; i++) {
            uint64_t current_ltp = shared_memory_ptr[i].last_price;
            
            if (current_ltp != last_seen_price[i]) {
                checkAndTriggerSL(i, current_ltp);
                last_seen_price[i] = current_ltp;
            }
        }
        }
        
    }
}

//STOP LOSS Processing
auto MaxTrigger = [](const ClientOrder& a, const ClientOrder& b) {
    return a.trigger_price < b.trigger_price; 
};

auto MinTrigger = [](const ClientOrder& a, const ClientOrder& b) {
    return a.trigger_price > b.trigger_price; 
};

void OrderManagementSystem::registerStopLoss(const ClientOrder& ord) {
    uint32_t sym_id = find_id(ord.symbol);
    if (sym_id == -1) return;

    matching_engine::Order immediate_ord;
    immediate_ord.order_id = ord.client_order_id;
    immediate_ord.symbol_id = sym_id;
    immediate_ord.timestamp = getCurrentTimestamp();
    immediate_ord.trader_id = ord.trader_id;
    immediate_ord.quantity = ord.quantity;
    immediate_ord.execution_type = matching_engine::OrderExecutionType::MARKET;

    auto& container = stop_loss_registry[sym_id%1000];
    if (container.sell_stops.capacity() == 0) container.init();

    if (ord.type == matching_engine::OrderType::BUY) {
        immediate_ord.type = matching_engine::OrderType::BUY;
        immediate_ord.price = TICK_MAX_PRICE; 
        //send the buy order to matching engine as market order
        sendToEngine(immediate_ord);

        //store the sell order in stop loss registry
        container.sell_stops.push_back(ord);
        std::push_heap(container.sell_stops.begin(), container.sell_stops.end(), MaxTrigger);
    }
    else {
        immediate_ord.type = matching_engine::OrderType::SELL;
        immediate_ord.price = TICK_MIN_PRICE;
        //send the sell order to matching engine as market order
        sendToEngine(immediate_ord);

        //store the buy order in stop loss registry
        container.buy_stops.push_back(ord);
        std::push_heap(container.buy_stops.begin(), container.buy_stops.end(), MinTrigger);
    }
}

void OrderManagementSystem::checkAndTriggerSL(uint32_t sym_id, uint64_t current_ltp) {
    auto& container = stop_loss_registry[sym_id];

    //Trigger all the orders with trigger price greater than current price
    while (!container.sell_stops.empty() && current_ltp <= container.sell_stops.front().trigger_price) {
        
        ClientOrder triggered_client_ord = container.sell_stops.front();
        std::pop_heap(container.sell_stops.begin(), container.sell_stops.end(), MaxTrigger);
        container.sell_stops.pop_back();

        matching_engine::Order engine_ord;
        engine_ord.order_id = triggered_client_ord.client_order_id;
        engine_ord.price = triggered_client_ord.price; 
        engine_ord.symbol_id = find_id(triggered_client_ord.symbol);
        engine_ord.timestamp = getCurrentTimestamp();
        engine_ord.trader_id = triggered_client_ord.trader_id;
        engine_ord.type = matching_engine::OrderType::SELL; 
        engine_ord.quantity = triggered_client_ord.quantity;
        engine_ord.execution_type = matching_engine::OrderExecutionType::LIMIT;

        //send the order to matching engine as limit order
        sendToEngine(engine_ord);
    }

    //Trigger all buy orders with trigger price less than current price
    while (!container.buy_stops.empty() && current_ltp >= container.buy_stops.front().trigger_price) {
        
        ClientOrder triggered_client = container.buy_stops.front();
        std::pop_heap(container.buy_stops.begin(), container.buy_stops.end(), MinTrigger);
        container.buy_stops.pop_back();

        matching_engine::Order engine_ord;
        engine_ord.order_id = triggered_client.client_order_id;
        engine_ord.price = triggered_client.price; 
        engine_ord.symbol_id = find_id(triggered_client.symbol);
        engine_ord.timestamp = getCurrentTimestamp();
        engine_ord.trader_id = triggered_client.trader_id;
        engine_ord.type = matching_engine::OrderType::BUY;
        engine_ord.quantity = triggered_client.quantity;
        engine_ord.execution_type = matching_engine::OrderExecutionType::LIMIT;

        //send the order to matching engine as limit order
        sendToEngine(engine_ord);
    }
}

//ICEBERG Processing
uint64_t OrderManagementSystem::submit_iceberg_order(uint32_t symbol_id, uint64_t price, matching_engine::OrderType side, uint32_t total_qty, uint32_t display_qty, uint64_t trader_id) {
    ClientOrder order;
    order.client_order_id = next_client_order_id.fetch_add(1, std::memory_order_relaxed);
    order.symbol_id = symbol_id;
    order.price = price;
    order.type = side;
    order.quantity = total_qty;
    order.display_quantity = display_qty;
    order.filled_quantity = 0;
    order.trader_id = trader_id;
    order.is_active = true;

    while(!incoming_orders.push(order)) { // Pushing into queue for oms thread to pick up
    }
    return order.client_order_id;
}

void OrderManagementSystem::send_slice(uint64_t parent_id) {
    ClientOrder& parent = active_icebergs[parent_id];
    if(!parent.is_active) return;

    uint32_t remaining = parent.quantity - parent.filled_quantity;
    if(remaining == 0) {
        parent.is_active = false;
        return;
    }

    uint32_t slice_qty = std::min(parent.display_quantity, remaining);
    uint64_t child_id = next_child_order_id.fetch_add(1, std::memory_order_relaxed);

    // Map child back to parent so we can track fills
    child_to_parent[child_id] = parent_id;
    InternalOrder child_order;
    child_order.order_id = child_id;
    child_order.symbol_id = parent.symbol_id;
    child_order.price = parent.price;
    child_order.quantity = slice_qty;
    child_order.type = parent.type;
    child_order.execution_type = matching_engine::OrderExecutionType::LIMIT;
    child_order.trader_id = parent.trader_id;
              
    engine->dispatch_order(child_order);
}

void OrderManagementSystem::check_fill(const matching_engine::Trade& trade) {
    // Find if the buy or sell order was ours
    uint64_t our_child_id = 0;
    if(child_to_parent.count(trade.buy_order_id)) our_child_id = trade.buy_order_id;
    else if(child_to_parent.count(trade.sell_order_id)) our_child_id = trade.sell_order_id;
    else return; // Not our order

    uint64_t parent_id = child_to_parent[our_child_id];
    ClientOrder& parent = active_icebergs[parent_id];

    parent.filled_quantity += trade.quantity;
    
    bool slice_filled = (parent.filled_quantity % parent.display_quantity == 0) || (parent.filled_quantity == parent.quantity);

    if(slice_filled) {
        child_to_parent.erase(our_child_id);
        send_slice(parent_id);
    }
}
} 