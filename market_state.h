#pragma once
#include <atomic>
#include <cstdint>
#include "order.h"

namespace shared_data {

struct MarketState {
    std::atomic<uint64_t> last_price[MAX_SYMBOLS];
};

}