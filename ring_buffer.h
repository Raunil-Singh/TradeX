#ifndef RING_BUFFER
#define RING_BUFFER 

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

const size_t mem_regions(32);
static int fileNumber{1};
constexpr static int maxTradesPerFile{10'000'000}; //change later
constexpr static int64_t fileSize{maxTradesPerFile * sizeof(matching_engine::Trade) / 24}; //Calculating the max file size, depending that all trades never exceed this amount
const std::string path{"./file_output/"};

class ring_buffer_mem
{
    private:
        std::array<std::pair<std::atomic_uint64_t, uint8_t*>, mem_regions> memRegions;
        std::array<int, mem_regions> fdArray;
        std::string file_name_base;
        alignas(64) std::atomic_uint64_t head;
        alignas(64) std::atomic_uint64_t tail;

    public:
        ring_buffer_mem(std::array<int, mem_regions>&);
        ring_buffer_mem(std::string, std::array<int, mem_regions>&);
        bool get_region(uint8_t*&); //get a region of memory to consume 
        bool give_region(uint8_t*); //return a region of memory to be consumed
};

#endif