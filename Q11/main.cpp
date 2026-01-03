#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <thread>
#include <algorithm>

double THRESHHOLD = 400.0;
int ITERATIONS = 1000;
int ORDER_NUM = 1000000;
int MIN_QTY = 75;

struct OrderAoS
{
    int id;
    double price;
    int qty;
    char side;
};

struct OrderSoA
{
    std::vector<int> id;
    std::vector<double> price;
    std::vector<int> qty;
    std::vector<char> side;
};

struct alignas(64) ThreadResult
{
    int scan_time;
    int update_time;
    int filter_time;
};

void initAoS(std::vector<OrderAoS> &order)
{
    thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<size_t> q(0, 100);
    std::uniform_real_distribution<double> p(100.0, 500.0);
    std::uniform_int_distribution<int> dist('a', 'z');

    for (int i = 0; i < ORDER_NUM; i++)
    {
        order[i].id = i + 1;
        order[i].price = p(rng);
        order[i].qty = q(rng);
        order[i].side = static_cast<char>(dist(rng));
    }
}

void scanAoS(std::vector<OrderAoS> &order, int start, int end)
{
    volatile int sink = 0;
    for (int i = 0; i < ITERATIONS; i++)
    {
        int cnt = 0;
        for (int j = start; j < end; j++)
        {
            if (order[j].price > THRESHHOLD)
                cnt++;
        }
        sink+=cnt;
    }
}

void updateAoS(std::vector<OrderAoS> &order, int start, int end)
{
    for (int i = 0; i < ITERATIONS; i++)
    {
        for (int j = start; j < end; j++)
        {
            order[j].price *= 1.01;
        }
    }
}

void filterAoS(std::vector<OrderAoS> &order, int start, int end)
{
    volatile int sink = 0;
    for (int i = 0; i < ITERATIONS; i++)
    {
        int cnt = 0;
        for (int j = start; j < end; j++)
        {
            if (order[j].price > THRESHHOLD && order[j].qty > MIN_QTY)
                cnt++;
        }
        sink+=cnt;
    }
}

void runAoS(std::vector<OrderAoS> &order, int t, ThreadResult &result)
{
    int start = t * (ORDER_NUM / 4);
    int end = (t + 1) * (ORDER_NUM / 4);

    auto start_scan = std::chrono::high_resolution_clock::now();
    scanAoS(order, start, end);
    auto stop_scan = std::chrono::high_resolution_clock::now();
    result.scan_time = std::chrono::duration_cast<std::chrono::milliseconds>(stop_scan - start_scan).count();

    auto start_update = std::chrono::high_resolution_clock::now();
    updateAoS(order, start, end);
    auto stop_update = std::chrono::high_resolution_clock::now();
    result.update_time = std::chrono::duration_cast<std::chrono::milliseconds>(stop_update - start_update).count();

    auto start_filter = std::chrono::high_resolution_clock::now();
    filterAoS(order, start, end);
    auto stop_filter = std::chrono::high_resolution_clock::now();
    result.filter_time = std::chrono::duration_cast<std::chrono::milliseconds>(stop_filter - start_filter).count();
}

void initSoA(OrderSoA &order)
{
    thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<size_t> q(0, 100);
    std::uniform_real_distribution<double> p(100.0, 500.0);
    std::uniform_int_distribution<int> dist('a', 'z');

    for (int i = 0; i < ORDER_NUM; i++)
    {
        order.id[i] = i + 1;
        order.price[i] = p(rng);
        order.qty[i] = q(rng);
        order.side[i] = static_cast<char>(dist(rng));
    }
}

void scanSoA(OrderSoA &order, int start, int end)
{
    volatile int sink = 0;
    for (int i = 0; i < ITERATIONS; i++)
    {
        int cnt = 0;
        for (int j = start; j < end; j++)
        {
            if (order.price[j] > THRESHHOLD)
                cnt++;
        }
        sink+=cnt;
    }
}

void updateSoA(OrderSoA &order, int start, int end)
{
    
    for (int i = 0; i < ITERATIONS; i++)
    {
        for (int j = start; j < end; j++)
        {
            order.price[j] *= 1.01;
        }
    }
}

void filterSoA(OrderSoA &order, int start, int end)
{
    volatile int sink = 0;
    for (int i = 0; i < ITERATIONS; i++)
    {
        int cnt = 0;
        for (int j = start; j < end; j++)
        {
            if (order.price[j] > THRESHHOLD && order.qty[j] > MIN_QTY)
                cnt++;
        }
        sink+=cnt;
    }
    
}

void runSoA(OrderSoA &order, int t, ThreadResult &result)
{
    int start = t * (ORDER_NUM / 4);
    int end = (t + 1) * (ORDER_NUM / 4);

    auto start_scan = std::chrono::high_resolution_clock::now();
    scanSoA(order, start, end);
    auto stop_scan = std::chrono::high_resolution_clock::now();
    result.scan_time = std::chrono::duration_cast<std::chrono::milliseconds>(stop_scan - start_scan).count();

    auto start_update = std::chrono::high_resolution_clock::now();
    updateSoA(order, start, end);
    auto stop_update = std::chrono::high_resolution_clock::now();
    result.update_time = std::chrono::duration_cast<std::chrono::milliseconds>(stop_update - start_update).count();

    auto start_filter = std::chrono::high_resolution_clock::now();
    filterSoA(order, start, end);
    auto stop_filter = std::chrono::high_resolution_clock::now();
    result.filter_time = std::chrono::duration_cast<std::chrono::milliseconds>(stop_filter - start_filter).count();
}

int main()
{
    // Array of Structure
    std::vector<OrderAoS> order(ORDER_NUM);
    initAoS(order);

    ThreadResult r1, r2, r3, r4;

    auto startAoS = std::chrono::high_resolution_clock::now();

    std::thread t1AoS(runAoS, std::ref(order), 0, std::ref(r1));
    std::thread t2AoS(runAoS, std::ref(order), 1, std::ref(r2));
    std::thread t3AoS(runAoS, std::ref(order), 2, std::ref(r3));
    std::thread t4AoS(runAoS, std::ref(order), 3, std::ref(r4));

    t1AoS.join();
    t2AoS.join();
    t3AoS.join();
    t4AoS.join();

    auto endAoS = std::chrono::high_resolution_clock::now();
    int durationAoS = std::chrono::duration_cast<std::chrono::milliseconds>(endAoS - startAoS).count();
    int scan = std::max({r1.scan_time, r2.scan_time, r3.scan_time, r4.scan_time});
    int update = std::max({r1.update_time, r2.update_time, r3.update_time, r4.update_time});
    int filter = std::max({r1.filter_time, r2.filter_time, r3.filter_time, r4.filter_time});
    std::cout << "Execution time of Array of Structures : " << durationAoS << "ms\n";
    std::cout << " Scanning Time : " << scan << "ms | Updating Time : " << update << "ms | Filtering Time : " << filter << "ms |\n";

    // Structure of Arrays
    OrderSoA orderSoA;
    orderSoA.id.resize(ORDER_NUM);
    orderSoA.price.resize(ORDER_NUM);
    orderSoA.qty.resize(ORDER_NUM);
    orderSoA.side.resize(ORDER_NUM);
    initSoA(orderSoA);

    ThreadResult R1, R2, R3, R4;

    auto startSoA = std::chrono::high_resolution_clock::now();

    std::thread t1SoA(runSoA, std::ref(orderSoA), 0, std::ref(R1));
    std::thread t2SoA(runSoA, std::ref(orderSoA), 1, std::ref(R2));
    std::thread t3SoA(runSoA, std::ref(orderSoA), 2, std::ref(R3));
    std::thread t4SoA(runSoA, std::ref(orderSoA), 3, std::ref(R4));

    t1SoA.join();
    t2SoA.join();
    t3SoA.join();
    t4SoA.join();

    auto endSoA = std::chrono::high_resolution_clock::now();
    int durationSoA = std::chrono::duration_cast<std::chrono::milliseconds>(endSoA - startSoA).count();
    int SCAN = std::max({R1.scan_time, R2.scan_time, R3.scan_time, R4.scan_time});
    int UPDATE = std::max({R1.update_time, R2.update_time, R3.update_time, R4.update_time});
    int FILTER = std::max({R1.filter_time, R2.filter_time, R3.filter_time, R4.filter_time});
    std::cout << "Execution time of Structure of Arrays : " << durationSoA << "ms\n";
    std::cout << " Scanning Time : " << SCAN << "ms | Updating Time : " << UPDATE << "ms | Filtering Time : " << FILTER << "ms |\n";
}