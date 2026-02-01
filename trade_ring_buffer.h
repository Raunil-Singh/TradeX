#ifndef TRADE_RING_BUFFER_H
#define TRADE_RING_BUFFER_H

#include "trade.h"
#include <string>
#include <fcntl.h>
#include <memory>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>

namespace TradeRingBuffer {

    constexpr std::string_view filename {"/Trade_Ring_Buffer"};

    const uint64_t RING_SIZE {1024LL*1024LL};   // number of blocks of data
    const uint64_t MOD {739474490608277033LL};  // A randomly generated 18 digit prime number for sequence number wrap-around

    // Struct that stores the 
    struct alignas(64) item_node {
        Trade curr_trade;
        _Atomic uint64_t seq;
    };

    struct ring_buffer {
        item_node ring[RING_SIZE];              // Stores the ring
        _Atomic pid_t producer_pid;             // Stores the pid of the producer that can used to check if producer is alive
    };


    class trade_ring_buffer {
    public:
        explicit trade_ring_buffer(bool);

        // Producer APIs
        bool add_trade(Trade &);                // Function used for adding data to ring buffer

        // Consumer APIs
        bool any_new_trade();                   // Returns true if there is an unprocessed trade
        bool lagged_out();                      // Returns true if some unread data was overwritten
        void get_trade(void *);                 // Returns true is the new data was copied successfully directly into 'address'
        Trade get_trade();                      // Returns an copy of the data

    private:
        uint64_t index;                         // stores the index in the ring buffer that is being accessed
        uint64_t next_expected_seq;             // calculated using MOD
        bool is_producer;                       // true if the instance is a producer   
    };
};

#endif
