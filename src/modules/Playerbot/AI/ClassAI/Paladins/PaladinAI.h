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
#include "../SpellValidation_WoW120_Part2.h"
#include "Position.h"
#include <memory>
#include <chrono>
#include <map>

namespace Playerbot
{

// Forward declarations for specialization classes (QW-4 FIX)
class HolyPaladinRefactored;
class ProtectionPaladinRefactored;
class RetributionPaladinRefactored;

// Type aliases for consistency with base naming
using HolyPaladin = HolyPaladinRefactored;
using ProtectionPaladin = ProtectionPaladinRefactored;
using RetributionPaladin = RetributionPaladinRefactored;

// Paladin AI with full CombatBehaviorIntegration
class TC_GAME_API PaladinAI : public ClassAI
{
public:
    explicit PaladinAI(Player* bot);
    ~PaladinAI();

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

    ::std::unique_ptr<HolyPaladin> _holySpec;
    ::std::unique_ptr<ProtectionPaladin> _protectionSpec;
    ::std::unique_ptr<RetributionPaladin> _retributionSpec;

    // Delegation to specialization
    void DelegateToSpecialization(::Unit* target);

    // Paladin-specific spell IDs (comprehensive list for all specs)
    // Paladin Spell IDs - Using central registry (WoW 12.0 only)
    enum PaladinSpells
    {
        // Interrupts
        REBUKE = WoW120Spells::Paladin::Common::REBUKE,
        HAMMER_OF_JUSTICE = WoW120Spells::Paladin::Common::HAMMER_OF_JUSTICE,

        // Defensive Cooldowns
        DIVINE_SHIELD = WoW120Spells::Paladin::Common::DIVINE_SHIELD,
        SHIELD_OF_VENGEANCE = WoW120Spells::Paladin::Common::SHIELD_OF_VENGEANCE,
        BLESSING_OF_PROTECTION = WoW120Spells::Paladin::Common::BLESSING_OF_PROTECTION,
        ARDENT_DEFENDER = WoW120Spells::Paladin::Common::ARDENT_DEFENDER,
        GUARDIAN_OF_ANCIENT_KINGS = WoW120Spells::Paladin::Common::GUARDIAN_OF_ANCIENT_KINGS,
        LAY_ON_HANDS = WoW120Spells::Paladin::Common::LAY_ON_HANDS,
        DIVINE_PROTECTION = WoW120Spells::Paladin::Common::DIVINE_PROTECTION,

        // Offensive Cooldowns
        AVENGING_WRATH = WoW120Spells::Paladin::Common::AVENGING_WRATH,
        CRUSADE = WoW120Spells::Paladin::Common::CRUSADE,
        HOLY_AVENGER = WoW120Spells::Paladin::Common::HOLY_AVENGER,
        EXECUTION_SENTENCE = WoW120Spells::Paladin::Common::EXECUTION_SENTENCE,

        // Holy Power Generators
        CRUSADER_STRIKE = WoW120Spells::Paladin::Common::CRUSADER_STRIKE,
        BLADE_OF_JUSTICE = WoW120Spells::Paladin::Common::BLADE_OF_JUSTICE,
        HAMMER_OF_THE_RIGHTEOUS = WoW120Spells::Paladin::Common::HAMMER_OF_THE_RIGHTEOUS,
        JUDGMENT = WoW120Spells::Paladin::Common::JUDGMENT,
        WAKE_OF_ASHES = WoW120Spells::Paladin::Common::WAKE_OF_ASHES,

        // Holy Power Spenders
        TEMPLARS_VERDICT = WoW120Spells::Paladin::Common::TEMPLARS_VERDICT,
        FINAL_VERDICT = WoW120Spells::Paladin::Common::FINAL_VERDICT,
        DIVINE_STORM = WoW120Spells::Paladin::Common::DIVINE_STORM,
        SHIELD_OF_THE_RIGHTEOUS = WoW120Spells::Paladin::Common::SHIELD_OF_THE_RIGHTEOUS,
        WORD_OF_GLORY = WoW120Spells::Paladin::Common::WORD_OF_GLORY,

        // AoE Abilities
        CONSECRATION = WoW120Spells::Paladin::Common::CONSECRATION,
        HAMMER_OF_LIGHT = WoW120Spells::Paladin::Protection::LIGHTS_GUIDANCE, // Hero talent (Templar)

        // Auras (WoW 12.0)
        RETRIBUTION_AURA = WoW120Spells::Paladin::Common::RETRIBUTION_AURA,
        DEVOTION_AURA = WoW120Spells::Paladin::Common::DEVOTION_AURA,
        CRUSADER_AURA = WoW120Spells::Paladin::Common::CRUSADER_AURA,
        CONCENTRATION_AURA = WoW120Spells::Paladin::CONCENTRATION_AURA,

        // Blessings (WoW 12.0)
        BLESSING_OF_FREEDOM = WoW120Spells::Paladin::Common::BLESSING_OF_FREEDOM,
        BLESSING_OF_SACRIFICE = WoW120Spells::Paladin::BLESSING_OF_SACRIFICE,

        // Healing Abilities (for Holy spec)
        FLASH_OF_LIGHT = WoW120Spells::Paladin::Common::FLASH_OF_LIGHT,
        HOLY_LIGHT = WoW120Spells::Paladin::Common::HOLY_LIGHT,
        HOLY_SHOCK = WoW120Spells::Paladin::Common::HOLY_SHOCK,
        LIGHT_OF_DAWN = WoW120Spells::Paladin::Common::LIGHT_OF_DAWN,
        BEACON_OF_LIGHT = WoW120Spells::Paladin::Common::BEACON_OF_LIGHT,

        // Utility
        HAND_OF_RECKONING = WoW120Spells::Paladin::Common::HAND_OF_RECKONING,
        CLEANSE = WoW120Spells::Paladin::Common::CLEANSE,
        CLEANSE_TOXINS = WoW120Spells::Paladin::CLEANSE_TOXINS,
        HAMMER_OF_WRATH = WoW120Spells::Paladin::Common::HAMMER_OF_WRATH,
        BLINDING_LIGHT = WoW120Spells::Paladin::Common::BLINDING_LIGHT,
        TURN_EVIL = WoW120Spells::Paladin::TURN_EVIL,

        // Movement
        DIVINE_STEED = WoW120Spells::Paladin::Common::DIVINE_STEED
    };


    // Paladin-specific combat logic
    void ExecuteBasicPaladinRotation(::Unit* target);
    void UpdatePaladinBuffs();
    void UseDefensiveCooldowns();
    void UseOffensiveCooldowns();
    void ManageHolyPower(::Unit* target);
    void UpdateBlessingManagement();
    void UpdateAuraManagement();

    // Holy Power management
    uint32 GetHolyPower() const;
    bool HasHolyPowerFor(uint32 spellId) const;
    void GenerateHolyPower(::Unit* target);
    void SpendHolyPower(::Unit* target);
    bool ShouldBuildHolyPower() const;

    // Utility functions
    bool IsInMeleeRange(::Unit* target) const;
    bool CanInterrupt(::Unit* target) const;
    uint32 GetNearbyEnemyCount(float range) const;
    uint32 GetNearbyAllyCount(float range) const;
    bool IsAllyInDanger() const;
    bool ShouldUseLayOnHands() const;
    Position CalculateOptimalMeleePosition(::Unit* target);
    bool IsValidTarget(::Unit* target);

    // Performance tracking
    void RecordAbilityUsage(uint32 spellId);
    void RecordInterruptAttempt(::Unit* target, uint32 spellId, bool success);
    void AnalyzeCombatEffectiveness();
    void UpdateMetrics(uint32 diff);
    float CalculateHolyPowerEfficiency();

    // Constants
    static constexpr float OPTIMAL_MELEE_RANGE = 5.0f;
    static constexpr float OPTIMAL_HEALING_RANGE = 40.0f;
    static constexpr float CONSECRATION_RADIUS = 8.0f;
    static constexpr float DIVINE_STORM_RADIUS = 8.0f;
    static constexpr uint32 HOLY_POWER_MAX = 5;
    static constexpr uint32 BLESSING_DURATION = 600000;  // 10 minutes
    static constexpr float HEALTH_CRITICAL_THRESHOLD = 20.0f;
    static constexpr float HEALTH_EMERGENCY_THRESHOLD = 30.0f;
    static constexpr float DEFENSIVE_COOLDOWN_THRESHOLD = 40.0f;
    static constexpr float LAY_ON_HANDS_THRESHOLD = 15.0f;
    static constexpr float HOLY_POWER_EFFICIENCY_TARGET = 0.85f;

    // Member variables
    // Cooldown tracking
    uint32 _lastBlessingTime;
    uint32 _lastAuraChange;
    uint32 _lastConsecration;
    uint32 _lastDivineShield;
    uint32 _lastLayOnHands;

    // State tracking
    bool _needsReposition;
    bool _shouldConserveMana;
    uint32 _currentSeal;
    uint32 _currentAura;
    uint32 _currentBlessing;

    // Combat metrics
    struct PaladinMetrics
    {
        uint32 totalAbilitiesUsed;
        uint32 holyPowerGenerated;
        uint32 holyPowerSpent;
        uint32 healingDone;
        uint32 damageDealt;
        float holyPowerEfficiency;
        ::std::chrono::steady_clock::time_point combatStartTime;
        ::std::chrono::steady_clock::time_point lastMetricsUpdate;
    };

    PaladinMetrics _paladinMetrics;
    ::std::map<uint32, uint32> _abilityUsage;
    uint32 _successfulInterrupts;
};

} // namespace Playerbot
