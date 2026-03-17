/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "../ClassAI.h"
#include "../SpellValidation_WoW120.h"
#include <memory>
#include <chrono>
#include <unordered_map>

namespace Playerbot
{

// Forward declarations for specialization classes (QW-4 FIX)
class HavocDemonHunterRefactored;
class VengeanceDemonHunterRefactored;

// Type aliases for consistency with base naming
using HavocDemonHunter = HavocDemonHunterRefactored;
using VengeanceDemonHunter = VengeanceDemonHunterRefactored;

// DemonHunter Spell IDs - Using central registry (WoW 12.0)
enum DemonHunterSpells
{
    // Havoc abilities
    CHAOS_STRIKE = WoW120Spells::DemonHunter::Common::CHAOS_STRIKE,
    BLADE_DANCE = WoW120Spells::DemonHunter::Common::BLADE_DANCE,
    DEATH_SWEEP = WoW120Spells::DemonHunter::Common::DEATH_SWEEP,
    ANNIHILATION = WoW120Spells::DemonHunter::Common::ANNIHILATION,
    METAMORPHOSIS_HAVOC = WoW120Spells::DemonHunter::Common::METAMORPHOSIS_HAVOC,
    EYE_BEAM = WoW120Spells::DemonHunter::Common::EYE_BEAM,
    DEMONS_BITE = WoW120Spells::DemonHunter::Common::DEMONS_BITE,
    FEL_BARRAGE = WoW120Spells::DemonHunter::Common::FEL_BARRAGE,

    // Vengeance abilities
    SOUL_CLEAVE = WoW120Spells::DemonHunter::Common::SOUL_CLEAVE,
    SPIRIT_BOMB = WoW120Spells::DemonHunter::Common::SPIRIT_BOMB,
    METAMORPHOSIS_VENGEANCE = WoW120Spells::DemonHunter::Common::METAMORPHOSIS_VENGEANCE,
    SHEAR = WoW120Spells::DemonHunter::Common::SHEAR,
    SOUL_BARRIER = WoW120Spells::DemonHunter::Common::SOUL_BARRIER,

    // Defensive abilities
    BLUR = WoW120Spells::DemonHunter::Common::BLUR,
    DARKNESS = WoW120Spells::DemonHunter::Common::DARKNESS,
    NETHERWALK = WoW120Spells::DemonHunter::Common::NETHERWALK,

    // Shared abilities
    DEMON_SPIKES = WoW120Spells::DemonHunter::Common::DEMON_SPIKES,
    IMMOLATION_AURA = WoW120Spells::DemonHunter::Common::IMMOLATION_AURA,
    FIERY_BRAND = WoW120Spells::DemonHunter::Common::FIERY_BRAND,
    FEL_RUSH = WoW120Spells::DemonHunter::Common::FEL_RUSH,
    VENGEFUL_RETREAT = WoW120Spells::DemonHunter::Common::VENGEFUL_RETREAT,
    CONSUME_MAGIC = WoW120Spells::DemonHunter::Common::CONSUME_MAGIC,
    SIGIL_OF_FLAME = WoW120Spells::DemonHunter::Common::SIGIL_OF_FLAME,
    SIGIL_OF_SILENCE = WoW120Spells::DemonHunter::Common::SIGIL_OF_SILENCE,
    SIGIL_OF_MISERY = WoW120Spells::DemonHunter::Common::SIGIL_OF_MISERY,
    CHAOS_NOVA = WoW120Spells::DemonHunter::Common::CHAOS_NOVA,

    // Utility/Interrupts
    DISRUPT = WoW120Spells::DemonHunter::Common::DISRUPT,
    IMPRISON = WoW120Spells::DemonHunter::Common::IMPRISON,

    // Offensive cooldowns
    NEMESIS = WoW120Spells::DemonHunter::Common::NEMESIS,

    // Talent abilities
    MOMENTUM_TALENT = WoW120Spells::DemonHunter::Common::MOMENTUM_TALENT,
    DEMONIC_TALENT = WoW120Spells::DemonHunter::Common::DEMONIC_TALENT,
    BLIND_FURY_TALENT = WoW120Spells::DemonHunter::Common::BLIND_FURY_TALENT
};

class TC_GAME_API DemonHunterAI : public ClassAI
{
public:
    explicit DemonHunterAI(Player* bot);
    ~DemonHunterAI();

    // ClassAI interface implementation
    void UpdateRotation(::Unit* target);
    void UpdateBuffs();
    void UpdateCooldowns(uint32 diff);
    bool CanUseAbility(uint32 spellId);

    // Combat state callbacks
    void OnCombatStart(::Unit* target);
    void OnCombatEnd();

protected:
    // Resource management
    bool HasEnoughResource(uint32 spellId);
    void ConsumeResource(uint32 spellId);

    // Positioning
    Position GetOptimalPosition(::Unit* target);
    float GetOptimalRange(::Unit* target);

private:
    // ========================================================================
    // QW-4 FIX: Per-instance specialization objects
    // Each bot has its own specialization object initialized with correct bot pointer
    // ========================================================================

    ::std::unique_ptr<HavocDemonHunter> _havocSpec;
    ::std::unique_ptr<VengeanceDemonHunter> _vengeanceSpec;

    // Delegation to specialization
    void DelegateToSpecialization(::Unit* target);

    void ExitMetamorphosis();
    bool ShouldUseMetamorphosis();
    void CastMetamorphosisHavoc();
    void CastMetamorphosisVengeance();
    void SpendPain(uint32 amount);
    void GeneratePain(uint32 amount);
    bool HasPain(uint32 amount);
    void SpendFury(uint32 amount);
    void GenerateFury(uint32 amount);
    bool HasFury(uint32 amount);
    void UpdatePainManagement(uint32 diff);
    void DecayPain(uint32 diff);
    uint32 GetFury() const;
    uint32 GetMaxFury() const;
    uint32 GetPain() const;
    uint32 GetMaxPain() const;

    // Combat rotation methods
    void UpdateHavocRotation(::Unit* target);
    void UpdateVengeanceRotation(::Unit* target);
    void HandleMetamorphosisAbilities(::Unit* target);
    void ExecuteBasicDemonHunterRotation(::Unit* target);

    // Priority system methods (CombatBehaviorIntegration)
    void HandleInterrupts(::Unit* target);
    void HandleDefensives();
    void HandleTargetSwitching(::Unit*& target);
    void HandleAoEDecisions(::Unit* target);
    void HandleCooldowns(::Unit* target);
    void HandleResourceGeneration(::Unit* target);
    void HandleMobility(::Unit* target);

    // Ability casting methods
    void CastEyeBeam(::Unit* target);
    void CastChaosStrike(::Unit* target);
    void CastBladeDance(::Unit* target);
    void CastDemonsBite(::Unit* target);
    void CastSoulCleave(::Unit* target);
    void CastShear(::Unit* target);

    // Utility methods
    ::std::vector<::Unit*> GetAoETargets(float range = 8.0f);
    uint32 GetNearbyEnemyCount(float range) const;
    bool IsInMeleeRange(::Unit* target) const;
    bool IsTargetInterruptible(::Unit* target) const;
    void RecordInterruptAttempt(::Unit* target, uint32 spellId, bool success);
    void RecordAbilityUsage(uint32 spellId);
    void OnTargetChanged(::Unit* newTarget);
    // Member variables
    // Combat tracking
    uint32 _lastInterruptTime;
    uint32 _lastDefensiveTime;
    uint32 _lastMobilityTime;
    uint32 _successfulInterrupts;
    ::std::unordered_map<uint32, uint32> _abilityUsage;

    // QW-4 FIX: Per-instance pain decay timer (was static - caused cross-bot contamination)
    uint32 _painDecayTimer = 0;

    // Metrics tracking
    struct DemonHunterMetrics
    {
        uint32 totalAbilitiesUsed;
        uint32 interruptsSucceeded;
        uint32 defensivesUsed;
        uint32 mobilityAbilitiesUsed;
        ::std::chrono::steady_clock::time_point combatStartTime;
        ::std::chrono::steady_clock::time_point lastMetricsUpdate;
    };
    DemonHunterMetrics _dhMetrics;

    // Helper methods
    void UpdateMetrics(uint32 diff);
    void AnalyzeCombatEffectiveness();
};

} // namespace Playerbot
