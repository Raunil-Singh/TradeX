#ifndef TRADE_H
#define TRADE_H

#include <cstring>
#include <iostream>
#include <cstdint>

typedef struct {
    uint64_t trade_id;
    uint64_t timestamp_ns;
    uint32_t symbol_id;
    uint64_t price;
    int32_t quantity;
    uint64_t buy_order_id;
    uint64_t sell_order_id;
} Trade;

#endif