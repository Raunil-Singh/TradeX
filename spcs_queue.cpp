#include "spsc_queue.h"


spsc_queue::spsc_queue(std::array<int, mem_regions>& fdArray):
fdArray(fdArray)
{
    head.store(0, std::memory_order_relaxed);
    tail.store(0, std::memory_order_relaxed);

    
}


spsc_queue::spsc_queue(std::string file_name_base, std::array<int, mem_regions>& fdArray) :
file_name_base(file_name_base), 
fdArray(fdArray)
{
    head.store(0, std::memory_order_relaxed);
    tail.store(0, std::memory_order_relaxed);
    for (int i = 0; i < mem_regions - 1; i++)
    {
        std::string file_name = path + file_name_base + std::to_string(fileNumber++);
        int fd = open(file_name.data(), O_CREAT | O_RDWR, 0644);
        if (fd == -1)
        {
            
            perror("Open failed for file");
            std::cout << i << "\n";
            exit(-1);
        }
        fdArray[i] = fd;
        ftruncate(fd, fileSize);//increase file size
        uint8_t* mmap_ptr = static_cast<uint8_t *>(mmap(nullptr, fileSize, PROT_WRITE, MAP_SHARED, fd, 0));
        if (mmap_ptr == MAP_FAILED)
        {
            perror("Error in initialization");
            exit(-1);
            // error in mmap, based on errno, handle
        }
        madvise(mmap_ptr, fileSize, MADV_WILLNEED);
        give_region(mmap_ptr);
    }
}
bool spsc_queue::give_region(uint8_t* pmem_region)
{
    uint64_t _head = head.load(std::memory_order_acquire);
    uint64_t _tail = tail.load(std::memory_order_relaxed);

    if (((_tail + 1) & (mem_regions - 1)) != (_head & (mem_regions - 1))) // queue not full
    {
        memRegions[_tail & (mem_regions - 1)] = pmem_region;
        tail.store(_tail + 1, std::memory_order_release);
        return true;
    }

    return false;
}

bool spsc_queue::get_region(uint8_t*& out)
{
    uint64_t _head = head.load(std::memory_order_relaxed);
    uint64_t _tail = tail.load(std::memory_order_acquire);

    if (_tail != _head) // queue not empty
    {
        out = memRegions[_head & (mem_regions - 1)];
        head.store(_head + 1, std::memory_order_release);
        return true;
    }

    return false;
}