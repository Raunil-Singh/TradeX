#ifndef TRADE_H
#define TRADE_H

#include <cstring>
#include <iostream>
#include <cstdint>

namespace matching_engine{

typedef struct {
    uint64_t trade_id;
    uint64_t timestamp_ns;
    uint64_t price;
    uint64_t buy_order_id;
    uint64_t sell_order_id;
    uint32_t symbol_id;
    int32_t quantity;
} Trade;
}
#endif