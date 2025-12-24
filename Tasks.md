# Multi-Threading and Concurrency Tasks

**Context**: These tasks prepare you for building a high-performance exchange-side trading system where multiple market participants submit orders concurrently, and the system must process them efficiently while maintaining data consistency.

Every task must have its own directory. Implement a solution and add screenshots to the screenshots directory within. Execute each program 2-3 times to make sure the output is consistent.

**Language**: C++ (using `<thread>`, `<mutex>`, `<condition_variable>`, `<atomic>`, `<chrono>` from C++11 and later)

---

## Q0. Performance Profiling with Linux `perf` (Prerequisite)

Before diving into multi-threading problems, learn to use `perf` - the essential tool for performance analysis in Linux. This profiler will be invaluable throughout all subsequent questions.

### Part A: Basic perf Commands

Write a simple C++ program that performs three computational tasks:
1. **CPU-bound loop**: Sum integers from 1 to 1,000,000,000
2. **Cache-friendly access**: Sum elements in array[10,000,000] sequentially (row-major)
3. **Cache-unfriendly access**: Sum elements in 2D array[5000][2000] column-major

Compile with: `g++ -O2 -g program.cpp -o program`

**Run and analyze with perf**:
```bash
# Record performance counters
perf stat ./program

# Record with specific events
perf stat -e cycles,instructions,cache-references,cache-misses,branches,branch-misses ./program

# Calculate Instructions Per Cycle (IPC) and cache hit rate
perf stat -e cycles,instructions,L1-dcache-loads,L1-dcache-load-misses ./program
```

**Document**:
- Total cycles and instructions executed
- IPC (Instructions Per Cycle): higher is better (>1.0 is good)
- Cache miss rate: cache-misses / cache-references (lower is better, <5% is good)
- Branch misprediction rate: branch-misses / branches (lower is better, <2% is good)
- Compare cache miss rates between cache-friendly and cache-unfriendly code

### Part B: Profiling Hotspots

Write a program with multiple functions having different CPU usage:
```cpp
void fast_function() { /* lightweight: 1M additions */ }
void slow_function() { /* heavy: 1M multiplications + divisions */ }
void very_slow_function() { /* very heavy: nested loops with 100M operations */ }
```

**Run sampling profiler**:
```bash
# Record CPU samples
perf record -g ./program

# Generate report
perf report

# Generate annotated source code (shows which lines are hot)
perf annotate
```

**Document**:
- Which function consumes most CPU time (from `perf report`)?
- What percentage of total execution is each function?
- Within the slowest function, which lines take the most time (from `perf annotate`)?

### Part C: Multi-threaded Analysis

Write a multi-threaded program (4 threads, each incrementing a counter 10,000,000 times):
- **Version 1**: Shared counter with `std::mutex`
- **Version 2**: Shared counter with `std::atomic`
- **Version 3**: Thread-local counters (no sharing)

**Analyze with perf**:
```bash
# Compare cache coherence traffic
perf stat -e cache-references,cache-misses,cpu-migrations,context-switches ./program_v1
perf stat -e cache-references,cache-misses,cpu-migrations,context-switches ./program_v2
perf stat -e cache-references,cache-misses,cpu-migrations,context-switches ./program_v3

# Detailed thread analysis
perf record -g --call-graph dwarf ./program_v1
perf report --sort comm,dso,symbol
```

**Document**:
- Cache miss rates for each version (mutex vs atomic vs thread-local)
- Number of context switches (indicates lock contention)
- CPU migrations (threads moving between cores)
- Explain why thread-local version has lowest cache misses

### Part D: Flame Graphs (Bonus) [Optional]

Install FlameGraph tools:
```bash
git clone https://github.com/brendangregg/FlameGraph
cd FlameGraph
```

Generate flame graph:
```bash
perf record -F 99 -a -g -- ./program
perf script | ./FlameGraph/stackcollapse-perf.pl | ./FlameGraph/flamegraph.pl > flame.svg
```

Open `flame.svg` in browser and identify hot code paths visually.

---

**Learning Target**: Master `perf` for performance analysis - identify CPU bottlenecks, cache behavior, branch prediction, and lock contention. This tool will be essential for all subsequent optimization tasks.

**Trading System Context**: In production trading systems, `perf` is used to identify latency hotspots, optimize cache usage, and eliminate lock contention. Every microsecond matters.

**Expected Outcome**: Understand how to measure IPC, cache misses, branch mispredictions. See dramatic differences in cache behavior between synchronized access patterns. Gain proficiency with `perf stat`, `perf record`, and `perf report`.

**Key Metrics to Master**:
- **IPC (Instructions Per Cycle)**: Measures CPU efficiency
- **Cache miss rate**: Indicates memory access patterns
- **Branch misprediction rate**: Shows branch prediction quality
- **Context switches**: Indicates thread contention/blocking
- **CPU migrations**: Shows CPU affinity issues

---

## Section 1: Thread Fundamentals (3 questions)

### Q1. Concurrent Execution and Race Conditions
Write a C++ program that demonstrates both thread creation and race conditions:
- Create 4 threads: 2 threads increment a shared counter 500,000 times, 2 threads decrement it 500,000 times
- Implement two versions in the same program:
  - **Unsynchronized**: No locks (observe inconsistent results across runs)
  - **Synchronized**: Using `std::mutex` with `std::lock_guard` (consistent results)
- Pass thread parameters (for example, iteration count) to each thread
- Print thread IDs using `std::this_thread::get_id()` during execution
- Measure and compare execution time for both versions
- Run the program 3 times and document result variations

**Learning Target**: Understand thread creation, parameter passing, race conditions, and basic synchronization in one integrated problem. Learn why unsynchronized access to shared data is dangerous and how mutex provides correctness at a performance cost.

**Trading System Context**: Similar to multiple threads updating order book state - without proper synchronization, price levels or quantities can become corrupted.

**Expected Outcome**: Unsynchronized version produces inconsistent final values (e.g., -1523, 2847, -891). Synchronized version always produces 0 but takes slightly longer.

---

### Q2. Thread Pools and Work Distribution
Write a C++ program implementing a thread pool for processing order batches:
- Create a fixed thread pool of 4 worker threads
- Implement a task queue (use `std::queue` protected by `std::mutex`)
- Workers wait on `std::condition_variable` when queue is empty
- Submit 50 tasks (each task: compute sum of 100,000 random numbers and sleep 10ms to simulate work)
- Each task prints: task ID, processing thread ID, and computed result
- Implement proper shutdown mechanism (workers exit gracefully when no more tasks)
- Compare execution time vs sequential processing of all tasks

**Learning Target**: Build producer-consumer pattern with thread pool - understand condition variables for thread coordination, task queue management, and thread lifetime. Learn when thread reuse is better than spawning new threads.

**Trading System Context**: Directly applicable to order processing where a pool of worker threads handles incoming orders from a queue, avoiding overhead of creating new threads per order.

**Expected Outcome**: All tasks processed correctly with work distributed across 4 threads. ~12x faster than sequential (50 tasks / 4 threads).

---

### Q3. Reader-Writer Locks and Performance Analysis
Write a C++ program implementing reader-writer synchronization for market data access:
- Shared resource: Array of 1000 "prices" (doubles initialized to 100.0)
- Create 8 reader threads: each reads random prices 100,000 times and computes average
- Create 2 writer threads: each updates random prices 10,000 times (add small random value)
- Implement three versions:
  1. **Mutex only**: Single `std::mutex` for all access (readers block each other)
  2. **Shared mutex**: Use `std::shared_mutex` (C++17) - readers concurrent, writers exclusive
  3. **Fine-grained locking**: Divide array into 10 segments, each with own lock
- Measure execution time for each version
- Track and print: total reads, total writes, average wait time per thread

**Learning Target**: Understand different synchronization strategies and their performance implications. Learn when reader-writer locks outperform simple mutexes, and when fine-grained locking reduces contention.

**Trading System Context**: Similar to multiple threads querying order book state (readers) while fewer threads update it with new orders (writers). Read-heavy workloads benefit from shared locks.

**Expected Outcome**: Shared mutex version 3-4x faster than mutex-only. Fine-grained locking 5-6x faster due to reduced contention.

---

## Section 2: Classical Problems (3 questions)

### Q4. Producer-Consumer with Bounded Buffer
Implement the producer-consumer problem with realistic order processing constraints:
- Bounded buffer: capacity of 20 orders (use `std::queue` with manual size tracking)
- 3 producer threads: generate orders (order ID, price, quantity) at different rates
  - Fast producer: 10ms delay between orders
  - Medium producer: 25ms delay
  - Slow producer: 50ms delay
- 2 consumer threads: process orders (sleep 15ms per order to simulate matching)
- Use `std::mutex` and two `std::condition_variable` objects (one for "not full", one for "not empty")
- Producers block when buffer is full, consumers block when empty
- Run for 5 seconds, then signal producers to stop and consumers to drain remaining orders
- Track: total orders produced by each producer, total consumed by each consumer, max buffer occupancy

**Learning Target**: Master condition variables for complex coordination - handle multiple producers/consumers with different rates, bounded resources, and graceful shutdown. Understand blocking vs busy-waiting.

**Trading System Context**: Models order ingestion queue where gateway threads produce orders at varying rates and matching engine threads consume them. Buffer prevents fast producers from overwhelming slow consumers.

**Expected Outcome**: All produced orders consumed exactly once. No lost orders. Buffer occupancy oscillates based on producer/consumer rate mismatch.

---

### Q5. Dining Philosophers with Deadlock Analysis
Implement the dining philosophers problem with deadlock detection and prevention:
- 5 philosophers, 5 forks (represented as `std::mutex`)
- Each philosopher: think (random 100-300ms) → try to pick up forks → eat (random 100-200ms) → put down forks
- Implement three versions:
  1. **Naive**: All philosophers pick left fork first, then right (will deadlock)
  2. **Resource ordering**: Odd philosophers pick left first, even pick right first (no deadlock)
  3. **Timeout-based**: Use `std::timed_mutex` with `try_lock_for(100ms)` - if timeout, release held forks and retry
- Run each version for 30 seconds (except naive which should deadlock within seconds)
- Detect deadlock in naive version: if no philosopher eats for 5 seconds, report deadlock
- Track: how many times each philosopher ate, average wait time before eating

**Learning Target**: Understand deadlock causes and prevention strategies - resource ordering, timeout-based recovery. Learn to detect and handle deadlock situations. Recognize circular wait conditions.

**Trading System Context**: Similar to preventing deadlock when threads need multiple locks (order book + position manager + risk limits) simultaneously. Wrong locking order causes deadlock.

**Expected Outcome**: Naive version deadlocks quickly. Resource ordering runs smoothly. Timeout version occasionally retries but makes progress.

---

### Q6. Parallel Merge Sort with Work Stealing
Implement parallel merge sort with dynamic load balancing:
- Sort array of 1,000,000 random integers
- Implement recursive divide-and-conquer with threading:
  - Spawn new thread for recursive calls only if depth < threshold (e.g., 4 levels deep)
  - Below threshold, use sequential sort to avoid excessive thread creation
- Compare three approaches:
  1. **Sequential**: Standard merge sort, single thread
  2. **Fixed parallelism**: Always spawn threads for first N levels (measure with 2, 4, 8 threads)
  3. **Work stealing**: Use `std::async` with automatic task scheduling
- Measure execution time and calculate speedup
- Experiment with array sizes: 100K, 1M, 10M elements
- Determine optimal parallelism threshold depth

**Learning Target**: Apply divide-and-conquer parallelism with careful granularity control. Understand when parallelization overhead exceeds benefits. Learn about work-stealing schedulers.

**Trading System Context**: Similar to parallel sorting of orders by price-time priority or parallel processing of bulk order submissions where work distribution is critical.

**Expected Outcome**: Fixed parallelism achieves 3-5x speedup for large arrays. Too many threads hurt performance. Work stealing with `std::async` automatically balances load.

---

## Section 3: Advanced Synchronization (3 questions)

### Q7. Lock-Free Atomic Operations
Write a C++ program comparing lock-based vs lock-free implementations for high-frequency counters:
- Implement a metrics collection system with 3 counters: order_count, trade_count, cancel_count
- Create 8 threads, each simulating 1,000,000 random events (incrementing counters)
- Implement four versions:
  1. **Mutex-based**: Single mutex protecting all counters
  2. **Per-counter mutex**: Separate mutex for each counter
  3. **Atomic**: Use `std::atomic<int>` for each counter
  4. **Atomic with memory order**: Use `std::atomic` with `memory_order_relaxed`
- Measure throughput (operations/second) for each version
- Verify correctness: total increments should equal 8,000,000 * 3
- Compare CPU cache coherence traffic (observe via `perf` or similar tools)

**Learning Target**: Master lock-free programming with atomic operations. Understand memory ordering guarantees. Learn when atomics outperform mutexes and their limitations.

**Trading System Context**: Similar to maintaining high-frequency metrics (order rates, trade counts) where lock contention would severely degrade performance. Atomics enable lock-free counters.

**Expected Outcome**: Atomic version 10-20x faster than single mutex. Relaxed memory order slightly faster still. Per-counter mutex shows moderate improvement.

---

### Q8. Thread-Local Storage and False Sharing
Write a C++ program demonstrating cache-line effects and optimization:
- Implement per-thread order counters for 8 threads, each incrementing 10,000,000 times
- Implement four versions:
  1. **Shared array**: `int counters[8]` - adjacent in memory (false sharing)
  2. **Padded array**: Add padding to separate counters onto different cache lines (64-byte alignment)
  3. **Thread-local**: Use `thread_local` storage, aggregate at end
  4. **Atomic with padding**: `std::atomic<int>` with cache-line padding
- Measure execution time for each version
- Final step: sum all counters and verify total is correct
- Bonus: Use `perf stat -e cache-misses` to measure cache coherence traffic

**Learning Target**: Understand false sharing and cache-line effects on performance. Learn thread-local storage to eliminate synchronization. Master cache-aware data structure design.

**Trading System Context**: Similar to per-thread statistics tracking (orders processed, latency measurements) where false sharing causes unnecessary cache coherence traffic, degrading performance.

**Expected Outcome**: Shared array suffers from false sharing (slow). Padded/thread-local versions 5-10x faster by eliminating cache-line bouncing.

---

### Q9. Barrier Synchronization and Phased Computation
Write a C++ program implementing a multi-phase parallel computation with barriers:
- Simulate 4 trading stages with 4 worker threads:
  1. **Order validation** (random 50-100ms per thread)
  2. **Risk check** (random 30-80ms per thread)
  3. **Order matching** (random 40-90ms per thread)
  4. **Trade reporting** (random 20-60ms per thread)
- All threads must complete each phase before any thread proceeds to next phase
- Implement barrier synchronization using `std::mutex` and `std::condition_variable`
  - Counter tracks how many threads reached barrier
  - Last thread to arrive wakes all others
- Each thread prints when it reaches and leaves each barrier
- Measure total execution time and per-phase time (slowest thread determines phase duration)
- Compare with non-synchronized version where threads don't wait

**Learning Target**: Implement barrier synchronization for phased parallel algorithms. Understand bulk synchronous parallel (BSP) model. Learn when phases must be synchronized vs overlapped.

**Trading System Context**: Similar to multi-stage order processing pipeline where certain stages must complete before others (e.g., all risk checks complete before any orders match).

**Expected Outcome**: Synchronized version ensures phase ordering but overall time determined by slowest thread in each phase. Non-synchronized faster but potentially incorrect.

---

## Section 4: Performance Optimization (3 questions)

### Q10. Parallel Array Operations with Compiler Optimizations
Write a C++ program demonstrating compiler optimization impact on parallel code:
- Implement vector addition: C[i] = A[i] + B[i] for arrays of 100,000,000 elements
- Implement four versions:
  1. **Sequential**: Single thread, simple loop
  2. **Parallel**: 4 threads, divide array into quarters
  3. **Parallel + manual unrolling**: Process 4 elements per iteration in each thread
  4. **Parallel + auto-vectorization hints**: Add `#pragma` hints for compiler
- Compile each with: `-O0`, `-O2`, `-O3`, `-O3 -march=native`
- Measure execution time for all combinations
- Create a table showing speedup from: parallelism, optimization level, vectorization
- Verify results are identical across versions

**Learning Target**: Understand cumulative effects of parallelism and compiler optimizations. Learn how -O3 enables auto-vectorization. See interaction between manual and compiler optimizations.

**Trading System Context**: Similar to bulk calculations in trading (computing portfolio values, aggregating market data) where both parallelism and compiler optimizations matter.

**Expected Outcome**: -O3 gives 10-20x speedup over -O0. 4 threads give additional 3-4x. Combined: 40-60x total speedup.

---

### Q11. Cache-Friendly Data Structures
Write a C++ program comparing array-of-structures vs structure-of-arrays:
- Represent 1,000,000 orders with: order_id (int), price (double), quantity (int), side (char)
- Implement two data layouts:
  1. **Array-of-Structures (AoS)**: `struct Order { int id; double price; int qty; char side; }; vector<Order>`
  2. **Structure-of-Arrays (SoA)**: Separate arrays: `vector<int> ids, qtys; vector<double> prices; vector<char> sides;`
- Implement 4 threads performing three operations (1,000 iterations each):
  - **Scan**: Count orders with price > threshold (read all prices)
  - **Update**: Increase all prices by 1% (read-modify-write all prices)
  - **Filter**: Find orders matching criteria (read price and quantity)
- Compile with `-O3` for both layouts
- Measure execution time for each operation and layout
- Explain performance differences based on cache behavior

**Learning Target**: Master cache-friendly data layout. Understand spatial locality and cache line utilization. Learn when SoA outperforms AoS.

**Trading System Context**: Order book storage layout significantly impacts scanning performance (finding orders at price level). SoA layout enables better vectorization and cache usage.

**Expected Outcome**: SoA layout 2-3x faster for operations touching subset of fields due to better cache utilization and vectorization potential.

---

### Q12. Thread Scaling Analysis with Amdahl's Law
Write a C++ program to empirically verify Amdahl's Law and determine optimal thread count:
- Implement portfolio valuation with 10,000 positions:
  - 80% simple positions: price * quantity (parallelizable)
  - 20% complex positions: Black-Scholes option pricing (parallelizable but compute-intensive)
  - Result aggregation: sum all values (sequential with lock)
- Measure execution time with 1, 2, 4, 8, 16 threads
- Calculate speedup and efficiency for each thread count
- Profile to identify: parallel work %, sequential work %, synchronization overhead %
- Plot speedup vs thread count and compare with Amdahl's Law prediction
- Determine optimal thread count for your hardware

**Learning Target**: Understand parallel scalability limits. Apply Amdahl's Law practically. Learn to identify bottlenecks preventing perfect scaling. Determine optimal parallelism.

**Trading System Context**: Real trading systems have sequential bottlenecks (aggregation, logging). Understanding scaling limits helps size thread pools appropriately.

**Expected Outcome**: Speedup plateaus around 8 threads due to sequential portions. Efficiency decreases with more threads. Matches Amdahl's Law for 80% parallel work.

---

## Section 5: Integration Challenges (3 questions)

### Q13. High-Performance Order Book Aggregator
**Integration Challenge**: Build a multi-threaded order book aggregator combining multiple concepts.

**Requirements**:
- Process 5,000,000 random orders (BUY/SELL, price in range 99.00-101.00, quantity 1-1000)
- Maintain aggregated volume at each 0.01 price level (200 levels total)
- Implement three progressively optimized versions:

**Version 1 - Baseline**: Single-threaded with `std::map<double, int>` (compile with -O0)

**Version 2 - Basic Parallel**: 
- 4 threads, each processes subset of orders
- Thread-local `std::map` per thread
- Final merge step combines thread-local results (synchronized)
- Compile with -O3

**Version 3 - Optimized**:
- Pre-allocate array[200] for price levels (index = (price - 99.00) * 100)
- 4 threads with thread-local arrays
- Lock-free aggregation using `std::atomic<int>` for each level
- Cache-aware data layout
- Compile with -O3 -march=native

**Critical Thinking Required**:
- Why is std::map slow for this use case?
- How does thread-local aggregation reduce contention?
- Why is array-based approach faster than map?
- What's the trade-off between memory and speed?

**Target**: Achieve 50x speedup from V1 to V3
- Measure: throughput (orders/sec), memory usage, cache miss rate
- Verify: sum of all volumes equals sum of order quantities

**Learning Target**: Synthesize data structure selection, parallelism, cache optimization, and atomic operations to solve realistic trading problem.

**Expected Outcome**: V1: 200K orders/sec. V2: 3M orders/sec. V3: 10M+ orders/sec.

---

### Q14. Lock-Free Work Queue with Progress Guarantees
**Integration Challenge**: Implement a lock-free multi-producer-multi-consumer queue.

**Requirements**:
- Build a bounded queue (capacity 1024) for order processing
- Support 4 producer threads and 4 consumer threads
- Use only atomic operations - no mutexes or condition variables
- Implement using circular buffer with atomic head/tail indices
- Handle: queue full (producers wait/retry), queue empty (consumers wait/retry)
- Process 10,000,000 orders total

**Critical Thinking Required**:
- How to coordinate multiple producers writing to same queue?
- How to avoid ABA problem with index wrap-around?
- Should producers/consumers spin or yield when waiting?
- How to guarantee progress (not just lock-freedom)?
- How to verify no orders are lost or duplicated?

**Key Techniques to Apply**:
- `std::atomic<size_t>` for head/tail indices
- Compare-and-swap (CAS) operations
- Memory ordering (`memory_order_acquire`, `memory_order_release`)
- Exponential backoff for retries
- Sequence numbers to detect duplicates

**Target**: Achieve 20M+ orders/sec throughput
- Compare with mutex-based queue implementation
- Measure contention rate and retry frequency
- Verify correctness: all produced orders consumed exactly once

**Learning Target**: Master lock-free programming with complex coordination. Understand atomic operations, memory ordering, and progress guarantees. Learn trade-offs between lock-free and blocking approaches.

**Expected Outcome**: Lock-free version 5-10x faster than mutex-based for high contention. More complex to implement and verify correctness.

---

### Q15. Parallel Market Data Processing Pipeline
**Integration Challenge**: Build a complete multi-stage parallel processing pipeline.

**Requirements**:

**Data Structures**:
- **Raw Message** (input): Pipe-delimited string with 6 fields
  - Format: `"TYPE|SIDE|SYMBOL|PRICE|QUANTITY|ORDERID"`
  - Example valid: `"ORDER|BUY|AAPL|150.25|100|12345678"`
  - Example invalid: `"ORDER|BUY|AAPL|-10.50|100|12345678"` (negative price)
  - Fields:
    - `TYPE`: Must be "ORDER", "CANCEL", or "MODIFY"
    - `SIDE`: Must be "BUY" or "SELL"
    - `SYMBOL`: 1-5 uppercase letters (e.g., "AAPL", "MSFT", "GOOGL")
    - `PRICE`: Positive decimal number (e.g., "150.25")
    - `QUANTITY`: Positive integer (e.g., "100")
    - `ORDERID`: Unique 8-digit number (e.g., "12345678")
  
- **Structured Order** (after parsing):
  ```cpp
  struct Order {
      enum Type { ORDER, CANCEL, MODIFY };
      enum Side { BUY, SELL };
      Type type;
      Side side;
      std::string symbol;
      double price;
      int quantity;
      long order_id;
      bool is_valid;  // Set by validation stage
  };
  ```

**Pipeline Stages (Detailed)**:

**Stage 1: Parse (10 parser threads)**
- Input: Raw message string from input queue
- Output: Structured Order object to validation queue
- Tasks:
  1. Split string by pipe delimiter `'|'` (expect exactly 6 fields)
  2. Convert TYPE string to enum (ORDER=0, CANCEL=1, MODIFY=2)
  3. Convert SIDE string to enum (BUY=0, SELL=1)
  4. Copy SYMBOL as string
  5. Parse PRICE to double using `std::stod()`
  6. Parse QUANTITY to int using `std::stoi()`
  7. Parse ORDERID to long using `std::stol()`
  8. Handle parsing errors (invalid format, conversion failures)
- Error handling: If parsing fails, log error and skip message (don't send to next stage)
- Simulate processing: Sleep for random 1-3 microseconds

**Stage 2: Validate (8 validator threads)**
- Input: Structured Order from parse queue
- Output: Validated Order (with `is_valid` flag set) to process queue
- Validity Checks (if any fail, set `is_valid = false`):
  1. **Price validation**: `price > 0.0` and `price < 1,000,000.0` (reasonable range)
  2. **Quantity validation**: `quantity > 0` and `quantity <= 10,000` (not too large)
  3. **Symbol validation**: Length between 1-5 chars, all uppercase letters
     - Regex check: `^[A-Z]{1,5}$`
  4. **Order ID validation**: Must be positive and non-zero
- Risk Checks (simulate real trading system):
  1. **Max order size**: `price * quantity <= 1,000,000` (max $1M per order)
  2. **Price reasonableness**: 
     - For BUY orders: `price <= 500.0` (not buying too high)
     - For SELL orders: `price >= 0.01` (not selling too low)
  3. **Symbol whitelist**: Check symbol against allowed list of 10 symbols
     - Allowed: "AAPL", "MSFT", "GOOGL", "AMZN", "TSLA", "META", "NVDA", "AMD", "INTC", "IBM"
     - Reject if symbol not in list
- Error handling: If invalid, set `is_valid = false` but still send to process stage for metrics
- Simulate processing: Sleep for random 2-5 microseconds

**Stage 3: Process (4 processor threads)**
- Input: Validated Order from validation queue
- Output: Trade confirmations (optional, for metrics)
- Tasks (only for valid orders with `is_valid = true`):
  1. **Maintain simple order book**:
     - Use two maps: `map<double, int> buy_book;` (price → total quantity)
     - And: `map<double, int> sell_book;`
  2. **Add order to book**:
     - For BUY orders: `buy_book[order.price] += order.quantity;`
     - For SELL orders: `sell_book[order.price] += order.quantity;`
  3. **Attempt matching** (simplified):
     - Check if best BUY price >= best SELL price
     - If yes: Execute trade, decrement quantities, record match
     - Best BUY = `buy_book.rbegin()->first` (highest price)
     - Best SELL = `sell_book.begin()->first` (lowest price)
  4. **Track statistics**:
     - Count: valid orders processed, invalid orders rejected
     - Count: successful matches
     - Track: total volume traded
- Error handling: Invalid orders are counted but not processed
- Simulate processing: Sleep for random 5-10 microseconds

**Implementation Details**:
- Connect stages with bounded queues (capacity 100 each)
- Input: 20,000,000 raw message strings (generate with random valid/invalid messages)
- Each stage has realistic processing time:
  - Parse: 1-3 microseconds (use `std::this_thread::sleep_for`)
  - Validate: 2-5 microseconds
  - Process: 5-10 microseconds
- Include ~5% invalid messages to test error handling

**Critical Thinking Required**:
- How to balance thread count across stages?
- What queue capacity prevents pipeline stalls?
- Should you use backpressure (block producers) or drop messages?
- How to handle graceful shutdown across pipeline stages?
- How to monitor pipeline health (queue depths, throughput per stage)?

**Implementation Requirements**:
- Use thread pools for each stage (from Q2)
- Use producer-consumer pattern for queues (from Q4)
- Track metrics per stage: messages processed, average queue depth, processing time
- Implement coordinated shutdown: parse completes → signal validators → processors drain queues
- Detect and report bottleneck stage

**Target**: Achieve 5M+ messages/sec end-to-end throughput
- Pipeline should be balanced (no stage idle while others backlogged)
- Queue depths should remain stable (not growing unbounded)
- Clean shutdown with no lost messages

**Learning Target**: Design and implement complete concurrent system with multiple coordination patterns. Balance throughput across stages. Monitor and optimize pipeline performance.

**Expected Outcome**: Bottleneck likely in parse or process stage. Proper balancing achieves near-linear speedup up to bottleneck.

---

## Summary

These 15 carefully designed questions provide a comprehensive yet efficient learning path:

**Foundation (Q1-Q3)**: Thread basics, race conditions, synchronization, work distribution - 3 questions integrating multiple concepts each

**Classical Problems (Q4-Q6)**: Producer-consumer, deadlock prevention, parallel algorithms - 3 questions with real-world complexity

**Advanced Synchronization (Q7-Q9)**: Lock-free programming, cache optimization, barriers - 3 questions on performance-critical techniques

**Performance Optimization (Q10-Q12)**: Compiler optimizations, data layout, scalability analysis - 3 questions on low-level optimization

**Integration Challenges (Q13-Q15)**: Complete systems combining all concepts - 3 questions requiring critical thinking and experimentation

Each question requires:
- **Critical thinking** about design trade-offs
- **Multiple implementations** to compare approaches  
- **Performance measurement** and analysis
- **Correctness verification**
- **Connection to trading systems**

Key transferable skills:
- Concurrent data structure design and synchronization strategies
- Performance optimization from algorithm to hardware level
- Lock-free programming and atomic operations
- Scalability analysis and bottleneck identification
- Complete system design with multiple coordination patterns
- Critical thinking about trade-offs in high-performance systems

This streamlined path provides deep learning through complex, integrated problems rather than many simple exercises.
