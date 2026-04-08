#include "MarketFeedReader.h"
#include "retransmitter.h"
#include "listener.h"
#include <thread>
#include <vector>

// 1. Change: Start with flag = true, pusher sets it to false when done.
std::atomic_bool flag{true}; 
void pusher();
matching_engine::Trade make_fake_trade(uint64_t i)
{
    matching_engine::Trade t;
    t.price        = 100.0 + (i % 100);
    t.quantity     = 10 + (i % 50);
    t.symbol_id    = i % 8;
    t.timestamp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    t.trade_id     = i;
    return t;
}
int main() {
    // 2. Startup Sequence: Objects must be created in order of "Server" to "Client"
    // because your constructors contain blocking connect() calls.

    // A. MFR starts first (it is a TCP Server for RT)
    MarketFeedReader mfr(flag); 
    std::cout << "[Test] MFR Initialized and listening on 8000...\n";

    // B. Retransmitter starts (it is a TCP Client to MFR, and Server for Listener)
    Retransmitter rt(flag);
    std::cout << "[Test] RT Connected to MFR and listening on 8080...\n";

    // C. Listener starts (it is a TCP Client to RT)
    Client::Listener listener;
    std::cout << "[Test] Listener Connected to RT and joined Multicast...\n";

    // 3. Launch Threads
    std::vector<std::thread> threads;

    // MFR Threads
    threads.emplace_back([&] { mfr.readThread(); });
    threads.emplace_back([&] { mfr.sendThread(); });

    // RT Threads
    threads.emplace_back([&] { rt.storeThread(); });
    threads.emplace_back([&] { rt.listenerThread(); });

    // Listener Threads
    threads.emplace_back([&] { listener.listener(); });
    threads.emplace_back([&] { listener.missing_packet_request(); });

    // 4. Run the Pusher
    // We run this in a separate thread so we don't block main
    std::thread pusher_thread(pusher);

    pusher_thread.join();
    
    // 5. Cleanup: Give the TCP buffers a second to drain before joining
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    std::cout << "[Test] All data pushed. Shutting down...\n";
    // In a real test, you'd implement a proper shutdown for the 'while(true)' loops
    // For now, main will exit and the threads will be destroyed.
    for (auto& t : threads) {
        if (t.joinable()) t.detach(); 
    }
    return 0;
}

// Fixed Pusher Logic
void pusher()
{
    std::array<TradeRingBuffer::trade_ring_buffer*, TradeRingBuffer::total_ring_buffer_count> trb; 
    for (int i = 0; i < TradeRingBuffer::total_ring_buffer_count; i++)
    {
        trb[i] = new TradeRingBuffer::trade_ring_buffer(true, i);
    }

    matching_engine::Trade trade;
    // We'll push 1,000,000 trades
    for (int i = 0; i < 1000000; i++)
    {
        // FIX: The inner loop should use 'j' for condition
        for (int j = 0; j < TradeRingBuffer::total_ring_buffer_count; j++)
        {
            trade = make_fake_trade(i);
            trb[j]->add_trade(trade);
        }
    }
    
    std::cout << "[Pusher] 1,000,000 trades pushed to ring buffers.\n";
    
    // FIX: Set flag to false so MFR readThread knows to stop
    flag.store(false);
}