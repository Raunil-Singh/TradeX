

#include <atomic>




// api file only

constexpr size_t queue_buffer_size{1 << 20}; // arbitrary size

class spsc_queue
{
private:
    alignas(64) std::atomic_uint64_t head;
    alignas(64) std::atomic_uint64_t tail;
    uint64_t *array;

public:
    spsc_queue() : array(new uint64_t[queue_buffer_size]), head(0), tail(0) {}
    bool push(uint64_t &&);
    bool pop(uint64_t &);
    bool empty()
    {
        return head.load(std::memory_order_acquire) == tail.load(std::memory_order_acquire);
    }
};
