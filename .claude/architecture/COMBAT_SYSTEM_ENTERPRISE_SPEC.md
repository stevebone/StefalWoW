# Combat System Enterprise Architecture Specification

**Version**: 2.0.0  
**Date**: January 26, 2026  
**Target**: 5,000 Concurrent Bots  
**Classification**: System-Critical Architecture Document

---

## Table of Contents

1. [Executive Summary](#1-executive-summary)
2. [Performance Budget Analysis](#2-performance-budget-analysis)
3. [Existing Infrastructure Audit](#3-existing-infrastructure-audit)
4. [Architecture Overview](#4-architecture-overview)
5. [Layer 1: Event Infrastructure](#5-layer-1-event-infrastructure)
6. [Layer 2: Combat Context System](#6-layer-2-combat-context-system)
7. [Layer 3: Coordination Framework](#7-layer-3-coordination-framework)
8. [Layer 4: Decision Engine](#8-layer-4-decision-engine)
9. [Layer 5: Execution Framework](#9-layer-5-execution-framework)
10. [Layer 6: Data-Driven Configuration](#10-layer-6-data-driven-configuration)
11. [Implementation Roadmap](#12-implementation-roadmap)

---

## 1. Executive Summary

### 1.1 Mission Statement

Design and implement a **production-grade combat system** capable of:
- Supporting **5,000 concurrent bots** with <1% CPU overhead per bot
- Handling all combat contexts: Solo, Group, Dungeon, Raid, Battleground, Arena
- Providing **sub-millisecond** event response times
- Achieving **zero-polling** architecture via TrinityCore event integration
- Enabling **data-driven** configuration without recompilation

### 1.2 Critical Design Principles

| Principle | Implementation |
|-----------|---------------|
| **Event-Driven** | Use existing HostileEventBus + EventDispatcher |
| **Hierarchical** | Server → Zone → Group → Bot coordination |
| **Context-Aware** | Solo uses minimal resources, Raid uses full coordination |
| **Data-Driven** | All thresholds, priorities in YAML |
| **Interface-First** | Every component has an interface for DI |

### 1.3 Success Metrics

| Metric | Target | Critical Threshold |
|--------|--------|-------------------|
| Bots per server | 5,000 | 3,000 minimum |
| CPU per bot (combat) | <0.5% | <1% |
| Event latency | <1ms | <5ms |
| Memory per bot | <2MB | <4MB |
| Interrupt response | <100ms | <200ms |

---

## 2. Performance Budget Analysis

### 2.1 Server Frame Budget

```
Server tick rate: 20 Hz (50ms per tick)
Available CPU per tick: ~48ms (2ms margin)

With 5,000 bots:
- Per-bot budget: 48ms / 5000 = 9.6 μs per bot per tick
- This is EXTREMELY tight

SOLUTION: Tiered updates + Hierarchical sharing
```

### 2.2 Tiered Update Strategy

```cpp
enum class UpdateTier : uint8
{
    CRITICAL = 0,   // Every tick (50ms)    - ~500 bots in combat
    HIGH = 1,       // Every 2 ticks        - ~1000 bots near combat
    NORMAL = 2,     // Every 4 ticks        - ~1500 bots grouped
    LOW = 3         // Every 10 ticks       - ~2000 bots idle/solo
};
```

### 2.3 Hierarchical Calculation Sharing

```
Instead of: 5000 bots × threat calculation = 5000 calculations

With hierarchy:
- 10 zones × zone threat aggregation = 10 calculations
- 200 groups × group threat distribution = 200 calculations
- Individual bots only get pre-calculated assignments

Savings: 96% reduction in threat calculations
```

---

## 3. Existing Infrastructure Audit

### 3.1 Components to REUSE (Already Exist)

| Component | File | Capability |
|-----------|------|------------|
| **HostileEventBus** | `Combat/HostileEventBus.h` | Lock-free MPMC, 10k events |
| **EventDispatcher** | `Core/Events/EventDispatcher.h` | Thread-safe routing |
| **ServiceContainer** | `Core/DI/ServiceContainer.h` | Full DI with factories |
| **BehaviorManager** | `AI/BehaviorManager.h` | Throttled update base |
| **CombatContextDetector** | `AI/Common/CombatContextDetector.h` | Context detection |
| **RaidOrchestrator** | `AI/Coordination/RaidOrchestrator.h` | 40-player coordination |
| **PlayerBotHooks** | `Core/PlayerBotHooks.h` | TrinityCore integration |

### 3.2 Components to FIX

| Component | Problem | Solution |
|-----------|---------|----------|
| **CombatAIIntegrator** | All 13 components disabled | Remove, rebuild with interfaces |
| **ThreatCoordinator** | O(b×t) nested loops | Event-driven, hierarchical |
| **InterruptCoordinator** | O(a×b log b) sorting | Priority queue |
| **28 switch statements** | Role detection scattered | ClassRoleResolver |

### 3.3 Components to CREATE

| Component | Purpose |
|-----------|---------|
| **ICombatCoordinator** | Interface for all coordinators |
| **ICombatContext** | Context-aware behavior |
| **ICombatPlugin** | Class ability plugins |
| **ClassRoleResolver** | Central class/role mapping |
| **ZoneCoordinator** | Zone-level coordination |

---

## 4. Architecture Overview

### 4.1 Six-Layer Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│              LAYER 1: EVENT INFRASTRUCTURE                       │
│  HostileEventBus + EventDispatcher + CombatEventRouter          │
│  • Lock-free event capture from TrinityCore hooks               │
│  • <50μs per event routing                                      │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│              LAYER 2: COMBAT CONTEXT SYSTEM                      │
│  CombatContextManager + ICombatContext implementations           │
│  • Auto-detect: Solo, Group, Dungeon, Raid, PvP                 │
│  • Context-specific resource allocation                          │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│              LAYER 3: COORDINATION FRAMEWORK                     │
│  ServerCoordinator → ZoneCoordinator → GroupCoordinator         │
│  • Threat, Interrupt, CC, Formation, Cooldown coordinators      │
│  • Hierarchical: calculations shared, assignments distributed   │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│              LAYER 4: DECISION ENGINE (Per-Bot)                  │
│  CombatDecisionEngine                                           │
│  • Priority cascade: Survival → Coordination → Rotation         │
│  • Context-aware decision making                                │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│              LAYER 5: EXECUTION FRAMEWORK                        │
│  ClassAI + ICombatPlugin system                                  │
│  • Plugin-based ability execution                               │
│  • Spell validation via SpellValidation_WoW112.h                │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│              LAYER 6: DATA-DRIVEN CONFIGURATION                  │
│  YAML configs: thresholds, priorities, encounters               │
│  • Hot-reloadable without server restart                        │
│  • Per-context tuning                                           │
└─────────────────────────────────────────────────────────────────┘
```

---

## 5. Layer 1: Event Infrastructure

### 5.1 TrinityCore Hook Extensions

**Add to existing PlayerBotHooks.h:**

```cpp
// ========================================================================
// COMBAT HOOKS (NEW - Add to PlayerBotHooks.h)
// ========================================================================

// Called from Unit::DealDamage - CRITICAL for combat response
static inline std::function<void(Unit* victim, Unit* attacker, 
    uint32 damage, SpellInfo const* spell)> OnDamageTaken = nullptr;

// Called from Spell::Prepare - CRITICAL for interrupt coordination
static inline std::function<void(Unit* caster, SpellInfo const* spell,
    Unit* target)> OnSpellCastStart = nullptr;

// Called from Unit::InterruptSpell
static inline std::function<void(Unit* caster, Unit* interrupter,
    uint32 spellId)> OnSpellInterrupted = nullptr;

// Called from ThreatManager - CRITICAL for tank coordination
static inline std::function<void(Unit* hostile, Unit* target,
    float oldThreat, float newThreat)> OnThreatChanged = nullptr;

// Called from Unit::_ApplyAura - For CC tracking
static inline std::function<void(Unit* target, Aura const* aura,
    Unit* caster)> OnAuraApplied = nullptr;

// Called from InstanceScript::SetBossState
static inline std::function<void(uint32 bossId, EncounterState state,
    Map* map)> OnEncounterStateChanged = nullptr;
```

### 5.2 CombatEventRouter Interface

```cpp
// File: Combat/Interfaces/ICombatEventRouter.h

namespace Playerbot::Combat
{

class ICombatEventSubscriber
{
public:
    virtual ~ICombatEventSubscriber() = default;
    virtual bool OnCombatEvent(CombatEvent const& event) = 0;
    virtual uint64 GetSubscribedEventMask() const = 0;
    virtual uint8 GetSubscriberPriority() const { return 100; }
};

class ICombatEventRouter
{
public:
    virtual ~ICombatEventRouter() = default;
    
    virtual void Publish(CombatEvent&& event) = 0;
    virtual void SubscribeZone(uint32 zoneId, ICombatEventSubscriber* sub) = 0;
    virtual void SubscribeGroup(ObjectGuid groupGuid, ICombatEventSubscriber* sub) = 0;
    virtual void SubscribeBot(ObjectGuid botGuid, ICombatEventSubscriber* sub) = 0;
    virtual void Unsubscribe(ICombatEventSubscriber* sub) = 0;
    virtual uint32 ProcessPendingEvents(uint32 maxEvents) = 0;
};

} // namespace
```

---

## 6. Layer 2: Combat Context System

### 6.1 ICombatContext Interface

```cpp
// File: Combat/Interfaces/ICombatContext.h

namespace Playerbot::Combat
{

enum class CombatContextType : uint8
{
    SOLO = 0,
    GROUP_OPENWORLD,
    DUNGEON_TRASH,
    DUNGEON_BOSS,
    RAID_TRASH,
    RAID_BOSS,
    PVP_BATTLEGROUND,
    PVP_ARENA,
    PVP_WORLD
};

class ICombatContext
{
public:
    virtual ~ICombatContext() = default;
    
    // Identification
    virtual CombatContextType GetType() const = 0;
    virtual std::string GetName() const = 0;
    
    // Update configuration
    virtual uint32 GetBaseUpdateIntervalMs() const = 0;
    virtual uint32 GetMaxEventsPerUpdate() const = 0;
    
    // Coordination requirements
    virtual bool RequiresThreatCoordination() const = 0;
    virtual bool RequiresInterruptCoordination() const = 0;
    virtual bool RequiresCCCoordination() const = 0;
    virtual bool RequiresFormationManagement() const = 0;
    virtual bool RequiresCooldownCoordination() const = 0;
    virtual bool RequiresHealingCoordination() const = 0;
    
    // Behavior modifiers
    virtual float GetCoordinationIntensity() const = 0;  // 0.0-1.0
    virtual float GetAggressionLevel() const = 0;        // 0.0-1.0
    virtual float GetSurvivalPriority() const = 0;       // 0.0-1.0
    virtual bool ShouldTrackEnemyCooldowns() const = 0;  // PvP only
    
    // Resource limits
    virtual uint32 GetMaxThreatTableSize() const = 0;
    virtual uint32 GetMaxTargetEvaluations() const = 0;
};

} // namespace
```

### 6.2 Context Configuration Matrix

| Context | Update | Threat | Interrupt | CC | Formation | Cooldown |
|---------|--------|--------|-----------|-----|-----------|----------|
| Solo | 500ms | ❌ | ❌ | ❌ | ❌ | ❌ |
| Group | 200ms | ⚠️ | ⚠️ | ❌ | ❌ | ❌ |
| Dungeon Trash | 100ms | ✅ | ✅ | ❌ | ❌ | ❌ |
| Dungeon Boss | 50ms | ✅ | ✅ | ✅ | ✅ | ⚠️ |
| Raid Trash | 100ms | ✅ | ✅ | ⚠️ | ✅ | ❌ |
| Raid Boss | 50ms | ✅ | ✅ | ✅ | ✅ | ✅ |
| Battleground | 100ms | ❌ | ✅ | ✅ | ❌ | ✅ |
| Arena | 25ms | ❌ | ✅ | ✅ | ❌ | ✅ |

Legend: ✅ = Required, ⚠️ = Optional, ❌ = Not needed

---

## 7. Layer 3: Coordination Framework

### 7.1 Hierarchical Coordinator Architecture

```
┌────────────────────────────────────────────────────────────────┐
│                   ServerCoordinator                             │
│  • Global bot registry                                         │
│  • Zone allocation                                             │
│  • Server-wide events                                          │
│  • One per server                                              │
└────────────────────────────────────────────────────────────────┘
              │
              │  Manages N zones (active zones only)
              ▼
┌────────────────────────────────────────────────────────────────┐
│                   ZoneCoordinator                               │
│  • Zone-level hostile tracking                                 │
│  • Spawn event aggregation                                     │
│  • Cross-group coordination                                    │
│  • One per active zone with bots                               │
└────────────────────────────────────────────────────────────────┘
              │
              │  Manages N groups in zone
              ▼
┌────────────────────────────────────────────────────────────────┐
│                   GroupCoordinator                              │
│  Uses: RaidOrchestrator for 10+ members                        │
│  Contains:                                                     │
│  ├── ThreatCoordinator (event-driven)                          │
│  ├── InterruptCoordinator (priority queue)                     │
│  ├── CCCoordinator (DR tracking)                               │
│  ├── FormationManager                                          │
│  └── CooldownCoordinator                                       │
└────────────────────────────────────────────────────────────────┘
              │
              │  Issues directives to N bots
              ▼
┌────────────────────────────────────────────────────────────────┐
│                   Bot (CombatDecisionEngine)                    │
│  Receives: Coordinator directives                               │
│  Produces: Actions to execute                                   │
└────────────────────────────────────────────────────────────────┘
```

### 7.2 ICombatCoordinator Interface

```cpp
// File: Combat/Interfaces/ICombatCoordinator.h

namespace Playerbot::Combat
{

enum class CoordinatorType : uint8
{
    THREAT,
    INTERRUPT,
    CROWD_CONTROL,
    FORMATION,
    COOLDOWN,
    HEALING
};

struct CoordinatorDirective
{
    ObjectGuid targetBot;
    DirectiveType type;
    ObjectGuid targetUnit;
    uint32 spellId;
    uint32 priority;
    uint32 expirationTime;
    std::string reason;
};

class ICombatCoordinator : public ICombatEventSubscriber
{
public:
    virtual ~ICombatCoordinator() = default;
    
    // Lifecycle
    virtual void Initialize(Group* group) = 0;
    virtual void Shutdown() = 0;
    virtual void Update(uint32 diff) = 0;
    
    // Identification
    virtual CoordinatorType GetType() const = 0;
    virtual std::string GetName() const = 0;
    
    // Context awareness
    virtual bool IsRequiredForContext(ICombatContext const& ctx) const = 0;
    virtual void OnContextChanged(ICombatContext const& newContext) = 0;
    
    // Directive management
    virtual std::vector<CoordinatorDirective> GetActiveDirectives() const = 0;
    virtual CoordinatorDirective* GetDirectiveForBot(ObjectGuid botGuid) = 0;
    
    // Statistics
    struct CoordinatorStats
    {
        uint64 directivesIssued;
        uint64 directivesExecuted;
        uint64 directivesFailed;
        uint32 avgResponseTimeMs;
    };
    virtual CoordinatorStats GetStats() const = 0;
};

} // namespace
```

### 7.3 ThreatCoordinator (Event-Driven Redesign)

```cpp
// File: Combat/Coordinators/ThreatCoordinator.h

namespace Playerbot::Combat
{

/**
 * @class ThreatCoordinator
 * @brief Event-driven threat management for groups
 *
 * OLD DESIGN (O(b×t)):
 * - Every update: iterate all bots × all targets
 * - Calculate threat for each combination
 * - 40 bots × 20 targets = 800 calculations per update
 *
 * NEW DESIGN (Event-driven):
 * - Only recalculate when threat event received
 * - Maintain threat matrix incrementally
 * - O(1) per event, O(n) per update for assignment changes
 */
class ThreatCoordinator : public ICombatCoordinator
{
public:
    ThreatCoordinator();
    
    // ICombatCoordinator
    void Initialize(Group* group) override;
    void Shutdown() override;
    void Update(uint32 diff) override;
    CoordinatorType GetType() const override { return CoordinatorType::THREAT; }
    
    // ICombatEventSubscriber
    bool OnCombatEvent(CombatEvent const& event) override;
    uint64 GetSubscribedEventMask() const override;
    
    // Threat-specific
    struct ThreatAssignment
    {
        ObjectGuid botGuid;
        ObjectGuid targetGuid;
        ThreatRole role;           // TANK_PRIMARY, TANK_SECONDARY, DPS, HEALER
        float currentThreatPct;
        float targetThreatPct;     // Where we want to be
        bool needsTaunt;
        bool needsThreatDrop;
    };
    
    ThreatAssignment const* GetAssignment(ObjectGuid botGuid) const;
    void RequestTaunt(ObjectGuid botGuid, ObjectGuid targetGuid);
    void RequestThreatDrop(ObjectGuid botGuid);
    
private:
    // Event handlers
    void OnThreatChanged(ObjectGuid hostile, ObjectGuid target, float newThreat);
    void OnAggroGained(ObjectGuid hostile, ObjectGuid target);
    void OnTankDeath(ObjectGuid tankGuid);
    
    // Threat matrix (sparse, only tracked targets)
    struct ThreatEntry
    {
        ObjectGuid targetGuid;
        float threat;
        uint32 lastUpdateTime;
    };
    
    // Bot -> Target -> Threat
    // Using flat_map for cache-friendly iteration
    std::unordered_map<ObjectGuid, 
        boost::container::flat_map<ObjectGuid, ThreatEntry>> _threatMatrix;
    
    // Current assignments (recalculated only when needed)
    std::unordered_map<ObjectGuid, ThreatAssignment> _assignments;
    bool _assignmentsDirty = false;
    
    // Tank tracking
    std::vector<ObjectGuid> _tankOrder;  // Primary, secondary, tertiary
    
    void RecalculateAssignments();
};

} // namespace
```

### 7.4 InterruptCoordinator (Priority Queue Redesign)

```cpp
// File: Combat/Coordinators/InterruptCoordinator.h

namespace Playerbot::Combat
{

/**
 * @class InterruptCoordinator
 * @brief Priority-queue based interrupt coordination
 *
 * OLD DESIGN (O(a×b log b)):
 * - For each active cast, sort all available interrupters
 * - 5 casts × 40 bots × log(40) = ~1000 comparisons
 *
 * NEW DESIGN (Priority Queue):
 * - Maintain sorted interrupter availability
 * - O(log n) assignment per cast
 * - Pre-computed interrupt capabilities
 */
class InterruptCoordinator : public ICombatCoordinator
{
public:
    // ICombatCoordinator
    void Initialize(Group* group) override;
    void Update(uint32 diff) override;
    
    // ICombatEventSubscriber
    bool OnCombatEvent(CombatEvent const& event) override;
    uint64 GetSubscribedEventMask() const override;
    
    // Interrupt-specific
    struct InterruptAssignment
    {
        ObjectGuid botGuid;
        ObjectGuid casterGuid;
        uint32 spellId;
        uint32 interruptSpellId;
        uint32 executeAtTime;      // When to cast interrupt
        InterruptPriority priority;
    };
    
    InterruptAssignment const* GetAssignment(ObjectGuid botGuid) const;
    
private:
    // Event handlers
    void OnEnemyCastStart(ObjectGuid caster, uint32 spellId, uint32 castTime);
    void OnInterruptSuccessful(ObjectGuid interrupter, ObjectGuid caster);
    void OnInterruptFailed(ObjectGuid interrupter, ObjectGuid caster);
    
    // Interrupter availability (priority queue)
    struct InterrupterInfo
    {
        ObjectGuid botGuid;
        uint32 interruptSpellId;
        uint32 cooldownEndTime;
        float distanceToCaster;
        uint8 priority;  // Class-based priority (rogues > warriors)
        
        bool operator<(InterrupterInfo const& other) const
        {
            if (cooldownEndTime != other.cooldownEndTime)
                return cooldownEndTime > other.cooldownEndTime;  // Earlier = higher priority
            return priority < other.priority;
        }
    };
    
    std::priority_queue<InterrupterInfo> _availableInterrupters;
    
    // Active casts requiring interrupt
    struct ActiveCast
    {
        ObjectGuid casterGuid;
        uint32 spellId;
        uint32 castEndTime;
        InterruptPriority priority;
        ObjectGuid assignedInterrupter;
    };
    
    std::vector<ActiveCast> _activeCasts;
    
    // Pre-computed interrupt capabilities per bot
    std::unordered_map<ObjectGuid, std::vector<uint32>> _botInterruptSpells;
    
    void AssignInterrupter(ActiveCast& cast);
    void RefreshInterrupterQueue();
};

} // namespace
```

---

## 8. Layer 4: Decision Engine

### 8.1 CombatDecisionEngine Interface

```cpp
// File: Combat/Interfaces/ICombatDecisionEngine.h

namespace Playerbot::Combat
{

enum class DecisionType : uint8
{
    NONE,
    CAST_SPELL,
    USE_ITEM,
    MOVE_TO_POSITION,
    MOVE_TO_TARGET,
    STOP_CASTING,
    WAIT
};

struct CombatDecision
{
    DecisionType type = DecisionType::NONE;
    uint32 spellId = 0;
    uint32 itemId = 0;
    ObjectGuid targetGuid;
    Position targetPosition;
    uint8 priority = 0;
    std::string reason;
    
    bool IsValid() const { return type != DecisionType::NONE; }
};

class ICombatDecisionEngine
{
public:
    virtual ~ICombatDecisionEngine() = default;
    
    /**
     * @brief Make a combat decision
     * @param context Current combat context
     * @param diff Time since last decision
     * @return Decision to execute
     *
     * Priority cascade:
     * 1. Survival (health critical, CC break needed)
     * 2. Coordinator directives (interrupt, taunt, CC)
     * 3. Role rotation (tank: threat, healer: heal, dps: damage)
     * 4. Utility (buffs, positioning)
     */
    virtual CombatDecision Decide(ICombatContext const& context, uint32 diff) = 0;
    
    /**
     * @brief Register coordinator for directive reception
     */
    virtual void RegisterCoordinator(ICombatCoordinator* coordinator) = 0;
    
    /**
     * @brief Set combat context
     */
    virtual void SetContext(ICombatContext const& context) = 0;
};

} // namespace
```

### 8.2 Decision Priority Cascade

```cpp
// File: Combat/Decision/CombatDecisionEngine.cpp

CombatDecision CombatDecisionEngine::Decide(ICombatContext const& context, uint32 diff)
{
    // PRIORITY 1: Survival (always checked)
    if (auto decision = CheckSurvival())
        return *decision;
    
    // PRIORITY 2: Coordinator directives
    for (auto* coordinator : _coordinators)
    {
        if (auto* directive = coordinator->GetDirectiveForBot(_botGuid))
        {
            if (auto decision = ExecuteDirective(*directive))
                return *decision;
        }
    }
    
    // PRIORITY 3: Role-specific rotation
    if (auto decision = _roleHandler->GetNextAction(context))
        return *decision;
    
    // PRIORITY 4: Utility actions
    if (auto decision = CheckUtility())
        return *decision;
    
    // No action needed
    return CombatDecision{};
}

std::optional<CombatDecision> CombatDecisionEngine::CheckSurvival()
{
    float healthPct = _bot->GetHealthPct();
    
    // CRITICAL: Health below 20%
    if (healthPct < 20.0f)
    {
        // Check for defensive cooldowns
        if (auto defensiveSpell = _classAI->GetBestDefensive())
        {
            return CombatDecision{
                .type = DecisionType::CAST_SPELL,
                .spellId = defensiveSpell,
                .targetGuid = _bot->GetGUID(),
                .priority = 255,
                .reason = "SURVIVAL: Health critical"
            };
        }
        
        // Check for health potion
        if (auto potionId = GetHealthPotion())
        {
            return CombatDecision{
                .type = DecisionType::USE_ITEM,
                .itemId = potionId,
                .priority = 255,
                .reason = "SURVIVAL: Using health potion"
            };
        }
    }
    
    // Check for dangerous debuffs that need removal
    if (auto debuffToRemove = GetDangerousDebuff())
    {
        if (auto removeSpell = _classAI->GetDebuffRemoval(debuffToRemove))
        {
            return CombatDecision{
                .type = DecisionType::CAST_SPELL,
                .spellId = removeSpell,
                .targetGuid = _bot->GetGUID(),
                .priority = 200,
                .reason = "SURVIVAL: Removing dangerous debuff"
            };
        }
    }
    
    return std::nullopt;
}
```

---

## 9. Layer 5: Execution Framework

### 9.1 ICombatPlugin Interface

```cpp
// File: Combat/Interfaces/ICombatPlugin.h

namespace Playerbot::Combat
{

enum class PluginCategory : uint8
{
    SPELL,          // Damage/healing spells
    COOLDOWN,       // Major cooldowns
    DEFENSIVE,      // Defensive abilities
    UTILITY,        // Buffs, movement abilities
    INTERRUPT,      // Interrupt abilities
    CC              // Crowd control
};

struct PluginContext
{
    Player* bot;
    Unit* target;
    ICombatContext const* combatContext;
    float healthPct;
    float resourcePct;
    uint32 enemyCount;
    bool isMoving;
};

struct ExecutionResult
{
    bool success;
    uint32 spellId;
    uint32 globalCooldownMs;
    std::string failureReason;
};

class ICombatPlugin
{
public:
    virtual ~ICombatPlugin() = default;
    
    // Identification
    virtual std::string GetPluginId() const = 0;
    virtual PluginCategory GetCategory() const = 0;
    virtual int GetPriority() const = 0;
    
    // Execution
    virtual bool CanExecute(PluginContext const& ctx) const = 0;
    virtual ExecutionResult Execute(PluginContext& ctx) = 0;
    
    // Configuration
    virtual void LoadFromConfig(YAML::Node const& config) = 0;
};

/**
 * @class SpellPlugin
 * @brief Generic spell execution plugin
 *
 * Configured via YAML, no code changes needed for new spells.
 */
class SpellPlugin : public ICombatPlugin
{
public:
    SpellPlugin(uint32 spellId, SpellPluginConfig const& config);
    
    bool CanExecute(PluginContext const& ctx) const override
    {
        // Check cooldown
        if (!ctx.bot->GetSpellHistory()->IsReady(_spellId))
            return false;
        
        // Check resources
        if (ctx.resourcePct < _minResourcePct)
            return false;
        
        // Check conditions from config
        for (auto const& condition : _conditions)
        {
            if (!condition.Evaluate(ctx))
                return false;
        }
        
        return true;
    }
    
private:
    uint32 _spellId;
    float _minResourcePct;
    std::vector<SpellCondition> _conditions;
};

} // namespace
```

### 9.2 SpecPluginRegistry

```cpp
// File: Combat/Execution/SpecPluginRegistry.h

namespace Playerbot::Combat
{

/**
 * @class SpecPluginRegistry
 * @brief Manages plugins for a specialization
 *
 * Loads plugins from YAML configuration.
 * Provides priority-sorted plugin access.
 */
class SpecPluginRegistry
{
public:
    SpecPluginRegistry(uint8 classId, uint8 specId);
    
    /**
     * @brief Load plugins from YAML config
     * @param configPath Path to spec YAML file
     */
    void LoadFromYaml(std::string const& configPath);
    
    /**
     * @brief Get best executable plugin
     * @param ctx Current plugin context
     * @return Plugin to execute, or nullptr
     */
    ICombatPlugin* GetBestPlugin(PluginContext const& ctx) const;
    
    /**
     * @brief Get best plugin by category
     */
    ICombatPlugin* GetBestPluginByCategory(
        PluginCategory category, PluginContext const& ctx) const;
    
    /**
     * @brief Register a plugin
     */
    void RegisterPlugin(std::unique_ptr<ICombatPlugin> plugin);
    
private:
    uint8 _classId;
    uint8 _specId;
    
    // Plugins sorted by priority (highest first)
    std::vector<std::unique_ptr<ICombatPlugin>> _plugins;
    
    // Category index for fast lookup
    std::unordered_map<PluginCategory, 
        std::vector<ICombatPlugin*>> _categoryIndex;
};

} // namespace
```

---

## 10. Layer 6: Data-Driven Configuration

### 10.1 Configuration File Structure

```yaml
# config/combat/thresholds.yaml
combat_thresholds:
  global:
    emergency_health_pct: 20.0
    low_health_pct: 50.0
    low_mana_pct: 20.0
    threat_warning_pct: 90.0
    threat_critical_pct: 110.0
    
  contexts:
    solo:
      update_interval_ms: 500
      max_events_per_update: 20
      
    raid_boss:
      update_interval_ms: 50
      max_events_per_update: 100
      threat_balance_tolerance: 5.0
      
    arena:
      update_interval_ms: 25
      max_events_per_update: 50
      burst_window_ms: 3000
```

```yaml
# config/combat/classes/warrior/arms.yaml
class: warrior
spec: arms
role: melee_dps

plugins:
  - id: mortal_strike
    type: spell
    spell_id: 12294
    priority: 100
    category: spell
    conditions:
      - type: cooldown_ready
      - type: in_melee_range
      - type: target_above_health_pct
        value: 20
        
  - id: execute
    type: spell
    spell_id: 163201
    priority: 110  # Higher than MS when available
    category: spell
    conditions:
      - type: cooldown_ready
      - type: target_below_health_pct
        value: 20
        
  - id: bladestorm
    type: spell
    spell_id: 227847
    priority: 80
    category: cooldown
    conditions:
      - type: cooldown_ready
      - type: enemy_count_above
        value: 3
      - type: not_in_context
        contexts: [arena]  # Don't bladestorm in arena
```

```yaml
# config/combat/encounters/mythicplus_interrupts.yaml
interrupt_priorities:
  critical:
    - spell_id: 387564
      name: "Arcane Expulsion"
      dungeons: ["The Nokhud Offensive"]
      notes: "Must interrupt - kills tank"
      
    - spell_id: 384365
      name: "Tectonic Upheaval"  
      dungeons: ["Halls of Infusion"]
      
  high:
    - spell_id: 386634
      name: "Scorching Strike"
      dungeons: ["The Vortex Pinnacle"]
      
  normal:
    - spell_id: 384523
      name: "Frostbolt"
      default_priority: true
```

---

## 11. Implementation Roadmap

### Phase 1: Foundation (Weeks 1-3)

| Week | Task | Deliverable |
|------|------|-------------|
| 1 | Spell deduplication (Claude Code) | SpellValidation centralized |
| 1 | ClassRoleResolver | 28 switches → 1 class |
| 1 | ICombatContext interface | Base + 5 implementations |
| 2 | TrinityCore hook extensions | 10 combat hooks added |
| 2 | CombatEventRouter | Event routing infrastructure |
| 3 | CombatContextManager | Auto context detection |

### Phase 2: Coordination (Weeks 4-6)

| Week | Task | Deliverable |
|------|------|-------------|
| 4 | ThreatCoordinator (event-driven) | O(1) per event |
| 4 | ServerCoordinator + ZoneCoordinator | Hierarchical structure |
| 5 | InterruptCoordinator (priority queue) | O(log n) assignment |
| 5 | GroupCoordinator integration | Full coordination layer |
| 6 | CCCoordinator + FormationManager | PvP + Raid support |

### Phase 3: Decision & Execution (Weeks 7-9)

| Week | Task | Deliverable |
|------|------|-------------|
| 7 | CombatDecisionEngine | Priority cascade |
| 7 | ICombatPlugin interface | Plugin framework |
| 8 | SpecPluginRegistry | YAML loading |
| 8-9 | Plugin migration (all 40 specs) | Full spec coverage |

### Phase 4: Configuration & Testing (Weeks 10-12)

| Week | Task | Deliverable |
|------|------|-------------|
| 10 | CombatConfigManager | YAML hot-reload |
| 10 | Threshold externalization | All magic numbers in YAML |
| 11 | Load testing (5000 bots) | Performance validation |
| 11 | Context-specific tuning | Per-context YAML |
| 12 | Documentation + Handover | Complete system docs |

---

## 12. Critical Success Factors

### 12.1 Performance Validation Checkpoints

| Checkpoint | Test | Pass Criteria |
|------------|------|---------------|
| Week 3 | 1000 bots, solo | <0.1% CPU per bot |
| Week 6 | 200 groups, dungeon | <5ms coordination overhead |
| Week 9 | 10 raids (400 bots) | <50ms total coordination |
| Week 12 | 5000 bots mixed | Server stable, <30% total CPU |

### 12.2 Quality Gates

- [ ] All interfaces have unit tests
- [ ] All coordinators have integration tests
- [ ] All contexts validated with load testing
- [ ] All YAML configs validated with schema
- [ ] Performance budgets enforced at runtime
- [ ] Documentation complete for each layer

---

## Appendix A: File Structure

```
src/modules/Playerbot/AI/Combat/
├── Interfaces/
│   ├── ICombatContext.h
│   ├── ICombatCoordinator.h
│   ├── ICombatDecisionEngine.h
│   ├── ICombatEventRouter.h
│   └── ICombatPlugin.h
├── Contexts/
│   ├── SoloContext.h
│   ├── GroupContext.h
│   ├── DungeonContext.h
│   ├── RaidContext.h
│   └── PvPContext.h
├── Coordinators/
│   ├── ServerCoordinator.h
│   ├── ZoneCoordinator.h
│   ├── GroupCoordinator.h
│   ├── ThreatCoordinator.h
│   ├── InterruptCoordinator.h
│   ├── CCCoordinator.h
│   └── FormationCoordinator.h
├── Decision/
│   ├── CombatDecisionEngine.h
│   └── RoleHandlers/
│       ├── TankRoleHandler.h
│       ├── HealerRoleHandler.h
│       └── DpsRoleHandler.h
├── Execution/
│   ├── SpecPluginRegistry.h
│   └── Plugins/
│       ├── SpellPlugin.h
│       ├── CooldownPlugin.h
│       └── DefensivePlugin.h
├── Config/
│   ├── CombatConfigManager.h
│   └── ConfigSchema.h
└── Events/
    ├── CombatEventTypes.h
    └── CombatEventRouter.h

config/combat/
├── thresholds.yaml
├── contexts/
│   ├── solo.yaml
│   ├── dungeon.yaml
│   ├── raid.yaml
│   └── pvp.yaml
├── classes/
│   ├── warrior/
│   │   ├── arms.yaml
│   │   ├── fury.yaml
│   │   └── protection.yaml
│   └── ... (all classes)
└── encounters/
    ├── mythicplus_interrupts.yaml
    └── raid_mechanics.yaml
```

---

**Document Status**: APPROVED FOR IMPLEMENTATION  
**Next Step**: Phase 1 Week 1 - Begin with ClassRoleResolver + Spell Deduplication  
**Owner**: Architecture Team  
**Review Date**: February 2, 2026
