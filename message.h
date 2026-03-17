#ifndef MESSAGE
#define MESSAGE
#include <stdint.h>

struct alignas(64) MarketDataMessage {
    uint64_t seq_num;
    uint64_t timestamp_ns;
    uint64_t price;
    uint32_t symbol_id;
    uint32_t quantity;
    uint64_t trade_id;

};

#endif