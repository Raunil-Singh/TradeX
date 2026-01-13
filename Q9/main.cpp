#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <random>
#include <chrono>

template <typename F>
long long measure(F f)
{
    auto start = std::chrono::high_resolution_clock::now();
    f();
    auto end = std::chrono::high_resolution_clock::now();

    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}


const int THREAD_COUNT = 4;
const int PHASE_COUNT = 4;

int randomBetween(int low, int high)
{
    static thread_local std::mt19937 mt(std::random_device{}());
    std::uniform_int_distribution<int> dist(low, high);
    return dist(mt);
}

std::mutex barrier_mutex;
std::condition_variable barrier_cv;

int arrived_threads = 0;
int current_phase = 0;



void barrier(int phase)
{
    std::unique_lock<std::mutex> lock(barrier_mutex);

    arrived_threads++;

    if (arrived_threads == THREAD_COUNT)
    {
        arrived_threads = 0;
        current_phase++;
        barrier_cv.notify_all();
    }
    else
    {
        barrier_cv.wait(lock, [&] {
            return current_phase > phase;
        });
    }
}

void doWork(int thread_id, int phase)
{
    int duration = 0;

    if (phase == 0)
        duration = randomBetween(50, 100);

    else if (phase == 1)
        duration = randomBetween(30, 80);

    else if (phase == 2)
        duration = randomBetween(40, 90);

    else
        duration = randomBetween(20, 60);

    std::this_thread::sleep_for(std::chrono::milliseconds(duration));
}

void synchronizedWorker(int thread_id)
{
    for (int phase = 0; phase < PHASE_COUNT; phase++)
    {
        std::cout << "Thread " << thread_id << " reached phase " << phase << "\n";

        doWork(thread_id, phase);

        barrier(phase);

        std::cout << "Thread " << thread_id << " leaving phase " << phase << "\n";
    }
}

void UnsynchronizedWorker(int thread_id)
{
    for (int phase = 0; phase < PHASE_COUNT; phase++)
    {
        doWork(thread_id, phase);
    }
}


void runSynchronized()
{
    std::vector<std::thread> threads;

    auto time = measure([&] {
        for (int i = 0; i < THREAD_COUNT; i++)
            threads.emplace_back(synchronizedWorker, i);

        for (auto& t : threads)
            t.join();
    });

    std::cout << "\nSynchronized total time: "<< time << " ms\n\n";
}

void runUnsynchronized()
{
    std::vector<std::thread> threads;

    auto time = measure([&] {
        for (int i = 0; i < THREAD_COUNT; i++)
            threads.emplace_back(UnsynchronizedWorker, i);

        for (auto& t : threads)
            t.join();
    });

    std::cout << "Unsynchronized total time: " << time << " ms\n\n";
}

int main()
{
    std::cout << "\nResult: \n\n";

    runSynchronized();

    runUnsynchronized();
}
