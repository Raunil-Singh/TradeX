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
        TradeProcessor(std::string fileName_):
        fileName(fileName_),
        trBuffer(false),
        rb_write("file_", fdArray),
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

        void writerThread(){
            //no shutdown implemented yet. 
            uint64_t writeOffset{0};
            auto tstart = std::chrono::steady_clock::now();
            uint8_t* mem_region = nullptr;
            // std::thread reallocatorThread(reallocator); why is this here
            int trade_counter{};
            int region_claims{};
            int region_gaves{};
            while(trade_counter < maxTradesPerFile){ //add in proper 
                while(!trBuffer.any_new_trade()){ // potential fix and backoff?

                }
                if (mem_region == nullptr)
                {
                    if (rb_write.get_region(mem_region))
                    {
                        std::cout << "claimed region at trade counter(rb_write) = " << ++region_claims<< "\n";
                        writeOffset = 0;
                    }
                    else
                    {
                        // std::cout << "..\n";
                        continue;
                    }
                }
                //write into our memory block
                // trBuffer.get_trade(mmapPtr + writeOffset);
                trBuffer.get_trade(mem_region + writeOffset);
                writeOffset += sizeof(matching_engine::Trade);
                trade_counter++;
                //NOT REQUIRED
                // //update current chunk to be most up to date
                // if(writeOffset >= (curr_chunk + 1) * chunk_size){
                //     curr_chunk.fetch_add((writeOffset - curr_chunk * chunk_size) / chunk_size, std::memory_order_release);
                // }


                /*This is the most problematic section, as we're updating global mmapPtr, but that affects our persistence thread as well.
                The root cause behind this is the fact that we have no guarantees we will not exceed our original mmap block. We need a way
                to reliably handle switchover to new blocks with persistence thread still having the info of the old block, as well as
                not creating too many blocks that we exceed ram storage.*/
                if(writeOffset >= fileSize){
                    rb_persist.give_region(mem_region);
                    std::cout << "gave region at trade counter(rb_persist) = " << ++region_gaves << "\n";
                    mem_region = nullptr;
                    //break;
                }
            }
            if (mem_region != nullptr){
                rb_persist.give_region(mem_region);
            }
            curr_chunk++;
            t2done.store(true, std::memory_order_release);
            auto tdone = std::chrono::steady_clock::now();
            writerDuration = std::chrono::duration_cast<std::chrono::microseconds>(tdone - tstart).count();

        }
        /*Only has an issue with losing previous block before all chunks could be synced.*/
        void persistenceThread(){
            auto tstart = std::chrono::steady_clock::now();
            // uint32_t local_chunk_counter{};
            uint8_t* mem_region = nullptr;
            int region_claims{};
            int region_gaves{};
            while(!t2done.load(std::memory_order_acquire)){
                if(mem_region == nullptr) // want potential backoffs, if necessary?
                {
                    if (!rb_persist.get_region(mem_region)) // get new changes
                    {
                        // std::cout << ".\n";
                        continue;
                    }
                }
                std::cout << "claimed region at trade counter(rb_persist) =  "<< ++region_claims<<"\n";
                // //check if local_chunk != global, improve this part
                // if(curr_chunk.load(std::memory_order_acquire) == local_chunk_counter){

                //     continue;
                // }

                msync(mem_region, fileSize, MS_SYNC); //want to change this to MS_SYNC
                rb_write.give_region(mem_region);
                std::cout << "gave region at trade counter(rb_write) = "<<++region_gaves<<" \n";
                mem_region = nullptr;
                // msync(mem_region + local_chunk_counter * chunk_size, (curr_chunk - local_chunk_counter) * chunk_size, MS_ASYNC); // potential spin wait workaround like above
                // local_chunk_counter = curr_chunk.load(std::memory_order_acquire);

                // if (local_chunk_counter * chunk_size >= maxTradesPerFile) //if exceeded filesize (possibly increase file size by some amount so that we dont crash?), but will we ever exceed, if everything is multiples??
                // {
                //     rb_write.give_region(mem_region);
                //     mem_region = nullptr;
                // }
            }

            //could all below be put in above working queue???
            // if(local_chunk_counter != curr_chunk.load(std::memory_order_relaxed) && mem_region != nullptr){
            //     msync(mem_region + local_chunk_counter * chunk_size, (curr_chunk - local_chunk_counter) * chunk_size, MS_ASYNC);
            // }
            if (mem_region != nullptr){
                msync(mem_region, fileSize, MS_SYNC);
                rb_write.give_region(mem_region);
            }

            //sync remaining regions
            while (rb_persist.get_region(mem_region))
            {
                rb_write.give_region(mem_region); //for easy closure later
                msync(mem_region, fileSize, MS_SYNC);

            }

            //this should be handled while closing the ring buffers
            munmap(mmapPtr, fileSize);
            close(fd);
            auto tend = std::chrono::steady_clock::now();
            persistenceDuration = std::chrono::duration_cast<std::chrono::microseconds>(tend - tstart).count();
        }

        //UNNECESSARY;
        //This is fine, no issues that we can see so far
        // void reallocator(){
        //     //do we need strict memory ordering here, and how to optimize any overusage of atomics
        //     while(marketOpen||!t2done.load(std::memory_order_acquire)){
        //         if(curr_chunk.load(std::memory_order_relaxed) * chunk_size > reallocation_threshold * fileSize){
        //             std::string file_name = "file" + std::to_string(++fileNumber);
        //             fd = open((path + file_name).data(), O_CREAT|O_RDWR, 0644); // updated file descriptor
        //             ftruncate(fd, fileSize); //size allocated
        //             mmapPtrTemp = static_cast<uint8_t*>(mmap(nullptr, fileSize, PROT_WRITE, MAP_SHARED, fd, 0));
        //         }
        //     }
        // }
    };

}

int main(){
    TradeProcessor::TradeProcessor tp("file1.txt");

    TradeRingBuffer::trade_ring_buffer trb(true);
    int trade_count{1};
    for(int i = 0; i < maxTradesPerFile; i++){
        matching_engine::Trade* t = new matching_engine::Trade();
        t->trade_id = trade_count++;
        trb.add_trade(*t);
    }
    auto tp1 = std::chrono::steady_clock::now();
    std::thread t2([&]{ tp.writerThread(); });
    std::thread t3([&]{ tp.persistenceThread(); });
    // std::thread t1([&]{ tp.reallocator();});
    marketOpen.store(false, std::memory_order_release);
    t2.join();
    t3.join();
    auto tp2 = std::chrono::steady_clock::now();
    std::cout<<"Time taken: "<<std::chrono::duration_cast<std::chrono::microseconds>(tp2 - tp1).count()<<" mus\n";
    std::cout<<"Time spent in writer thread: "<<writerDuration<<" mus \n";
    std::cout<<"Time spent in persistence thread: "<<persistenceDuration<<" mus \n";
    std::cout<<"Average time per trade: "<<static_cast<double>(persistenceDuration)/maxTradesPerFile<<" mus\n";

}