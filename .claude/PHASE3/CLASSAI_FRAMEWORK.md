# Class AI Framework - Phase 3 Sprint 3A
## Base Architecture for All Class-Specific AI

## Overview

The Class AI Framework provides the foundation for implementing sophisticated, class-specific bot behaviors. This framework leverages the Strategy pattern from Phase 2's AI system while adding class-specific mechanics, resource management, and combat rotations.

## Architecture Design

### Class Hierarchy
```cpp
// Base class for all class-specific AI
class ClassAI : public BotAI {
public:
    ClassAI(Player* bot);
    virtual ~ClassAI() = default;

    // Core AI loop
    void UpdateAI(uint32 diff) override;
    
    // Class-specific interface
    virtual void UpdateRotation(Unit* target) = 0;
    virtual void UpdateBuffs() = 0;
    virtual void UpdateCooldowns(uint32 diff) = 0;
    virtual bool CanUseAbility(uint32 spellId) = 0;
    
protected:
    // Resource management
    virtual bool HasEnoughResource(uint32 spellId) = 0;
    virtual void ConsumeResource(uint32 spellId) = 0;
    
    // Positioning
    virtual Position GetOptimalPosition(Unit* target) = 0;
    virtual float GetOptimalRange(Unit* target) = 0;
    
    // Utility functions
    bool IsSpellReady(uint32 spellId);
    bool IsInRange(Unit* target, uint32 spellId);
    bool HasLineOfSight(Unit* target);
};
```

## Implementation Components

### 1. Action Priority System

```cpp
// src/modules/Playerbot/AI/ClassAI/ActionPriority.h

enum class ActionPriority : uint8 {
    EMERGENCY = 0,      // Defensive cooldowns, health potions
    SURVIVAL = 1,       // Heals, defensive abilities
    INTERRUPT = 2,      // Interrupt enemy casts
    BURST = 3,          // Offensive cooldowns
    ROTATION = 4,       // Normal rotation abilities
    MOVEMENT = 5,       // Positioning adjustments
    BUFF = 6,           // Maintain buffs
    IDLE = 7            // Out of combat activities
};

struct PrioritizedAction {
    uint32 spellId;
    ActionPriority priority;
    float score;        // Dynamic scoring within priority
    Unit* target;
    
    bool operator<(const PrioritizedAction& other) const {
        if (priority != other.priority)
            return priority < other.priority;
        return score > other.score;
    }
};

class ActionPriorityQueue {
public:
    void AddAction(uint32 spellId, ActionPriority priority, float score, Unit* target = nullptr);
    PrioritizedAction GetNextAction();
    void Clear();
    
private:
    tbb::concurrent_priority_queue<PrioritizedAction> _queue;
};
```

### 2. Cooldown Management System

```cpp
// src/modules/Playerbot/AI/ClassAI/CooldownManager.h

class CooldownManager {
public:
    struct CooldownInfo {
        uint32 spellId;
        uint32 cooldownMs;
        uint32 remainingMs;
        bool onGCD;
        uint32 charges;
        uint32 maxCharges;
    };
    
    void Update(uint32 diff);
    void StartCooldown(uint32 spellId, uint32 cooldownMs);
    bool IsReady(uint32 spellId) const;
    uint32 GetRemaining(uint32 spellId) const;
    
    // Global cooldown
    void TriggerGCD(uint32 durationMs = 1500);
    bool IsGCDReady() const;
    
    // Charge-based abilities
    void SetCharges(uint32 spellId, uint32 charges, uint32 maxCharges);
    uint32 GetCharges(uint32 spellId) const;
    
private:
    phmap::flat_hash_map<uint32, CooldownInfo> _cooldowns;
    uint32 _globalCooldown = 0;
};
```

### 3. Resource Tracking

```cpp
// src/modules/Playerbot/AI/ClassAI/ResourceManager.h

enum class ResourceType : uint8 {
    MANA,
    RAGE,
    ENERGY,
    COMBO_POINTS,
    RUNES,
    RUNIC_POWER,
    SOUL_SHARDS,
    LUNAR_POWER,
    HOLY_POWER,
    MAELSTROM,
    CHI,
    INSANITY,
    FURY,
    PAIN,
    ESSENCE
};

class ResourceManager {
public:
    ResourceManager(Player* bot);
    
    // Resource queries
    uint32 GetCurrent(ResourceType type) const;
    uint32 GetMax(ResourceType type) const;
    float GetPercent(ResourceType type) const;
    
    // Resource predictions
    uint32 GetCost(uint32 spellId) const;
    bool CanAfford(uint32 spellId) const;
    uint32 GetGeneration(uint32 spellId) const;
    
    // Special resources
    uint8 GetComboPoints() const;
    uint8 GetHolyPower() const;
    uint8 GetChi() const;
    
    // Death Knight runes
    struct RuneState {
        uint8 blood;
        uint8 frost;
        uint8 unholy;
        uint32 nextRuneTime;
    };
    RuneState GetRuneState() const;
    
private:
    Player* _bot;
    mutable phmap::flat_hash_map<uint32, uint32> _costCache;
};
```

## Class-Specific Implementations

### Warrior AI Example

```cpp
// src/modules/Playerbot/AI/ClassAI/WarriorAI.h

class WarriorAI : public ClassAI {
public:
    enum Spec { ARMS, FURY, PROTECTION };
    
    WarriorAI(Player* bot);
    
    void UpdateRotation(Unit* target) override;
    void UpdateBuffs() override;
    void UpdateCooldowns(uint32 diff) override;
    
protected:
    bool HasEnoughResource(uint32 spellId) override;
    void ConsumeResource(uint32 spellId) override;
    Position GetOptimalPosition(Unit* target) override;
    
private:
    // Spec-specific rotations
    void ExecuteArmsRotation(Unit* target);
    void ExecuteFuryRotation(Unit* target);
    void ExecuteProtectionRotation(Unit* target);
    
    // Warrior mechanics
    void ManageStance();
    void ManageShouts();
    void ManageDefensives();
    
    // Rage management
    bool ShouldPoolRage() const;
    uint32 GetRageDump() const;
    
    Spec _spec;
    uint32 _currentStance;
    bool _hasEnrage;
};

// src/modules/Playerbot/AI/ClassAI/WarriorAI.cpp

void WarriorAI::UpdateRotation(Unit* target) {
    if (!target || !target->IsAlive())
        return;
    
    ActionPriorityQueue actions;
    
    // Emergency actions
    if (_bot->GetHealthPct() < 30) {
        actions.AddAction(SPELL_LAST_STAND, ActionPriority::EMERGENCY, 100);
        actions.AddAction(SPELL_SHIELD_WALL, ActionPriority::EMERGENCY, 90);
    }
    
    // Interrupts
    if (target->IsNonMeleeSpellCast(false)) {
        actions.AddAction(SPELL_PUMMEL, ActionPriority::INTERRUPT, 100);
    }
    
    // Execute spec rotation
    switch (_spec) {
        case ARMS:
            ExecuteArmsRotation(target);
            break;
        case FURY:
            ExecuteFuryRotation(target);
            break;
        case PROTECTION:
            ExecuteProtectionRotation(target);
            break;
    }
}

void WarriorAI::ExecuteArmsRotation(Unit* target) {
    ActionPriorityQueue actions;
    
    // Colossus Smash on cooldown
    if (IsSpellReady(SPELL_COLOSSUS_SMASH)) {
        actions.AddAction(SPELL_COLOSSUS_SMASH, ActionPriority::ROTATION, 100);
    }
    
    // Mortal Strike on cooldown
    if (IsSpellReady(SPELL_MORTAL_STRIKE)) {
        actions.AddAction(SPELL_MORTAL_STRIKE, ActionPriority::ROTATION, 90);
    }
    
    // Execute phase
    if (target->GetHealthPct() < 20 && IsSpellReady(SPELL_EXECUTE)) {
        actions.AddAction(SPELL_EXECUTE, ActionPriority::ROTATION, 95);
    }
    
    // Overpower procs
    if (HasAura(SPELL_OVERPOWER_PROC) && IsSpellReady(SPELL_OVERPOWER)) {
        actions.AddAction(SPELL_OVERPOWER, ActionPriority::ROTATION, 85);
    }
    
    // Slam as filler
    if (GetCurrent(ResourceType::RAGE) > 40) {
        actions.AddAction(SPELL_SLAM, ActionPriority::ROTATION, 70);
    }
    
    // Process action queue
    while (!actions.Empty()) {
        auto action = actions.GetNextAction();
        if (CanUseAbility(action.spellId)) {
            CastSpell(action.target ? action.target : target, action.spellId);
            break;
        }
    }
}
```

### Priest AI Example

```cpp
// src/modules/Playerbot/AI/ClassAI/PriestAI.h

class PriestAI : public ClassAI {
public:
    enum Spec { DISCIPLINE, HOLY, SHADOW };
    
    PriestAI(Player* bot);
    
    void UpdateRotation(Unit* target) override;
    void UpdateBuffs() override;
    
private:
    // Healing logic
    Unit* GetHealTarget();
    uint32 GetHealSpell(Unit* target);
    bool ShouldUseCooldown(Unit* target);
    
    // Shadow mechanics
    void ManageShadowform();
    void ManageDoTs(Unit* target);
    void BuildInsanity();
    void EnterVoidform();
    
    // Discipline mechanics
    void ApplyAtonement();
    void ManageShields();
    
    Spec _spec;
    std::vector<ObjectGuid> _atonementTargets;
    uint32 _insanity;
    bool _inVoidform;
};
```

## File Structure

```
src/modules/Playerbot/AI/ClassAI/
├── ClassAI.h                  # Base class
├── ClassAI.cpp
├── ActionPriority.h           # Priority queue system
├── ActionPriority.cpp
├── CooldownManager.h          # Cooldown tracking
├── CooldownManager.cpp
├── ResourceManager.h          # Resource management
├── ResourceManager.cpp
├── Warriors/
│   ├── WarriorAI.h
│   ├── WarriorAI.cpp
│   ├── ArmsWarriorAI.cpp
│   ├── FuryWarriorAI.cpp
│   └── ProtectionWarriorAI.cpp
├── Paladins/
│   ├── PaladinAI.h
│   ├── PaladinAI.cpp
│   ├── HolyPaladinAI.cpp
│   ├── ProtectionPaladinAI.cpp
│   └── RetributionPaladinAI.cpp
├── Hunters/
│   ├── HunterAI.h
│   ├── HunterAI.cpp
│   ├── BeastMasteryHunterAI.cpp
│   ├── MarksmanshipHunterAI.cpp
│   └── SurvivalHunterAI.cpp
└── ... (other classes)
```

## Performance Considerations

### Memory Optimization
```cpp
// Use object pooling for frequently created objects
class ActionPool {
public:
    static PrioritizedAction* Acquire() {
        return _pool.construct();
    }
    
    static void Release(PrioritizedAction* action) {
        _pool.destroy(action);
    }
    
private:
    static boost::object_pool<PrioritizedAction> _pool;
};
```

### CPU Optimization
```cpp
// Cache expensive calculations
class SpellDataCache {
public:
    struct SpellData {
        uint32 manaCost;
        float range;
        uint32 castTime;
        uint32 cooldown;
        bool instant;
    };
    
    static const SpellData& Get(uint32 spellId) {
        auto it = _cache.find(spellId);
        if (it != _cache.end())
            return it->second;
        
        // Load from DBC
        SpellData data = LoadFromDBC(spellId);
        _cache[spellId] = data;
        return _cache[spellId];
    }
    
private:
    static phmap::flat_hash_map<uint32, SpellData> _cache;
};
```

## Testing Framework

```cpp
// src/modules/Playerbot/Tests/ClassAITest.cpp

TEST_F(ClassAITest, WarriorRotation) {
    auto bot = CreateTestBot(CLASS_WARRIOR, 80);
    auto ai = std::make_unique<WarriorAI>(bot);
    
    auto target = CreateTestTarget();
    
    // Test rotation produces expected spell sequence
    std::vector<uint32> expectedSpells = {
        SPELL_CHARGE,
        SPELL_COLOSSUS_SMASH,
        SPELL_MORTAL_STRIKE,
        SPELL_OVERPOWER
    };
    
    for (auto spellId : expectedSpells) {
        ai->UpdateRotation(target);
        EXPECT_EQ(bot->GetLastCastSpell(), spellId);
    }
}

TEST_F(ClassAITest, ResourceManagement) {
    auto bot = CreateTestBot(CLASS_WARRIOR, 80);
    auto manager = ResourceManager(bot);
    
    // Test rage tracking
    bot->SetPower(POWER_RAGE, 50);
    EXPECT_EQ(manager.GetCurrent(ResourceType::RAGE), 50);
    EXPECT_TRUE(manager.CanAfford(SPELL_MORTAL_STRIKE));
}
```

## Next Steps

1. **Implement Base ClassAI** - Create foundation classes
2. **Add Priority System** - Implement action queue
3. **Create Resource Manager** - Build resource tracking
4. **Implement First Class** - Start with Warrior
5. **Add Unit Tests** - Test each component

---

**Status**: Ready for Implementation
**Dependencies**: Phase 2 AI Framework ✅
**Estimated Time**: Sprint 3A Days 1-3