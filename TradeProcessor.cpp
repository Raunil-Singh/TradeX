/* This is not robust yet, as we're dealing with hand off issues with the block. Refer to writer thread for more details.*/
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

static const auto time1 = std::chrono::microseconds(1);
constexpr static int maxTradesPerFile{10'000'000}; //change later
constexpr static int64_t fileSize{maxTradesPerFile * sizeof(matching_engine::Trade)}; //Calculating the max file size, depending that all trades never exceed this amount
const int static page_size{getpagesize()};
const int static pages_per_chunk{4}; //Having fixed chunk size
const int chunk_size{pages_per_chunk * page_size};
constexpr static double reallocation_threshold{0.5};
static int fileNumber{1};
bool marketOpen{true};
static int writerDuration;
static int persistenceDuration;
namespace TradeProcessor{
    class TradeProcessor{
    private:
        std::atomic_uint64_t safeOffset{0};
        std::atomic_uint32_t curr_chunk{0};
        uint8_t* mmapPtr;
        uint8_t* mmapPtrTemp;
        std::string fileName;
        uint64_t lastOffset{0};
        int fd;
        std::atomic_bool t2done{false};
        TradeRingBuffer::trade_ring_buffer trBuffer;
        std::shared_mutex smutex;
        std::condition_variable cv;


    public:
        TradeProcessor(std::string fileName_):
        fileName(fileName_),
        trBuffer(false)
        {
            //fileName must follow our convention, specified by reallocator
            fd = open(fileName.data(), O_CREAT|O_RDWR, 0644);
            if(fd == -1){
                //error in opening file
            }
            ftruncate(fd, fileSize);//increase file size
            mmapPtr = static_cast<uint8_t*> (mmap(nullptr, fileSize, PROT_WRITE, MAP_SHARED, fd, 0));
            if(mmapPtr == MAP_FAILED){
                //error in mmap, based on errno, handle
            }
        }

        void writerThread(){
            //no shutdown implemented yet. 
            uint64_t writeOffset{0};
            auto tstart = std::chrono::steady_clock::now();
            // std::thread reallocatorThread(reallocator); why is this here
            while(true){
                while(!trBuffer.any_new_trade()){

                }
                //write into our memory block
                trBuffer.get_trade(mmapPtr + writeOffset);
                writeOffset += sizeof(matching_engine::Trade);
                //update current chunk to be most up to date
                if(writeOffset > curr_chunk * chunk_size){
                    curr_chunk.fetch_add((writeOffset - curr_chunk * chunk_size) / chunk_size, std::memory_order_release);
                }
                /*This is the most problematic section, as we're updating global mmapPtr, but that affects our persistence thread as well.
                The root cause behind this is the fact that we have no guarantees we will not exceed our original mmap block. We need a way
                to reliably handle switchover to new blocks with persistence thread still having the info of the old block, as well as
                not creating too many blocks that we exceed ram storage.*/
                if(writeOffset >= fileSize){
                    // curr_chunk.store(0);
                    // mmapPtr = mmapPtrTemp;
                    // writeOffset = 0;
                    // munmap(mmapPtr); //This is BAD

                    break;
                }
            }
            curr_chunk++;
            t2done.store(true, std::memory_order_release);
            auto tdone = std::chrono::steady_clock::now();
            writerDuration = std::chrono::duration_cast<std::chrono::microseconds>(tdone - tstart).count();

        }
        /*Only has an issue with losing previous block before all chunks could be synced.*/
        void persistenceThread(){
            auto tstart = std::chrono::steady_clock::now();
            uint32_t local_chunk_counter{};
            while(!t2done.load(std::memory_order_acquire)){

                //check if local_chunk != global, improve this part
                if(curr_chunk.load(std::memory_order_acquire) == local_chunk_counter){

                    continue;
                }
                msync(mmapPtr + local_chunk_counter * chunk_size, (curr_chunk - local_chunk_counter) * chunk_size, MS_ASYNC);
                local_chunk_counter = curr_chunk.load(std::memory_order_acquire);
            }
            if(local_chunk_counter != curr_chunk.load(std::memory_order_relaxed)){
                msync(mmapPtr + local_chunk_counter * chunk_size, (curr_chunk - local_chunk_counter) * chunk_size, MS_ASYNC);
            }
            munmap(mmapPtr, fileSize);
            close(fd);
            auto tend = std::chrono::steady_clock::now();
            persistenceDuration = std::chrono::duration_cast<std::chrono::microseconds>(tend - tstart).count();
        }
        //This is fine, no issues that we can see so far
        void reallocator(){
            //do we need strict memory ordering here, and how to optimize any overusage of atomics
            while(marketOpen||!t2done.load(std::memory_order_acquire)){
                if(curr_chunk.load(std::memory_order_relaxed) * chunk_size > reallocation_threshold * fileSize){
                    std::string file_name = "file_" + std::to_string(++fileNumber);
                    fd = open(file_name.data(), O_CREAT|O_RDWR, 0644); // updated file descriptor
                    ftruncate(fd, fileSize); //size allocated
                    mmapPtrTemp = static_cast<uint8_t*>(mmap(nullptr, fileSize, PROT_WRITE, MAP_SHARED, fd, 0));
                }
            }
        }
    };

}

int main(){
    TradeProcessor::TradeProcessor tp("file1.txt");

    TradeRingBuffer::trade_ring_buffer trb(true);
    for(int i = 0; i < maxTradesPerFile; i++){
        matching_engine::Trade* t = new matching_engine::Trade();
        trb.add_trade(*t);
    }
    auto tp1 = std::chrono::steady_clock::now();
    std::thread t2([&]{ tp.writerThread(); });
    std::thread t3([&]{ tp.persistenceThread(); });
    // std::thread t1([&]{ tp.reallocator();});
    marketOpen = false;
    t2.join();
    t3.join();
    auto tp2 = std::chrono::steady_clock::now();
    std::cout<<"Time taken: "<<std::chrono::duration_cast<std::chrono::microseconds>(tp2 - tp1).count()<<" mus\n";
    std::cout<<"Time spent in writer thread: "<<writerDuration<<" mus \n";
    std::cout<<"Time spent in persistence thread: "<<persistenceDuration<<" mus \n";
    std::cout<<"Average time per trade: "<<static_cast<double>(persistenceDuration)/maxTradesPerFile<<" mus\n";

}