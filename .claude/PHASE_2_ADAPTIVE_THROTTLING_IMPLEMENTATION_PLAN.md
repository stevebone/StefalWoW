# Phase 2: Adaptive Throttling System - Implementation Plan

**Project**: TrinityCore PlayerBot Module
**Phase**: 2 of 5
**Date**: 2025-10-29
**Objective**: Implement adaptive spawn throttling system for scalable bot spawning (500 → 5000 bots)
**Estimated Effort**: 2-3 weeks
**Status**: Planning → Implementation

---

## 🎯 Executive Summary

Implement a sophisticated adaptive throttling system that dynamically adjusts bot spawn rates based on real-time server resource availability. This enables safe scaling from 500 to 5000 concurrent bots without overwhelming the server during startup or high-load periods.

### Design Principles
- ✅ **Resource-Aware**: Monitor CPU, memory, database load in real-time
- ✅ **Priority-Based**: Critical bots spawn first, low-priority bots can be deferred
- ✅ **Phased Approach**: Gradual spawning over 10 minutes to avoid startup spikes
- ✅ **Self-Healing**: Automatic backoff on failures, circuit breaker pattern
- ✅ **Zero Server Impact**: <1% CPU overhead for throttling system itself
- ✅ **Enterprise-Grade**: Full error handling, comprehensive logging, metrics

---

## 📦 Components to Implement

### 1. **ResourceMonitor.h/cpp**
**Purpose**: Real-time server resource monitoring
**Location**: `src/modules/Playerbot/Lifecycle/ResourceMonitor.h/cpp`
**Estimated LOC**: ~400 lines

**Key Features**:
- CPU usage tracking (per-core and aggregate)
- Memory usage tracking (working set, commit size)
- Database connection pool monitoring
- Map instance load tracking
- Moving average calculations (5s, 30s, 60s windows)

**Metrics Tracked**:
```cpp
struct ResourceMetrics {
    float cpuUsagePercent;           // 0-100
    float memoryUsagePercent;        // 0-100
    uint32 activeDbConnections;      // Current DB pool usage
    uint32 maxDbConnections;         // DB pool capacity
    uint32 activeMapInstances;       // Number of loaded maps
    uint32 totalActiveBots;          // Currently spawned bots
    SystemTime timestamp;            // Metric collection time
};
```

**Pressure Levels**:
```cpp
enum class ResourcePressure {
    NORMAL,      // <60% CPU, <70% memory - full speed spawning
    ELEVATED,    // 60-75% CPU, 70-80% memory - reduce spawn rate 50%
    HIGH,        // 75-85% CPU, 80-90% memory - reduce spawn rate 75%
    CRITICAL     // >85% CPU, >90% memory - pause spawning, circuit breaker
};
```

**API**:
```cpp
class ResourceMonitor {
public:
    void Update(uint32 diff);
    ResourceMetrics GetCurrentMetrics() const;
    ResourcePressure GetPressureLevel() const;
    bool IsSpawningSafe() const;
    float GetRecommendedSpawnRateMultiplier() const; // 0.0 - 1.0
};
```

---

### 2. **SpawnPriorityQueue.h/cpp**
**Purpose**: Priority-based spawn request management
**Location**: `src/modules/Playerbot/Lifecycle/SpawnPriorityQueue.h/cpp`
**Estimated LOC**: ~350 lines

**Priority Levels**:
```cpp
enum class SpawnPriority {
    CRITICAL,    // Guild leaders, raid leaders - spawn immediately
    HIGH,        // Party members, friends - spawn within 30s
    NORMAL,      // Standard bots - spawn within 2 minutes
    LOW          // Background/filler bots - spawn within 10 minutes
};
```

**Spawn Request**:
```cpp
struct SpawnRequest {
    ObjectGuid characterGuid;
    uint32 accountId;
    SpawnPriority priority;
    SystemTime requestTime;
    uint32 retryCount;
    std::string reason;  // For debugging/metrics

    // Priority comparison operator
    bool operator<(const SpawnRequest& other) const;
};
```

**API**:
```cpp
class SpawnPriorityQueue {
public:
    void EnqueueSpawnRequest(SpawnRequest request);
    std::optional<SpawnRequest> DequeueNextRequest();
    size_t GetQueueSize(SpawnPriority priority) const;
    void ClearQueue();
    void RemoveRequest(ObjectGuid guid);
};
```

---

### 3. **AdaptiveSpawnThrottler.h/cpp**
**Purpose**: Core throttling logic with adaptive batch sizing
**Location**: `src/modules/Playerbot/Lifecycle/AdaptiveSpawnThrottler.h/cpp`
**Estimated LOC**: ~500 lines

**Throttling Configuration**:
```cpp
struct ThrottleConfig {
    uint32 minBatchSize = 1;          // Minimum bots per batch
    uint32 maxBatchSize = 20;         // Maximum bots per batch
    uint32 baseBatchInterval = 500;   // Base interval between batches (ms)
    float aggressivenessMultiplier = 1.0f;  // Tuning parameter
};
```

**Spawn Phases** (Startup Orchestration):
```cpp
enum class SpawnPhase {
    PHASE_0_IMMEDIATE,    // 0-30s:   Critical priority only, 5-10 bots/sec
    PHASE_1_RAPID,        // 30-120s: Critical+High, 10-15 bots/sec
    PHASE_2_STEADY,       // 2-10min: All priorities, 5-10 bots/sec
    PHASE_3_BACKGROUND    // >10min:  Remaining bots, 1-5 bots/sec
};
```

**API**:
```cpp
class AdaptiveSpawnThrottler {
public:
    void Initialize(ResourceMonitor* resourceMonitor);
    void Update(uint32 diff);

    bool CanSpawnNow() const;
    uint32 GetCurrentBatchSize() const;
    uint32 GetRecommendedSpawnDelay() const;

    SpawnPhase GetCurrentPhase() const;
    void RecordSpawnSuccess(SpawnRequest request);
    void RecordSpawnFailure(SpawnRequest request, std::string_view reason);

    ThrottlingMetrics GetMetrics() const;
};
```

**Adaptive Algorithm**:
```cpp
// Pseudo-code for batch size calculation
batchSize = baseBatchSize * resourceMultiplier * phaseMultiplier * failureMultiplier

where:
- resourceMultiplier: 1.0 (NORMAL) → 0.25 (CRITICAL)
- phaseMultiplier: 2.0 (PHASE_0) → 0.5 (PHASE_3)
- failureMultiplier: 1.0 (0% failure) → 0.1 (>50% failure rate)
```

---

### 4. **StartupSpawnOrchestrator.h/cpp**
**Purpose**: Orchestrate phased bot spawning during server startup
**Location**: `src/modules/Playerbot/Lifecycle/StartupSpawnOrchestrator.h/cpp`
**Estimated LOC**: ~450 lines

**Startup Timeline**:
```
Server Start
    ↓
0-30s: PHASE_0_IMMEDIATE
    - Spawn only CRITICAL priority bots (guild leaders, etc.)
    - Batch size: 5-10 bots
    - Target: 50-300 critical bots
    ↓
30s-2min: PHASE_1_RAPID
    - Spawn CRITICAL + HIGH priority bots
    - Batch size: 10-15 bots
    - Target: Additional 500-1000 bots
    ↓
2-10min: PHASE_2_STEADY
    - Spawn all priorities (CRITICAL/HIGH/NORMAL)
    - Batch size: 5-10 bots
    - Target: Reach 3000-4000 bots
    ↓
>10min: PHASE_3_BACKGROUND
    - Spawn remaining LOW priority bots
    - Batch size: 1-5 bots
    - Target: Fill to configured max (5000)
    ↓
STEADY STATE
    - On-demand spawning only
    - Replace disconnected bots
    - Handle dynamic spawn requests
```

**API**:
```cpp
class StartupSpawnOrchestrator {
public:
    void BeginStartupSequence();
    void Update(uint32 diff);
    bool IsStartupComplete() const;

    void EnqueueStartupBots(std::vector<SpawnRequest> requests);
    SpawnPhase GetCurrentPhase() const;

    StartupMetrics GetMetrics() const;
};
```

---

### 5. **SpawnCircuitBreaker.h/cpp**
**Purpose**: Prevent cascading failures via circuit breaker pattern
**Location**: `src/modules/Playerbot/Lifecycle/SpawnCircuitBreaker.h/cpp`
**Estimated LOC**: ~300 lines

**Circuit States**:
```cpp
enum class CircuitState {
    CLOSED,      // Normal operation, spawning allowed
    HALF_OPEN,   // Testing recovery, limited spawning
    OPEN         // Failure detected, spawning blocked
};
```

**Failure Thresholds**:
- **Open Circuit**: >10% spawn failure rate over 1 minute
- **Half-Open Transition**: After 60-second cooldown
- **Close Circuit**: <5% failure rate for 2 minutes in half-open state

**API**:
```cpp
class SpawnCircuitBreaker {
public:
    void RecordAttempt();
    void RecordSuccess();
    void RecordFailure();

    bool AllowSpawn() const;
    CircuitState GetState() const;
    float GetFailureRate() const;

    void Reset();  // Manual override
};
```

---

## 🔌 Integration Points

### BotSpawner.h/cpp Modifications
```cpp
class BotSpawner {
private:
    // PHASE 2 ADDITIONS
    std::unique_ptr<ResourceMonitor> _resourceMonitor;
    std::unique_ptr<SpawnPriorityQueue> _spawnQueue;
    std::unique_ptr<AdaptiveSpawnThrottler> _throttler;
    std::unique_ptr<StartupSpawnOrchestrator> _startupOrchestrator;
    std::unique_ptr<SpawnCircuitBreaker> _circuitBreaker;

public:
    // Modified spawning logic
    void SpawnBot(uint32 accountId, ObjectGuid characterGuid, SpawnPriority priority);
    void Update(uint32 diff);  // Enhanced with throttling logic
};
```

### Integration Flow
```
User Requests Bot Spawn
    ↓
SpawnPriorityQueue::EnqueueSpawnRequest(priority)
    ↓
BotSpawner::Update(diff)
    ↓
ResourceMonitor::Update(diff)  // Collect metrics
    ↓
AdaptiveSpawnThrottler::CanSpawnNow()  // Check throttle
    ↓
SpawnCircuitBreaker::AllowSpawn()  // Check circuit breaker
    ↓
[YES] → SpawnPriorityQueue::DequeueNextRequest()
    ↓
Spawn Bot (existing logic)
    ↓
Record Success/Failure → Update Metrics
```

---

## 📊 Configuration (playerbots.conf)

```ini
###################################################################################################
# PHASE 2: ADAPTIVE THROTTLING CONFIGURATION
###################################################################################################

# Enable/disable adaptive throttling system
Playerbot.EnableAdaptiveThrottling = 1

# Maximum total bots (hard cap)
Playerbot.MaxBotsTotal = 5000

# Throttle tuning
Playerbot.Throttle.MinBatchSize = 1
Playerbot.Throttle.MaxBatchSize = 20
Playerbot.Throttle.BaseBatchInterval = 500

# Resource pressure thresholds (percentage)
Playerbot.Resource.CpuNormalThreshold = 60
Playerbot.Resource.CpuElevatedThreshold = 75
Playerbot.Resource.CpuHighThreshold = 85
Playerbot.Resource.MemoryNormalThreshold = 70
Playerbot.Resource.MemoryElevatedThreshold = 80
Playerbot.Resource.MemoryHighThreshold = 90

# Startup phases (seconds)
Playerbot.Startup.Phase0Duration = 30
Playerbot.Startup.Phase1Duration = 120
Playerbot.Startup.Phase2Duration = 600

# Circuit breaker
Playerbot.CircuitBreaker.FailureRateThreshold = 10
Playerbot.CircuitBreaker.CooldownSeconds = 60
Playerbot.CircuitBreaker.RecoveryThreshold = 5
```

---

## 📈 Success Metrics

| Metric | Baseline | Target | Validation |
|--------|----------|--------|------------|
| **Startup Time (5000 bots)** | N/A | <10 minutes | Measure actual spawn time |
| **Spawn Rate (adaptive)** | 10 bots/sec (fixed) | 5-20 bots/sec (dynamic) | Monitor throttler metrics |
| **Server Stability** | Crashes during mass spawn | Zero crashes | Stress test |
| **CPU Usage During Spawn** | >90% spikes | <85% peak | Resource monitor logs |
| **Memory Usage During Spawn** | >90% spikes | <80% peak | Resource monitor logs |
| **Circuit Breaker Triggers** | N/A | <5 per startup | Count breaker activations |
| **Spawn Failure Rate** | Unknown | <2% overall | Track success/failure ratio |

---

## 🛠️ Implementation Order

### Step 1: Foundation (ResourceMonitor)
1. Create `ResourceMonitor.h/cpp`
2. Implement Windows performance counter integration
3. Implement pressure level calculation
4. Unit tests for resource monitoring
5. Integration test with BotSpawner

### Step 2: Priority Queue
1. Create `SpawnPriorityQueue.h/cpp`
2. Implement priority heap data structure
3. Add enqueue/dequeue logic
4. Unit tests for priority ordering
5. Integration test with sample requests

### Step 3: Circuit Breaker
1. Create `SpawnCircuitBreaker.h/cpp`
2. Implement state machine (CLOSED/HALF_OPEN/OPEN)
3. Add failure rate tracking
4. Unit tests for state transitions
5. Integration test with failure scenarios

### Step 4: Adaptive Throttler
1. Create `AdaptiveSpawnThrottler.h/cpp`
2. Implement adaptive batch size algorithm
3. Integrate with ResourceMonitor
4. Add metrics collection
5. Unit tests for throttling logic

### Step 5: Startup Orchestrator
1. Create `StartupSpawnOrchestrator.h/cpp`
2. Implement phased spawning logic
3. Integrate with AdaptiveSpawnThrottler
4. Add phase transition logic
5. Integration test for full startup sequence

### Step 6: BotSpawner Integration
1. Modify `BotSpawner.h/cpp`
2. Integrate all Phase 2 components
3. Update spawn logic to use throttling
4. Add configuration loading
5. Full integration testing

---

## 🧪 Testing Strategy

### Unit Tests (Google Test Framework)
- ResourceMonitor metrics calculation
- SpawnPriorityQueue ordering
- SpawnCircuitBreaker state transitions
- AdaptiveSpawnThrottler batch size calculation

### Integration Tests
- 100-bot startup sequence
- 500-bot startup sequence
- Resource pressure simulation
- Circuit breaker failure handling

### Stress Tests
- 1000-bot rapid spawn
- 5000-bot full startup
- Resource exhaustion scenarios
- Recovery from circuit breaker

---

## 📝 Documentation Updates
- Update `.claude/BOT_LOGIN_REFACTORING_IMPLEMENTATION_SUMMARY.md`
- Create `.claude/PHASE_2_IMPLEMENTATION_COMPLETE.md`
- Update `system_architecture_overview.md` memory
- Update `playerbots.conf.dist` with Phase 2 config

---

## ✅ Phase 2 Completion Criteria
- [ ] All 5 components implemented (ResourceMonitor, SpawnPriorityQueue, etc.)
- [ ] BotSpawner integration complete
- [ ] Configuration system implemented
- [ ] Debug build successful
- [ ] RelWithDebInfo build successful
- [ ] Unit tests passing (>90% coverage)
- [ ] Integration tests passing
- [ ] 5000-bot stress test successful
- [ ] Documentation complete
- [ ] Code review passed

---

**Status**: Ready for Implementation
**Next Action**: Begin with Step 1 - ResourceMonitor implementation
**Estimated Timeline**: 2-3 weeks full-time development
