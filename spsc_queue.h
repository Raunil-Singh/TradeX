#ifndef SPSC_QUEUE
#define SPSC_QUEUE

#include <atomic>

#include "trade.h"
#include "message.h"

//api file only


constexpr size_t queue_buffer_size{1 << 20}; //arbitrary size

class spsc_queue{
    private:
        alignas(64) std::atomic_uint64_t head;
        alignas(64) std::atomic_uint64_t tail;
        MarketDataMessage array[queue_buffer_size];

    public:
        bool push(MarketDataMessage&&);
        bool pop(MarketDataMessage&);
};

#endif