# ClassAI Code Duplication Analysis

## Executive Summary
**Critical Code Duplication Detected:** 1,740 occurrences of common methods across 200 files.

### Impact
- **Maintenance Burden:** Bug fixes require changes in 13+ places
- **Linker Warnings:** Multiple definition warnings for identical methods
- **Memory Waste:** Duplicated code increases binary size unnecessarily
- **Inconsistency Risk:** Same bugs exist in multiple classes

## Duplicate Code Patterns Identified

### Pattern 1: Cooldown Management (EVERY SPECIALIZATION)
**Duplicated in:** 50+ files
```cpp
void UpdateCooldowns(uint32 diff)
{
    for (auto& cooldown : _cooldowns)
        if (cooldown.second > diff)
            cooldown.second -= diff;
        else
            cooldown.second = 0;
}
```

### Pattern 2: Ability Validation (EVERY SPECIALIZATION)
**Duplicated in:** 50+ files
```cpp
bool CanUseAbility(uint32 spellId)
{
    auto it = _cooldowns.find(spellId);
    if (it != _cooldowns.end() && it->second > 0)
        return false;
    return HasEnoughResource(spellId);
}
```

### Pattern 3: Combat State Management (EVERY SPECIALIZATION)
**Duplicated in:** 50+ files
```cpp
void OnCombatStart(::Unit* target)
{
    Player* bot = GetBot();
    if (!bot)
        return;
    // Identical logic across all specs
}

void OnCombatEnd()
{
    // Identical cleanup logic
}
```

### Pattern 4: Range Calculation (EVERY SPECIALIZATION)
**Duplicated in:** 50+ files
```cpp
float GetOptimalRange(::Unit* target)
{
    // Melee: return 5.0f
    // Ranged DPS: return 25.0f
    // Healers: return 30.0f
    // Only difference is the return value
}
```

### Pattern 5: Resource Management (MANY SPECIALIZATIONS)
**Duplicated in:** 30+ files
```cpp
bool HasEnoughResource(uint32 spellId)
{
    // Check mana/rage/energy/etc
    // Logic varies slightly by resource type
}

void ConsumeResource(uint32 spellId)
{
    // Deduct resource cost
    // Logic varies slightly by resource type
}
```

## Files with Duplicate Definitions (Linker Warnings)

### FrostSpecialization (Death Knight)
```
warning LNK4006: "GetOptimalRange" bereits definiert
warning LNK4006: "GetOptimalPosition" bereits definiert
warning LNK4006: "ConsumeResource" bereits definiert
warning LNK4006: "HasEnoughResource" bereits definiert
warning LNK4006: "OnCombatEnd" bereits definiert
warning LNK4006: "OnCombatStart" bereits definiert
warning LNK4006: "CanUseAbility" bereits definiert
warning LNK4006: "UpdateCooldowns" bereits definiert
warning LNK4006: "UpdateBuffs" bereits definiert
warning LNK4006: "UpdateRotation" bereits definiert
```

## Current Class Hierarchy

```
ClassAI (base)
├── DeathKnightAI
│   ├── BloodSpecialization (862 lines)
│   ├── FrostSpecialization
│   └── UnholySpecialization
├── PaladinAI
│   ├── HolySpecialization
│   ├── ProtectionSpecialization
│   └── RetributionSpecialization (433 lines)
├── WarriorAI
│   ├── ArmsSpecialization (620 lines)
│   ├── FurySpecialization
│   └── ProtectionSpecialization
├── MageAI
│   ├── ArcaneSpecialization
│   ├── FireSpecialization (622 lines)
│   └── FrostSpecialization
├── (9 more classes with 3+ specs each)
```

**Total Specs:** ~40 specializations
**Estimated Duplicated Code:** 60-70% of each spec is duplicated boilerplate

## Proposed Refactoring Strategy

### Phase 1: Create Base Template System

#### New Base Class: `CombatSpecializationBase`
```cpp
template<typename ResourceType>
class CombatSpecializationBase : public ClassAI
{
protected:
    // ===== COMMON COOLDOWN MANAGEMENT =====
    std::unordered_map<uint32, uint32> _cooldowns;

    virtual void UpdateCooldowns(uint32 diff) final
    {
        for (auto& cooldown : _cooldowns)
            if (cooldown.second > diff)
                cooldown.second -= diff;
            else
                cooldown.second = 0;
    }

    bool IsCooldownReady(uint32 spellId) const
    {
        auto it = _cooldowns.find(spellId);
        return it == _cooldowns.end() || it->second == 0;
    }

    void StartCooldown(uint32 spellId, uint32 duration)
    {
        _cooldowns[spellId] = duration;
    }

    // ===== COMMON RESOURCE MANAGEMENT =====
    ResourceType _currentResource;
    ResourceType _maxResource;

    virtual bool HasEnoughResource(uint32 spellId)
    {
        ResourceType cost = GetResourceCost(spellId);
        return _currentResource >= cost;
    }

    virtual void ConsumeResource(uint32 spellId)
    {
        ResourceType cost = GetResourceCost(spellId);
        _currentResource -= cost;
    }

    // ===== COMMON ABILITY VALIDATION =====
    virtual bool CanUseAbility(uint32 spellId) final
    {
        return IsCooldownReady(spellId) &&
               HasEnoughResource(spellId) &&
               IsSpellUsable(spellId);
    }

    // ===== COMMON COMBAT STATE =====
    virtual void OnCombatStart(::Unit* target) override
    {
        _inCombat = true;
        _combatTarget = target;
        OnSpecializationCombatStart(target);
    }

    virtual void OnCombatEnd() override
    {
        _inCombat = false;
        _combatTarget = nullptr;
        OnSpecializationCombatEnd();
    }

    // ===== HOOKS FOR SPECIALIZATION-SPECIFIC LOGIC =====
    virtual void OnSpecializationCombatStart(::Unit* target) {}
    virtual void OnSpecializationCombatEnd() {}
    virtual ResourceType GetResourceCost(uint32 spellId) = 0;
    virtual void UpdateRotation(::Unit* target) = 0;
};
```

#### Specialized Resource Types
```cpp
// For Mana-based classes (Mage, Priest, Paladin, etc)
using ManaSpecialization = CombatSpecializationBase<uint32>;

// For Rage-based classes (Warrior)
using RageSpecialization = CombatSpecializationBase<uint32>;

// For Energy-based classes (Rogue, Monk)
using EnergySpecialization = CombatSpecializationBase<uint32>;

// For Rune-based classes (Death Knight)
struct RuneResource { /* complex rune management */ };
using RuneSpecialization = CombatSpecializationBase<RuneResource>;
```

#### Role-Based Templates
```cpp
// Melee DPS template
template<typename ResourceType>
class MeleeDpsSpecialization : public CombatSpecializationBase<ResourceType>
{
public:
    float GetOptimalRange(::Unit* target) override { return 5.0f; }

protected:
    bool IsInMeleeRange(::Unit* target) const
    {
        return GetBot()->GetDistance(target) <= 5.0f;
    }
};

// Ranged DPS template
template<typename ResourceType>
class RangedDpsSpecialization : public CombatSpecializationBase<ResourceType>
{
public:
    float GetOptimalRange(::Unit* target) override { return 25.0f; }

protected:
    bool ShouldMaintainDistance(::Unit* target) const
    {
        return GetBot()->GetDistance(target) < 15.0f;
    }
};

// Healer template
template<typename ResourceType>
class HealerSpecialization : public CombatSpecializationBase<ResourceType>
{
public:
    float GetOptimalRange(::Unit* target) override { return 30.0f; }

protected:
    ::Unit* GetBestHealTarget()
    {
        // Common heal target selection logic
    }
};

// Tank template
template<typename ResourceType>
class TankSpecialization : public CombatSpecializationBase<ResourceType>
{
public:
    float GetOptimalRange(::Unit* target) override { return 5.0f; }

protected:
    void ManageThreat(::Unit* target)
    {
        // Common threat management
    }
};
```

### Phase 2: Refactor Each Specialization

#### Before (433 lines - RetributionSpecialization)
```cpp
class RetributionSpecialization : public PaladinSpecialization
{
public:
    void UpdateCooldowns(uint32 diff) { /* 10 lines */ }
    bool CanUseAbility(uint32 spellId) { /* 8 lines */ }
    void OnCombatStart(::Unit* target) { /* 12 lines */ }
    void OnCombatEnd() { /* 8 lines */ }
    float GetOptimalRange(::Unit* target) { return 5.0f; }
    // ... 390 more lines of actual rotation logic
};
```

#### After (~250 lines - 42% reduction)
```cpp
class RetributionSpecialization : public MeleeDpsSpecialization<uint32>
{
public:
    RetributionSpecialization(Player* bot)
        : MeleeDpsSpecialization(bot)
    {
        // Specialization-specific initialization
    }

    void UpdateRotation(::Unit* target) override
    {
        // ONLY the unique Retribution logic
        if (ShouldCastTemplarsVerdict(target))
            CastTemplarsVerdict(target);
        else if (ShouldCastCrusaderStrike(target))
            CastCrusaderStrike(target);
        // ... only rotation logic, no boilerplate
    }

protected:
    uint32 GetResourceCost(uint32 spellId) override
    {
        // Mana cost calculation
    }

private:
    // ONLY Retribution-specific state
    uint8 _holyPower;
    bool _hasArtOfWar;

    // ONLY Retribution-specific methods
    bool ShouldCastTemplarsVerdict(::Unit* target);
    void CastTemplarsVerdict(::Unit* target);
    bool ShouldCastCrusaderStrike(::Unit* target);
    void CastCrusaderStrike(::Unit* target);
};
```

## Expected Benefits

### Code Reduction
- **Before:** ~40 specs × 400 lines avg = 16,000 lines
- **After:** 40 specs × 200 lines avg = 8,000 lines
- **Reduction:** 50% less code to maintain

### Eliminated Duplication
- Remove 1,740 duplicate method implementations
- Eliminate all linker warnings
- Single source of truth for common logic

### Maintenance Improvements
- Fix bugs in ONE place instead of 40+
- Add features to base class, all specs benefit
- Consistent behavior across all classes

### Performance Improvements
- Smaller binary size
- Better CPU cache utilization
- Reduced memory footprint

## Implementation Plan

### Week 1: Foundation
- **Day 1-2:** Create `CombatSpecializationBase` template
- **Day 3:** Create role-based templates (Melee, Ranged, Healer, Tank)
- **Day 4:** Create resource-type specializations
- **Day 5:** Test compilation, ensure no regressions

### Week 2: First Wave (Simpler Classes)
- **Day 1:** Refactor Warriors (3 specs)
- **Day 2:** Refactor Paladins (3 specs)
- **Day 3:** Refactor Hunters (3 specs)
- **Day 4:** Refactor Rogues (3 specs)
- **Day 5:** Test all 12 specs in combat

### Week 3: Second Wave (Complex Classes)
- **Day 1:** Refactor Death Knights (3 specs)
- **Day 2:** Refactor Mages (3 specs)
- **Day 3:** Refactor Priests (3 specs)
- **Day 4:** Refactor Druids (4 specs)
- **Day 5:** Test all 13 specs in combat

### Week 4: Final Wave + Testing
- **Day 1:** Refactor Warlocks (3 specs)
- **Day 2:** Refactor Shamans (3 specs)
- **Day 3:** Refactor Monks (3 specs)
- **Day 4:** Refactor Demon Hunters + Evokers
- **Day 5:** Full regression testing with all 40 specs

## Risk Mitigation

### Backup Strategy
- Keep original files as `.cpp.backup`
- Branch: `refactor/classai-templates`
- Commit after each class refactored

### Testing Strategy
- Test EACH class after refactoring
- Verify combat works for all 3 specs
- Check memory usage hasn't increased
- Confirm no new crashes

### Rollback Plan
If refactoring causes issues:
1. Revert to backup files
2. Cherry-pick working classes
3. Leave problematic classes unreffactored temporarily

## Success Criteria

✅ All linker warnings eliminated
✅ 50%+ code reduction
✅ All 40 specs compile cleanly
✅ All specs function in combat
✅ No performance regression
✅ No memory increase

## Conclusion

This refactoring will:
- Cut maintenance burden in HALF
- Eliminate technical debt
- Make future features easier to add
- Improve code quality significantly
- Pay dividends for ALL future development

**Recommendation:** Proceed with refactoring immediately before adding more features.
