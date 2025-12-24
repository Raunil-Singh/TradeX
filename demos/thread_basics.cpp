#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <chrono>

// Demonstrates various thread creation patterns

// 1. Simple function as thread
void simple_function(int id) {
    std::cout << "Thread " << id << " executing simple function\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::cout << "Thread " << id << " completed\n";
}

// 2. Function with multiple parameters
void function_with_params(int id, const std::string& message, double value) {
    std::cout << "Thread " << id << ": " << message 
              << " (value: " << value << ")\n";
}

// 3. Function with reference parameter (requires std::ref)
void function_with_reference(int& counter) {
    for (int i = 0; i < 5; ++i) {
        ++counter;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

// 4. Lambda functions as threads
void demonstrate_lambda_threads() {
    std::cout << "\n=== Lambda Function Threads ===\n";
    
    int shared_value = 100;
    
    // Lambda with capture by value
    std::thread t1([shared_value]() {
        std::cout << "Lambda captured value: " << shared_value << "\n";
    });
    
    // Lambda with capture by reference
    std::thread t2([&shared_value]() {
        shared_value += 50;
        std::cout << "Lambda modified shared value to: " << shared_value << "\n";
    });
    
    // Lambda with parameters
    std::thread t3([](int x, int y) {
        std::cout << "Lambda with params: " << x << " + " << y 
                  << " = " << (x + y) << "\n";
    }, 10, 20);
    
    t1.join();
    t2.join();
    t3.join();
    
    std::cout << "Final shared_value: " << shared_value << "\n";
}

// 5. Functor (function object) as thread
class Worker {
private:
    int id_;
    
public:
    Worker(int id) : id_(id) {}
    
    void operator()() const {
        std::cout << "Worker functor " << id_ << " executing\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        std::cout << "Worker functor " << id_ << " done\n";
    }
};

// 6. Thread with return value (using reference parameter)
void calculate_sum(const std::vector<int>& data, int start, int end, long long& result) {
    result = 0;
    for (int i = start; i < end && i < data.size(); ++i) {
        result += data[i];
    }
}

void demonstrate_thread_basics() {
    std::cout << "=== Basic Thread Creation ===\n";
    
    // Creating threads with simple function
    std::thread t1(simple_function, 1);
    std::thread t2(simple_function, 2);
    
    t1.join();
    t2.join();
}

void demonstrate_params_and_references() {
    std::cout << "\n=== Threads with Parameters ===\n";
    
    std::thread t1(function_with_params, 1, "Hello from thread", 3.14);
    std::thread t2(function_with_params, 2, "Another message", 2.71);
    
    t1.join();
    t2.join();
    
    std::cout << "\n=== Thread with Reference Parameter ===\n";
    int counter = 0;
    std::cout << "Counter before: " << counter << "\n";
    
    // Must use std::ref for reference parameters
    std::thread t3(function_with_reference, std::ref(counter));
    t3.join();
    
    std::cout << "Counter after: " << counter << "\n";
}

void demonstrate_functors() {
    std::cout << "\n=== Functor Threads ===\n";
    
    std::vector<std::thread> threads;
    
    for (int i = 0; i < 3; ++i) {
        threads.emplace_back(Worker(i));
    }
    
    for (auto& t : threads) {
        t.join();
    }
}

void demonstrate_parallel_computation() {
    std::cout << "\n=== Parallel Sum Computation ===\n";
    
    // Create data
    std::vector<int> data(1000000);
    for (int i = 0; i < data.size(); ++i) {
        data[i] = i + 1;
    }
    
    // Split work among threads
    const int num_threads = 4;
    std::vector<std::thread> threads;
    std::vector<long long> partial_sums(num_threads);
    
    int chunk_size = data.size() / num_threads;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_threads; ++i) {
        int start_idx = i * chunk_size;
        int end_idx = (i == num_threads - 1) ? data.size() : (i + 1) * chunk_size;
        
        threads.emplace_back(calculate_sum, 
                           std::ref(data), 
                           start_idx, 
                           end_idx, 
                           std::ref(partial_sums[i]));
    }
    
    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }
    
    // Combine results
    long long total_sum = 0;
    for (long long partial : partial_sums) {
        total_sum += partial;
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
        end_time - start_time
    );
    
    std::cout << "Computed sum: " << total_sum << "\n";
    std::cout << "Time taken: " << duration.count() << " μs\n";
    std::cout << "Using " << num_threads << " threads\n";
}

void demonstrate_thread_info() {
    std::cout << "\n=== Thread Information ===\n";
    
    std::cout << "Hardware concurrency: " 
              << std::thread::hardware_concurrency() << " threads\n";
    
    std::thread t([]() {
        std::cout << "Thread ID: " << std::this_thread::get_id() << "\n";
    });
    
    std::cout << "Main thread ID: " << std::this_thread::get_id() << "\n";
    std::cout << "Created thread ID: " << t.get_id() << "\n";
    
    t.join();
}

int main() {
    std::cout << "╔════════════════════════════════════════╗\n";
    std::cout << "║  Thread Creation Basics Demo          ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";
    
    demonstrate_thread_basics();
    demonstrate_params_and_references();
    demonstrate_lambda_threads();
    demonstrate_functors();
    demonstrate_parallel_computation();
    demonstrate_thread_info();
    
    std::cout << "\n=== Key Concepts ===\n";
    std::cout << "1. Always join() or detach() threads before they go out of scope\n";
    std::cout << "2. Use std::ref() for reference parameters\n";
    std::cout << "3. Lambda captures can be by value [=] or reference [&]\n";
    std::cout << "4. Thread ID is unique and can be used for identification\n";
    std::cout << "5. hardware_concurrency() suggests optimal thread count\n";
    
    return 0;
}
