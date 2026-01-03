#include <iostream>
#include <mutex>
#include <thread>
#include <random>
#include <vector>
#include <chrono>
#include <atomic>

std::mutex single_mtx;
thread_local std::mt19937 rng(std::random_device{}());

void increment_thread(int &order_count, int &trade_count, int &cancel_count)
{
    std::uniform_int_distribution<size_t> dist(0, 2);
    for (int i = 0; i < 1000000; i++)
    {
        int num = dist(rng);

        {
            std::lock_guard<std::mutex> lock(single_mtx);
            if (num == 0)
            {
                order_count++;
            }
            else if (num == 1)
            {
                trade_count++;
            }
            else
            {
                cancel_count++;
            }
        }
    }
}

std::mutex order_mtx;
std::mutex trade_mtx;
std::mutex cancel_mtx;
void increment_thread_2(int &order_count, int &trade_count, int &cancel_count)
{
    std::uniform_int_distribution<size_t> dist(0, 2);
    for (int i = 0; i < 1000000; i++)
    {
        int num = dist(rng);

        {
            if (num == 0)
            {
                {
                    std::lock_guard<std::mutex> lock(order_mtx);
                    order_count++;
                }
            }
            else if (num == 1)
            {
                {
                    std::lock_guard<std::mutex> lock(trade_mtx);
                    trade_count++;
                }
            }
            else
            {
                {
                    std::lock_guard<std::mutex> lock(cancel_mtx);
                    cancel_count++;
                }
            }
        }
    }
}

void increment_thread_3(std::atomic<int> &order_count, std::atomic<int> &trade_count, std::atomic<int> &cancel_count)
{
    std::uniform_int_distribution<size_t> dist(0, 2);
    for (int i = 0; i < 1000000; i++)
    {
        int num = dist(rng);

        {
            if (num == 0)
            {
                order_count++;
            }
            else if (num == 1)
            {
                trade_count++;
            }
            else
            {
                cancel_count++;
            }
        }
    }
}

void increment_thread_4(std::atomic<int> &order_count, std::atomic<int> &trade_count, std::atomic<int> &cancel_count)
{
    std::uniform_int_distribution<size_t> dist(0, 2);
    for (int i = 0; i < 1000000; i++)
    {
        int num = dist(rng);

        {
            if (num == 0)
            {
                order_count.fetch_add(1, std::memory_order_relaxed);
            }
            else if (num == 1)
            {
                trade_count.fetch_add(1, std::memory_order_relaxed);
            }
            else
            {
                cancel_count.fetch_add(1, std::memory_order_relaxed);
            }
        }
    }
}

void single_mutex()
{
    int order_count = 0;
    int trade_count = 0;
    int cancel_count = 0;

    std::vector<std::thread> threads;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 8; i++)
    {
        threads.emplace_back(increment_thread, std::ref(order_count), std::ref(trade_count), std::ref(cancel_count));
    }
    for (auto &p : threads)
    {
        p.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double>(end - start).count();
    int total_count = order_count + trade_count + cancel_count;
    int throughput = total_count/duration/1e6;

    std::cout<<"Mutex-Based : "<<std::endl;
    std::cout << "Order Count : "<<order_count << " | Trade Count : "<<trade_count << " | Cancel Count : " << cancel_count << std::endl;
    std::cout<<"Execution Time : "<<(int)(duration*1000)<<"ms | Throughput : "<<throughput<<std::endl;
}

void per_counter_mutex()
{
    int order_count = 0;
    int trade_count = 0;
    int cancel_count = 0;

    std::vector<std::thread> threads;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 8; i++)
    {
        threads.emplace_back(increment_thread_2, std::ref(order_count), std::ref(trade_count), std::ref(cancel_count));
    }
    for (auto &p : threads)
    {
        p.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double>(end - start).count();
    int total_count = order_count + trade_count + cancel_count;
    int throughput = total_count/duration/1e6;

    std::cout<<"Per-Counter-Mutex-Based : "<<std::endl;
    std::cout << "Order Count : "<<order_count << " | Trade Count : "<<trade_count << " | Cancel Count : " << cancel_count << std::endl;
    std::cout<<"Execution Time : "<<(int)(duration*1000)<<"ms | Throughput : "<<throughput<<std::endl;
}

void atomic_int(){
    std::atomic<int> order_count{0};
    std::atomic<int> trade_count{0};
    std::atomic<int> cancel_count{0};

    std::vector<std::thread> threads;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 8; i++)
    {
        threads.emplace_back(increment_thread_3, std::ref(order_count), std::ref(trade_count), std::ref(cancel_count));
    }
    for (auto &p : threads)
    {
        p.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double>(end - start).count();
    int total_count = order_count.load() + trade_count.load() + cancel_count.load();
    int throughput = total_count/duration/1e6;

    std::cout<<"Atomic : "<<std::endl;
    std::cout << "Order Count : "<<order_count << " | Trade Count : "<<trade_count << " | Cancel Count : " << cancel_count << std::endl;
    std::cout<<"Execution Time : "<<(int)(duration*1000)<<"ms | Throughput : "<<throughput<<std::endl;

}

void memory_order(){
    std::atomic<int> order_count{0};
    std::atomic<int> trade_count{0};
    std::atomic<int> cancel_count{0};

    std::vector<std::thread> threads;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 8; i++)
    {
        threads.emplace_back(increment_thread_4, std::ref(order_count), std::ref(trade_count), std::ref(cancel_count));
    }
    for (auto &p : threads)
    {
        p.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double>(end - start).count();
    int total_count = order_count.load() + trade_count.load() + cancel_count.load();
    int throughput = total_count/duration/1e6;

    std::cout<<"Atomic with Memory Order : "<<std::endl;
    std::cout << "Order Count : "<<order_count << " | Trade Count : "<<trade_count << " | Cancel Count : " << cancel_count << std::endl;
    std::cout<<"Execution Time : "<<(int)(duration*1000)<<"ms | Throughput : "<<throughput<<std::endl;

}

int main()
{
    single_mutex();
    per_counter_mutex();
    atomic_int();
    memory_order();
}