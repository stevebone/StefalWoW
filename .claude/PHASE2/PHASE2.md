# Phase 2: Bot AI Core System - Detailed Project Plan

## Executive Summary
**Duration:** 8 Weeks (Weeks 7-14)  
**Objective:** Implement the core AI framework for bot decision-making, movement, and combat  
**Dependencies:** Phase 1 completion (Session Management)  
**Success Metric:** Functional bots that can move, fight, and interact autonomously  

## Phase 2 Architecture Overview

```
Bot AI Core System
â”œâ”€â”€ Session Layer (Week 7-8)
â”‚   â”œâ”€â”€ Virtual WorldSession Implementation
â”‚   â”œâ”€â”€ Packet Queue Management
â”‚   â””â”€â”€ Session Pool & Lifecycle
â”œâ”€â”€ AI Framework (Week 9-10)
â”‚   â”œâ”€â”€ Base AI Engine
â”‚   â”œâ”€â”€ Strategy Pattern Implementation
â”‚   â”œâ”€â”€ Action/Trigger System
â”‚   â””â”€â”€ Value/Context System
â”œâ”€â”€ Movement System (Week 11)
â”‚   â”œâ”€â”€ Pathfinding Integration
â”‚   â”œâ”€â”€ Following Behavior
â”‚   â”œâ”€â”€ Random Movement
â”‚   â””â”€â”€ Formation Movement
â”œâ”€â”€ Combat System (Week 12-13)
â”‚   â”œâ”€â”€ Threat Management
â”‚   â”œâ”€â”€ Target Selection
â”‚   â”œâ”€â”€ Spell Rotation
â”‚   â””â”€â”€ Positioning Logic
â””â”€â”€ Class-Specific AI (Week 14)
    â”œâ”€â”€ Role-Based Strategies
    â”œâ”€â”€ Class Rotation Logic
    â””â”€â”€ Specialization Support
```

---

## WEEK 7-8: Bot Session Management

### Objective
Create socketless virtual sessions that allow bots to exist in the game world without network connections.

### Day 1-2: BotSession Implementation
**Time:** 16 hours  
**Location:** `src/modules/Playerbot/Session/`

#### Task 1.1: Create BotSession Header
**File:** `src/modules/Playerbot/Session/BotSession.h`

```cpp
/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 */

#ifndef BOT_SESSION_H
#define BOT_SESSION_H

#include "WorldSession.h"
#include <queue>
#include <memory>
#include <atomic>

class BotAI;
class Player;

class TC_GAME_API BotSession : public WorldSession
{
public:
    explicit BotSession(uint32 accountId);
    virtual ~BotSession();

    // Override WorldSession network methods
    void SendPacket(WorldPacket const* packet, bool forced = false) override;
    void QueuePacket(WorldPacket&& packet) override;
    bool IsConnectionIdle() const override { return false; }
    
    // Bot-specific methods
    void ProcessPacketQueue();
    void SimulatePacketReceive(WorldPacket const& packet);
    void SetBot(Player* bot) { _bot = bot; }
    Player* GetBot() const { return _bot; }
    BotAI* GetAI() const { return _ai.get(); }
    
    // Session state
    bool IsBot() const { return true; }
    void Update(uint32 diff) override;
    
private:
    void HandleBotPacket(WorldPacket const& packet);
    void ProcessMovementPackets();
    void ProcessCombatPackets();
    
    Player* _bot;
    std::unique_ptr<BotAI> _ai;
    std::queue<WorldPacket> _outgoingPackets;
    std::queue<WorldPacket> _incomingPackets;
    std::atomic<bool> _processingPackets;
    uint32 _lastUpdateTime;
};
```

#### Task 1.2: Implement BotSession
**File:** `src/modules/Playerbot/Session/BotSession.cpp`

Key Implementation Points:
- Override SendPacket to queue packets internally
- Process packets without network I/O
- Maintain packet queues for AI processing
- Handle session lifecycle without sockets

#### Task 1.3: Create BotSessionMgr
**File:** `src/modules/Playerbot/Session/BotSessionMgr.h`

```cpp
class TC_GAME_API BotSessionMgr
{
public:
    static BotSessionMgr* instance();
    
    bool Initialize();
    void Shutdown();
    void Update(uint32 diff);
    
    // Session management
    BotSession* CreateBotSession(uint32 accountId);
    void RemoveBotSession(uint32 accountId);
    BotSession* GetBotSession(uint32 accountId) const;
    
    // Batch operations
    void UpdateAllSessions(uint32 diff);
    uint32 GetActiveSessionCount() const;
    
private:
    std::unordered_map<uint32, std::unique_ptr<BotSession>> _sessions;
    mutable std::mutex _sessionMutex;
    uint32 _maxConcurrentSessions;
    uint32 _sessionUpdateInterval;
};
```

### Day 3-4: Session Pool & Threading
**Time:** 16 hours

#### Task 2.1: Implement Session Pool
- Thread-safe session allocation
- Session recycling mechanism  
- Memory management optimization
- Connection limit enforcement

#### Task 2.2: Multi-threaded Update System
- Worker thread pool for AI updates
- Load balancing across threads
- Lock-free queue implementation
- Performance monitoring

### Day 5-6: Integration Testing
**Time:** 16 hours

#### Task 3.1: Session Lifecycle Tests
- Test session creation/destruction
- Validate packet processing
- Memory leak detection
- Thread safety verification

#### Task 3.2: Performance Benchmarks
- Measure session overhead
- Test with 100+ concurrent sessions
- Profile CPU/Memory usage
- Optimize bottlenecks

---

## WEEK 9-10: AI Framework Foundation

### Objective
Implement the core AI decision-making framework using Strategy, Action, and Trigger patterns.

### Day 7-8: Base AI Architecture
**Time:** 16 hours  
**Location:** `src/modules/Playerbot/AI/`

#### Task 4.1: Create BotAI Base Class
**File:** `src/modules/Playerbot/AI/BotAI.h`

```cpp
class TC_GAME_API BotAI
{
public:
    explicit BotAI(Player* bot);
    virtual ~BotAI();
    
    // Core AI loop
    virtual void Update(uint32 diff);
    virtual void Reset();
    
    // Strategy management
    void SetStrategy(std::string const& name);
    void AddStrategy(std::string const& name);
    void RemoveStrategy(std::string const& name);
    
    // Action execution
    bool ExecuteAction(std::string const& name);
    
    // Context & Values
    float GetValue(std::string const& name);
    void SetValue(std::string const& name, float value);
    
    // Events
    virtual void OnCombatStart(Unit* enemy);
    virtual void OnCombatEnd();
    virtual void OnDeath();
    virtual void OnRevive();
    
protected:
    Player* _bot;
    std::vector<std::unique_ptr<Strategy>> _strategies;
    std::unique_ptr<ActionExecutor> _actionExecutor;
    std::unique_ptr<ValueContext> _valueContext;
    std::unique_ptr<TriggerContext> _triggerContext;
    
private:
    void UpdateStrategies();
    void ProcessTriggers();
    Action* SelectNextAction();
};
```

#### Task 4.2: Strategy Pattern Implementation
**File:** `src/modules/Playerbot/AI/Strategy/Strategy.h`

```cpp
class TC_GAME_API Strategy
{
public:
    explicit Strategy(BotAI* ai) : _ai(ai) {}
    virtual ~Strategy() = default;
    
    virtual std::string GetName() const = 0;
    virtual float GetPriority() const { return 1.0f; }
    
    // Trigger evaluation
    virtual void InitTriggers(std::vector<std::unique_ptr<Trigger>>& triggers) = 0;
    
    // Action provision
    virtual NextAction* GetNextAction(Trigger* trigger) = 0;
    
protected:
    BotAI* _ai;
};
```

### Day 9-10: Action System
**Time:** 16 hours

#### Task 5.1: Action Base Implementation
**File:** `src/modules/Playerbot/AI/Actions/Action.h`

```cpp
class TC_GAME_API Action
{
public:
    explicit Action(BotAI* ai) : _ai(ai) {}
    virtual ~Action() = default;
    
    virtual bool Execute(Event const& event) = 0;
    virtual bool IsUseful() { return true; }
    virtual bool IsPossible() { return true; }
    
    virtual std::string GetName() const = 0;
    virtual float GetPriority() const { return 1.0f; }
    
    // Cost calculation for decision making
    virtual float GetCost() const { return 0.0f; }
    
protected:
    BotAI* _ai;
    Player* GetBot() const;
};
```

#### Task 5.2: Core Actions
Create fundamental actions:
- `MoveAction` - Basic movement
- `AttackAction` - Melee attack
- `CastSpellAction` - Spell casting
- `UseItemAction` - Item usage
- `InteractAction` - NPC/Object interaction

### Day 11-12: Trigger System
**Time:** 16 hours

#### Task 6.1: Trigger Implementation
**File:** `src/modules/Playerbot/AI/Triggers/Trigger.h`

```cpp
class TC_GAME_API Trigger
{
public:
    explicit Trigger(BotAI* ai) : _ai(ai) {}
    virtual ~Trigger() = default;
    
    virtual bool IsActive() = 0;
    virtual std::string GetName() const = 0;
    
    void Update() { _needsUpdate = true; }
    void Reset() { _needsUpdate = false; }
    
protected:
    BotAI* _ai;
    bool _needsUpdate = true;
};
```

#### Task 6.2: Core Triggers
- `HealthTrigger` - Low health detection
- `ManaTrigger` - Low mana detection
- `EnemyTrigger` - Enemy in range
- `LootTrigger` - Lootable corpse nearby
- `FollowTrigger` - Master too far

### Day 13-14: Value Context System
**Time:** 16 hours

#### Task 7.1: Value Provider System
**File:** `src/modules/Playerbot/AI/Values/Value.h`

```cpp
template<typename T>
class TC_GAME_API Value
{
public:
    explicit Value(BotAI* ai) : _ai(ai) {}
    virtual ~Value() = default;
    
    virtual T Get() = 0;
    virtual void Set(T value) { _cachedValue = value; }
    virtual void Update() { _lastUpdate = getMSTime(); }
    
    bool NeedsUpdate() const 
    { 
        return getMSTime() - _lastUpdate > _updateInterval; 
    }
    
protected:
    BotAI* _ai;
    T _cachedValue;
    uint32 _lastUpdate = 0;
    uint32 _updateInterval = 1000; // ms
};
```

#### Task 7.2: Core Values
- `HealthValue` - Current health percentage
- `ManaValue` - Current mana percentage
- `TargetValue` - Current target
- `ThreatValue` - Threat on target
- `PositionValue` - Bot position

---

## WEEK 11: Movement System

### Objective
Implement intelligent movement behaviors including pathfinding, following, and formations.

### Day 15-16: Pathfinding Integration
**Time:** 16 hours  
**Location:** `src/modules/Playerbot/AI/Movement/`

#### Task 8.1: Movement Manager
**File:** `src/modules/Playerbot/AI/Movement/MovementManager.h`

```cpp
class TC_GAME_API MovementManager
{
public:
    explicit MovementManager(Player* bot);
    
    // Movement commands
    bool MoveTo(Position const& pos, bool generatePath = true);
    bool MoveTo(Unit* target, float distance = 0.0f);
    bool Follow(Unit* target, float distance = 2.0f, float angle = 0.0f);
    void Stop();
    
    // Formation movement
    void SetFormation(Formation* formation);
    void UpdateFormationPosition();
    
    // Movement state
    bool IsMoving() const;
    float GetDistanceToDestination() const;
    
private:
    void GeneratePath(Position const& dest);
    void SendMovementPacket();
    
    Player* _bot;
    Movement::MoveSplineInit* _splineInit;
    std::vector<G3D::Vector3> _path;
    Position _destination;
    Unit* _followTarget;
    float _followDistance;
    float _followAngle;
};
```

#### Task 8.2: Movement Generators
- `RandomMovementGenerator` - Wander behavior
- `FollowMovementGenerator` - Follow target
- `FormationMovementGenerator` - Maintain formation
- `FleeMovementGenerator` - Run away behavior

### Day 17: Formation System
**Time:** 8 hours

#### Task 9.1: Formation Templates
```cpp
class Formation
{
public:
    virtual Position GetPosition(uint8 index, Position const& anchor) = 0;
    virtual uint8 GetMaxMembers() const = 0;
    virtual std::string GetName() const = 0;
};

class ArrowFormation : public Formation { /* ... */ };
class CircleFormation : public Formation { /* ... */ };
class LineFormation : public Formation { /* ... */ };
```

### Day 18: Movement Validation
**Time:** 8 hours

#### Task 10.1: Collision Detection
- Line of sight checks
- Obstacle avoidance
- Water/cliff detection
- Indoor/outdoor validation

---

## WEEK 12-13: Combat System

### Objective
Implement intelligent combat behaviors including threat management, targeting, and spell rotation.

### Day 19-20: Threat & Targeting
**Time:** 16 hours  
**Location:** `src/modules/Playerbot/AI/Combat/`

#### Task 11.1: Threat Calculator
**File:** `src/modules/Playerbot/AI/Combat/ThreatManager.h`

```cpp
class TC_GAME_API BotThreatManager
{
public:
    explicit BotThreatManager(Player* bot);
    
    void Update();
    void AddThreat(Unit* enemy, float threat);
    void ModifyThreat(Unit* enemy, float factor);
    void ClearThreat(Unit* enemy);
    
    Unit* GetHighestThreatTarget() const;
    float GetThreat(Unit* enemy) const;
    bool IsTanking() const;
    
private:
    struct ThreatInfo
    {
        Unit* unit;
        float threat;
        uint32 lastUpdate;
    };
    
    Player* _bot;
    std::vector<ThreatInfo> _threats;
};
```

#### Task 11.2: Target Selection
```cpp
class TargetSelector
{
public:
    enum Priority
    {
        PRIORITY_LOW_HEALTH,
        PRIORITY_HIGH_THREAT,
        PRIORITY_HEALERS,
        PRIORITY_CASTERS,
        PRIORITY_NEAREST,
        PRIORITY_MARKED
    };
    
    Unit* SelectTarget(std::vector<Unit*> const& enemies, Priority priority);
};
```

### Day 21-22: Spell System
**Time:** 16 hours

#### Task 12.1: Spell Manager
**File:** `src/modules/Playerbot/AI/Combat/SpellManager.h`

```cpp
class TC_GAME_API BotSpellManager
{
public:
    explicit BotSpellManager(Player* bot);
    
    // Spell casting
    bool CastSpell(uint32 spellId, Unit* target = nullptr);
    bool CanCastSpell(uint32 spellId, Unit* target = nullptr) const;
    
    // Spell selection
    uint32 GetBestSpell(SpellCategory category) const;
    std::vector<uint32> GetAvailableSpells(SpellCategory category) const;
    
    // Rotation management
    void SetRotation(std::vector<uint32> const& spells);
    uint32 GetNextRotationSpell();
    
    // Cooldown tracking
    bool IsSpellReady(uint32 spellId) const;
    uint32 GetSpellCooldown(uint32 spellId) const;
    
private:
    Player* _bot;
    std::vector<uint32> _rotation;
    uint32 _rotationIndex;
    std::unordered_map<uint32, uint32> _cooldowns;
};
```

#### Task 12.2: Spell Categories
- Define spell categories (damage, heal, buff, debuff, cc)
- Implement spell priority system
- Create interrupt logic
- Handle global cooldowns

### Day 23-24: Positioning Logic
**Time:** 16 hours

#### Task 13.1: Combat Positioning
```cpp
class CombatPositioning
{
public:
    Position GetOptimalPosition(Player* bot, Unit* target, Role role);
    
    // Role-specific positioning
    Position GetMeleePosition(Unit* target);
    Position GetRangedPosition(Unit* target, float idealRange);
    Position GetHealerPosition(Group* group);
    Position GetTankPosition(Unit* target, Group* group);
    
    // Safety checks
    bool IsSafePosition(Position const& pos);
    bool HasLineOfSight(Position const& from, Position const& to);
};
```

### Day 25-26: Combat State Machine
**Time:** 16 hours

#### Task 14.1: State Implementation
```cpp
enum CombatState
{
    COMBAT_STATE_IDLE,
    COMBAT_STATE_ENGAGING,
    COMBAT_STATE_FIGHTING,
    COMBAT_STATE_KITING,
    COMBAT_STATE_FLEEING,
    COMBAT_STATE_DEAD
};

class CombatStateMachine
{
public:
    void Update(uint32 diff);
    void TransitionTo(CombatState newState);
    CombatState GetCurrentState() const { return _currentState; }
    
private:
    void HandleIdleState();
    void HandleEngagingState();
    void HandleFightingState();
    void HandleKitingState();
    void HandleFleeingState();
    
    CombatState _currentState;
    uint32 _stateTimer;
};
```

---

## WEEK 14: Class-Specific AI

### Objective
Implement specialized AI behaviors for each class, including rotations and role-specific logic.

### Day 27: Warrior & Paladin AI
**Time:** 8 hours  
**Location:** `src/modules/Playerbot/AI/ClassAI/`

#### Task 15.1: WarriorBotAI
```cpp
class WarriorBotAI : public CombatBotAI
{
public:
    void InitializeRotation() override
    {
        // Tank rotation
        if (GetRole() == ROLE_TANK)
        {
            _rotation = {
                SPELL_SHIELD_SLAM,
                SPELL_REVENGE,
                SPELL_DEVASTATE,
                SPELL_THUNDER_CLAP
            };
        }
        // DPS rotation
        else
        {
            _rotation = {
                SPELL_BLOODTHIRST,
                SPELL_WHIRLWIND,
                SPELL_SLAM,
                SPELL_EXECUTE
            };
        }
    }
    
    void HandleStance();
    void ManageRage();
    void UseDefensiveCooldowns();
};
```

#### Task 15.2: PaladinBotAI
- Blessing management
- Aura selection
- Seal switching
- Healing priority

### Day 28: Hunter & Rogue AI
**Time:** 8 hours

#### Task 16.1: HunterBotAI
- Pet management
- Aspect switching
- Trap placement
- Range management

#### Task 16.2: RogueBotAI
- Stealth usage
- Combo point management
- Poison application
- Positioning for backstab

### Day 29: Caster Classes
**Time:** 8 hours

#### Task 17.1: MageBotAI
- Mana management
- Polymorph usage
- Evocation timing
- Gem creation

#### Task 17.2: WarlockBotAI
- Pet selection
- DoT management
- Soul shard usage
- Healthstone creation

### Day 30: Healer Classes
**Time:** 8 hours

#### Task 18.1: PriestBotAI
```cpp
class PriestBotAI : public HealerBotAI
{
public:
    void UpdateHealing() override
    {
        // Priority healing
        if (Unit* target = GetLowestHealthTarget())
        {
            if (target->GetHealthPct() < 30)
                CastSpell(SPELL_FLASH_HEAL, target);
            else if (target->GetHealthPct() < 60)
                CastSpell(SPELL_GREATER_HEAL, target);
            else if (!target->HasAura(SPELL_RENEW))
                CastSpell(SPELL_RENEW, target);
        }
        
        // Group healing
        if (GetGroupAverageHealth() < 70)
            CastSpell(SPELL_PRAYER_OF_HEALING);
    }
    
    void ManageBuffs();
    void HandleShadowForm();
};
```

#### Task 18.2: Other Healers
- `DruidBotAI` - HoT management, Tree form
- `ShamanBotAI` - Totem placement, Chain heal
- `MonkBotAI` - Chi management, Statue placement

---

## Testing & Validation Plan

### Week 14 Day 31-32: Integration Testing
**Time:** 16 hours

#### Test Suite 1: Session Tests
```cpp
TEST(BotSessionTest, CreateDestroy)
{
    auto session = std::make_unique<BotSession>(testAccountId);
    EXPECT_TRUE(session->IsBot());
    EXPECT_FALSE(session->GetBot()); // No bot assigned yet
}

TEST(BotSessionTest, PacketProcessing)
{
    BotSession session(testAccountId);
    WorldPacket packet(CMSG_CAST_SPELL, 4);
    packet << uint32(12345); // spell id
    
    session.SimulatePacketReceive(packet);
    session.ProcessPacketQueue();
    // Verify spell was processed
}
```

#### Test Suite 2: AI Framework Tests
- Strategy switching
- Action execution
- Trigger evaluation
- Value caching

#### Test Suite 3: Combat Tests
- Target selection
- Threat calculation
- Spell rotation
- Positioning

### Performance Benchmarks

#### Benchmark 1: Session Scaling
```cpp
void BenchmarkSessions(benchmark::State& state)
{
    for (auto _ : state)
    {
        std::vector<std::unique_ptr<BotSession>> sessions;
        for (int i = 0; i < state.range(0); ++i)
        {
            sessions.push_back(std::make_unique<BotSession>(1000 + i));
        }
        
        // Update all sessions
        for (auto& session : sessions)
            session->Update(100);
    }
}
BENCHMARK(BenchmarkSessions)->Range(10, 500);
```

#### Benchmark 2: AI Decision Time
- Measure decision latency
- Profile memory usage
- Test with different strategies

---

## Configuration Integration

### New Configuration Sections
Add to `playerbots.conf`:

```ini
###################################################################################################
# AI FRAMEWORK SETTINGS
###################################################################################################

# Playerbot.AI.DecisionInterval
#     Description: How often bots make decisions (ms)
#     Default:     100
#     Range:       50-1000
Playerbot.AI.DecisionInterval = 100

# Playerbot.AI.MaxActionsPerUpdate
#     Description: Maximum actions a bot can execute per update
#     Default:     3
#     Range:       1-10
Playerbot.AI.MaxActionsPerUpdate = 3

# Playerbot.AI.ReactTime
#     Description: Simulated reaction time for bots (ms)
#     Default:     500
#     Range:       100-2000
Playerbot.AI.ReactTime = 500

###################################################################################################
# MOVEMENT SETTINGS
###################################################################################################

# Playerbot.Movement.GeneratePaths
#     Description: Use pathfinding for bot movement
#     Default:     1
Playerbot.Movement.GeneratePaths = 1

# Playerbot.Movement.FormationSpacing
#     Description: Distance between bots in formation (yards)
#     Default:     3.0
#     Range:       1.0-10.0
Playerbot.Movement.FormationSpacing = 3.0

###################################################################################################
# COMBAT SETTINGS
###################################################################################################

# Playerbot.Combat.AttackDelay
#     Description: Delay before engaging combat (ms)
#     Default:     1000
#     Range:       0-5000
Playerbot.Combat.AttackDelay = 1000

# Playerbot.Combat.FleeHealthPercent
#     Description: Health percentage to flee at
#     Default:     20
#     Range:       0-50
Playerbot.Combat.FleeHealthPercent = 20

# Playerbot.Combat.UseConsumables
#     Description: Allow bots to use potions/food
#     Default:     1
Playerbot.Combat.UseConsumables = 1
```

---

## Risk Mitigation

### Technical Risks

1. **Memory Leaks in Sessions**
   - Mitigation: RAII, smart pointers, Valgrind testing
   - Monitoring: Track session memory usage

2. **AI Performance Impact**
   - Mitigation: Async processing, caching, throttling
   - Monitoring: Profile CPU usage per bot

3. **Thread Safety Issues**
   - Mitigation: Minimize shared state, use atomics
   - Monitoring: Thread sanitizer, stress testing

### Design Risks

1. **Overly Complex AI**
   - Mitigation: Start simple, iterate
   - Fallback: Basic attack/follow behavior

2. **Class Balance**
   - Mitigation: Configurable weights
   - Testing: PvP simulations

---

## Success Metrics

### Functional Metrics
- âœ… Bots can login without crashes
- âœ… Bots can move to destinations
- âœ… Bots can engage in combat
- âœ… Each class has working rotation
- âœ… Bots can follow players
- âœ… Bots respond to threats

### Performance Metrics
- AI decision time < 50ms (P95)
- Memory usage < 10MB per bot
- CPU usage < 0.5% per bot
- 100+ concurrent bots stable
- Zero memory leaks over 24h

### Quality Metrics
- Unit test coverage > 80%
- Integration tests pass 100%
- No regression in core functionality
- Code review approval
- Documentation complete

---

## Deliverables

### Week 7-8 Deliverables
1. `BotSession.h/cpp` - Virtual session implementation
2. `BotSessionMgr.h/cpp` - Session management
3. Session unit tests
4. Performance benchmarks

### Week 9-10 Deliverables
1. `BotAI.h/cpp` - Core AI framework
2. Strategy/Action/Trigger system
3. Value context implementation
4. AI unit tests

### Week 11 Deliverables
1. `MovementManager.h/cpp` - Movement system
2. Formation templates
3. Pathfinding integration
4. Movement tests

### Week 12-13 Deliverables
1. `ThreatManager.h/cpp` - Threat system
2. `SpellManager.h/cpp` - Spell management
3. Combat positioning
4. Combat state machine

### Week 14 Deliverables
1. All 13 class AI implementations
2. Role-specific behaviors
3. Class rotation tests
4. Integration test suite

---

## Development Checklist

### Pre-Development
- [ ] Phase 1 complete and tested
- [ ] Development environment ready
- [ ] Git branch created for Phase 2
- [ ] Dependencies identified

### Week 7-8 Tasks
- [ ] BotSession class implemented
- [ ] BotSessionMgr implemented
- [ ] Packet queue system working
- [ ] Session pool tested
- [ ] Thread safety verified

### Week 9-10 Tasks
- [ ] BotAI base class complete
- [ ] Strategy pattern implemented
- [ ] Action system working
- [ ] Trigger system functional
- [ ] Value context operational

### Week 11 Tasks
- [ ] Movement manager complete
- [ ] Pathfinding integrated
- [ ] Formations working
- [ ] Movement validated

### Week 12-13 Tasks
- [ ] Threat system operational
- [ ] Spell manager complete
- [ ] Positioning logic working
- [ ] Combat state machine functional

### Week 14 Tasks
- [ ] All class AIs implemented
- [ ] Rotations tested
- [ ] Role behaviors working
- [ ] Integration tests passing

### Post-Development
- [ ] Performance profiling complete
- [ ] Memory leak testing done
- [ ] Documentation updated
- [ ] Code review passed
- [ ] Merge to main branch

---

## Phase 2 to Phase 3 Transition

### Phase 2 Exit Criteria
1. All deliverables complete
2. Tests passing (>95%)
3. Performance targets met
4. No critical bugs
5. Documentation current

### Phase 3 Preview
**Next Phase:** Advanced Bot Behaviors
- Group/Raid coordination
- Dungeon navigation
- PvP strategies
- Quest automation
- Economy participation

---

## Appendix A: File Structure

```
src/modules/Playerbot/
â”œâ”€â”€ Session/
â”‚   â”œâ”€â”€ BotSession.h
â”‚   â”œâ”€â”€ BotSession.cpp
â”‚   â”œâ”€â”€ BotSessionMgr.h
â”‚   â””â”€â”€ BotSessionMgr.cpp
â”œâ”€â”€ AI/
â”‚   â”œâ”€â”€ BotAI.h
â”‚   â”œâ”€â”€ BotAI.cpp
â”‚   â”œâ”€â”€ Strategy/
â”‚   â”‚   â”œâ”€â”€ Strategy.h
â”‚   â”‚   â”œâ”€â”€ CombatStrategy.cpp
â”‚   â”‚   â””â”€â”€ NonCombatStrategy.cpp
â”‚   â”œâ”€â”€ Actions/
â”‚   â”‚   â”œâ”€â”€ Action.h
â”‚   â”‚   â”œâ”€â”€ MovementActions.cpp
â”‚   â”‚   â””â”€â”€ CombatActions.cpp
â”‚   â”œâ”€â”€ Triggers/
â”‚   â”‚   â”œâ”€â”€ Trigger.h
â”‚   â”‚   â””â”€â”€ GenericTriggers.cpp
â”‚   â”œâ”€â”€ Values/
â”‚   â”‚   â”œâ”€â”€ Value.h
â”‚   â”‚   â””â”€â”€ CommonValues.cpp
â”‚   â”œâ”€â”€ Movement/
â”‚   â”‚   â”œâ”€â”€ MovementManager.h
â”‚   â”‚   â””â”€â”€ MovementManager.cpp
â”‚   â”œâ”€â”€ Combat/
â”‚   â”‚   â”œâ”€â”€ ThreatManager.h
â”‚   â”‚   â”œâ”€â”€ SpellManager.h
â”‚   â”‚   â””â”€â”€ CombatPositioning.cpp
â”‚   â””â”€â”€ ClassAI/
â”‚       â”œâ”€â”€ WarriorBotAI.cpp
â”‚       â”œâ”€â”€ PaladinBotAI.cpp
â”‚       â”œâ”€â”€ HunterBotAI.cpp
â”‚       â”œâ”€â”€ RogueBotAI.cpp
â”‚       â”œâ”€â”€ PriestBotAI.cpp
â”‚       â”œâ”€â”€ DeathKnightBotAI.cpp
â”‚       â”œâ”€â”€ ShamanBotAI.cpp
â”‚       â”œâ”€â”€ MageBotAI.cpp
â”‚       â”œâ”€â”€ WarlockBotAI.cpp
â”‚       â”œâ”€â”€ MonkBotAI.cpp
â”‚       â”œâ”€â”€ DruidBotAI.cpp
â”‚       â”œâ”€â”€ DemonHunterBotAI.cpp
â”‚       â””â”€â”€ EvokerBotAI.cpp
â””â”€â”€ Tests/
    â”œâ”€â”€ SessionTests.cpp
    â”œâ”€â”€ AIFrameworkTests.cpp
    â”œâ”€â”€ MovementTests.cpp
    â”œâ”€â”€ CombatTests.cpp
    â””â”€â”€ ClassAITests.cpp
```

---

## Appendix B: Critical Code Patterns

### Pattern 1: Thread-Safe Updates
```cpp
void BotSessionMgr::Update(uint32 diff)
{
    std::vector<BotSession*> sessionsToUpdate;
    
    {
        std::lock_guard<std::mutex> lock(_sessionMutex);
        for (auto& [accountId, session] : _sessions)
        {
            sessionsToUpdate.push_back(session.get());
        }
    }
    
    // Update outside lock
    std::for_each(std::execution::par_unseq,
        sessionsToUpdate.begin(), 
        sessionsToUpdate.end(),
        [diff](BotSession* session) {
            session->Update(diff);
        });
}
```

### Pattern 2: Action Execution
```cpp
bool BotAI::ExecuteAction(std::string const& name)
{
    if (auto* action = _actionExecutor->GetAction(name))
    {
        if (!action->IsPossible())
            return false;
            
        if (!action->IsUseful())
            return false;
            
        Event event;
        if (action->Execute(event))
        {
            _lastAction = name;
            _lastActionTime = getMSTime();
            return true;
        }
    }
    return false;
}
```

### Pattern 3: Strategy Selection
```cpp
Action* BotAI::SelectNextAction()
{
    std::vector<std::pair<Action*, float>> candidates;
    
    for (auto& strategy : _strategies)
    {
        strategy->UpdateTriggers();
        
        for (auto& trigger : strategy->GetTriggers())
        {
            if (trigger->IsActive())
            {
                if (auto* action = strategy->GetAction(trigger))
                {
                    float score = CalculateActionScore(action);
                    candidates.push_back({action, score});
                }
            }
        }
    }
    
    // Sort by score
    std::sort(candidates.begin(), candidates.end(),
        [](const auto& a, const auto& b) {
            return a.second > b.second;
        });
    
    return candidates.empty() ? nullptr : candidates[0].first;
}
```

---

## Document Version
- **Version:** 1.0.0
- **Created:** 2024-01-XX
- **Last Updated:** 2024-01-XX
- **Author:** TrinityCore Playerbot Team
- **Status:** READY FOR IMPLEMENTATION

---

*End of Phase 2 Project Plan*