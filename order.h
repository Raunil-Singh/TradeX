#ifndef ORDER_H
#define ORDER_H

#include <cstring>
#include <iostream>
#include <cstdint>

namespace matching_engine{

#define MAX_ORDERS_PER_LEVEL 100
#define SYMBOL_MAX_LENGTH 20
uint32_t NULL_IDX = UINT32_MAX;  

enum class OrderType {
    BUY,
    SELL
};

enum class OrderExecutionType{
    MARKET,             
    LIMIT,
    //FOK,
    //IOC,
    //ICEBERG
};

//Order Structure
typedef struct {
    uint64_t order_id;
    uint64_t price;
    uint64_t symbol_id;
    uint64_t timestamp;
    uint64_t trader_id;
    OrderType type;  
    uint32_t quantity;
    OrderExecutionType execution_type;
} Order;
}
#endif