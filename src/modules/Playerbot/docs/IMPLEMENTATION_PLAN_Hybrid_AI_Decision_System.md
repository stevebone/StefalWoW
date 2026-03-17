# Implementation Plan: Hybrid AI Decision System & Group Coordination

**Document Version:** 1.0
**Created:** 2025-11-09
**Architecture:** Utility AI + Behavior Trees + Hierarchical Multi-Agent Coordination
**Target Scale:** 5,000 concurrent bots
**Quality Standard:** Enterprise-grade, future-proof

---

## Table of Contents

1. [Executive Summary](#executive-summary)
2. [Research Findings: Modern AI Decision Systems](#research-findings-modern-ai-decision-systems)
3. [Architectural Design](#architectural-design)
4. [Phase 1: Utility AI Decision Layer](#phase-1-utility-ai-decision-layer)
5. [Phase 2: Behavior Tree Execution Layer](#phase-2-behavior-tree-execution-layer)
6. [Phase 3: Hierarchical Group Coordination](#phase-3-hierarchical-group-coordination)
7. [Phase 4: Blackboard Shared State System](#phase-4-blackboard-shared-state-system)
8. [Phase 5: ClassAI Integration](#phase-5-classai-integration)
9. [Performance & Scalability](#performance--scalability)
10. [Implementation Timeline](#implementation-timeline)

---

## Executive Summary

### Current Problems

**Static Decision System:**
- Hardcoded priority values (COMBAT=100, FOLLOW=50)
- Cannot adapt to dynamic situations
- No learning or context awareness
- Linear action selection

**No Group Synchronization:**
- Each bot decides independently
- Tanks and DPS can conflict
- No coordinated role-based behavior
- Reactive rather than proactive

**Scale Limitations:**
- Current system not designed for 5,000 bots
- No hierarchical coordination
- Potential bottlenecks in decision-making

### Solution Architecture

**Hybrid System: Utility AI + Behavior Trees**
- **Utility AI Layer**: Dynamic scoring of behaviors based on context
- **Behavior Tree Layer**: Structured execution with composability
- **Result**: Adaptive yet predictable, scalable and maintainable

**Hierarchical Multi-Agent Coordination**
- **Group Orchestrators**: Coordinate 5-40 bots per group
- **Role-Based Agents**: Tank, Healer, DPS specialized decision-making
- **Blackboard Pattern**: Shared state for synchronized decisions
- **Result**: Coordinated group behavior with all roles considered

### Success Metrics

| Metric | Target | Impact |
|--------|--------|--------|
| Decision Latency (per bot) | <2ms | Real-time responsiveness |
| Group Sync Overhead | <5ms per group | Minimal coordination cost |
| Scalability | 5,000 concurrent bots | Enterprise-grade capacity |
| Memory per bot | <500KB | Efficient resource usage |
| Adaptability Score | 85%+ | Context-aware decisions |
| Code Maintainability | +200% vs current | Future-proof architecture |

---

## Research Findings: Modern AI Decision Systems

### 1. Behavior Trees vs GOAP vs HTN vs Utility AI (2024-2025)

**Industry Consensus (2024-2025):**
- **Behavior Trees (BT)**: Best for predictable, tactical AI. Computationally cheap (no search). Works well for 80% of game AI needs.
- **GOAP**: Good for adaptive combat but computationally expensive due to constant re-planning. Scalability issues with 5,000 agents.
- **HTN**: Better than GOAP for forward planning, used successfully in Killzone series. Requires designer-built networks.
- **Utility AI**: Always picks highest-scoring action. Excellent for adaptive behavior without planning overhead.

**Best Practice (2024):**
> "Planning isn't about finding the 'one true system' but choosing the right tool for your design goals, with **hybrid approaches** becoming increasingly popular in 2024."
> — Game AI Pro, 2024

**Recommended Hybrid: Utility AI + Behavior Trees**
- Utility AI scores and selects high-level behaviors (WHAT to do)
- Behavior Trees execute the selected behavior (HOW to do it)
- Combines adaptability with structure and performance

### 2. Multi-Agent Coordination Systems

**Scalability Patterns:**
- **Orchestrator-Worker Pattern**: Lead agent coordinates while delegating to specialized subagents
- **Hierarchical Agent Pattern**: Reduces coordination complexity from exponential to linear
  - Example: 10 managers × 100 agents each = 1,000 agents coordinated efficiently
  - For 5,000 bots: ~100 group orchestrators × ~50 bots each
- **Blackboard Pattern**: Shared workspace where agents read/write state asynchronously

**Key Finding:**
> "Multi-agent systems scale naturally by adding agents rather than upgrading centralized components. Hierarchical pattern reduces coordination complexity from exponential to linear."
> — Microsoft Azure AI Architecture, 2024

### 3. Blackboard Pattern for Shared State

**Core Components:**
1. **Blackboard**: Shared data structure (player position, group target, threat table)
2. **Agents**: Independent components that read/write to blackboard
3. **Control Component**: Manages interactions and determines which agents activate

**Benefits for 5,000 Bots:**
- Separates AI code from game world (modular)
- Avoids repeated calculations (performance)
- Enables asynchronous collaboration (scalability)
- Reduces interdependencies (maintainability)

### 4. Industry Implementations

**Unreal Engine 5 (2024):**
- Behavior Trees with AI Perception
- MetaHuman AI for realistic NPCs
- Machine learning integration for adaptive gameplay

**Unity 2025:**
- ML-Agents Toolkit for reinforcement learning
- Unity Sentis for integrated ML models
- AI-powered animation systems

**AgentOrchestra (Research, 2024):**
- Central Planning Agent decomposes complex tasks
- Team of specialized agents handle subtasks
- Hierarchical coordination with adaptive planning

---

## Architectural Design

### System Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────────┐
│                         WORLD STATE                                  │
│  (TrinityCore Game World + Spatial Grids + Events)                  │
└────────────────────────────┬────────────────────────────────────────┘
                             │
                             ▼
┌─────────────────────────────────────────────────────────────────────┐
│                    BLACKBOARD LAYER                                  │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐             │
│  │ Group Board  │  │ Combat Board │  │ Quest Board  │  ...        │
│  │ (per group)  │  │ (per encounter)│ │ (shared)    │             │
│  └──────────────┘  └──────────────┘  └──────────────┘             │
└────────────────────────────┬────────────────────────────────────────┘
                             │
                             ▼
┌─────────────────────────────────────────────────────────────────────┐
│              HIERARCHICAL COORDINATION LAYER                         │
│                                                                      │
│  ┌────────────────────────────────────────────────────────────┐   │
│  │              Raid Orchestrator (40 bots)                    │   │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐    │   │
│  │  │Tank Squad (5)│  │Healer Squad  │  │DPS Squad (30)│    │   │
│  │  │ Orchestrator │  │ Orchestrator │  │ Orchestrator │    │   │
│  │  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘    │   │
│  └─────────┼──────────────────┼──────────────────┼────────────┘   │
│            │                  │                  │                 │
│  ┌─────────▼────────┐  ┌──────▼───────┐  ┌──────▼───────┐        │
│  │ Group Orch. (5)  │  │ Group Orch.  │  │ Group Orch.  │  ...   │
│  │ (Tank Squad)     │  │ (Healers)    │  │ (DPS Squad)  │        │
│  └─────────┬────────┘  └──────┬───────┘  └──────┬───────┘        │
└────────────┼────────────────────┼──────────────────┼───────────────┘
             │                    │                  │
             ▼                    ▼                  ▼
┌─────────────────────────────────────────────────────────────────────┐
│                     BOT AI DECISION LAYER                            │
│                                                                      │
│  ┌────────────────────────────────────────────────────────────┐   │
│  │                    Per-Bot Decision System                  │   │
│  │                                                              │   │
│  │  ┌─────────────────────────────────────────────────────┐  │   │
│  │  │         UTILITY AI LAYER (Decision Maker)           │  │   │
│  │  │                                                      │  │   │
│  │  │  ┌──────────┐ ┌──────────┐ ┌──────────┐           │  │   │
│  │  │  │ Combat   │ │ Healing  │ │ Movement │  ...      │  │   │
│  │  │  │Utility   │ │ Utility  │ │ Utility  │           │  │   │
│  │  │  │Evaluator │ │Evaluator │ │Evaluator │           │  │   │
│  │  │  └────┬─────┘ └────┬─────┘ └────┬─────┘           │  │   │
│  │  │       │            │            │                  │  │   │
│  │  │       └────────────┼────────────┘                  │  │   │
│  │  │                    ▼                               │  │   │
│  │  │           ┌──────────────────┐                    │  │   │
│  │  │           │ Utility Selector │                    │  │   │
│  │  │           │ (picks highest)  │                    │  │   │
│  │  │           └────────┬─────────┘                    │  │   │
│  │  └──────────────────────┼──────────────────────────────┘  │   │
│  │                         │                                  │   │
│  │  ┌──────────────────────▼──────────────────────────────┐  │   │
│  │  │      BEHAVIOR TREE LAYER (Executor)                 │  │   │
│  │  │                                                      │  │   │
│  │  │       Selector                                      │  │   │
│  │  │       ├── Sequence: Combat Behavior                │  │   │
│  │  │       │   ├── CheckInRange                         │  │   │
│  │  │       │   ├── FaceTarget                           │  │   │
│  │  │       │   └── ExecuteRotation                      │  │   │
│  │  │       ├── Sequence: Healing Behavior               │  │   │
│  │  │       │   ├── FindWoundedAlly                      │  │   │
│  │  │       │   ├── CheckLoS                             │  │   │
│  │  │       │   └── CastHeal                             │  │   │
│  │  │       └── Sequence: Movement Behavior              │  │   │
│  │  │           ├── CalculatePath                        │  │   │
│  │  │           ├── CheckObstacles                       │  │   │
│  │  │           └── Move                                 │  │   │
│  │  └──────────────────────────────────────────────────────┘  │   │
│  └─────────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────────┘
                             │
                             ▼
┌─────────────────────────────────────────────────────────────────────┐
│                      ACTION EXECUTION                                │
│              (TrinityCore Spell/Movement Systems)                    │
└─────────────────────────────────────────────────────────────────────┘
```

### Key Design Principles

**1. Separation of Concerns**
- **Utility AI**: Scores and selects WHAT behavior to execute
- **Behavior Tree**: Defines HOW to execute the selected behavior
- **Blackboard**: WHERE shared state is stored
- **Orchestrator**: WHO coordinates group decisions

**2. Hierarchical Coordination**
- **Level 1**: Individual bots make local decisions using Utility AI + BT
- **Level 2**: Group Orchestrators (5-40 bots) coordinate roles and tactics
- **Level 3**: Raid Orchestrators (up to 40 bots) manage squad coordination
- **Level 4**: Zone Orchestrators (100-500 bots) handle world-scale coordination

**3. Data Flow**
1. World State → Blackboard (centralized shared state)
2. Blackboard → Utility Evaluators (context-aware scoring)
3. Utility Selector → Behavior Tree (select highest-scoring behavior)
4. Behavior Tree → Actions (execute structured behavior)
5. Actions → World State (modify game world)
6. World State → Blackboard (update shared state)

**4. Scalability Strategy**
- **Lazy Evaluation**: Only evaluate utilities when decision needed
- **Spatial Partitioning**: Blackboards scoped to groups/zones
- **Update Throttling**: Stagger bot updates across frames
- **Hierarchical Aggregation**: Orchestrators reduce coordination complexity

---

## Phase 1: Utility AI Decision Layer

**Estimated Time:** 2-3 weeks
**Complexity:** HIGH
**Impact:** Foundation for all adaptive behavior

### 1.1: Core Utility System (Week 1)

#### Architecture

**File:** `AI/Utility/UtilitySystem.h`

```cpp
#pragma once

#include "Define.h"
#include <vector>
#include <memory>
#include <string>
#include <functional>

namespace Playerbot
{

class BotAI;

/**
 * @brief Context for utility evaluation
 * Contains all relevant state for decision-making
 */
struct UtilityContext
{
    BotAI* bot;
    class Blackboard* blackboard;

    // Bot state
    float healthPercent;
    float manaPercent;
    bool inCombat;
    bool hasAggro;

    // Group state
    bool inGroup;
    uint32 groupSize;
    float lowestAllyHealthPercent;
    uint32 enemiesInRange;

    // Role state
    enum class Role { TANK, HEALER, DPS, SUPPORT } role;

    // Timing
    uint32 timeSinceCombatStart;
    uint32 lastDecisionTime;
};

/**
 * @brief Base class for utility evaluators
 * Scores a specific behavior based on context
 */
class TC_GAME_API UtilityEvaluator
{
public:
    UtilityEvaluator(std::string const& name, float weight = 1.0f)
        : _name(name), _weight(weight) {}

    virtual ~UtilityEvaluator() = default;

    /**
     * @brief Calculate utility score for this behavior
     * @param context Current bot/world state
     * @return Score between 0.0 (useless) and 1.0 (critical)
     */
    virtual float Evaluate(UtilityContext const& context) const = 0;

    /**
     * @brief Get the name of this evaluator
     */
    std::string const& GetName() const { return _name; }

    /**
     * @brief Get the weight multiplier
     */
    float GetWeight() const { return _weight; }

    /**
     * @brief Set the weight multiplier
     */
    void SetWeight(float weight) { _weight = weight; }

    /**
     * @brief Calculate weighted score
     */
    float GetWeightedScore(UtilityContext const& context) const
    {
        return Evaluate(context) * _weight;
    }

protected:
    std::string _name;
    float _weight;

    // Utility curve functions
    static float Linear(float x) { return x; }
    static float Quadratic(float x) { return x * x; }
    static float Cubic(float x) { return x * x * x; }
    static float InverseLinear(float x) { return 1.0f - x; }
    static float Logistic(float x, float steepness = 10.0f)
    {
        return 1.0f / (1.0f + std::exp(-steepness * (x - 0.5f)));
    }
    static float Clamp(float value, float min = 0.0f, float max = 1.0f)
    {
        if (value < min) return min;
        if (value > max) return max;
        return value;
    }
};

/**
 * @brief Utility-based behavior
 * Combines multiple evaluators to score a behavior
 */
class TC_GAME_API UtilityBehavior
{
public:
    UtilityBehavior(std::string const& name)
        : _name(name), _cachedScore(0.0f), _lastEvalTime(0) {}

    /**
     * @brief Add an evaluator to this behavior
     */
    void AddEvaluator(std::shared_ptr<UtilityEvaluator> evaluator)
    {
        _evaluators.push_back(evaluator);
    }

    /**
     * @brief Calculate total utility score
     * Combines all evaluator scores
     */
    float CalculateUtility(UtilityContext const& context)
    {
        if (_evaluators.empty())
            return 0.0f;

        // Multiplicative combination (all factors must be favorable)
        float score = 1.0f;
        for (auto const& evaluator : _evaluators)
        {
            score *= evaluator->GetWeightedScore(context);
        }

        _cachedScore = score;
        _lastEvalTime = getMSTime();

        return score;
    }

    /**
     * @brief Get the behavior name
     */
    std::string const& GetName() const { return _name; }

    /**
     * @brief Get cached score (no recalculation)
     */
    float GetCachedScore() const { return _cachedScore; }

    /**
     * @brief Get time since last evaluation
     */
    uint32 GetTimeSinceEval() const { return getMSTime() - _lastEvalTime; }

private:
    std::string _name;
    std::vector<std::shared_ptr<UtilityEvaluator>> _evaluators;
    float _cachedScore;
    uint32 _lastEvalTime;
};

/**
 * @brief Utility-based decision maker
 * Selects highest-scoring behavior
 */
class TC_GAME_API UtilityAI
{
public:
    UtilityAI() = default;

    /**
     * @brief Register a behavior
     */
    void AddBehavior(std::shared_ptr<UtilityBehavior> behavior)
    {
        _behaviors.push_back(behavior);
    }

    /**
     * @brief Select the best behavior based on current context
     * @return Pointer to highest-scoring behavior, or nullptr
     */
    UtilityBehavior* SelectBehavior(UtilityContext const& context)
    {
        if (_behaviors.empty())
            return nullptr;

        float bestScore = -1.0f;
        UtilityBehavior* bestBehavior = nullptr;

        for (auto& behavior : _behaviors)
        {
            float score = behavior->CalculateUtility(context);

            if (score > bestScore)
            {
                bestScore = score;
                bestBehavior = behavior.get();
            }
        }

        return bestBehavior;
    }

    /**
     * @brief Get all behaviors sorted by score (descending)
     */
    std::vector<std::pair<UtilityBehavior*, float>> GetRankedBehaviors(UtilityContext const& context)
    {
        std::vector<std::pair<UtilityBehavior*, float>> ranked;

        for (auto& behavior : _behaviors)
        {
            float score = behavior->CalculateUtility(context);
            ranked.emplace_back(behavior.get(), score);
        }

        std::sort(ranked.begin(), ranked.end(),
            [](auto const& a, auto const& b) { return a.second > b.second; });

        return ranked;
    }

private:
    std::vector<std::shared_ptr<UtilityBehavior>> _behaviors;
};

} // namespace Playerbot
```

#### Example Utility Evaluators

**File:** `AI/Utility/Evaluators/CombatEvaluators.h`

```cpp
#pragma once

#include "AI/Utility/UtilitySystem.h"

namespace Playerbot
{

/**
 * @brief Evaluates need to engage in combat
 * High score when enemies nearby and bot is healthy
 */
class CombatEngageEvaluator : public UtilityEvaluator
{
public:
    CombatEngageEvaluator() : UtilityEvaluator("CombatEngage", 1.0f) {}

    float Evaluate(UtilityContext const& context) const override
    {
        // No enemies = no combat
        if (context.enemiesInRange == 0)
            return 0.0f;

        // Already in combat = maintain
        if (context.inCombat)
            return 0.8f;

        // Health-based scoring
        float healthFactor = Logistic(context.healthPercent, 15.0f);

        // Enemy count factor (more enemies = higher priority)
        float enemyFactor = Clamp(context.enemiesInRange / 5.0f, 0.0f, 1.0f);

        return healthFactor * enemyFactor;
    }
};

/**
 * @brief Evaluates need to heal allies
 * High score when allies are wounded
 */
class HealAllyEvaluator : public UtilityEvaluator
{
public:
    HealAllyEvaluator() : UtilityEvaluator("HealAlly", 1.2f) {}

    float Evaluate(UtilityContext const& context) const override
    {
        // Not a healer = can't heal
        if (context.role != UtilityContext::Role::HEALER)
            return 0.0f;

        // No mana = can't heal
        if (context.manaPercent < 0.1f)
            return 0.0f;

        // Inverse of lowest ally health (lower health = higher priority)
        float urgency = InverseLinear(context.lowestAllyHealthPercent);

        // Mana availability factor
        float manaFactor = Quadratic(context.manaPercent);

        return urgency * manaFactor;
    }
};

/**
 * @brief Evaluates need to maintain threat (tanks)
 * High score when tank has no aggro or enemies attacking healers
 */
class TankThreatEvaluator : public UtilityEvaluator
{
public:
    TankThreatEvaluator() : UtilityEvaluator("TankThreat", 1.5f) {}

    float Evaluate(UtilityContext const& context) const override
    {
        // Not a tank = not responsible for threat
        if (context.role != UtilityContext::Role::TANK)
            return 0.0f;

        // No enemies = no threat needed
        if (context.enemiesInRange == 0)
            return 0.0f;

        // Tank doesn't have aggro = CRITICAL
        if (!context.hasAggro)
            return 1.0f;

        // Tank has aggro = maintain moderate priority
        return 0.6f;
    }
};

/**
 * @brief Evaluates need to use defensive cooldowns
 * High score when health is low and in combat
 */
class DefensiveCooldownEvaluator : public UtilityEvaluator
{
public:
    DefensiveCooldownEvaluator() : UtilityEvaluator("DefensiveCooldown", 2.0f) {}

    float Evaluate(UtilityContext const& context) const override
    {
        // Not in combat = no need
        if (!context.inCombat)
            return 0.0f;

        // Inverse health with steep curve (panic at low health)
        float healthUrgency = Cubic(InverseLinear(context.healthPercent));

        // Time in combat factor (longer combat = more likely to use)
        float combatTimeFactor = Clamp(context.timeSinceCombatStart / 30000.0f, 0.0f, 1.0f);

        return healthUrgency * (0.7f + 0.3f * combatTimeFactor);
    }
};

} // namespace Playerbot
```

### 1.2: Utility Context Builder (Week 1)

**File:** `AI/Utility/UtilityContextBuilder.h`

```cpp
#pragma once

#include "UtilitySystem.h"
#include "AI/BotAI.h"
#include "Player.h"
#include "Group.h"

namespace Playerbot
{

/**
 * @brief Builds utility context from game world state
 */
class TC_GAME_API UtilityContextBuilder
{
public:
    static UtilityContext Build(BotAI* ai, Blackboard* blackboard)
    {
        UtilityContext context;
        context.bot = ai;
        context.blackboard = blackboard;

        Player* bot = ai->GetBot();

        // Bot state
        context.healthPercent = bot->GetHealthPct() / 100.0f;
        context.manaPercent = bot->GetMaxPower(POWER_MANA) > 0
            ? bot->GetPower(POWER_MANA) / float(bot->GetMaxPower(POWER_MANA))
            : 1.0f;
        context.inCombat = bot->IsInCombat();
        context.hasAggro = ai->HasAggro();

        // Group state
        Group* group = bot->GetGroup();
        context.inGroup = (group != nullptr);
        context.groupSize = group ? group->GetMembersCount() : 1;
        context.lowestAllyHealthPercent = GetLowestAllyHealth(bot, group);
        context.enemiesInRange = CountEnemiesInRange(bot, 40.0f);

        // Role detection
        context.role = DetectRole(ai);

        // Timing
        context.timeSinceCombatStart = ai->GetTimeSinceCombatStart();
        context.lastDecisionTime = getMSTime();

        return context;
    }

private:
    static float GetLowestAllyHealth(Player* bot, Group* group)
    {
        if (!group)
            return 1.0f;

        float lowest = 1.0f;
        for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
        {
            Player* member = ref->GetSource();
            if (member && member->IsInWorld())
            {
                float healthPct = member->GetHealthPct() / 100.0f;
                if (healthPct < lowest)
                    lowest = healthPct;
            }
        }

        return lowest;
    }

    static uint32 CountEnemiesInRange(Player* bot, float range)
    {
        // Use spatial grid to count enemies
        // Implementation depends on existing spatial system
        return 0; // Placeholder
    }

    static UtilityContext::Role DetectRole(BotAI* ai)
    {
        // Based on spec, talents, gear
        // Tanks: Protection warriors, Prot paladins, Blood DKs, etc.
        // Healers: Holy priests, Resto druids, Holy paladins, etc.
        // DPS: Everything else
        return UtilityContext::Role::DPS; // Placeholder
    }
};

} // namespace Playerbot
```

### 1.3: Integration with BotAI (Week 2)

**File:** `AI/BotAI.h` (modifications)

```cpp
class TC_GAME_API BotAI
{
    // ... existing code ...

private:
    // NEW: Utility AI system
    std::unique_ptr<UtilityAI> _utilityAI;
    UtilityContext _lastContext;
    UtilityBehavior* _activeBehavior;
    uint32 _lastUtilityUpdate;

public:
    /**
     * @brief Initialize utility-based decision system
     */
    void InitializeUtilityAI();

    /**
     * @brief Update utility AI decision (called every frame or throttled)
     */
    void UpdateUtilityDecision(uint32 diff);

    /**
     * @brief Get currently active utility behavior
     */
    UtilityBehavior* GetActiveBehavior() const { return _activeBehavior; }
};
```

**File:** `AI/BotAI.cpp` (implementation)

```cpp
void BotAI::InitializeUtilityAI()
{
    _utilityAI = std::make_unique<UtilityAI>();

    // Register combat behavior
    auto combatBehavior = std::make_shared<UtilityBehavior>("Combat");
    combatBehavior->AddEvaluator(std::make_shared<CombatEngageEvaluator>());
    combatBehavior->AddEvaluator(std::make_shared<DefensiveCooldownEvaluator>());
    _utilityAI->AddBehavior(combatBehavior);

    // Register healing behavior
    auto healBehavior = std::make_shared<UtilityBehavior>("Healing");
    healBehavior->AddEvaluator(std::make_shared<HealAllyEvaluator>());
    _utilityAI->AddBehavior(healBehavior);

    // Register tanking behavior
    auto tankBehavior = std::make_shared<UtilityBehavior>("Tanking");
    tankBehavior->AddEvaluator(std::make_shared<TankThreatEvaluator>());
    _utilityAI->AddBehavior(tankBehavior);

    // ... more behaviors registered here

    _activeBehavior = nullptr;
    _lastUtilityUpdate = 0;
}

void BotAI::UpdateUtilityDecision(uint32 diff)
{
    // Throttle updates to every 500ms
    _lastUtilityUpdate += diff;
    if (_lastUtilityUpdate < 500)
        return;

    _lastUtilityUpdate = 0;

    // Build current context
    _lastContext = UtilityContextBuilder::Build(this, GetBlackboard());

    // Select best behavior
    UtilityBehavior* newBehavior = _utilityAI->SelectBehavior(_lastContext);

    // Log behavior transitions
    if (newBehavior != _activeBehavior)
    {
        TC_LOG_DEBUG("playerbot.utility", "Bot {} switching from {} to {}",
            GetBot()->GetName(),
            _activeBehavior ? _activeBehavior->GetName() : "None",
            newBehavior ? newBehavior->GetName() : "None");
    }

    _activeBehavior = newBehavior;
}
```

### 1.4: Testing & Validation (Week 2)

**File:** `Tests/UtilityAITest.cpp`

```cpp
#include "catch2/catch.hpp"
#include "AI/Utility/UtilitySystem.h"
#include "AI/Utility/Evaluators/CombatEvaluators.h"

using namespace Playerbot;

TEST_CASE("Utility AI: Combat evaluator scores correctly", "[utility][combat]")
{
    UtilityContext context;
    context.healthPercent = 0.8f;
    context.inCombat = false;
    context.enemiesInRange = 3;

    CombatEngageEvaluator evaluator;

    SECTION("High health with enemies gives positive score")
    {
        float score = evaluator.Evaluate(context);
        REQUIRE(score > 0.0f);
        REQUIRE(score < 1.0f);
    }

    SECTION("No enemies gives zero score")
    {
        context.enemiesInRange = 0;
        float score = evaluator.Evaluate(context);
        REQUIRE(score == 0.0f);
    }

    SECTION("Already in combat maintains high score")
    {
        context.inCombat = true;
        float score = evaluator.Evaluate(context);
        REQUIRE(score >= 0.8f);
    }
}

TEST_CASE("Utility AI: Healer evaluator prioritizes wounded allies", "[utility][healing]")
{
    UtilityContext context;
    context.role = UtilityContext::Role::HEALER;
    context.manaPercent = 0.8f;
    context.lowestAllyHealthPercent = 0.3f; // Wounded ally

    HealAllyEvaluator evaluator;

    SECTION("Wounded ally with mana gives high score")
    {
        float score = evaluator.Evaluate(context);
        REQUIRE(score > 0.5f);
    }

    SECTION("No mana gives zero score")
    {
        context.manaPercent = 0.05f;
        float score = evaluator.Evaluate(context);
        REQUIRE(score == 0.0f);
    }

    SECTION("Non-healer gives zero score")
    {
        context.role = UtilityContext::Role::DPS;
        float score = evaluator.Evaluate(context);
        REQUIRE(score == 0.0f);
    }
}
```

---

## Phase 2: Behavior Tree Execution Layer

**Estimated Time:** 3-4 weeks
**Complexity:** HIGH
**Impact:** Structured execution of selected behaviors

### 2.1: BehaviorTree.CPP Integration (Week 1)

#### Dependencies

```cmake
# CMakeLists.txt additions
find_package(BehaviorTreeCPP REQUIRED)
target_link_libraries(playerbot PRIVATE BehaviorTree::behaviortree_cpp)
```

#### Core Integration

**File:** `AI/BehaviorTree/BTIntegration.h`

```cpp
#pragma once

#include <behaviortree_cpp/bt_factory.h>
#include <behaviortree_cpp/loggers/bt_cout_logger.h>
#include <behaviortree_cpp/loggers/bt_file_logger.h>

namespace Playerbot
{

/**
 * @brief TrinityCore integration for BehaviorTree.CPP
 */
class TC_GAME_API BehaviorTreeManager
{
public:
    BehaviorTreeManager(BotAI* ai);
    ~BehaviorTreeManager();

    /**
     * @brief Register all custom nodes
     */
    void RegisterNodes();

    /**
     * @brief Load behavior tree from XML
     */
    bool LoadTree(std::string const& xmlPath);

    /**
     * @brief Load tree from string
     */
    bool LoadTreeFromString(std::string const& xml);

    /**
     * @brief Tick the behavior tree
     * @return Current tree status
     */
    BT::NodeStatus Tick();

    /**
     * @brief Get the blackboard
     */
    BT::Blackboard::Ptr GetBlackboard() const { return _blackboard; }

    /**
     * @brief Enable/disable logging
     */
    void SetLogging(bool enabled);

private:
    BotAI* _ai;
    BT::BehaviorTreeFactory _factory;
    BT::Tree _tree;
    BT::Blackboard::Ptr _blackboard;
    std::unique_ptr<BT::StdCoutLogger> _logger;
};

} // namespace Playerbot
```

### 2.2: Custom BT Nodes (Week 2)

**File:** `AI/BehaviorTree/Nodes/CombatNodes.h`

```cpp
#pragma once

#include <behaviortree_cpp/action_node.h>
#include "AI/BotAI.h"

namespace Playerbot
{

/**
 * @brief Check if bot is in melee range of target
 */
class CheckInRangeNode : public BT::ConditionNode
{
public:
    CheckInRangeNode(const std::string& name, const BT::NodeConfiguration& config)
        : BT::ConditionNode(name, config) {}

    static BT::PortsList providedPorts()
    {
        return { BT::InputPort<float>("range") };
    }

    BT::NodeStatus tick() override
    {
        BotAI* ai = config().blackboard->get<BotAI*>("bot_ai");
        float range = 5.0f;
        getInput("range", range);

        Unit* target = ai->GetBot()->GetVictim();
        if (!target)
            return BT::NodeStatus::FAILURE;

        float dist = ai->GetBot()->GetDistance(target);
        return (dist <= range) ? BT::NodeStatus::SUCCESS : BT::NodeStatus::FAILURE;
    }
};

/**
 * @brief Face the current target
 */
class FaceTargetNode : public BT::SyncActionNode
{
public:
    FaceTargetNode(const std::string& name, const BT::NodeConfiguration& config)
        : BT::SyncActionNode(name, config) {}

    static BT::PortsList providedPorts() { return {}; }

    BT::NodeStatus tick() override
    {
        BotAI* ai = config().blackboard->get<BotAI*>("bot_ai");
        Unit* target = ai->GetBot()->GetVictim();

        if (!target)
            return BT::NodeStatus::FAILURE;

        ai->GetBot()->SetFacingTo(ai->GetBot()->GetAngle(target));
        return BT::NodeStatus::SUCCESS;
    }
};

/**
 * @brief Execute combat rotation
 */
class ExecuteRotationNode : public BT::StatefulActionNode
{
public:
    ExecuteRotationNode(const std::string& name, const BT::NodeConfiguration& config)
        : BT::StatefulActionNode(name, config) {}

    static BT::PortsList providedPorts() { return {}; }

    BT::NodeStatus onStart() override
    {
        BotAI* ai = config().blackboard->get<BotAI*>("bot_ai");
        // Start rotation
        return BT::NodeStatus::RUNNING;
    }

    BT::NodeStatus onRunning() override
    {
        BotAI* ai = config().blackboard->get<BotAI*>("bot_ai");

        // Execute class-specific rotation
        // This delegates to ClassAI system
        bool success = ai->ExecuteClassRotation();

        return success ? BT::NodeStatus::SUCCESS : BT::NodeStatus::RUNNING;
    }

    void onHalted() override
    {
        // Cleanup when interrupted
    }
};

} // namespace Playerbot
```

### 2.3: Behavior Tree Definitions (Week 3)

**File:** `data/playerbots/behaviors/combat_melee.xml`

```xml
<root main_tree_to_execute="MeleeCombat">
    <BehaviorTree ID="MeleeCombat">
        <Sequence name="MeleeCombatSequence">
            <!-- Check if we have a target -->
            <Condition ID="HasTarget"/>

            <!-- Approach target if out of range -->
            <Fallback>
                <Condition ID="CheckInRange" range="5.0"/>
                <Action ID="MoveToTarget" range="5.0"/>
            </Fallback>

            <!-- Face the target -->
            <Action ID="FaceTarget"/>

            <!-- Execute rotation -->
            <Fallback>
                <!-- Priority rotation -->
                <Sequence>
                    <Condition ID="CanCastSpell" spell_id="12294"/> <!-- Mortal Strike -->
                    <Action ID="CastSpell" spell_id="12294"/>
                </Sequence>

                <Sequence>
                    <Condition ID="CanCastSpell" spell_id="7384"/> <!-- Overpower -->
                    <Condition ID="HasBuff" buff_id="7384"/> <!-- Overpower proc -->
                    <Action ID="CastSpell" spell_id="7384"/>
                </Sequence>

                <Sequence>
                    <Condition ID="CanCastSpell" spell_id="5308"/> <!-- Execute -->
                    <Condition ID="TargetHealthBelow" percent="20"/>
                    <Action ID="CastSpell" spell_id="5308"/>
                </Sequence>

                <!-- Default: auto-attack -->
                <Action ID="AutoAttack"/>
            </Fallback>
        </Sequence>
    </BehaviorTree>
</root>
```

**File:** `data/playerbots/behaviors/healing.xml`

```xml
<root main_tree_to_execute="GroupHealing">
    <BehaviorTree ID="GroupHealing">
        <Sequence name="HealingSequence">
            <!-- Find wounded ally -->
            <Action ID="FindWoundedAlly" health_threshold="0.7"/>

            <!-- Check line of sight -->
            <Condition ID="HasLineOfSight"/>

            <!-- Priority healing -->
            <Fallback>
                <!-- Emergency heal (target <30% health) -->
                <Sequence>
                    <Condition ID="TargetHealthBelow" percent="30"/>
                    <Condition ID="CanCastSpell" spell_id="48782"/> <!-- Flash Heal -->
                    <Action ID="CastSpell" spell_id="48782"/>
                </Sequence>

                <!-- Renew if not active -->
                <Sequence>
                    <Condition ID="TargetHealthBelow" percent="80"/>
                    <Condition ID="TargetMissingBuff" buff_id="48068"/> <!-- Renew -->
                    <Condition ID="CanCastSpell" spell_id="48068"/>
                    <Action ID="CastSpell" spell_id="48068"/>
                </Sequence>

                <!-- Greater Heal (efficient) -->
                <Sequence>
                    <Condition ID="TargetHealthBelow" percent="70"/>
                    <Condition ID="CanCastSpell" spell_id="48063"/> <!-- Greater Heal -->
                    <Action ID="CastSpell" spell_id="48063"/>
                </Sequence>
            </Fallback>
        </Sequence>
    </BehaviorTree>
</root>
```

### 2.4: Hybrid Integration: Utility AI → Behavior Tree (Week 4)

**File:** `AI/HybridAIController.h`

```cpp
#pragma once

#include "Utility/UtilitySystem.h"
#include "BehaviorTree/BTIntegration.h"

namespace Playerbot
{

/**
 * @brief Hybrid controller: Utility AI selects behavior, BT executes it
 */
class TC_GAME_API HybridAIController
{
public:
    HybridAIController(BotAI* ai);
    ~HybridAIController();

    /**
     * @brief Initialize both systems
     */
    void Initialize();

    /**
     * @brief Update decision + execution
     */
    void Update(uint32 diff);

private:
    BotAI* _ai;

    // Decision layer
    std::unique_ptr<UtilityAI> _utilityAI;
    UtilityBehavior* _activeBehavior;

    // Execution layer
    std::unique_ptr<BehaviorTreeManager> _btManager;
    std::unordered_map<std::string, std::string> _behaviorToTreeMap;

    // State
    std::string _currentTreeName;
    uint32 _decisionUpdateTimer;

    /**
     * @brief Switch to a different behavior tree
     */
    void SwitchTree(std::string const& treeName);
};

} // namespace Playerbot
```

**Implementation:**

```cpp
HybridAIController::HybridAIController(BotAI* ai)
    : _ai(ai)
    , _utilityAI(std::make_unique<UtilityAI>())
    , _btManager(std::make_unique<BehaviorTreeManager>(ai))
    , _activeBehavior(nullptr)
    , _decisionUpdateTimer(0)
{
}

void HybridAIController::Initialize()
{
    // Map utility behaviors to behavior trees
    _behaviorToTreeMap["Combat"] = "combat_melee.xml";
    _behaviorToTreeMap["Healing"] = "healing.xml";
    _behaviorToTreeMap["Tanking"] = "tanking.xml";
    _behaviorToTreeMap["Movement"] = "movement.xml";

    // Initialize utility behaviors (from Phase 1)
    // ... register combat, healing, tank, etc. behaviors

    // Load default tree
    _btManager->LoadTree("data/playerbots/behaviors/idle.xml");
    _currentTreeName = "idle";
}

void HybridAIController::Update(uint32 diff)
{
    // PHASE 1: Utility AI Decision (every 500ms)
    _decisionUpdateTimer += diff;
    if (_decisionUpdateTimer >= 500)
    {
        _decisionUpdateTimer = 0;

        // Build context
        UtilityContext context = UtilityContextBuilder::Build(_ai, _ai->GetBlackboard());

        // Select best behavior
        UtilityBehavior* newBehavior = _utilityAI->SelectBehavior(context);

        // Switch tree if behavior changed
        if (newBehavior != _activeBehavior)
        {
            _activeBehavior = newBehavior;

            if (newBehavior)
            {
                std::string treeName = _behaviorToTreeMap[newBehavior->GetName()];
                SwitchTree(treeName);
            }
        }
    }

    // PHASE 2: Behavior Tree Execution (every frame)
    if (_activeBehavior)
    {
        BT::NodeStatus status = _btManager->Tick();

        // Handle tree completion
        if (status == BT::NodeStatus::SUCCESS || status == BT::NodeStatus::FAILURE)
        {
            // Tree finished, re-evaluate next frame
            _decisionUpdateTimer = 500; // Force decision update
        }
    }
}

void HybridAIController::SwitchTree(std::string const& treeName)
{
    if (treeName == _currentTreeName)
        return;

    TC_LOG_DEBUG("playerbot.hybrid", "Bot {} switching tree: {} -> {}",
        _ai->GetBot()->GetName(), _currentTreeName, treeName);

    _btManager->LoadTree("data/playerbots/behaviors/" + treeName);
    _currentTreeName = treeName;
}
```

---

## Phase 3: Hierarchical Group Coordination

**Estimated Time:** 3-4 weeks
**Complexity:** VERY HIGH
**Impact:** Synchronized group decisions for all roles

### 3.1: Group Orchestrator Architecture (Week 1)

**File:** `AI/GroupCoordination/GroupOrchestrator.h`

```cpp
#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include <vector>
#include <memory>

namespace Playerbot
{

class BotAI;
class Blackboard;

/**
 * @brief Role-specific agent within group
 */
enum class GroupRole
{
    TANK,
    HEALER,
    DPS_MELEE,
    DPS_RANGED,
    SUPPORT
};

/**
 * @brief Group-wide tactical decision
 */
struct GroupTactic
{
    enum class Type
    {
        FOCUS_FIRE,      // All DPS attack same target
        SPREAD_OUT,      // Spread to avoid AoE
        STACK_UP,        // Stack for healing/buffs
        CC_ROTATION,     // Coordinate crowd control
        INTERRUPT_ROTATION, // Coordinate interrupts
        DEFENSIVE_COOLDOWNS // Coordinate defensive CDs
    } type;

    ObjectGuid primaryTarget;
    std::vector<ObjectGuid> ccTargets;
    uint32 duration;
    uint32 startTime;
};

/**
 * @brief Orchestrates decisions for a group (5-40 bots)
 */
class TC_GAME_API GroupOrchestrator
{
public:
    GroupOrchestrator(Group* group);
    ~GroupOrchestrator();

    /**
     * @brief Update group coordination
     * Called once per group, not per bot
     */
    void Update(uint32 diff);

    /**
     * @brief Register a bot with this orchestrator
     */
    void RegisterBot(BotAI* bot, GroupRole role);

    /**
     * @brief Unregister a bot
     */
    void UnregisterBot(BotAI* bot);

    /**
     * @brief Get current group tactic
     */
    GroupTactic const* GetActiveTactic() const { return _activeTactic.get(); }

    /**
     * @brief Get assigned role for a bot
     */
    GroupRole GetBotRole(ObjectGuid botGuid) const;

    /**
     * @brief Get all bots with specific role
     */
    std::vector<BotAI*> GetBotsWithRole(GroupRole role) const;

    /**
     * @brief Get group blackboard (shared state)
     */
    Blackboard* GetGroupBlackboard() const { return _groupBlackboard.get(); }

private:
    Group* _group;

    // Role assignments
    std::unordered_map<ObjectGuid, GroupRole> _roleAssignments;
    std::unordered_map<ObjectGuid, BotAI*> _registeredBots;

    // Shared state
    std::unique_ptr<Blackboard> _groupBlackboard;

    // Current tactic
    std::unique_ptr<GroupTactic> _activeTactic;
    uint32 _tacticEvalTimer;

    // Role-specific coordination
    struct TankCoordination
    {
        ObjectGuid mainTank;
        ObjectGuid offTank;
        std::vector<ObjectGuid> threatTargets;
    };

    struct HealerCoordination
    {
        std::unordered_map<ObjectGuid, ObjectGuid> healerAssignments; // healer -> tank
        uint32 nextRotationIndex;
    };

    struct DPSCoordination
    {
        ObjectGuid focusTarget;
        std::vector<ObjectGuid> ccAssignments;
        std::unordered_map<uint32, ObjectGuid> interruptRotation; // spell_id -> assigned_bot
    };

    TankCoordination _tankCoord;
    HealerCoordination _healerCoord;
    DPSCoordination _dpsCoord;

    /**
     * @brief Evaluate and select group tactic
     */
    void EvaluateGroupTactic();

    /**
     * @brief Update tank coordination
     */
    void UpdateTankCoordination();

    /**
     * @brief Update healer coordination
     */
    void UpdateHealerCoordination();

    /**
     * @brief Update DPS coordination
     */
    void UpdateDPSCoordination();

    /**
     * @brief Assign interrupt responsibilities
     */
    void AssignInterrupts(Unit* target);
};

} // namespace Playerbot
```

### 3.2: Role-Based Decision Synchronization (Week 2)

**File:** `AI/GroupCoordination/RoleCoordinator.h`

```cpp
#pragma once

#include "GroupOrchestrator.h"

namespace Playerbot
{

/**
 * @brief Tank-specific coordination logic
 */
class TC_GAME_API TankCoordinator
{
public:
    TankCoordinator(GroupOrchestrator* orchestrator);

    /**
     * @brief Coordinate tank decisions
     * Ensures main tank holds aggro, off-tank picks up adds
     */
    void CoordinateTanks(uint32 diff);

    /**
     * @brief Assign threat targets
     */
    void AssignThreatTargets(std::vector<Unit*> const& enemies);

    /**
     * @brief Coordinate taunt rotation
     */
    void CoordinateTaunts(Unit* boss);

private:
    GroupOrchestrator* _orchestrator;
    uint32 _lastTauntTime;
    ObjectGuid _currentTaunter;
};

/**
 * @brief Healer-specific coordination logic
 */
class TC_GAME_API HealerCoordinator
{
public:
    HealerCoordinator(GroupOrchestrator* orchestrator);

    /**
     * @brief Coordinate healer decisions
     * Assigns healers to specific tanks, avoids overhealing
     */
    void CoordinateHealers(uint32 diff);

    /**
     * @brief Assign healing responsibilities
     * Each healer gets primary targets to focus on
     */
    void AssignHealingTargets();

    /**
     * @brief Coordinate mana cooldowns
     */
    void CoordinateManaCooldowns();

private:
    GroupOrchestrator* _orchestrator;
    std::unordered_map<ObjectGuid, std::vector<ObjectGuid>> _healerAssignments;
};

/**
 * @brief DPS-specific coordination logic
 */
class TC_GAME_API DPSCoordinator
{
public:
    DPSCoordinator(GroupOrchestrator* orchestrator);

    /**
     * @brief Coordinate DPS decisions
     * Focus fire, interrupt rotations, CC assignments
     */
    void CoordinateDPS(uint32 diff);

    /**
     * @brief Assign focus target for all DPS
     */
    void SetFocusTarget(Unit* target);

    /**
     * @brief Coordinate interrupt rotation
     */
    void SetupInterruptRotation(Unit* caster);

    /**
     * @brief Assign crowd control targets
     */
    void AssignCrowdControl(std::vector<Unit*> const& targets);

private:
    GroupOrchestrator* _orchestrator;
    ObjectGuid _focusTarget;
    std::unordered_map<uint32, std::vector<ObjectGuid>> _interruptRotations; // spell_id -> bot rotation
    uint32 _lastInterruptIndex;
};

} // namespace Playerbot
```

### 3.3: Group Decision Synchronization Protocol (Week 3)

**File:** `AI/GroupCoordination/SyncProtocol.h`

```cpp
#pragma once

#include "Define.h"
#include <atomic>

namespace Playerbot
{

/**
 * @brief Group decision synchronization point
 * Ensures all bots wait until group decision is made
 */
class TC_GAME_API GroupSyncPoint
{
public:
    GroupSyncPoint(uint32 requiredCount)
        : _requiredCount(requiredCount)
        , _arrivalCount(0)
        , _generation(0)
    {}

    /**
     * @brief Bot arrives at sync point and waits
     * @return true when all bots have arrived
     */
    bool Arrive()
    {
        uint32 gen = _generation.load(std::memory_order_acquire);
        uint32 count = _arrivalCount.fetch_add(1, std::memory_order_acq_rel) + 1;

        if (count == _requiredCount)
        {
            // Last bot to arrive resets counter and advances generation
            _arrivalCount.store(0, std::memory_order_release);
            _generation.fetch_add(1, std::memory_order_release);
            return true;
        }
        else
        {
            // Wait for generation to advance
            while (_generation.load(std::memory_order_acquire) == gen)
            {
                std::this_thread::yield();
            }
            return false;
        }
    }

    /**
     * @brief Reset sync point
     */
    void Reset()
    {
        _arrivalCount.store(0, std::memory_order_release);
    }

private:
    uint32 _requiredCount;
    std::atomic<uint32> _arrivalCount;
    std::atomic<uint32> _generation;
};

/**
 * @brief Manages group-wide decision synchronization
 */
class TC_GAME_API GroupDecisionSync
{
public:
    /**
     * @brief Broadcast decision to all group members
     */
    static void BroadcastDecision(GroupOrchestrator* orch, GroupTactic const& tactic);

    /**
     * @brief Wait for all bots to acknowledge decision
     */
    static bool WaitForAcknowledgement(GroupOrchestrator* orch, uint32 timeoutMs);

    /**
     * @brief Bot acknowledges group decision
     */
    static void AcknowledgeDecision(BotAI* bot, GroupOrchestrator* orch);
};

} // namespace Playerbot
```

### 3.4: Hierarchical Scaling: Raid Orchestrator (Week 4)

**File:** `AI/GroupCoordination/RaidOrchestrator.h`

```cpp
#pragma once

#include "GroupOrchestrator.h"

namespace Playerbot
{

/**
 * @brief Coordinates multiple groups in a raid (up to 40 bots)
 * Hierarchical: Raid → Squad → Group → Bot
 */
class TC_GAME_API RaidOrchestrator
{
public:
    RaidOrchestrator(Group* raid);
    ~RaidOrchestrator();

    /**
     * @brief Update raid coordination
     */
    void Update(uint32 diff);

    /**
     * @brief Register a squad orchestrator
     */
    void RegisterSquad(std::string const& name, std::unique_ptr<GroupOrchestrator> squad);

    /**
     * @brief Get squad by name
     */
    GroupOrchestrator* GetSquad(std::string const& name) const;

    /**
     * @brief Broadcast raid-wide tactic
     */
    void BroadcastRaidTactic(GroupTactic const& tactic);

private:
    Group* _raid;

    // Squad orchestrators (e.g., "Tank Squad", "Healer Squad", "DPS Squad 1")
    std::unordered_map<std::string, std::unique_ptr<GroupOrchestrator>> _squads;

    // Raid-wide state
    std::unique_ptr<Blackboard> _raidBlackboard;

    /**
     * @brief Evaluate raid-wide tactics
     */
    void EvaluateRaidTactic();
};

} // namespace Playerbot
```

---

## Phase 4: Blackboard Shared State System

**Estimated Time:** 2 weeks
**Complexity:** MEDIUM
**Impact:** Efficient shared state for coordination

### 4.1: Blackboard Architecture (Week 1)

**File:** `AI/Blackboard/Blackboard.h`

```cpp
#pragma once

#include "Define.h"
#include <unordered_map>
#include <any>
#include <string>
#include <shared_mutex>

namespace Playerbot
{

/**
 * @brief Shared data structure for multi-agent coordination
 * Implements the Blackboard architectural pattern
 */
class TC_GAME_API Blackboard
{
public:
    Blackboard() = default;
    ~Blackboard() = default;

    /**
     * @brief Write a value to the blackboard
     * Thread-safe
     */
    template<typename T>
    void Set(std::string const& key, T const& value)
    {
        std::unique_lock lock(_mutex);
        _data[key] = value;
    }

    /**
     * @brief Read a value from the blackboard
     * Thread-safe
     */
    template<typename T>
    bool Get(std::string const& key, T& outValue) const
    {
        std::shared_lock lock(_mutex);

        auto it = _data.find(key);
        if (it == _data.end())
            return false;

        try
        {
            outValue = std::any_cast<T>(it->second);
            return true;
        }
        catch (std::bad_any_cast const&)
        {
            return false;
        }
    }

    /**
     * @brief Get value with default fallback
     */
    template<typename T>
    T GetOr(std::string const& key, T const& defaultValue) const
    {
        T value;
        return Get(key, value) ? value : defaultValue;
    }

    /**
     * @brief Check if key exists
     */
    bool Has(std::string const& key) const
    {
        std::shared_lock lock(_mutex);
        return _data.find(key) != _data.end();
    }

    /**
     * @brief Remove a key
     */
    void Remove(std::string const& key)
    {
        std::unique_lock lock(_mutex);
        _data.erase(key);
    }

    /**
     * @brief Clear all data
     */
    void Clear()
    {
        std::unique_lock lock(_mutex);
        _data.clear();
    }

    /**
     * @brief Get all keys
     */
    std::vector<std::string> GetKeys() const
    {
        std::shared_lock lock(_mutex);
        std::vector<std::string> keys;
        keys.reserve(_data.size());
        for (auto const& [key, _] : _data)
            keys.push_back(key);
        return keys;
    }

private:
    mutable std::shared_mutex _mutex;
    std::unordered_map<std::string, std::any> _data;
};

/**
 * @brief Scoped blackboard keys (namespace-like organization)
 */
namespace BlackboardKeys
{
    // Group coordination
    constexpr const char* GROUP_FOCUS_TARGET = "group.focus_target";
    constexpr const char* GROUP_TACTIC = "group.active_tactic";
    constexpr const char* GROUP_MAIN_TANK = "group.main_tank";
    constexpr const char* GROUP_OFF_TANK = "group.off_tank";

    // Combat state
    constexpr const char* COMBAT_PULL_TIMER = "combat.pull_timer";
    constexpr const char* COMBAT_THREAT_TABLE = "combat.threat_table";
    constexpr const char* COMBAT_CC_TARGETS = "combat.cc_targets";
    constexpr const char* COMBAT_INTERRUPT_ROTATION = "combat.interrupt_rotation";

    // Healer coordination
    constexpr const char* HEALER_ASSIGNMENTS = "healer.assignments";
    constexpr const char* HEALER_MANA_STATE = "healer.mana_state";
    constexpr const char* HEALER_NEXT_ROTATION = "healer.next_rotation";

    // DPS coordination
    constexpr const char* DPS_FOCUS_TARGET = "dps.focus_target";
    constexpr const char* DPS_BURST_WINDOW = "dps.burst_window";
    constexpr const char* DPS_KILL_ORDER = "dps.kill_order";
}

} // namespace Playerbot
```

### 4.2: Blackboard Integration with Group System (Week 2)

**File:** `AI/Blackboard/GroupBlackboard.h`

```cpp
#pragma once

#include "Blackboard.h"
#include "AI/GroupCoordination/GroupOrchestrator.h"

namespace Playerbot
{

/**
 * @brief Specialized blackboard for group coordination
 */
class TC_GAME_API GroupBlackboard : public Blackboard
{
public:
    GroupBlackboard(GroupOrchestrator* orchestrator);

    /**
     * @brief Update group state on blackboard
     * Called by orchestrator each frame
     */
    void UpdateGroupState(Group* group);

    /**
     * @brief Get focus target
     */
    Unit* GetFocusTarget() const;

    /**
     * @brief Set focus target
     */
    void SetFocusTarget(Unit* target);

    /**
     * @brief Get main tank
     */
    Player* GetMainTank() const;

    /**
     * @brief Set main tank
     */
    void SetMainTank(Player* tank);

    /**
     * @brief Get active group tactic
     */
    GroupTactic const* GetActiveTactic() const;

    /**
     * @brief Set active group tactic
     */
    void SetActiveTactic(GroupTactic const& tactic);

private:
    GroupOrchestrator* _orchestrator;
};

} // namespace Playerbot
```

**Usage Example:**

```cpp
// Tank bot reads group focus target from blackboard
void TankAI::SelectTarget()
{
    GroupBlackboard* board = GetGroupOrchestrator()->GetGroupBlackboard();

    Unit* focusTarget = board->GetFocusTarget();
    if (focusTarget && !focusTarget->IsDead())
    {
        // Attack the group's focus target
        SetTarget(focusTarget);
    }
}

// DPS coordinator writes focus target to blackboard
void DPSCoordinator::SetFocusTarget(Unit* target)
{
    GroupBlackboard* board = _orchestrator->GetGroupBlackboard();
    board->SetFocusTarget(target);

    TC_LOG_DEBUG("playerbot.group", "DPS focus target set to: {}", target->GetName());
}
```

---

## Phase 5: ClassAI Integration

**Estimated Time:** 4-6 weeks
**Complexity:** VERY HIGH
**Impact:** Clean up class-specific code, integrate with hybrid system

### 5.1: Deduplication Strategy

**Current State:**
- 13 classes × 3 specs × ~175 lines = ~6,825 lines of duplicated rotation logic
- Hardcoded if/else chains for spell priorities
- Difficult to maintain and extend

**New Architecture:**
- Utility evaluators score each spell based on context
- Behavior trees define rotation structure
- Data-driven spell priorities in XML/JSON

### 5.2: Example: Warrior Class Integration

**File:** `data/playerbots/classes/warrior/arms_rotation.xml`

```xml
<root main_tree_to_execute="ArmsWarrior">
    <BehaviorTree ID="ArmsWarrior">
        <Sequence name="ArmsRotation">
            <!-- Ensure we have a target and are in range -->
            <Condition ID="HasTarget"/>
            <Condition ID="CheckInRange" range="5.0"/>
            <Action ID="FaceTarget"/>

            <!-- Execute priority rotation -->
            <Fallback>
                <!-- 1. Mortal Strike (primary ability) -->
                <Sequence>
                    <Condition ID="CanCastSpell" spell_id="12294"/>
                    <Condition ID="SpellUtilityAbove" spell_id="12294" threshold="0.7"/>
                    <Action ID="CastSpell" spell_id="12294"/>
                </Sequence>

                <!-- 2. Overpower (if proc active) -->
                <Sequence>
                    <Condition ID="CanCastSpell" spell_id="7384"/>
                    <Condition ID="HasBuff" buff_id="7384"/>
                    <Action ID="CastSpell" spell_id="7384"/>
                </Sequence>

                <!-- 3. Execute (if target <20% health) -->
                <Sequence>
                    <Condition ID="CanCastSpell" spell_id="5308"/>
                    <Condition ID="TargetHealthBelow" percent="20"/>
                    <Action ID="CastSpell" spell_id="5308"/>
                </Sequence>

                <!-- 4. Slam (if excess rage) -->
                <Sequence>
                    <Condition ID="CanCastSpell" spell_id="1464"/>
                    <Condition ID="RageAbove" amount="60"/>
                    <Action ID="CastSpell" spell_id="1464"/>
                </Sequence>

                <!-- 5. Whirlwind (if 3+ enemies) -->
                <Sequence>
                    <Condition ID="CanCastSpell" spell_id="1680"/>
                    <Condition ID="EnemyCountAbove" count="3" range="8.0"/>
                    <Action ID="CastSpell" spell_id="1680"/>
                </Sequence>

                <!-- Default: auto-attack -->
                <Action ID="AutoAttack"/>
            </Fallback>
        </Sequence>
    </BehaviorTree>
</root>
```

**File:** `AI/ClassAI/Warriors/WarriorUtilityEvaluators.h`

```cpp
#pragma once

#include "AI/Utility/UtilitySystem.h"

namespace Playerbot
{

/**
 * @brief Evaluates utility of Mortal Strike
 */
class MortalStrikeEvaluator : public UtilityEvaluator
{
public:
    MortalStrikeEvaluator() : UtilityEvaluator("MortalStrike", 1.0f) {}

    float Evaluate(UtilityContext const& context) const override
    {
        // High base priority (primary ability)
        float basePriority = 0.9f;

        // Bonus if target has no healing reduction
        if (!TargetHasDebuff(context.bot, MORTAL_STRIKE_DEBUFF))
            basePriority += 0.1f;

        return basePriority;
    }
};

/**
 * @brief Evaluates utility of Execute
 */
class ExecuteEvaluator : public UtilityEvaluator
{
public:
    ExecuteEvaluator() : UtilityEvaluator("Execute", 1.0f) {}

    float Evaluate(UtilityContext const& context) const override
    {
        float targetHealth = GetTargetHealthPercent(context.bot);

        // Only viable below 20% health
        if (targetHealth > 0.2f)
            return 0.0f;

        // Higher priority as health drops
        return InverseLinear(targetHealth / 0.2f);
    }
};

} // namespace Playerbot
```

### 5.3: ClassAI Deduplication Results

**Before:**
```cpp
// ArmsWarriorAI.cpp (175 lines)
void ArmsWarriorAI::ExecuteRotation()
{
    if (CanCast(MORTAL_STRIKE) && InMeleeRange())
        CastSpell(MORTAL_STRIKE);
    else if (CanCast(OVERPOWER) && HasBuff(OVERPOWER_PROC))
        CastSpell(OVERPOWER);
    // ... 170 more lines
}

// FuryWarriorAI.cpp (175 lines - 70% duplicate logic)
void FuryWarriorAI::ExecuteRotation()
{
    if (CanCast(BLOODTHIRST) && InMeleeRange())
        CastSpell(BLOODTHIRST);
    // ... 170 more lines
}

// ProtectionWarriorAI.cpp (175 lines - 70% duplicate logic)
void ProtectionWarriorAI::ExecuteRotation()
{
    if (CanCast(SHIELD_SLAM) && InMeleeRange())
        CastSpell(SHIELD_SLAM);
    // ... 170 more lines
}
```

**After:**
```cpp
// All 3 specs use same execution engine
void WarriorAI::ExecuteRotation()
{
    // Hybrid AI controller handles both utility + BT
    _hybridController->Update(diff);
}
```

**Data Files:**
- `arms_rotation.xml` (50 lines)
- `fury_rotation.xml` (50 lines)
- `protection_rotation.xml` (50 lines)

**LOC Reduction:**
- Before: 3 specs × 175 lines = 525 lines
- After: 1 shared implementation (20 lines) + 3 XML files (150 lines) = 170 lines
- **Reduction: 67% (-355 lines per class)**
- **Total for 13 classes: ~4,615 lines removed**

---

## Performance & Scalability

### Performance Targets

| Component | Target Latency | Notes |
|-----------|----------------|-------|
| Utility AI Evaluation | <2ms per bot | Throttled to 500ms intervals |
| Behavior Tree Tick | <1ms per bot | Runs every frame |
| Blackboard Read | <0.01ms | Lock-free read with shared_mutex |
| Blackboard Write | <0.05ms | Exclusive write lock |
| Group Orchestrator | <5ms per group | Coordinates 5-40 bots |
| Raid Orchestrator | <10ms per raid | Coordinates up to 40 bots |
| Total per-bot overhead | <3ms | Amortized across frames |

### Scalability Analysis

**5,000 Bots Configuration:**
- **100 raid groups** × 40 bots each = 4,000 bots (raids/dungeons)
- **200 solo groups** × 5 bots each = 1,000 bots (world questing)

**Update Strategy:**
- **Staggered updates**: Spread 5,000 bot updates across 16.67ms frame (60 FPS)
  - 5,000 bots / 60 FPS = ~83 bots per frame
  - 83 bots × 3ms = 249ms total AI time per frame
  - With 8-core CPU: 249ms / 8 = 31ms per core ✅

**Hierarchical Reduction:**
- Without hierarchy: 5,000 bots × 5,000 bots = 25M potential interactions
- With hierarchy: 100 orchestrators × 40 bots = 4,000 interactions
- **Reduction: 99.98%**

### Memory Footprint

| Component | Memory per Bot | 5,000 Bots Total |
|-----------|----------------|------------------|
| Utility AI (evaluators) | 200 KB | 976 MB |
| Behavior Tree (nodes) | 150 KB | 732 MB |
| Blackboard (shared) | 50 KB | 244 MB |
| Orchestrator (shared) | 2 KB | 10 MB |
| **Total** | **402 KB** | **1,962 MB (1.9 GB)** ✅ |

**Comparison:**
- Current system: ~300 KB per bot × 5,000 = 1.5 GB
- New system: ~400 KB per bot × 5,000 = 1.9 GB
- **Increase: +26%** (acceptable for major feature upgrade)

---

## Implementation Timeline

### Phase 1: Utility AI Decision Layer (2-3 weeks)
- **Week 1**: Core utility system, evaluators, context builder
- **Week 2**: BotAI integration, testing
- **Week 3**: Buffer for debugging

### Phase 2: Behavior Tree Execution Layer (3-4 weeks)
- **Week 1**: BehaviorTree.CPP integration, custom nodes
- **Week 2**: Tree definitions (combat, healing, movement)
- **Week 3**: Hybrid controller (Utility AI → BT)
- **Week 4**: Testing, optimization

### Phase 3: Hierarchical Group Coordination (3-4 weeks)
- **Week 1**: Group orchestrator architecture
- **Week 2**: Role-based coordinators (tank, healer, DPS)
- **Week 3**: Synchronization protocol
- **Week 4**: Raid orchestrator, testing

### Phase 4: Blackboard Shared State System (2 weeks)
- **Week 1**: Blackboard implementation, threading
- **Week 2**: Group integration, testing

### Phase 5: ClassAI Integration (4-6 weeks)
- **Week 1-2**: Warrior, Paladin, Death Knight (plate classes)
- **Week 3-4**: Priest, Mage, Warlock (cloth classes)
- **Week 5-6**: Rogue, Hunter, Druid, Shaman (leather/mail classes)

**Total Timeline: 14-19 weeks (~3.5 to 4.5 months)**

### Parallel Development Opportunities

Some phases can be parallelized:
- Phase 1 & 2 can overlap (Utility AI while designing BT trees)
- Phase 3 & 4 can overlap (Orchestrator while building Blackboard)
- Phase 5 can start as soon as Phase 2 completes

**Optimized Timeline: 12-16 weeks (~3 to 4 months)**

---

## Risk Mitigation

### Risk 1: BehaviorTree.CPP Integration Complexity
**Mitigation:**
- Prototype integration in Week 1
- Fallback: Implement lightweight BT system in-house
- BehaviorTree.CPP is mature (5+ years, 2.5k GitHub stars)

### Risk 2: Performance Degradation with 5,000 Bots
**Mitigation:**
- Profiling after each phase
- Throttling and staggered updates from Day 1
- Hierarchical architecture limits exponential growth

### Risk 3: Backward Compatibility with Existing Bots
**Mitigation:**
- Hybrid system coexists with old system
- Gradual migration class-by-class
- Old BehaviorManager infrastructure remains

### Risk 4: Group Sync Deadlocks
**Mitigation:**
- Lock-free blackboard reads (shared_mutex)
- Timeout-based sync points
- Extensive testing with stress tests (100+ groups)

---

## Success Criteria

### Functional Requirements
✅ Bots make context-aware decisions (not hardcoded priorities)
✅ Group members coordinate roles (tank, healer, DPS)
✅ 5,000 bots run concurrently without performance degradation
✅ Behavior trees are data-driven (XML files)
✅ ClassAI code reduced by 60%+

### Performance Requirements
✅ <3ms AI overhead per bot (amortized)
✅ <5ms group coordination per group
✅ <2 GB total memory for 5,000 bots
✅ 60 FPS maintained with 5,000 active bots

### Quality Requirements
✅ Enterprise-grade code quality (const-correctness, RAII, thread-safety)
✅ Comprehensive unit tests (>80% coverage)
✅ Documentation for all public APIs
✅ Future-proof architecture (extensible for ML/RL in Phase 6)

---

## Future Enhancements (Post-Implementation)

### Phase 6: Machine Learning Integration
- Reinforcement learning for adaptive tactics
- Player pattern recognition
- Dynamic difficulty adjustment

### Phase 7: Advanced Coordination
- Multi-raid coordination (100+ bots)
- Battleground strategy (80v80 PvP)
- World boss coordination (200+ bots)

### Phase 8: Performance Dashboard
- Real-time decision visualization
- Grafana metrics for utility scores
- Group coordination heatmaps

---

## Conclusion

This hybrid architecture combines the best of modern AI research:
- **Utility AI** for adaptive, context-aware decisions
- **Behavior Trees** for structured, maintainable execution
- **Hierarchical Multi-Agent** for scalable group coordination
- **Blackboard Pattern** for efficient shared state

The system is designed to scale to **5,000 concurrent bots** while maintaining **enterprise-grade quality**, **future-proof extensibility**, and **60 FPS performance**.

**Estimated Development Time:** 12-16 weeks (3-4 months)
**Team Size:** 1-2 senior C++ developers
**Total LOC:** ~15,000 lines (vs ~40,000 with old approach = 62% reduction)

---

**Document Maintained By:** PlayerBot Architecture Team
**Last Updated:** 2025-11-09
**Next Review:** 2025-12-09
