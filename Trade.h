#include <stdint.h>

typedef struct {
uint64_t trade_id;
uint64_t timestamp_ns;
uint32_t symbol_id;
double price;
int32_t quantity;
uint64_t buyer_id;
uint64_t seller_id;
uint64_t buy_order_id;
uint64_t sell_order_id;
} Trade;