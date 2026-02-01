#ifndef ORDER_H
#define ORDER_H

#include <cstring>
#include <iostream>
#include <cstdint>

#define PRICE_SCALE 100

#define LOWER_LIMIT_PRICE 100   
#define UPPER_LIMIT_PRICE 101

#define LOWER_LIMIT_TICKS (LOWER_LIMIT_PRICE * PRICE_SCALE)
#define UPPER_LIMIT_TICKS (UPPER_LIMIT_PRICE * PRICE_SCALE)

#define PRICE_LEVELS (UPPER_LIMIT_TICKS - LOWER_LIMIT_TICKS + 1)

#define MAX_ORDERS_PER_LEVEL 100
#define SYMBOL_MAX_LENGTH 20

typedef enum {
    BUY,
    SELL
} OrderType;

typedef enum{
    MARKET,             
    LIMIT,
    //FOK,
    //IOC,
    //ICEBERG
} OrderExecutionType;

//Order Structure
typedef struct {
    uint64_t order_id;
    char symbol[SYMBOL_MAX_LENGTH];
    OrderType type;
    double price;
    uint32_t quantity;
    OrderExecutionType execution_type;
    uint64_t timestamp;
    uint64_t trader_id;
} Order;

#endif