/* This is not robust yet, as we're dealing with hand off issues with the block. Refer to writer thread for more details.*/
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <atomic>
#include <thread>
#include <chrono>
#include <string>
#include <string_view>
#include <cstring>
#include <shared_mutex>
#include <mutex>
#include <condition_variable>
#include <array>
#include <vector>

#include "trade.h"
#include "trade_ring_buffer.h"
#include "spsc_queue.h"
// #include "ring_buffer.h"

static const auto time1 = std::chrono::microseconds(1);
// const int chunk_size{pages_per_chunk * page_size};
// constexpr static double reallocation_threshold{0.5};

std::atomic<bool> marketOpen{true};
static int writerDuration;
static int persistenceDuration;


namespace TradeProcessor{
    class TradeProcessor{
    private:
        std::atomic_uint32_t curr_chunk{0};
        uint8_t* mmapPtr;
        uint8_t* mmapPtrTemp;
        std::string fileName;
        uint64_t lastOffset{0};
        int fd;
        std::atomic_bool t2done{false};
        TradeRingBuffer::trade_ring_buffer trBuffer;
        #ifdef SPSC_QUEUE
        spsc_queue rb_write;
        spsc_queue rb_persist;
        #endif

        #ifdef RING_BUFFER
        ring_buffer_mem rb_write;
        ring_buffer_mem rb_persist;
        #endif
        std::array<int, mem_regions> fdArray;

    public:
        TradeProcessor(std::string fileName_, int id):
        fileName(fileName_),
        trBuffer(false, id),
        rb_write(fileName_, fdArray),
        rb_persist(fdArray)
        {
            


            // //fileName must follow our convention, specified by reallocator
            // fd = open((path + fileName).data(), O_CREAT|O_RDWR, 0644);
            // if(fd == -1){
            //     //error in opening file
            // }
            // ftruncate(fd, fileSize);//increase file size
            // mmapPtr = static_cast<uint8_t*> (mmap(nullptr, fileSize, PROT_WRITE, MAP_SHARED, fd, 0));
            // madvise(mmapPtr, fileSize, MADV_WILLNEED);
            
        }

        void writerThread(){};
        /*Only has an issue with losing previous block before all chunks could be synced.*/
        void persistenceThread(){};
    };

}