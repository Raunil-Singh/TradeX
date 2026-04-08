#include "oms.h"
#include "oms_order_receiving_server.h"
#include "symbol_loader.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cassert>
#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>
#include <vector>

namespace {

constexpr uint16_t kTestPort = 9101;

const SymbolInfo* find_symbol(const std::vector<SymbolInfo>& symbols, const char* symbol) {
    for (const auto& s : symbols) {
        if (s.symbol == symbol) {
            return &s;
        }
    }
    return nullptr;
}

uint64_t apply_percent_offset(uint64_t base, int numerator, int denominator = 100) {
    return (base * static_cast<uint64_t>(numerator)) / static_cast<uint64_t>(denominator);
}

oms::ClientOrder make_limit_order(const char* symbol,
                                  matching_engine::OrderType side,
                                  uint32_t quantity,
                                  uint64_t price,
                                  uint64_t trader_id) {
    oms::ClientOrder order{};
    std::strncpy(order.symbol, symbol, sizeof(order.symbol) - 1);
    order.type = side;
    order.quantity = quantity;
    order.price = price;
    order.trader_id = trader_id;
    order.execution_type = oms::ClientOrderType::LIMIT;
    order.is_active = true;
    return order;
}

int connect_with_retry(uint16_t port, int retries = 100, int sleep_ms = 10) {
    for (int i = 0; i < retries; ++i) {
        int fd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
            continue;
        }

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

        if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0) {
            return fd;
        }

        ::close(fd);
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
    }

    return -1;
}

bool send_all(int fd, const void* data, std::size_t size) {
    const char* ptr = static_cast<const char*>(data);
    std::size_t sent = 0;

    while (sent < size) {
        ssize_t n = ::send(fd, ptr + sent, size - sent, 0);
        if (n <= 0) {
            return false;
        }
        sent += static_cast<std::size_t>(n);
    }

    return true;
}

bool send_order_in_chunks(int fd, const oms::ClientOrder& order) {
    const char* raw = reinterpret_cast<const char*>(&order);
    constexpr std::size_t kChunkPattern[] = {7, 5, 11, 3, 13};

    std::size_t offset = 0;
    std::size_t chunk_index = 0;
    while (offset < sizeof(order)) {
        std::size_t chunk = kChunkPattern[chunk_index % (sizeof(kChunkPattern) / sizeof(kChunkPattern[0]))];
        std::size_t rem = sizeof(order) - offset;
        if (chunk > rem) chunk = rem;

        if (!send_all(fd, raw + offset, chunk)) {
            return false;
        }

        offset += chunk;
        ++chunk_index;

        // Force packet fragmentation behavior to exercise readExact().
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return true;
}

bool wait_for_ltp(shared_data::MarketState* market_state,
                  uint32_t symbol_id,
                  uint64_t expected_ltp,
                  std::chrono::milliseconds timeout) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    const uint32_t idx = symbol_id & matching_engine::SYMBOL_MASK;

    while (std::chrono::steady_clock::now() < deadline) {
        uint64_t ltp = market_state->last_price[idx].load(std::memory_order_relaxed);
        if (ltp == expected_ltp) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    return false;
}

} // namespace

int main() {
    std::cout << "--- Order Receiving Server Integration Test (3 Senders) ---\n";

    matching_engine::MatchingEngineDispatcher engine(1024);

    uint64_t lower[TOTAL_SYMBOLS] = {0};
    uint64_t upper[TOTAL_SYMBOLS] = {0};

    std::vector<SymbolInfo> symbols = loadSymbolCSV("data/symbols.csv");
    assert(!symbols.empty());

    const SymbolInfo* aapl = find_symbol(symbols, "AAPL");
    const SymbolInfo* goog = find_symbol(symbols, "GOOG");
    assert(aapl != nullptr);
    assert(goog != nullptr);

    const uint64_t aapl_trade_price = apply_percent_offset(aapl->ltp, 101); // +1%
    const uint64_t goog_trade_price = apply_percent_offset(goog->ltp, 101); // +1%

    for (const auto& s : symbols) {
        lower[s.symbol_id] = s.lower_limit;
        upper[s.symbol_id] = s.upper_limit;
    }

    engine.initialize_engine(lower, upper);

    oms::OrderManagementSystem oms_system(&engine, 4096);
    oms::OrderReceivingServer server(oms_system, kTestPort);

    std::thread engine_thread([&engine]() { engine.start(); });
    oms_system.start();
    server.start();

    // Sender 1: provides AAPL asks and GOOG bids.
    auto sender1 = [aapl_trade_price, goog_trade_price]() {
        int fd = connect_with_retry(kTestPort);
        assert(fd >= 0);

        std::vector<oms::ClientOrder> orders;
        orders.push_back(make_limit_order("AAPL", matching_engine::OrderType::SELL, 200, aapl_trade_price, 1001));
        orders.push_back(make_limit_order("GOOG", matching_engine::OrderType::BUY, 60, goog_trade_price, 1001));

        for (const auto& ord : orders) {
            assert(send_order_in_chunks(fd, ord));
        }

        ::close(fd);
    };

    // Sender 2: provides AAPL bids and GOOG asks.
    auto sender2 = [aapl_trade_price, goog_trade_price]() {
        int fd = connect_with_retry(kTestPort);
        assert(fd >= 0);

        std::vector<oms::ClientOrder> orders;
        orders.push_back(make_limit_order("AAPL", matching_engine::OrderType::BUY, 100, aapl_trade_price, 1002));
        orders.push_back(make_limit_order("GOOG", matching_engine::OrderType::SELL, 120, goog_trade_price, 1002));

        for (const auto& ord : orders) {
            assert(send_order_in_chunks(fd, ord));
        }

        ::close(fd);
    };

    // Sender 3: complements the resting quantity to complete both symbols.
    auto sender3 = [aapl_trade_price, goog_trade_price]() {
        int fd = connect_with_retry(kTestPort);
        assert(fd >= 0);

        std::vector<oms::ClientOrder> orders;
        orders.push_back(make_limit_order("AAPL", matching_engine::OrderType::BUY, 100, aapl_trade_price, 1003));
        orders.push_back(make_limit_order("GOOG", matching_engine::OrderType::BUY, 60, goog_trade_price, 1003));

        for (const auto& ord : orders) {
            assert(send_order_in_chunks(fd, ord));
        }

        ::close(fd);
    };

    std::thread t1(sender1);
    std::thread t2(sender2);
    std::thread t3(sender3);

    t1.join();
    t2.join();
    t3.join();

    assert(oms_system.shared_memory_ptr != nullptr);

    // Validate that both symbols traded at expected prices.
    bool aapl_ok = wait_for_ltp(oms_system.shared_memory_ptr, aapl->symbol_id, aapl_trade_price, std::chrono::milliseconds(1500));
    bool goog_ok = wait_for_ltp(oms_system.shared_memory_ptr, goog->symbol_id, goog_trade_price, std::chrono::milliseconds(1500));

    if (!aapl_ok || !goog_ok) {
        std::cerr << "Expected LTP updates not observed. AAPL=" << aapl_ok << " GOOG=" << goog_ok << "\n";
    }

    assert(aapl_ok);
    assert(goog_ok);

    server.stop();
    engine.terminate();
    oms_system.stop();

    if (engine_thread.joinable()) {
        engine_thread.join();
    }

    saveEndOfDayCSV("data/symbols.csv", oms_system.shared_memory_ptr, symbols);

    std::cout << "Order receiving server integration test passed.\n";
    return 0;
}
