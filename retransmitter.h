#include "message.h"
#include <stdint.h>

static const uint64_t SIZE{}; // configure slot sizes to service requests for sequence numbers transmitted 1s ago
class Retransmitter{
    MarketDataMessage slots[SIZE];
    public:
        void store(MarketDataMessage& msg);

        void listener(); //separate thread that processes requests by clients for missing messages
};