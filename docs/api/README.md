# TrinityCore PlayerBot API Documentation

**Version**: 1.0
**Last Updated**: 2025-10-04

---

## Overview

This directory contains API documentation for the TrinityCore PlayerBot module. The API is designed for developers who want to extend, customize, or integrate with the PlayerBot system.

---

## Documentation Generation

### Using Doxygen

The PlayerBot module uses Doxygen for API documentation generation. All public APIs are documented with Doxygen-style comments.

#### Prerequisites

- **Doxygen** 1.9.0 or later
- **Graphviz** (for class diagrams)

#### Installation

**Windows**:
```powershell
# Using Chocolatey
choco install doxygen.install
choco install graphviz

# Or download from:
# https://www.doxygen.nl/download.html
# https://graphviz.org/download/
```

**Linux**:
```bash
# Ubuntu/Debian
sudo apt-get install doxygen graphviz

# Fedora/RHEL
sudo dnf install doxygen graphviz

# Arch
sudo pacman -S doxygen graphviz
```

**macOS**:
```bash
brew install doxygen graphviz
```

#### Generate Documentation

```bash
# Navigate to docs/api directory
cd docs/api

# Generate HTML documentation
doxygen Doxyfile

# Output will be in docs/api/html/
# Open docs/api/html/index.html in browser
```

---

## API Categories

### Core APIs

**BotSession Management**:
- `BotSessionMgr::CreateSession()` - Create new bot session
- `BotSessionMgr::DestroySession()` - Destroy bot session
- `BotSessionMgr::GetSession()` - Get existing session

**BotAI Framework**:
- `BotAI::UpdateAI()` - Main AI update loop
- `BotAI::HandleEvent()` - Event handler
- `BotAI::ExecuteAction()` - Action execution

**ClassAI System**:
- `ClassAI::ExecuteRotation()` - Spec-specific rotation
- `ClassAI::HandleCombat()` - Combat logic
- `ClassAI::UseAbilities()` - Ability usage

### Performance APIs

**ThreadPool**:
- `ThreadPool::Submit()` - Submit async task
- `ThreadPool::GetMetrics()` - Get performance metrics
- `ThreadPool::SetWorkerCount()` - Configure workers

**MemoryPool**:
- `MemoryPool<T>::Allocate()` - Allocate object
- `MemoryPool<T>::Deallocate()` - Free object
- `MemoryPool<T>::GetMetrics()` - Get memory metrics

**QueryOptimizer**:
- `QueryOptimizer::ExecuteQuery()` - Execute optimized query
- `QueryOptimizer::GetMetrics()` - Get query metrics
- `QueryOptimizer::ClearCache()` - Clear statement cache

**Profiler**:
- `Profiler::ScopedTimer` - RAII profiling
- `Profiler::GetResults()` - Get profiling data
- `PROFILE_FUNCTION()` - Convenience macro

### Game System APIs

**Combat**:
- `TargetSelector::SelectTarget()` - Target selection
- `InterruptCoordinator::ShouldInterrupt()` - Interrupt coordination
- `CooldownManager::IsReady()` - Cooldown tracking

**Movement**:
- `MovementManager::MoveToPosition()` - Pathfinding movement
- `LeaderFollowBehavior::FollowLeader()` - Leader following
- `PositionManager::GetOptimalPosition()` - Position calculation

**Quest**:
- `QuestPickup::PickupQuests()` - Auto quest pickup
- `QuestManager::UpdateProgress()` - Quest progress tracking
- `QuestManager::CompleteQuest()` - Quest completion

**Social**:
- `SocialManager::SendChatMessage()` - Chat interaction
- `SocialManager::PerformEmote()` - Emote execution
- `GuildManager::JoinGuild()` - Guild integration

---

## Quick Start Examples

### Creating a Custom Bot Action

```cpp
#include "AI/BotAI.h"
#include "AI/Actions/Action.h"

namespace Playerbot {

/**
 * @brief Custom action example
 *
 * Demonstrates how to create a custom bot action.
 */
class CustomAction : public Action
{
public:
    /**
     * @brief Constructor
     * @param ai Pointer to bot AI instance
     */
    CustomAction(BotAI* ai) : Action(ai, "custom_action") {}

    /**
     * @brief Execute the action
     * @param event Event that triggered this action
     * @return true if action succeeded, false otherwise
     */
    bool Execute(Event event) override
    {
        Player* bot = GetBot();
        if (!bot)
            return false;

        // Your custom logic here
        LOG_INFO("playerbot", "CustomAction executed for bot {}", bot->GetName());

        return true;
    }

    /**
     * @brief Check if action is valid
     * @return true if action can be executed
     */
    bool IsValid() override
    {
        return GetBot() != nullptr;
    }
};

} // namespace Playerbot
```

### Using ThreadPool for Async Tasks

```cpp
#include "Performance/ThreadPool/ThreadPool.h"

namespace Playerbot {

void ProcessBotsAsync(std::vector<BotAI*>& bots, uint32 diff)
{
    ThreadPool& pool = GetThreadPool();

    for (BotAI* bot : bots)
    {
        // Submit bot update as async task with NORMAL priority
        auto future = pool.Submit(TaskPriority::NORMAL, [bot, diff]() {
            bot->UpdateAI(diff);
            return true;
        });

        // Optional: Wait for completion
        // future.get();
    }
}

void ProcessCriticalReaction(BotAI* bot, Unit* attacker)
{
    ThreadPool& pool = GetThreadPool();

    // Submit critical combat reaction with HIGH priority
    pool.Submit(TaskPriority::CRITICAL, [bot, attacker]() {
        bot->HandleAttacked(attacker);
    });
}

} // namespace Playerbot
```

### Using MemoryPool for Efficient Allocation

```cpp
#include "Performance/MemoryPool/MemoryPool.h"

namespace Playerbot {

class BotFactory
{
    MemoryPool<BotAI> _aiPool;

public:
    BotAI* CreateBotAI(Player* player)
    {
        // Fast allocation from pool (<100ns)
        BotAI* ai = _aiPool.Allocate(player);
        return ai;
    }

    void DestroyBotAI(BotAI* ai)
    {
        // Return to pool for reuse
        _aiPool.Deallocate(ai);
    }
};

} // namespace Playerbot
```

### Custom ClassAI Specialization

```cpp
#include "AI/ClassAI/ClassAI.h"

namespace Playerbot {

/**
 * @brief Custom specialization AI
 *
 * Example of extending ClassAI for a specific specialization.
 */
class CustomMageSpecialization : public ClassAI
{
public:
    CustomMageSpecialization(BotAI* ai) : ClassAI(ai, "CustomMage") {}

    /**
     * @brief Execute rotation
     * @return true if rotation executed successfully
     */
    bool ExecuteRotation() override
    {
        Player* bot = GetBot();
        Unit* target = bot->GetSelectedUnit();

        if (!target || !target->IsAlive())
            return false;

        // Custom rotation logic
        if (CanCast("Fireball"))
            return CastSpell("Fireball", target);

        return false;
    }

    /**
     * @brief Handle combat start
     */
    void OnCombatStart(Unit* target) override
    {
        LOG_INFO("playerbot", "CustomMage entering combat with {}", target->GetName());

        // Buff up before combat
        CastSpell("Arcane Intellect", GetBot());
    }

    /**
     * @brief Handle combat end
     */
    void OnCombatEnd() override
    {
        LOG_INFO("playerbot", "CustomMage exiting combat");

        // Restore mana after combat
        if (GetBot()->GetPowerPct(POWER_MANA) < 50)
            CastSpell("Evocation", GetBot());
    }
};

} // namespace Playerbot
```

### Profiling Performance

```cpp
#include "Performance/Profiler/Profiler.h"

namespace Playerbot {

void ExpensiveOperation()
{
    // Automatic profiling with RAII
    PROFILE_FUNCTION();

    // Expensive code here...
    for (int i = 0; i < 1000000; ++i)
    {
        // Work...
    }
}

void ManualProfiling()
{
    {
        Profiler::ScopedTimer timer("CustomSection");

        // Code to profile
        ProcessBots();
    }

    // Get results
    auto results = Profiler::Instance().GetResults();
    for (const auto& [section, data] : results.sections)
    {
        LOG_INFO("playerbot", "Section: {}, Avg: {}us, Calls: {}",
            section, data.GetAverage(), data.callCount.load());
    }
}

} // namespace Playerbot
```

---

## API Reference Structure

### Namespaces

- `Playerbot` - Main namespace for all PlayerBot code
- `Playerbot::Performance` - Performance optimization components
- `Playerbot::AI` - AI framework and behaviors
- `Playerbot::Combat` - Combat-related systems
- `Playerbot::Movement` - Movement and pathfinding
- `Playerbot::Quest` - Quest automation
- `Playerbot::Social` - Social features

### Key Classes

#### Core Framework
- `BotSessionMgr` - Session management singleton
- `BotSession` - Individual bot session
- `BotScheduler` - Bot update coordination
- `BotAccountMgr` - Account creation/deletion
- `PlayerbotConfig` - Configuration management

#### AI Framework
- `BotAI` - Base AI class
- `ClassAI` - Class-specific AI base
- `Action` - Individual bot action
- `Trigger` - Event trigger
- `Strategy` - Behavioral strategy
- `EventBus` - Event distribution

#### Performance Systems
- `ThreadPool` - Work-stealing thread pool
- `MemoryPool<T>` - Object pooling
- `QueryOptimizer` - Database optimization
- `Profiler` - Performance profiling
- `PerformanceManager` - Central coordinator

#### Combat Systems
- `TargetSelector` - Target selection
- `InterruptCoordinator` - Interrupt timing
- `CooldownManager` - Cooldown tracking
- `ThreatManager` - Threat calculation
- `PositionManager` - Tactical positioning

#### Game Systems
- `MovementManager` - Movement control
- `QuestManager` - Quest automation
- `GroupCoordinator` - Group mechanics
- `EconomyManager` - AH and professions
- `SocialManager` - Social features

---

## Code Documentation Standards

### Doxygen Comment Style

```cpp
/**
 * @brief Brief description (one line)
 *
 * Detailed description (multiple lines if needed).
 * Explains what the function/class does and why.
 *
 * @param paramName Description of parameter
 * @param anotherParam Description of another parameter
 * @return Description of return value
 *
 * @throws ExceptionType When this exception is thrown
 *
 * @note Important notes about usage
 * @warning Warnings about edge cases or performance
 *
 * @code
 * // Example usage
 * MyClass obj;
 * obj.DoSomething(42, "test");
 * @endcode
 *
 * @see RelatedFunction
 * @see RelatedClass
 */
ReturnType FunctionName(ParamType paramName, AnotherType anotherParam);
```

### Documentation Requirements

All public APIs must include:
- ✅ Brief description (`@brief`)
- ✅ Parameter descriptions (`@param`)
- ✅ Return value description (`@return`)
- ✅ Exception documentation (`@throws`)
- ✅ Usage examples (`@code`)
- ✅ Related function references (`@see`)

---

## Building with Documentation

### CMake Configuration

```cmake
# Enable API documentation generation
cmake .. -DBUILD_PLAYERBOT_DOCS=ON

# Build with documentation
cmake --build . --target playerbot_docs
```

This will:
1. Run Doxygen to generate HTML docs
2. Generate class diagrams with Graphviz
3. Create searchable API reference
4. Output to `build/docs/api/html/`

---

## Contributing to API Documentation

### Guidelines

1. **Document as you code** - Don't defer documentation
2. **Use examples** - Show how to use the API
3. **Keep it updated** - Update docs when changing APIs
4. **Link related items** - Use `@see` for related functions
5. **Explain edge cases** - Document limitations and gotchas

### Documentation Review Checklist

- [ ] All public functions documented
- [ ] All parameters described
- [ ] Return values explained
- [ ] Examples provided for complex APIs
- [ ] Thread safety noted
- [ ] Performance characteristics mentioned
- [ ] Related functions linked

---

## Additional Resources

### External Documentation

- **TrinityCore Documentation**: https://trinitycore.info/
- **C++ Reference**: https://en.cppreference.com/
- **Doxygen Manual**: https://www.doxygen.nl/manual/

### Project Documentation

- **User Guide**: `docs/guides/PLAYERBOT_USER_GUIDE.md`
- **Developer Guide**: `docs/guides/PLAYERBOT_DEVELOPER_GUIDE.md`
- **Performance Tuning**: `docs/guides/PLAYERBOT_PERFORMANCE_TUNING.md`
- **Architecture**: `docs/architecture/PLAYERBOT_ARCHITECTURE.md`

---

## Support

For API questions or issues:

1. **Check API docs**: `docs/api/html/index.html` (after generation)
2. **Read guides**: `docs/guides/`
3. **GitHub Issues**: https://github.com/TrinityCore/TrinityCore/issues
4. **Discord**: https://discord.gg/trinitycore

---

**End of API Documentation**

*Last Updated: 2025-10-04*
*Version: 1.0*
*TrinityCore PlayerBot - Enterprise Edition*
