#include "ring_buffer.h"

ring_buffer_mem::ring_buffer_mem(std::string file_name_base) : file_name_base(file_name_base)
{
    for (int i = 0; i < mem_regions; i++)
    {
        std::string file_name = path + file_name_base + std::to_string(fileNumber++);
        int fd = open(file_name.data(), O_CREAT | O_RDWR, 0644);
        if (fd == -1)
        {
            // error in opening file
        }
        fdArray[i] = fd;
        memRegions[i] = std::pair<std::atomic_uint64_t, uint8_t *>({i, static_cast<uint8_t *>(mmap(nullptr, fileSize, PROT_WRITE, MAP_SHARED, fd, 0))});
        madvise(memRegions[i].second, fileSize, MADV_WILLNEED);
        if (memRegions[i].second == MAP_FAILED)
        {
            // error in mmap, based on errno, handle
        }
    }
}

bool ring_buffer_mem::get_region((uint8_t*)& out)
{

    uint64_t local_head = head.load(std::memory_order_relaxed);
    uint64_t local_tail = tail.load(std::memory_order_relaxed);
    if (local_head != local_tail)
    {
        int index_pos = local_head & (mem_regions - 1);
        uint64_t seq_number = memRegions[indexPos].first.load(std::memory_order_acquire);

        if (local_head + 1 == seq_number)
        {
            if (head.compare_exchange_weak(local_head, local_head + 1, std::memory_order_relaxed, std::memory_order_relaxed))
            {
                memRegions[indexPos].first.store(local_head - 1 + (mem_regions - 1), std::memory_order_release); 
                out = memRegions.second;
                return true;
            }
        }
    }
    return false;
}

bool ring_buffer_mem::give_region(uint8_t* pmem_region)
{
    uint64_t local_head = head.load(std::memory_order_relaxed);
    uint64_t local_tail = tail.load(std::memory_order_relaxed);
    if (local_tail + 1 != local_head)
    {
        int index_pos = local_tail & (mem_regions - 1);
        uint64_t seq_number = memRegions[index_pos].first.load(std::memory_order::acquire)

        if (local_tail == seq_number)
        {
            if (tail.compare_exchange_weak(local_tail, local_tail + 1, std::memory_order_relaxed, std::memory_order_relaxed))
            {
                memRegions[index_pos].second = pmem_region
                memRegions[index_pos].first.store(seq_number + 1, std::memory_order_release);
                return true;
            }
        }
    }
    return false;
}