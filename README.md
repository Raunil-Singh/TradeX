# TradeX Multi-Threading Curriculum

Welcome! This curriculum teaches multi-threading and concurrency concepts through hands-on C++ programming, preparing you for building high-performance trading systems.

---

## 🚀 Quick Start

1. **Setup your environment**: Follow [SETUP.md](SETUP.md) for platform-specific instructions
2. **Try the demos**: Run programs in `demos/` to learn threading basics
3. **Start Q0**: Begin with the profiling prerequisite in [Tasks.md](Tasks.md)

---

## 📁 Working with Tasks

### Directory Structure

Create a separate directory for each question:

```
Q1/
├── main.cpp              # Your solution
├── screenshots/          # Execution screenshots (2-3 runs)
│   ├── run1.png
│   ├── run2.png
│   └── run3.png
└── README.md             # Optional but recommended: your notes
```

### Quick Commands

```bash
# Create directory for question
mkdir -p Q1/screenshots
cd Q1

# Write code on vim
vim main.cpp


# Compile
g++ -std=c++17 -pthread -O2 main.cpp -o program

# Run multiple times and take screenshots
./program
./program
./program

# Commit your work
git add .
git commit -m "Q1: Completed race condition demonstration"
git push origin yourname
```

---

## 🔧 Compilation Flags

**Standard compilation:**
```bash
g++ -std=c++17 -pthread -O2 main.cpp -o program
```

**With debugging:**
```bash
g++ -std=c++17 -pthread -g -O0 main.cpp -o program
```

**For profiling:**
```bash
g++ -std=c++17 -pthread -O2 -g main.cpp -o program
perf record ./program
perf report
```

**Detect bugs:**
```bash
# Thread sanitizer (finds race conditions)
g++ -std=c++17 -pthread -g -fsanitize=thread main.cpp -o program

# Address sanitizer (finds memory errors)
g++ -std=c++17 -pthread -g -fsanitize=address main.cpp -o program
```

---

## 📊 Documentation Requirements

For each question:

1. **Run 2-3 times**: Verify consistent behavior (or document inconsistencies)
2. **Take screenshots**: Show terminal output with timing, thread IDs, results
3. **Save to screenshots/**: Name clearly (run1.png, run2.png, run3.png)
4. **Commit everything**: Code + screenshots + optional notes

---

## 📝 Git Workflow

```bash
# Daily workflow
git checkout yourname
git pull origin learning_phase_tasks  # Get updates
git merge learning_phase_tasks        # If needed

# Work on tasks
# ... create directories, write code, test ...

# Commit frequently
git add Q1/
git commit -m "Q1: Implemented race condition demo"
git push origin yourname
```

**Commit message format:**
```
Q1: Completed race condition demonstration
Q2: Added thread pool implementation
Q3: Finished reader-writer lock comparison
```

---

## 📖 Task Overview

All task descriptions are in **[Tasks.md](Tasks.md)**:

- **Q0**: Performance profiling with `perf` (prerequisite)
- **Q1-Q3**: Thread fundamentals (race conditions, thread pools, reader-writer locks)
- **Q4-Q6**: Classical problems (producer-consumer, dining philosophers, parallel merge sort)
- **Q7-Q9**: Advanced synchronization (atomics, false sharing, barriers)
- **Q10-Q12**: Performance optimization (compiler flags, cache-friendly structures, scalability)
- **Q13-Q15**: Integration challenges (order book aggregator, lock-free queue, processing pipeline)

---

## 💡 Tips

- **Start with Q0**: Master profiling before tackling optimization questions
- **Follow the order**: Questions build on previous concepts
- **Use the demos**: `demos/` has examples for threading and timing
- **Measure everything**: Use `perf` to validate performance improvements
- **Ask questions**: Check SETUP.md troubleshooting or reach out for help

---

## 📚 Resources

### Learning Materials

- **[SETUP.md](SETUP.md)**: Complete environment setup for Windows/macOS/Linux
- **[Tasks.md](Tasks.md)**: All question descriptions with learning targets
- **[demos/](demos/)**: Example programs demonstrating core concepts

### Reference Documentation

**C++ Threading & Concurrency:**
- [C++ Thread Support Library](https://en.cppreference.com/w/cpp/thread) - Official reference for `<thread>`, `<mutex>`, `<condition_variable>`
- [C++ Atomic Operations](https://en.cppreference.com/w/cpp/atomic) - Complete guide to `std::atomic`
- [C++ Memory Model](https://en.cppreference.com/w/cpp/language/memory_model) - Understanding memory ordering
- [C++ Chrono Library](https://en.cppreference.com/w/cpp/chrono) - Time measurement and duration handling

**Video Tutorials:**
- [Multi-Threading and Concurrency Playlist](https://youtube.com/playlist?list=PLvv0ScY6vfd_ocTP2ZLicgqKnvq50OCXM&si=XMaaV6E56P72rlPw)

**General C++ Reference:**
- [cppreference.com](https://en.cppreference.com/) - Comprehensive C++ language and library reference
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines) - Best practices for modern C++

---

**Ready?** Head to [Tasks.md](Tasks.md) and start with Q0!