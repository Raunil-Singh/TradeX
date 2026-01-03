#include <iostream>
#include <mutex>
#include <thread>
#include <random>
#include <shared_mutex>
#include <vector>
#include <chrono>
#include <iomanip>

#define SLEEP std::this_thread::sleep_for(std::chrono::nanoseconds(10))
#define READER_COUNT 8
#define WRITER_COUNT 2

struct ThreadMetrics
{
    long long operations;
    double wait_time_us;
    double computed_average;
    int thread_id;

    ThreadMetrics() : operations(0), wait_time_us(0.0), computed_average(0.0), thread_id(0) {}
};

//============================================================
//                 MUTEX ONLY VERSION
//============================================================
std::mutex mtx_rw_entry;
int reader_count = 0;
std::mutex mtx_reader_count;
thread_local std::mt19937 rng1(std::random_device{}());
void reader_mutex(std::vector<double> &price, ThreadMetrics &metrics, int thread_id)
{
    metrics.thread_id = thread_id;
    std::uniform_int_distribution<size_t> dist(0, price.size() - 1);
    double sum = 0.0;
    double total_wait_us = 0.0;
    for (long long i = 0; i < 100000; i++)
    {
        long long idx = dist(rng1);
        double value = 0;
        auto wait_start = std::chrono::high_resolution_clock::now();
        {
            {
                std::lock_guard<std::mutex> lock(mtx_reader_count);
                if (reader_count == 0)
                    mtx_rw_entry.lock();
                reader_count++;
            }
            auto wait_end = std::chrono::high_resolution_clock::now();
            total_wait_us += std::chrono::duration_cast<std::chrono::microseconds>(wait_end - wait_start).count();

            value = price[idx];
            {
                std::lock_guard<std::mutex> lock(mtx_reader_count);
                reader_count--;
                if (reader_count == 0)
                    mtx_rw_entry.unlock();
            }
        }
        SLEEP;
        sum += value;
    }
    metrics.operations = 100000;
    metrics.wait_time_us = total_wait_us;
    metrics.computed_average = sum / 100000;
}

void writer_mutex(std::vector<double> &price, ThreadMetrics &metrics, int thread_id)
{
    metrics.thread_id = thread_id;
    double total_wait_us = 0.0;
    std::uniform_int_distribution<size_t> dist(0, price.size() - 1);
    std::uniform_int_distribution<int> change_dist(0, 4);
    for (long long i = 0; i < 10000; i++)
    {
        long long idx = dist(rng1);
        long long change = change_dist(rng1);
        auto wait_start = std::chrono::high_resolution_clock::now();
        {
            std::lock_guard<std::mutex> lock1(mtx_rw_entry);
            auto wait_end = std::chrono::high_resolution_clock::now();
            total_wait_us += std::chrono::duration_cast<std::chrono::microseconds>(wait_end - wait_start).count();
            price[idx] += change;
        }
        SLEEP;
    }
    metrics.operations = 10000;
    metrics.wait_time_us = total_wait_us;
}

void run_mutex_version()
{
    std::vector<double> prices(1000, 100.0);
    std::vector<ThreadMetrics> reader_metrics(READER_COUNT);
    std::vector<ThreadMetrics> writer_metrics(WRITER_COUNT);
    reader_count = 0;

    long long avg_time = 0;
    for(int i = 0; i<3; i++)
    {auto start = std::chrono::high_resolution_clock::now();

    std::vector<std::thread> threads;

    for (int i = 0; i < READER_COUNT; ++i)
    {
        threads.emplace_back(reader_mutex, std::ref(prices), std::ref(reader_metrics[i]), i);
    }

    for (int i = 0; i < WRITER_COUNT; ++i)
    {
        threads.emplace_back(writer_mutex, std::ref(prices), std::ref(writer_metrics[i]), i);
    }

    for (auto &t : threads)
    {
        t.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    avg_time += duration.count();
}

    long long duration = avg_time/3;
    long long total_reads = 0;
    long long total_writes = 0;
    double total_wait_time = 0.0;
    double sum_averages = 0.0;

    std::cout << "\nReader Threads:\n";
    for (const auto &m : reader_metrics)
    {
        total_reads += m.operations;
        total_wait_time += m.wait_time_us;
        sum_averages += m.computed_average;
        std::cout << "  Thread " << m.thread_id
                  << " - Reads: " << m.operations
                  << ", Avg Price: " << std::fixed << std::setprecision(2) << m.computed_average
                  << ", Wait Time: " << std::fixed << std::setprecision(2) << (m.wait_time_us / 1000.0) << " ms\n";
    }

    std::cout << "\nWriter Threads:\n";
    for (const auto &m : writer_metrics)
    {
        total_writes += m.operations;
        total_wait_time += m.wait_time_us;
        std::cout << "  Thread " << m.thread_id
                  << " - Writes: " << m.operations
                  << ", Wait Time: " << std::fixed << std::setprecision(2) << (m.wait_time_us / 1000.0) << " ms\n";
    }

    std::cout << "\n--- Summary ---\n";
    std::cout << "Execution Time: " << duration<< " ms\n";
    std::cout << "Total Reads: " << total_reads << "\n";
    std::cout << "Total Writes: " << total_writes << "\n";
    std::cout << "Average Read Value: " << std::fixed << std::setprecision(4) << (sum_averages / READER_COUNT) << "\n";
    std::cout << "Average Wait Time per Thread: "
              << std::fixed << std::setprecision(2)
              << (total_wait_time / (READER_COUNT + WRITER_COUNT) / 1000.0) << " ms\n";
}

//============================================================
//                    SHARED MUTEX VERSION
//============================================================

std::shared_mutex shared_mtx;
thread_local std::mt19937 rng2(std::random_device{}());
void reader_shared_mutex(std::vector<double> &price, ThreadMetrics &metrics, int thread_id)
{
    metrics.thread_id = thread_id;
    double total_wait_us = 0.0;
    double sum = 0.0;
    std::uniform_int_distribution<size_t> dist(0, price.size() - 1);
    std::uniform_int_distribution<int> change_dist(0, 4);
    for (long long i = 0; i < 100000; i++)
    {
        long long idx = dist(rng2);
        double value;
        auto wait_start = std::chrono::high_resolution_clock::now();
        {
            std::shared_lock<std::shared_mutex> lock(shared_mtx);
            auto wait_end = std::chrono::high_resolution_clock::now();
            total_wait_us += std::chrono::duration_cast<std::chrono::microseconds>(wait_end - wait_start).count();
            value = price[idx];
        }
        SLEEP;
        sum += value;
    }
    metrics.operations = 100000;
    metrics.wait_time_us = total_wait_us;
    metrics.computed_average = sum / 100000;
}

void writer_shared_mutex(std::vector<double> &price, ThreadMetrics &metrics, int thread_id)
{
    double total_wait_us = 0.0;
    metrics.thread_id = thread_id;
    std::uniform_int_distribution<size_t> dist(0, price.size() - 1);
    std::uniform_int_distribution<int> change_dist(0, 4);
    for (long long i = 0; i < 10000; i++)
    {
        long long idx = dist(rng2);
        long long change = change_dist(rng2);
        auto wait_start = std::chrono::high_resolution_clock::now();
        {
            std::unique_lock<std::shared_mutex> lock(shared_mtx);
            auto wait_end = std::chrono::high_resolution_clock::now();
            total_wait_us += std::chrono::duration_cast<std::chrono::microseconds>(wait_end - wait_start).count();
            price[idx] += change;
        }
        SLEEP;
    }
    metrics.operations = 10000;
    metrics.wait_time_us = total_wait_us;
}

void run_shared_mutex_version()
{
    std::vector<double> prices(1000, 100.0);
    std::vector<ThreadMetrics> reader_metrics(READER_COUNT);
    std::vector<ThreadMetrics> writer_metrics(WRITER_COUNT);

    long long avg_time = 0;
    for(int i = 0; i<3; i++)
    {auto start = std::chrono::high_resolution_clock::now();

    std::vector<std::thread> threads;

    for (int i = 0; i < READER_COUNT; ++i)
    {
        threads.emplace_back(reader_shared_mutex, std::ref(prices), std::ref(reader_metrics[i]), i);
    }

    for (int i = 0; i < WRITER_COUNT; ++i)
    {
        threads.emplace_back(writer_shared_mutex, std::ref(prices), std::ref(writer_metrics[i]), i);
    }

    for (auto &t : threads)
    {
        t.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    avg_time += duration.count();
}

    long long duration = avg_time/3;
    long long total_reads = 0;
    long long total_writes = 0;
    double total_wait_time = 0.0;
    double sum_averages = 0.0;

    std::cout << "\nReader Threads:\n";
    for (const auto &m : reader_metrics)
    {
        total_reads += m.operations;
        total_wait_time += m.wait_time_us;
        sum_averages += m.computed_average;
        std::cout << "  Thread " << m.thread_id
                  << " - Reads: " << m.operations
                  << ", Avg Price: " << std::fixed << std::setprecision(2) << m.computed_average
                  << ", Wait Time: " << std::fixed << std::setprecision(2) << (m.wait_time_us / 1000.0) << " ms\n";
    }

    std::cout << "\nWriter Threads:\n";
    for (const auto &m : writer_metrics)
    {
        total_writes += m.operations;
        total_wait_time += m.wait_time_us;
        std::cout << "  Thread " << m.thread_id
                  << " - Writes: " << m.operations
                  << ", Wait Time: " << std::fixed << std::setprecision(2) << (m.wait_time_us / 1000.0) << " ms\n";
    }

    std::cout << "\n--- Summary ---\n";
    std::cout << "Execution Time: " << duration << " ms\n";
    std::cout << "Total Reads: " << total_reads << "\n";
    std::cout << "Total Writes: " << total_writes << "\n";
    std::cout << "Average Read Value: " << std::fixed << std::setprecision(4) << (sum_averages / READER_COUNT) << "\n";
    std::cout << "Average Wait Time per Thread: "
              << std::fixed << std::setprecision(2)
              << (total_wait_time / (READER_COUNT + WRITER_COUNT) / 1000.0) << " ms\n";
}

//============================================================
//                    FINE GRAINED VERSION
//============================================================

std::mutex segmented[10];
thread_local std::mt19937 rng3(std::random_device{}());
void reader_segmented_mutex(std::vector<double> &price, ThreadMetrics &metrics, int thread_id)
{
    metrics.thread_id = thread_id;
    double total_wait_us = 0.0;
    std::uniform_int_distribution<size_t> dist(0, price.size() - 1);
    double sum = 0.0;
    for (long long i = 0; i < 100000; i++)
    {
        long long idx = dist(rng3);
        double value;
        auto wait_start = std::chrono::high_resolution_clock::now();
        {
            std::lock_guard<std::mutex> lock(segmented[idx % 10]);
            auto wait_end = std::chrono::high_resolution_clock::now();
            total_wait_us += std::chrono::duration_cast<std::chrono::microseconds>(wait_end - wait_start).count();
            value = price[idx];
        }
        SLEEP;
        sum += value;
    }
    metrics.operations = 100000;
    metrics.wait_time_us = total_wait_us;
    metrics.computed_average = sum / 100000;
}

void writer_segmented_mutex(std::vector<double> &price, ThreadMetrics &metrics, int thread_id)
{
    metrics.thread_id = thread_id;
    double total_wait_us = 0.0;
    std::uniform_int_distribution<size_t> dist(0, price.size() - 1);
    std::uniform_int_distribution<int> change_dist(0, 4);
    for (long long i = 0; i < 10000; i++)
    {
        long long idx = dist(rng3);
        long long change = change_dist(rng3);
        auto wait_start = std::chrono::high_resolution_clock::now();
        {
            std::lock_guard<std::mutex> lock(segmented[idx % 10]);
            auto wait_end = std::chrono::high_resolution_clock::now();
            total_wait_us += std::chrono::duration_cast<std::chrono::microseconds>(wait_end - wait_start).count();
            price[idx] += change;
        }
        SLEEP;
    }
    metrics.operations = 10000;
    metrics.wait_time_us = total_wait_us;
}

void run_segmented_mutex_version()
{
    std::vector<double> prices(1000, 100.0);
    std::vector<ThreadMetrics> reader_metrics(READER_COUNT);
    std::vector<ThreadMetrics> writer_metrics(WRITER_COUNT);

    long long avg_time = 0;
    for (int i = 0; i < 3; i++)
    {
        auto start = std::chrono::high_resolution_clock::now();

        std::vector<std::thread> threads;

        for (int i = 0; i < READER_COUNT; ++i)
        {
            threads.emplace_back(reader_segmented_mutex, std::ref(prices), std::ref(reader_metrics[i]), i);
        }

        for (int i = 0; i < WRITER_COUNT; ++i)
        {
            threads.emplace_back(writer_segmented_mutex, std::ref(prices), std::ref(writer_metrics[i]), i);
        }

        for (auto &t : threads)
        {
            t.join();
        }

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        avg_time += duration.count();
    }

    long long duration = avg_time / 3;

    long long total_reads = 0;
    long long total_writes = 0;
    double total_wait_time = 0.0;
    double sum_averages = 0.0;

    std::cout << "\nReader Threads:\n";
    for (const auto &m : reader_metrics)
    {
        total_reads += m.operations;
        total_wait_time += m.wait_time_us;
        sum_averages += m.computed_average;
        std::cout << "  Thread " << m.thread_id
                  << " - Reads: " << m.operations
                  << ", Avg Price: " << std::fixed << std::setprecision(2) << m.computed_average
                  << ", Wait Time: " << std::fixed << std::setprecision(2) << (m.wait_time_us / 1000.0) << " ms\n";
    }

    std::cout << "\nWriter Threads:\n";
    for (const auto &m : writer_metrics)
    {
        total_writes += m.operations;
        total_wait_time += m.wait_time_us;
        std::cout << "  Thread " << m.thread_id
                  << " - Writes: " << m.operations
                  << ", Wait Time: " << std::fixed << std::setprecision(2) << (m.wait_time_us / 1000.0) << " ms\n";
    }

    std::cout << "\n--- Summary ---\n";
    std::cout << "Execution Time: " << duration << " ms\n";
    std::cout << "Total Reads: " << total_reads << "\n";
    std::cout << "Total Writes: " << total_writes << "\n";
    std::cout << "Average Read Value: " << std::fixed << std::setprecision(4) << (sum_averages / READER_COUNT) << "\n";
    std::cout << "Average Wait Time per Thread: "
              << std::fixed << std::setprecision(2)
              << (total_wait_time / (READER_COUNT + WRITER_COUNT) / 1000.0) << " ms\n";
}

int main()
{
    run_mutex_version();
    run_shared_mutex_version();
    run_segmented_mutex_version();
    return 0;
}