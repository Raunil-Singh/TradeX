
#include "InternalQueue.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <atomic>
#include <thread>
#include <chrono>
#include <string>
#include <string_view>
#include <cstring>

static const auto time1 = std::chrono::microseconds(1);
constexpr static int maxTrades{10'000};
constexpr static int64_t fileSize{maxTrades * sizeof(Trade)}; //Calculating the max file size, depending that all trades never exceed this amount
bool marketOpen{true};
namespace TradeProcessor{
    class TradeProcessor{
    private:
        std::atomic_uint64_t safeOffset{0};
        InternalQueue iQueue;
        uint8_t* mmapPtr;
        std::string fileName;
        uint64_t lastOffset{0};
        int fd;
        std::atomic_bool t2done{false};

    public:
        TradeProcessor(std::string fileName_):
        fileName(fileName_)
        {
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
        void thread1(){
            for(int i = 0; i < 1'000; i++){
                iQueue.push(Trade());
            }
        }

        void thread2(){
            //no shutdown implemented yet. 
            uint64_t writeOffset{0};
            while(marketOpen||!iQueue.empty()){
                auto trade_opt = iQueue.pop();
                std::memcpy(mmapPtr + writeOffset, &trade_opt, sizeof(Trade));
                writeOffset += sizeof(Trade);
                safeOffset.store(writeOffset, std::memory_order_release);
            }
            t2done.store(true, std::memory_order_release);
        }

        void thread3(){
            //no shutdown yet, frequency of sleep not decided yet, 

            uint64_t currOffset{0};
            while(marketOpen||!t2done.load(std::memory_order_acquire)){
                std::this_thread::sleep_for(time1);
                currOffset = safeOffset.load(std::memory_order_acquire);
                if(currOffset != lastOffset){

                    msync(mmapPtr + lastOffset, currOffset - lastOffset, MS_ASYNC); 
                    lastOffset = safeOffset.load(std::memory_order_acquire);
                }
            }
            currOffset = safeOffset.load(std::memory_order_acquire);
            if(currOffset != lastOffset){
                msync(mmapPtr + lastOffset, safeOffset.load(std::memory_order_acquire) - lastOffset, MS_ASYNC); 
                lastOffset = safeOffset.load(std::memory_order_acquire);
            }
            munmap(mmapPtr, fileSize);
            close(fd);
        }
    };

}

int main(){
    TradeProcessor::TradeProcessor tp("file1.txt");
    std::thread t1([&]{ tp.thread1(); });
    std::thread t2([&]{ tp.thread2(); });
    std::thread t3([&]{ tp.thread3(); });
    marketOpen = false;
    t1.join();
    t2.join();
    t3.join();
}