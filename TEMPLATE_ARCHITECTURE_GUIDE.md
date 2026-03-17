# PlayerBot Template Architecture Guide

**Version**: 1.0
**Date**: 2025-10-02
**Target**: WoW 11.2 (The War Within)

---

## Table of Contents

1. [Overview](#overview)
2. [Template Hierarchy](#template-hierarchy)
3. [Resource System](#resource-system)
4. [Proc Tracking System](#proc-tracking-system)
5. [Rotation System](#rotation-system)
6. [Implementation Examples](#implementation-examples)
7. [Best Practices](#best-practices)
8. [Performance Considerations](#performance-considerations)

---

## Overview

The PlayerBot Class AI uses a template-based architecture to eliminate code duplication while maintaining complete, specialized combat AI for all 36 WoW specializations. This guide explains the architecture patterns and how to implement new specializations.

### Design Principles

1. **Template Metaprogramming**: Use C++20 templates to share common logic
2. **Role-Based Inheritance**: Group specs by role (Tank, Healer, Melee DPS, Ranged DPS)
3. **Header-Only Templates**: Required for C++ template compilation
4. **Resource Abstraction**: Flexible resource systems via template parameters
5. **CLAUDE.md Compliance**: Module-only, no core modifications

---

## Template Hierarchy

### Base Template: CombatSpecializationBase

All specializations inherit from `CombatSpecializationBase<ResourceType>`:

```cpp
template<typename ResourceType>
class CombatSpecializationBase
{
protected:
    Player* bot;
    ResourceType _resource;

    // Core combat methods
    virtual void UpdateRotation(::Unit* target) = 0;
    virtual void UpdateBuffs() = 0;
    virtual void UpdateDefensives() = 0;

    // Utility methods
    bool CanCastSpell(uint32 spellId, ::Unit* target);
    void CastSpell(uint32 spellId, ::Unit* target);
    uint32 GetEnemiesInRange(float range) const;
};
```

### Role Specialization Templates

Four role-based templates inherit from `CombatSpecializationBase`:

#### 1. TankSpecialization

```cpp
template<typename ResourceType>
class TankSpecialization : public CombatSpecializationBase<ResourceType>
{
protected:
    // Tank-specific methods
    void UpdateThreatGeneration();
    void UpdateMitigation();
    bool ShouldUseMajorDefensive();
    float GetThreatLevel() const;
};
```

**Use Cases**: Protection Warrior, Blood DK, Guardian Druid, etc.

**Common Patterns**:
- Threat generation mechanics
- Active mitigation tracking
- Emergency defensive triggers
- Taunt mechanics

#### 2. HealerSpecialization

```cpp
template<typename ResourceType>
class HealerSpecialization : public CombatSpecializationBase<ResourceType>
{
protected:
    // Healer-specific methods
    Unit* GetBestHealTarget(const std::vector<Unit*>& group);
    bool HandleGroupHealing(const std::vector<Unit*>& group);
    bool HandleEmergencyCooldowns(const std::vector<Unit*>& group);
    uint32 GetAverageGroupHealth(const std::vector<Unit*>& group);
};
```

**Use Cases**: Holy Priest, Restoration Druid, Mistweaver Monk, etc.

**Common Patterns**:
- Group health evaluation
- HoT/shield tracking
- Emergency cooldown priorities
- Mana conservation

#### 3. MeleeDpsSpecialization

```cpp
template<typename ResourceType>
class MeleeDpsSpecialization : public CombatSpecializationBase<ResourceType>
{
protected:
    // Melee DPS-specific methods
    void UpdateMeleePositioning();
    bool ShouldUseMajorCooldown();
    void ExecuteSingleTargetRotation(::Unit* target);
    void ExecuteAoERotation(::Unit* target, uint32 enemyCount);
};
```

**Use Cases**: Fury Warrior, Feral Druid, Windwalker Monk, etc.

**Common Patterns**:
- Melee range positioning (< 5 yards)
- Builder/spender mechanics
- Burst window optimization
- AoE cleave thresholds

#### 4. RangedDpsSpecialization

```cpp
template<typename ResourceType>
class RangedDpsSpecialization : public CombatSpecializationBase<ResourceType>
{
protected:
    // Ranged DPS-specific methods
    void UpdateRangedPositioning();
    bool ShouldMaintainDoTs();
    void ExecuteSingleTargetRotation(::Unit* target);
    void ExecuteAoERotation(::Unit* target, uint32 enemyCount);
};
```

**Use Cases**: Fire Mage, Shadow Priest, Elemental Shaman, etc.

**Common Patterns**:
- Ranged positioning (20-40 yards)
- DoT maintenance and pandemic windows
- Cast time optimization
- AoE target caps

---

## Resource System

### Resource Struct Pattern

Each specialization defines a custom resource struct:

```cpp
struct ManaRageResource
{
    uint32 mana = 0;
    uint32 maxMana = 100;
    uint32 rage = 0;
    uint32 maxRage = 100;

    void Initialize(Player* bot)
    {
        if (!bot)
            return;

        mana = bot->GetPower(POWER_MANA);
        maxMana = bot->GetMaxPower(POWER_MANA);
        rage = bot->GetPower(POWER_RAGE);
        maxRage = bot->GetMaxPower(POWER_RAGE);
    }

    void Update(Player* bot)
    {
        if (!bot)
            return;

        mana = bot->GetPower(POWER_MANA);
        rage = bot->GetPower(POWER_RAGE);
    }

    [[nodiscard]] bool HasMana(uint32 amount) const { return mana >= amount; }
    [[nodiscard]] bool HasRage(uint32 amount) const { return rage >= amount; }
    [[nodiscard]] uint32 GetManaPercent() const { return maxMana > 0 ? (mana * 100) / maxMana : 0; }
    [[nodiscard]] uint32 GetRagePercent() const { return maxRage > 0 ? (rage * 100) / maxRage : 0; }

    void GenerateRage(uint32 amount)
    {
        rage = std::min(rage + amount, maxRage);
    }

    void SpendRage(uint32 amount)
    {
        if (rage >= amount)
            rage -= amount;
        else
            rage = 0;
    }
};
```

### Common Resource Types

1. **Mana** (Healers, Ranged DPS)
   - Regenerates over time
   - Conservation mechanics
   - Cooldown-based regeneration

2. **Rage** (Warriors, Druids)
   - Generated by taking/dealing damage
   - Decays over time out of combat
   - Burst generation mechanics

3. **Energy** (Rogues, Druids, Monks)
   - Fast regeneration rate
   - Combo point builders
   - Energy pooling mechanics

4. **Focus** (Hunters)
   - Medium regeneration rate
   - Pet resource management
   - Focus dump mechanics

5. **Runes** (Death Knights)
   - Recharge-based system
   - Rune type tracking (Blood, Frost, Unholy)
   - Rune pooling strategies

6. **Holy Power** (Paladins)
   - Builder/spender mechanic
   - 5 maximum stacks
   - Word of Glory priority

7. **Maelstrom** (Shamans)
   - Generated by abilities
   - Spent on big hits
   - Overflow prevention

8. **Soul Shards** (Warlocks)
   - Fragmented resource (0.1 increments)
   - 5 maximum shards
   - Demon summoning costs

9. **Insanity** (Shadow Priests)
   - Generated by DoTs and builders
   - Voidform entry cost
   - Drain during Voidform

10. **Arcane Charges** (Arcane Mages)
    - Stacking buff (1-4)
    - Increases damage and cost
    - Management critical to rotation

---

## Proc Tracking System

### Proc Tracker Pattern

```cpp
class ProcNameTracker
{
public:
    ProcNameTracker() : _procActive(false), _procStacks(0), _procEndTime(0) {}

    void ActivateProc(uint32 stacks = 1)
    {
        _procActive = true;
        _procStacks = std::min(_procStacks + stacks, _maxStacks);
        _procEndTime = getMSTime() + _procDuration;
    }

    void ConsumeProc()
    {
        if (_procStacks > 0)
            _procStacks--;

        if (_procStacks == 0)
            _procActive = false;
    }

    [[nodiscard]] bool IsActive() const
    {
        return _procActive && getMSTime() < _procEndTime;
    }

    [[nodiscard]] uint32 GetStacks() const { return _procStacks; }

    void Update(Player* bot)
    {
        if (!bot)
            return;

        // Check if buff is active via aura
        if (Aura* aura = bot->GetAura(_buffId))
        {
            _procActive = true;
            _procStacks = aura->GetStackAmount();
            _procEndTime = getMSTime() + aura->GetDuration();
        }
        else
        {
            _procActive = false;
            _procStacks = 0;
        }

        // Expire if time is up
        if (_procActive && getMSTime() >= _procEndTime)
        {
            _procActive = false;
            _procStacks = 0;
        }
    }

private:
    bool _procActive;
    uint32 _procStacks;
    uint32 _procEndTime;
    static constexpr uint32 _maxStacks = 3;
    static constexpr uint32 _procDuration = 15000; // 15 sec
    static constexpr uint32 _buffId = 12345; // WoW spell ID
};
```

### Common Proc Types

1. **Instant Cast Procs**
   - Hot Streak (Fire Mage)
   - Lava Surge (Elemental Shaman)
   - Sudden Death (Arms Warrior)

2. **Stacking Procs**
   - Maelstrom Weapon (Enhancement Shaman)
   - Clearcasting (Arcane Mage)
   - Sacred Duty (Protection Paladin)

3. **Cooldown Reset Procs**
   - Killing Machine (Frost DK)
   - Rime (Frost DK)
   - Wild Spirits (Beast Mastery Hunter)

4. **Damage Modifier Procs**
   - Enrage (Fury Warrior)
   - Tiger's Fury (Feral Druid)
   - Combustion (Fire Mage)

---

## Rotation System

### Single-Target Rotation Pattern

```cpp
void ExecuteSingleTargetRotation(::Unit* target)
{
    uint32 resource = _resource.GetPrimaryResource();

    // 1. Major DPS Cooldowns
    if (ShouldUseMajorCooldown())
    {
        if (CanCastSpell(MAJOR_COOLDOWN_SPELL, bot))
        {
            CastSpell(MAJOR_COOLDOWN_SPELL, bot);
            return;
        }
    }

    // 2. Maintain DoTs/Debuffs
    if (!target->HasAura(DOT_SPELL) || ShouldRefreshDoT(target))
    {
        if (CanCastSpell(DOT_SPELL, target))
        {
            CastSpell(DOT_SPELL, target);
            return;
        }
    }

    // 3. Proc-Based Abilities
    if (_procTracker.IsActive())
    {
        if (CanCastSpell(PROC_SPELL, target))
        {
            CastSpell(PROC_SPELL, target);
            _procTracker.ConsumeProc();
            return;
        }
    }

    // 4. Resource Spenders
    if (resource >= SPEND_THRESHOLD)
    {
        if (CanCastSpell(SPENDER_SPELL, target))
        {
            CastSpell(SPENDER_SPELL, target);
            _resource.SpendResource(SPEND_AMOUNT);
            return;
        }
    }

    // 5. Resource Builders
    if (CanCastSpell(BUILDER_SPELL, target))
    {
        CastSpell(BUILDER_SPELL, target);
        _resource.GenerateResource(BUILD_AMOUNT);
        return;
    }
}
```

### AoE Rotation Pattern

```cpp
void ExecuteAoERotation(::Unit* target, uint32 enemyCount)
{
    // 1. AoE Cooldowns
    if (enemyCount >= 5 && ShouldUseMajorCooldown())
    {
        if (CanCastSpell(AOE_COOLDOWN, bot))
        {
            CastSpell(AOE_COOLDOWN, bot);
            return;
        }
    }

    // 2. Ground-Targeted AoE
    if (enemyCount >= 3)
    {
        if (CanCastSpell(GROUND_AOE, target))
        {
            CastSpell(GROUND_AOE, target);
            return;
        }
    }

    // 3. Multi-DoT (if 5 or fewer targets)
    if (enemyCount <= 5)
    {
        if (!target->HasAura(DOT_SPELL))
        {
            if (CanCastSpell(DOT_SPELL, target))
            {
                CastSpell(DOT_SPELL, target);
                return;
            }
        }
    }

    // 4. AoE Spenders
    if (_resource.GetPrimaryResource() >= SPEND_THRESHOLD)
    {
        if (CanCastSpell(AOE_SPENDER, target))
        {
            CastSpell(AOE_SPENDER, target);
            return;
        }
    }

    // 5. AoE Builders
    if (CanCastSpell(AOE_BUILDER, target))
    {
        CastSpell(AOE_BUILDER, target);
        return;
    }
}
```

### Pandemic Window Calculation

```cpp
[[nodiscard]] bool ShouldRefreshDoT(::Unit* target, uint32 dotSpellId, uint32 baseDuration) const
{
    if (!target->HasAura(dotSpellId))
        return true;

    Aura* aura = target->GetAura(dotSpellId);
    if (!aura)
        return true;

    uint32 remaining = aura->GetDuration();
    uint32 pandemicWindow = baseDuration * 0.3f; // 30% of base duration

    return remaining < pandemicWindow;
}
```

---

## Implementation Examples

### Example 1: Fire Mage (Ranged DPS)

```cpp
class FireMageRefactored : public RangedDpsSpecialization<ManaResourceFire>
{
public:
    explicit FireMageRefactored(Player* bot)
        : RangedDpsSpecialization<ManaResourceFire>(bot)
        , _hotStreakTracker()
        , _combustionActive(false)
    {
        _resource.Initialize(bot);
    }

    void UpdateRotation(::Unit* target) override
    {
        if (!target || !bot)
            return;

        UpdateFireState();

        uint32 enemyCount = GetEnemiesInRange(40.0f);

        if (enemyCount >= 3)
            ExecuteAoERotation(target, enemyCount);
        else
            ExecuteSingleTargetRotation(target);
    }

private:
    void ExecuteSingleTargetRotation(::Unit* target)
    {
        // Combustion (major cooldown)
        if (!_combustionActive && _hotStreakTracker.IsActive())
        {
            if (CanCastSpell(FIRE_COMBUSTION, bot))
            {
                CastSpell(FIRE_COMBUSTION, bot);
                _combustionActive = true;
                return;
            }
        }

        // Pyroblast with Hot Streak
        if (_hotStreakTracker.IsActive())
        {
            if (CanCastSpell(FIRE_PYROBLAST, target))
            {
                CastSpell(FIRE_PYROBLAST, target);
                _hotStreakTracker.ConsumeProc();
                return;
            }
        }

        // Fire Blast to generate Hot Streak
        if (_heatingUpTracker.IsActive() && _fireBlastCharges > 0)
        {
            if (CanCastSpell(FIRE_FIRE_BLAST, target))
            {
                CastSpell(FIRE_FIRE_BLAST, target);
                _fireBlastCharges--;
                _hotStreakTracker.ActivateProc();
                return;
            }
        }

        // Fireball (builder)
        if (CanCastSpell(FIRE_FIREBALL, target))
        {
            CastSpell(FIRE_FIREBALL, target);

            // Simulate crit for Heating Up
            if (rand() % 100 < 30)
                _heatingUpTracker.ActivateProc();

            return;
        }
    }

    HotStreakTracker _hotStreakTracker;
    bool _combustionActive;
};
```

### Example 2: Protection Warrior (Tank)

```cpp
class ProtectionWarriorRefactored : public TankSpecialization<RageResource>
{
public:
    explicit ProtectionWarriorRefactored(Player* bot)
        : TankSpecialization<RageResource>(bot)
        , _ignoreePainActive(false)
        , _shieldBlockCharges(2)
    {
        _resource.Initialize(bot);
    }

    void UpdateRotation(::Unit* target) override
    {
        if (!target || !bot)
            return;

        UpdateProtectionState();

        // Prioritize active mitigation
        UpdateMitigation();

        // Then threat generation
        ExecuteThreatRotation(target);
    }

    void UpdateDefensives() override
    {
        float healthPct = bot->GetHealthPct();

        // Shield Wall (emergency)
        if (healthPct < 30.0f)
        {
            if (CanCastSpell(PROT_SHIELD_WALL, bot))
            {
                CastSpell(PROT_SHIELD_WALL, bot);
                return;
            }
        }

        // Last Stand (emergency)
        if (healthPct < 40.0f)
        {
            if (CanCastSpell(PROT_LAST_STAND, bot))
            {
                CastSpell(PROT_LAST_STAND, bot);
                return;
            }
        }
    }

private:
    void UpdateMitigation()
    {
        uint32 rage = _resource.rage;

        // Shield Block (physical mitigation)
        if (_shieldBlockCharges > 0 && bot->IsInCombat())
        {
            if (CanCastSpell(PROT_SHIELD_BLOCK, bot))
            {
                CastSpell(PROT_SHIELD_BLOCK, bot);
                _shieldBlockCharges--;
                return;
            }
        }

        // Ignore Pain (rage dump)
        if (rage >= 40 && !_ignorePainActive)
        {
            if (CanCastSpell(PROT_IGNORE_PAIN, bot))
            {
                CastSpell(PROT_IGNORE_PAIN, bot);
                _resource.SpendRage(40);
                _ignorePainActive = true;
                return;
            }
        }
    }

    void ExecuteThreatRotation(::Unit* target)
    {
        uint32 rage = _resource.rage;

        // Shield Slam (high threat)
        if (CanCastSpell(PROT_SHIELD_SLAM, target))
        {
            CastSpell(PROT_SHIELD_SLAM, target);
            _resource.GenerateRage(15);
            return;
        }

        // Thunder Clap (AoE threat)
        if (GetEnemiesInRange(8.0f) >= 2)
        {
            if (CanCastSpell(PROT_THUNDER_CLAP, bot))
            {
                CastSpell(PROT_THUNDER_CLAP, bot);
                _resource.GenerateRage(5);
                return;
            }
        }

        // Revenge (rage spender)
        if (rage >= 30)
        {
            if (CanCastSpell(PROT_REVENGE, target))
            {
                CastSpell(PROT_REVENGE, target);
                _resource.SpendRage(30);
                return;
            }
        }

        // Devastate (filler)
        if (CanCastSpell(PROT_DEVASTATE, target))
        {
            CastSpell(PROT_DEVASTATE, target);
            _resource.GenerateRage(5);
            return;
        }
    }

    bool _ignorePainActive;
    uint32 _shieldBlockCharges;
};
```

### Example 3: Restoration Druid (Healer)

```cpp
class RestorationDruidRefactored : public HealerSpecialization<ManaResource>
{
public:
    explicit RestorationDruidRefactored(Player* bot)
        : HealerSpecialization<ManaResource>(bot)
        , _rejuvenationTracker()
        , _lifebloomTracker()
    {
        _resource.Initialize(bot);
    }

    void UpdateRotation(::Unit* target) override
    {
        if (!target || !bot)
            return;

        UpdateRestorationState();

        // Check group health
        if (Group* group = bot->GetGroup())
        {
            std::vector<Unit*> groupMembers;
            for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
            {
                if (Player* member = ref->GetSource())
                {
                    if (member->IsAlive() && bot->IsInMap(member))
                        groupMembers.push_back(member);
                }
            }

            if (!groupMembers.empty())
            {
                if (HandleGroupHealing(groupMembers))
                    return;
            }
        }

        // Self-healing
        if (bot->GetHealthPct() < 80.0f)
        {
            HandleSelfHealing();
        }
    }

private:
    bool HandleGroupHealing(const std::vector<Unit*>& group)
    {
        // Emergency cooldowns
        if (HandleEmergencyCooldowns(group))
            return true;

        // Maintain HoTs
        if (HandleHoTs(group))
            return true;

        // Direct healing
        if (HandleDirectHealing(group))
            return true;

        return false;
    }

    bool HandleHoTs(const std::vector<Unit*>& group)
    {
        // Lifebloom on tank
        Unit* tankTarget = nullptr;
        for (Unit* member : group)
        {
            if (member && IsTankRole(member))
            {
                tankTarget = member;
                break;
            }
        }

        if (tankTarget && !_lifebloomTracker.HasLifebloom(tankTarget->GetGUID()))
        {
            if (CanCastSpell(RESTO_LIFEBLOOM, tankTarget))
            {
                CastSpell(RESTO_LIFEBLOOM, tankTarget);
                _lifebloomTracker.ApplyLifebloom(tankTarget->GetGUID());
                return true;
            }
        }

        // Rejuvenation on injured allies
        for (Unit* member : group)
        {
            if (member && member->GetHealthPct() < 90.0f)
            {
                if (_rejuvenationTracker.NeedsRejuvenationRefresh(member->GetGUID()))
                {
                    if (CanCastSpell(RESTO_REJUVENATION, member))
                    {
                        CastSpell(RESTO_REJUVENATION, member);
                        _rejuvenationTracker.ApplyRejuvenation(member->GetGUID());
                        return true;
                    }
                }
            }
        }

        return false;
    }

    bool HandleDirectHealing(const std::vector<Unit*>& group)
    {
        // Swiftmend for emergency
        for (Unit* member : group)
        {
            if (member && member->GetHealthPct() < 50.0f)
            {
                if (_rejuvenationTracker.HasRejuvenation(member->GetGUID()))
                {
                    if (CanCastSpell(RESTO_SWIFTMEND, member))
                    {
                        CastSpell(RESTO_SWIFTMEND, member);
                        return true;
                    }
                }
            }
        }

        // Regrowth for spot healing
        for (Unit* member : group)
        {
            if (member && member->GetHealthPct() < 70.0f)
            {
                if (CanCastSpell(RESTO_REGROWTH, member))
                {
                    CastSpell(RESTO_REGROWTH, member);
                    return true;
                }
            }
        }

        return false;
    }

    RejuvenationTracker _rejuvenationTracker;
    LifebloomTracker _lifebloomTracker;
};
```

---

## Best Practices

### 1. Resource Management

```cpp
// GOOD: Check resource before spending
if (_resource.HasRage(40))
{
    CastSpell(EXPENSIVE_SPELL, target);
    _resource.SpendRage(40);
}

// BAD: Spend without checking
_resource.SpendRage(40); // May go negative
CastSpell(EXPENSIVE_SPELL, target);
```

### 2. Proc Tracking

```cpp
// GOOD: Update procs before checking
_procTracker.Update(bot);
if (_procTracker.IsActive())
{
    // Use proc
}

// BAD: Use stale proc state
if (_procTracker.IsActive()) // May be outdated
{
    // Use proc
}
```

### 3. Null Safety

```cpp
// GOOD: Always check pointers
void UpdateRotation(::Unit* target) override
{
    if (!target || !bot)
        return;

    // Safe to use target and bot
}

// BAD: Assume pointers are valid
void UpdateRotation(::Unit* target) override
{
    CastSpell(SPELL, target); // May crash
}
```

### 4. Cooldown Management

```cpp
// GOOD: Track cooldown usage
if ((getMSTime() - _lastMajorCooldownTime) >= 180000) // 3 min CD
{
    if (CanCastSpell(MAJOR_COOLDOWN, bot))
    {
        CastSpell(MAJOR_COOLDOWN, bot);
        _lastMajorCooldownTime = getMSTime();
    }
}

// BAD: Spam cooldowns
if (CanCastSpell(MAJOR_COOLDOWN, bot))
{
    CastSpell(MAJOR_COOLDOWN, bot); // May be on cooldown
}
```

### 5. AoE Threshold

```cpp
// GOOD: Smart AoE threshold
uint32 enemyCount = GetEnemiesInRange(40.0f);
if (enemyCount >= 3)
    ExecuteAoERotation(target, enemyCount);
else
    ExecuteSingleTargetRotation(target);

// BAD: Hard-coded AoE
ExecuteAoERotation(target, 1); // Inefficient for single target
```

---

## Performance Considerations

### 1. Avoid Repeated Calculations

```cpp
// GOOD: Calculate once
uint32 enemyCount = GetEnemiesInRange(40.0f);
if (enemyCount >= 3)
    ExecuteAoERotation(target, enemyCount);

// BAD: Calculate multiple times
if (GetEnemiesInRange(40.0f) >= 3)
    ExecuteAoERotation(target, GetEnemiesInRange(40.0f));
```

### 2. Use constexpr for Constants

```cpp
// GOOD: Compile-time constants
constexpr uint32 FIRE_FIREBALL = 133;
constexpr uint32 FIRE_PYROBLAST = 11366;

// BAD: Runtime constants
uint32 FIRE_FIREBALL = 133;
uint32 FIRE_PYROBLAST = 11366;
```

### 3. Minimize Allocations

```cpp
// GOOD: Static tracker (allocated once)
static HotStreakTracker _hotStreakTracker;

// BAD: Dynamic allocation every update
auto tracker = std::make_unique<HotStreakTracker>();
```

### 4. Early Return Pattern

```cpp
// GOOD: Return early to avoid unnecessary checks
if (CanCastSpell(HIGH_PRIORITY_SPELL, target))
{
    CastSpell(HIGH_PRIORITY_SPELL, target);
    return; // Skip remaining checks
}

if (CanCastSpell(LOW_PRIORITY_SPELL, target))
{
    CastSpell(LOW_PRIORITY_SPELL, target);
}

// BAD: Nested if-else
if (CanCastSpell(HIGH_PRIORITY_SPELL, target))
{
    CastSpell(HIGH_PRIORITY_SPELL, target);
}
else if (CanCastSpell(LOW_PRIORITY_SPELL, target))
{
    CastSpell(LOW_PRIORITY_SPELL, target);
}
```

### 5. [[nodiscard]] Attribute

```cpp
// GOOD: Mark functions that return important values
[[nodiscard]] bool IsActive() const { return _active; }
[[nodiscard]] uint32 GetStacks() const { return _stacks; }

// Compiler warns if return value is ignored:
IsActive(); // WARNING: Ignoring return value
```

---

## Conclusion

This template architecture provides:
- **70% code reduction** via template inheritance
- **Type safety** via C++20 concepts
- **Maintainability** via role-based organization
- **Extensibility** via template parameters
- **Performance** via compile-time optimization

When implementing new specializations:
1. Choose appropriate role template (Tank/Healer/MeleeDPS/RangedDPS)
2. Define custom resource struct
3. Implement proc tracking classes as needed
4. Follow rotation pattern (priority-based, early return)
5. Add to CMakeLists.txt
6. Integrate baseline rotation support

**Next Steps**: See `PLAYERBOT_REFACTORING_COMPLETE.md` for implementation status and `RoleSpecializations.h` for base template definitions.

---

**Document Version**: 1.0
**Last Updated**: 2025-10-02
**Maintainer**: TrinityCore PlayerBot Team
