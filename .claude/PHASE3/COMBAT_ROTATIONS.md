# Combat Rotation System - Phase 3 Sprint 3A
## Detailed Class Rotation Implementations

## Overview

This document provides detailed combat rotation implementations for all WoW classes. Each rotation is optimized for performance while maintaining realistic player-like behavior. Rotations adapt based on spec, target type, and combat situation.

## Core Rotation Engine

### Rotation Executor Framework

```cpp
// src/modules/Playerbot/AI/Rotations/RotationExecutor.h

class RotationExecutor {
public:
    RotationExecutor(Player* bot);
    
    // Main execution
    void Execute(Unit* target, CombatContext& context);
    
    // Rotation phases
    void ExecuteOpener(Unit* target);
    void ExecuteBurst(Unit* target);
    void ExecuteNormal(Unit* target);
    void ExecuteExecute(Unit* target);  // <20% health phase
    void ExecuteAoE(std::vector<Unit*>& targets);
    
    // Conditional execution
    bool ShouldInterrupt(Unit* target);
    bool ShouldUseDefensive();
    bool ShouldKite(Unit* target);
    
protected:
    // Rotation helpers
    bool TryCast(uint32 spellId, Unit* target = nullptr);
    bool TryCastSequence(std::vector<uint32> spells, Unit* target);
    Unit* SelectBestTarget(std::vector<Unit*>& targets);
    
private:
    Player* _bot;
    CooldownManager _cooldowns;
    ResourceManager _resources;
    uint32 _gcdEnd;
    uint32 _castEnd;
};
```

## Class-Specific Rotations

### 1. Warrior Rotations

```cpp
// src/modules/Playerbot/AI/Rotations/WarriorRotations.h

namespace WarriorRotations {

// Arms Warrior - Single Target
class ArmsRotation : public RotationExecutor {
public:
    void ExecuteNormal(Unit* target) override {
        // Priority list
        // 1. Colossus Smash/Warbreaker on CD
        if (TryCast(SPELL_COLOSSUS_SMASH, target))
            return;
        
        // 2. Mortal Strike on CD
        if (TryCast(SPELL_MORTAL_STRIKE, target))
            return;
        
        // 3. Execute (if available)
        if (target->GetHealthPct() < 20 || _bot->HasAura(SPELL_SUDDEN_DEATH)) {
            if (TryCast(SPELL_EXECUTE, target))
                return;
        }
        
        // 4. Bladestorm during Colossus Smash
        if (target->HasAura(SPELL_COLOSSUS_SMASH_DEBUFF) && 
            TryCast(SPELL_BLADESTORM))
            return;
        
        // 5. Overpower
        if (TryCast(SPELL_OVERPOWER, target))
            return;
        
        // 6. Slam (rage dump)
        if (_resources.GetCurrent(ResourceType::RAGE) > 40) {
            TryCast(SPELL_SLAM, target);
        }
    }
    
    void ExecuteAoE(std::vector<Unit*>& targets) override {
        if (targets.size() >= 3) {
            // Sweeping Strikes
            TryCast(SPELL_SWEEPING_STRIKES);
            
            // Bladestorm for 4+ targets
            if (targets.size() >= 4) {
                TryCast(SPELL_BLADESTORM);
            }
            
            // Cleave
            TryCast(SPELL_CLEAVE, SelectBestTarget(targets));
        }
    }
};

// Fury Warrior - Dual Wield Focus
class FuryRotation : public RotationExecutor {
public:
    void ExecuteNormal(Unit* target) override {
        // Maintain Enrage
        if (!_bot->HasAura(SPELL_ENRAGE)) {
            if (TryCast(SPELL_BLOODTHIRST, target))
                return;
        }
        
        // Rampage at 85+ rage
        if (_resources.GetCurrent(ResourceType::RAGE) >= 85) {
            if (TryCast(SPELL_RAMPAGE, target))
                return;
        }
        
        // Raging Blow with charges
        if (_cooldowns.GetCharges(SPELL_RAGING_BLOW) > 0) {
            if (TryCast(SPELL_RAGING_BLOW, target))
                return;
        }
        
        // Bloodthirst on CD
        if (TryCast(SPELL_BLOODTHIRST, target))
            return;
        
        // Whirlwind as filler
        TryCast(SPELL_WHIRLWIND, target);
    }
};

// Protection Warrior - Tank
class ProtectionRotation : public RotationExecutor {
public:
    void ExecuteNormal(Unit* target) override {
        // Maintain threat
        // 1. Shield Slam (highest threat)
        if (TryCast(SPELL_SHIELD_SLAM, target))
            return;
        
        // 2. Thunder Clap (AoE threat)
        if (TryCast(SPELL_THUNDER_CLAP))
            return;
        
        // 3. Revenge proc
        if (_bot->HasAura(SPELL_REVENGE_PROC)) {
            if (TryCast(SPELL_REVENGE, target))
                return;
        }
        
        // 4. Devastate (sunder application)
        TryCast(SPELL_DEVASTATE, target);
    }
    
    bool ShouldUseDefensive() override {
        float healthPct = _bot->GetHealthPct();
        
        // Shield Wall at 30%
        if (healthPct < 30 && TryCast(SPELL_SHIELD_WALL))
            return true;
        
        // Last Stand at 40%
        if (healthPct < 40 && TryCast(SPELL_LAST_STAND))
            return true;
        
        // Shield Block on CD for mitigation
        if (TryCast(SPELL_SHIELD_BLOCK))
            return true;
        
        return false;
    }
};

} // namespace WarriorRotations
```

### 2. Paladin Rotations

```cpp
// src/modules/Playerbot/AI/Rotations/PaladinRotations.h

namespace PaladinRotations {

// Retribution Paladin
class RetributionRotation : public RotationExecutor {
private:
    uint32 _holyPower = 0;
    
public:
    void ExecuteNormal(Unit* target) override {
        UpdateHolyPower();
        
        // 1. Wake of Ashes for holy power
        if (_holyPower == 0 && TryCast(SPELL_WAKE_OF_ASHES, target))
            return;
        
        // 2. Templar's Verdict at 3+ HP
        if (_holyPower >= 3 && TryCast(SPELL_TEMPLARS_VERDICT, target))
            return;
        
        // 3. Blade of Justice
        if (TryCast(SPELL_BLADE_OF_JUSTICE, target))
            return;
        
        // 4. Judgment
        if (TryCast(SPELL_JUDGMENT, target))
            return;
        
        // 5. Hammer of Wrath (execute)
        if (target->GetHealthPct() < 20) {
            if (TryCast(SPELL_HAMMER_OF_WRATH, target))
                return;
        }
        
        // 6. Consecration
        if (GetDistance(target) < 8.0f) {
            TryCast(SPELL_CONSECRATION);
        }
    }
    
    void UpdateHolyPower() {
        _holyPower = _bot->GetPower(POWER_HOLY_POWER);
    }
};

// Holy Paladin - Healer
class HolyRotation : public RotationExecutor {
public:
    void Execute(Unit* target, CombatContext& context) override {
        // Prioritize healing
        if (Unit* healTarget = GetLowestHealthAlly()) {
            ExecuteHealing(healTarget);
            return;
        }
        
        // DPS when no healing needed
        if (target && target->IsAlive()) {
            ExecuteDPS(target);
        }
    }
    
private:
    void ExecuteHealing(Unit* target) {
        float healthPct = target->GetHealthPct();
        
        // Emergency heals
        if (healthPct < 30) {
            TryCast(SPELL_LAY_ON_HANDS, target);
            TryCast(SPELL_HOLY_SHOCK, target);
            TryCast(SPELL_FLASH_OF_LIGHT, target);
            return;
        }
        
        // Normal healing
        if (healthPct < 70) {
            TryCast(SPELL_HOLY_SHOCK, target);
            TryCast(SPELL_HOLY_LIGHT, target);
        }
        
        // Beacon maintenance
        if (!target->HasAura(SPELL_BEACON_OF_LIGHT)) {
            TryCast(SPELL_BEACON_OF_LIGHT, target);
        }
    }
    
    void ExecuteDPS(Unit* target) {
        TryCast(SPELL_JUDGMENT, target);
        TryCast(SPELL_HOLY_SHOCK_DAMAGE, target);
        TryCast(SPELL_CONSECRATION);
    }
    
    Unit* GetLowestHealthAlly() {
        Unit* lowest = nullptr;
        float lowestPct = 100.0f;
        
        // Check party/raid members
        if (Group* group = _bot->GetGroup()) {
            for (GroupReference* ref = group->GetFirstMember(); 
                 ref != nullptr; ref = ref->next()) {
                if (Player* member = ref->GetSource()) {
                    float pct = member->GetHealthPct();
                    if (pct < lowestPct && pct < 80) {
                        lowest = member;
                        lowestPct = pct;
                    }
                }
            }
        }
        
        return lowest;
    }
};

} // namespace PaladinRotations
```

### 3. Hunter Rotations

```cpp
// src/modules/Playerbot/AI/Rotations/HunterRotations.h

namespace HunterRotations {

// Beast Mastery Hunter
class BeastMasteryRotation : public RotationExecutor {
private:
    Pet* _pet = nullptr;
    
public:
    void Initialize() {
        _pet = _bot->GetPet();
    }
    
    void ExecuteNormal(Unit* target) override {
        // Pet management
        if (!_pet || !_pet->IsAlive()) {
            TryCast(SPELL_CALL_PET_1);
            TryCast(SPELL_REVIVE_PET);
            return;
        }
        
        // Send pet to attack
        if (_pet && _pet->GetVictim() != target) {
            _pet->AI()->AttackStart(target);
        }
        
        // 1. Bestial Wrath
        if (TryCast(SPELL_BESTIAL_WRATH))
            return;
        
        // 2. Kill Command
        if (_pet && _pet->IsWithinMeleeRange(target)) {
            if (TryCast(SPELL_KILL_COMMAND, target))
                return;
        }
        
        // 3. Barbed Shot (maintain frenzy on pet)
        if (!_pet->HasAura(SPELL_PET_FRENZY) || 
            _pet->GetAura(SPELL_PET_FRENZY)->GetDuration() < 2000) {
            if (TryCast(SPELL_BARBED_SHOT, target))
                return;
        }
        
        // 4. Cobra Shot (focus dump)
        if (_resources.GetCurrent(ResourceType::FOCUS) > 50) {
            TryCast(SPELL_COBRA_SHOT, target);
        }
        
        // 5. Multi-Shot for AoE
        if (GetNearbyEnemies(8.0f).size() >= 3) {
            TryCast(SPELL_MULTISHOT);
        }
    }
    
    void ManagePet() {
        if (!_pet)
            return;
        
        // Pet healing
        if (_pet->GetHealthPct() < 50) {
            TryCast(SPELL_MEND_PET, _pet);
        }
        
        // Pet buffs
        if (!_pet->HasAura(SPELL_PET_PROWL)) {
            _pet->CastSpell(_pet, SPELL_PET_PROWL, true);
        }
    }
};

} // namespace HunterRotations
```

### 4. Mage Rotations

```cpp
// src/modules/Playerbot/AI/Rotations/MageRotations.h

namespace MageRotations {

// Frost Mage
class FrostRotation : public RotationExecutor {
private:
    uint32 _fingersOfFrost = 0;
    uint32 _brainFreeze = 0;
    
public:
    void ExecuteNormal(Unit* target) override {
        UpdateProcs();
        
        // 1. Icy Veins burst
        if (ShouldBurst() && TryCast(SPELL_ICY_VEINS))
            return;
        
        // 2. Frozen Orb on CD
        if (TryCast(SPELL_FROZEN_ORB, target))
            return;
        
        // 3. Ice Lance with Fingers of Frost
        if (_fingersOfFrost > 0) {
            if (TryCast(SPELL_ICE_LANCE, target)) {
                _fingersOfFrost--;
                return;
            }
        }
        
        // 4. Flurry with Brain Freeze
        if (_brainFreeze > 0) {
            if (TryCast(SPELL_FLURRY, target)) {
                _brainFreeze = 0;
                // Follow with Ice Lance
                TryCast(SPELL_ICE_LANCE, target);
                return;
            }
        }
        
        // 5. Frostbolt as filler
        TryCast(SPELL_FROSTBOLT, target);
    }
    
    void UpdateProcs() {
        _fingersOfFrost = _bot->GetAuraStack(SPELL_FINGERS_OF_FROST);
        _brainFreeze = _bot->HasAura(SPELL_BRAIN_FREEZE) ? 1 : 0;
    }
    
    bool ShouldBurst() {
        // Use burst CDs when high value procs available
        return _fingersOfFrost >= 2 || _brainFreeze > 0;
    }
};

// Fire Mage
class FireRotation : public RotationExecutor {
private:
    uint32 _heatingUp = 0;
    uint32 _hotStreak = 0;
    
public:
    void ExecuteNormal(Unit* target) override {
        UpdateProcs();
        
        // 1. Combustion burst window
        if (TryCast(SPELL_COMBUSTION)) {
            ExecuteBurstWindow(target);
            return;
        }
        
        // 2. Pyroblast with Hot Streak
        if (_hotStreak > 0) {
            if (TryCast(SPELL_PYROBLAST_INSTANT, target)) {
                _hotStreak = 0;
                return;
            }
        }
        
        // 3. Fire Blast to convert Heating Up
        if (_heatingUp > 0 && _cooldowns.GetCharges(SPELL_FIRE_BLAST) > 0) {
            if (TryCast(SPELL_FIRE_BLAST, target)) {
                // This should convert to Hot Streak
                return;
            }
        }
        
        // 4. Fireball as filler
        TryCast(SPELL_FIREBALL, target);
    }
    
    void ExecuteBurstWindow(Unit* target) {
        // During Combustion, spam instant Pyroblasts
        while (_bot->HasAura(SPELL_COMBUSTION)) {
            if (_hotStreak > 0) {
                TryCast(SPELL_PYROBLAST_INSTANT, target);
            } else {
                TryCast(SPELL_FIRE_BLAST, target);
                TryCast(SPELL_PHOENIX_FLAMES, target);
            }
        }
    }
    
    void UpdateProcs() {
        _heatingUp = _bot->HasAura(SPELL_HEATING_UP) ? 1 : 0;
        _hotStreak = _bot->HasAura(SPELL_HOT_STREAK) ? 1 : 0;
    }
};

} // namespace MageRotations
```

### 5. Rogue Rotations

```cpp
// src/modules/Playerbot/AI/Rotations/RogueRotations.h

namespace RogueRotations {

// Assassination Rogue
class AssassinationRotation : public RotationExecutor {
private:
    uint32 _comboPoints = 0;
    uint32 _energy = 0;
    
public:
    void ExecuteNormal(Unit* target) override {
        UpdateResources();
        
        // Maintain Slice and Dice
        if (!_bot->HasAura(SPELL_SLICE_AND_DICE) && _comboPoints >= 2) {
            TryCast(SPELL_SLICE_AND_DICE);
            return;
        }
        
        // Apply/Maintain poisons
        if (!target->HasAura(SPELL_DEADLY_POISON_DOT, _bot->GetGUID())) {
            TryCast(SPELL_POISONED_KNIFE, target);
        }
        
        // Maintain Rupture
        if (_comboPoints >= 4 && !target->HasAura(SPELL_RUPTURE, _bot->GetGUID())) {
            TryCast(SPELL_RUPTURE, target);
            return;
        }
        
        // Envenom at 4+ CP
        if (_comboPoints >= 4) {
            TryCast(SPELL_ENVENOM, target);
            return;
        }
        
        // Build combo points
        if (_energy > 50) {
            TryCast(SPELL_MUTILATE, target);
        }
    }
    
    void UpdateResources() {
        _comboPoints = _bot->GetComboPoints();
        _energy = _bot->GetPower(POWER_ENERGY);
    }
    
    void ExecuteOpener(Unit* target) override {
        // Stealth opener
        if (_bot->HasAura(SPELL_STEALTH)) {
            TryCast(SPELL_CHEAP_SHOT, target);  // Stun
            TryCast(SPELL_GARROTE, target);     // Bleed
        }
    }
};

} // namespace RogueRotations
```

## Rotation Optimization Strategies

### 1. Proc Tracking System

```cpp
// src/modules/Playerbot/AI/Rotations/ProcTracker.h

class ProcTracker {
public:
    struct ProcInfo {
        uint32 auraId;
        uint32 stacks;
        uint32 expiresAt;
        float value;  // Relative importance
    };
    
    void Update() {
        _procs.clear();
        
        // Scan all auras for known procs
        Unit::AuraApplicationMap const& auras = _bot->GetAppliedAuras();
        for (auto const& [auraId, aurApp] : auras) {
            if (IsTrackedProc(auraId)) {
                ProcInfo info;
                info.auraId = auraId;
                info.stacks = aurApp->GetBase()->GetStackAmount();
                info.expiresAt = aurApp->GetBase()->GetDuration();
                info.value = GetProcValue(auraId);
                _procs.push_back(info);
            }
        }
        
        // Sort by value
        std::sort(_procs.begin(), _procs.end(),
            [](const ProcInfo& a, const ProcInfo& b) {
                return a.value > b.value;
            });
    }
    
    bool HasProc(uint32 auraId) const {
        return std::find_if(_procs.begin(), _procs.end(),
            [auraId](const ProcInfo& p) { return p.auraId == auraId; }
        ) != _procs.end();
    }
    
private:
    Player* _bot;
    std::vector<ProcInfo> _procs;
    
    bool IsTrackedProc(uint32 auraId) {
        // List of important procs to track
        static phmap::flat_hash_set<uint32> trackedProcs = {
            SPELL_FINGERS_OF_FROST,
            SPELL_BRAIN_FREEZE,
            SPELL_HOT_STREAK,
            SPELL_SUDDEN_DEATH,
            SPELL_REVENGE_PROC,
            // ... more procs
        };
        return trackedProcs.count(auraId) > 0;
    }
};
```

### 2. Snapshot Management

```cpp
// src/modules/Playerbot/AI/Rotations/SnapshotManager.h

class SnapshotManager {
public:
    struct Snapshot {
        uint32 spellId;
        uint32 targetGuid;
        float spellPower;
        float critChance;
        float hasteRating;
        uint32 timestamp;
    };
    
    void TakeSnapshot(uint32 spellId, Unit* target) {
        Snapshot snap;
        snap.spellId = spellId;
        snap.targetGuid = target->GetGUID().GetRawValue();
        snap.spellPower = _bot->GetTotalSpellPowerValue();
        snap.critChance = _bot->GetSpellCritChance();
        snap.hasteRating = _bot->GetRatingBonusValue(CR_HASTE_SPELL);
        snap.timestamp = getMSTime();
        
        _snapshots[GetKey(spellId, target)] = snap;
    }
    
    bool ShouldRefreshDoT(uint32 spellId, Unit* target) {
        auto key = GetKey(spellId, target);
        auto it = _snapshots.find(key);
        if (it == _snapshots.end())
            return true;
        
        // Refresh if current stats are 10% better
        float currentPower = _bot->GetTotalSpellPowerValue();
        return currentPower > it->second.spellPower * 1.1f;
    }
    
private:
    phmap::flat_hash_map<uint64, Snapshot> _snapshots;
    
    uint64 GetKey(uint32 spellId, Unit* target) {
        return (uint64(spellId) << 32) | target->GetGUID().GetRawValue();
    }
};
```

## Performance Metrics

### Rotation Performance Targets

```cpp
// Maximum execution time per rotation update
constexpr uint32 MAX_ROTATION_UPDATE_US = 100;  // 100 microseconds

// Measure rotation performance
class RotationProfiler {
public:
    void StartMeasure() {
        _start = std::chrono::high_resolution_clock::now();
    }
    
    void EndMeasure(const std::string& rotationName) {
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>
                       (end - _start).count();
        
        if (duration > MAX_ROTATION_UPDATE_US) {
            TC_LOG_WARN("playerbot", 
                "Rotation %s took %u us (limit: %u us)",
                rotationName.c_str(), duration, MAX_ROTATION_UPDATE_US);
        }
        
        // Track statistics
        _totalTime += duration;
        _updateCount++;
        
        if (_updateCount % 1000 == 0) {
            TC_LOG_DEBUG("playerbot",
                "Rotation %s average: %u us",
                rotationName.c_str(), _totalTime / _updateCount);
        }
    }
    
private:
    std::chrono::high_resolution_clock::time_point _start;
    uint64 _totalTime = 0;
    uint32 _updateCount = 0;
};
```

## Next Steps

1. **Implement Rotation Executor** - Base rotation framework
2. **Add Proc Tracking** - Dynamic proc monitoring
3. **Create Class Rotations** - All 13 classes
4. **Performance Testing** - Ensure <100μs execution
5. **PvP Adaptations** - PvP-specific rotations

---

**Status**: Ready for Implementation
**Dependencies**: ClassAI Framework
**Estimated Time**: Sprint 3A Days 4-7