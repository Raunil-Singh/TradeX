#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <algorithm>

const int ARRAY_SIZE = 100'000'000;
const int THREAD_COUNT = 4;


template <typename F>
long long measure(F f)
{
    auto start = std::chrono::high_resolution_clock::now();
    f();
    auto end = std::chrono::high_resolution_clock::now();

    return std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start).count();
}


void initialize(std::vector<float>& A, std::vector<float>& B, std::vector<float>& C)
{
    for (int i = 0; i < A.size(); ++i)
    {
        A[i] = static_cast<float>(i);
        B[i] = static_cast<float>(2 * i);
        C[i] = 0.0f;
    }
}



void sequential(const std::vector<float>& A, const std::vector<float>& B, std::vector<float>& C)
{
    for (int i = 0; i < A.size(); ++i)
    {
        C[i] = A[i] + B[i];
    }
}


void parallelWorker(const std::vector<float>& A, const std::vector<float>& B, std::vector<float>& C,
                    int start, int end)
{
    for (int i = start; i < end; ++i)
    {
        C[i] = A[i] + B[i];
    }
}

void runParallel(const std::vector<float>& A, const std::vector<float>& B, std::vector<float>& C)
{
    std::vector<std::thread> threads;
    int section = A.size() / THREAD_COUNT;

    for (int i = 0; i < THREAD_COUNT; ++i)
    {
        int start = i * section;
        int end = (i == THREAD_COUNT - 1)
                ? A.size()
                : start + section;

        threads.emplace_back(parallelWorker, std::ref(A), std::ref(B), std::ref(C), start, end);
    }
    for (auto& t : threads)
        t.join();
}

void parallelUnrolledWorker(const std::vector<float>& A, const std::vector<float>& B, std::vector<float>& C,
                         int start, int end)
{
    int i = start;

    for (; i + 3 < end; i += 4)
    {
        C[i] = A[i] + B[i];
        C[i + 1] = A[i + 1] + B[i + 1];
        C[i + 2] = A[i + 2] + B[i + 2];
        C[i + 3] = A[i + 3] + B[i + 3];
    }

    for (; i < end; ++i)
    {
        C[i] = A[i] + B[i];
    }
}

void runParallelUnrolled(const std::vector<float>& A, const std::vector<float>& B, std::vector<float>& C)
{
    std::vector<std::thread> threads;
    int chunk = A.size() / THREAD_COUNT;

    for (int i = 0; i < THREAD_COUNT; ++i)
    {
        int start = i * chunk;
        int end  = (i == THREAD_COUNT - 1)
                    ? A.size()
                    : start + chunk;

        threads.emplace_back(parallelUnrolledWorker, std::ref(A), std::ref(B), std::ref(C), start, end);
    }

    for (auto& t : threads)
        t.join();
}


void parallelVectorizedWorker(const std::vector<float>& A, const std::vector<float>& B, std::vector<float>& C,
                              int start, int end)
{
#pragma GCC ivdep
#pragma GCC unroll 4
    for (int i = start; i < end; ++i)
    {
        C[i] = A[i] + B[i];
    }
}

void runParallelVectorized(const std::vector<float>& A, const std::vector<float>& B, std::vector<float>& C)
{
    std::vector<std::thread> threads;
    int section = A.size() / THREAD_COUNT;

    for (int i = 0; i < THREAD_COUNT; ++i)
    {
        int start = i * section;
        int end = (i == THREAD_COUNT - 1)
                     ? A.size()
                     : start + section;

        threads.emplace_back(parallelVectorizedWorker, std::ref(A), std::ref(B), std::ref(C), start, end);
    }

    for (auto& t : threads)
        t.join();
}


bool verify(const std::vector<float>& A, const std::vector<float>& B, const std::vector<float>& C)
{
    for (int i = 0; i < A.size(); ++i)
    {
        if (C[i] != A[i] + B[i])
            return false;
    }
    return true;
}



int main()
{
    std::vector<float> A(ARRAY_SIZE);
    std::vector<float> B(ARRAY_SIZE);
    std::vector<float> C(ARRAY_SIZE);

    initialize(A, B, C);

    auto t1 = measure([&] {
        sequential(A, B, C);
    });

    initialize(A, B, C);

    auto t2 = measure([&] {
        runParallel(A, B, C);
    });

    initialize(A, B, C);

    auto t3 = measure([&] {
        runParallelUnrolled(A, B, C);
    });

    initialize(A, B, C);

    auto t4 = measure([&] {
        runParallelVectorized(A, B, C);
    });

    std::cout << "\nResults (ms):\n\n";
    std::cout << "Sequential:           " << t1 << "\n";
    std::cout << "Parallel:             " << t2 << "\n";
    std::cout << "Parallel + unrolled:  " << t3 << "\n";
    std::cout << "Parallel + vectorized:" << t4 << "\n\n";

    std::cout << "Verification: " << (verify(A, B, C) ? "OK" : "FAIL") << "\n\n";
}
