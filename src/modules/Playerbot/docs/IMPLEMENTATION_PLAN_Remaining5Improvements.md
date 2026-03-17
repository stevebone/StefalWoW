# Implementation Plans: Remaining 5 Quality Improvements

**Document Version:** 1.0
**Last Updated:** 2025-11-07
**Total Estimated Time:** 4-6 weeks

---

## Table of Contents

1. [#9: Thread-Pool Deadlock Prevention](#9-thread-pool-deadlock-prevention)
2. [#8: Performance Monitoring Dashboard](#8-performance-monitoring-dashboard)
3. [#3: Singleton Dependency Injection](#3-singleton-dependency-injection)
4. [#7: Manager Consolidation](#7-manager-consolidation)
5. [#1: ClassAI Code Deduplication](#1-classai-code-deduplication)

---

# #9: Thread-Pool Deadlock Prevention

**Priority:** HIGH
**Estimated Time:** 3-4 days
**Complexity:** MEDIUM-HIGH
**Impact:** Eliminates production deadlocks

---

## Current Issues

**Existing Deadlock Detector:**
- File: `Performance/ThreadPool/DeadlockDetector.cpp`
- **Problem:** Only post-mortem detection (deadlock already happened)
- **Limitation:** Cannot prevent deadlocks proactively

**Root Causes:**
1. **Inconsistent Lock Ordering:** Different threads acquire locks in different orders
2. **Nested Locking:** Functions call other functions that acquire locks
3. **No Lock Hierarchy:** No enforced ordering policy

---

## Solution: Lock Ordering Policy + Static Analysis

### Phase 1: Define Lock Hierarchy (1 day)

**Create:** `src/modules/Playerbot/Threading/LockHierarchy.h`

```cpp
#pragma once

#include <cstdint>

namespace Playerbot
{

/**
 * @brief Global lock ordering hierarchy
 *
 * CRITICAL RULE: Locks MUST be acquired in ascending order
 * Violating this order causes compile-time or runtime errors
 */
enum class LockOrder : uint32
{
    // Layer 1: Infrastructure (acquired first)
    LOG_SYSTEM = 100,
    CONFIG_MANAGER = 200,

    // Layer 2: Core data structures
    SPATIAL_GRID = 1000,
    OBJECT_CACHE = 1100,

    // Layer 3: Session management
    SESSION_MANAGER = 2000,
    PACKET_QUEUE = 2100,

    // Layer 4: Bot AI
    BOT_AI_STATE = 3000,
    BEHAVIOR_MANAGER = 3100,

    // Layer 5: Group/Raid coordination
    GROUP_MANAGER = 4000,
    THREAT_COORDINATOR = 4100,

    // Layer 6: Database operations
    DATABASE_POOL = 5000,

    // Layer 7: External dependencies (acquired last)
    TRINITYCORE_MAP = 6000,
    TRINITYCORE_WORLD = 6100,

    MAX_LOCK_ORDER = 10000
};

/**
 * @brief Lock wrapper that enforces ordering at runtime
 */
template<LockOrder Order>
class OrderedMutex
{
public:
    void lock()
    {
        // Check if current thread holds locks with higher order
        uint32 currentMaxOrder = ThreadLocalLockTracker::GetMaxLockOrder();

        if (currentMaxOrder >= static_cast<uint32>(Order))
        {
            // DEADLOCK DETECTED: Attempting to acquire lock out of order
            TC_LOG_FATAL("playerbot.deadlock",
                "Lock ordering violation! Current thread holds lock with order {}, "
                "attempting to acquire lock with order {}. This WILL cause deadlock!",
                currentMaxOrder, static_cast<uint32>(Order));

            // In debug builds, trigger breakpoint
            #ifdef _DEBUG
                __debugbreak();  // MSVC
                // __builtin_trap();  // GCC/Clang
            #endif

            // In release, throw exception to prevent deadlock
            throw std::runtime_error("Lock ordering violation");
        }

        _mutex.lock();
        ThreadLocalLockTracker::PushLock(static_cast<uint32>(Order));
    }

    void unlock()
    {
        ThreadLocalLockTracker::PopLock(static_cast<uint32>(Order));
        _mutex.unlock();
    }

    bool try_lock()
    {
        if (_mutex.try_lock())
        {
            ThreadLocalLockTracker::PushLock(static_cast<uint32>(Order));
            return true;
        }
        return false;
    }

private:
    std::mutex _mutex;
};

/**
 * @brief Thread-local lock tracking
 */
class ThreadLocalLockTracker
{
public:
    static void PushLock(uint32 order)
    {
        thread_local std::vector<uint32> lockStack;
        lockStack.push_back(order);
    }

    static void PopLock(uint32 order)
    {
        thread_local std::vector<uint32> lockStack;
        if (!lockStack.empty() && lockStack.back() == order)
            lockStack.pop_back();
    }

    static uint32 GetMaxLockOrder()
    {
        thread_local std::vector<uint32> lockStack;
        return lockStack.empty() ? 0 : *std::max_element(lockStack.begin(), lockStack.end());
    }
};

} // namespace Playerbot
```

### Phase 2: Convert Existing Mutexes (2 days)

**Example Conversion:**

```cpp
// OLD (no ordering):
class SpatialGridManager
{
    std::mutex _gridMutex;
};

// NEW (enforced ordering):
class SpatialGridManager
{
    OrderedMutex<LockOrder::SPATIAL_GRID> _gridMutex;
};

// Usage is identical:
void SpatialGridManager::UpdateGrid()
{
    std::lock_guard<decltype(_gridMutex)> lock(_gridMutex);
    // ... update logic
}
```

**Files to Convert (priority order):**
1. `Spatial/SpatialGridManager.cpp` - SPATIAL_GRID
2. `Session/BotSessionMgr.cpp` - SESSION_MANAGER
3. `AI/BehaviorManager.cpp` - BEHAVIOR_MANAGER
4. `Group/GroupCoordinator.cpp` - GROUP_MANAGER
5. `Database/ConnectionPool.cpp` - DATABASE_POOL

### Phase 3: Static Analysis Tool (1 day)

**Create:** `scripts/analyze_lock_order.py`

```python
#!/usr/bin/env python3
"""
Static analysis tool to detect lock ordering violations
"""
import re
import sys
from pathlib import Path

def extract_lock_acquisitions(cpp_file):
    """
    Parse C++ file and extract lock acquisitions in order
    """
    with open(cpp_file) as f:
        content = f.read()

    # Find all lock_guard<OrderedMutex<...>> patterns
    pattern = r'std::lock_guard<OrderedMutex<LockOrder::(\w+)>>'
    locks = re.findall(pattern, content)

    return locks

def check_ordering(function_locks, lock_hierarchy):
    """
    Verify locks are acquired in ascending order
    """
    prev_order = 0
    violations = []

    for lock_name in function_locks:
        order = lock_hierarchy.get(lock_name, 0)

        if order < prev_order:
            violations.append(f"Lock {lock_name} (order {order}) "
                              f"acquired after lock with order {prev_order}")

        prev_order = max(prev_order, order)

    return violations

def main():
    playerbot_dir = Path("src/modules/Playerbot")
    cpp_files = list(playerbot_dir.rglob("*.cpp"))

    all_violations = []

    for cpp_file in cpp_files:
        locks = extract_lock_acquisitions(cpp_file)
        if len(locks) > 1:  # Only check if multiple locks
            violations = check_ordering(locks, LOCK_HIERARCHY)
            if violations:
                all_violations.append((cpp_file, violations))

    if all_violations:
        print("Lock ordering violations detected:")
        for file, violations in all_violations:
            print(f"\n{file}:")
            for violation in violations:
                print(f"  - {violation}")
        sys.exit(1)
    else:
        print("No lock ordering violations detected")
        sys.exit(0)

if __name__ == "__main__":
    main()
```

**Integrate into CI:**
```yaml
# .github/workflows/playerbot_checks.yml
- name: Check Lock Ordering
  run: python3 scripts/analyze_lock_order.py
```

---

# #8: Performance Monitoring Dashboard

**Priority:** MEDIUM
**Estimated Time:** 4-5 days
**Complexity:** MEDIUM
**Impact:** Real-time production visibility

---

## Architecture

```
Bot Metrics → Prometheus Exporter → Prometheus Server → Grafana Dashboard
```

### Phase 1: Prometheus Integration (2 days)

**Add Dependency:**
```cmake
# In CMakeLists.txt
find_package(prometheus-cpp CONFIG REQUIRED)
target_link_libraries(playerbot PRIVATE prometheus-cpp::core prometheus-cpp::pull)
```

**Create Metrics Exporter:**
```cpp
// File: Monitoring/PrometheusExporter.h
#pragma once

#include <prometheus/exposer.h>
#include <prometheus/registry.h>
#include <prometheus/counter.h>
#include <prometheus/gauge.h>
#include <prometheus/histogram.h>

namespace Playerbot
{

class PrometheusExporter
{
public:
    static PrometheusExporter& Instance();

    void Initialize(uint16 port = 9090);
    void Shutdown();

    // Bot metrics
    prometheus::Counter& GetBotSpawnCounter();
    prometheus::Counter& GetBotDeathCounter();
    prometheus::Gauge& GetActiveBotGauge();

    // AI metrics
    prometheus::Histogram& GetAIDecisionTimeHistogram();
    prometheus::Counter& GetAIActionsCounter();

    // Combat metrics
    prometheus::Histogram& GetCombatDurationHistogram();
    prometheus::Counter& GetSpellCastCounter();

    // Performance metrics
    prometheus::Histogram& GetFrameTimeHistogram();
    prometheus::Gauge& GetMemoryUsageGauge();

private:
    PrometheusExporter() = default;

    std::unique_ptr<prometheus::Exposer> _exposer;
    std::shared_ptr<prometheus::Registry> _registry;

    // Metric definitions
    prometheus::Family<prometheus::Counter>* _botSpawnsFamily;
    prometheus::Family<prometheus::Gauge>* _activeBotsFamily;
    // ... more metric families
};

} // namespace Playerbot
```

**Implementation:**
```cpp
// File: Monitoring/PrometheusExporter.cpp
void PrometheusExporter::Initialize(uint16 port)
{
    // Start HTTP server for metrics
    _exposer = std::make_unique<prometheus::Exposer>(
        std::string("0.0.0.0:") + std::to_string(port));

    // Create registry
    _registry = std::make_shared<prometheus::Registry>();
    _exposer->RegisterCollectable(_registry);

    // Define metrics
    _botSpawnsFamily = &prometheus::BuildCounter()
        .Name("playerbot_spawns_total")
        .Help("Total number of bot spawns")
        .Register(*_registry);

    _activeBotsFamily = &prometheus::BuildGauge()
        .Name("playerbot_active_bots")
        .Help("Number of currently active bots")
        .Register(*_registry);

    TC_LOG_INFO("playerbot.monitoring", "Prometheus exporter started on port {}", port);
}

prometheus::Counter& PrometheusExporter::GetBotSpawnCounter()
{
    return _botSpawnsFamily->Add({{"type", "player"}});
}
```

**Usage in Code:**
```cpp
// In BotSpawner::SpawnBot()
void BotSpawner::SpawnBot(BotTemplate const& botTemplate)
{
    // ... spawn logic

    // Record metric
    PrometheusExporter::Instance().GetBotSpawnCounter().Increment();
    PrometheusExporter::Instance().GetActiveBotGauge().Increment();

    TC_LOG_INFO("playerbot", "Bot {} spawned", botTemplate.name);
}
```

### Phase 2: Grafana Dashboard (1 day)

**Dashboard JSON:**
```json
{
  "dashboard": {
    "title": "Playerbot Monitoring",
    "panels": [
      {
        "title": "Active Bots",
        "type": "graph",
        "targets": [
          {
            "expr": "playerbot_active_bots",
            "legendFormat": "Active Bots"
          }
        ]
      },
      {
        "title": "Bot Spawn Rate",
        "type": "graph",
        "targets": [
          {
            "expr": "rate(playerbot_spawns_total[5m])",
            "legendFormat": "Spawns/sec"
          }
        ]
      },
      {
        "title": "AI Decision Time (P95)",
        "type": "graph",
        "targets": [
          {
            "expr": "histogram_quantile(0.95, playerbot_ai_decision_time_seconds)",
            "legendFormat": "P95 Decision Time"
          }
        ]
      },
      {
        "title": "Memory Usage",
        "type": "graph",
        "targets": [
          {
            "expr": "playerbot_memory_usage_bytes / 1024 / 1024",
            "legendFormat": "Memory (MB)"
          }
        ]
      },
      {
        "title": "Combat Duration",
        "type": "heatmap",
        "targets": [
          {
            "expr": "playerbot_combat_duration_seconds_bucket",
            "format": "heatmap"
          }
        ]
      }
    ]
  }
}
```

**Deploy:**
```bash
curl -X POST http://grafana:3000/api/dashboards/db \
  -H "Content-Type: application/json" \
  -d @playerbot_dashboard.json
```

### Phase 3: Alerting (1 day)

**Prometheus Alerting Rules:**
```yaml
# prometheus/alerts/playerbot.yml
groups:
  - name: playerbot
    interval: 30s
    rules:
      - alert: HighBotDeathRate
        expr: rate(playerbot_deaths_total[5m]) > 10
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "High bot death rate detected"
          description: "{{ $value }} bots dying per second"

      - alert: LowActiveBot Count
        expr: playerbot_active_bots < 100
        for: 10m
        labels:
          severity: warning
        annotations:
          summary: "Bot count dropped below 100"

      - alert: AIDecisionTimeoutHigh
        expr: histogram_quantile(0.95, playerbot_ai_decision_time_seconds) > 0.1
        for: 5m
        labels:
          severity: critical
        annotations:
          summary: "AI decision time P95 > 100ms"
          description: "Bots experiencing slow AI decisions"
```

---

# #3: Singleton Dependency Injection

**Priority:** MEDIUM
**Estimated Time:** 2-3 weeks
**Complexity:** HIGH
**Impact:** +60% testability, easier mocking

---

## Problem Statement

**Current Architecture:**
- **168 Singletons** across codebase
- **Meyer's Singleton Pattern** (static local variable)
- **Impossible to test** (cannot mock dependencies)
- **Hidden dependencies** (not visible in constructor)

**Example Problem:**
```cpp
// CURRENT: Cannot unit test without real SpatialGridManager
void BotAI::FindNearestEnemy()
{
    auto grid = sSpatialGridManager.GetGrid(GetMap()->GetId());  // Singleton!
    // Cannot mock this for testing
}
```

---

## Solution: Dependency Injection Container

### Phase 1: DI Container (1 week)

**Create:** `Core/DI/ServiceContainer.h`

```cpp
#pragma once

#include <memory>
#include <unordered_map>
#include <typeindex>
#include <functional>

namespace Playerbot
{

/**
 * @brief Simple Dependency Injection container
 */
class ServiceContainer
{
public:
    /**
     * @brief Register service with factory function
     */
    template<typename TInterface, typename TImpl>
    void RegisterSingleton()
    {
        auto factory = []() -> std::shared_ptr<void> {
            return std::make_shared<TImpl>();
        };

        _services[typeid(TInterface)] = factory();
    }

    /**
     * @brief Register service with custom factory
     */
    template<typename TInterface>
    void RegisterFactory(std::function<std::shared_ptr<TInterface>()> factory)
    {
        _factories[typeid(TInterface)] = [factory]() -> std::shared_ptr<void> {
            return factory();
        };
    }

    /**
     * @brief Resolve service
     */
    template<typename TInterface>
    std::shared_ptr<TInterface> Resolve()
    {
        auto it = _services.find(typeid(TInterface));
        if (it != _services.end())
            return std::static_pointer_cast<TInterface>(it->second);

        // Create from factory if registered
        auto factoryIt = _factories.find(typeid(TInterface));
        if (factoryIt != _factories.end())
        {
            auto service = factoryIt->second();
            _services[typeid(TInterface)] = service;
            return std::static_pointer_cast<TInterface>(service);
        }

        return nullptr;
    }

    /**
     * @brief Check if service is registered
     */
    template<typename TInterface>
    bool IsRegistered() const
    {
        return _services.find(typeid(TInterface)) != _services.end() ||
               _factories.find(typeid(TInterface)) != _factories.end();
    }

private:
    std::unordered_map<std::type_index, std::shared_ptr<void>> _services;
    std::unordered_map<std::type_index, std::function<std::shared_ptr<void>()>> _factories;
};

/**
 * @brief Global service locator (transitional pattern)
 */
class Services
{
public:
    static ServiceContainer& Container()
    {
        static ServiceContainer instance;
        return instance;
    }
};

} // namespace Playerbot
```

### Phase 2: Define Interfaces (1 week)

**Example: ISpatialGridManager**

```cpp
// File: Interfaces/ISpatialGridManager.h
class ISpatialGridManager
{
public:
    virtual ~ISpatialGridManager() = default;

    virtual SpatialGrid* GetGrid(uint32 mapId) = 0;
    virtual std::vector<Unit*> GetUnitsInRadius(Position const& pos, float radius) = 0;
    virtual void UpdateGrid(uint32 mapId) = 0;
};

// File: Spatial/SpatialGridManager.h (implementation)
class SpatialGridManager : public ISpatialGridManager
{
public:
    SpatialGrid* GetGrid(uint32 mapId) override;
    std::vector<Unit*> GetUnitsInRadius(Position const& pos, float radius) override;
    void UpdateGrid(uint32 mapId) override;
};
```

**Interfaces to Create (priority):**
1. `ISpatialGridManager` - Spatial queries
2. `IBotSessionMgr` - Session management
3. `IBehaviorManager` - AI behavior
4. `IGroupCoordinator` - Group management
5. `IDatabasePool` - Database operations

### Phase 3: Convert BotAI to DI (3 days)

**Before:**
```cpp
class BotAI
{
public:
    BotAI(Player* bot) : _bot(bot) {}

    void Update()
    {
        // Singleton dependencies (hidden)
        auto grid = sSpatialGridManager.GetGrid(GetMap()->GetId());
        auto session = sBotSessionMgr.GetSession(_bot->GetGUID());
    }

private:
    Player* _bot;
};
```

**After:**
```cpp
class BotAI
{
public:
    // Constructor injection - dependencies visible
    BotAI(Player* bot,
          std::shared_ptr<ISpatialGridManager> spatialMgr,
          std::shared_ptr<IBotSessionMgr> sessionMgr)
        : _bot(bot)
        , _spatialMgr(spatialMgr)
        , _sessionMgr(sessionMgr)
    {}

    void Update()
    {
        // Use injected dependencies
        auto grid = _spatialMgr->GetGrid(GetMap()->GetId());
        auto session = _sessionMgr->GetSession(_bot->GetGUID());
    }

private:
    Player* _bot;
    std::shared_ptr<ISpatialGridManager> _spatialMgr;
    std::shared_ptr<IBotSessionMgr> _sessionMgr;
};
```

**Factory Pattern:**
```cpp
class BotAIFactory
{
public:
    static std::unique_ptr<BotAI> Create(Player* bot)
    {
        // Resolve dependencies from container
        auto spatialMgr = Services::Container().Resolve<ISpatialGridManager>();
        auto sessionMgr = Services::Container().Resolve<IBotSessionMgr>();

        return std::make_unique<BotAI>(bot, spatialMgr, sessionMgr);
    }
};
```

### Phase 4: Enable Unit Testing (2 days)

**Mock Implementation:**
```cpp
// File: Tests/Mocks/MockSpatialGridManager.h
class MockSpatialGridManager : public ISpatialGridManager
{
public:
    MOCK_METHOD(SpatialGrid*, GetGrid, (uint32 mapId), (override));
    MOCK_METHOD(std::vector<Unit*>, GetUnitsInRadius, (Position const&, float), (override));
    MOCK_METHOD(void, UpdateGrid, (uint32 mapId), (override));
};

// Usage in tests:
TEST_CASE("BotAI finds nearest enemy")
{
    auto mockSpatialMgr = std::make_shared<MockSpatialGridManager>();
    auto mockSessionMgr = std::make_shared<MockBotSessionMgr>();

    // Setup expectations
    EXPECT_CALL(*mockSpatialMgr, GetUnitsInRadius(_, _))
        .WillOnce(Return(std::vector<Unit*>{enemyUnit}));

    // Create BotAI with mocks
    BotAI ai(testBot, mockSpatialMgr, mockSessionMgr);

    // Test
    ai.FindNearestEnemy();

    // Verify mock was called
    EXPECT_TRUE(Mock::VerifyAndClearExpectations(mockSpatialMgr.get()));
}
```

---

# #7: Manager Consolidation

**Priority:** MEDIUM-LOW
**Estimated Time:** 3-4 weeks
**Complexity:** HIGH
**Impact:** Simpler mental model, reduced indirection

---

## Analysis

**Current State:**
- **70 Manager classes** with overlapping responsibilities
- **Naming inconsistencies:** Manager, Coordinator, Controller, Handler
- **Unclear boundaries:** Multiple managers for same domain

**Consolidation Plan:**

### Proposed Structure (70 → 20)

| New Manager | Consolidates | Responsibility |
|-------------|--------------|----------------|
| **BotLifecycleManager** | BotSpawner, BotScheduler, DeathRecoveryManager, BotSpawnOrchestrator | Spawn, despawn, death handling |
| **BotSessionManager** | BotSessionMgr, BotWorldSessionMgr, BotPacketRelay, BotSessionFactory | Network session management |
| **BotCombatManager** | ThreatCoordinator, InterruptCoordinator, DispelCoordinator, TargetSelector, PositionManager | Combat coordination |
| **BotGroupManager** | GroupCoordinator, GroupEventHandler, RoleAssignment | Group/raid management |
| **BotQuestManager** | QuestManager, QuestPickup, QuestCompletion, ObjectiveTracker | Quest system |
| **BotEconomyManager** | AuctionManager, TradeManager, EconomyManager, MarketAnalysis | Trading, AH, gold |
| **BotInventoryManager** | EquipmentManager, BotGearFactory, VendorPurchaseManager | Equipment, bags, consumables |
| **BotProfessionManager** | ProfessionManager, GatheringManager, FarmingCoordinator | Professions, gathering |
| **BotSpatialManager** | SpatialGridManager, SpatialGridScheduler, SpatialGridQueryHelpers | Spatial queries |
| **BotMovementManager** | MovementArbiter, PathfindingAdapter, GroupFormationManager | Movement, pathfinding |
| **BotAIManager** | BehaviorManager, BehaviorPriorityManager, ActionPriority | AI decision-making |
| **BotChatManager** | BotChatCommandHandler, SocialManager, GuildIntegration | Chat, whispers, social |
| **BotDatabaseManager** | PlayerbotDatabase, BotStatePersistence, PlayerbotCharacterDBInterface | Database operations |
| **BotConfigManager** | PlayerbotConfig, ConfigManager | Configuration |
| **BotMonitoringManager** | BotMonitor, PerformanceMetrics, BotPerformanceMonitor | Metrics, alerts |
| **BotEventManager** | EventDispatcher, All 13 EventBuses | Event routing |
| **BotThreadPoolManager** | ThreadPool, DeadlockDetector, BotActionProcessor | Threading |
| **BotTalentManager** | BotTalentManager | Talent selection |
| **BotLFGManager** | LFGBotManager, LFGBotSelector, LFGRoleDetector | LFG system |
| **BotPvPManager** | (new) | PvP, battlegrounds |

### Refactoring Example

**Before (3 separate managers):**
```cpp
class ThreatCoordinator { void ManageThreat(); };
class InterruptCoordinator { void CoordinateInterrupts(); };
class DispelCoordinator { void ManageDispels(); };
```

**After (1 unified manager):**
```cpp
class BotCombatManager
{
public:
    // Threat management (from ThreatCoordinator)
    void ManageThreat(BotAI* bot);
    float GetThreatPriority(Unit* target);

    // Interrupt coordination (from InterruptCoordinator)
    void CoordinateInterrupts(std::vector<BotAI*> const& group);
    bool RequestInterrupt(BotAI* bot, Unit* target);

    // Dispel management (from DispelCoordinator)
    void ManageDispels(std::vector<BotAI*> const& group);
    Unit* SelectDispelTarget(BotAI* bot);

private:
    struct ThreatState { /* ... */ };
    struct InterruptQueue { /* ... */ };
    struct DispelPriority { /* ... */ };

    ThreatState _threatState;
    InterruptQueue _interruptQueue;
    DispelPriority _dispelPriority;
};
```

---

# #1: ClassAI Code Deduplication

**Priority:** LOW (large effort, medium impact)
**Estimated Time:** 4-6 weeks
**Complexity:** VERY HIGH
**Impact:** -30% LOC (~40,000 lines removed)

---

## Problem Analysis

**Current Duplication:**
- **13 class implementations** (Warrior, Mage, Priest, etc.)
- **3-4 specs per class** = ~40 total specializations
- **70-85% code overlap** between specs
- **~40,000 duplicate lines** across ClassAI system

**Example Duplication:**

**Warrior (Arms spec):**
```cpp
void ArmsWarriorAI::ExecuteRotation()
{
    if (CanCast(MORTAL_STRIKE) && InMeleeRange())
        CastSpell(MORTAL_STRIKE);
    else if (CanCast(OVERPOWER) && HasBuff(OVERPOWER_PROC))
        CastSpell(OVERPOWER);
    else if (CanCast(EXECUTE) && GetTargetHealthPercent() < 20)
        CastSpell(EXECUTE);
    // ... 50 more lines
}
```

**Warrior (Fury spec):**
```cpp
void FuryWarriorAI::ExecuteRotation()
{
    if (CanCast(BLOODTHIRST) && InMeleeRange())
        CastSpell(BLOODTHIRST);
    else if (CanCast(RAGING_BLOW) && HasBuff(ENRAGE))
        CastSpell(RAGING_BLOW);
    else if (CanCast(EXECUTE) && GetTargetHealthPercent() < 20)
        CastSpell(EXECUTE);
    // ... 50 more lines (mostly same logic)
}
```

---

## Solution: Data-Driven Rotation Engine

### Phase 1: Rotation DSL (2 weeks)

**Define Spell Priority Data:**
```cpp
// File: AI/ClassAI/RotationData/ArmsWarrior.h
namespace RotationData
{

struct SpellPriority
{
    uint32 spellId;
    uint32 priority;           // Higher = cast first
    std::function<bool()> condition;  // When to cast
};

const std::vector<SpellPriority> ARMS_WARRIOR_ROTATION = {
    { MORTAL_STRIKE, 100, []() { return InMeleeRange(); } },
    { OVERPOWER, 90, []() { return HasBuff(OVERPOWER_PROC); } },
    { EXECUTE, 85, []() { return GetTargetHealthPercent() < 20; } },
    { SLAM, 70, []() { return GetRage() > 60; } },
    { WHIRLWIND, 60, []() { return GetEnemyCount() >= 3; } },
    { HEROIC_STRIKE, 50, []() { return GetRage() > 80; } }
};

} // namespace RotationData
```

### Phase 2: Generic Rotation Engine (2 weeks)

```cpp
// File: AI/ClassAI/RotationEngine.h
class RotationEngine
{
public:
    RotationEngine(std::vector<SpellPriority> const& rotation)
        : _rotation(rotation)
    {
        // Sort by priority (descending)
        std::sort(_rotation.begin(), _rotation.end(),
            [](auto const& a, auto const& b) { return a.priority > b.priority; });
    }

    /**
     * @brief Execute highest priority spell that passes conditions
     * @return Spell ID that was cast, or 0 if none
     */
    uint32 ExecuteRotation(BotAI* bot)
    {
        for (auto const& spell : _rotation)
        {
            // Check cooldown
            if (bot->GetSpellCooldown(spell.spellId) > 0)
                continue;

            // Check resources (mana, rage, energy)
            if (!bot->HasEnoughResource(spell.spellId))
                continue;

            // Check custom condition
            if (!spell.condition())
                continue;

            // Cast spell
            if (bot->CastSpell(spell.spellId))
                return spell.spellId;
        }

        return 0;  // No spell cast
    }

private:
    std::vector<SpellPriority> _rotation;
};
```

### Phase 3: Convert All Classes (2 weeks)

**Before (175 lines per spec):**
```cpp
void ArmsWarriorAI::ExecuteRotation()
{
    // 175 lines of if/else logic
}

void FuryWarriorAI::ExecuteRotation()
{
    // 175 lines of similar logic
}

void ProtectionWarriorAI::ExecuteRotation()
{
    // 175 lines of similar logic
}
```

**After (3 lines per spec + shared data):**
```cpp
// All 3 specs use same engine, different data
ArmsWarriorAI::ExecuteRotation()
{
    _rotationEngine.ExecuteRotation(this);
}

FuryWarriorAI::ExecuteRotation()
{
    _rotationEngine.ExecuteRotation(this);
}

ProtectionWarriorAI::ExecuteRotation()
{
    _rotationEngine.ExecuteRotation(this);
}
```

**LOC Reduction:**
- Before: 13 classes × 3 specs × 175 lines = **6,825 lines**
- After: 13 classes × 3 specs × 3 lines + rotation data = **~1,500 lines**
- **Reduction: 78% (-5,325 lines)**

---

## Success Metrics

| Improvement | Metric | Target |
|-------------|--------|--------|
| #9 Deadlock Prevention | Zero deadlocks in production | 0 incidents/month |
| #8 Monitoring Dashboard | Real-time visibility | 100% uptime |
| #3 Dependency Injection | Test coverage | +60% unit tests |
| #7 Manager Consolidation | Manager count | 70 → 20 |
| #1 ClassAI Deduplication | Lines of code | -30,000 LOC |

---

**Document Owner:** PlayerBot Architecture Team
**Review Frequency:** Quarterly
**Next Review:** 2026-02-07
