#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <random>
#include <chrono>
#include <cmath>


const int POSITION_COUNT = 1'000'000;
const double RISK_FREE_RATE = 0.01;
const double VOLATILITY = 0.2;

struct Position
{
    double price;
    double quantity;
    bool is_option;
};

std::vector<Position> generatePortfolio()
{
    std::vector<Position> portfolio;
    portfolio.reserve(POSITION_COUNT);

    std::mt19937 mt(std::random_device{}());
    std::uniform_real_distribution<double> price_dist(50.0, 150.0);
    std::uniform_real_distribution<double> qty_dist(1.0, 100.0);
    std::bernoulli_distribution option_dist(0.2);

    for (int i = 0; i < POSITION_COUNT; i++)
    {
        portfolio.push_back({ price_dist(mt), qty_dist(mt), option_dist(mt) });
    }

    return portfolio;
}

double valueSimple(const Position& p)
{
    return p.price * p.quantity;
}

double blackScholes(const Position& p)
{
    double S = p.price;
    double K = p.price * 1.05;
    double T = 1.5;

    double value = 0.0;

    for (int i = 0; i < 50; i++)
    {
        double d1 = (std::log(S / K)
                    + (RISK_FREE_RATE + 0.5 * VOLATILITY * VOLATILITY) * T)
                    / (VOLATILITY * std::sqrt(T));

        double d2 = d1 - VOLATILITY * std::sqrt(T);

        value += S * std::exp(-d2 * d2 / 2.0);
        S *= 1.0001;
    }

    return value * p.quantity;
}


void worker(const std::vector<Position>& portfolio, int start, int end,
            double& global_sum, std::mutex& sum_mutex)
{
    double local_sum = 0.0;

    for (int i = start; i < end; i++)
    {
        if (portfolio[i].is_option)
            local_sum += blackScholes(portfolio[i]);
        else
            local_sum += valueSimple(portfolio[i]);
    }

    std::lock_guard<std::mutex> lock(sum_mutex);
    global_sum += local_sum;
}

template <typename F>
long long measure(F f)
{
    auto start = std::chrono::high_resolution_clock::now();
    f();
    auto end = std::chrono::high_resolution_clock::now();

    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}

long long runParallel(const std::vector<Position>& portfolio, int thread_count)
{
    std::vector<std::thread> threads;
    std::mutex sum_mutex;
    double total_value = 0.0;

    int section = portfolio.size() / thread_count;

    auto time = measure([&] {
        for (int i = 0; i < thread_count; i++)
        {
            int start = i * section;
            int end = (i == thread_count - 1)
                    ? portfolio.size()
                    : start + section;

            threads.emplace_back(worker, std::ref(portfolio), start, end,
                                 std::ref(total_value), std::ref(sum_mutex));
        }

        for (auto& t : threads)
            t.join();
    });

    return time;
}

double speedup(long long t1, long long tn)
{
    return static_cast<double>(t1) / tn;
}

double efficiency(double speedup, int threads)
{
    return speedup / threads;
}

double amdahl(int threads, double parallel_fraction)
{
    return 1.0 / ((1.0 - parallel_fraction) + parallel_fraction / threads);
}

int main()
{
    auto portfolio = generatePortfolio();

    std::vector<int> thread_counts = {1, 2, 4, 8, 16};
    std::vector<long long> times;

    std::cout << "\nThread Scaling Analysis\n\n";

    for (int threads : thread_counts)
    {
        long long t = runParallel(portfolio, threads);
        times.push_back(t);

        std::cout << "Threads: " << threads << " Time: " << t << " ms\n";
    }

    long long t1 = times[0];

    std::cout << "\nSpeedup and Efficiency:\n\n";

    for (size_t i = 0; i < thread_counts.size(); i++)
    {
        double s = speedup(t1, times[i]);
        double e = efficiency(s, thread_counts[i]);
        double a = amdahl(thread_counts[i], 0.8);

        std::cout << "Threads: " << thread_counts[i]
                  << " Speedup: " << s
                  << " Efficiency: " << e
                  << " Amdahl: " << a << "\n";
    }
}

