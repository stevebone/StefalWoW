# PlayerBot Developer Guide

## Table of Contents

1. [Introduction](#introduction)
2. [Development Setup](#development-setup)
3. [Adding a New Manager](#adding-a-new-manager)
4. [Adding a New Strategy](#adding-a-new-strategy)
5. [Writing Tests](#writing-tests)
6. [Debugging Techniques](#debugging-techniques)
7. [Performance Profiling](#performance-profiling)
8. [Common Pitfalls](#common-pitfalls)
9. [Best Practices](#best-practices)
10. [Code Examples](#code-examples)

## Introduction

This guide provides comprehensive instructions for developers working on the PlayerBot module for TrinityCore 11.2. The PlayerBot system is a sophisticated AI framework that enables bots to behave like real players, supporting up to 5000 concurrent bots with minimal performance impact.

### Prerequisites

- C++20 compatible compiler (MSVC 2022, GCC 11+, Clang 14+)
- CMake 3.24+
- Boost 1.74+
- MySQL 9.4
- Understanding of TrinityCore architecture
- Basic knowledge of game mechanics

## Development Setup

### Building the Module

```bash
# Clone the repository
git clone https://github.com/TrinityCore/TrinityCore.git
cd TrinityCore
git checkout playerbot-dev

# Create build directory
mkdir build
cd build

# Configure with PlayerBot enabled
cmake .. -DWITH_PLAYERBOT=1 -DCMAKE_BUILD_TYPE=RelWithDebInfo

# Build (adjust -j based on your CPU cores)
make -j8

# Or on Windows with Visual Studio
msbuild TrinityCore.sln /p:Configuration=RelWithDebInfo /m
```

### Configuration

Create `playerbots.conf` in your server directory:

```ini
# Enable PlayerBot system
Playerbot.Enable = 1

# Performance settings
Playerbot.MaxBots = 100
Playerbot.UpdateInterval = 100

# Manager intervals (milliseconds)
Playerbot.QuestManager.UpdateInterval = 2000
Playerbot.TradeManager.UpdateInterval = 5000
Playerbot.GatheringManager.UpdateInterval = 1000
Playerbot.AuctionManager.UpdateInterval = 10000
```

## Adding a New Manager

### Step 1: Create Manager Header

```cpp
// src/modules/Playerbot/CustomManager.h
#pragma once

#include "AI/Strategy/BehaviorManager.h"
#include <atomic>

namespace Playerbot
{
    class CustomManager : public BehaviorManager
    {
    public:
        explicit CustomManager(BotAI* ai);
        ~CustomManager() override = default;

        // BehaviorManager interface
        std::string GetName() const override { return "CustomManager"; }

        // Custom public interface
        bool HasWork() const { return m_hasWork.load(); }
        void QueueTask(uint32 taskId);

    protected:
        // Required virtual method from BehaviorManager
        void UpdateBehavior(uint32 timeDelta) override;

    private:
        // State management (atomic for thread safety)
        std::atomic<bool> m_hasWork{false};
        std::atomic<uint32> m_taskCount{0};

        // Internal methods
        void ProcessTasks();
        void ValidateState();
    };
}
```

### Step 2: Implement Manager Logic

```cpp
// src/modules/Playerbot/CustomManager.cpp
#include "CustomManager.h"
#include "AI/BotAI.h"
#include "Player.h"
#include "Log.h"

namespace Playerbot
{
    CustomManager::CustomManager(BotAI* ai)
        : BehaviorManager(ai, 3000) // 3 second update interval
    {
        // Initialize manager state
        LOG_DEBUG("bot.ai", "CustomManager initialized for bot {}",
                  m_ai->GetBot()->GetName());
    }

    void CustomManager::UpdateBehavior(uint32 timeDelta)
    {
        // This method is called by BehaviorManager template method
        // It's already throttled based on update interval

        if (!m_ai || !m_ai->GetBot())
            return;

        Player* bot = m_ai->GetBot();

        // Validate bot state
        if (bot->isDead() || bot->IsBeingTeleported())
            return;

        // Process manager logic
        ProcessTasks();
        ValidateState();

        // Update state flags
        m_hasWork.store(m_taskCount.load() > 0);
    }

    void CustomManager::ProcessTasks()
    {
        uint32 tasks = m_taskCount.load();
        if (tasks == 0)
            return;

        // Process up to 5 tasks per update
        uint32 processed = 0;
        while (processed < 5 && m_taskCount.load() > 0)
        {
            // Task processing logic here
            m_taskCount.fetch_sub(1);
            processed++;
        }

        LOG_DEBUG("bot.ai", "CustomManager processed {} tasks", processed);
    }

    void CustomManager::QueueTask(uint32 taskId)
    {
        m_taskCount.fetch_add(1);
        m_hasWork.store(true);
    }
}
```

### Step 3: Register Manager in BotAI

```cpp
// In BotAI.h - Add to manager list
class BotAI
{
private:
    std::unique_ptr<CustomManager> m_customManager;
    // ... other managers
};

// In BotAI.cpp - Initialize in constructor
BotAI::BotAI(Player* bot) : m_bot(bot)
{
    // ... existing initialization
    m_customManager = std::make_unique<CustomManager>(this);
    m_customManager->Enable(); // Enable by default or based on config
}

// In UpdateManagers method
void BotAI::UpdateManagers(uint32 diff)
{
    // ... existing managers
    if (m_customManager)
        m_customManager->Update(diff);
}
```

## Adding a New Strategy

### Step 1: Define Strategy Interface

```cpp
// src/modules/Playerbot/AI/Strategy/CustomStrategy.h
#pragma once

#include "Strategy.h"
#include <vector>

namespace Playerbot
{
    class CustomStrategy : public Strategy
    {
    public:
        explicit CustomStrategy(BotAI* ai);
        ~CustomStrategy() override = default;

        // Strategy interface
        std::string getName() const override { return "custom"; }
        int getPriority() const override { return 50; }

        // Triggers this strategy handles
        void InitTriggers(std::list<std::shared_ptr<Trigger>>& triggers) override;

        // Actions this strategy can perform
        void InitActions(ActionList& actions) override;

    private:
        bool ShouldActivate() const;
        void ExecuteRotation();
    };
}
```

### Step 2: Implement Strategy Logic

```cpp
// src/modules/Playerbot/AI/Strategy/CustomStrategy.cpp
#include "CustomStrategy.h"
#include "AI/BotAI.h"
#include "Actions/CustomAction.h"
#include "Triggers/CustomTrigger.h"

namespace Playerbot
{
    CustomStrategy::CustomStrategy(BotAI* ai) : Strategy(ai)
    {
        // Initialize strategy-specific data
    }

    void CustomStrategy::InitTriggers(std::list<std::shared_ptr<Trigger>>& triggers)
    {
        // Add triggers that activate this strategy
        triggers.push_back(std::make_shared<CustomTrigger>(m_ai));
        triggers.push_back(std::make_shared<HealthLowTrigger>(m_ai));
    }

    void CustomStrategy::InitActions(ActionList& actions)
    {
        // Define action priority mappings
        actions.push_back(ActionNode("custom action", 100));
        actions.push_back(ActionNode("heal", 90));
        actions.push_back(ActionNode("flee", 80));
    }

    bool CustomStrategy::ShouldActivate() const
    {
        if (!m_ai || !m_ai->GetBot())
            return false;

        Player* bot = m_ai->GetBot();

        // Custom activation conditions
        return bot->GetHealthPct() < 50.0f ||
               bot->GetPowerPct(bot->getPowerType()) < 20.0f;
    }
}
```

## Writing Tests

### Unit Test Template

```cpp
// src/modules/Playerbot/Tests/CustomManagerTest.cpp
#include "gtest/gtest.h"
#include "CustomManager.h"
#include "TestUtilities.h"

using namespace Playerbot;

class CustomManagerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        // Create test bot and AI
        m_testBot = TestUtilities::CreateTestBot();
        m_botAI = std::make_unique<BotAI>(m_testBot);
        m_manager = std::make_unique<CustomManager>(m_botAI.get());
    }

    void TearDown() override
    {
        m_manager.reset();
        m_botAI.reset();
        TestUtilities::CleanupTestBot(m_testBot);
    }

    Player* m_testBot = nullptr;
    std::unique_ptr<BotAI> m_botAI;
    std::unique_ptr<CustomManager> m_manager;
};

TEST_F(CustomManagerTest, InitializationTest)
{
    EXPECT_NE(m_manager, nullptr);
    EXPECT_FALSE(m_manager->IsEnabled());
    EXPECT_EQ(m_manager->GetName(), "CustomManager");
}

TEST_F(CustomManagerTest, EnableDisableTest)
{
    m_manager->Enable();
    EXPECT_TRUE(m_manager->IsEnabled());

    m_manager->Disable();
    EXPECT_FALSE(m_manager->IsEnabled());
}

TEST_F(CustomManagerTest, UpdateThrottlingTest)
{
    m_manager->Enable();
    m_manager->SetUpdateInterval(1000); // 1 second

    // First update should execute
    m_manager->Update(500);
    EXPECT_EQ(m_manager->GetUpdateCount(), 0); // Not enough time

    // Second update should trigger
    m_manager->Update(600);
    EXPECT_EQ(m_manager->GetUpdateCount(), 1); // Should update now
}

TEST_F(CustomManagerTest, PerformanceTest)
{
    m_manager->Enable();

    auto start = std::chrono::high_resolution_clock::now();

    // Simulate 1000 updates
    for (int i = 0; i < 1000; ++i)
    {
        m_manager->Update(33); // ~30 FPS
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // Should complete in under 100ms
    EXPECT_LT(duration.count(), 100);
}
```

### Integration Test Example

```cpp
// src/modules/Playerbot/Tests/IntegrationTest.cpp
TEST(BotIntegration, FullUpdateCycle)
{
    // Create bot environment
    auto bot = TestUtilities::CreateTestBot();
    auto ai = std::make_unique<BotAI>(bot);

    // Enable all managers
    ai->EnableManager(ManagerType::Quest);
    ai->EnableManager(ManagerType::Trade);
    ai->EnableManager(ManagerType::Gathering);

    // Simulate game loop
    for (int frame = 0; frame < 100; ++frame)
    {
        ai->UpdateAI(33); // 33ms per frame (~30 FPS)
    }

    // Verify no crashes and proper state
    EXPECT_TRUE(bot->IsAlive());
    EXPECT_GE(ai->GetPerformanceMetrics().totalUpdates, 100);
}
```

## Debugging Techniques

### 1. Enable Verbose Logging

```cpp
// In your code
LOG_DEBUG("bot.ai", "Manager {} updating, state: {}, tasks: {}",
          GetName(), m_enabled.load(), m_taskCount.load());

// Set log level in config
LogLevel = 3  # Debug level
Logger.bot.ai = 3,Console Server
```

### 2. Use Performance Profiling Macros

```cpp
#define PROFILE_SCOPE(name) \
    ProfileTimer timer##__LINE__(name)

class ProfileTimer
{
public:
    explicit ProfileTimer(const char* name) : m_name(name)
    {
        m_start = std::chrono::high_resolution_clock::now();
    }

    ~ProfileTimer()
    {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - m_start);
        LOG_DEBUG("bot.performance", "{}: {}μs", m_name, duration.count());
    }

private:
    const char* m_name;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_start;
};

// Usage
void UpdateBehavior(uint32 timeDelta) override
{
    PROFILE_SCOPE("CustomManager::UpdateBehavior");
    // Your code here
}
```

### 3. Visual Studio Debugging

```cpp
// Add conditional breakpoints
if (m_taskCount.load() > 100)
{
    __debugbreak(); // Windows-specific breakpoint
}

// Use debug visualizers in .natvis file
<?xml version="1.0" encoding="utf-8"?>
<AutoVisualizer xmlns="http://schemas.microsoft.com/vstudio/debugger/natvis/2010">
  <Type Name="Playerbot::BehaviorManager">
    <DisplayString>{{Manager: {m_name}, Enabled: {m_enabled}, Interval: {m_updateInterval}ms}}</DisplayString>
  </Type>
</AutoVisualizer>
```

### 4. Memory Leak Detection

```cpp
// Windows-specific memory debugging
#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

// In main() or test setup
_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

// Linux with Valgrind
// valgrind --leak-check=full --track-origins=yes ./worldserver
```

## Performance Profiling

### 1. Built-in Performance Metrics

```cpp
class PerformanceMonitor
{
public:
    void StartFrame()
    {
        m_frameStart = std::chrono::high_resolution_clock::now();
    }

    void EndFrame()
    {
        auto frameEnd = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>
                       (frameEnd - m_frameStart);

        m_frameTimes.push_back(duration.count());

        if (m_frameTimes.size() > 100)
            m_frameTimes.erase(m_frameTimes.begin());
    }

    double GetAverageFrameTime() const
    {
        if (m_frameTimes.empty())
            return 0.0;

        double sum = 0.0;
        for (auto time : m_frameTimes)
            sum += time;

        return sum / m_frameTimes.size();
    }

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> m_frameStart;
    std::vector<uint64_t> m_frameTimes;
};
```

### 2. CPU Profiling with perf (Linux)

```bash
# Record performance data
perf record -g ./worldserver

# Analyze results
perf report

# Generate flame graph
perf script | stackcollapse-perf.pl | flamegraph.pl > flame.svg
```

### 3. Visual Studio Performance Profiler

1. Open project in Visual Studio 2022
2. Debug → Performance Profiler
3. Select "CPU Usage" and "Memory Usage"
4. Start profiling with debugging
5. Analyze hot paths and memory allocations

## Common Pitfalls

### 1. Forgetting Atomic Operations

```cpp
// WRONG - Race condition
class BadManager
{
    bool m_enabled = false; // Not atomic!

    void Enable() { m_enabled = true; }
    bool IsEnabled() { return m_enabled; }
};

// CORRECT - Thread-safe
class GoodManager
{
    std::atomic<bool> m_enabled{false};

    void Enable() { m_enabled.store(true); }
    bool IsEnabled() { return m_enabled.load(); }
};
```

### 2. Blocking Operations in Update Loop

```cpp
// WRONG - Blocks entire update
void UpdateBehavior(uint32 timeDelta) override
{
    // This can block for seconds!
    auto result = DatabaseQuery("SELECT * FROM huge_table");
    ProcessResult(result);
}

// CORRECT - Async or cached
void UpdateBehavior(uint32 timeDelta) override
{
    if (m_cacheExpired)
    {
        // Queue async query
        QueryDatabase(callback);
        return;
    }

    ProcessCachedData();
}
```

### 3. Memory Leaks with Shared Pointers

```cpp
// WRONG - Circular reference
class A
{
    std::shared_ptr<B> b;
};

class B
{
    std::shared_ptr<A> a; // Circular!
};

// CORRECT - Use weak_ptr
class A
{
    std::shared_ptr<B> b;
};

class B
{
    std::weak_ptr<A> a; // Breaks cycle
};
```

## Best Practices

### 1. Follow RAII Principles

```cpp
class ResourceManager
{
public:
    ResourceManager()
    {
        m_resource = AcquireResource();
    }

    ~ResourceManager()
    {
        if (m_resource)
            ReleaseResource(m_resource);
    }

    // Delete copy operations
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;

    // Allow move operations
    ResourceManager(ResourceManager&& other) noexcept
        : m_resource(std::exchange(other.m_resource, nullptr))
    {
    }

private:
    Resource* m_resource = nullptr;
};
```

### 2. Use Modern C++20 Features

```cpp
// Concepts for template constraints
template<typename T>
concept Updatable = requires(T t, uint32 diff)
{
    { t.Update(diff) } -> std::same_as<void>;
    { t.IsEnabled() } -> std::convertible_to<bool>;
};

template<Updatable T>
class UpdateQueue
{
    // Only accepts types that can be updated
};

// Designated initializers
struct ManagerConfig
{
    uint32 updateInterval = 1000;
    bool enabled = false;
    std::string name;
};

auto config = ManagerConfig{
    .updateInterval = 2000,
    .enabled = true,
    .name = "QuestManager"
};

// Ranges and views
auto activeManagers = m_managers
    | std::views::filter([](auto& mgr) { return mgr->IsEnabled(); })
    | std::views::take(5);
```

### 3. Document Intent with Comments

```cpp
class BehaviorManager
{
protected:
    // Template method pattern - defines the update algorithm
    // Derived classes override UpdateBehavior() to customize
    void Update(uint32 diff)
    {
        // Early exit for disabled managers (atomic check)
        if (!m_enabled.load(std::memory_order_acquire))
            return;

        // Throttling mechanism - reduces CPU by 80-95%
        m_timeSinceLastUpdate += diff;
        if (m_timeSinceLastUpdate < m_updateInterval)
            return;

        // Performance tracking for profiling
        auto start = std::chrono::high_resolution_clock::now();

        // Call derived implementation
        UpdateBehavior(m_timeSinceLastUpdate);

        // Calculate and store performance metrics
        auto end = std::chrono::high_resolution_clock::now();
        m_lastUpdateDuration = std::chrono::duration_cast<std::chrono::microseconds>
                              (end - start).count();

        // Reset throttle timer for next cycle
        m_timeSinceLastUpdate = 0;
    }
};
```

### 4. Error Handling Strategy

```cpp
enum class ErrorAction
{
    IGNORE,      // Log and continue
    RETRY,       // Retry operation
    DISABLE,     // Disable component
    FATAL        // Terminate bot
};

template<typename Func>
void ExecuteWithErrorHandling(Func&& func, ErrorAction onError = ErrorAction::IGNORE)
{
    try
    {
        func();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("bot.ai", "Exception in bot update: {}", e.what());

        switch (onError)
        {
            case ErrorAction::RETRY:
                // Retry once
                try { func(); }
                catch (...) { /* Give up */ }
                break;

            case ErrorAction::DISABLE:
                Disable();
                break;

            case ErrorAction::FATAL:
                m_ai->DisableBot();
                break;

            default:
                break;
        }
    }
}
```

## Code Examples

### Complete Manager Implementation

```cpp
// FishingManager.h
#pragma once

#include "AI/Strategy/BehaviorManager.h"
#include <atomic>
#include <chrono>

namespace Playerbot
{
    class FishingManager : public BehaviorManager
    {
    public:
        explicit FishingManager(BotAI* ai);
        ~FishingManager() override = default;

        std::string GetName() const override { return "FishingManager"; }

        bool IsFishing() const { return m_isFishing.load(); }
        bool HasFishingPole() const;
        uint32 GetFishCaught() const { return m_fishCaught.load(); }

    protected:
        void UpdateBehavior(uint32 timeDelta) override;

    private:
        void FindFishingSpot();
        void CastLine();
        void CheckBobber();
        void LootFish();

        std::atomic<bool> m_isFishing{false};
        std::atomic<uint32> m_fishCaught{0};
        std::chrono::steady_clock::time_point m_castTime;
        ObjectGuid m_bobberGuid;
    };
}

// FishingManager.cpp
#include "FishingManager.h"
#include "AI/BotAI.h"
#include "GameObject.h"
#include "SpellInfo.h"

namespace Playerbot
{
    FishingManager::FishingManager(BotAI* ai)
        : BehaviorManager(ai, 2000) // Check every 2 seconds
    {
    }

    void FishingManager::UpdateBehavior(uint32 timeDelta)
    {
        if (!m_ai || !m_ai->GetBot())
            return;

        Player* bot = m_ai->GetBot();

        // Can't fish while in combat or dead
        if (bot->IsInCombat() || bot->isDead())
        {
            m_isFishing.store(false);
            return;
        }

        // Check if we have a fishing pole equipped
        if (!HasFishingPole())
            return;

        if (!m_isFishing.load())
        {
            FindFishingSpot();
            if (bot->IsNearWater(5.0f))
            {
                CastLine();
            }
        }
        else
        {
            CheckBobber();
        }
    }

    bool FishingManager::HasFishingPole() const
    {
        Player* bot = m_ai->GetBot();
        Item* mainHand = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);

        if (!mainHand)
            return false;

        ItemTemplate const* proto = mainHand->GetTemplate();
        return proto && proto->SubClass == ITEM_SUBCLASS_WEAPON_FISHING_POLE;
    }

    void FishingManager::CastLine()
    {
        Player* bot = m_ai->GetBot();

        // Cast fishing spell (id: 131474 for current expansion)
        if (bot->HasSpell(131474))
        {
            bot->CastSpell(bot, 131474, false);
            m_isFishing.store(true);
            m_castTime = std::chrono::steady_clock::now();

            LOG_DEBUG("bot.ai.fishing", "Bot {} started fishing", bot->GetName());
        }
    }

    void FishingManager::CheckBobber()
    {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_castTime);

        // Fishing cast lasts ~20 seconds
        if (elapsed.count() > 20)
        {
            m_isFishing.store(false);
            return;
        }

        Player* bot = m_ai->GetBot();

        // Find fishing bobber game object
        GameObject* bobber = bot->GetGameObjectIfCanInteractWith(m_bobberGuid, GAMEOBJECT_TYPE_FISHINGNODE);
        if (bobber && bobber->IsReady())
        {
            LootFish();
        }
    }

    void FishingManager::LootFish()
    {
        Player* bot = m_ai->GetBot();

        GameObject* bobber = bot->GetGameObjectIfCanInteractWith(m_bobberGuid, GAMEOBJECT_TYPE_FISHINGNODE);
        if (!bobber)
            return;

        bot->SendLoot(bobber->GetGUID(), LOOT_FISHING);
        m_fishCaught.fetch_add(1);
        m_isFishing.store(false);

        LOG_DEBUG("bot.ai.fishing", "Bot {} caught fish! Total: {}",
                  bot->GetName(), m_fishCaught.load());
    }

    void FishingManager::FindFishingSpot()
    {
        // Simple logic - could be enhanced with pathfinding to water
        Player* bot = m_ai->GetBot();

        if (!bot->IsNearWater(10.0f))
        {
            // Move towards nearest water
            // This would integrate with movement system
        }
    }
}
```

## Conclusion

This developer guide provides the foundation for extending and maintaining the PlayerBot system. Key takeaways:

1. **Always use BehaviorManager** as base for new managers
2. **Implement strategies** for combat and behavior logic
3. **Write comprehensive tests** for all new features
4. **Profile performance** regularly
5. **Follow modern C++20** best practices
6. **Maintain thread safety** with atomic operations
7. **Document your code** thoroughly

For additional help, consult the existing code examples in `src/modules/Playerbot/` and the integration tests in the Tests directory.

---

*Developer Guide Version 2.0*
*TrinityCore 11.2 - The War Within*
*Last Updated: October 2024*