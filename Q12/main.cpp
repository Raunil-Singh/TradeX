#include <vector>
#include <random>
#include <iostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <cmath>

struct Position
{
    bool is_complex;
    double price;
    double quantity;
    double strike;
    double volatility;
    double time_to_expiry;
    double risk_free_rate;
};

std::vector<Position> initialize_portfolio()
{
    std::vector<Position> positions(10000);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> price_dist(50.0, 150.0);
    std::uniform_real_distribution<> qty_dist(100.0, 1000.0);
    std::uniform_real_distribution<> strike_dist(50.0, 150.0);
    std::uniform_real_distribution<> vol_dist(0.15, 0.40);
    std::uniform_real_distribution<> time_dist(0.1, 2.0);

    for (int i = 0; i < 8000; i++)
    {
        positions[i].is_complex = false;
        positions[i].price = price_dist(gen);
        positions[i].quantity = qty_dist(gen);
    }

    for (int i = 8000; i < 10000; i++)
    {
        positions[i].is_complex = true;
        positions[i].price = price_dist(gen);
        positions[i].quantity = qty_dist(gen);
        positions[i].strike = strike_dist(gen);
        positions[i].volatility = vol_dist(gen);
        positions[i].time_to_expiry = time_dist(gen);
        positions[i].risk_free_rate = 0.05;
    }

    return positions;
}

double norm_cdf(double x)
{
    return 0.5 * erfc(-x * M_SQRT1_2);
}

double black_scholes(double S, double K, double r, double sigma, double T)
{
    double d1 = (log(S / K) + (r + 0.5 * sigma * sigma) * T) / (sigma * sqrt(T));
    double d2 = d1 - sigma * sqrt(T);
    return S * norm_cdf(d1) - K * exp(-r * T) * norm_cdf(d2);
}

double value_position(const Position &pos)
{
    if (pos.is_complex)
    {
        double option_price = black_scholes(pos.price, pos.strike, pos.risk_free_rate, pos.volatility, pos.time_to_expiry);
        return option_price * pos.quantity;
    }
    else
    {
        return pos.price * pos.quantity;
    }
}

std::mutex total_mutex;

void worker_thread_cyclic(const std::vector<Position> &positions, int thread_id, int num_threads, double &total)
{
    double local_sum = 0.0;

    for (size_t i = thread_id; i < positions.size(); i += num_threads)
    {
        local_sum += value_position(positions[i]);
    }

    {
        std::lock_guard<std::mutex> lock(total_mutex);
        total += local_sum;
    }
}

double valuate_portfolio_cyclic(const std::vector<Position> &positions, int num_threads)
{
    double total = 0.0;
    std::vector<std::thread> threads;

    for (int t = 0; t < num_threads; t++)
    {
        threads.emplace_back(worker_thread_cyclic, std::ref(positions), t, num_threads, std::ref(total));
    }

    for (auto &th : threads)
    {
        th.join();
    }

    return total;
}

struct BenchmarkResult
{
    int num_threads;
    double time_ms;
    double speedup;
    double efficiency;
};

std::vector<BenchmarkResult> run_benchmarks(const std::vector<Position> &positions)
{
    std::vector<int> thread_counts = {1, 2, 4, 8, 16};
    std::vector<BenchmarkResult> results;

    double baseline_time = 0.0;

    for (int num_threads : thread_counts)
    {
        valuate_portfolio_cyclic(positions, num_threads);

        double total_time = 0.0;
        for (int run = 0; run < 3; run++)
        {
            auto start = std::chrono::high_resolution_clock::now();

            double total_value = valuate_portfolio_cyclic(positions, num_threads);

            auto end = std::chrono::high_resolution_clock::now();
            double time_ms = std::chrono::duration<double, std::milli>(end - start).count();
            total_time += time_ms;
        }
        double avg_time = total_time / 3.0;

        if (num_threads == 1)
        {
            baseline_time = avg_time;
        }

        BenchmarkResult result;
        result.num_threads = num_threads;
        result.time_ms = avg_time;
        result.speedup = baseline_time / avg_time;
        result.efficiency = (result.speedup / num_threads) * 100.0;

        results.push_back(result);
    }

    return results;
}

void print_results(const std::vector<BenchmarkResult> &results)
{
    std::cout << "\n=========== BENCHMARK RESULTS =========\n";
    std::cout << "Threads | Time(ms) | Speedup | Efficiency\n";
    std::cout << "--------|----------|---------|------------\n";

    for (const auto &r : results)
    {
        printf("%7d | %8.2f | %7.2fx | %9.1f%%\n", r.num_threads, r.time_ms, r.speedup, r.efficiency);
    }

    int optimal = 1;
    double best_efficiency = 0.0;

    for (const auto &r : results)
    {
        if (r.efficiency >= 70.0 && r.efficiency > best_efficiency)
        {
            best_efficiency = r.efficiency;
            optimal = r.num_threads;
        }
    }

    std::cout << "\nOptimal thread count: " << optimal << " threads\n";

    double parallel_fraction = 0.85;
    std::cout << "\nAmdahl's Law predictions:\n";
    for (const auto &r : results)
    {
        double predicted = 1.0 / ((1.0 - parallel_fraction) + parallel_fraction / r.num_threads);
        printf("%2d threads: Predicted %.2fx, Actual %.2fx\n",
               r.num_threads, predicted, r.speedup);
    }
}

int main()
{
    std::vector<Position> positions = initialize_portfolio();
    std::vector<BenchmarkResult> results = run_benchmarks(positions);
    print_results(results);
    return 0;
}