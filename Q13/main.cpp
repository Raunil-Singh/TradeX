#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <map>
#include <random>
#include <chrono>
#include <atomic>

const int ORDER_COUNT = 5'000'000;
const int THREAD_COUNT = 4;
const int PRICE_LEVELS = 200;

struct Order
{
    double price;
    int quantity;
};


template <typename F>
long long measure(F f)
{
    auto start = std::chrono::high_resolution_clock::now();
    f();
    auto end = std::chrono::high_resolution_clock::now();

    return std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start).count();
}

double calcLevel(int order_price) {
    return std::floor((order_price - 99.0) * 100.0) / 100.0 + 99.0;
}

std::vector<Order> generateOrders()
{
    std::vector<Order> orders;
    orders.reserve(ORDER_COUNT);

    std::mt19937 mt(std::random_device{}());
    std::uniform_real_distribution<double> price_dist(99.0, 101.0);
    std::uniform_int_distribution<int> qty_dist(1, 1000);

    for (int i = 0; i < ORDER_COUNT; i++)
    {
        orders.push_back({ price_dist(mt), qty_dist(mt) });
    }

    return orders;
}

long long runV1(const std::vector<Order>& orders, std::map<double, long long>& book)
{
    auto time = measure([&] {
        for (const auto& order : orders)
        {
            double level = calcLevel(order.price);

            book[level] += order.quantity;
        }
    });

    return time;
}



void workerV2(const std::vector<Order>& orders, int start, int end, std::map<double, long long>& local_book)
{
    for (int i = start; i < end; i++)
    {
        double level = calcLevel(orders[i].price);

        local_book[level] += orders[i].quantity;
    }
}

long long runV2(const std::vector<Order>& orders, std::map<double, long long>& global_book)
{
    std::vector<std::thread> threads;
    std::vector<std::map<double, long long>> local_books(THREAD_COUNT);

    int section = orders.size() / THREAD_COUNT;

    auto time = measure([&] {
        for (int i = 0; i < THREAD_COUNT; i++)
        {
            int start = i * section;
            int end = (i == THREAD_COUNT - 1)
                      ? orders.size()
                      : start + section;

            threads.emplace_back(workerV2, std::ref(orders), start, end, std::ref(local_books[i]));
        }

        for (auto& t : threads)
            t.join();

        for (auto& local : local_books)
        {
            for (auto& [price, qty] : local)
            {
                global_book[price] += qty;
            }
        }
    });

    return time;
}

inline int priceToIndex(double price)
{
    return static_cast<int>((price - 99.0) * 100.0);
}

alignas(64) std::atomic<long long> book_atomic[PRICE_LEVELS];

void workerV3(const std::vector<Order>& orders, int start, int end, long long* local_book)
{
    for (int i = start; i < end; i++)
    {
        int idx = priceToIndex(orders[i].price);
        local_book[idx] += orders[i].quantity;
    }
}

long long runV3(const std::vector<Order>& orders)
{
    std::vector<std::thread> threads;
    std::vector<std::vector<long long>> locals(THREAD_COUNT, std::vector<long long>(PRICE_LEVELS, 0));

    int section = orders.size() / THREAD_COUNT;

    auto time = measure([&] {
        for (int i = 0; i < THREAD_COUNT; i++)
        {
            int start = i * section;
            int end = (i == THREAD_COUNT - 1)
                      ? orders.size()
                      : start + section;

            threads.emplace_back(workerV3, std::ref(orders), start, end, locals[i].data());
        }

        for (auto& t : threads)
            t.join();

        for (int t = 0; t < THREAD_COUNT; t++)
        {
            for (int i = 0; i < PRICE_LEVELS; i++)
            {
                book_atomic[i].fetch_add(locals[t][i], std::memory_order_relaxed);
            }
        }
    });

    return time;
}


long long sumOrders(const std::vector<Order>& orders)
{
    long long total = 0;
    for (const auto& order : orders)
        total += order.quantity;
    return total;
}

long long sumBook()
{
    long long total = 0;
    for (int i = 0; i < PRICE_LEVELS; i++)
        total += book_atomic[i].load();
    return total;
}


int main()
{
    auto orders = generateOrders();

    std::map<double, long long> bookV1;
    long long t1 = runV1(orders, bookV1);

    std::map<double, long long> bookV2;
    long long t2 = runV2(orders, bookV2);

    for (int i = 0; i < PRICE_LEVELS; i++)
        book_atomic[i] = 0;

    long long t3 = runV3(orders);

    long long expected = sumOrders(orders);
    long long actual = sumBook();

    std::cout << "\nResults:\n\n";
    std::cout << "V1 (map, single): " << t1 << " ms\n";
    std::cout << "V2 (map, parallel): " << t2 << " ms\n";
    std::cout << "V3 (array, atomic): " << t3 << " ms\n\n";

    std::cout << "Verification: "
              << (expected == actual ? "OK" : "FAIL")
              << "\n\n";
}
