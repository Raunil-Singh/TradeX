#include <iostream>
#include <vector>
#include <thread>
#include <future>
#include <random>
#include <chrono>
#include <algorithm>


const int MAX_DEPTH = 4;


void generateData(std::vector<int>& data)
{
    std::mt19937 mt(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, 1'000'000);

    for (int& x : data)
        x = dist(mt);
}

void merge(std::vector<int>& data, int left, int mid, int right)
{
    std::vector<int> temp(right - left);

    int i = left;
    int j = mid;
    int k = 0;

    while (i < mid && j < right)
    {
        if (data[i] <= data[j])
            temp[k++] = data[i++];
        else
            temp[k++] = data[j++];
    }

    while (i < mid)
        temp[k++] = data[i++];

    while (j < right)
        temp[k++] = data[j++];

    std::copy(temp.begin(), temp.end(), data.begin() + left);
}

void mergesortSequential(std::vector<int>& data, int left, int right)
{
    if (right - left <= 1)
        return;

    int mid = (left + right) / 2;

    mergesortSequential(data, left, mid);
    mergesortSequential(data, mid, right);

    merge(data, left, mid, right);
}

void mergesortParallel(std::vector<int>& data, int left, int right, int depth)
{
    if (right - left <= 1)
        return;

    if (depth >= MAX_DEPTH)
    {
        mergesortSequential(data, left, right);
        return;
    }

    int mid = (left + right) / 2;

    std::thread branch_thread(mergesortParallel, std::ref(data), left, mid, depth + 1);

    mergesortParallel(data, mid, right, depth + 1);

    branch_thread.join();

    merge(data, left, mid, right);
}

void mergesortAsync(std::vector<int>& data, int left, int right, int depth)
{
    if (right - left <= 1)
        return;

    if (depth >= MAX_DEPTH)
    {
        mergesortSequential(data, left, right);
        return;
    }

    int mid = (left + right) / 2;

    auto future_branch = std::async(std::launch::async, mergesortAsync, std::ref(data), left, mid, depth + 1);

    mergesortAsync(data, mid, right, depth + 1);

    future_branch.get();

    merge(data, left, mid, right);
}

template<typename mergesort>
long long measureTime(mergesort f)
{
    auto start = std::chrono::high_resolution_clock::now();
    f();
    auto end = std::chrono::high_resolution_clock::now();

    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}

void runTest(int size)
{
    std::vector<int> data(size);

    generateData(data);

    auto seq_data = data;
    auto par_data = data;
    auto async_data = data;

    long long t_seq = measureTime([&] {
        mergesortSequential(seq_data, 0, seq_data.size());
    });

    long long t_par = measureTime([&] {
        mergesortParallel(par_data, 0, par_data.size(), 0);
    });

    long long t_async = measureTime([&] {
        mergesortAsync(async_data, 0, async_data.size(), 0);
    });

    std::cout << "Size: " << size << "\n";
    std::cout << "Sequential: " << t_seq << " ms\n";
    std::cout << "Parallel: " << t_par << " ms\n";
    std::cout << "Work stealing: " << t_async << " ms\n\n";
}

int main()
{
    runTest(100'000);
    runTest(1'000'000);
    runTest(10'000'000);
}
