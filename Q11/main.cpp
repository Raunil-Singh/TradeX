#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <random>

const int ORDER_COUNT = 1'000'000;
const int THREAD_COUNT = 4;
const int ITERATIONS = 1'000;


template <typename F>
long long measure(F f)
{
    auto start = std::chrono::high_resolution_clock::now();
    f();
    auto end = std::chrono::high_resolution_clock::now();

    return std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start).count();
}


struct Order
{
    int id;
    double price;
    int quantity;
    char side;
};

void initAOS(std::vector<Order>& orders)
{
    orders.resize(ORDER_COUNT);

    for (int i = 0; i < ORDER_COUNT; ++i)
    {
        orders[i] = { i, 100.0 + (i % 100), i % 500, (i % 2 == 0) ? 'B' : 'S' };
    }
}

struct OrdersSOA
{
    std::vector<int> ids;
    std::vector<double> prices;
    std::vector<int> quantities;
    std::vector<char> sides;
};

void initSOA(OrdersSOA& data)
{
    data.ids.resize(ORDER_COUNT);
    data.prices.resize(ORDER_COUNT);
    data.quantities.resize(ORDER_COUNT);
    data.sides.resize(ORDER_COUNT);

    for (int i = 0; i < ORDER_COUNT; ++i)
    {
        data.ids[i] = i;
        data.prices[i] = 100.0 + (i % 100);
        data.quantities[i] = i % 500;
        data.sides[i] = (i % 2 == 0) ? 'B' : 'S';
    }
}



void scanAOS(const std::vector<Order>& orders, double threshold)
{
    int count = 0;

    for (int i = 0; i < ORDER_COUNT; ++i)
    {
        if (orders[i].price > threshold)
            count++;
    }
}

void updateAOS(std::vector<Order>& orders)
{
    for (int i = 0; i < ORDER_COUNT; ++i)
    {
        orders[i].price *= 1.01;
    }
}

void filterAOS(const std::vector<Order>& orders, double price_limit, int qty_limit)
{
    int count = 0;

    for (int i = 0; i < ORDER_COUNT; ++i)
    {
        if (orders[i].price > price_limit &&
            orders[i].quantity > qty_limit)
        {
            count++;
        }
    }
}

void scanSOA(const OrdersSOA& data, double threshold)
{
    int count = 0;

    for (int i = 0; i < ORDER_COUNT; ++i)
    {
        if (data.prices[i] > threshold)
            count++;
    }
}

void updateSOA(OrdersSOA& data)
{
    for (int i = 0; i < ORDER_COUNT; ++i)
    {
        data.prices[i] *= 1.01;
    }
}

void filterSOA(const OrdersSOA& data,
                double price_limit,
                int qty_limit)
{
    int count = 0;

    for (int i = 0; i < ORDER_COUNT; ++i)
    {
        if (data.prices[i] > price_limit &&
            data.quantities[i] > qty_limit)
        {
            count++;
        }
    }
}

template <typename F>
long long runParallel(F f)
{
    std::vector<std::thread> threads;

    auto time = measure([&] {
        for (int i = 0; i < THREAD_COUNT; ++i)
        {
            threads.emplace_back([&] {
                for (int j = 0; j < ITERATIONS; ++j)
                    f();
            });
        }

        for (auto& t : threads)
            t.join();
    });

    return time;
}


int main()
{
    std::vector<Order> orders;
    OrdersSOA soa;

    initAOS(orders);
    initSOA(soa);

    double threshold = 150.0;
    int qty_limit = 250;

    std::cout << "\nAoS layout:\n";

    std::cout << "Scan:   " << runParallel([&] { scanAOS(orders, threshold); }) << " ms\n";

    std::cout << "Update: " << runParallel([&] { updateAOS(orders); }) << " ms\n";

    std::cout << "Filter: " << runParallel([&] { filterAOS(orders, threshold, qty_limit); }) << " ms\n\n";

    std::cout << "SoA layout:\n";

    std::cout << "Scan:   " << runParallel([&] { scanSOA(soa, threshold); }) << " ms\n";

    std::cout << "Update: " << runParallel([&] { updateSOA(soa); }) << " ms\n";

    std::cout << "Filter: " << runParallel([&] { filterSOA(soa, threshold, qty_limit); }) << " ms\n\n";
}
