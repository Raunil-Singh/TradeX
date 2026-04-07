#include "spsc_queue.h"

bool spsc_queue::push(uint64_t&& msg)
{
    uint64_t _head = head.load(std::memory_order_acquire);
    uint64_t _tail = tail.load(std::memory_order_relaxed);

    if (((_tail + 1) & (queue_buffer_size - 1)) != (_head & (queue_buffer_size - 1))) // queue not full
    {
        array[_tail & (queue_buffer_size - 1)] = std::move(msg);
        tail.store(_tail + 1, std::memory_order_release);
        return true;
    }

    return false;
}

bool spsc_queue::pop(uint64_t& out_msg)
{
    uint64_t _head = head.load(std::memory_order_relaxed);
    uint64_t _tail = tail.load(std::memory_order_acquire);

    if (_tail != _head) // queue not empty
    {
        out_msg = array[_head & (queue_buffer_size - 1)];
        head.store(_head + 1, std::memory_order_release);
        return true;
    }

    return false;
}