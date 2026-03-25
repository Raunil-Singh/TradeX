#ifndef SPSC_QUEUE
#define SPSC_QUEUE
#include "trade.h"
#include "message.h"
//api file only

class spsc_queue{

    public:
        bool push(MarketDataMessage&&);
        bool pop(MarketDataMessage&);
};

#endif