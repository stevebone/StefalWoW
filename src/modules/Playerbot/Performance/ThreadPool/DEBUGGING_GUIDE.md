# ThreadPool Debugging and Monitoring System

## Overview

This document describes the comprehensive debugging and monitoring system built into the TrinityCore Playerbot ThreadPool. The system is designed to detect and diagnose deadlocks, performance issues, and threading problems with minimal runtime overhead (<1% CPU).

## Features

### 1. Thread State Tracking

Every worker thread continuously tracks its state through the following states:

```cpp
enum class WorkerState {
    UNINITIALIZED,     // Thread not yet started
    INITIALIZING,      // Thread starting up
    IDLE_SLEEPING,     // In Sleep() waiting for work (CV wait)
    IDLE_SPINNING,     // Yielding in steal backoff
    CHECKING_QUEUES,   // Scanning local queues for work
    STEALING,          // Attempting to steal work from another worker
    EXECUTING,         // Executing a task
    WAITING_MUTEX,     // Waiting to acquire a mutex
    SHUTTING_DOWN,     // Shutdown in progress
    TERMINATED         // Thread ended
};
```

### 2. Wait Location Tracking

When a thread enters any wait state, the system records:
- **Function name** (Sleep, TryStealTask, etc.)
- **Wait type** (condition_variable, yield, mutex, etc.)
- **Timeout value** (10ms, 1ms, etc.)
- **Source location** (file, line number)
- **Entry timestamp**

This immediately shows where threads are blocked, e.g.:
```
Worker 14640 waiting at Sleep() [ThreadPool.cpp:485]
  Type: condition_variable, Timeout: 10ms, Waiting: 5234ms
```

### 3. Automatic Deadlock Detection

The `DeadlockDetector` runs in a background thread and checks every second for:

#### Detection Criteria:
- **Critical**: All workers sleeping for >2 seconds with pending tasks
- **Error**: Workers stuck in same state for >30 seconds
- **Warning**: >50% of workers sleeping for >5 seconds
- **Info**: Queue growing with no task completion

#### Response Actions:
- Generate diagnostic dump to `logs/threadpool/`
- Log warnings/errors
- Optional automatic recovery (wake sleeping workers)
- Console alerts for operators

### 4. Performance Metrics

The system tracks detailed performance metrics:

#### Per-Worker Metrics:
- Tasks executed/failed
- Time spent in each state (%)
- Steal attempt success rate
- Task latency histogram (p50, p95, p99)
- Queue wait time distribution
- Spurious wakeup count

#### Global Metrics:
- Total throughput (tasks/sec)
- Average latency (submission to completion)
- Queue depth by priority
- Context switches per thread

### 5. Console Commands

Runtime diagnostic commands for operators:

```bash
# Show current thread states
.bot threadpool status

# Show performance metrics
.bot threadpool stats

# Detailed info for specific worker
.bot threadpool worker <id>

# Run deadlock detection manually
.bot threadpool deadlock

# Enable/disable diagnostics
.bot threadpool diagnostics [on|off]

# Enable detailed tracing for a worker
.bot threadpool trace <id> [on|off]
```

## Usage Examples

### Example 1: Identifying a Deadlock

When a deadlock occurs, you'll see:

```
[ERROR] [playerbot.threadpool.deadlock] CRITICAL: All workers sleeping with pending tasks
  - 4/4 workers sleeping for >2000ms
  - Queue size: 157 tasks pending
  - Throughput: 0.0 tasks/sec

Diagnostic dump generated: logs/threadpool/threadpool_deadlock_20241020_143521.txt
```

The dump file contains:
- Complete thread state for all workers
- Wait locations with timestamps
- State transition history (last 100 transitions)
- Queue contents
- Task submission history
- Current configuration

### Example 2: Performance Analysis

Use `.bot threadpool stats` to see:

```
=== ThreadPool Statistics ===
Average Latency: 245us
Throughput: 4821.3 tasks/sec

=== Worker Statistics ===
Worker 0:
  Tasks Executed: 182341
  Steal Success Rate: 12.3% (1523/12398)
  Task Latency:
    Avg: 198us
    P50: 142us
    P95: 412us
    P99: 1823us
```

### Example 3: Debugging Specific Worker

Use `.bot threadpool worker 2` to see:

```
=== Worker 2 Details ===
Current State: IDLE_SLEEPING (for 234ms)
Wait Location: Sleep() at ThreadPool.cpp:512
  Type: condition_variable, Timeout: 10ms

Performance Metrics:
  Tasks Executed: 45234
  Tasks Failed: 0
  Wakeup Count: 8923
  Spurious Wakeups: 234 (2.6%)

Time Distribution:
  EXECUTING: 78.2%
  IDLE_SLEEPING: 15.3%
  STEALING: 4.1%
  CHECKING_QUEUES: 2.4%

Recent State Transitions:
  14:35:21 EXECUTING -> CHECKING_QUEUES
  14:35:21 CHECKING_QUEUES -> STEALING
  14:35:21 STEALING -> IDLE_SLEEPING at Sleep
```

## Configuration

### Enable/Disable Diagnostics

```cpp
ThreadPool pool;
pool.SetDiagnosticsEnabled(true);  // Enable at runtime
```

### Deadlock Detector Configuration

```cpp
DeadlockDetectorConfig config;
config.checkInterval = std::chrono::milliseconds(1000);
config.allWorkersSleepThreshold = std::chrono::milliseconds(2000);
config.enableAutoDump = true;
config.dumpDirectory = "logs/threadpool/";
```

### Logging Levels

Configure in worldserver.conf:
```ini
# State transitions (very verbose)
Logger.playerbot.threadpool.state=6,Console Server

# Wait events (debugging)
Logger.playerbot.threadpool.wait=5,Console Server

# Deadlock warnings
Logger.playerbot.threadpool.deadlock=4,Console Server

# Performance issues
Logger.playerbot.threadpool.perf=4,Console Server
```

## Interpreting Diagnostic Output

### Common Patterns

#### Pattern 1: All Workers Sleeping
**Symptom**: All workers in IDLE_SLEEPING with tasks queued
**Cause**: Wake notifications lost or not sent
**Solution**: Check Wake() calls in Submit()

#### Pattern 2: Workers Stuck in STEALING
**Symptom**: Multiple workers in STEALING state >1s
**Cause**: Steal contention or infinite loop
**Solution**: Check steal backoff logic

#### Pattern 3: One Worker Overloaded
**Symptom**: One worker EXECUTING, others IDLE_SLEEPING
**Cause**: Poor work distribution
**Solution**: Check worker selection in Submit()

### Diagnostic Dump Format

```
========================================
ThreadPool Deadlock Diagnostic Report
========================================
Generated: 20241020_143521
Severity: CRITICAL
Description: All workers sleeping with pending tasks

Summary
-------
Total Queued Tasks: 157
Completed Tasks: 892341
Throughput: 0.00 tasks/sec

Worker Issues
-------------
Worker 0: Sleeping for 5234ms
Worker 1: Sleeping for 5234ms
Worker 2: Sleeping for 5234ms
Worker 3: Sleeping for 5234ms

Detailed Worker Diagnostics
==========================
[Full worker state for each thread...]
```

## Performance Impact

The debugging system is designed for production use:

- **State tracking**: <10ns per transition (atomic write)
- **Wait location**: ~50ns per wait entry
- **Metrics collection**: <100ns per task
- **Deadlock detection**: 1ms check every second
- **Memory overhead**: ~2KB per worker thread

Total overhead: <1% CPU, <100KB memory for 8 workers

## Troubleshooting

### Issue: "Worker X in unknown location"

The thread crashed or is in untracked code. Check:
1. Stack trace with debugger
2. Last known state transition
3. Core dump if available

### Issue: "Deadlock detector not running"

Check if diagnostics are enabled:
```cpp
if (!pool.IsDiagnosticsEnabled())
    pool.SetDiagnosticsEnabled(true);
```

### Issue: "No diagnostic output"

Verify logging is configured:
```bash
grep "playerbot.threadpool" worldserver.conf
```

## Best Practices

1. **Always enable diagnostics in development**
2. **Monitor steal success rate** (should be >10%)
3. **Watch for spurious wakeups** (<5% is normal)
4. **Check time distribution** (>70% EXECUTING is good)
5. **Save diagnostic dumps** for post-mortem analysis
6. **Use console commands** before restarting server
7. **Enable detailed logging** when reproducing issues

## Integration with GDB

For deep debugging, combine with GDB:

```bash
# Attach to worldserver
gdb -p $(pidof worldserver)

# Show all threads
info threads

# Switch to worker thread
thread 14

# Show current location
where

# Show thread-local diagnostics
print _diagnostics->currentState
print _diagnostics->GetCurrentWait()
```

## Future Enhancements

Planned improvements:
- Visual thread state timeline
- Real-time web dashboard
- Automatic performance tuning
- Machine learning for anomaly detection
- Integration with monitoring systems (Prometheus/Grafana)