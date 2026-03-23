#ifndef TRADE_RING_BUFFER_H
#define TRADE_RING_BUFFER_H

#include "trade.h"
#include <string>
#include <fcntl.h>
#include <memory>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
#include <atomic>

namespace TradeRingBuffer {

    constexpr uint64_t RING_SIZE {1 << 20};     // number of blocks of data
    constexpr uint64_t MOD       {1LL << 60};   // used for sequence number wrapping 

    // Struct that stores the 
    struct alignas(64) item_node {
        Trade curr_trade;
        std::atomic<uint64_t> seq;
    };

    struct ring_buffer {
        item_node ring[RING_SIZE];                      // Stores the ring
        alignas(64) std::atomic<pid_t> producer_pid;    // Stores the pid of the producer that can used to check if producer is alive
    };

    // Class that implements the ring buffer functionality
    // All methods are non-blocking and use atomic operations for synchronization
    // 
    class trade_ring_buffer {
    public:
        explicit trade_ring_buffer(bool IsProducer, const std::string& shm_name);       // Takes filename as parameter
        ~trade_ring_buffer();

        // Producer APIs
        bool add_trade(Trade &);                // Function used for adding data to ring buffer

        // Consumer APIs
        bool any_new_trade();                   // Returns true if there is an unprocessed trade
        bool lagged_out();                      // Returns true if some unread data was overwritten
        void get_trade(void *);                 // Returns true is the new data was copied successfully directly into 'address'
        Trade get_trade();                      // Returns an copy of the data

    private:
        void update_index_and_seq();            // Updates index and next_expected_seq after reading a trade

        ring_buffer * rb;                       // Pointer to the shared ring buffer
        uint64_t index;                         // stores the index in the ring buffer that is being accessed
        uint64_t next_expected_seq;             // calculated using MOD
        bool is_producer;                       // true if the instance is a producer   
        std::string filename;
    };
};

#endif




/*

Some things to note:
- MOD and RING_SIZE constants are powers of 2 to allow for faster modulo operations using bitwise AND
- All the APIs run deterministically with minimum logic branches to reduce latency
- All the data is cache aligned to reduce false sharing and improve performance

TODO:
- Try getting rid of MOD entirely by experimentally verifying that it is not needed with 64-bit seq numbers
- Extend the producer identity because pid can be reused by the OS after a process exits

*/
