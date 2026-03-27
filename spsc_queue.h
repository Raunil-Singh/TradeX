#ifndef SPSC_QUEUE
#define SPSC_QUEUE


#include "trade.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <atomic>
#include <thread>
#include <chrono>
#include <string>
#include <string_view>
#include <cstring>
#include "trade_ring_buffer.h"
#include <shared_mutex>
#include <mutex>
#include <condition_variable>
#include <array>
#include <utility>
#include <cstdint>

constexpr size_t queue_buffer_size{1 << 20}; //arbitrary size
const size_t mem_regions(32);
static int fileNumber{1};
constexpr static int maxTradesPerFile{10'000'000}; //change later
constexpr static int64_t fileSize{maxTradesPerFile * sizeof(matching_engine::Trade) / mem_regions}; //Calculating the max file size, depending that all trades never exceed this amount
const std::string path{"./file_output/"};


class spsc_queue{
    private:
        std::array<uint8_t*, mem_regions> memRegions;
        std::array<int, mem_regions> fdArray;
        std::string file_name_base;
        alignas(64) std::atomic_uint64_t head;
        alignas(64) std::atomic_uint64_t tail;

    public:
        spsc_queue(std::array<int, mem_regions>&);
        spsc_queue(std::string, std::array<int, mem_regions>&);
        bool get_region(uint8_t*&); //get a region of memory to consume 
        bool give_region(uint8_t*); //return a region of memory to be consumed
};

#endif