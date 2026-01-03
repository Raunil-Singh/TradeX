#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <chrono>
#include <random>
#include <map>
#include <atomic>

const int ORDER_NUM = 5000000;
const int PRICE_NUM = 200;

struct Order
{
    bool type;
    double price;
    int qty;
};

void order_init(std::vector<Order> &order)
{
    std::mt19937 rng(42);
    std::uniform_int_distribution<size_t> q(1, 1000);
    std::uniform_int_distribution<size_t> b(0, 1);
    std::uniform_real_distribution<double> p(99.00, 101.00);

    for (int i = 0; i < ORDER_NUM; i++)
    {
        order[i].type = b(rng) ? true : false;
        order[i].price = p(rng);
        order[i].qty = q(rng);
    }
}

void sequential_processing(std::vector<Order> &orders, std::map<double, int> &m)
{
    for (int i = 0; i < ORDER_NUM; i++)
    {
        m[orders[i].price] += orders[i].qty;
    }
}

void run_version1()
{
    std::vector<Order> orders(ORDER_NUM);
    order_init(orders);
    std::map<double, int> m;

    int avg_dur = 0;
    volatile int sink = 0;
    for (int i = 0; i < 3; i++)
    {
        std::map<double, int> M;
        auto start = std::chrono::high_resolution_clock::now();
        sequential_processing(orders, M);
        auto end = std::chrono::high_resolution_clock::now();
        int duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        avg_dur += duration;
        sink += M[0];
        if (i == 0)
            m = M;
    }
    int duration = avg_dur / 3;

    size_t size = m.size();

    long long s1 = 0, s2 = 0;
    for (int i = 0; i < ORDER_NUM; i++)
    {
        s1 += orders[i].qty;
    }
    for (const auto &pair : m)
    {
        s2 += pair.second;
    }

    std::cout << "Execution Time : " << duration << "ms\n";
    std::cout << "Throughput : " << ORDER_NUM * 1000.0 / duration << " orders/s\n";
    if (s1 == s2)
    {
        std::cout << "Verification success!!!!\n";
    }
    else
    {
        std::cout << "Verification failed!!\n";
    }
}

void parallel_processing(std::vector<Order> &orders, int start, int end, std::map<double, int> &m)
{
    for (int i = start; i < end; i++)
    {
        m[orders[i].price] += orders[i].qty;
    }
}

void run_version2()
{
    int NUM_THREADS = 4;
    std::vector<Order> orders(ORDER_NUM);
    order_init(orders);
    std::map<double, int> final_global;

    int avg_dur = 0;
    volatile int sink = 0;
    for (int i = 0; i < 3; i++)
    {
        std::map<double, int> m1, m2, m3, m4, global;

        auto start = std::chrono::high_resolution_clock::now();
        std::thread t1(parallel_processing, std::ref(orders), 0, ORDER_NUM / 4, std::ref(m1));
        std::thread t2(parallel_processing, std::ref(orders), ORDER_NUM / 4, ORDER_NUM / 2, std::ref(m2));
        std::thread t3(parallel_processing, std::ref(orders), ORDER_NUM / 2, 3 * ORDER_NUM / 4, std::ref(m3));
        std::thread t4(parallel_processing, std::ref(orders), 3 * ORDER_NUM / 4, ORDER_NUM, std::ref(m4));

        t1.join();
        t2.join();
        t3.join();
        t4.join();

        sink += m1[0] + m2[0] + m3[0] + m4[0];

        for (const auto &[price, qty] : m1)
        {
            global[price] += qty;
        }
        for (const auto &[price, qty] : m2)
        {
            global[price] += qty;
        }
        for (const auto &[price, qty] : m3)
        {
            global[price] += qty;
        }
        for (const auto &[price, qty] : m4)
        {
            global[price] += qty;
        }
        auto end = std::chrono::high_resolution_clock::now();
        int duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        avg_dur += duration;
        if (i == 0)
        {
            final_global = global;
        }
    }
    int duration = avg_dur / 3;

    size_t size = final_global.size();

    long long s1 = 0, s2 = 0;
    for (int i = 0; i < ORDER_NUM; i++)
    {
        s1 += orders[i].qty;
    }
    for (const auto &pair : final_global)
    {
        s2 += pair.second;
    }

    std::cout << "Execution Time : " << duration << "ms\n";
    std::cout << "Throughput : " << ORDER_NUM * 1000.0 / duration << " orders/s\n";
    if (s1 == s2)
    {
        std::cout << "Verification success!!!!\n";
    }
    else
    {
        std::cout << "Verification failed!!\n";
    }
}

struct alignas(64) Aligned
{
    std::atomic<int> value;
};

void parallel_array_processing(const std::vector<Order>& orders, int start, int end, int *arr)
{
    for (int i = start; i < end; i++)
    {
        int index = static_cast<int>(std::round((orders[i].price - 99.00) * 100));
        if (index < 0) index = 0;
        if (index >= PRICE_NUM) index = PRICE_NUM - 1;
        arr[index] += orders[i].qty;
    }
}

void run_version3()
{
    std::vector<Order> orders(ORDER_NUM);
    order_init(orders);
    Aligned arr[PRICE_NUM];
    for (int i = 0; i < PRICE_NUM; i++)
    {
        arr[i].value.store(0, std::memory_order_relaxed);
    }

    int avg_dur = 0;
    volatile int sink = 0;
    for (int i = 0; i < 3; i++)
    {
        alignas(64) int l1[PRICE_NUM], l2[PRICE_NUM], l3[PRICE_NUM], l4[PRICE_NUM], a[PRICE_NUM];
        for (int j = 0; j < PRICE_NUM; j++)
        {
            l1[j] = 0;
            l2[j] = 0;
            l3[j] = 0;
            l4[j] = 0;
        }
        auto start = std::chrono::high_resolution_clock::now();

        std::thread t1(parallel_array_processing, std::cref(orders), 0, ORDER_NUM / 4, l1);
        std::thread t2(parallel_array_processing, std::cref(orders), ORDER_NUM / 4, ORDER_NUM / 2, l2);
        std::thread t3(parallel_array_processing, std::cref(orders), ORDER_NUM / 2, 3 * ORDER_NUM / 4, l3);
        std::thread t4(parallel_array_processing, std::cref(orders), 3 * ORDER_NUM / 4, ORDER_NUM, l4);

        t1.join();
        t2.join();
        t3.join();
        t4.join();

        sink += l1[0] + l2[0] + l3[0] + l4[0];
        for (int j = 0; j < PRICE_NUM; j++)
        {
            a[j] = l1[j] + l2[j] + l3[j] + l4[j];
        }
        auto end = std::chrono::high_resolution_clock::now();
        int duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        avg_dur += duration;
        if (i == 0)
        {
            for (int j = 0; j < PRICE_NUM; j++)
            {
                arr[j].value.store(a[j], std::memory_order_relaxed);
            }
        }
    }

    int duration = avg_dur / 3;

    long long s1 = 0, s2 = 0;
    for (int i = 0; i < ORDER_NUM; i++)
    {
        s1 += orders[i].qty;
    }
    for (int i = 0; i < PRICE_NUM; i++)
    {
        s2 += arr[i].value.load(std::memory_order_relaxed);
    }

    std::cout << "Execution Time : " << duration << "ms\n";
    std::cout << "Throughput : " << ORDER_NUM * 1000.0 / duration << " orders/s\n";
    if (s1 == s2)
    {
        std::cout << "Verification success!!!!\n";
    }
    else
    {
        std::cout << "Verification failed!!\n";
    }
}

int main()
{
    run_version1();
    run_version2();
    run_version3();
}