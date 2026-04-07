#ifndef TRADE_RING_BUFFER_H
#define TRADE_RING_BUFFER_H

#include "trade.h"
#include <string>
#include <array>
#include <string_view>
#include <fcntl.h>
#include <memory>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
#include <atomic>

namespace TradeRingBuffer {

    constexpr int total_ring_buffer_count = 2; // number of ring buffers, relates to number of groups in matching engine

    // Templatized code to generate unique filenames for shared memory objects at compile time
    template <std::size_t N>
    struct filename_table {
        std::array<std::array<char, 32>, N> storage{};
        std::array<std::string_view, N> views{};

        constexpr filename_table() {
            constexpr std::string_view prefix = "/Trade_Ring_Buffer_";

            for (std::size_t i = 0; i < N; ++i) {
                std::size_t p = 0;

                for (char c : prefix) storage[i][p++] = c;

                // append index i as decimal
                char digits[20]{};
                std::size_t d = 0;
                std::size_t x = i;
                do {
                    digits[d++] = static_cast<char>('0' + (x % 10));
                    x /= 10;
                } while (x > 0);

                while (d > 0) storage[i][p++] = digits[--d];
                storage[i][p] = '\0';

                views[i] = std::string_view{storage[i].data(), p};
            }
        }
    };
    inline constexpr filename_table<total_ring_buffer_count> generated{};
    inline constexpr const auto& filename = generated.views;    // stores the filenames for the shared memory objects 
    static_assert(filename.size() == total_ring_buffer_count);


    constexpr uint64_t RING_SIZE {1 << 20};     // number of blocks of data
    constexpr uint64_t MOD       {1LL << 60};   // used for sequence number wrapping 

    // Struct that stores the trade data and sequence number
    struct alignas(64) item_node {
        matching_engine::Trade curr_trade;
        std::atomic<uint64_t> seq;
    };

    struct ring_buffer {
        item_node ring[RING_SIZE];                      // Stores the ring
    };

    // Class that implements the ring buffer functionality
    // All methods are non-blocking and use atomic operations for synchronization
    // 
    class trade_ring_buffer {
    public:
        explicit trade_ring_buffer(bool, int32_t);
        ~trade_ring_buffer();

        // Producer APIs
        bool add_trade(matching_engine::Trade &);                // Function used for adding data to ring buffer

        // Consumer APIs
        bool any_new_trade();                   // Returns true if there is an unprocessed trade
        bool lagged_out();                      // Returns true if some unread data was overwritten
        void get_trade(void *);                 // Returns true is the new data was copied successfully directly into 'address'
        matching_engine::Trade get_trade();     // Returns an copy of the data

    private:
        void update_index_and_seq();            // Updates index and next_expected_seq after reading a trade

        ring_buffer * rb;                       // Pointer to the shared ring buffer
        uint64_t index;                         // stores the index in the ring buffer that is being accessed
        uint64_t next_expected_seq;             // calculated using MOD
        bool is_producer;                       // true if the instance is a producer   
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