#include "trade.h"
#include "trade_ring_buffer.h"
#include <chrono>
#include <queue>
#include <vector>
#include <numeric>
#include <iostream>
#include <fstream>

int main() {
    TradeRingBuffer::trade_ring_buffer producer(true);

    auto ops_count = 1000 * TradeRingBuffer::RING_SIZE;

    constexpr uint64_t queue_ops_count = 10000000;
    std::vector<int64_t> time_rb(ops_count), time_q(queue_ops_count);

    std::cout << "Starting ring buffer test...\n";

    Trade t{};
    {
        for (uint64_t i = 0; i < ops_count; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            producer.add_trade(t);
            auto end = std::chrono::high_resolution_clock::now();
            time_rb[i] = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        }
	    double avg_ns = static_cast<double>(std::accumulate(time_rb.begin(), time_rb.end(), int64_t(0))) / static_cast<double>(ops_count);
        std::cout << "Producer write speed: " << avg_ns << " ns/op\n";
    }

    std::cout << "Starting queue test...\n";

    {
        std::queue<Trade> q;
        for (uint64_t i = 0; i < queue_ops_count; ++i) {
            auto start = std::chrono::high_resolution_clock::now();
            q.push(t);
            auto end = std::chrono::high_resolution_clock::now();
            time_q[i] = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        }
	    double avg_ns = static_cast<double>(std::accumulate(time_q.begin(), time_q.end(), int64_t(0))) / static_cast<double>(queue_ops_count);
        std::cout << "Queue write speed: " << avg_ns << " ns/op\n";
    }

    std::cout << "Writing results to CSV files...\n";

    {
        std::ofstream rb_out("rb_times_ns.csv");
        rb_out << "ns_per_op\n";
        for (const auto &v : time_rb) {
            rb_out << v << "\n";
        }
    
        std::ofstream q_out("queue_times_ns.csv");
        q_out << "ns_per_op\n";
        for (const auto &v : time_q) {
            q_out << v << "\n";
        }
    }

    std::cout << "Done.\n";

    return 0;
}
