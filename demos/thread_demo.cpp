#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <cmath>

// Worker function that performs some computation
void worker_task(int id, int iterations) {
    auto start = std::chrono::high_resolution_clock::now();
    
    // Do some work (calculate sum of square roots)
    double result = 0.0;
    for (int i = 1; i <= iterations; ++i) {
        result += std::sqrt(i);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    
    // Calculate elapsed time in different units
    auto duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    auto duration_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Thread " << id << " completed:\n"
              << "  Result: " << result << "\n"
              << "  Time: " << duration_ns.count() << " nanoseconds\n"
              << "  Time: " << duration_us.count() << " microseconds\n"
              << "  Time: " << duration_ms.count() << " milliseconds\n\n";
}

int main() {
    std::cout << "=== Thread and Timing Demo ===\n\n";
    
    // Measure total execution time
    auto program_start = std::chrono::high_resolution_clock::now();
    
    const int num_threads = 4;
    const int iterations = 10000000;
    
    std::vector<std::thread> threads;
    
    std::cout << "Launching " << num_threads << " threads...\n\n";
    
    // Create threads
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(worker_task, i, iterations);
    }
    
    // Wait for all threads to complete
    for (auto& t : threads) {
        t.join();
    }
    
    auto program_end = std::chrono::high_resolution_clock::now();
    
    // Calculate total program time
    auto total_duration = std::chrono::duration_cast<std::chrono::microseconds>(
        program_end - program_start
    );
    
    std::cout << "All threads completed!\n";
    std::cout << "Total execution time: " << total_duration.count() 
              << " microseconds (" << (total_duration.count() / 1000.0) 
              << " milliseconds)\n";
    
    // Demonstrate high-resolution timing for short operations
    std::cout << "\n=== Nanosecond Precision Demo ===\n";
    
    auto ns_start = std::chrono::high_resolution_clock::now();
    volatile int x = 0;
    for (int i = 0; i < 1000; ++i) {
        x += i;
    }
    auto ns_end = std::chrono::high_resolution_clock::now();
    
    auto ns_duration = std::chrono::duration_cast<std::chrono::nanoseconds>(
        ns_end - ns_start
    );
    
    std::cout << "Small loop (1000 iterations) took: " 
              << ns_duration.count() << " nanoseconds\n";
    
    return 0;
}
