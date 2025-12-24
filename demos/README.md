# Demo Programs

This directory contains demonstration programs to help you understand thread creation and timing concepts before starting the main tasks.

## Programs

### 1. `thread_demo.cpp`
Complete demonstration combining threading and timing.
- Creates multiple worker threads
- Measures execution time in nanoseconds, microseconds, and milliseconds
- Shows parallel execution of tasks

**Compile and run:**
```bash
g++ -std=c++17 -pthread -O2 thread_demo.cpp -o thread_demo
./thread_demo
```

### 2. `thread_basics.cpp`
Comprehensive guide to thread creation patterns.
- Simple function threads
- Threads with parameters
- Reference parameters with `std::ref()`
- Lambda function threads
- Functor threads
- Parallel computation example
- Thread information (ID, hardware concurrency)

**Compile and run:**
```bash
g++ -std=c++17 -pthread -O2 thread_basics.cpp -o thread_basics
./thread_basics
```

### 3. `timing_precision.cpp`
Deep dive into chrono timing capabilities.
- Nanosecond precision measurements
- Microsecond precision measurements
- Sleep precision and overhead
- Time point arithmetic
- Clock comparison (high_resolution, steady, system)
- Unit conversions

**Compile and run:**
```bash
g++ -std=c++17 -pthread -O2 timing_precision.cpp -o timing_precision
./timing_precision
```

## Quick Start

Run all demos:
```bash
cd demos

# Compile all
g++ -std=c++17 -pthread -O2 thread_demo.cpp -o thread_demo
g++ -std=c++17 -pthread -O2 thread_basics.cpp -o thread_basics
g++ -std=c++17 -pthread -O2 timing_precision.cpp -o timing_precision

# Run all
./thread_demo
./thread_basics
./timing_precision
```

## What You'll Learn

### Threading Concepts
- Creating and managing threads
- Passing parameters to threads
- Using references with `std::ref()`
- Lambda functions as thread targets
- Joining threads
- Parallel computation patterns

### Timing Concepts
- High-resolution time measurement
- Nanosecond, microsecond, millisecond precision
- Different clock types
- Time point arithmetic
- Duration conversions
- Performance measurement best practices

## Important Notes

1. **Always join threads**: Every thread must be joined or detached
2. **Use std::ref()**: Required for passing references to threads
3. **Choose the right clock**: Use `steady_clock` for performance measurements
4. **Understand precision vs accuracy**: Nanosecond precision doesn't mean nanosecond accuracy
5. **Consider OS overhead**: Sleep functions and context switches have overhead

## Next Steps

After understanding these demos:
1. Complete Q0 (perf profiling prerequisite)
2. Start the main curriculum tasks in README.md
3. Apply these concepts to build high-performance multi-threaded systems

## Troubleshooting

**Compilation errors:**
```bash
# Make sure you have g++ installed
g++ --version

# Ensure you use -pthread flag
g++ -std=c++17 -pthread yourfile.cpp -o yourfile
```

**Permission denied:**
```bash
chmod +x thread_demo thread_basics timing_precision
```

**Segmentation fault:**
- Ensure all threads are joined before program exit
- Check for data races if you modify the examples
