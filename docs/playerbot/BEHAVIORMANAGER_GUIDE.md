# BehaviorManager Developer Guide

**Version**: 1.0
**Module**: Playerbot
**Audience**: Developers implementing new bot managers

---

## Table of Contents

1. [Introduction](#introduction)
2. [When to Use BehaviorManager](#when-to-use-behaviormanager)
3. [Step-by-Step Tutorial](#step-by-step-tutorial)
4. [Best Practices](#best-practices)
5. [Common Patterns](#common-patterns)
6. [Common Pitfalls](#common-pitfalls)
7. [Performance Tuning](#performance-tuning)
8. [Debugging Techniques](#debugging-techniques)
9. [Integration with BotAI](#integration-with-botai)
10. [Testing Your Manager](#testing-your-manager)

---

## Introduction

The `BehaviorManager` base class is the foundation for all bot behavior management systems in the TrinityCore Playerbot module. It solves a critical performance problem: **how to run AI logic for thousands of bots without overwhelming the CPU**.

### The Problem

Without throttling:
- Each bot's AI runs every frame (~100 FPS = 100 updates/second)
- 1000 bots × 100 updates/sec = 100,000 updates/second
- Even at 1ms per update, that's 100 seconds of CPU time per second (100% of 100 cores!)

### The Solution

BehaviorManager implements **intelligent throttling**:
- `Update()` called every frame (cheap: <0.001ms)
- `OnUpdate()` called at configured intervals (e.g., every 1-5 seconds)
- 1000 bots × 1 update/second × 5ms = 5 seconds of CPU time per second (5% of 100 cores)

**Result**: 20x reduction in CPU usage with minimal impact on bot responsiveness.

---

## When to Use BehaviorManager

### Use BehaviorManager For:

1. **Quest Management**
   - Tracking quest progress
   - Finding quest objectives
   - Managing quest rewards

2. **Trade/Economy Management**
   - Evaluating trade offers
   - Managing inventory
   - Interacting with vendors

3. **Profession Management**
   - Tracking profession skills
   - Finding gathering nodes
   - Crafting items

4. **Social Behavior**
   - Managing group membership
   - Processing chat messages
   - Responding to player interactions

5. **Strategic Planning**
   - Long-term goal planning
   - Route optimization
   - Resource management

### Do NOT Use BehaviorManager For:

1. **Combat Logic** (use OnCombatUpdate in ClassAI instead)
   - Combat needs frame-by-frame updates
   - Throttling breaks responsiveness
   - Use class-specific AI for combat

2. **Movement** (use strategies instead)
   - Movement must be smooth (no throttling)
   - Use LeaderFollowBehavior or movement strategies

3. **Critical Reactions** (use triggers instead)
   - Immediate response required (no delay acceptable)
   - Use trigger system for instant reactions

4. **Frame-Sensitive Logic** (use UpdateAI directly)
   - Animation timing
   - Precise positioning
   - Collision detection

### Decision Matrix

| Criteria | Use BehaviorManager | Alternative |
|----------|---------------------|-------------|
| Updates needed | < 10 per second | >= 10 per second |
| Response time | 100ms+ acceptable | < 100ms required |
| CPU cost | > 1ms per update | < 1ms per update |
| State queries | Frequent from strategies | Rare |
| Scope | Background/strategic | Combat/movement |

---

## Step-by-Step Tutorial

This tutorial will guide you through creating a complete manager from scratch. We'll build a `PetManager` that tracks hunter pet status and manages pet actions.

### Step 1: Create Header File

Create `src/modules/Playerbot/AI/Managers/PetManager.h`:

```cpp
/*
 * This file is part of the TrinityCore Project.
 * See AUTHORS file for Copyright information.
 */

#ifndef TRINITYCORE_PET_MANAGER_H
#define TRINITYCORE_PET_MANAGER_H

#include "BehaviorManager.h"
#include <atomic>

class Player;
class Pet;

namespace Playerbot
{
    class BotAI;

    /**
     * @class PetManager
     * @brief Manages hunter/warlock pet behavior for bots
     *
     * Responsibilities:
     * - Track pet summon status
     * - Monitor pet health/happiness
     * - Manage pet abilities
     * - Handle pet revive/heal
     */
    class TC_GAME_API PetManager : public BehaviorManager
    {
    public:
        /**
         * @brief Construct PetManager
         * @param bot The bot player (must be hunter/warlock)
         * @param ai The bot AI controller
         */
        explicit PetManager(Player* bot, BotAI* ai);

        /**
         * @brief Destructor
         */
        ~PetManager() override;

        // ====================================================================
        // PUBLIC QUERY INTERFACE (Thread-safe, lock-free)
        // ====================================================================

        /**
         * @brief Check if bot has an active pet
         * @return true if pet exists and is alive
         */
        bool HasPet() const
        {
            return m_hasPet.load(std::memory_order_acquire);
        }

        /**
         * @brief Check if pet needs attention (low health, happiness, etc.)
         * @return true if pet needs immediate care
         */
        bool PetNeedsAttention() const
        {
            return m_petNeedsAttention.load(std::memory_order_acquire);
        }

        /**
         * @brief Get pet health percentage
         * @return Health percent (0-100), or 0 if no pet
         */
        uint32 GetPetHealthPercent() const
        {
            return m_petHealthPercent.load(std::memory_order_acquire);
        }

        /**
         * @brief Check if pet can attack current target
         * @return true if pet is ready to attack
         */
        bool CanPetAttack() const
        {
            return m_petCanAttack.load(std::memory_order_acquire);
        }

        // ====================================================================
        // EXTERNAL EVENT HANDLERS
        // ====================================================================

        /**
         * @brief Called when pet dies
         */
        void OnPetDied();

        /**
         * @brief Called when pet is summoned
         */
        void OnPetSummoned();

    protected:
        // ====================================================================
        // BEHAVIORMANAGER INTERFACE
        // ====================================================================

        /**
         * @brief Update pet status and perform actions
         * @param elapsed Time since last update in milliseconds
         */
        void OnUpdate(uint32 elapsed) override;

        /**
         * @brief Initialize pet manager
         * @return true if initialized successfully
         */
        bool OnInitialize() override;

        /**
         * @brief Cleanup on shutdown
         */
        void OnShutdown() override;

    private:
        // ====================================================================
        // INTERNAL METHODS
        // ====================================================================

        /**
         * @brief Update pet status flags
         */
        void UpdatePetStatus();

        /**
         * @brief Check if pet needs healing
         */
        void CheckPetHealth();

        /**
         * @brief Check if pet needs feeding (hunter only)
         */
        void CheckPetHappiness();

        /**
         * @brief Summon pet if missing
         */
        void SummonPetIfNeeded();

        /**
         * @brief Get bot's pet
         * @return Pet pointer or nullptr
         */
        Pet* GetPet() const;

        // ====================================================================
        // ATOMIC STATE FLAGS (for lock-free queries)
        // ====================================================================

        std::atomic<bool> m_hasPet{false};              ///< Bot has active pet
        std::atomic<bool> m_petNeedsAttention{false};   ///< Pet needs care
        std::atomic<uint32> m_petHealthPercent{0};      ///< Pet health (0-100)
        std::atomic<bool> m_petCanAttack{false};        ///< Pet ready to attack

        // ====================================================================
        // INTERNAL STATE
        // ====================================================================

        ObjectGuid m_petGuid;                           ///< Current pet GUID
        uint32 m_lastPetSummonAttempt{0};              ///< Last summon attempt time
        uint32 m_lastPetFeedTime{0};                   ///< Last feeding time (hunter)
        uint32 m_petSummonCooldown{5000};              ///< Cooldown between summon attempts
    };

} // namespace Playerbot

#endif // TRINITYCORE_PET_MANAGER_H
```

### Step 2: Create Implementation File

Create `src/modules/Playerbot/AI/Managers/PetManager.cpp`:

```cpp
/*
 * This file is part of the TrinityCore Project.
 * See AUTHORS file for Copyright information.
 */

#include "PetManager.h"
#include "BotAI.h"
#include "Player.h"
#include "Pet.h"
#include "Log.h"
#include "Timer.h"

namespace Playerbot
{
    PetManager::PetManager(Player* bot, BotAI* ai)
        : BehaviorManager(bot, ai, 2000, "PetManager") // 2 second update interval
    {
        // Manager name and interval set in base class
        LogDebug("Created for bot {}", bot ? bot->GetName() : "unknown");
    }

    PetManager::~PetManager()
    {
        // Call shutdown to ensure cleanup
        OnShutdown();

        // Log final statistics
        if (GetBot())
        {
            LogDebug("Shutting down for bot {} - {} total updates",
                     GetBot()->GetName(), m_updateCount.load());
        }
    }

    bool PetManager::OnInitialize()
    {
        Player* bot = GetBot();
        if (!bot)
            return false;

        // Check if bot is in world
        if (!bot->IsInWorld())
        {
            LogDebug("Bot {} not in world yet, deferring initialization", bot->GetName());
            return false; // Retry next update
        }

        // Check if bot is a pet class (hunter or warlock)
        uint8 botClass = bot->getClass();
        if (botClass != CLASS_HUNTER && botClass != CLASS_WARLOCK)
        {
            LogWarning("Bot {} is not a pet class ({}), disabling manager",
                       bot->GetName(), botClass);
            SetEnabled(false);
            return true; // Don't retry
        }

        // Check if bot already has a pet
        Pet* pet = GetPet();
        if (pet)
        {
            m_petGuid = pet->GetGUID();
            LogDebug("Bot {} already has pet: {}", bot->GetName(), pet->GetName());
        }

        LogDebug("Initialized successfully for bot {} (Class: {})",
                 bot->GetName(), botClass);

        return true;
    }

    void PetManager::OnShutdown()
    {
        // Clear pet GUID
        m_petGuid = ObjectGuid::Empty;

        // Reset all flags
        m_hasPet.store(false, std::memory_order_release);
        m_petNeedsAttention.store(false, std::memory_order_release);
        m_petHealthPercent.store(0, std::memory_order_release);
        m_petCanAttack.store(false, std::memory_order_release);

        LogDebug("Shutdown complete");
    }

    void PetManager::OnUpdate(uint32 elapsed)
    {
        Player* bot = GetBot();
        if (!bot)
            return;

        // Update pet status flags
        UpdatePetStatus();

        // Get pet pointer
        Pet* pet = GetPet();

        if (pet && pet->IsAlive())
        {
            // Pet exists and is alive - check its status
            CheckPetHealth();

            // Hunter-specific: check happiness
            if (bot->getClass() == CLASS_HUNTER)
            {
                CheckPetHappiness();
            }

            // Update work flag - we have work if pet needs attention
            m_hasWork.store(m_petNeedsAttention.load(std::memory_order_acquire),
                            std::memory_order_release);
        }
        else
        {
            // No pet or pet is dead - try to summon
            SummonPetIfNeeded();

            // Always have work when pet is missing
            m_hasWork.store(true, std::memory_order_release);
        }
    }

    void PetManager::UpdatePetStatus()
    {
        Pet* pet = GetPet();

        if (!pet || !pet->IsAlive())
        {
            // No pet or pet is dead
            m_hasPet.store(false, std::memory_order_release);
            m_petHealthPercent.store(0, std::memory_order_release);
            m_petCanAttack.store(false, std::memory_order_release);
            m_petNeedsAttention.store(true, std::memory_order_release); // Missing pet needs attention
            return;
        }

        // Pet exists and is alive
        m_hasPet.store(true, std::memory_order_release);

        // Calculate health percentage
        uint32 healthPercent = pet->GetHealthPct();
        m_petHealthPercent.store(healthPercent, std::memory_order_release);

        // Check if pet can attack
        bool canAttack = !pet->HasUnitState(UNIT_STATE_CASTING) &&
                        !pet->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED) &&
                        pet->GetCurrentSpell(CURRENT_MELEE_SPELL) == nullptr;
        m_petCanAttack.store(canAttack, std::memory_order_release);

        // Determine if pet needs attention
        bool needsAttention = healthPercent < 50; // Low health threshold

        // Hunter: also check happiness
        if (GetBot()->getClass() == CLASS_HUNTER)
        {
            // Note: Happiness system varies by WoW version
            // This is a simplified check - implement based on your TrinityCore version
            // needsAttention |= (pet->GetHappinessState() != HAPPY);
        }

        m_petNeedsAttention.store(needsAttention, std::memory_order_release);
    }

    void PetManager::CheckPetHealth()
    {
        Pet* pet = GetPet();
        if (!pet)
            return;

        uint32 healthPercent = pet->GetHealthPct();

        if (healthPercent < 30)
        {
            LogDebug("Pet health critical: {}%", healthPercent);

            // Request immediate update for healing
            m_needsUpdate.store(true, std::memory_order_release);

            // TODO: Implement pet healing logic
            // - Cast Mend Pet (hunter)
            // - Use Health Funnel (warlock)
            // - Feed pet health-restoring items
        }
        else if (healthPercent < 50)
        {
            LogDebug("Pet health low: {}%", healthPercent);
        }
    }

    void PetManager::CheckPetHappiness()
    {
        Player* bot = GetBot();
        if (!bot || bot->getClass() != CLASS_HUNTER)
            return;

        Pet* pet = GetPet();
        if (!pet)
            return;

        // Check if enough time has passed since last feed
        uint32 currentTime = getMSTime();
        if (getMSTimeDiff(m_lastPetFeedTime, currentTime) < 30000) // 30 seconds
            return;

        // TODO: Implement happiness checking based on TrinityCore version
        // Hunter pet happiness system varies by expansion

        // Example pseudocode:
        // if (pet->GetHappinessState() != HAPPY)
        // {
        //     FeedPet();
        //     m_lastPetFeedTime = currentTime;
        // }
    }

    void PetManager::SummonPetIfNeeded()
    {
        Player* bot = GetBot();
        if (!bot)
            return;

        // Check summon cooldown
        uint32 currentTime = getMSTime();
        if (getMSTimeDiff(m_lastPetSummonAttempt, currentTime) < m_petSummonCooldown)
            return;

        // Check if bot is in combat (don't summon during combat)
        if (bot->IsInCombat())
        {
            LogDebug("Bot in combat, deferring pet summon");
            return;
        }

        // Check if bot is casting
        if (bot->IsNonMeleeSpellCast(false))
        {
            LogDebug("Bot is casting, deferring pet summon");
            return;
        }

        LogDebug("Attempting to summon pet for bot {}", bot->GetName());

        // TODO: Implement pet summoning based on class
        // Hunter: Call Pet spell
        // Warlock: Summon appropriate demon

        m_lastPetSummonAttempt = currentTime;
    }

    void PetManager::OnPetDied()
    {
        LogDebug("Pet died for bot {}", GetBot() ? GetBot()->GetName() : "unknown");

        // Clear pet GUID
        m_petGuid = ObjectGuid::Empty;

        // Force immediate update
        ForceUpdate();
    }

    void PetManager::OnPetSummoned()
    {
        Pet* pet = GetPet();
        if (pet)
        {
            m_petGuid = pet->GetGUID();
            LogDebug("Pet summoned for bot {}: {}",
                     GetBot() ? GetBot()->GetName() : "unknown",
                     pet->GetName());
        }

        // Force immediate update
        ForceUpdate();
    }

    Pet* PetManager::GetPet() const
    {
        Player* bot = GetBot();
        if (!bot)
            return nullptr;

        return bot->GetPet();
    }

} // namespace Playerbot
```

### Step 3: Add to CMakeLists.txt

Edit `src/modules/Playerbot/CMakeLists.txt`:

```cmake
# Add your new files to the appropriate section
set(PLAYERBOT_AI_MANAGERS
    AI/Managers/PetManager.h
    AI/Managers/PetManager.cpp
    # ... other managers
)
```

### Step 4: Integrate with BotAI

Edit `src/modules/Playerbot/AI/BotAI.h`:

```cpp
class TC_GAME_API BotAI
{
public:
    // ... existing code ...

    // Add accessor
    PetManager* GetPetManager() { return _petManager.get(); }
    PetManager const* GetPetManager() const { return _petManager.get(); }

protected:
    // ... existing code ...

    // Add member
    std::unique_ptr<PetManager> _petManager;
};
```

Edit `src/modules/Playerbot/AI/BotAI.cpp`:

```cpp
#include "PetManager.h"

BotAI::BotAI(Player* bot)
    : _bot(bot)
    // ... existing initialization ...
{
    // Create pet manager for pet classes
    uint8 botClass = bot->getClass();
    if (botClass == CLASS_HUNTER || botClass == CLASS_WARLOCK)
    {
        _petManager = std::make_unique<PetManager>(bot, this);
    }
}

void BotAI::UpdateAI(uint32 diff)
{
    // ... existing update code ...

    // Update pet manager
    if (_petManager)
        _petManager->Update(diff);
}
```

### Step 5: Use in Strategies

Create a strategy that uses the PetManager:

```cpp
// In PetStrategy.cpp
bool PetStrategy::IsRelevant()
{
    PetManager* pm = m_ai->GetPetManager();
    if (!pm || !pm->IsEnabled())
        return false;

    // Strategy is relevant if we have work to do
    return pm->m_hasWork.load(std::memory_order_acquire);
}

void PetStrategy::Execute()
{
    PetManager* pm = m_ai->GetPetManager();
    if (!pm)
        return;

    // Check if pet needs attention
    if (pm->PetNeedsAttention())
    {
        if (pm->GetPetHealthPercent() < 30)
        {
            // Execute heal pet action
            m_ai->ExecuteAction("heal_pet");
        }
        else if (!pm->HasPet())
        {
            // Execute summon pet action
            m_ai->ExecuteAction("summon_pet");
        }
    }

    // Check if pet can attack current target
    if (pm->CanPetAttack() && m_ai->IsInCombat())
    {
        // Execute pet attack action
        m_ai->ExecuteAction("pet_attack");
    }
}
```

### Step 6: Test Your Manager

Create unit tests in `src/modules/Playerbot/Tests/PetManagerTest.cpp`:

```cpp
#include "catch2/catch.hpp"
#include "PetManager.h"
#include "TestHelpers.h"

using namespace Playerbot;

TEST_CASE("PetManager - Construction", "[petmanager]")
{
    auto [bot, ai] = CreateTestBot(CLASS_HUNTER);
    PetManager manager(bot, ai);

    REQUIRE(manager.IsEnabled());
    REQUIRE(!manager.IsInitialized());
    REQUIRE(!manager.IsBusy());
}

TEST_CASE("PetManager - Initialization", "[petmanager]")
{
    auto [bot, ai] = CreateTestBot(CLASS_HUNTER);
    PetManager manager(bot, ai);

    // Bot not in world - initialization should fail
    REQUIRE(manager.OnInitialize() == false);

    // Add bot to world
    AddBotToWorld(bot);

    // Initialization should succeed
    REQUIRE(manager.OnInitialize() == true);
    REQUIRE(manager.IsInitialized());
}

TEST_CASE("PetManager - Update without pet", "[petmanager]")
{
    auto [bot, ai] = CreateTestBot(CLASS_HUNTER);
    AddBotToWorld(bot);

    PetManager manager(bot, ai);
    manager.OnInitialize();

    // Update should set hasWork=true (missing pet)
    manager.OnUpdate(1000);

    REQUIRE(!manager.HasPet());
    REQUIRE(manager.m_hasWork.load() == true);
}

TEST_CASE("PetManager - Update with healthy pet", "[petmanager]")
{
    auto [bot, ai] = CreateTestBot(CLASS_HUNTER);
    AddBotToWorld(bot);
    Pet* pet = CreatePetForBot(bot, 100); // 100% health

    PetManager manager(bot, ai);
    manager.OnInitialize();
    manager.OnUpdate(1000);

    REQUIRE(manager.HasPet());
    REQUIRE(manager.GetPetHealthPercent() == 100);
    REQUIRE(!manager.PetNeedsAttention());
    REQUIRE(manager.m_hasWork.load() == false);
}

TEST_CASE("PetManager - Update with injured pet", "[petmanager]")
{
    auto [bot, ai] = CreateTestBot(CLASS_HUNTER);
    AddBotToWorld(bot);
    Pet* pet = CreatePetForBot(bot, 30); // 30% health

    PetManager manager(bot, ai);
    manager.OnInitialize();
    manager.OnUpdate(1000);

    REQUIRE(manager.HasPet());
    REQUIRE(manager.GetPetHealthPercent() == 30);
    REQUIRE(manager.PetNeedsAttention());
    REQUIRE(manager.m_hasWork.load() == true);
}
```

---

## Best Practices

### 1. Choose Appropriate Update Interval

```cpp
// HIGH FREQUENCY (200-500ms) - Use for:
// - Combat-adjacent systems (not direct combat, but related)
// - Time-sensitive state tracking
CombatSupportManager(bot, ai, 300, "CombatSupportManager")

// MEDIUM FREQUENCY (1000-2000ms) - Use for:
// - Active gameplay systems (quests, group, pets)
// - Systems that need timely response
QuestManager(bot, ai, 2000, "QuestManager")

// LOW FREQUENCY (5000-10000ms) - Use for:
// - Background maintenance (inventory, economy)
// - Infrequent status checks
InventoryManager(bot, ai, 5000, "InventoryManager")
```

### 2. Use Atomic Flags for State Queries

```cpp
// GOOD: Lock-free state query
class QuestManager : public BehaviorManager
{
public:
    bool HasActiveQuests() const
    {
        return m_hasActiveQuests.load(std::memory_order_acquire);
    }

protected:
    void OnUpdate(uint32 elapsed) override
    {
        // Update state
        bool hasQuests = !m_activeQuests.empty();
        m_hasActiveQuests.store(hasQuests, std::memory_order_release);
    }

private:
    std::atomic<bool> m_hasActiveQuests{false};
};

// BAD: Requires lock or exposes internal state
class QuestManager : public BehaviorManager
{
public:
    bool HasActiveQuests() const
    {
        std::lock_guard<std::mutex> lock(m_mutex); // Performance hit!
        return !m_activeQuests.empty();
    }
};
```

### 3. Handle Initialization Properly

```cpp
// GOOD: Defers initialization until ready
bool PetManager::OnInitialize()
{
    Player* bot = GetBot();
    if (!bot || !bot->IsInWorld())
        return false; // Retry later

    if (!IsPetClass(bot))
    {
        SetEnabled(false); // Disable permanently
        return true; // Don't retry
    }

    LoadPetData();
    return true;
}

// BAD: Forces initialization or crashes
bool PetManager::OnInitialize()
{
    Player* bot = GetBot();
    LoadPetData(); // Crashes if bot not ready!
    return true;
}
```

### 4. Update m_hasWork Flag

```cpp
// GOOD: Always updates work flag
void QuestManager::OnUpdate(uint32 elapsed)
{
    // Do work
    ProcessQuests();

    // Always update flag at end
    bool hasWork = !m_activeQuests.empty() || m_hasPendingRewards;
    m_hasWork.store(hasWork, std::memory_order_release);
}

// BAD: Forgets to update flag
void QuestManager::OnUpdate(uint32 elapsed)
{
    // Do work
    ProcessQuests();

    // Missing: m_hasWork update
    // Strategies won't know manager state has changed!
}
```

### 5. Use Early Returns

```cpp
// GOOD: Early returns for fast paths
void CombatManager::OnUpdate(uint32 elapsed)
{
    Player* bot = GetBot();
    if (!bot)
        return;

    if (!bot->IsInCombat())
    {
        m_inCombat.store(false, std::memory_order_release);
        m_hasWork.store(false, std::memory_order_release);
        return; // Skip expensive operations
    }

    // Expensive combat operations only when in combat
    UpdateThreatList();
    UpdateCooldowns();
}

// BAD: Performs expensive operations unnecessarily
void CombatManager::OnUpdate(uint32 elapsed)
{
    Player* bot = GetBot();

    UpdateThreatList(); // Expensive even when not in combat!
    UpdateCooldowns();  // Expensive even when not in combat!

    bool inCombat = bot && bot->IsInCombat();
    m_inCombat.store(inCombat, std::memory_order_release);
}
```

### 6. Log Appropriately

```cpp
// GOOD: Uses manager logging with appropriate levels
void QuestManager::OnUpdate(uint32 elapsed)
{
    // Periodic debug logging
    if (m_updateCount % 20 == 0)
    {
        LogDebug("Processing {} quests", m_activeQuests.size());
    }

    // Warning for issues
    if (m_failedQuestLoads > 10)
    {
        LogWarning("Failed to load quest data {} times", m_failedQuestLoads);
    }
}

// BAD: Excessive logging or uses wrong level
void QuestManager::OnUpdate(uint32 elapsed)
{
    // Logs every update (spam!)
    TC_LOG_INFO("module.playerbot", "Quest update");

    // Uses wrong log level (error for non-error)
    if (m_activeQuests.empty())
    {
        TC_LOG_ERROR("module.playerbot", "No active quests"); // Not an error!
    }
}
```

### 7. Handle External Events

```cpp
// GOOD: Forces immediate update for time-sensitive events
void GroupManager::OnGroupInvite(Player* inviter)
{
    m_pendingInviter = inviter->GetGUID();
    ForceUpdate(); // Respond immediately
}

void GroupManager::OnUpdate(uint32 elapsed)
{
    // Process pending invite
    if (!m_pendingInviter.IsEmpty())
    {
        ProcessInvite(m_pendingInviter);
        m_pendingInviter = ObjectGuid::Empty;
    }
}

// BAD: Waits for next throttled update
void GroupManager::OnGroupInvite(Player* inviter)
{
    m_pendingInviter = inviter->GetGUID();
    // Missing: ForceUpdate()
    // May wait several seconds before processing!
}
```

---

## Common Patterns

### Pattern 1: Periodic Expensive Operations

```cpp
void InventoryManager::OnUpdate(uint32 elapsed)
{
    // Expensive: Full inventory scan
    // Only do every 10+ seconds (accumulate elapsed time)
    if (elapsed >= 10000)
    {
        ScanEntireInventory(); // ~10ms
    }
    else
    {
        // Cheap: Quick check
        QuickInventoryCheck(); // ~1ms
    }

    // Expensive: Database query for item values
    // Only do every 20th update
    if (m_updateCount % 20 == 0)
    {
        UpdateItemValueCache(); // ~5ms
    }
}
```

### Pattern 2: State Machine Manager

```cpp
class TradeManager : public BehaviorManager
{
public:
    enum class State : uint32
    {
        IDLE = 0,
        EVALUATING = 1,
        NEGOTIATING = 2,
        ACCEPTING = 3
    };

    State GetState() const
    {
        return static_cast<State>(m_state.load(std::memory_order_acquire));
    }

protected:
    void OnUpdate(uint32 elapsed) override
    {
        State currentState = GetState();

        switch (currentState)
        {
            case State::EVALUATING:
                UpdateEvaluating();
                break;
            case State::NEGOTIATING:
                UpdateNegotiating();
                break;
            case State::ACCEPTING:
                UpdateAccepting();
                break;
            default:
                break;
        }

        // Update work flag
        m_hasWork.store(currentState != State::IDLE,
                        std::memory_order_release);
    }

private:
    std::atomic<uint32> m_state{static_cast<uint32>(State::IDLE)};
};
```

### Pattern 3: Event-Driven Manager

```cpp
class GroupManager : public BehaviorManager
{
public:
    // External event handlers
    void OnGroupInvite(ObjectGuid inviter)
    {
        std::lock_guard<std::mutex> lock(m_eventMutex);
        m_pendingEvents.push({EventType::INVITE, inviter});
        m_needsUpdate.store(true, std::memory_order_release);
    }

    void OnGroupKick()
    {
        std::lock_guard<std::mutex> lock(m_eventMutex);
        m_pendingEvents.push({EventType::KICK, ObjectGuid::Empty});
        m_needsUpdate.store(true, std::memory_order_release);
    }

protected:
    void OnUpdate(uint32 elapsed) override
    {
        // Process all pending events
        std::lock_guard<std::mutex> lock(m_eventMutex);
        while (!m_pendingEvents.empty())
        {
            Event event = m_pendingEvents.front();
            m_pendingEvents.pop();

            ProcessEvent(event);
        }
    }

private:
    enum class EventType { INVITE, KICK, LEAVE };
    struct Event { EventType type; ObjectGuid guid; };

    std::mutex m_eventMutex;
    std::queue<Event> m_pendingEvents;
};
```

### Pattern 4: Caching Manager

```cpp
class QuestManager : public BehaviorManager
{
protected:
    void OnUpdate(uint32 elapsed) override
    {
        // Rebuild cache every 10 seconds
        if (elapsed >= 10000)
        {
            RebuildQuestCache();
        }

        // Use cached data for fast queries
        UpdateQuestProgress(); // Uses cache
    }

    bool OnInitialize() override
    {
        // Build initial cache
        RebuildQuestCache();
        return true;
    }

private:
    void RebuildQuestCache()
    {
        m_questCache.clear();
        // Expensive: Query database and build cache
        LoadQuestsFromDB();
        BuildObjectiveCache();
    }

    std::unordered_map<uint32, QuestData> m_questCache;
};
```

---

## Common Pitfalls

### Pitfall 1: Forgetting to Check for Null

```cpp
// BAD: Crashes if bot is null
void QuestManager::OnUpdate(uint32 elapsed)
{
    Player* bot = GetBot();
    bot->GetQuestStatus(questId); // CRASH if bot is null!
}

// GOOD: Always check for null
void QuestManager::OnUpdate(uint32 elapsed)
{
    Player* bot = GetBot();
    if (!bot)
    {
        LogWarning("Bot pointer is null");
        return;
    }

    bot->GetQuestStatus(questId); // Safe
}
```

### Pitfall 2: Blocking Operations in OnUpdate

```cpp
// BAD: Blocks for extended time
void InventoryManager::OnUpdate(uint32 elapsed)
{
    // Synchronous database query - may take 50-100ms!
    QueryResult result = WorldDatabase.Query("SELECT * FROM expensive_table");

    // Blocks the entire update thread!
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// GOOD: Uses async queries or caching
void InventoryManager::OnUpdate(uint32 elapsed)
{
    // Use cached data
    ProcessCachedInventory();

    // Queue async database update
    if (m_needsDatabaseUpdate)
    {
        QueueAsyncDatabaseUpdate();
        m_needsDatabaseUpdate = false;
    }
}
```

### Pitfall 3: Not Updating State Flags

```cpp
// BAD: Strategies can't detect state changes
void QuestManager::OnUpdate(uint32 elapsed)
{
    ProcessQuests();
    // Missing: Update m_hasWork flag
}

// In strategy:
bool QuestStrategy::IsRelevant()
{
    // Always returns old state!
    return m_ai->GetQuestManager()->m_hasWork.load();
}

// GOOD: Always updates flags
void QuestManager::OnUpdate(uint32 elapsed)
{
    ProcessQuests();

    // Update flags at end of update
    m_hasWork.store(!m_activeQuests.empty(), std::memory_order_release);
}
```

### Pitfall 4: Using Wrong Update Interval

```cpp
// BAD: High-frequency updates for background task
InventoryManager(bot, ai, 100, "InventoryManager") // 100ms!
// Result: Wasted CPU on frequent inventory scans

// GOOD: Appropriate interval for task frequency
InventoryManager(bot, ai, 5000, "InventoryManager") // 5 seconds
// Result: Adequate responsiveness with minimal CPU usage

// BAD: Low-frequency updates for time-sensitive task
CombatSupportManager(bot, ai, 10000, "CombatSupportManager") // 10 seconds!
// Result: Delayed reactions in combat

// GOOD: Appropriate interval for responsiveness
CombatSupportManager(bot, ai, 300, "CombatSupportManager") // 300ms
// Result: Responsive without excessive CPU usage
```

### Pitfall 5: Throwing Exceptions

```cpp
// BAD: Throws exception (disables manager)
void QuestManager::OnUpdate(uint32 elapsed)
{
    Quest const* quest = GetQuest(questId);
    if (!quest)
        throw std::runtime_error("Quest not found"); // Manager disabled!
}

// GOOD: Handles errors gracefully
void QuestManager::OnUpdate(uint32 elapsed)
{
    Quest const* quest = GetQuest(questId);
    if (!quest)
    {
        LogWarning("Quest {} not found, skipping", questId);
        return; // Continue processing next update
    }
}
```

### Pitfall 6: Race Conditions with Shared State

```cpp
// BAD: Modifies shared state without protection
void QuestManager::OnQuestAccepted(uint32 questId)
{
    // Called from event thread
    m_activeQuests.insert(questId); // RACE CONDITION with OnUpdate!
}

void QuestManager::OnUpdate(uint32 elapsed)
{
    // Called from main thread
    for (uint32 questId : m_activeQuests) // RACE CONDITION with OnQuestAccepted!
    {
        ProcessQuest(questId);
    }
}

// GOOD: Uses mutex for shared state
void QuestManager::OnQuestAccepted(uint32 questId)
{
    std::lock_guard<std::mutex> lock(m_questMutex);
    m_activeQuests.insert(questId);
    m_needsUpdate.store(true, std::memory_order_release);
}

void QuestManager::OnUpdate(uint32 elapsed)
{
    // Copy active quests under lock
    std::vector<uint32> questsToProcess;
    {
        std::lock_guard<std::mutex> lock(m_questMutex);
        questsToProcess.assign(m_activeQuests.begin(), m_activeQuests.end());
    }

    // Process without holding lock
    for (uint32 questId : questsToProcess)
    {
        ProcessQuest(questId);
    }
}
```

---

## Performance Tuning

### Measuring Performance

Use the built-in performance monitoring:

```cpp
void QuestManager::OnUpdate(uint32 elapsed)
{
    // Performance automatically monitored by base class
    ProcessQuests();

    // Log performance periodically
    if (m_updateCount % 50 == 0)
    {
        LogDebug("Average update: {}ms, Total updates: {}",
                 m_averageUpdateTime, m_updateCount.load());
    }
}
```

### Optimization Techniques

#### 1. Time Budgeting

```cpp
void QuestManager::OnUpdate(uint32 elapsed)
{
    uint32 startTime = getMSTime();
    const uint32 timeBudget = 8; // 8ms budget

    for (auto const& [questId, data] : m_activeQuests)
    {
        // Check time budget
        uint32 elapsedTime = getMSTimeDiff(startTime, getMSTime());
        if (elapsedTime > timeBudget)
        {
            LogDebug("Time budget exceeded, deferring {} quests",
                     m_activeQuests.size() - processed);
            break;
        }

        ProcessQuest(questId);
    }
}
```

#### 2. Work Splitting

```cpp
void InventoryManager::OnUpdate(uint32 elapsed)
{
    const uint32 itemsPerUpdate = 10;

    // Process items in batches
    uint32 startIndex = m_lastProcessedIndex;
    uint32 endIndex = std::min(startIndex + itemsPerUpdate, m_items.size());

    for (uint32 i = startIndex; i < endIndex; ++i)
    {
        ProcessItem(m_items[i]);
    }

    m_lastProcessedIndex = (endIndex >= m_items.size()) ? 0 : endIndex;
}
```

#### 3. Adaptive Intervals

```cpp
void QuestManager::OnUpdate(uint32 elapsed)
{
    ProcessQuests();

    // Adjust interval based on workload
    uint32 questCount = m_activeQuests.size();

    if (questCount > 10)
    {
        // Many quests - update more frequently
        SetUpdateInterval(1000);
    }
    else if (questCount > 5)
    {
        // Medium quests - normal frequency
        SetUpdateInterval(2000);
    }
    else
    {
        // Few quests - update less frequently
        SetUpdateInterval(5000);
    }
}
```

#### 4. Caching

```cpp
void QuestManager::OnUpdate(uint32 elapsed)
{
    // Rebuild cache only when needed
    if (m_cacheInvalid || elapsed >= 30000)
    {
        RebuildQuestCache();
        m_cacheInvalid = false;
    }

    // Use cached data for processing
    for (auto const& [questId, cachedData] : m_questCache)
    {
        ProcessCachedQuest(cachedData);
    }
}
```

### Performance Targets

| Manager Type | Update Interval | Update Cost | Max Bots |
|--------------|-----------------|-------------|----------|
| Combat Support | 200-500ms | <5ms | 2000 |
| Active Gameplay | 1000-2000ms | <10ms | 5000 |
| Background | 5000-10000ms | <20ms | 10000 |

---

## Debugging Techniques

### 1. Enable Debug Logging

```cpp
// In playerbots.conf
Log.Level.Module.Playerbot = 5  # Enable DEBUG logging

// Or programmatically
void QuestManager::OnUpdate(uint32 elapsed)
{
    if (isDebugEnabled)
    {
        LogDebug("Update: {} active quests, {}ms since last",
                 m_activeQuests.size(), elapsed);
    }
}
```

### 2. Performance Profiling

```cpp
void QuestManager::OnUpdate(uint32 elapsed)
{
    uint32 t0 = getMSTime();

    LoadQuestData();
    uint32 t1 = getMSTime();

    ProcessQuests();
    uint32 t2 = getMSTime();

    UpdateCache();
    uint32 t3 = getMSTime();

    LogDebug("Timing: Load={}ms, Process={}ms, Cache={}ms",
             getMSTimeDiff(t0, t1),
             getMSTimeDiff(t1, t2),
             getMSTimeDiff(t2, t3));
}
```

### 3. State Inspection Commands

```cpp
// Create a debug command to inspect manager state
void CommandHandler::HandleBotManagerDebug(Player* player, std::string managerName)
{
    BotAI* ai = GetBotAI(player);
    if (!ai)
        return;

    if (managerName == "quest")
    {
        QuestManager* qm = ai->GetQuestManager();
        if (!qm)
        {
            SendMessage("Quest manager not found");
            return;
        }

        SendMessage("Quest Manager Status:");
        SendMessage("  Enabled: {}", qm->IsEnabled());
        SendMessage("  Initialized: {}", qm->IsInitialized());
        SendMessage("  Busy: {}", qm->IsBusy());
        SendMessage("  Has Work: {}", qm->m_hasWork.load());
        SendMessage("  Update Count: {}", qm->m_updateCount.load());
        SendMessage("  Interval: {}ms", qm->GetUpdateInterval());
        SendMessage("  Time Since Last: {}ms", qm->GetTimeSinceLastUpdate());
    }
}
```

### 4. Crash Debugging

If manager crashes:

```cpp
// Add validation checks
void QuestManager::OnUpdate(uint32 elapsed)
{
    // Validate state before processing
    if (!ValidateInternalState())
    {
        LogWarning("Internal state invalid, skipping update");
        return;
    }

    // Wrap risky operations
    try
    {
        ProcessQuests();
    }
    catch (const std::exception& e)
    {
        LogWarning("Exception in ProcessQuests: {}", e.what());
        // Manager will be auto-disabled by base class
    }
}

bool QuestManager::ValidateInternalState()
{
    if (!GetBot())
    {
        LogWarning("Bot pointer is null");
        return false;
    }

    if (m_activeQuests.size() > 25) // WoW quest log limit
    {
        LogWarning("Too many active quests: {}", m_activeQuests.size());
        return false;
    }

    return true;
}
```

---

## Integration with BotAI

### Registration Pattern

```cpp
// In BotAI constructor
BotAI::BotAI(Player* bot)
    : _bot(bot)
{
    // Create managers based on bot class/spec
    _questManager = std::make_unique<QuestManager>(bot, this);

    uint8 botClass = bot->getClass();
    if (botClass == CLASS_HUNTER || botClass == CLASS_WARLOCK)
    {
        _petManager = std::make_unique<PetManager>(bot, this);
    }

    _inventoryManager = std::make_unique<InventoryManager>(bot, this);
    _groupManager = std::make_unique<GroupManager>(bot, this);
}

// In BotAI update
void BotAI::UpdateAI(uint32 diff)
{
    // Update all managers
    if (_questManager)
        _questManager->Update(diff);

    if (_petManager)
        _petManager->Update(diff);

    if (_inventoryManager)
        _inventoryManager->Update(diff);

    if (_groupManager)
        _groupManager->Update(diff);

    // ... other update logic
}
```

### Event Forwarding Pattern

```cpp
// In BotAI
void BotAI::OnQuestAccepted(Quest const* quest)
{
    // Forward event to quest manager
    if (_questManager)
        _questManager->OnQuestAccepted(quest);
}

void BotAI::OnGroupInvite(Player* inviter)
{
    // Forward event to group manager
    if (_groupManager)
        _groupManager->OnGroupInvite(inviter->GetGUID());
}

void BotAI::OnCombatStart(Unit* target)
{
    // Disable non-combat managers during combat
    if (_questManager)
        _questManager->SetEnabled(false);

    if (_inventoryManager)
        _inventoryManager->SetEnabled(false);
}

void BotAI::OnCombatEnd()
{
    // Re-enable managers after combat
    if (_questManager)
        _questManager->SetEnabled(true);

    if (_inventoryManager)
        _inventoryManager->SetEnabled(true);
}
```

---

## Testing Your Manager

### Unit Test Structure

```cpp
// PetManagerTest.cpp
#include "catch2/catch.hpp"
#include "PetManager.h"
#include "TestHelpers.h"

using namespace Playerbot;

TEST_CASE("PetManager - Basic functionality", "[petmanager]")
{
    SECTION("Construction")
    {
        auto [bot, ai] = CreateTestBot(CLASS_HUNTER);
        PetManager manager(bot, ai);

        REQUIRE(manager.IsEnabled());
        REQUIRE(!manager.IsInitialized());
    }

    SECTION("Initialization - Not in world")
    {
        auto [bot, ai] = CreateTestBot(CLASS_HUNTER);
        PetManager manager(bot, ai);

        REQUIRE(!manager.OnInitialize());
    }

    SECTION("Initialization - In world")
    {
        auto [bot, ai] = CreateTestBot(CLASS_HUNTER);
        AddBotToWorld(bot);

        PetManager manager(bot, ai);
        REQUIRE(manager.OnInitialize());
        REQUIRE(manager.IsInitialized());
    }

    SECTION("Wrong class disables manager")
    {
        auto [bot, ai] = CreateTestBot(CLASS_WARRIOR); // Not a pet class
        AddBotToWorld(bot);

        PetManager manager(bot, ai);
        manager.OnInitialize();

        REQUIRE(!manager.IsEnabled());
    }
}

TEST_CASE("PetManager - Pet status tracking", "[petmanager]")
{
    auto [bot, ai] = CreateTestBot(CLASS_HUNTER);
    AddBotToWorld(bot);

    PetManager manager(bot, ai);
    manager.OnInitialize();

    SECTION("No pet")
    {
        manager.OnUpdate(1000);

        REQUIRE(!manager.HasPet());
        REQUIRE(manager.GetPetHealthPercent() == 0);
        REQUIRE(manager.PetNeedsAttention());
    }

    SECTION("Healthy pet")
    {
        CreatePetForBot(bot, 100);
        manager.OnUpdate(1000);

        REQUIRE(manager.HasPet());
        REQUIRE(manager.GetPetHealthPercent() == 100);
        REQUIRE(!manager.PetNeedsAttention());
    }

    SECTION("Injured pet")
    {
        CreatePetForBot(bot, 30);
        manager.OnUpdate(1000);

        REQUIRE(manager.HasPet());
        REQUIRE(manager.GetPetHealthPercent() == 30);
        REQUIRE(manager.PetNeedsAttention());
    }
}

TEST_CASE("PetManager - Event handling", "[petmanager]")
{
    auto [bot, ai] = CreateTestBot(CLASS_HUNTER);
    AddBotToWorld(bot);
    Pet* pet = CreatePetForBot(bot, 100);

    PetManager manager(bot, ai);
    manager.OnInitialize();

    SECTION("Pet death")
    {
        manager.OnUpdate(1000);
        REQUIRE(manager.HasPet());

        KillPet(pet);
        manager.OnPetDied();
        manager.OnUpdate(1000);

        REQUIRE(!manager.HasPet());
    }

    SECTION("Pet summon")
    {
        manager.OnUpdate(1000);

        Pet* newPet = CreatePetForBot(bot, 100);
        manager.OnPetSummoned();
        manager.OnUpdate(1000);

        REQUIRE(manager.HasPet());
    }
}
```

### Integration Testing

```cpp
// Test with full bot setup
TEST_CASE("PetManager - Integration with BotAI", "[petmanager][integration]")
{
    // Create full bot with AI
    auto [bot, ai] = CreateFullBot(CLASS_HUNTER);
    AddBotToWorld(bot);

    // Bot should have pet manager
    REQUIRE(ai->GetPetManager() != nullptr);

    PetManager* pm = ai->GetPetManager();

    // Initialize bot
    ai->Reset();

    // Update AI (which updates manager)
    ai->UpdateAI(100);

    // Manager should be initialized
    REQUIRE(pm->IsInitialized());

    // Create pet
    Pet* pet = CreatePetForBot(bot, 50);

    // Update AI
    ai->UpdateAI(2100); // Exceeds manager update interval

    // Manager should detect pet
    REQUIRE(pm->HasPet());
    REQUIRE(pm->GetPetHealthPercent() == 50);
    REQUIRE(pm->PetNeedsAttention());
}
```

---

## Summary

You now have a complete understanding of how to create, implement, and integrate a `BehaviorManager`-based system. Remember:

1. **Choose appropriate update intervals** based on task requirements
2. **Use atomic flags** for lock-free state queries
3. **Handle initialization properly** with retries for async dependencies
4. **Update m_hasWork** to enable strategy activation
5. **Use early returns** for performance optimization
6. **Log appropriately** with manager prefixes
7. **Test thoroughly** with unit and integration tests

For API reference details, see [BEHAVIORMANAGER_API.md](BEHAVIORMANAGER_API.md).
For architecture deep-dive, see [BEHAVIORMANAGER_ARCHITECTURE.md](BEHAVIORMANAGER_ARCHITECTURE.md).

---

**Document Version**: 1.0
**Last Updated**: Phase 2.1 Completion
**Related Documents**:
- [BehaviorManager API Reference](BEHAVIORMANAGER_API.md)
- [BehaviorManager Architecture](BEHAVIORMANAGER_ARCHITECTURE.md)
