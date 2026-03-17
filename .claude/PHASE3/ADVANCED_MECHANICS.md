# Advanced Class Mechanics - Phase 3 Sprint 3B
## Complex Class-Specific Systems

## Overview

This document details the implementation of advanced class mechanics including Death Knight runes, Warlock soul shards, Druid forms, and other complex resource systems. These mechanics require sophisticated tracking and decision-making algorithms.

## Death Knight Rune System

### Rune Management Implementation

```cpp
// src/modules/Playerbot/AI/ClassAI/DeathKnight/RuneManager.h

class RuneManager {
public:
    enum RuneType {
        RUNE_BLOOD = 0,
        RUNE_FROST = 1,
        RUNE_UNHOLY = 2,
        RUNE_DEATH = 3  // Death runes can be used as any type
    };
    
    struct Rune {
        RuneType baseType;
        RuneType currentType;
        bool isReady;
        uint32 cooldownEnd;
        bool isDeathRune;
    };
    
    RuneManager(Player* dk);
    
    // Rune queries
    uint8 GetAvailableRunes(RuneType type) const;
    bool CanCastSpell(uint32 spellId) const;
    uint32 GetNextRuneReadyTime(RuneType type) const;
    
    // Rune consumption
    void ConsumeRunes(uint32 spellId);
    void RefreshRune(uint8 runeIndex);
    
    // Death rune conversion
    void ConvertToDeathRune(uint8 runeIndex);
    bool HasDeathRunes() const;
    
    // Update cooldowns
    void Update(uint32 diff);
    
private:
    std::array<Rune, 6> _runes;  // 2 of each type
    Player* _player;
    
    // Rune regeneration
    static constexpr uint32 RUNE_COOLDOWN = 10000;  // 10 seconds base
    
    float GetHasteMultiplier() const;
    uint32 GetRuneCooldown() const;
};

// src/modules/Playerbot/AI/ClassAI/DeathKnight/DeathKnightAI.cpp

class DeathKnightAI : public ClassAI {
private:
    RuneManager _runeManager;
    
    // Disease tracking
    struct DiseaseInfo {
        bool hasBloodPlague;
        bool hasFrostFever;
        uint32 bloodPlagueExpires;
        uint32 frostFeverExpires;
    };
    
    phmap::flat_hash_map<ObjectGuid, DiseaseInfo> _diseases;
    
public:
    void UpdateRotation(Unit* target) override {
        _runeManager.Update(diff);
        UpdateDiseases(target);
        
        // Unholy presence for DPS
        if (!_bot->HasAura(SPELL_UNHOLY_PRESENCE)) {
            TryCast(SPELL_UNHOLY_PRESENCE);
        }
        
        // Apply diseases first
        if (!HasBothDiseases(target)) {
            ApplyDiseases(target);
            return;
        }
        
        // Death and Decay for AoE
        if (GetNearbyEnemies(10.0f).size() >= 3) {
            if (_runeManager.GetAvailableRunes(RuneType::RUNE_UNHOLY) >= 1) {
                TryCast(SPELL_DEATH_AND_DECAY);
            }
        }
        
        // Obliterate/Scourge Strike based on spec
        ExecuteSpecRotation(target);
        
        // Runic Power dump
        if (_bot->GetPower(POWER_RUNIC_POWER) > 80) {
            TryCast(SPELL_DEATH_COIL, target);
        }
    }
    
private:
    void ApplyDiseases(Unit* target) {
        // Outbreak for instant application
        if (IsSpellReady(SPELL_OUTBREAK)) {
            TryCast(SPELL_OUTBREAK, target);
            return;
        }
        
        // Manual application
        if (!target->HasAura(SPELL_FROST_FEVER)) {
            if (_runeManager.GetAvailableRunes(RuneType::RUNE_FROST) >= 1) {
                TryCast(SPELL_ICY_TOUCH, target);
            }
        }
        
        if (!target->HasAura(SPELL_BLOOD_PLAGUE)) {
            if (_runeManager.GetAvailableRunes(RuneType::RUNE_UNHOLY) >= 1) {
                TryCast(SPELL_PLAGUE_STRIKE, target);
            }
        }
    }
    
    void ExecuteFrostRotation(Unit* target) {
        // Obliterate spam with Killing Machine procs
        if (_bot->HasAura(SPELL_KILLING_MACHINE)) {
            if (_runeManager.CanCastSpell(SPELL_OBLITERATE)) {
                TryCast(SPELL_OBLITERATE, target);
                return;
            }
        }
        
        // Howling Blast with Rime proc
        if (_bot->HasAura(SPELL_FREEZING_FOG)) {
            TryCast(SPELL_HOWLING_BLAST, target);
            return;
        }
        
        // Frost Strike runic dump
        if (_bot->GetPower(POWER_RUNIC_POWER) >= 25) {
            TryCast(SPELL_FROST_STRIKE, target);
        }
    }
};
```

## Warlock Soul Shard & Pet System

### Soul Shard Management

```cpp
// src/modules/Playerbot/AI/ClassAI/Warlock/SoulShardManager.h

class SoulShardManager {
public:
    SoulShardManager(Player* warlock) : _player(warlock) {}
    
    // Soul shard queries
    uint32 GetShardCount() const;
    bool HasShards(uint32 count = 1) const;
    
    // Shard generation
    void OnTargetDeath(Unit* target);
    bool ShouldDrainSoul(Unit* target) const;
    
    // Shard consumption decisions
    bool ShouldCreateHealthstone() const;
    bool ShouldCreateSoulstone() const;
    bool ShouldSummonPet() const;
    
private:
    Player* _player;
    
    uint32 CountItemsInBags(uint32 itemId) const {
        return _player->GetItemCount(ITEM_SOUL_SHARD);
    }
};

// src/modules/Playerbot/AI/ClassAI/Warlock/WarlockPetAI.h

class WarlockPetAI {
public:
    enum PetType {
        PET_IMP,
        PET_VOIDWALKER,
        PET_SUCCUBUS,
        PET_FELHUNTER,
        PET_FELGUARD
    };
    
    WarlockPetAI(Player* warlock) : _player(warlock) {}
    
    // Pet selection
    PetType SelectBestPet(CombatContext& context) {
        if (context.needsTank)
            return PET_VOIDWALKER;
        
        if (context.targetIsCaster)
            return PET_FELHUNTER;
        
        if (context.needsCC)
            return PET_SUCCUBUS;
        
        // Default DPS pet
        return _player->GetSpecialization() == SPEC_DEMONOLOGY ? 
               PET_FELGUARD : PET_IMP;
    }
    
    // Pet control
    void CommandPet(Unit* target) {
        Pet* pet = _player->GetPet();
        if (!pet)
            return;
        
        // Specific pet abilities
        switch (GetPetType(pet)) {
            case PET_VOIDWALKER:
                // Taunt if needed
                if (!target->GetVictim() || target->GetVictim() != pet) {
                    pet->CastSpell(target, SPELL_TORMENT, false);
                }
                break;
                
            case PET_FELHUNTER:
                // Interrupt/dispel
                if (target->IsNonMeleeSpellCast(false)) {
                    pet->CastSpell(target, SPELL_SPELL_LOCK, false);
                }
                break;
                
            case PET_SUCCUBUS:
                // Seduce CC
                if (context.needsCC) {
                    pet->CastSpell(target, SPELL_SEDUCTION, false);
                }
                break;
        }
    }
    
    void ManagePetHealth() {
        Pet* pet = _player->GetPet();
        if (!pet)
            return;
        
        if (pet->GetHealthPct() < 50) {
            _player->CastSpell(pet, SPELL_HEALTH_FUNNEL, false);
        }
    }
    
private:
    Player* _player;
    CombatContext context;
};
```

### Warlock DoT Management

```cpp
// src/modules/Playerbot/AI/ClassAI/Warlock/WarlockDoTManager.h

class WarlockDoTManager {
public:
    struct DoTInfo {
        uint32 spellId;
        uint32 duration;
        uint32 appliedAt;
        float snapshotPower;
        bool pandemic;  // Can refresh in last 30%
    };
    
    void TrackDoT(Unit* target, uint32 spellId) {
        DoTInfo info;
        info.spellId = spellId;
        info.appliedAt = getMSTime();
        info.duration = GetDoTDuration(spellId);
        info.snapshotPower = _player->GetTotalSpellPowerValue();
        info.pandemic = true;
        
        _dots[target->GetGUID()][spellId] = info;
    }
    
    bool ShouldRefreshDoT(Unit* target, uint32 spellId) {
        auto targetDots = _dots.find(target->GetGUID());
        if (targetDots == _dots.end())
            return true;
        
        auto dot = targetDots->second.find(spellId);
        if (dot == targetDots->second.end())
            return true;
        
        uint32 elapsed = getMSTime() - dot->second.appliedAt;
        uint32 remaining = dot->second.duration - elapsed;
        
        // Pandemic window (last 30%)
        if (dot->second.pandemic && 
            remaining < dot->second.duration * 0.3f) {
            return true;
        }
        
        // Snapshot improvement (15% better stats)
        float currentPower = _player->GetTotalSpellPowerValue();
        if (currentPower > dot->second.snapshotPower * 1.15f) {
            return true;
        }
        
        return false;
    }
    
    void ApplyDoTs(Unit* target) {
        // Priority order for DoTs
        static const std::vector<uint32> dotPriority = {
            SPELL_AGONY,
            SPELL_CORRUPTION,
            SPELL_UNSTABLE_AFFLICTION,
            SPELL_SIPHON_LIFE
        };
        
        for (uint32 spellId : dotPriority) {
            if (ShouldRefreshDoT(target, spellId)) {
                if (_player->CastSpell(target, spellId, false) == SPELL_CAST_OK) {
                    TrackDoT(target, spellId);
                    return;  // One per GCD
                }
            }
        }
    }
    
private:
    Player* _player;
    phmap::flat_hash_map<ObjectGuid, 
        phmap::flat_hash_map<uint32, DoTInfo>> _dots;
};
```

## Druid Form Management

### Shape-shifting Logic

```cpp
// src/modules/Playerbot/AI/ClassAI/Druid/DruidFormManager.h

class DruidFormManager {
public:
    enum Form {
        FORM_NONE = 0,
        FORM_CAT = 1,
        FORM_TREE = 2,
        FORM_TRAVEL = 3,
        FORM_AQUATIC = 4,
        FORM_BEAR = 5,
        FORM_MOONKIN = 8,
        FORM_FLIGHT = 27
    };
    
    DruidFormManager(Player* druid) : _player(druid) {}
    
    // Form decisions
    Form SelectBestForm(CombatContext& context) {
        // Check specialization
        switch (_player->GetSpecialization()) {
            case SPEC_DRUID_BALANCE:
                return FORM_MOONKIN;
                
            case SPEC_DRUID_FERAL:
                if (context.role == ROLE_TANK)
                    return FORM_BEAR;
                return FORM_CAT;
                
            case SPEC_DRUID_RESTORATION:
                return FORM_TREE;  // If talented
                
            case SPEC_DRUID_GUARDIAN:
                return FORM_BEAR;
        }
        
        return FORM_NONE;
    }
    
    // Form shifting
    void ShiftToForm(Form form) {
        if (GetCurrentForm() == form)
            return;
        
        // Cancel current form if needed
        if (GetCurrentForm() != FORM_NONE) {
            _player->RemoveAurasByType(SPELL_AURA_MOD_SHAPESHIFT);
        }
        
        // Shift to new form
        uint32 spellId = GetFormSpell(form);
        if (spellId)
            _player->CastSpell(_player, spellId, true);
    }
    
    // Form-specific abilities
    bool CanUseAbilityInForm(uint32 spellId, Form form) {
        // Check form requirements from spell data
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
        if (!spellInfo)
            return false;
        
        // Check Stances requirement
        if (spellInfo->Stances) {
            if (!(spellInfo->Stances & (1 << form)))
                return false;
        }
        
        return true;
    }
    
    // Powershift for energy/rage reset
    void PowerShift() {
        Form current = GetCurrentForm();
        if (current == FORM_CAT || current == FORM_BEAR) {
            ShiftToForm(FORM_NONE);
            ShiftToForm(current);
        }
    }
    
private:
    Player* _player;
    
    Form GetCurrentForm() const {
        return Form(_player->GetShapeshiftForm());
    }
    
    uint32 GetFormSpell(Form form) const {
        switch (form) {
            case FORM_CAT: return SPELL_CAT_FORM;
            case FORM_BEAR: return SPELL_BEAR_FORM;
            case FORM_MOONKIN: return SPELL_MOONKIN_FORM;
            case FORM_TREE: return SPELL_TREE_OF_LIFE;
            default: return 0;
        }
    }
};

// src/modules/Playerbot/AI/ClassAI/Druid/FeralDruidAI.cpp

class FeralDruidAI : public DruidAI {
private:
    DruidFormManager _formManager;
    
    // Cat form tracking
    uint32 _comboPoints = 0;
    uint32 _energy = 0;
    bool _hasRip = false;
    bool _hasRake = false;
    
public:
    void UpdateRotation(Unit* target) override {
        // Ensure cat form
        _formManager.ShiftToForm(DruidFormManager::FORM_CAT);
        
        UpdateCatResources();
        
        // Maintain bleeds
        if (!_hasRake) {
            TryCast(SPELL_RAKE, target);
            return;
        }
        
        // Maintain Rip at 5 combo points
        if (_comboPoints == 5 && !_hasRip) {
            TryCast(SPELL_RIP, target);
            return;
        }
        
        // Ferocious Bite at 5 combo points
        if (_comboPoints == 5 && _hasRip) {
            TryCast(SPELL_FEROCIOUS_BITE, target);
            return;
        }
        
        // Build combo points
        if (_energy > 35) {
            TryCast(SPELL_SHRED, target);  // From behind
            TryCast(SPELL_MANGLE_CAT, target);  // From front
        }
        
        // Tiger's Fury for energy
        if (_energy < 30) {
            TryCast(SPELL_TIGERS_FURY);
        }
    }
    
    void UpdateCatResources() {
        _comboPoints = _bot->GetComboPoints();
        _energy = _bot->GetPower(POWER_ENERGY);
        
        if (Unit* target = _bot->GetVictim()) {
            _hasRip = target->HasAura(SPELL_RIP, _bot->GetGUID());
            _hasRake = target->HasAura(SPELL_RAKE, _bot->GetGUID());
        }
    }
};
```

## Monk Chi & Stagger System

### Chi Management

```cpp
// src/modules/Playerbot/AI/ClassAI/Monk/MonkResourceManager.h

class MonkResourceManager {
public:
    struct ChiState {
        uint32 current;
        uint32 maximum;
        bool hasTeachings;  // Teachings of the Monastery buff
    };
    
    ChiState GetChiState() const {
        ChiState state;
        state.current = _player->GetPower(POWER_CHI);
        state.maximum = _player->GetMaxPower(POWER_CHI);
        state.hasTeachings = _player->HasAura(SPELL_TEACHINGS_OF_MONASTERY);
        return state;
    }
    
    // Brewmaster stagger
    float GetStaggerAmount() const {
        float light = 0, moderate = 0, heavy = 0;
        
        if (Aura* aura = _player->GetAura(SPELL_LIGHT_STAGGER))
            light = aura->GetEffect(0)->GetAmount();
        if (Aura* aura = _player->GetAura(SPELL_MODERATE_STAGGER))
            moderate = aura->GetEffect(0)->GetAmount();
        if (Aura* aura = _player->GetAura(SPELL_HEAVY_STAGGER))
            heavy = aura->GetEffect(0)->GetAmount();
        
        return light + moderate + heavy;
    }
    
    bool ShouldPurifyStagger() const {
        float stagger = GetStaggerAmount();
        float maxHealth = _player->GetMaxHealth();
        
        // Purify if stagger > 60% of max health
        return stagger > maxHealth * 0.6f;
    }
    
    // Mistweaver mana tea
    uint32 GetManaTeaStacks() const {
        if (Aura* aura = _player->GetAura(SPELL_MANA_TEA_STACKS))
            return aura->GetStackAmount();
        return 0;
    }
    
private:
    Player* _player;
};
```

## Shaman Totem Management

### Totem System

```cpp
// src/modules/Playerbot/AI/ClassAI/Shaman/TotemManager.h

class TotemManager {
public:
    enum TotemSlot {
        TOTEM_FIRE = 0,
        TOTEM_EARTH = 1,
        TOTEM_WATER = 2,
        TOTEM_AIR = 3
    };
    
    struct TotemInfo {
        uint32 spellId;
        TotemSlot slot;
        uint32 duration;
        float range;
        bool offensive;
    };
    
    void UpdateTotems(CombatContext& context) {
        // Check existing totems
        for (int i = 0; i < MAX_TOTEM_SLOT; ++i) {
            if (Totem* totem = _player->GetTotem(TotemSlot(i))) {
                // Check if totem needs replacement
                if (ShouldReplaceTotem(totem, context)) {
                    totem->UnSummon();
                }
            }
        }
        
        // Place new totems
        PlaceOptimalTotems(context);
    }
    
    void PlaceOptimalTotems(CombatContext& context) {
        // Fire totem
        if (!HasTotem(TOTEM_FIRE)) {
            if (context.enemyCount > 2)
                PlaceTotem(SPELL_MAGMA_TOTEM);
            else
                PlaceTotem(SPELL_SEARING_TOTEM);
        }
        
        // Earth totem
        if (!HasTotem(TOTEM_EARTH)) {
            if (context.needsDefense)
                PlaceTotem(SPELL_STONECLAW_TOTEM);
            else
                PlaceTotem(SPELL_STRENGTH_OF_EARTH_TOTEM);
        }
        
        // Water totem
        if (!HasTotem(TOTEM_WATER)) {
            if (context.needsHealing)
                PlaceTotem(SPELL_HEALING_STREAM_TOTEM);
            else
                PlaceTotem(SPELL_MANA_SPRING_TOTEM);
        }
        
        // Air totem
        if (!HasTotem(TOTEM_AIR)) {
            if (context.needsHaste)
                PlaceTotem(SPELL_WINDFURY_TOTEM);
            else
                PlaceTotem(SPELL_WRATH_OF_AIR_TOTEM);
        }
    }
    
    bool HasTotem(TotemSlot slot) const {
        return _player->GetTotem(slot) != nullptr;
    }
    
private:
    Player* _player;
    
    void PlaceTotem(uint32 spellId) {
        _player->CastSpell(_player, spellId, false);
    }
    
    bool ShouldReplaceTotem(Totem* totem, CombatContext& context) {
        // Replace if totem is about to expire
        if (totem->GetDuration() < 5000)
            return true;
        
        // Replace if better totem needed
        // ... context-specific logic
        
        return false;
    }
};
```

## Demon Hunter Metamorphosis

### Fury/Pain Management

```cpp
// src/modules/Playerbot/AI/ClassAI/DemonHunter/DemonHunterResourceManager.h

class DemonHunterResourceManager {
public:
    enum Spec {
        HAVOC,    // Uses Fury
        VENGEANCE // Uses Pain
    };
    
    // Havoc Fury management
    struct FuryState {
        uint32 current;
        uint32 maximum;
        bool hasMetamorphosis;
        uint32 metamorphosisRemaining;
    };
    
    FuryState GetFuryState() const {
        FuryState state;
        state.current = _player->GetPower(POWER_FURY);
        state.maximum = _player->GetMaxPower(POWER_FURY);
        state.hasMetamorphosis = _player->HasAura(SPELL_METAMORPHOSIS_HAVOC);
        
        if (Aura* meta = _player->GetAura(SPELL_METAMORPHOSIS_HAVOC))
            state.metamorphosisRemaining = meta->GetDuration();
        
        return state;
    }
    
    // Vengeance Pain management
    uint32 GetPain() const {
        return _player->GetPower(POWER_PAIN);
    }
    
    bool ShouldUseDemonSpikes() const {
        // Use when taking physical damage
        return GetPain() >= 20 && !_player->HasAura(SPELL_DEMON_SPIKES);
    }
    
    // Soul Fragment tracking
    uint32 GetSoulFragments() const {
        uint32 fragments = 0;
        // Count soul fragment objects near player
        std::list<GameObject*> goList;
        _player->GetGameObjectListWithEntryInGrid(goList, 
            GO_SOUL_FRAGMENT, 10.0f);
        fragments = goList.size();
        
        return fragments;
    }
    
private:
    Player* _player;
};
```

## Performance Optimizations

### Resource Caching

```cpp
// src/modules/Playerbot/AI/ClassAI/ResourceCache.h

template<typename T>
class ResourceCache {
public:
    ResourceCache(uint32 invalidateMs = 100) 
        : _invalidateMs(invalidateMs) {}
    
    T Get(std::function<T()> calculator) {
        uint32 now = getMSTime();
        
        if (!_cached || now - _lastUpdate > _invalidateMs) {
            _value = calculator();
            _lastUpdate = now;
            _cached = true;
        }
        
        return _value;
    }
    
    void Invalidate() {
        _cached = false;
    }
    
private:
    T _value;
    uint32 _lastUpdate = 0;
    uint32 _invalidateMs;
    bool _cached = false;
};

// Usage example
class OptimizedClassAI : public ClassAI {
private:
    ResourceCache<uint32> _comboPointCache{50};  // Cache for 50ms
    ResourceCache<float> _hasteCache{1000};       // Cache for 1 second
    
public:
    uint32 GetComboPoints() {
        return _comboPointCache.Get([this]() {
            return _bot->GetComboPoints();
        });
    }
    
    float GetHaste() {
        return _hasteCache.Get([this]() {
            return _bot->GetHasteMultiplier();
        });
    }
};
```

## Testing Framework

```cpp
// src/modules/Playerbot/Tests/AdvancedMechanicsTest.cpp

TEST_F(AdvancedMechanicsTest, DeathKnightRunes) {
    auto dk = CreateTestBot(CLASS_DEATH_KNIGHT, 80);
    RuneManager runeManager(dk);
    
    // Test rune consumption
    EXPECT_EQ(runeManager.GetAvailableRunes(RuneManager::RUNE_BLOOD), 2);
    
    runeManager.ConsumeRunes(SPELL_BLOOD_STRIKE);
    EXPECT_EQ(runeManager.GetAvailableRunes(RuneManager::RUNE_BLOOD), 1);
    
    // Test rune regeneration
    runeManager.Update(10000);  // 10 seconds
    EXPECT_EQ(runeManager.GetAvailableRunes(RuneManager::RUNE_BLOOD), 2);
}

TEST_F(AdvancedMechanicsTest, DruidForms) {
    auto druid = CreateTestBot(CLASS_DRUID, 80);
    DruidFormManager formManager(druid);
    
    // Test form shifting
    formManager.ShiftToForm(DruidFormManager::FORM_CAT);
    EXPECT_EQ(druid->GetShapeshiftForm(), FORM_CAT);
    
    // Test ability restrictions
    EXPECT_FALSE(formManager.CanUseAbilityInForm(
        SPELL_HEALING_TOUCH, DruidFormManager::FORM_CAT));
}
```

## Next Steps

1. **Implement Rune Manager** - Death Knight rune system
2. **Add Soul Shard System** - Warlock resource
3. **Create Form Manager** - Druid shape-shifting
4. **Add Totem System** - Shaman totems
5. **Performance Testing** - Ensure efficiency

---

**Status**: Ready for Implementation
**Dependencies**: Combat Rotations
**Estimated Time**: Sprint 3B Days 1-6