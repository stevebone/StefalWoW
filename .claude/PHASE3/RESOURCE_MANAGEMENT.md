# Resource Management System - Phase 3 Sprint 3B
## Comprehensive Resource Tracking and Optimization

## Overview

This document details the unified resource management system for all class resources including mana, rage, energy, focus, runic power, and specialized resources. The system provides efficient tracking, prediction, and optimization for resource-based decision making.

## Core Resource Framework

### Unified Resource Manager

```cpp
// src/modules/Playerbot/AI/Resources/UnifiedResourceManager.h

class UnifiedResourceManager {
public:
    enum ResourceType {
        RESOURCE_HEALTH = 0,
        RESOURCE_MANA = 1,
        RESOURCE_RAGE = 2,
        RESOURCE_FOCUS = 3,
        RESOURCE_ENERGY = 4,
        RESOURCE_COMBO_POINTS = 5,
        RESOURCE_RUNES = 6,
        RESOURCE_RUNIC_POWER = 7,
        RESOURCE_SOUL_SHARDS = 8,
        RESOURCE_LUNAR_POWER = 9,
        RESOURCE_HOLY_POWER = 10,
        RESOURCE_MAELSTROM = 11,
        RESOURCE_CHI = 12,
        RESOURCE_INSANITY = 13,
        RESOURCE_FURY = 14,
        RESOURCE_PAIN = 15,
        RESOURCE_ESSENCE = 16
    };
    
    struct ResourceInfo {
        ResourceType type;
        int32 current;
        int32 maximum;
        float regenRate;
        uint32 lastUpdate;
        
        // Predictions
        int32 predicted;      // Predicted value after pending actions
        uint32 nextTick;      // Next regen tick
        
        // Thresholds
        int32 conserveThreshold;  // Start conserving
        int32 dumpThreshold;      // Start dumping excess
    };
    
    UnifiedResourceManager(Player* bot);
    
    // Resource queries
    int32 GetCurrent(ResourceType type) const;
    int32 GetMax(ResourceType type) const;
    float GetPercent(ResourceType type) const;
    float GetRegenRate(ResourceType type) const;
    
    // Resource prediction
    int32 PredictResource(ResourceType type, uint32 timeMs) const;
    bool WillHaveResource(ResourceType type, int32 amount, uint32 timeMs) const;
    uint32 TimeUntilResource(ResourceType type, int32 amount) const;
    
    // Resource costs
    int32 GetSpellCost(uint32 spellId, ResourceType type) const;
    bool CanAffordSpell(uint32 spellId) const;
    bool CanAffordRotation(const std::vector<uint32>& spells) const;
    
    // Resource optimization
    bool ShouldConserve(ResourceType type) const;
    bool ShouldDump(ResourceType type) const;
    float GetEfficiency(uint32 spellId) const;  // Damage per resource
    
    // Update system
    void Update(uint32 diff);
    void OnSpellCast(uint32 spellId);
    void OnAuraApplied(uint32 auraId);
    void OnAuraRemoved(uint32 auraId);
    
private:
    Player* _bot;
    std::array<ResourceInfo, 17> _resources;
    phmap::flat_hash_map<uint32, int32> _spellCostCache;
    
    // Resource regeneration calculations
    float CalculateManaRegen() const;
    float CalculateEnergyRegen() const;
    float CalculateFocusRegen() const;
    
    // Class-specific calculations
    void UpdateComboPoints();
    void UpdateHolyPower();
    void UpdateChi();
    void UpdateSoulShards();
};
```

### Resource Prediction Engine

```cpp
// src/modules/Playerbot/AI/Resources/ResourcePredictor.h

class ResourcePredictor {
public:
    struct Prediction {
        int32 value;
        float confidence;  // 0.0 to 1.0
        uint32 timestamp;
    };
    
    // Predict resource state after action sequence
    Prediction PredictAfterSequence(
        const std::vector<uint32>& spells,
        ResourceType resource,
        uint32 gcdMs = 1500
    ) {
        Prediction pred;
        pred.value = GetCurrent(resource);
        pred.confidence = 1.0f;
        pred.timestamp = getMSTime();
        
        uint32 totalTime = 0;
        
        for (uint32 spellId : spells) {
            // Account for cost
            int32 cost = GetSpellCost(spellId, resource);
            pred.value -= cost;
            
            // Account for generation
            int32 generation = GetSpellGeneration(spellId, resource);
            pred.value += generation;
            
            // Account for cast time
            uint32 castTime = GetCastTime(spellId);
            totalTime += std::max(castTime, gcdMs);
            
            // Account for regeneration during cast
            float regenDuringCast = GetRegenRate(resource) * (totalTime / 1000.0f);
            pred.value += int32(regenDuringCast);
            
            // Reduce confidence for longer predictions
            pred.confidence *= 0.95f;
            
            // Clamp to valid range
            pred.value = std::min(pred.value, GetMax(resource));
            pred.value = std::max(pred.value, 0);
        }
        
        return pred;
    }
    
    // Machine learning-based prediction
    Prediction PredictWithML(ResourceType resource, uint32 futureMs) {
        // Use historical data to predict future resource state
        auto history = GetResourceHistory(resource);
        
        // Simple linear regression for now
        float slope = CalculateSlope(history);
        float intercept = history.back().value;
        
        Prediction pred;
        pred.value = int32(intercept + slope * (futureMs / 1000.0f));
        pred.confidence = CalculateConfidence(history, slope);
        pred.timestamp = getMSTime() + futureMs;
        
        return pred;
    }
    
private:
    struct HistoryPoint {
        int32 value;
        uint32 timestamp;
    };
    
    // Circular buffer for resource history
    boost::circular_buffer<HistoryPoint> _history{100};
    
    float CalculateSlope(const boost::circular_buffer<HistoryPoint>& history) {
        if (history.size() < 2)
            return 0.0f;
        
        // Simple linear regression
        float sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0;
        uint32 baseTime = history.front().timestamp;
        
        for (const auto& point : history) {
            float x = (point.timestamp - baseTime) / 1000.0f;
            float y = point.value;
            
            sumX += x;
            sumY += y;
            sumXY += x * y;
            sumX2 += x * x;
        }
        
        float n = history.size();
        return (n * sumXY - sumX * sumY) / (n * sumX2 - sumX * sumX);
    }
};
```

## Class-Specific Resource Optimization

### Mana Management (Healers/Casters)

```cpp
// src/modules/Playerbot/AI/Resources/ManaManager.h

class ManaManager : public UnifiedResourceManager {
public:
    struct ManaStrategy {
        float conservePercent;     // Start conserving at this %
        float emergencyPercent;    // Only emergency spells below this
        bool usePotion;           // Use mana potion
        bool useManaGem;          // Use mana gem (mage)
        bool useEvocation;        // Use evocation (mage)
        bool useInnervate;        // Request innervate (druid)
    };
    
    ManaStrategy DetermineManaStrategy(CombatContext& context) {
        ManaStrategy strategy;
        
        float manaPercent = GetPercent(RESOURCE_MANA);
        float combatTimeRemaining = EstimateCombatTime(context);
        
        // Long fight - be conservative
        if (combatTimeRemaining > 180.0f) {
            strategy.conservePercent = 70.0f;
            strategy.emergencyPercent = 30.0f;
        }
        // Short fight - can be aggressive
        else if (combatTimeRemaining < 30.0f) {
            strategy.conservePercent = 30.0f;
            strategy.emergencyPercent = 10.0f;
        }
        // Normal fight
        else {
            strategy.conservePercent = 50.0f;
            strategy.emergencyPercent = 20.0f;
        }
        
        // Use consumables/abilities
        strategy.usePotion = manaPercent < 30.0f && !_potionOnCooldown;
        strategy.useManaGem = manaPercent < 40.0f && HasManaGem();
        strategy.useEvocation = manaPercent < 35.0f && CanEvocate();
        strategy.useInnervate = manaPercent < 25.0f;
        
        return strategy;
    }
    
    // Mana-efficient spell selection
    uint32 SelectManaEfficientSpell(const std::vector<uint32>& options) {
        uint32 bestSpell = 0;
        float bestEfficiency = 0.0f;
        
        for (uint32 spellId : options) {
            float efficiency = GetSpellEfficiency(spellId);
            if (efficiency > bestEfficiency && CanAffordSpell(spellId)) {
                bestSpell = spellId;
                bestEfficiency = efficiency;
            }
        }
        
        return bestSpell;
    }
    
private:
    float GetSpellEfficiency(uint32 spellId) {
        // Calculate healing/damage per mana
        int32 cost = GetSpellCost(spellId, RESOURCE_MANA);
        if (cost <= 0)
            return 999.0f;  // Free spell
        
        float output = GetSpellOutput(spellId);  // Healing or damage
        return output / cost;
    }
};
```

### Energy/Combo Point Management (Rogues/Feral)

```cpp
// src/modules/Playerbot/AI/Resources/EnergyComboManager.h

class EnergyComboManager : public UnifiedResourceManager {
public:
    struct ComboDecision {
        bool shouldPoolEnergy;
        bool shouldSpendCombo;
        uint32 targetComboPoints;
        uint32 finisherSpell;
    };
    
    ComboDecision MakeComboDecision(Unit* target) {
        ComboDecision decision;
        
        uint32 combo = GetCurrent(RESOURCE_COMBO_POINTS);
        uint32 energy = GetCurrent(RESOURCE_ENERGY);
        
        // Check for important buffs/debuffs expiring
        bool sliceAndDiceExpiring = GetBuffRemaining(SPELL_SLICE_AND_DICE) < 5000;
        bool ruptureExpiring = GetDebuffRemaining(target, SPELL_RUPTURE) < 5000;
        
        // Maintenance takes priority
        if (sliceAndDiceExpiring && combo >= 1) {
            decision.shouldSpendCombo = true;
            decision.targetComboPoints = 1;  // Minimum for refresh
            decision.finisherSpell = SPELL_SLICE_AND_DICE;
        }
        else if (ruptureExpiring && combo >= 1) {
            decision.shouldSpendCombo = true;
            decision.targetComboPoints = std::max(combo, 4u);  // Ideal 4-5
            decision.finisherSpell = SPELL_RUPTURE;
        }
        // Normal rotation
        else if (combo >= 5) {
            decision.shouldSpendCombo = true;
            decision.targetComboPoints = 5;
            decision.finisherSpell = SelectFinisher(target);
        }
        else {
            decision.shouldSpendCombo = false;
            decision.targetComboPoints = 5;
        }
        
        // Pool energy for burst windows
        decision.shouldPoolEnergy = ShouldPoolEnergy(energy);
        
        return decision;
    }
    
    bool ShouldPoolEnergy(uint32 currentEnergy) {
        // Pool for Shadow Dance, Vendetta, etc.
        if (CooldownReady(SPELL_SHADOW_DANCE) && 
            CooldownRemaining(SPELL_SHADOW_DANCE) < 5000) {
            return currentEnergy < 80;
        }
        
        // Pool for Vanish burst
        if (CooldownReady(SPELL_VANISH) && 
            CooldownRemaining(SPELL_VANISH) < 3000) {
            return currentEnergy < 60;
        }
        
        // Normal - don't overcap
        return currentEnergy >= 95;
    }
    
private:
    uint32 SelectFinisher(Unit* target) {
        float healthPercent = target->GetHealthPct();
        
        // Execute range
        if (healthPercent < 35)
            return SPELL_EVISCERATE;  // Maximum damage
        
        // Long fight - maintain Rupture
        if (EstimateTimeToKill(target) > 20.0f)
            return SPELL_RUPTURE;
        
        // Default damage finisher
        return SPELL_EVISCERATE;
    }
};
```

### Rage Management (Warriors)

```cpp
// src/modules/Playerbot/AI/Resources/RageManager.h

class RageManager : public UnifiedResourceManager {
public:
    struct RageDecision {
        bool shouldDump;
        bool shouldPool;
        uint32 dumpSpell;
        uint32 poolTarget;
    };
    
    RageDecision MakeRageDecision(CombatContext& context) {
        RageDecision decision;
        
        uint32 rage = GetCurrent(RESOURCE_RAGE);
        
        // About to use important ability
        if (CooldownRemaining(SPELL_COLOSSUS_SMASH) < 2000) {
            decision.shouldPool = true;
            decision.poolTarget = 30;  // Need rage for follow-up
        }
        // Execute phase
        else if (context.targetHealthPercent < 20) {
            decision.shouldDump = rage > 40;
            decision.dumpSpell = SPELL_EXECUTE;
        }
        // Normal - prevent capping
        else if (rage > 85) {
            decision.shouldDump = true;
            decision.dumpSpell = SelectRageDump(context);
        }
        // Defensive stance needs rage for Shield Block
        else if (IsInDefensiveStance() && rage < 20) {
            decision.shouldPool = true;
            decision.poolTarget = 20;
        }
        
        return decision;
    }
    
    // Dynamic rage generation tracking
    float GetRageGeneration() {
        float baseGen = 0.0f;
        
        // Auto-attack rage
        if (_bot->IsInCombat()) {
            float weaponSpeed = _bot->GetAttackTime(BASE_ATTACK) / 1000.0f;
            baseGen += CalculateAutoAttackRage(weaponSpeed);
        }
        
        // Talent modifiers
        if (HasTalent(TALENT_ENDLESS_RAGE))
            baseGen *= 1.25f;
        
        return baseGen;
    }
    
private:
    uint32 SelectRageDump(CombatContext& context) {
        if (context.enemyCount > 2)
            return SPELL_WHIRLWIND;
        
        if (HasBuff(SPELL_SUDDEN_DEATH))
            return SPELL_EXECUTE;
        
        return SPELL_SLAM;  // Default single target
    }
    
    float CalculateAutoAttackRage(float weaponSpeed) {
        // Formula based on weapon damage and speed
        float damage = _bot->GetWeaponDamageRange(BASE_ATTACK).second;
        return (damage / weaponSpeed) * 0.0091f * 7.5f;
    }
};
```

## Resource Pool Optimization

### Object Pooling for Performance

```cpp
// src/modules/Playerbot/AI/Resources/ResourcePool.h

template<typename T>
class ResourcePool {
public:
    ResourcePool(size_t initialSize = 100) {
        for (size_t i = 0; i < initialSize; ++i) {
            _available.push(std::make_unique<T>());
        }
    }
    
    T* Acquire() {
        std::lock_guard<std::mutex> lock(_mutex);
        
        if (_available.empty()) {
            return new T();  // Allocate new if pool empty
        }
        
        T* obj = _available.top().release();
        _available.pop();
        _inUse.insert(obj);
        return obj;
    }
    
    void Release(T* obj) {
        std::lock_guard<std::mutex> lock(_mutex);
        
        if (_inUse.erase(obj)) {
            obj->Reset();  // Reset to default state
            _available.push(std::unique_ptr<T>(obj));
        }
    }
    
    size_t GetPoolSize() const {
        return _available.size() + _inUse.size();
    }
    
private:
    std::stack<std::unique_ptr<T>> _available;
    std::unordered_set<T*> _inUse;
    mutable std::mutex _mutex;
};

// Resource calculation objects pooled for efficiency
static ResourcePool<ResourceInfo> resourceInfoPool(1000);
static ResourcePool<Prediction> predictionPool(500);
```

## Performance Monitoring

### Resource System Metrics

```cpp
// src/modules/Playerbot/AI/Resources/ResourceMetrics.h

class ResourceMetrics {
public:
    struct Metrics {
        // Timing
        uint32 avgUpdateTimeUs;
        uint32 maxUpdateTimeUs;
        
        // Accuracy
        float predictionAccuracy;
        uint32 mispredictions;
        
        // Efficiency
        float resourceWaste;      // Overcapping
        float resourceStarvation;  // Time spent empty
        
        // Memory
        size_t memoryUsage;
        uint32 cacheHitRate;
    };
    
    void RecordUpdate(uint32 microseconds) {
        _updateTimes.push_back(microseconds);
        if (_updateTimes.size() > 1000)
            _updateTimes.pop_front();
        
        // Calculate averages
        uint32 sum = 0;
        _metrics.maxUpdateTimeUs = 0;
        for (uint32 time : _updateTimes) {
            sum += time;
            _metrics.maxUpdateTimeUs = std::max(_metrics.maxUpdateTimeUs, time);
        }
        _metrics.avgUpdateTimeUs = sum / _updateTimes.size();
    }
    
    void RecordPrediction(int32 predicted, int32 actual) {
        _totalPredictions++;
        
        int32 error = abs(predicted - actual);
        if (error <= 5) {  // Within 5 resource tolerance
            _accuratePredictions++;
        } else {
            _metrics.mispredictions++;
        }
        
        _metrics.predictionAccuracy = 
            float(_accuratePredictions) / _totalPredictions;
    }
    
    void RecordWaste(ResourceType type, int32 amount) {
        _wastedResources[type] += amount;
    }
    
    Metrics GetMetrics() const { return _metrics; }
    
private:
    Metrics _metrics;
    std::deque<uint32> _updateTimes;
    uint32 _totalPredictions = 0;
    uint32 _accuratePredictions = 0;
    phmap::flat_hash_map<ResourceType, int32> _wastedResources;
};
```

## Testing Framework

```cpp
// src/modules/Playerbot/Tests/ResourceManagementTest.cpp

TEST_F(ResourceManagementTest, ManaPrediction) {
    auto priest = CreateTestBot(CLASS_PRIEST, 80);
    UnifiedResourceManager manager(priest);
    
    // Set known mana state
    priest->SetPower(POWER_MANA, 10000);
    priest->SetMaxPower(POWER_MANA, 20000);
    
    // Test prediction after spell sequence
    std::vector<uint32> spells = {
        SPELL_POWER_WORD_SHIELD,  // 1000 mana
        SPELL_FLASH_HEAL,          // 800 mana
        SPELL_RENEW                // 500 mana
    };
    
    auto prediction = manager.PredictAfterSequence(
        spells, RESOURCE_MANA);
    
    // Should predict: 10000 - 2300 + regen
    EXPECT_NEAR(prediction.value, 7700, 100);  // Allow for regen variance
}

TEST_F(ResourceManagementTest, EnergyPooling) {
    auto rogue = CreateTestBot(CLASS_ROGUE, 80);
    EnergyComboManager manager(rogue);
    
    // Test pooling decision
    rogue->SetPower(POWER_ENERGY, 75);
    
    // Should pool when Shadow Dance coming up
    SetCooldown(SPELL_SHADOW_DANCE, 3000);
    EXPECT_TRUE(manager.ShouldPoolEnergy(75));
    
    // Should not pool when no CDs
    SetCooldown(SPELL_SHADOW_DANCE, 60000);
    EXPECT_FALSE(manager.ShouldPoolEnergy(75));
}

TEST_F(ResourceManagementTest, RageEfficiency) {
    auto warrior = CreateTestBot(CLASS_WARRIOR, 80);
    RageManager manager(warrior);
    
    // Test rage decision making
    warrior->SetPower(POWER_RAGE, 90);
    
    auto decision = manager.MakeRageDecision(context);
    EXPECT_TRUE(decision.shouldDump);
    EXPECT_EQ(decision.dumpSpell, SPELL_SLAM);
}
```

## Next Steps

1. **Implement Unified Manager** - Core resource framework
2. **Add Prediction Engine** - Resource forecasting
3. **Create Class Managers** - Specialized resource logic
4. **Add Object Pooling** - Performance optimization
5. **Implement Metrics** - Performance monitoring

---

**Status**: Ready for Implementation
**Dependencies**: Advanced Mechanics
**Estimated Time**: Sprint 3B Days 2-4