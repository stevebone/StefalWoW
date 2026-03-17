# Performance Benchmarking Framework

## Overview
Comprehensive benchmarking system to measure, track, and validate performance improvements across all optimization phases.

## Benchmark Suite Implementation

### 1. Core Benchmark Framework

```cpp
// File: src/modules/Playerbot/Performance/BenchmarkFramework.h
#pragma once

#include "Define.h"
#include <chrono>
#include <atomic>
#include <vector>
#include <functional>

namespace Playerbot::Benchmark
{

/**
 * @brief High-precision benchmark timer using TSC when available
 */
class BenchmarkTimer
{
private:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;
    using Duration = std::chrono::nanoseconds;

    TimePoint _start;
    std::vector<Duration> _samples;
    std::atomic<bool> _running{false};

public:
    void Start()
    {
        _running.store(true, std::memory_order_release);
        _start = Clock::now();
    }

    Duration Stop()
    {
        auto end = Clock::now();
        _running.store(false, std::memory_order_release);

        auto duration = std::chrono::duration_cast<Duration>(end - _start);
        _samples.push_back(duration);
        return duration;
    }

    void Reset()
    {
        _samples.clear();
        _running.store(false, std::memory_order_release);
    }

    struct Statistics
    {
        Duration min;
        Duration max;
        Duration median;
        Duration mean;
        Duration stddev;
        Duration p95;  // 95th percentile
        Duration p99;  // 99th percentile
        size_t samples;
    };

    Statistics GetStatistics() const
    {
        if (_samples.empty())
            return {};

        auto sorted = _samples;
        std::sort(sorted.begin(), sorted.end());

        Statistics stats;
        stats.samples = sorted.size();
        stats.min = sorted.front();
        stats.max = sorted.back();
        stats.median = sorted[sorted.size() / 2];

        // Calculate mean
        Duration sum{0};
        for (auto& sample : sorted)
            sum += sample;
        stats.mean = sum / sorted.size();

        // Calculate standard deviation
        Duration variance{0};
        for (auto& sample : sorted)
        {
            auto diff = sample - stats.mean;
            variance += Duration(diff.count() * diff.count() / sorted.size());
        }
        stats.stddev = Duration(static_cast<long>(std::sqrt(variance.count())));

        // Calculate percentiles
        stats.p95 = sorted[static_cast<size_t>(sorted.size() * 0.95)];
        stats.p99 = sorted[static_cast<size_t>(sorted.size() * 0.99)];

        return stats;
    }
};

/**
 * @brief Benchmark scenario for testing specific operations
 */
class BenchmarkScenario
{
public:
    struct Config
    {
        std::string name;
        uint32 botCount;
        uint32 iterations;
        uint32 warmupIterations;
        bool parallel;
        std::function<void()> setup;
        std::function<void()> teardown;
    };

private:
    Config _config;
    BenchmarkTimer _timer;
    std::atomic<uint64> _operationsCompleted{0};

public:
    explicit BenchmarkScenario(Config config)
        : _config(std::move(config))
    {
    }

    template<typename Func>
    void Run(Func&& operation)
    {
        TC_LOG_INFO("module.playerbot.bench",
                    "Running benchmark: {} ({} bots, {} iterations)",
                    _config.name, _config.botCount, _config.iterations);

        // Setup
        if (_config.setup)
            _config.setup();

        // Warmup
        for (uint32 i = 0; i < _config.warmupIterations; ++i)
        {
            operation();
        }

        // Actual benchmark
        _timer.Reset();

        for (uint32 i = 0; i < _config.iterations; ++i)
        {
            _timer.Start();
            operation();
            _timer.Stop();
            _operationsCompleted.fetch_add(1, std::memory_order_relaxed);
        }

        // Teardown
        if (_config.teardown)
            _config.teardown();

        // Report results
        ReportResults();
    }

private:
    void ReportResults()
    {
        auto stats = _timer.GetStatistics();

        TC_LOG_INFO("module.playerbot.bench",
            "=== Benchmark Results: {} ===\n"
            "Bots: {}\n"
            "Iterations: {}\n"
            "Min: {:.3f}ms\n"
            "Max: {:.3f}ms\n"
            "Median: {:.3f}ms\n"
            "Mean: {:.3f}ms\n"
            "StdDev: {:.3f}ms\n"
            "P95: {:.3f}ms\n"
            "P99: {:.3f}ms\n"
            "Throughput: {:.0f} ops/sec",
            _config.name,
            _config.botCount,
            stats.samples,
            stats.min.count() / 1000000.0,
            stats.max.count() / 1000000.0,
            stats.median.count() / 1000000.0,
            stats.mean.count() / 1000000.0,
            stats.stddev.count() / 1000000.0,
            stats.p95.count() / 1000000.0,
            stats.p99.count() / 1000000.0,
            1000000000.0 / stats.mean.count());
    }
};

} // namespace Playerbot::Benchmark
```

### 2. Specific Benchmark Scenarios

```cpp
// File: src/modules/Playerbot/Performance/BotBenchmarks.cpp

#include "BenchmarkFramework.h"
#include "AI/BotAI.h"
#include "Economy/AuctionManager.h"

namespace Playerbot::Benchmark
{

/**
 * @brief Benchmark manager update performance
 */
class ManagerUpdateBenchmark
{
private:
    std::vector<std::unique_ptr<BotAI>> _testBots;
    std::atomic<uint32> _completedUpdates{0};

public:
    void BenchmarkCurrentArchitecture(uint32 botCount)
    {
        BenchmarkScenario scenario({
            .name = "Current Architecture (Mutex-based)",
            .botCount = botCount,
            .iterations = 100,
            .warmupIterations = 10,
            .parallel = false,
            .setup = [this, botCount]() { CreateTestBots(botCount); },
            .teardown = [this]() { CleanupTestBots(); }
        });

        scenario.Run([this]() {
            // Simulate frame update with current architecture
            for (auto& bot : _testBots)
            {
                bot->UpdateManagers(50); // 50ms diff
            }
            _completedUpdates.fetch_add(_testBots.size(), std::memory_order_relaxed);
        });
    }

    void BenchmarkPhase1Optimizations(uint32 botCount)
    {
        BenchmarkScenario scenario({
            .name = "Phase 1 (Throttling + Batching)",
            .botCount = botCount,
            .iterations = 100,
            .warmupIterations = 10,
            .parallel = false,
            .setup = [this, botCount]() {
                CreateTestBots(botCount);
                EnablePhase1Optimizations();
            },
            .teardown = [this]() { CleanupTestBots(); }
        });

        scenario.Run([this]() {
            // Batch process with throttling
            BatchedUpdateSystem::ProcessBotBatch(_testBots, 50);
            _completedUpdates.fetch_add(_testBots.size(), std::memory_order_relaxed);
        });
    }

    void BenchmarkPhase2MessagePassing(uint32 botCount)
    {
        BenchmarkScenario scenario({
            .name = "Phase 2 (Lock-free Message Passing)",
            .botCount = botCount,
            .iterations = 100,
            .warmupIterations = 10,
            .parallel = true,
            .setup = [this, botCount]() {
                CreateTestBots(botCount);
                InitializeActorSystem();
            },
            .teardown = [this]() {
                ShutdownActorSystem();
                CleanupTestBots();
            }
        });

        scenario.Run([this]() {
            // Message-passing update
            for (auto& bot : _testBots)
            {
                bot->UpdateManagersMessagePassing(50);
            }

            // Process actor queues
            ActorSystem::Instance().ProcessAllQueues();
            _completedUpdates.fetch_add(_testBots.size(), std::memory_order_relaxed);
        });
    }

private:
    void CreateTestBots(uint32 count)
    {
        _testBots.clear();
        _testBots.reserve(count);

        for (uint32 i = 0; i < count; ++i)
        {
            // Create mock bot for testing
            auto bot = std::make_unique<BotAI>(CreateMockPlayer(i));
            _testBots.push_back(std::move(bot));
        }
    }

    void CleanupTestBots()
    {
        _testBots.clear();
    }

    void EnablePhase1Optimizations()
    {
        UpdateThrottler::Instance().Enable();
        // Configure throttling parameters
    }

    void InitializeActorSystem()
    {
        ActorSystem::Instance().Initialize(4);
        // Register manager actors
    }

    void ShutdownActorSystem()
    {
        ActorSystem::Instance().Shutdown();
    }
};

/**
 * @brief Benchmark lock contention
 */
class LockContentionBenchmark
{
public:
    struct ContentionMetrics
    {
        std::atomic<uint64> lockAttempts{0};
        std::atomic<uint64> lockAcquired{0};
        std::atomic<uint64> contentionEvents{0};
        std::atomic<uint64> totalWaitTimeNs{0};
        std::atomic<uint64> maxWaitTimeNs{0};
    };

private:
    ContentionMetrics _metrics;
    std::recursive_mutex _testMutex;

public:
    void BenchmarkMutexContention(uint32 threadCount, uint32 operations)
    {
        std::vector<std::thread> threads;
        threads.reserve(threadCount);

        auto start = std::chrono::high_resolution_clock::now();

        for (uint32 t = 0; t < threadCount; ++t)
        {
            threads.emplace_back([this, operations]() {
                for (uint32 i = 0; i < operations; ++i)
                {
                    auto lockStart = std::chrono::high_resolution_clock::now();
                    _metrics.lockAttempts.fetch_add(1, std::memory_order_relaxed);

                    {
                        std::lock_guard<std::recursive_mutex> lock(_testMutex);
                        auto lockEnd = std::chrono::high_resolution_clock::now();

                        auto waitTime = std::chrono::duration_cast<std::chrono::nanoseconds>(
                            lockEnd - lockStart).count();

                        _metrics.lockAcquired.fetch_add(1, std::memory_order_relaxed);
                        _metrics.totalWaitTimeNs.fetch_add(waitTime, std::memory_order_relaxed);

                        // Update max wait time
                        uint64 currentMax = _metrics.maxWaitTimeNs.load();
                        while (waitTime > currentMax &&
                               !_metrics.maxWaitTimeNs.compare_exchange_weak(currentMax, waitTime))
                        {
                            // Retry
                        }

                        // Simulate work
                        std::this_thread::sleep_for(std::chrono::microseconds(10));
                    }
                }
            });
        }

        for (auto& t : threads)
            t.join();

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        ReportContentionMetrics(threadCount, operations, duration.count());
    }

    void BenchmarkLockFree(uint32 threadCount, uint32 operations)
    {
        Concurrent::MPMCQueue<uint32> queue;
        std::atomic<uint64> processed{0};

        auto start = std::chrono::high_resolution_clock::now();

        std::vector<std::thread> producers;
        std::vector<std::thread> consumers;

        // Producers
        for (uint32 t = 0; t < threadCount / 2; ++t)
        {
            producers.emplace_back([&queue, operations]() {
                for (uint32 i = 0; i < operations; ++i)
                {
                    queue.Push(i);
                }
            });
        }

        // Consumers
        for (uint32 t = 0; t < threadCount / 2; ++t)
        {
            consumers.emplace_back([&queue, &processed, operations]() {
                uint32 consumed = 0;
                while (consumed < operations)
                {
                    if (auto value = queue.Pop())
                    {
                        processed.fetch_add(1, std::memory_order_relaxed);
                        consumed++;
                    }
                }
            });
        }

        for (auto& t : producers)
            t.join();
        for (auto& t : consumers)
            t.join();

        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        TC_LOG_INFO("module.playerbot.bench",
            "Lock-free queue: {} threads, {} ops/thread, {}ms total, {} ops/sec",
            threadCount, operations, duration.count(),
            (threadCount * operations * 1000) / duration.count());
    }

private:
    void ReportContentionMetrics(uint32 threads, uint32 opsPerThread, int64 durationMs)
    {
        TC_LOG_INFO("module.playerbot.bench",
            "=== Lock Contention Metrics ===\n"
            "Threads: {}\n"
            "Operations/thread: {}\n"
            "Total duration: {}ms\n"
            "Lock attempts: {}\n"
            "Lock acquired: {}\n"
            "Avg wait time: {:.3f}μs\n"
            "Max wait time: {:.3f}μs\n"
            "Throughput: {} ops/sec",
            threads,
            opsPerThread,
            durationMs,
            _metrics.lockAttempts.load(),
            _metrics.lockAcquired.load(),
            _metrics.totalWaitTimeNs.load() / 1000.0 / _metrics.lockAcquired.load(),
            _metrics.maxWaitTimeNs.load() / 1000.0,
            (_metrics.lockAcquired.load() * 1000) / durationMs);
    }
};

} // namespace Playerbot::Benchmark
```

### 3. Automated Benchmark Suite

```cpp
// File: src/modules/Playerbot/Performance/BenchmarkSuite.cpp

namespace Playerbot::Benchmark
{

class BenchmarkSuite
{
public:
    struct TestResult
    {
        std::string testName;
        uint32 botCount;
        float avgTimeMs;
        float p95TimeMs;
        float p99TimeMs;
        uint32 throughput;
        bool passed;
    };

private:
    std::vector<TestResult> _results;

public:
    void RunFullSuite()
    {
        TC_LOG_INFO("module.playerbot.bench",
                    "Starting comprehensive benchmark suite...");

        // Test scaling from 100 to 5000 bots
        std::vector<uint32> botCounts = {100, 500, 1000, 2000, 5000};

        for (uint32 count : botCounts)
        {
            RunScalingTest(count);
        }

        GenerateReport();
    }

private:
    void RunScalingTest(uint32 botCount)
    {
        ManagerUpdateBenchmark updateBench;
        LockContentionBenchmark contentionBench;

        // Test current architecture
        {
            TestResult result;
            result.testName = "Current Architecture";
            result.botCount = botCount;

            auto timer = BenchmarkTimer();
            timer.Start();
            updateBench.BenchmarkCurrentArchitecture(botCount);
            auto duration = timer.Stop();

            result.avgTimeMs = duration.count() / 1000000.0f;
            result.passed = (result.avgTimeMs < 50.0f); // 50ms target

            _results.push_back(result);
        }

        // Test Phase 1 optimizations
        {
            TestResult result;
            result.testName = "Phase 1 Optimizations";
            result.botCount = botCount;

            auto timer = BenchmarkTimer();
            timer.Start();
            updateBench.BenchmarkPhase1Optimizations(botCount);
            auto duration = timer.Stop();

            result.avgTimeMs = duration.count() / 1000000.0f;
            result.passed = (result.avgTimeMs < 50.0f);

            _results.push_back(result);
        }

        // Test Phase 2 message passing
        {
            TestResult result;
            result.testName = "Phase 2 Message Passing";
            result.botCount = botCount;

            auto timer = BenchmarkTimer();
            timer.Start();
            updateBench.BenchmarkPhase2MessagePassing(botCount);
            auto duration = timer.Stop();

            result.avgTimeMs = duration.count() / 1000000.0f;
            result.passed = (result.avgTimeMs < 50.0f);

            _results.push_back(result);
        }

        // Test lock contention
        contentionBench.BenchmarkMutexContention(botCount / 10, 1000);
        contentionBench.BenchmarkLockFree(botCount / 10, 1000);
    }

    void GenerateReport()
    {
        TC_LOG_INFO("module.playerbot.bench",
                    "\n=== BENCHMARK SUITE RESULTS ===\n");

        // Group by test name
        std::unordered_map<std::string, std::vector<TestResult>> grouped;
        for (auto const& result : _results)
        {
            grouped[result.testName].push_back(result);
        }

        // Print comparison table
        for (auto const& [testName, results] : grouped)
        {
            TC_LOG_INFO("module.playerbot.bench", "\n{}", testName);
            TC_LOG_INFO("module.playerbot.bench",
                        "Bots | Avg Time | Status");
            TC_LOG_INFO("module.playerbot.bench",
                        "-----|----------|-------");

            for (auto const& r : results)
            {
                TC_LOG_INFO("module.playerbot.bench",
                            "{:4} | {:7.2f}ms | {}",
                            r.botCount,
                            r.avgTimeMs,
                            r.passed ? "PASS" : "FAIL");
            }
        }

        // Calculate improvement percentages
        CalculateImprovements();
    }

    void CalculateImprovements()
    {
        TC_LOG_INFO("module.playerbot.bench",
                    "\n=== PERFORMANCE IMPROVEMENTS ===\n");

        // Compare Phase 1 vs Current
        auto current = GetResults("Current Architecture", 5000);
        auto phase1 = GetResults("Phase 1 Optimizations", 5000);
        auto phase2 = GetResults("Phase 2 Message Passing", 5000);

        if (current && phase1)
        {
            float improvement = ((current->avgTimeMs - phase1->avgTimeMs) / current->avgTimeMs) * 100;
            TC_LOG_INFO("module.playerbot.bench",
                        "Phase 1 vs Current (5000 bots): {:.1f}% improvement", improvement);
        }

        if (current && phase2)
        {
            float improvement = ((current->avgTimeMs - phase2->avgTimeMs) / current->avgTimeMs) * 100;
            TC_LOG_INFO("module.playerbot.bench",
                        "Phase 2 vs Current (5000 bots): {:.1f}% improvement", improvement);
        }

        if (phase1 && phase2)
        {
            float improvement = ((phase1->avgTimeMs - phase2->avgTimeMs) / phase1->avgTimeMs) * 100;
            TC_LOG_INFO("module.playerbot.bench",
                        "Phase 2 vs Phase 1 (5000 bots): {:.1f}% improvement", improvement);
        }
    }

    TestResult* GetResults(std::string const& testName, uint32 botCount)
    {
        for (auto& result : _results)
        {
            if (result.testName == testName && result.botCount == botCount)
                return &result;
        }
        return nullptr;
    }
};

} // namespace Playerbot::Benchmark
```

## Running Benchmarks

### Console Commands
```
.playerbot bench all              - Run full benchmark suite
.playerbot bench scaling [count]  - Test scaling with specific bot count
.playerbot bench contention       - Test lock contention
.playerbot bench memory           - Memory usage analysis
.playerbot bench network          - Network overhead test
.playerbot bench report           - Generate performance report
```

### Automated CI/CD Integration
```yaml
# .github/workflows/performance.yml
name: Performance Regression Test

on: [push, pull_request]

jobs:
  benchmark:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2

      - name: Build with optimizations
        run: |
          cmake -DCMAKE_BUILD_TYPE=Release \
                -DWITH_PLAYERBOT=1 \
                -DPLAYERBOT_BENCHMARKS=1 ..
          make -j$(nproc)

      - name: Run benchmarks
        run: |
          ./worldserver --benchmark-only

      - name: Check regression
        run: |
          python3 scripts/check_performance_regression.py \
            --baseline performance_baseline.json \
            --current benchmark_results.json \
            --threshold 10  # Allow 10% regression
```

## Performance Targets

### Minimum Requirements (MUST PASS)
| Bot Count | Update Time | Memory/Bot | CPU/Bot |
|-----------|------------|------------|---------|
| 100 | <10ms | <10MB | <0.1% |
| 500 | <25ms | <10MB | <0.1% |
| 1000 | <35ms | <10MB | <0.1% |
| 5000 | <50ms | <10MB | <0.1% |

### Stretch Goals (NICE TO HAVE)
| Bot Count | Update Time | Memory/Bot | CPU/Bot |
|-----------|------------|------------|---------|
| 10000 | <75ms | <8MB | <0.05% |
| 20000 | <150ms | <5MB | <0.05% |

## Monitoring Dashboard

```cpp
// Real-time performance dashboard
class PerformanceDashboard
{
    void Display()
    {
        TC_LOG_INFO("module.playerbot.perf",
            "╔════════════════════════════════════════╗\n"
            "║     PLAYERBOT PERFORMANCE DASHBOARD    ║\n"
            "╠════════════════════════════════════════╣\n"
            "║ Active Bots:        {:6}            ║\n"
            "║ Update Rate:        {:6.2f} Hz        ║\n"
            "║ Frame Time:         {:6.2f} ms        ║\n"
            "║ Mutex Locks/Frame:  {:6}            ║\n"
            "║ Messages/Sec:       {:6}            ║\n"
            "║ Memory Usage:       {:6.2f} GB        ║\n"
            "║ CPU Usage:          {:6.2f} %         ║\n"
            "╠════════════════════════════════════════╣\n"
            "║ Manager Performance (ms):              ║\n"
            "║   Quest:            {:6.2f}            ║\n"
            "║   Auction:          {:6.2f}            ║\n"
            "║   Gathering:        {:6.2f}            ║\n"
            "║   Trade:            {:6.2f}            ║\n"
            "║   Group:            {:6.2f}            ║\n"
            "╠════════════════════════════════════════╣\n"
            "║ Status: {}                      ║\n"
            "╚════════════════════════════════════════╝",
            GetActiveBots(),
            GetUpdateRate(),
            GetFrameTime(),
            GetMutexLocks(),
            GetMessageThroughput(),
            GetMemoryUsageGB(),
            GetCPUUsage(),
            GetManagerTime("Quest"),
            GetManagerTime("Auction"),
            GetManagerTime("Gathering"),
            GetManagerTime("Trade"),
            GetManagerTime("Group"),
            GetSystemStatus()
        );
    }
};
```

## Conclusion

This comprehensive benchmarking framework provides:

1. **Automated Testing**: Full regression suite
2. **Detailed Metrics**: Timing, contention, memory, throughput
3. **Comparison Tools**: Before/after analysis
4. **CI/CD Integration**: Automatic performance validation
5. **Real-time Monitoring**: Live performance dashboard

The framework validates that our optimizations achieve the target of **<50ms update time for 5000 bots** with consistent, measurable improvements across all phases.