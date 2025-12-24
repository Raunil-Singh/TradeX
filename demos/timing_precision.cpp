#include <iostream>
#include <chrono>
#include <thread>
#include <cmath>
#include <iomanip>

// Demonstrates different time measurement precisions

void demonstrate_nanosecond_precision() {
    std::cout << "=== Nanosecond Precision ===\n";
    
    // Measure a very short operation
    auto start = std::chrono::high_resolution_clock::now();
    
    volatile int sum = 0;
    for (int i = 0; i < 100; ++i) {
        sum += i;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    
    std::cout << "100 iterations: " << duration.count() << " ns\n";
    std::cout << "Average per iteration: " 
              << (duration.count() / 100.0) << " ns\n\n";
}

void demonstrate_microsecond_precision() {
    std::cout << "=== Microsecond Precision ===\n";
    
    // Measure a moderate operation
    auto start = std::chrono::high_resolution_clock::now();
    
    volatile double sum = 0.0;
    for (int i = 0; i < 10000; ++i) {
        sum += std::sqrt(i);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "10,000 sqrt operations: " << duration.count() << " μs\n";
    std::cout << "That's " << (duration.count() / 1000.0) << " ms\n\n";
}

void demonstrate_sleep_precision() {
    std::cout << "=== Sleep Precision Test ===\n";
    
    // Test how accurate sleep_for is
    auto requested = std::chrono::microseconds(100);
    
    auto start = std::chrono::high_resolution_clock::now();
    std::this_thread::sleep_for(requested);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto actual_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    auto actual_us = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Requested sleep: 100 μs\n";
    std::cout << "Actual sleep: " << actual_us.count() << " μs (" 
              << actual_ns.count() << " ns)\n";
    std::cout << "Overhead: " << (actual_us.count() - requested.count()) << " μs\n\n";
}

void demonstrate_time_point_arithmetic() {
    std::cout << "=== Time Point Arithmetic ===\n";
    
    auto now = std::chrono::high_resolution_clock::now();
    
    // Add durations to time points
    auto future_us = now + std::chrono::microseconds(1000);
    auto future_ms = now + std::chrono::milliseconds(1);
    
    // Calculate differences
    auto diff = std::chrono::duration_cast<std::chrono::nanoseconds>(future_us - now);
    
    std::cout << "Added 1000 μs to current time\n";
    std::cout << "Difference: " << diff.count() << " ns\n";
    std::cout << "Which is: " << (diff.count() / 1000.0) << " μs\n\n";
}

void demonstrate_steady_vs_system_clock() {
    std::cout << "=== Clock Comparison ===\n";
    
    // High resolution clock (typically steady_clock)
    auto hr_start = std::chrono::high_resolution_clock::now();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    auto hr_end = std::chrono::high_resolution_clock::now();
    auto hr_duration = std::chrono::duration_cast<std::chrono::microseconds>(hr_end - hr_start);
    
    // Steady clock (monotonic, good for measuring intervals)
    auto steady_start = std::chrono::steady_clock::now();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    auto steady_end = std::chrono::steady_clock::now();
    auto steady_duration = std::chrono::duration_cast<std::chrono::microseconds>(steady_end - steady_start);
    
    // System clock (wall clock time, can jump)
    auto system_start = std::chrono::system_clock::now();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    auto system_end = std::chrono::system_clock::now();
    auto system_duration = std::chrono::duration_cast<std::chrono::microseconds>(system_end - system_start);
    
    std::cout << "10ms sleep measured by different clocks:\n";
    std::cout << "  high_resolution_clock: " << hr_duration.count() << " μs\n";
    std::cout << "  steady_clock: " << steady_duration.count() << " μs\n";
    std::cout << "  system_clock: " << system_duration.count() << " μs\n\n";
    
    std::cout << "Note: Use steady_clock for performance measurements\n";
    std::cout << "      Use system_clock for wall-clock time (timestamps)\n\n";
}

void demonstrate_conversion_between_units() {
    std::cout << "=== Unit Conversions ===\n";
    
    auto duration = std::chrono::nanoseconds(1500000); // 1.5 milliseconds
    
    auto as_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
    auto as_us = std::chrono::duration_cast<std::chrono::microseconds>(duration);
    auto as_ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
    
    std::cout << "Original: 1,500,000 nanoseconds\n";
    std::cout << "As nanoseconds: " << as_ns.count() << " ns\n";
    std::cout << "As microseconds: " << as_us.count() << " μs\n";
    std::cout << "As milliseconds: " << as_ms.count() << " ms (truncated)\n\n";
    
    // Using double for precise conversion
    std::cout << "Precise conversions using double:\n";
    std::cout << "As microseconds: " << (duration.count() / 1000.0) << " μs\n";
    std::cout << "As milliseconds: " << (duration.count() / 1000000.0) << " ms\n";
    std::cout << "As seconds: " << (duration.count() / 1000000000.0) << " s\n\n";
}

int main() {
    std::cout << std::fixed << std::setprecision(2);
    
    std::cout << "╔════════════════════════════════════════╗\n";
    std::cout << "║     Chrono Timing Precision Demos      ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";
    
    demonstrate_nanosecond_precision();
    demonstrate_microsecond_precision();
    demonstrate_sleep_precision();
    demonstrate_time_point_arithmetic();
    demonstrate_steady_vs_system_clock();
    demonstrate_conversion_between_units();
    
    std::cout << "Key Takeaways:\n";
    std::cout << "1. Use high_resolution_clock or steady_clock for benchmarking\n";
    std::cout << "2. Nanosecond precision is available but OS scheduling affects accuracy\n";
    std::cout << "3. Sleep functions have overhead (typically 50-200 μs)\n";
    std::cout << "4. Always measure execution time around critical code sections\n";
    std::cout << "5. Use duration_cast<> to convert between time units\n";
    
    return 0;
}
