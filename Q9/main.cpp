#include <thread>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <chrono>
#include <random>

std::condition_variable cv;
std::mutex orderAccess;
int generation{};
int barrierCounter{};
constexpr int THREAD_COUNT{4};
std::uniform_int_distribution<> ovTime(50, 100);
std::uniform_int_distribution<> rcTime(30, 80);
std::uniform_int_distribution<> omTime(40, 90);
std::uniform_int_distribution<> trTime(20, 60);

std::chrono::steady_clock::duration phaseZeroTime;
std::chrono::steady_clock::duration phaseOneTime;
std::chrono::steady_clock::duration phaseTwoTime;
std::chrono::steady_clock::duration phaseThreeTime;
std::vector<std::chrono::steady_clock::duration> totalThreadTime(THREAD_COUNT);

std::chrono::steady_clock::time_point phaseStart;
std::chrono::steady_clock::time_point phaseEnd;
void workerThread(int id)
{
    std::mt19937 rGen(std::random_device{}());
    int generation_local{0};
    // order validation;
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    std::this_thread::sleep_for(std::chrono::milliseconds(ovTime(rGen)));
    {
        std::unique_lock<std::mutex> lock(orderAccess);
        generation_local = generation;

        std::cout << "Thread with id: " << id << " reached the barrier in generation: " << generation_local << "\n";
        if (barrierCounter == 0)
        {
            phaseStart = std::chrono::steady_clock::now();
        }

        barrierCounter++;
        if (barrierCounter == THREAD_COUNT)
        {
            phaseEnd = std::chrono::steady_clock::now();
            barrierCounter = 0;
            generation++;
            cv.notify_all();
            phaseZeroTime = phaseEnd - phaseStart;
        }
        while (generation == generation_local)
        {
            cv.wait(lock);
        }
        std::cout << "Thread with id: " << id << " left the barrier in generation: " << generation_local << "\n";
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(rcTime(rGen)));
    // risk check
    {
        std::unique_lock<std::mutex> lock(orderAccess);
        generation_local = generation;
        std::cout << "Thread with id: " << id << " reached the barrier in generation: " << generation_local << "\n";
        if (barrierCounter == 0)
        {
            phaseStart = std::chrono::steady_clock::now();
        }

        barrierCounter++;
        if (barrierCounter == THREAD_COUNT)
        {
            phaseEnd = std::chrono::steady_clock::now();
            barrierCounter = 0;
            generation++;
            cv.notify_all();
            phaseOneTime = phaseEnd - phaseStart;
        }
        while (generation == generation_local)
        {
            cv.wait(lock);
        }
        std::cout << "Thread with id: " << id << " left the barrier in generation: " << generation_local << "\n";
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(omTime(rGen)));
    // order matching
    {
        std::unique_lock<std::mutex> lock(orderAccess);
        generation_local = generation;
        std::cout << "Thread with id: " << id << " reached the barrier in generation: " << generation_local << "\n";
        if (barrierCounter == 0)
        {
            phaseStart = std::chrono::steady_clock::now();
        }
        barrierCounter++;
        if (barrierCounter == THREAD_COUNT)
        {
            phaseEnd = std::chrono::steady_clock::now();
            barrierCounter = 0;
            generation++;
            cv.notify_all();
            phaseTwoTime = phaseEnd - phaseStart;
        }
        while (generation == generation_local)
        {
            cv.wait(lock);
        }
        std::cout << "Thread with id: " << id << " left the barrier in generation: " << generation_local << "\n";
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(trTime(rGen)));
    // thread reporting
    {
        std::unique_lock<std::mutex> lock(orderAccess);
        generation_local = generation;
        std::cout << "Thread with id: " << id << " reached the barrier in generation: " << generation_local << "\n";
        if (barrierCounter == 0)
        {
            phaseStart = std::chrono::steady_clock::now();
        }
        barrierCounter++;
        if (barrierCounter == THREAD_COUNT)
        {
            phaseEnd = std::chrono::steady_clock::now();
            barrierCounter = 0;
            generation++;
            phaseThreeTime = phaseEnd - phaseStart;
            cv.notify_all();
        }
        while (generation == generation_local)
        {
            cv.wait(lock);
        }
        std::cout << "Thread with id: " << id << " left the barrier in generation: " << generation_local << "\n";
    }
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    totalThreadTime[id] = end - start;
}

struct Order
{
    int id;
    double price;
    int quantity;
};

int main()
{
    std::vector<std::thread> workerThreadPool;
    for (int i = 0; i < THREAD_COUNT; i++)
    {
        workerThreadPool.emplace_back(workerThread, i);
    }
    for (auto &thread : workerThreadPool)
    {
        thread.join();
    }
    for (int i = 0; i < THREAD_COUNT; i++)
    {
        std::cout << "Thread id: " << i << ", Execution time: " << std::chrono::duration_cast<std::chrono::milliseconds>(totalThreadTime[i]).count() << "\n";
    }
    std::cout << "Phase zero time: " << std::chrono::duration_cast<std::chrono::microseconds>(phaseZeroTime).count() << "muS. \n";
    std::cout << "Phase one time: " << std::chrono::duration_cast<std::chrono::microseconds>(phaseOneTime).count() << "muS. \n";
    std::cout << "Phase two time: " << std::chrono::duration_cast<std::chrono::microseconds>(phaseTwoTime).count() << "muS. \n";
    std::cout << "Phase three time: " << std::chrono::duration_cast<std::chrono::microseconds>(phaseThreeTime).count() << "muS. \n";
}
