/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * PRIEST TALENT ENHANCEMENTS
 *
 * Enterprise-grade support for new Priest talents from TrinityCore 12.0 upstream merge.
 * This file contains spell IDs, trackers, and helper classes for the following talents:
 *
 * SHADOW PRIEST TALENTS:
 * - Shadowy Apparitions (341491): Creates shadowy copies that deal damage
 * - Mental Decay (375994): Mind abilities extend DoT duration
 * - Death's Torment (1240364): Shadow Word: Death hits multiple times
 * - Insidious Ire (373212): DoT damage increases based on duration
 * - Inescapable Torment (373427): Mind Blast/SW:D extend Mindbender duration
 *
 * HOLY PRIEST TALENTS:
 * - Power Surge (453109): Halo triggers healing/damage surge
 * - Empyreal Blaze (372616): Holy Fire grants powerful buff
 *
 * Usage:
 *   #include "PriestTalentEnhancements.h"
 *
 *   // In Shadow Priest AI:
 *   if (_talentTracker.HasShadowyApparitions())
 *       // Use Shadowy Apparitions mechanics
 */

#pragma once

#include "Define.h"
#include "SpellAuras.h"
#include "SpellMgr.h"
#include "Player.h"
#include "ObjectGuid.h"
#include "GameTime.h"
#include <unordered_map>
#include <unordered_set>
#include <chrono>

namespace Playerbot
{

// ============================================================================
// SPELL IDS - NEW PRIEST TALENTS (TrinityCore 12.0)
// ============================================================================

namespace PriestTalents
{
    // ========================================================================
    // SHADOW PRIEST TALENTS
    // ========================================================================

    // Shadowy Apparitions - Creates shadowy copies that deal damage
    // Proc: Vampiric Touch and Shadow Word: Pain critical hits
    // Effect: Spawns apparition that travels to target and deals damage
    constexpr uint32 SHADOWY_APPARITIONS_TALENT = 341491;    // Talent passive
    constexpr uint32 SHADOWY_APPARITION_DUMMY = 341263;       // Projectile spell
    constexpr uint32 SHADOWY_APPARITION_DAMAGE = 148859;      // Damage spell

    // Mental Decay - Mind abilities extend DoT duration
    // Proc: Mind Flay, Mind Sear, Mind Spike
    // Effect: Extends Vampiric Touch and Shadow Word: Pain by 1 sec
    constexpr uint32 MENTAL_DECAY = 375994;

    // Death's Torment - Shadow Word: Death hits multiple times
    // Proc: When target affected by SW:P or VT dies
    // Effect: SW:D hits additional times at reduced effectiveness
    constexpr uint32 DEATHS_TORMENT = 1240364;

    // Insidious Ire - DoT damage increases based on duration
    // Effect: Each tick of SW:P and VT increases their damage
    constexpr uint32 INSIDIOUS_IRE = 373212;

    // Inescapable Torment - Mind Blast/SW:D extend Mindbender duration
    // Proc: Mind Blast, Shadow Word: Death, Penance, Dark Reprimand
    // Effect: Extends Mindbender/Shadowfiend duration and triggers damage
    constexpr uint32 INESCAPABLE_TORMENT = 373427;

    // ========================================================================
    // HOLY PRIEST TALENTS
    // ========================================================================

    // Power Surge - Halo triggers healing/damage surge
    // Proc: Casting Halo (Holy: 120517, Shadow: 120644)
    // Effect: Periodic healing/damage after Halo
    constexpr uint32 POWER_SURGE = 453109;                   // Base talent
    constexpr uint32 POWER_SURGE_HOLY_PERIODIC = 453112;     // Holy periodic
    constexpr uint32 POWER_SURGE_SHADOW_PERIODIC = 453113;   // Shadow periodic

    // Empyreal Blaze - Holy Fire grants powerful buff
    // Proc: Casting Holy Fire
    // Effect: Makes next Holy Fire instant and triggers AoE healing
    constexpr uint32 EMPYREAL_BLAZE = 372616;
    constexpr uint32 EMPYREAL_BLAZE_AURA = 372617;

    // ========================================================================
    // RELATED SPELLS (for proc detection)
    // ========================================================================

    // Shadow spells that interact with talents
    constexpr uint32 MIND_FLAY = 15407;
    constexpr uint32 MIND_SEAR = 48045;
    constexpr uint32 MIND_SPIKE = 73510;
    constexpr uint32 MIND_BLAST = 8092;
    constexpr uint32 SHADOW_WORD_DEATH = 32379;
    constexpr uint32 VAMPIRIC_TOUCH = 34914;
    constexpr uint32 SHADOW_WORD_PAIN = 589;
    constexpr uint32 MINDBENDER = 123040;
    constexpr uint32 SHADOWFIEND = 34433;

    // Holy spells that interact with talents
    constexpr uint32 HALO_HOLY = 120517;
    constexpr uint32 HALO_SHADOW = 120644;
    constexpr uint32 HOLY_FIRE = 14914;
}

// ============================================================================
// TALENT TRACKER - Tracks which talents the bot has
// ============================================================================

class PriestTalentTracker
{
public:
    explicit PriestTalentTracker(Player* bot) : _bot(bot) {}

    // ========================================================================
    // SHADOW TALENTS
    // ========================================================================

    [[nodiscard]] bool HasShadowyApparitions() const
    {
        return _bot && _bot->HasSpell(PriestTalents::SHADOWY_APPARITIONS_TALENT);
    }

    [[nodiscard]] bool HasMentalDecay() const
    {
        return _bot && _bot->HasSpell(PriestTalents::MENTAL_DECAY);
    }

    [[nodiscard]] bool HasDeathsTorment() const
    {
        return _bot && _bot->HasSpell(PriestTalents::DEATHS_TORMENT);
    }

    [[nodiscard]] bool HasInsidiousIre() const
    {
        return _bot && _bot->HasSpell(PriestTalents::INSIDIOUS_IRE);
    }

    [[nodiscard]] bool HasInescapableTorment() const
    {
        return _bot && _bot->HasSpell(PriestTalents::INESCAPABLE_TORMENT);
    }

    // ========================================================================
    // HOLY TALENTS
    // ========================================================================

    [[nodiscard]] bool HasPowerSurge() const
    {
        return _bot && _bot->HasSpell(PriestTalents::POWER_SURGE);
    }

    [[nodiscard]] bool HasEmpyrealBlaze() const
    {
        return _bot && _bot->HasSpell(PriestTalents::EMPYREAL_BLAZE);
    }

    // ========================================================================
    // UTILITY
    // ========================================================================

    [[nodiscard]] bool HasMindbender() const
    {
        return _bot && _bot->HasSpell(PriestTalents::MINDBENDER);
    }

    [[nodiscard]] bool HasShadowfiend() const
    {
        return _bot && _bot->HasSpell(PriestTalents::SHADOWFIEND);
    }

    void Update()
    {
        // Cache talent states if needed for performance
    }

private:
    Player* _bot;
};

// ============================================================================
// SHADOWY APPARITIONS TRACKER
// ============================================================================

/**
 * Tracks Shadowy Apparitions procs and spawned apparitions.
 *
 * Mechanics:
 * - Vampiric Touch and Shadow Word: Pain critical hits spawn apparitions
 * - Apparitions travel to target and deal damage
 * - With Auspicious Spirits talent, they generate Insanity
 */
class ShadowyApparitionsTracker
{
public:
    ShadowyApparitionsTracker() = default;

    void OnDoTCritical(ObjectGuid targetGuid)
    {
        // Record that an apparition should spawn
        _lastApparitionSpawn = GameTime::GetGameTimeMS();
        _activeApparitions++;
        _totalSpawned++;
    }

    void OnApparitionHit(ObjectGuid targetGuid, uint32 damage)
    {
        if (_activeApparitions > 0)
            _activeApparitions--;
        _totalDamage += damage;
    }

    [[nodiscard]] uint32 GetActiveApparitions() const { return _activeApparitions; }
    [[nodiscard]] uint32 GetTotalSpawned() const { return _totalSpawned; }
    [[nodiscard]] uint64 GetTotalDamage() const { return _totalDamage; }

    void Reset()
    {
        _activeApparitions = 0;
        _totalSpawned = 0;
        _totalDamage = 0;
    }

private:
    uint32 _activeApparitions{0};
    uint32 _totalSpawned{0};
    uint64 _totalDamage{0};
    uint32 _lastApparitionSpawn{0};
};

// ============================================================================
// INSIDIOUS IRE TRACKER
// ============================================================================

/**
 * Tracks Insidious Ire stacking damage bonus.
 *
 * Mechanics:
 * - Each DoT tick increases damage of future ticks
 * - Stacks up to a maximum bonus
 * - Resets when DoT expires or is reapplied
 */
class InsidiousIreTracker
{
public:
    struct DoTState
    {
        uint32 tickCount{0};
        uint32 appliedTime{0};
        float currentBonus{0.0f};
    };

    InsidiousIreTracker() = default;

    void OnDoTApplied(ObjectGuid targetGuid, uint32 spellId)
    {
        DoTKey key{targetGuid, spellId};
        _dotStates[key] = DoTState{
            .tickCount = 0,
            .appliedTime = GameTime::GetGameTimeMS(),
            .currentBonus = 0.0f
        };
    }

    void OnDoTTick(ObjectGuid targetGuid, uint32 spellId)
    {
        DoTKey key{targetGuid, spellId};
        auto it = _dotStates.find(key);
        if (it != _dotStates.end())
        {
            it->second.tickCount++;
            // Each tick adds 1% damage, capping at 20%
            it->second.currentBonus = std::min(it->second.currentBonus + 1.0f, 20.0f);
        }
    }

    [[nodiscard]] float GetCurrentBonus(ObjectGuid targetGuid, uint32 spellId) const
    {
        DoTKey key{targetGuid, spellId};
        auto it = _dotStates.find(key);
        if (it != _dotStates.end())
            return it->second.currentBonus;
        return 0.0f;
    }

    void OnDoTExpired(ObjectGuid targetGuid, uint32 spellId)
    {
        DoTKey key{targetGuid, spellId};
        _dotStates.erase(key);
    }

    void Update(Player* bot)
    {
        if (!bot)
            return;

        // Clean up expired states
        uint32 now = GameTime::GetGameTimeMS();
        for (auto it = _dotStates.begin(); it != _dotStates.end();)
        {
            // DoTs typically last 16-21 seconds, clean up after 30 seconds
            if (now - it->second.appliedTime > 30000)
                it = _dotStates.erase(it);
            else
                ++it;
        }
    }

private:
    struct DoTKey
    {
        ObjectGuid targetGuid;
        uint32 spellId;

        bool operator==(DoTKey const& other) const
        {
            return targetGuid == other.targetGuid && spellId == other.spellId;
        }
    };

    struct DoTKeyHash
    {
        std::size_t operator()(DoTKey const& key) const
        {
            // TrinityCore 12.0: Use std::hash<ObjectGuid> specialization
            return std::hash<ObjectGuid>{}(key.targetGuid) ^
                   (std::hash<uint32>{}(key.spellId) << 1);
        }
    };

    std::unordered_map<DoTKey, DoTState, DoTKeyHash> _dotStates;
};

// ============================================================================
// MENTAL DECAY TRACKER
// ============================================================================

/**
 * Tracks Mental Decay DoT extensions.
 *
 * Mechanics:
 * - Mind Flay, Mind Sear, and Mind Spike extend VT and SW:P
 * - Each proc extends by 1 second
 * - Used to maximize DoT uptime without recasting
 */
class MentalDecayTracker
{
public:
    MentalDecayTracker() = default;

    void OnMindAbilityUsed(ObjectGuid targetGuid)
    {
        uint32 now = GameTime::GetGameTimeMS();
        _lastExtensionTime[targetGuid] = now;
        _totalExtensions++;
    }

    [[nodiscard]] uint32 GetLastExtensionTime(ObjectGuid targetGuid) const
    {
        auto it = _lastExtensionTime.find(targetGuid);
        if (it != _lastExtensionTime.end())
            return it->second;
        return 0;
    }

    [[nodiscard]] uint32 GetTotalExtensions() const { return _totalExtensions; }

    void Update(Player* bot)
    {
        if (!bot)
            return;

        // Clean up old entries
        uint32 now = GameTime::GetGameTimeMS();
        for (auto it = _lastExtensionTime.begin(); it != _lastExtensionTime.end();)
        {
            if (now - it->second > 60000) // Clean up after 1 minute
                it = _lastExtensionTime.erase(it);
            else
                ++it;
        }
    }

private:
    std::unordered_map<ObjectGuid, uint32> _lastExtensionTime;
    uint32 _totalExtensions{0};
};

// ============================================================================
// DEATHS TORMENT TRACKER
// ============================================================================

/**
 * Tracks Death's Torment multi-hit procs.
 *
 * Mechanics:
 * - When target with VT/SW:P dies, Shadow Word: Death hits multiple times
 * - Each additional hit is at reduced effectiveness
 * - Great for execute phase optimization
 */
class DeathsTormentTracker
{
public:
    DeathsTormentTracker() = default;

    void OnTargetKilled(ObjectGuid targetGuid, uint32 additionalHits)
    {
        _lastKillTime = GameTime::GetGameTimeMS();
        _pendingHits += additionalHits;
        _totalProcs++;
    }

    void OnHitCompleted()
    {
        if (_pendingHits > 0)
            _pendingHits--;
    }

    [[nodiscard]] uint32 GetPendingHits() const { return _pendingHits; }
    [[nodiscard]] uint32 GetTotalProcs() const { return _totalProcs; }

    [[nodiscard]] bool ShouldPrioritizeSWDeath(Unit* target) const
    {
        // SW:Death is higher priority when target is low and has DoTs
        return target && target->GetHealthPct() < 20.0f;
    }

private:
    uint32 _pendingHits{0};
    uint32 _totalProcs{0};
    uint32 _lastKillTime{0};
};

// ============================================================================
// INESCAPABLE TORMENT TRACKER
// ============================================================================

/**
 * Tracks Inescapable Torment pet extension and damage procs.
 *
 * Mechanics:
 * - Mind Blast and Shadow Word: Death extend Mindbender/Shadowfiend
 * - Also triggers additional damage from the pet
 * - Critical for maximizing pet uptime
 */
class InescapableTormentTracker
{
public:
    InescapableTormentTracker() = default;

    void OnPetSummoned(ObjectGuid petGuid, uint32 baseDuration)
    {
        _currentPetGuid = petGuid;
        _petSummonTime = GameTime::GetGameTimeMS();
        _basePetDuration = baseDuration;
        _extensionsApplied = 0;
    }

    void OnExtensionApplied(uint32 extensionMs)
    {
        _extensionsApplied++;
        _totalExtensionMs += extensionMs;
    }

    [[nodiscard]] bool IsPetActive() const
    {
        if (!_currentPetGuid)
            return false;
        uint32 now = GameTime::GetGameTimeMS();
        return (now - _petSummonTime) < (_basePetDuration + _totalExtensionMs);
    }

    [[nodiscard]] uint32 GetRemainingPetDuration() const
    {
        if (!IsPetActive())
            return 0;
        uint32 now = GameTime::GetGameTimeMS();
        uint32 totalDuration = _basePetDuration + _totalExtensionMs;
        uint32 elapsed = now - _petSummonTime;
        return totalDuration > elapsed ? totalDuration - elapsed : 0;
    }

    [[nodiscard]] bool ShouldPrioritizeMindBlast() const
    {
        // Prioritize Mind Blast when pet is active for extensions
        return IsPetActive() && GetRemainingPetDuration() > 2000;
    }

    void OnPetExpired()
    {
        _currentPetGuid = ObjectGuid::Empty;
        _totalExtensionMs = 0;
        _extensionsApplied = 0;
    }

private:
    ObjectGuid _currentPetGuid;
    uint32 _petSummonTime{0};
    uint32 _basePetDuration{0};
    uint32 _extensionsApplied{0};
    uint32 _totalExtensionMs{0};
};

// ============================================================================
// POWER SURGE TRACKER (HOLY)
// ============================================================================

/**
 * Tracks Power Surge healing/damage surge from Halo.
 *
 * Mechanics:
 * - Casting Halo triggers Power Surge buff
 * - Buff causes periodic healing (Holy) or damage (Shadow)
 * - Timing Halo usage for maximum effect
 */
class PowerSurgeTracker
{
public:
    PowerSurgeTracker() = default;

    void OnHaloCast(bool isHoly)
    {
        _lastHaloTime = GameTime::GetGameTimeMS();
        _surgeActive = true;
        _isHolySurge = isHoly;
        _totalSurges++;
    }

    void OnSurgeTick(uint32 healingOrDamage)
    {
        if (_isHolySurge)
            _totalHealing += healingOrDamage;
        else
            _totalDamage += healingOrDamage;
    }

    void OnSurgeExpired()
    {
        _surgeActive = false;
    }

    [[nodiscard]] bool IsSurgeActive() const { return _surgeActive; }
    [[nodiscard]] bool IsHolySurge() const { return _isHolySurge; }
    [[nodiscard]] uint64 GetTotalHealing() const { return _totalHealing; }
    [[nodiscard]] uint64 GetTotalDamage() const { return _totalDamage; }

    void Update(Player* bot)
    {
        if (!bot)
            return;

        // Check if surge buff is still active
        if (_surgeActive)
        {
            bool hasHolySurge = bot->HasAura(PriestTalents::POWER_SURGE_HOLY_PERIODIC);
            bool hasShadowSurge = bot->HasAura(PriestTalents::POWER_SURGE_SHADOW_PERIODIC);
            if (!hasHolySurge && !hasShadowSurge)
                _surgeActive = false;
        }
    }

private:
    uint32 _lastHaloTime{0};
    bool _surgeActive{false};
    bool _isHolySurge{true};
    uint32 _totalSurges{0};
    uint64 _totalHealing{0};
    uint64 _totalDamage{0};
};

// ============================================================================
// EMPYREAL BLAZE TRACKER (HOLY)
// ============================================================================

/**
 * Tracks Empyreal Blaze buff from Holy Fire.
 *
 * Mechanics:
 * - Casting Holy Fire grants Empyreal Blaze buff
 * - Buff makes next Holy Fire instant cast
 * - Also triggers AoE healing effect
 */
class EmpyrealBlazeTracker
{
public:
    EmpyrealBlazeTracker() = default;

    void OnHolyFireCast()
    {
        _lastHolyFireTime = GameTime::GetGameTimeMS();
        _totalCasts++;
    }

    void OnBlazeProc()
    {
        _blazeActive = true;
        _blazeStartTime = GameTime::GetGameTimeMS();
        _totalProcs++;
    }

    void OnBlazeConsumed()
    {
        _blazeActive = false;
    }

    [[nodiscard]] bool IsBlazeActive() const { return _blazeActive; }

    [[nodiscard]] bool ShouldCastHolyFire(Player* bot) const
    {
        if (!bot)
            return false;

        // Higher priority when we have Empyreal Blaze buff (instant cast)
        return bot->HasAura(PriestTalents::EMPYREAL_BLAZE_AURA);
    }

    void Update(Player* bot)
    {
        if (!bot)
            return;

        // Check if blaze buff is still active
        _blazeActive = bot->HasAura(PriestTalents::EMPYREAL_BLAZE_AURA);
    }

    [[nodiscard]] uint32 GetTotalCasts() const { return _totalCasts; }
    [[nodiscard]] uint32 GetTotalProcs() const { return _totalProcs; }

private:
    uint32 _lastHolyFireTime{0};
    bool _blazeActive{false};
    uint32 _blazeStartTime{0};
    uint32 _totalCasts{0};
    uint32 _totalProcs{0};
};

// ============================================================================
// COMBINED PRIEST TALENT STATE
// ============================================================================

/**
 * Combined state manager for all Priest talent mechanics.
 *
 * Usage:
 *   PriestTalentState talentState(bot);
 *   talentState.Update();
 *
 *   if (talentState.talents.HasShadowyApparitions())
 *       // Handle apparitions
 */
class PriestTalentState
{
private:
    // Note: _bot must be declared before talents for correct initialization order
    Player* _bot;

public:
    explicit PriestTalentState(Player* bot)
        : _bot(bot)
        , talents(bot)
    {}

    void Update()
    {
        if (!_bot)
            return;

        talents.Update();
        insidiousIre.Update(_bot);
        mentalDecay.Update(_bot);
        powerSurge.Update(_bot);
        empyrealBlaze.Update(_bot);
    }

    // Public accessors
    PriestTalentTracker talents;
    ShadowyApparitionsTracker apparitions;
    InsidiousIreTracker insidiousIre;
    MentalDecayTracker mentalDecay;
    DeathsTormentTracker deathsTorment;
    InescapableTormentTracker inescapableTorment;
    PowerSurgeTracker powerSurge;
    EmpyrealBlazeTracker empyrealBlaze;
};

} // namespace Playerbot
