#include "trade.h"
#include "trade_ring_buffer.h"
#include <chrono>
#include <iostream>

int main() {
    TradeRingBuffer::trade_ring_buffer producer(true);
    TradeRingBuffer::trade_ring_buffer consumer(false);

    auto ops_count = 10000 * TradeRingBuffer::RING_SIZE;
    

    Trade t{};
    auto start = std::chrono::high_resolution_clock::now();
    for (uint64_t i = 0; i < ops_count; ++i) {
        producer.add_trade(t);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto total_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    double avg_ns = static_cast<double>(total_ns) / static_cast<double>(ops_count);
    std::cout << "Producer write speed: " << avg_ns << " ns/op\n";
    return 0;
}