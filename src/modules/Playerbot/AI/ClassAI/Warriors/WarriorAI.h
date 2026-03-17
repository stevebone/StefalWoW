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
#include "../../Combat/BotThreatManager.h"
#include "../../Combat/TargetSelector.h"
#include "../../Combat/PositionManager.h"
#include "../../Combat/InterruptManager.h"
#include "../../Combat/FormationManager.h"
#include <unordered_map>
#include <memory>
#include <atomic>
#include <chrono>

namespace Playerbot
{

// Forward declarations for specialization classes (QW-4 FIX)
class ArmsWarriorRefactored;
class FuryWarriorRefactored;
class ProtectionWarriorRefactored;

// Type aliases for consistency with base naming
using ArmsWarrior = ArmsWarriorRefactored;
using FuryWarrior = FuryWarriorRefactored;
using ProtectionWarrior = ProtectionWarriorRefactored;

// Warrior AI implementation
class TC_GAME_API WarriorAI : public ClassAI
{
public:
    explicit WarriorAI(Player* bot);
    ~WarriorAI();

    // ClassAI interface implementation
    void UpdateRotation(::Unit* target);
    void UpdateBuffs();
    void UpdateCooldowns(uint32 diff);
    bool CanUseAbility(uint32 spellId);

    // Combat state callbacks
    void OnCombatStart(::Unit* target);
    void OnCombatEnd();

    // Warrior stance management (public for specialization classes)
    enum class WarriorStance : uint8
    {
        NONE = 0,
        BATTLE = 1,
        DEFENSIVE = 2,
        BERSERKER = 3
    };

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

    ::std::unique_ptr<ArmsWarrior> _armsSpec;
    ::std::unique_ptr<FuryWarrior> _furySpec;
    ::std::unique_ptr<ProtectionWarrior> _protectionSpec;

    // Enhanced performance tracking
    ::std::atomic<uint32> _rageSpent{0};
    ::std::atomic<uint32> _damageDealt{0};
    ::std::atomic<uint32> _damageAbsorbed{0};
    ::std::atomic<uint32> _threatGenerated{0};
    ::std::atomic<uint32> _successfulCharges{0};
    ::std::atomic<uint32> _successfulInterrupts{0};
    uint32 _lastStanceChange;

    // Combat system integration
    ::std::unique_ptr<ThreatManager> _threatManager;
    ::std::unique_ptr<TargetSelector> _targetSelector;
    ::std::unique_ptr<PositionManager> _positionManager;
    ::std::unique_ptr<InterruptManager> _interruptManager;
    ::std::unique_ptr<FormationManager> _formationManager;

    // Shared utility tracking
    ::std::unordered_map<uint32, uint32> _abilityUsage;
    uint32 _lastBattleShout;
    uint32 _lastCommandingShout;
    bool _needsIntercept;
    bool _needsCharge;
    ::Unit* _lastChargeTarget;
    uint32 _lastChargeTime;

    void UpdateSpecialization();
    // Delegation to specialization
    void DelegateToSpecialization(::Unit* target);

    // Shared warrior utilities
    void UpdateWarriorBuffs();
    void CastBattleShout();
    void CastCommandingShout();
    void UseChargeAbilities(::Unit* target);
    bool IsInMeleeRange(::Unit* target) const;
    bool CanCharge(::Unit* target) const;

    // Enhanced target evaluation and combat AI
    bool IsValidTarget(::Unit* target);
    ::Unit* GetBestChargeTarget();
    ::Unit* GetHighestThreatTarget();
    ::Unit* GetLowestHealthEnemy();
    ::Unit* SelectOptimalTarget(const ::std::vector<::Unit*>& enemies);
    float CalculateTargetPriority(::Unit* target);

    // Advanced combat mechanics
    void UpdateAdvancedCombatLogic(::Unit* target);
    void HandleMultipleEnemies(const ::std::vector<::Unit*>& enemies);
    void OptimizeStanceForSituation(::Unit* target);
    void ManageRageEfficiency();
    void ExecuteAdvancedRotation(::Unit* target);

    // Stance dancing optimization
    void OptimizeStanceDancing(::Unit* target);
    WarriorStance DetermineOptimalStance(::Unit* target, const ::std::vector<::Unit*>& enemies);
    void HandleStanceSpecificAbilities(WarriorStance stance, ::Unit* target);
    void ManageTacticalStanceSwitching();

    // Defensive and survival mechanics
    void HandleDefensiveSituation();
    void UseDefensiveCooldowns();
    void ManageHealthThresholds();
    void HandleCrowdControl();
    void OptimizeInterruptTiming();

    // Group combat optimization
    void HandleGroupCombatRole();
    void ManageThreatInGroup();
    void CoordinateWithGroup();
    void OptimizeFormationPosition();

    // Performance metrics and analytics
    struct WarriorMetrics {
        ::std::atomic<uint32> totalAbilitiesUsed{0};
        ::std::atomic<uint32> successfulStanceChanges{0};
        ::std::atomic<uint32> rageDumpInstances{0};
        ::std::atomic<float> averageRageEfficiency{0.0f};
        ::std::atomic<float> stanceOptimizationScore{0.0f};
        ::std::atomic<float> survivabilityScore{0.0f};
        ::std::chrono::steady_clock::time_point combatStartTime;
        ::std::chrono::steady_clock::time_point lastMetricsUpdate;
        void Reset()
        {
            totalAbilitiesUsed = 0; successfulStanceChanges = 0; rageDumpInstances = 0;
            averageRageEfficiency = 0.0f; stanceOptimizationScore = 0.0f; survivabilityScore = 0.0f;
            combatStartTime = ::std::chrono::steady_clock::now();
            lastMetricsUpdate = combatStartTime;
        }
    } _warriorMetrics;

    // Enhanced performance optimization
    void RecordAbilityUsage(uint32 spellId);
    void RecordStanceChange(WarriorStance fromStance, WarriorStance toStance);
    void RecordChargeSuccess(::Unit* target, bool success);
    void RecordInterruptAttempt(::Unit* target, uint32 spellId, bool success);
    void AnalyzeCombatEffectiveness();
    void UpdateMetrics(uint32 diff);
    float CalculateRageEfficiency();
    void OptimizeAbilityPriorities();

    // Rage management optimization
    void OptimizeRageUsage();
    bool ShouldConserveRage();
    bool ShouldDumpRage();
    void HandleRageStarvation();
    void PrioritizeRageSpenders(::Unit* target);

    // Weapon and equipment optimization
    void OptimizeWeaponChoice();
    void HandleWeaponSwapping();
    void UpdateWeaponSpecialization();
    bool ShouldUseShield();

    // Mobility and positioning
    void OptimizeMobility(::Unit* target);
    void HandleChargeOpportunities(::Unit* target);
    void ManageInterceptUsage(::Unit* target);
    void OptimizeHeroicLeapTiming();
    Position CalculateOptimalChargePosition(::Unit* target);

    // Advanced threat management
    void ManageAdvancedThreat();
    void BuildThreatOnTarget(::Unit* target);
    void ReduceThreatWhenNeeded();
    void HandleThreatEmergency();

    // Combat behavior integration helpers
    void ExecuteBasicWarriorRotation(::Unit* target);
    uint32 GetNearbyEnemyCount(float range) const;

    // Enhanced constants
    static constexpr uint32 STANCE_CHANGE_COOLDOWN = 1000; // 1 second
    static constexpr uint32 CHARGE_MIN_RANGE = 8;
    static constexpr uint32 CHARGE_MAX_RANGE = 25;
    static constexpr uint32 INTERCEPT_MIN_RANGE = 8;
    static constexpr uint32 INTERCEPT_MAX_RANGE = 25;
    static constexpr uint32 BATTLE_SHOUT_DURATION = 120000; // 2 minutes
    static constexpr uint32 COMMANDING_SHOUT_DURATION = 120000; // 2 minutes
    static constexpr float OPTIMAL_MELEE_RANGE = 5.0f;
    static constexpr float OPTIMAL_CHARGE_DISTANCE = 15.0f;
    static constexpr float THREAT_MANAGEMENT_RANGE = 30.0f;
    static constexpr float RAGE_EFFICIENCY_TARGET = 0.85f; // 85% efficiency
    static constexpr uint32 RAGE_CONSERVATION_THRESHOLD = 20;
    static constexpr uint32 RAGE_DUMP_THRESHOLD = 80;
    static constexpr uint32 HEALTH_EMERGENCY_THRESHOLD = 25; // 25% health
    static constexpr uint32 DEFENSIVE_COOLDOWN_THRESHOLD = 40; // 40% health
    static constexpr uint32 STANCE_OPTIMIZATION_INTERVAL = 3000; // 3 seconds
    static constexpr float MULTI_TARGET_THRESHOLD = 3.0f; // 3+ enemies
    static constexpr uint32 FORMATION_CHECK_INTERVAL = 2000; // 2 seconds

    // Spell IDs - Using central registry (WoW 12.0)
    enum WarriorSpells
    {
        // Stances
        BATTLE_STANCE = WoW120Spells::Warrior::Common::BATTLE_STANCE,
        DEFENSIVE_STANCE = WoW120Spells::Warrior::Common::DEFENSIVE_STANCE,
        BERSERKER_STANCE = WoW120Spells::Warrior::Common::BERSERKER_STANCE,

        // Basic attacks
        HEROIC_STRIKE = WoW120Spells::Warrior::Common::HEROIC_STRIKE,
        CLEAVE = WoW120Spells::Warrior::Common::CLEAVE,
        WHIRLWIND = WoW120Spells::Warrior::Common::WHIRLWIND,

        // Arms abilities
        MORTAL_STRIKE = WoW120Spells::Warrior::Common::MORTAL_STRIKE,
        COLOSSUS_SMASH = WoW120Spells::Warrior::Common::COLOSSUS_SMASH,
        OVERPOWER = WoW120Spells::Warrior::Common::OVERPOWER,
        REND = WoW120Spells::Warrior::Common::REND,

        // Fury abilities
        BLOODTHIRST = WoW120Spells::Warrior::Common::BLOODTHIRST,
        RAMPAGE = WoW120Spells::Warrior::Common::RAMPAGE,
        RAGING_BLOW = WoW120Spells::Warrior::Common::RAGING_BLOW,
        EXECUTE = WoW120Spells::Warrior::Common::EXECUTE,

        // Protection abilities
        SHIELD_SLAM = WoW120Spells::Warrior::Common::SHIELD_SLAM,
        THUNDER_CLAP = WoW120Spells::Warrior::Common::THUNDER_CLAP,
        REVENGE = WoW120Spells::Warrior::Common::REVENGE,
        DEVASTATE = WoW120Spells::Warrior::Common::DEVASTATE,
        SHIELD_BLOCK = WoW120Spells::Warrior::Common::SHIELD_BLOCK,

        // Defensive cooldowns
        SHIELD_WALL = WoW120Spells::Warrior::Common::SHIELD_WALL,
        LAST_STAND = WoW120Spells::Warrior::Common::LAST_STAND,
        SPELL_REFLECTION = WoW120Spells::Warrior::Common::SPELL_REFLECTION,

        // Offensive cooldowns
        RECKLESSNESS = WoW120Spells::Warrior::Common::RECKLESSNESS,
        BLADESTORM = WoW120Spells::Warrior::Common::BLADESTORM,
        AVATAR = WoW120Spells::Warrior::Common::AVATAR,

        // Movement abilities
        CHARGE = WoW120Spells::Warrior::Common::CHARGE,
        INTERCEPT = WoW120Spells::Warrior::Common::INTERVENE, // INTERCEPT merged with INTERVENE in 12.0
        HEROIC_LEAP = WoW120Spells::Warrior::Common::HEROIC_LEAP,

        // Utility
        PUMMEL = WoW120Spells::Warrior::Common::PUMMEL,
        DISARM = 676, // Removed in modern WoW, keeping legacy ID
        TAUNT = WoW120Spells::Warrior::Common::TAUNT,
        SUNDER_ARMOR = 7386, // Removed in modern WoW, keeping legacy ID

        // Buffs
        BATTLE_SHOUT = WoW120Spells::Warrior::Common::BATTLE_SHOUT,
        COMMANDING_SHOUT = WoW120Spells::Warrior::Common::COMMANDING_SHOUT,

        // Weapon buffs (if applicable)
        WEAPON_MASTER = 16538 // Passive talent, no spell ID needed
    };
};

} // namespace Playerbot
