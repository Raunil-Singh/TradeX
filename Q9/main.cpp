#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <random>
#include <condition_variable>
#include <array>

std::mutex cout_mtx;
class Barrier
{
private:
    int count;
    int total_threads;
    int generation;
    std::mutex mtx;
    std::condition_variable cv;

public:
    Barrier(int num_threads) : count(0), total_threads(num_threads), generation(0) {};

    bool wait()
    {
        std::unique_lock<std::mutex> lock(mtx);
        int my_gen = generation;

        count++;

        if (count == total_threads)
        {
            generation++;
            count = 0;
            cv.notify_all();
            return true;
        }
        else
        {
            cv.wait(lock, [this, my_gen] { return my_gen != generation; });
            return false;
        }
    }
};

class PhaseTimer
{
private:
    std::mutex mtx;
    std::chrono::high_resolution_clock::time_point start_time;
    std::chrono::high_resolution_clock::time_point end_time;
    bool started;

public:
    explicit PhaseTimer(int num_threads) : started(false) {}

    void phase_start()
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (!started)
        {
            start_time = std::chrono::high_resolution_clock::now();
            started = true;
        }
    }

    void phase_end()
    {
        std::lock_guard<std::mutex> lock(mtx);
        end_time = std::chrono::high_resolution_clock::now();
    }

    double get_duration_ms()
    {
        std::lock_guard<std::mutex> lock(mtx);
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            end_time - start_time);
        return duration.count() / 1000.0;
    }

    void reset()
    {
        std::lock_guard<std::mutex> lock(mtx);
        started = false;
    }
};

void workers_with_barriers(int thread_id, std::array<Barrier, 4> &barriers, std::array<PhaseTimer, 4> &timers)
{
    std::random_device rd;
    std::mt19937 gen(rd() + thread_id);

    struct PhaseConfig
    {
        int min_ms;
        int max_ms;
        Barrier *barrier;
        PhaseTimer *timer;
        const char *thread_name;
    };

    PhaseConfig phase_configs[] = {
        {50, 100, &barriers[0], &timers[0], "Order Validation"},
        {30, 80, &barriers[1], &timers[1], "Risk Checking"},
        {40, 90, &barriers[2], &timers[2], "Order Matching"},
        {20, 60, &barriers[3], &timers[3], "Trade Reporting"}};

    for (int i = 0; i < 4; i++)
    {
        PhaseConfig &config = phase_configs[i];

        config.timer->phase_start();

        std::uniform_int_distribution<> dis(config.min_ms, config.max_ms);
        int work_time = dis(gen);

        {
            std::lock_guard<std::mutex> lock(cout_mtx);
            std::cout << "Thread " << thread_id << " has entered " << config.thread_name << " phase." << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(work_time));

        {
            std::lock_guard<std::mutex> lock(cout_mtx);
            std::cout << "Thread " << thread_id << " has entered the barrier" << std::endl;
        }
        
        bool last = config.barrier->wait();
        if(last) config.timer->phase_end();

        {
            std::lock_guard<std::mutex> lock(cout_mtx);
            std::cout << "Thread " << thread_id << " has left the barrier" << std::endl;
        }
    }
}

void workers_without_barriers(int thread_id, std::array<PhaseTimer, 4> &timers)
{
    std::random_device rd;
    std::mt19937 gen(rd() + thread_id);

    struct PhaseConfig
    {
        int min_ms;
        int max_ms;
        PhaseTimer *timer;
        const char *thread_name;
    };

    PhaseConfig phase_configs[] = {
        {50, 100, &timers[0], "Order Validation"},
        {30, 80, &timers[1], "Risk Checking"},
        {40, 90, &timers[2], "Order Matching"},
        {20, 60, &timers[3], "Trade Reporting"}};

    for (int i = 0; i < 4; i++)
    {
        PhaseConfig &config = phase_configs[i];

        config.timer->phase_start();

        std::uniform_int_distribution<> dis(config.min_ms, config.max_ms);
        int work_time = dis(gen);

        {
            std::lock_guard<std::mutex> lock(cout_mtx);
            std::cout << "Thread " << thread_id << " has entered " << config.thread_name << " phase." << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(work_time));

        {
            std::lock_guard<std::mutex> lock(cout_mtx);
            std::cout << "Thread " << thread_id << " has finished the phase" << std::endl;
        }
        config.timer->phase_end();
    }
}

void with_barriers()
{
    std::array<Barrier, 4> barriers = {Barrier(4), Barrier(4), Barrier(4), Barrier(4)};

    std::array<PhaseTimer, 4> timers = {PhaseTimer(4), PhaseTimer(4), PhaseTimer(4), PhaseTimer(4)};

    std::vector<std::thread> threads;
    for (int i = 0; i < 4; i++)
    {
        threads.emplace_back(workers_with_barriers, i, std::ref(barriers), std::ref(timers));
    }

    for (auto &p : threads)
        p.join();

    for (int i = 0; i < 4; i++)
    {
        double duration = timers[i].get_duration_ms();
        std::cout << "Execution time of Phase " << i + 1 << " : " << duration << std::endl;  //Time between earliest start and last end of the phase
    }
}

void without_barriers()
{

    std::array<PhaseTimer, 4> timers = {PhaseTimer(4), PhaseTimer(4), PhaseTimer(4), PhaseTimer(4)};

    std::vector<std::thread> threads;
    for (int i = 0; i < 4; i++)
    {
        threads.emplace_back(workers_without_barriers, i , std::ref(timers));
    }

    for (auto &p : threads)
        p.join();

    for (int i = 0; i < 4; i++)
    {
        double duration = timers[i].get_duration_ms();
        std::cout << "Execution time of Phase " << i + 1 << " : " << duration << std::endl;
    }
}

int main()
{
    auto start1 = std::chrono::high_resolution_clock::now();
    with_barriers();
    auto end1 = std::chrono::high_resolution_clock::now();
    auto duration1 = std::chrono::duration_cast<std::chrono::milliseconds>(end1 - start1);
    std::cout << "Total Execution time with barriers : " << duration1.count() << "ms\n";

    std::cout << "\n\n";

    auto start2 = std::chrono::high_resolution_clock::now();
    without_barriers();
    auto end2 = std::chrono::high_resolution_clock::now();
    auto duration2 = std::chrono::duration_cast<std::chrono::milliseconds>(end2 - start2);
    std::cout << "Total Execution time without barriers : " << duration2.count() << "ms\n";
}