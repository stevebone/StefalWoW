/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * DEMON HUNTER TALENT ENHANCEMENTS
 *
 * Enterprise-grade support for new Demon Hunter talents from TrinityCore 12.0 upstream merge.
 * This file contains spell IDs, trackers, and helper classes for the following talents:
 *
 * VENGEANCE DEMON HUNTER TALENTS:
 * - Painbringer (207387): Soul Cleave grants damage increase buff to Shear
 * - Soulmonger (389711): Soul Fragments have a chance to create additional fragments
 * - Cycle of Binding (389718): Sigils reduce cooldown of other Sigils
 * - Retaliation (389729): Demon Spikes causes melee attackers to take fire damage
 *
 * GENERAL IMPROVEMENTS:
 * - Sigil of Flame energize fix (389787): Properly generates Fury
 *
 * Usage:
 *   #include "DemonHunterTalentEnhancements.h"
 *
 *   // In Vengeance DH AI:
 *   if (_talentTracker.HasPainbringer())
 *       // Prioritize Soul Cleave before Shear for damage boost
 */

#pragma once

#include "Define.h"
#include "SpellAuras.h"
#include "SpellMgr.h"
#include "Player.h"
#include "ObjectGuid.h"
#include "GameTime.h"
#include <unordered_map>
#include <array>

namespace Playerbot
{

// ============================================================================
// SPELL IDS - NEW DEMON HUNTER TALENTS (TrinityCore 12.0)
// ============================================================================

namespace DHtalents
{
    // ========================================================================
    // VENGEANCE TALENTS
    // ========================================================================

    // Painbringer - Soul Cleave grants damage increase to Shear
    // Proc: Soul Cleave
    // Effect: Next Shear deals increased damage
    constexpr uint32 PAINBRINGER = 207387;
    constexpr uint32 PAINBRINGER_BUFF = 212988;  // Damage reduction aura

    // Soulmonger - Soul Fragments have chance to create additional fragments
    // Proc: Soul Fragment consumption
    // Effect: May spawn additional Soul Fragment
    constexpr uint32 SOULMONGER = 389711;

    // Cycle of Binding - Sigils reduce cooldown of other Sigils
    // Proc: Any Sigil activation
    // Effect: Reduces cooldown of other Sigils by 2 seconds
    constexpr uint32 CYCLE_OF_BINDING = 389718;

    // Retaliation - Demon Spikes causes attackers to take fire damage
    // Proc: Being melee attacked with Demon Spikes active
    // Effect: Deals fire damage to attacker
    constexpr uint32 RETALIATION = 389729;

    // ========================================================================
    // SIGIL SPELLS (for Cycle of Binding)
    // ========================================================================

    constexpr uint32 SIGIL_OF_FLAME = 204596;
    constexpr uint32 SIGIL_OF_CHAINS = 202138;
    constexpr uint32 SIGIL_OF_MISERY = 207684;
    constexpr uint32 SIGIL_OF_SILENCE = 202137;
    constexpr uint32 SIGIL_OF_SPITE = 390163;

    // Array of all Sigil spell IDs for iteration
    constexpr std::array<uint32, 5> ALL_SIGILS = {
        SIGIL_OF_FLAME,
        SIGIL_OF_CHAINS,
        SIGIL_OF_MISERY,
        SIGIL_OF_SILENCE,
        SIGIL_OF_SPITE
    };

    // ========================================================================
    // RELATED SPELLS
    // ========================================================================

    constexpr uint32 SOUL_CLEAVE = 228477;
    constexpr uint32 SHEAR = 203782;
    constexpr uint32 DEMON_SPIKES = 203819;
    constexpr uint32 SOUL_FRAGMENT = 203981;
    constexpr uint32 SIGIL_OF_FLAME_ENERGIZE = 389787;  // New energize effect
}

// ============================================================================
// TALENT TRACKER - Tracks which talents the bot has
// ============================================================================

class DHTalentTracker
{
public:
    explicit DHTalentTracker(Player* bot) : _bot(bot) {}

    // ========================================================================
    // VENGEANCE TALENTS
    // ========================================================================

    [[nodiscard]] bool HasPainbringer() const
    {
        return _bot && _bot->HasSpell(DHtalents::PAINBRINGER);
    }

    [[nodiscard]] bool HasSoulmonger() const
    {
        return _bot && _bot->HasSpell(DHtalents::SOULMONGER);
    }

    [[nodiscard]] bool HasCycleOfBinding() const
    {
        return _bot && _bot->HasSpell(DHtalents::CYCLE_OF_BINDING);
    }

    [[nodiscard]] bool HasRetaliation() const
    {
        return _bot && _bot->HasSpell(DHtalents::RETALIATION);
    }

    // ========================================================================
    // UTILITY
    // ========================================================================

    [[nodiscard]] bool HasSigilOfSpite() const
    {
        return _bot && _bot->HasSpell(DHtalents::SIGIL_OF_SPITE);
    }

    void Update()
    {
        // Cache talent states if needed for performance
    }

private:
    Player* _bot;
};

// ============================================================================
// PAINBRINGER TRACKER
// ============================================================================

/**
 * Tracks Painbringer buff from Soul Cleave.
 *
 * Mechanics:
 * - Soul Cleave grants Painbringer buff
 * - Buff increases next Shear damage by X%
 * - Consumed on Shear cast
 */
class PainbringerTracker
{
public:
    PainbringerTracker() = default;

    void OnSoulCleave()
    {
        _lastSoulCleaveTime = GameTime::GetGameTimeMS();
        _buffActive = true;
        _buffStacks++;
        _totalProcs++;
    }

    void OnShear()
    {
        if (_buffActive)
        {
            _buffStacks = 0;
            _buffActive = false;
        }
    }

    [[nodiscard]] bool IsBuffActive() const { return _buffActive; }
    [[nodiscard]] uint32 GetBuffStacks() const { return _buffStacks; }
    [[nodiscard]] uint32 GetTotalProcs() const { return _totalProcs; }

    /**
     * Determines optimal rotation order based on Painbringer
     *
     * With Painbringer, the optimal rotation is:
     * Soul Cleave -> Shear (boosted) -> repeat
     */
    [[nodiscard]] bool ShouldPrioritizeSoulCleave(uint32 currentPain, uint32 soulFragments) const
    {
        // If we have Painbringer buff, use Shear to consume it
        if (_buffActive)
            return false;  // Use Shear first

        // If we have resources, Soul Cleave to proc Painbringer
        return currentPain >= 30 && soulFragments >= 1;
    }

    void Update(Player* bot)
    {
        if (!bot)
            return;

        // Check if buff expired (Painbringer lasts until Shear is used)
        // But also has a time limit
        uint32 now = GameTime::GetGameTimeMS();
        if (_buffActive && (now - _lastSoulCleaveTime) > 15000)
        {
            _buffActive = false;
            _buffStacks = 0;
        }

        // Also check actual aura
        if (_buffActive && !bot->HasAura(DHtalents::PAINBRINGER_BUFF))
        {
            _buffActive = false;
            _buffStacks = 0;
        }
    }

private:
    uint32 _lastSoulCleaveTime{0};
    bool _buffActive{false};
    uint32 _buffStacks{0};
    uint32 _totalProcs{0};
};

// ============================================================================
// SOULMONGER TRACKER
// ============================================================================

/**
 * Tracks Soulmonger Soul Fragment generation bonus.
 *
 * Mechanics:
 * - When consuming Soul Fragments, has chance to create additional fragments
 * - 5% base chance per fragment consumed
 * - Used to maximize Soul Fragment uptime
 */
class SoulmongerTracker
{
public:
    SoulmongerTracker() = default;

    void OnFragmentsConsumed(uint32 fragmentsConsumed)
    {
        _fragmentsConsumed += fragmentsConsumed;

        // Each fragment has ~5% chance to spawn another
        // Track expected procs for statistics
        _expectedProcs += fragmentsConsumed * 0.05f;
    }

    void OnExtraFragmentSpawned()
    {
        _extraFragmentsSpawned++;
    }

    [[nodiscard]] uint32 GetTotalFragmentsConsumed() const { return _fragmentsConsumed; }
    [[nodiscard]] uint32 GetExtraFragmentsSpawned() const { return _extraFragmentsSpawned; }

    /**
     * Determines if we should aggressively consume fragments
     *
     * With Soulmonger, consuming more fragments = more chances for bonus fragments
     * This creates a positive feedback loop for Soul Fragment generation
     */
    [[nodiscard]] bool ShouldAggressivelyConsumeFragments(uint32 currentFragments) const
    {
        // With Soulmonger, it's valuable to consume fragments even with fewer
        // because each consumption has a chance to spawn more
        return currentFragments >= 2;
    }

private:
    uint32 _fragmentsConsumed{0};
    uint32 _extraFragmentsSpawned{0};
    float _expectedProcs{0.0f};
};

// ============================================================================
// CYCLE OF BINDING TRACKER
// ============================================================================

/**
 * Tracks Cycle of Binding Sigil cooldown reduction.
 *
 * Mechanics:
 * - When any Sigil activates (triggers its effect), reduces CD of other Sigils by 2 sec
 * - Does not affect the Sigil that just activated
 * - Encourages using multiple Sigil types
 */
class CycleOfBindingTracker
{
public:
    CycleOfBindingTracker() = default;

    void OnSigilActivated(uint32 sigilSpellId)
    {
        _lastSigilActivated = sigilSpellId;
        _lastActivationTime = GameTime::GetGameTimeMS();
        _totalActivations++;

        // Track which sigils were affected (all except the one used)
        for (uint32 sigil : DHtalents::ALL_SIGILS)
        {
            if (sigil != sigilSpellId)
                _cooldownReductionsApplied[sigil]++;
        }
    }

    [[nodiscard]] uint32 GetLastSigilActivated() const { return _lastSigilActivated; }
    [[nodiscard]] uint32 GetTotalActivations() const { return _totalActivations; }

    [[nodiscard]] uint32 GetCooldownReductionForSigil(uint32 sigilSpellId) const
    {
        auto it = _cooldownReductionsApplied.find(sigilSpellId);
        if (it != _cooldownReductionsApplied.end())
            return it->second * 2000;  // 2 seconds per reduction
        return 0;
    }

    /**
     * Determines optimal Sigil usage order for Cycle of Binding
     *
     * To maximize Cycle of Binding value, alternate between different Sigils:
     * Flame -> Silence -> Flame (reduced CD) -> Misery -> etc.
     */
    [[nodiscard]] uint32 GetRecommendedNextSigil(Player* bot) const
    {
        if (!bot)
            return DHtalents::SIGIL_OF_FLAME;

        // Find the Sigil with the most cooldown reduction applied
        // that isn't the last one used
        uint32 bestSigil = DHtalents::SIGIL_OF_FLAME;
        uint32 bestReduction = 0;

        for (uint32 sigil : DHtalents::ALL_SIGILS)
        {
            if (sigil == _lastSigilActivated)
                continue;

            if (!bot->HasSpell(sigil))
                continue;

            auto it = _cooldownReductionsApplied.find(sigil);
            uint32 reduction = (it != _cooldownReductionsApplied.end()) ? it->second : 0;

            if (reduction > bestReduction)
            {
                bestReduction = reduction;
                bestSigil = sigil;
            }
        }

        return bestSigil;
    }

private:
    uint32 _lastSigilActivated{0};
    uint32 _lastActivationTime{0};
    uint32 _totalActivations{0};
    std::unordered_map<uint32, uint32> _cooldownReductionsApplied;
};

// ============================================================================
// RETALIATION TRACKER
// ============================================================================

/**
 * Tracks Retaliation damage output when Demon Spikes is active.
 *
 * Mechanics:
 * - While Demon Spikes is active, melee attacks against you deal fire damage to attacker
 * - Damage scales with Attack Power
 * - Encourages maintaining Demon Spikes uptime
 */
class RetaliationTracker
{
public:
    RetaliationTracker() = default;

    void OnDemonSpikesActivated()
    {
        _demonSpikesActive = true;
        _demonSpikesStartTime = GameTime::GetGameTimeMS();
    }

    void OnDemonSpikesExpired()
    {
        _demonSpikesActive = false;
        _totalDemonSpikesUptime += GameTime::GetGameTimeMS() - _demonSpikesStartTime;
    }

    void OnRetaliationProc(uint32 damage)
    {
        _totalRetaliationDamage += damage;
        _retaliationProcs++;
    }

    [[nodiscard]] bool IsDemonSpikesActive() const { return _demonSpikesActive; }
    [[nodiscard]] uint64 GetTotalRetaliationDamage() const { return _totalRetaliationDamage; }
    [[nodiscard]] uint32 GetRetaliationProcs() const { return _retaliationProcs; }

    /**
     * Determines if Demon Spikes should be prioritized for Retaliation damage
     *
     * With Retaliation, Demon Spikes becomes more valuable against many attackers
     * because each attack against you deals damage back
     */
    [[nodiscard]] bool ShouldPrioritizeDemonSpikes(uint32 attackerCount) const
    {
        // More attackers = more Retaliation value
        // 3+ attackers makes Demon Spikes very high priority
        return attackerCount >= 3 && !_demonSpikesActive;
    }

    void Update(Player* bot)
    {
        if (!bot)
            return;

        // Check if Demon Spikes expired
        if (_demonSpikesActive)
        {
            // Demon Spikes has ~6 second duration
            if (GameTime::GetGameTimeMS() - _demonSpikesStartTime > 6000)
            {
                OnDemonSpikesExpired();
            }

            // Also check actual aura
            if (!bot->HasAura(DHtalents::DEMON_SPIKES))
            {
                OnDemonSpikesExpired();
            }
        }
    }

private:
    bool _demonSpikesActive{false};
    uint32 _demonSpikesStartTime{0};
    uint32 _totalDemonSpikesUptime{0};
    uint64 _totalRetaliationDamage{0};
    uint32 _retaliationProcs{0};
};

// ============================================================================
// COMBINED DEMON HUNTER TALENT STATE
// ============================================================================

/**
 * Combined state manager for all Demon Hunter talent mechanics.
 *
 * Usage:
 *   DHTalentState talentState(bot);
 *   talentState.Update();
 *
 *   if (talentState.talents.HasPainbringer())
 *       // Optimize rotation for Painbringer
 */
class DHTalentState
{
private:
    // Note: _bot must be declared before talents for correct initialization order
    Player* _bot;

public:
    explicit DHTalentState(Player* bot)
        : _bot(bot)
        , talents(bot)
    {}

    void Update()
    {
        if (!_bot)
            return;

        talents.Update();
        painbringer.Update(_bot);
        retaliation.Update(_bot);
    }

    // Public accessors
    DHTalentTracker talents;
    PainbringerTracker painbringer;
    SoulmongerTracker soulmonger;
    CycleOfBindingTracker cycleOfBinding;
    RetaliationTracker retaliation;
};

} // namespace Playerbot
