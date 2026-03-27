#include "ring_buffer.h"

ring_buffer_mem::ring_buffer_mem(std::array<int, mem_regions>& fdArray):
fdArray(fdArray)
{
    head.store(0, std::memory_order_relaxed);
    tail.store(0, std::memory_order_relaxed);

    for (int i = 0; i < mem_regions; i++)
    {
        memRegions[i].first.store(i, std::memory_order_relaxed);
    }
}


ring_buffer_mem::ring_buffer_mem(std::string file_name_base, std::array<int, mem_regions>& fdArray) :
file_name_base(file_name_base), 
fdArray(fdArray)
{
    head.store(0, std::memory_order_relaxed);
    tail.store(0, std::memory_order_relaxed);
    for (int i = 0; i < mem_regions; i++)
    {
        memRegions[i].first.store(i, std::memory_order_relaxed);
    }
    for (int i = 0; i < mem_regions; i++)
    {
        std::string file_name = path + file_name_base + std::to_string(fileNumber++);
        int fd = open(file_name.data(), O_CREAT | O_RDWR, 0644);
        if (fd == -1)
        {
            // error in opening file
        }
        fdArray[i] = fd;
        uint8_t* mmap_ptr = static_cast<uint8_t *>(mmap(nullptr, fileSize, PROT_WRITE, MAP_SHARED, fd, 0));
        if (memRegions[i].second == MAP_FAILED)
        {
            std::cout << "Error in initialization\n";
            exit(-1);
            // error in mmap, based on errno, handle
        }
        ftruncate(fd, fileSize);//increase file size
        madvise(memRegions[i].second, fileSize, MADV_WILLNEED);
        give_region(mmap_ptr);
    }
}

bool ring_buffer_mem::get_region(uint8_t*& out)
{
    if (out != nullptr)
    {
        return false;
    }
    uint64_t pos = head.load(std::memory_order_relaxed);

    
    int index_pos = pos & (mem_regions - 1);
    uint64_t seq_number = memRegions[index_pos].first.load(std::memory_order_acquire);

    if (pos + 1 == seq_number)
    {
        if (head.compare_exchange_strong(pos, pos + 1, std::memory_order_acquire, std::memory_order_relaxed)) // i dont understand the memory ordering requirements for cas
        {
            out = memRegions[index_pos].second;
            memRegions[index_pos].first.store(pos + mem_regions, std::memory_order_release); //indicating slot ready for usage as storage again
            return true;
        }
    }

    return false;
}

bool ring_buffer_mem::give_region(uint8_t* pmem_region)
{
    if (pmem_region == nullptr)
    {
        return false;
    }
    uint64_t pos = tail.load(std::memory_order_relaxed);
    
    int index_pos = pos & (mem_regions - 1);
    uint64_t seq_number = memRegions[index_pos].first.load(std::memory_order_acquire);

    if (pos == seq_number)
    {
        if (tail.compare_exchange_strong(pos, pos + 1, std::memory_order_release, std::memory_order_relaxed)) // i dont understand the memory ordering requirements for CAS
        {
            memRegions[index_pos].second = pmem_region;
            memRegions[index_pos].first.store(seq_number + 1, std::memory_order_release);
            return true;
        }
    }
    
    return false;
}