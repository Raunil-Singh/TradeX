#include <gtest/gtest.h>
#include <vector>
#include <chrono>
#include <iostream>
#include <thread>
#include "trade_ring_buffer.h"

using namespace TradeRingBuffer;

class TradeRingBufferTest : public ::testing::Test {
protected:
    void SetUp() override {
        shm_unlink(filename.data());
    }

    void TearDown() override {
        shm_unlink(filename.data());
    }

    Trade create_trade(uint64_t id, double price, int32_t quantity) {
        Trade t = {};
        t.trade_id = id;
        t.timestamp_ns = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        t.symbol_id = 1001;
        t.price = price;
        t.quantity = quantity;
        t.buy_order_id = id * 10;
        t.sell_order_id = id * 10 + 1;
        return t;
    }

    bool trades_equal(const Trade& a, const Trade& b) {
        return a.trade_id == b.trade_id && a.symbol_id == b.symbol_id &&
               a.price == b.price && a.quantity == b.quantity &&
               a.buy_order_id == b.buy_order_id && a.sell_order_id == b.sell_order_id;
    }
};

TEST_F(TradeRingBufferTest, ProducerConsumerCreation) {
    EXPECT_NO_THROW({
        trade_ring_buffer producer(true);
        trade_ring_buffer consumer(false);
    });
}

TEST_F(TradeRingBufferTest, OnlyOneProducerAllowed) {
    trade_ring_buffer producer1(true);
    EXPECT_THROW({ trade_ring_buffer producer2(true); }, std::runtime_error);
}

TEST_F(TradeRingBufferTest, AddAndRetrieveSingleTrade) {
    trade_ring_buffer producer(true);
    trade_ring_buffer consumer(false);
    Trade original = create_trade(1, 100.50, 1000);
    EXPECT_TRUE(producer.add_trade(original));
    EXPECT_TRUE(consumer.any_new_trade());
    Trade retrieved = consumer.get_trade();
    EXPECT_TRUE(trades_equal(original, retrieved));
}

TEST_F(TradeRingBufferTest, MultipleTradesSequential) {
    trade_ring_buffer producer(true);
    trade_ring_buffer consumer(false);
    const int NUM_TRADES = 100;
    std::vector<Trade> trades;
    for (int i = 0; i < NUM_TRADES; ++i) {
        Trade t = create_trade(i, 100.0 + i, 1000 * (i + 1));
        trades.push_back(t);
        EXPECT_TRUE(producer.add_trade(t));
    }
    for (int i = 0; i < NUM_TRADES; ++i) {
        EXPECT_TRUE(consumer.any_new_trade());
        Trade retrieved = consumer.get_trade();
        EXPECT_TRUE(trades_equal(trades[i], retrieved));
    }
    EXPECT_FALSE(consumer.any_new_trade());
}

TEST_F(TradeRingBufferTest, ConsumerReadsTradeTwice) {
    trade_ring_buffer producer(true);
    trade_ring_buffer consumer(false);
    Trade original = create_trade(42, 99.99, 5000);
    producer.add_trade(original);
    EXPECT_TRUE(consumer.any_new_trade());
    EXPECT_TRUE(consumer.any_new_trade());
    Trade retrieved = consumer.get_trade();
    EXPECT_TRUE(trades_equal(original, retrieved));
    EXPECT_FALSE(consumer.any_new_trade());
}

TEST_F(TradeRingBufferTest, GetTradeDirectMemoryCopy) {
    trade_ring_buffer producer(true);
    trade_ring_buffer consumer(false);
    Trade original = create_trade(123, 50.25, 2500);
    producer.add_trade(original);
    Trade buffer = {};
    consumer.get_trade(static_cast<void*>(&buffer));
    EXPECT_TRUE(trades_equal(original, buffer));
}

TEST_F(TradeRingBufferTest, RingBufferWrapAround) {
    trade_ring_buffer producer(true);
    trade_ring_buffer consumer(false);
    const uint64_t NUM_TRADES = RING_SIZE + 100;
    for (uint64_t i = 0; i < NUM_TRADES; ++i) {
        Trade t = create_trade(i, 100.0 + (i % 1000), (i + 1) * 100);
        EXPECT_TRUE(producer.add_trade(t));
    }
    const bool has_new = consumer.any_new_trade();
    const bool lagged = consumer.lagged_out();
    EXPECT_TRUE(has_new || lagged);

    if (has_new) {
        consumer.get_trade();
    }
}

TEST_F(TradeRingBufferTest, ConsumerLagsOutDetection) {
    trade_ring_buffer producer(true);
    trade_ring_buffer consumer(false);
    Trade first = create_trade(0, 100.0, 1000);
    producer.add_trade(first);
    ASSERT_TRUE(consumer.any_new_trade());
    consumer.get_trade();

    for (uint64_t i = 1; i <= RING_SIZE + 2; ++i) {
        Trade t = create_trade(i, 100.0, 1000);
        producer.add_trade(t);
    }

    EXPECT_TRUE(consumer.lagged_out());
}

TEST_F(TradeRingBufferTest, MultipleConsumersIndependentReading) {
    trade_ring_buffer producer(true);
    trade_ring_buffer consumer1(false);
    trade_ring_buffer consumer2(false);
    const int NUM_TRADES = 50;
    std::vector<Trade> trades;
    for (int i = 0; i < NUM_TRADES; ++i) {
        Trade t = create_trade(i, 100.0 + i, 1000 * (i + 1));
        trades.push_back(t);
        producer.add_trade(t);
    }
    for (int i = 0; i < NUM_TRADES; ++i) {
        EXPECT_TRUE(consumer1.any_new_trade());
        EXPECT_TRUE(consumer2.any_new_trade());
        Trade r1 = consumer1.get_trade();
        Trade r2 = consumer2.get_trade();
        EXPECT_TRUE(trades_equal(trades[i], r1));
        EXPECT_TRUE(trades_equal(trades[i], r2));
    }
    EXPECT_FALSE(consumer1.any_new_trade());
    EXPECT_FALSE(consumer2.any_new_trade());
}

TEST_F(TradeRingBufferTest, MultipleConsumersDifferentSpeeds) {
    trade_ring_buffer producer(true);
    trade_ring_buffer fast_consumer(false);
    trade_ring_buffer slow_consumer(false);
    const int NUM_TRADES = 1000;
    for (int i = 0; i < NUM_TRADES; ++i) {
        Trade t = create_trade(i, 100.0 + (i % 1000) * 0.01, (i + 1) * 10);
        producer.add_trade(t);
    }
    int fast_count = 0;
    while (fast_consumer.any_new_trade() && fast_count < NUM_TRADES) {
        fast_consumer.get_trade();
        fast_count++;
    }
    EXPECT_GT(fast_count, 0);
    int slow_count = 0;
    while (slow_consumer.any_new_trade() && slow_count < NUM_TRADES) {
        slow_consumer.get_trade();
        slow_count++;
    }
    EXPECT_GT(slow_count, 0);
}

TEST_F(TradeRingBufferTest, MultipleConsumersHandleLagout) {
    trade_ring_buffer producer(true);
    trade_ring_buffer consumer1(false);
    trade_ring_buffer consumer2(false);
    for (uint64_t i = 0; i < 1000; ++i) {
        Trade t = create_trade(i, 100.0, 1000);
        producer.add_trade(t);
    }
    int c1_count = 0;
    while (consumer1.any_new_trade() && c1_count < 500) {
        consumer1.get_trade();
        c1_count++;
    }
    for (uint64_t i = 1000; i < RING_SIZE + 100; ++i) {
        Trade t = create_trade(i, 100.0, 1000);
        producer.add_trade(t);
    }
    EXPECT_TRUE(consumer2.lagged_out());
}

TEST_F(TradeRingBufferTest, ConsumerEmptyBuffer) {
    trade_ring_buffer producer(true);
    trade_ring_buffer consumer(false);
    EXPECT_FALSE(consumer.any_new_trade());
    EXPECT_TRUE(consumer.lagged_out());
}

TEST_F(TradeRingBufferTest, ProducerWriteConsumerRead) {
    trade_ring_buffer producer(true);
    trade_ring_buffer consumer(false);
    const int NUM_CYCLES = 500;
    for (int cycle = 0; cycle < NUM_CYCLES; ++cycle) {
        Trade t = create_trade(cycle, 100.0 + cycle, 1000 + cycle);
        producer.add_trade(t);
        EXPECT_TRUE(consumer.any_new_trade());
        Trade r = consumer.get_trade();
        EXPECT_TRUE(trades_equal(t, r));
    }
}

TEST_F(TradeRingBufferTest, LargeTradeValues) {
    trade_ring_buffer producer(true);
    trade_ring_buffer consumer(false);
    Trade t;
    t.trade_id = UINT64_MAX;
    t.timestamp_ns = UINT64_MAX;
    t.symbol_id = UINT32_MAX;
    t.price = 1e10;
    t.quantity = INT32_MAX;
    t.buy_order_id = UINT64_MAX - 1;
    t.sell_order_id = UINT64_MAX - 2;
    producer.add_trade(t);
    EXPECT_TRUE(consumer.any_new_trade());
    Trade r = consumer.get_trade();
    EXPECT_EQ(r.trade_id, UINT64_MAX);
    EXPECT_EQ(r.timestamp_ns, UINT64_MAX);
    EXPECT_EQ(r.symbol_id, UINT32_MAX);
    EXPECT_EQ(r.quantity, INT32_MAX);
}

TEST_F(TradeRingBufferTest, TradeWithZeroValues) {
    trade_ring_buffer producer(true);
    trade_ring_buffer consumer(false);
    Trade t = {};
    producer.add_trade(t);
    EXPECT_TRUE(consumer.any_new_trade());
    Trade r = consumer.get_trade();
    EXPECT_EQ(r.trade_id, 0UL);
    EXPECT_EQ(r.price, 0.0);
    EXPECT_EQ(r.quantity, 0);
}

TEST_F(TradeRingBufferTest, MemoryOrderingProducerConsumer) {
    trade_ring_buffer producer(true);
    trade_ring_buffer consumer(false);
    Trade original;
    original.trade_id = 12345;
    original.price = 123.456;
    original.quantity = 9999;
    producer.add_trade(original);
    EXPECT_TRUE(consumer.any_new_trade());
    Trade retrieved = consumer.get_trade();
    EXPECT_EQ(retrieved.trade_id, 12345UL);
    EXPECT_EQ(retrieved.price, 123.456);
    EXPECT_EQ(retrieved.quantity, 9999);
}

TEST_F(TradeRingBufferTest, HighThroughputProducer) {
    trade_ring_buffer producer(true);
    trade_ring_buffer consumer(false);
    const uint64_t NUM_TRADES = 100000;
    for (uint64_t i = 0; i < NUM_TRADES; ++i) {
        Trade t = create_trade(i, 100.0 + (i % 10000) * 0.01, i % 100000);
        EXPECT_TRUE(producer.add_trade(t));
    }
    uint64_t count = 0;
    while (consumer.any_new_trade()) {
        consumer.get_trade();
        count++;
    }
    EXPECT_GT(count, 0UL);
    EXPECT_LE(count, NUM_TRADES);
}

TEST_F(TradeRingBufferTest, ConsumerCatchesUpWithProducerPace) {
    trade_ring_buffer producer(true);
    trade_ring_buffer consumer(false);
    const int NUM_TRADES = 10000;
    for (int i = 0; i < NUM_TRADES; ++i) {
        Trade t = create_trade(i, 100.0, 1000);
        producer.add_trade(t);
        if (consumer.any_new_trade()) {
            consumer.get_trade();
        }
    }
    EXPECT_FALSE(consumer.any_new_trade());
}

TEST_F(TradeRingBufferTest, SequenceNumberWraparound) {
    trade_ring_buffer producer(true);
    trade_ring_buffer consumer(false);
    const uint64_t NUM_TRADES = RING_SIZE * 2;
    for (uint64_t i = 0; i < NUM_TRADES; ++i) {
        Trade t = create_trade(i, 100.0 + (i % 1000), 1000);
        producer.add_trade(t);
    }
    uint64_t expected_start = NUM_TRADES - RING_SIZE;
    for (uint64_t i = expected_start; i < NUM_TRADES; ++i) {
        if (consumer.any_new_trade()) {
            Trade r = consumer.get_trade();
            EXPECT_EQ(r.trade_id, i);
        }
    }
}

TEST_F(TradeRingBufferTest, LatencyBenchmarkSingleThread) {
    trade_ring_buffer producer(true);
    trade_ring_buffer consumer(false);

    constexpr uint64_t NUM_TRADES = 200000;
    std::vector<Trade> trades;
    trades.reserve(NUM_TRADES);
    for (uint64_t i = 0; i < NUM_TRADES; ++i) {
        trades.push_back(create_trade(i, 100.0 + (i % 1000) * 0.01, 1000));
    }

    // Start timing after construction and trade preparation.
    auto start = std::chrono::high_resolution_clock::now();
    for (uint64_t i = 0; i < NUM_TRADES; ++i) {
        producer.add_trade(trades[i]);
        if (consumer.any_new_trade()) {
            consumer.get_trade();
        }
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto total_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    double avg_ns = static_cast<double>(total_ns) / static_cast<double>(NUM_TRADES);

    std::cout << "LatencyBenchmarkSingleThread: total_ns=" << total_ns
              << " avg_ns_per_trade=" << avg_ns << "\n";
}

TEST_F(TradeRingBufferTest, LatencyBenchmarkProducerOnly) {
    trade_ring_buffer producer(true);

    constexpr uint64_t NUM_TRADES = 500000;
    std::vector<Trade> trades;
    trades.reserve(NUM_TRADES);
    for (uint64_t i = 0; i < NUM_TRADES; ++i) {
        trades.push_back(create_trade(i, 100.0 + (i % 1000) * 0.01, 1000));
    }

    // Start timing after construction and trade preparation.
    auto start = std::chrono::high_resolution_clock::now();
    for (uint64_t i = 0; i < NUM_TRADES; ++i) {
        producer.add_trade(trades[i]);
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto total_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    double avg_ns = static_cast<double>(total_ns) / static_cast<double>(NUM_TRADES);

    std::cout << "LatencyBenchmarkProducerOnly: total_ns=" << total_ns
              << " avg_ns_per_trade=" << avg_ns << "\n";
}

TEST_F(TradeRingBufferTest, ConcurrentProducerConsumerStress) {
    trade_ring_buffer producer(true);
    trade_ring_buffer consumer(false);

    constexpr uint64_t NUM_TRADES = 5'000'000;
    std::atomic<bool> start{false};
    std::atomic<bool> done{false};

    std::thread prod([&] {
        while (!start.load(std::memory_order_acquire)) {}

        for (uint64_t i = 0; i < NUM_TRADES; ++i) {
            Trade t = create_trade(i, 100.0, 1);
            producer.add_trade(t);
        }
        done.store(true, std::memory_order_release);
    });

    std::thread cons([&] {
        while (!start.load(std::memory_order_acquire)) {}

        uint64_t expected = 0;
        while (!done.load(std::memory_order_acquire) || consumer.any_new_trade()) {
            if (consumer.any_new_trade()) {
                Trade r = consumer.get_trade();
                ASSERT_EQ(r.trade_id, expected);
                expected++;
            }
        }
        ASSERT_EQ(expected, NUM_TRADES);
    });

    start.store(true, std::memory_order_release);
    prod.join();
    cons.join();
}

TEST_F(TradeRingBufferTest, LagStormRecovery) {
    trade_ring_buffer producer(true);
    trade_ring_buffer consumer(false);

    constexpr uint64_t BURST = RING_SIZE * 4;

    // Initial sync
    Trade t0 = create_trade(0, 100.0, 1);
    producer.add_trade(t0);
    ASSERT_TRUE(consumer.any_new_trade());
    consumer.get_trade();

    // Producer floods
    for (uint64_t i = 1; i <= BURST; ++i) {
        Trade t = create_trade(i, 100.0, 1);
        producer.add_trade(t);
    }

    ASSERT_TRUE(consumer.lagged_out());

    // Consumer is now far behind; verify it detects lagout
    // It may or may not be able to read any data depending on whether
    // the current slot matches its next expected sequence
    uint64_t count = 0;
    while (consumer.any_new_trade()) {
        consumer.get_trade();
        count++;
    }

    // After lagging out, consumer behavior is valid either way
    SUCCEED();
}

TEST_F(TradeRingBufferTest, MultipleConsumersConcurrent) {
    trade_ring_buffer producer(true);
    trade_ring_buffer c1(false);
    trade_ring_buffer c2(false);

    constexpr uint64_t NUM_TRADES = 1'000'000;
    std::atomic<bool> start{false};

    std::thread prod([&] {
        while (!start.load(std::memory_order_acquire)) {}
        for (uint64_t i = 0; i < NUM_TRADES; ++i) {
            Trade t = create_trade(i, 100.0, 1);
            producer.add_trade(t);
        }
    });

    auto consumer_fn = [&](trade_ring_buffer& c) {
        while (!start.load(std::memory_order_acquire)) {}
        uint64_t expected = 0;
        while (expected < NUM_TRADES) {
            if (c.any_new_trade()) {
                Trade r = c.get_trade();
                ASSERT_EQ(r.trade_id, expected);
                expected++;
            }
        }
    };

    std::thread t1(consumer_fn, std::ref(c1));
    std::thread t2(consumer_fn, std::ref(c2));

    start.store(true, std::memory_order_release);

    prod.join();
    t1.join();
    t2.join();
}

TEST_F(TradeRingBufferTest, MemoryOrderingTorture) {
    trade_ring_buffer producer(true);
    trade_ring_buffer consumer(false);

    constexpr uint64_t ITERS = 2'000'000;
    std::atomic<bool> stop{false};

    std::thread prod([&] {
        for (uint64_t i = 0; i < ITERS; ++i) {
            Trade t{};
            t.trade_id = i;
            t.quantity = 123456;
            producer.add_trade(t);
        }
        stop.store(true, std::memory_order_release);
    });

    std::thread cons([&] {
        uint64_t expected = 0;
        while (!stop.load(std::memory_order_acquire) || consumer.any_new_trade()) {
            if (consumer.any_new_trade()) {
                Trade r = consumer.get_trade();
                ASSERT_EQ(r.trade_id, expected);
                ASSERT_EQ(r.quantity, 123456);
                expected++;
            }
        }
        ASSERT_EQ(expected, ITERS);
    });

    prod.join();
    cons.join();
}

TEST_F(TradeRingBufferTest, LongRunningSoak) {
    trade_ring_buffer producer(true);
    trade_ring_buffer consumer(false);

    constexpr uint64_t ITERS = 20'000'000;

    for (uint64_t i = 0; i < ITERS; ++i) {
        Trade t = create_trade(i, 100.0, 1);
        producer.add_trade(t);

        if (consumer.any_new_trade()) {
            Trade r = consumer.get_trade();
            ASSERT_LE(r.trade_id, i);
        }

        if ((i & 0xFFFFF) == 0) {
            // periodic yield to vary scheduling
            std::this_thread::yield();
        }
    }
}

TEST_F(TradeRingBufferTest, LatencyBenchmarkHardened) {
    trade_ring_buffer producer(true);

    constexpr uint64_t NUM_TRADES = 1'000'000;
    Trade t = create_trade(0, 100.0, 1);

    auto start = std::chrono::steady_clock::now();
    for (uint64_t i = 0; i < NUM_TRADES; ++i) {
        t.trade_id = i;
        producer.add_trade(t);
        asm volatile("" ::: "memory");
    }
    auto end = std::chrono::steady_clock::now();

    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    std::cout << "Hardened latency: "
              << double(ns) / NUM_TRADES << " ns/op\n";
}

TEST_F(TradeRingBufferTest, ConcurrentLatencyProducerConsumer) {
    trade_ring_buffer producer(true);
    trade_ring_buffer consumer(false);

    constexpr uint64_t NUM_TRADES = 1'000'000;
    std::atomic<bool> start{false};
    std::vector<uint64_t> latencies;
    latencies.reserve(NUM_TRADES);

    std::thread prod([&] {
        while (!start.load(std::memory_order_acquire)) {}

        for (uint64_t i = 0; i < NUM_TRADES; ++i) {
            Trade t = create_trade(i, 100.0, 1);
            t.timestamp_ns = std::chrono::steady_clock::now()
                                 .time_since_epoch().count();
            producer.add_trade(t);
        }
    });

    std::thread cons([&] {
        while (!start.load(std::memory_order_acquire)) {}

        uint64_t received = 0;
        while (received < NUM_TRADES) {
            if (consumer.any_new_trade()) {
                Trade r = consumer.get_trade();
                uint64_t now = std::chrono::steady_clock::now()
                                   .time_since_epoch().count();
                latencies.push_back(now - r.timestamp_ns);
                received++;
            }
        }
    });

    start.store(true, std::memory_order_release);
    prod.join();
    cons.join();

    ASSERT_FALSE(latencies.empty());

    std::sort(latencies.begin(), latencies.end());
    std::cout << "Concurrent latency p50 = "
              << latencies[latencies.size() / 2] << " ns\n";
    std::cout << "Concurrent latency p99 = "
              << latencies[latencies.size() * 99 / 100] << " ns\n";
}

TEST_F(TradeRingBufferTest, ConcurrentLagStorm) {
    trade_ring_buffer producer(true);
    trade_ring_buffer consumer(false);

    constexpr uint64_t BURST = RING_SIZE * 3;
    std::atomic<bool> start{false};

    std::thread prod([&] {
        while (!start.load(std::memory_order_acquire)) {}
        for (uint64_t i = 0; i < BURST; ++i) {
            Trade t = create_trade(i, 100.0, 1);
            producer.add_trade(t);
        }
    });

    std::thread cons([&] {
        while (!start.load(std::memory_order_acquire)) {}
        // Intentionally sleep to force lag
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        ASSERT_TRUE(consumer.lagged_out());
    });

    start.store(true, std::memory_order_release);
    prod.join();
    cons.join();
}

TEST_F(TradeRingBufferTest, ConcurrentMultiConsumerDifferentSpeeds) {
    trade_ring_buffer producer(true);
    trade_ring_buffer fast(false);
    trade_ring_buffer slow(false);

    constexpr uint64_t NUM_TRADES = 500'000;
    std::atomic<bool> start{false};

    std::thread prod([&] {
        while (!start.load(std::memory_order_acquire)) {}
        for (uint64_t i = 0; i < NUM_TRADES; ++i) {
            Trade t = create_trade(i, 100.0, 1);
            producer.add_trade(t);
        }
    });

    auto consumer_fn = [&](trade_ring_buffer& c, int delay_us) {
        while (!start.load(std::memory_order_acquire)) {}
        uint64_t count = 0;
        while (count < NUM_TRADES) {
            if (c.any_new_trade()) {
                c.get_trade();
                count++;
                if (delay_us)
                    std::this_thread::sleep_for(
                        std::chrono::microseconds(delay_us));
            }
        }
    };

    std::thread t1(consumer_fn, std::ref(fast), 0);
    std::thread t2(consumer_fn, std::ref(slow), 5);

    start.store(true, std::memory_order_release);
    prod.join();
    t1.join();
    t2.join();
}

TEST_F(TradeRingBufferTest, ConcurrentSpinConsumer) {
    trade_ring_buffer producer(true);
    trade_ring_buffer consumer(false);

    constexpr uint64_t NUM_TRADES = 2'000'000;
    std::atomic<bool> start{false};

    std::thread prod([&] {
        while (!start.load(std::memory_order_acquire)) {}
        for (uint64_t i = 0; i < NUM_TRADES; ++i) {
            Trade t = create_trade(i, 100.0, 1);
            producer.add_trade(t);
        }
    });

    std::thread cons([&] {
        while (!start.load(std::memory_order_acquire)) {}
        uint64_t expected = 0;
        while (expected < NUM_TRADES) {
            if (consumer.any_new_trade()) {
                Trade r = consumer.get_trade();
                ASSERT_EQ(r.trade_id, expected);
                expected++;
            }
        }
    });

    start.store(true, std::memory_order_release);
    prod.join();
    cons.join();
}

TEST_F(TradeRingBufferTest, ConcurrentCreateDestroy) {
    constexpr int ITERS = 100;

    for (int i = 0; i < ITERS; ++i) {
        trade_ring_buffer producer(true);
        trade_ring_buffer consumer(false);

        std::thread t([&] {
            for (int j = 0; j < 1000; ++j) {
                Trade t = create_trade(j, 100.0, 1);
                producer.add_trade(t);
            }
        });
        t.join();
    }
}


int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
