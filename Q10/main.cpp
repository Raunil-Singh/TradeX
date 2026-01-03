#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <chrono>
#include <random>

int ARRAY_SIZE = 100000000;

bool verify(const std::vector<int> &ref, const std::vector<int> &test)
{
    for (size_t i = 0; i < ref.size(); i++)
    {
        if (ref[i] != test[i])
            return false;
    }
    return true;
}

void init(std::vector<int> &a, std::vector<int> &b)
{
    thread_local std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<size_t> dist(0, 100);

    for (int i = 0; i < ARRAY_SIZE; i++)
    {
        a[i] = dist(rng);
        b[i] = dist(rng);
    }
}

void sequential_sum(std::vector<int> &a, std::vector<int> &b, std::vector<int> &c)
{
    for (int i = 0; i < ARRAY_SIZE; i++)
    {
        c[i] = a[i] + b[i];
    }
}

void parallel_sum(std::vector<int> &a, std::vector<int> &b, std::vector<int> &c, int start, int end)
{
    for (int i = start; i < end; i++)
    {
        c[i] = a[i] + b[i];
    }
}

void parallel_unrolling_sum(std::vector<int> &a, std::vector<int> &b, std::vector<int> &c, int start, int end)
{
    int i = start;
    for (; i + 3 < end; i += 4)
    {
        c[i] = a[i] + b[i];
        c[i + 1] = a[i + 1] + b[i + 1];
        c[i + 2] = a[i + 2] + b[i + 2];
        c[i + 3] = a[i + 3] + b[i + 3];
    }

    for (; i < end; i++)
    {
        c[i] = a[i] + b[i];
    }
}

void parallel_vectorized_sum(std::vector<int> &a, std::vector<int> &b, std::vector<int> &c, int start, int end)
{
#pragma GCC ivdep
#pragma GCC unroll 4
    for (int i = start; i < end; i++)
    {
        c[i] = a[i] + b[i];
    }
}

int sequential(std::vector<int> &a, std::vector<int> &b, std::vector<int> &c)
{

    int total_duration = 0;
    for (int i = 0; i < 3; i++)
    {
        auto start = std::chrono::high_resolution_clock::now();
        sequential_sum(a, b, c);
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        total_duration += duration.count();
    }

    return total_duration / 3;
}

int parallel(std::vector<int> &a, std::vector<int> &b, std::vector<int> &c)
{

    int total_duration = 0;
    for (int i = 0; i < 3; i++)
    {
        auto start = std::chrono::high_resolution_clock::now();
        std::thread t1(parallel_sum, std::ref(a), std::ref(b), std::ref(c), 0, ARRAY_SIZE / 4);
        std::thread t2(parallel_sum, std::ref(a), std::ref(b), std::ref(c), ARRAY_SIZE / 4, ARRAY_SIZE / 2);
        std::thread t3(parallel_sum, std::ref(a), std::ref(b), std::ref(c), ARRAY_SIZE / 2, 3 * ARRAY_SIZE / 4);
        std::thread t4(parallel_sum, std::ref(a), std::ref(b), std::ref(c), 3 * ARRAY_SIZE / 4, ARRAY_SIZE);

        t1.join();
        t2.join();
        t3.join();
        t4.join();

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        total_duration += duration.count();
    }
    return total_duration / 3;
}

int parallel_unrolling(std::vector<int> &a, std::vector<int> &b, std::vector<int> &c)
{

    int total_duration = 0;
    for (int i = 0; i < 3; i++)
    {
        auto start = std::chrono::high_resolution_clock::now();
        std::thread t1(parallel_unrolling_sum, std::ref(a), std::ref(b), std::ref(c), 0, ARRAY_SIZE / 4);
        std::thread t2(parallel_unrolling_sum, std::ref(a), std::ref(b), std::ref(c), ARRAY_SIZE / 4, ARRAY_SIZE / 2);
        std::thread t3(parallel_unrolling_sum, std::ref(a), std::ref(b), std::ref(c), ARRAY_SIZE / 2, 3 * ARRAY_SIZE / 4);
        std::thread t4(parallel_unrolling_sum, std::ref(a), std::ref(b), std::ref(c), 3 * ARRAY_SIZE / 4, ARRAY_SIZE);

        t1.join();
        t2.join();
        t3.join();
        t4.join();
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        total_duration += duration.count();
    }
    return total_duration / 3;
}

int parallel_vectorized(std::vector<int> &a, std::vector<int> &b, std::vector<int> &c)
{

    int total_duration = 0;
    for (int i = 0; i < 3; i++)
    {
        auto start = std::chrono::high_resolution_clock::now();
        std::thread t1(parallel_vectorized_sum, std::ref(a), std::ref(b), std::ref(c), 0, ARRAY_SIZE / 4);
        std::thread t2(parallel_vectorized_sum, std::ref(a), std::ref(b), std::ref(c), ARRAY_SIZE / 4, ARRAY_SIZE / 2);
        std::thread t3(parallel_vectorized_sum, std::ref(a), std::ref(b), std::ref(c), ARRAY_SIZE / 2, 3 * ARRAY_SIZE / 4);
        std::thread t4(parallel_vectorized_sum, std::ref(a), std::ref(b), std::ref(c), 3 * ARRAY_SIZE / 4, ARRAY_SIZE);

        t1.join();
        t2.join();
        t3.join();
        t4.join();
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        total_duration += duration.count();
    }
    return total_duration / 3;
}

int main()
{
    std::vector<int> a(ARRAY_SIZE);
    std::vector<int> b(ARRAY_SIZE);
    std::vector<int> c_parallel(ARRAY_SIZE);
    std::vector<int> c_unroll(ARRAY_SIZE);
    std::vector<int> c_vec(ARRAY_SIZE);
    std::vector<int> ref(ARRAY_SIZE);
    init(a, b);

    int duration1 = sequential(a, b, ref);
    // std::cout<<"Execution time for Sequential : "<<duration1<<"ms\n";

    int duration2 = parallel(a, b, c_parallel);
    if (!verify(ref, c_parallel))
        std::cout << "Parallel FAILED\n";
    // std::cout<<"Execution time for Parallel : "<<duration2<<"ms\n";

    int duration3 = parallel_unrolling(a, b, c_unroll);
    if (!verify(ref, c_unroll))
        std::cout << "Parallel manual unrolling FAILED\n";
    // std::cout<<"Execution time for Parallel manual unrolling : "<<duration3<<"ms\n";

    int duration4 = parallel_vectorized(a, b, c_vec);
    if (!verify(ref, c_vec))
        std::cout << "Parallel Vectorization FAILED\n";
    // std::cout<<"Execution time for Parallel with Vectorization hint : "<<duration4<<"ms\n";

    std::cout << "==========================================\n";
    std::cout << "| Version                    | Time (ms) |\n";
    std::cout << "==========================================\n";
    std::cout << "| Sequential                 |   " << duration1 << "ms   |\n";
    std::cout << "| Parallel                   |   " << duration2 << "ms   |\n";
    std::cout << "| Parallel manual unrolling  |   " << duration3 << "ms   |\n";
    std::cout << "| Parallel Vectorization     |   " << duration4 << "ms   |\n";
    std::cout << "==========================================\n";
}