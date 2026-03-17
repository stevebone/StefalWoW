/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef DEATHKNIGHT_AI_H
#define DEATHKNIGHT_AI_H

#include "../ClassAI.h"
#include "DiseaseManager.h"
#include "RuneManager.h"
#include "../Combat/BotThreatManager.h"
#include "../Combat/TargetSelector.h"
#include "../Combat/PositionManager.h"
#include "../Combat/InterruptManager.h"
#include "../Common/CooldownManager.h"
#include <memory>
#include <atomic>
#include <chrono>
#include <unordered_map>

namespace Playerbot
{

// Forward declarations for specialization classes (QW-4 FIX)
class BloodDeathKnightRefactored;
class FrostDeathKnightRefactored;
class UnholyDeathKnightRefactored;

// Type aliases for consistency with base naming
using BloodDeathKnight = BloodDeathKnightRefactored;
using FrostDeathKnight = FrostDeathKnightRefactored;
using UnholyDeathKnight = UnholyDeathKnightRefactored;

// Death Knight Specializations
enum class DeathKnightSpec : uint8
{
    BLOOD = 0,
    FROST = 1,
    UNHOLY = 2
};

// Forward declarations
struct DeathKnightMetrics;
class DeathKnightCombatMetrics;
class DeathKnightCombatPositioning;


class DeathKnightAI : public ClassAI
{
public:
    explicit DeathKnightAI(Player* bot);
    ~DeathKnightAI();

    // Core AI Interface
    void UpdateRotation(Unit* target);
    void UpdateBuffs();
    void UpdateCooldowns(uint32 diff);
    bool CanUseAbility(uint32 spellId);
    void OnCombatStart(Unit* target);
    void OnCombatEnd();

    // Resource Management
    bool HasEnoughResource(uint32 spellId);
    void ConsumeResource(uint32 spellId);

    // Positioning
    Position GetOptimalPosition(Unit* target);
    float GetOptimalRange(Unit* target);

    // Specialization Management
    DeathKnightSpec GetCurrentSpecialization() const;

private:
    // ========================================================================
    // QW-4 FIX: Per-instance specialization objects
    // Each bot has its own specialization object initialized with correct bot pointer
    // ========================================================================

    ::std::unique_ptr<BloodDeathKnight> _bloodSpec;
    ::std::unique_ptr<FrostDeathKnight> _frostSpec;
    ::std::unique_ptr<UnholyDeathKnight> _unholySpec;

    // Delegation to specialization
    void DelegateToSpecialization(::Unit* target);

    // Specialization detection
    void DetectSpecialization();
    // Initialization
    void InitializeCombatSystems();
    // Combat execution
    void ExecuteFallbackRotation(Unit* target);
    void ExecuteSpecializationRotation(Unit* target);
    void ActivateBurstCooldowns(Unit* target);

    // Combat behavior integration helpers
    bool HandleInterrupts(Unit* target);
    bool HandleDefensives();
    bool HandleTargetSwitching(Unit*& target);
    bool HandleAoERotation(Unit* target);
    bool HandleOffensiveCooldowns(Unit* target);
    bool HandleRuneAndPowerManagement(Unit* target);
    void UpdatePresenceIfNeeded();

    // Death Knight specific defensive abilities
    void UseDefensiveCooldowns();
    void UseAntiMagicDefenses(Unit* target);

    // Death Knight specific utility
    bool ShouldUseDeathGrip(Unit* target) const;
    bool ShouldUseDarkCommand(Unit* target) const;
    uint32 GetNearbyEnemyCount(float range) const;
    bool IsInMeleeRange(Unit* target) const;

    // Resource helpers
    bool HasRunesForSpell(uint32 spellId) const;
    uint32 GetRunicPowerCost(uint32 spellId) const;

    // Tracking helpers
    void RecordInterruptAttempt(Unit* target, uint32 spellId, bool success);
    void RecordAbilityUsage(uint32 spellId);
    void OnTargetChanged(Unit* newTarget);
    // Combat systems
    ::std::unique_ptr<BotThreatManager> _threatManager;
    ::std::unique_ptr<TargetSelector> _targetSelector;
    ::std::unique_ptr<PositionManager> _positionManager;
    ::std::unique_ptr<InterruptManager> _interruptManager;
    ::std::unique_ptr<CooldownManager> _cooldownManager;

    // Death Knight-specific systems
    ::std::unique_ptr<DeathKnightMetrics> _metrics;
    ::std::unique_ptr<DeathKnightCombatMetrics> _combatMetrics;
    ::std::unique_ptr<DeathKnightCombatPositioning> _positioning;
    ::std::unique_ptr<RuneManager> _runeManager;
    ::std::unique_ptr<DiseaseManager> _diseaseManager;

    // Specialization
    DeathKnightSpec _detectedSpec;

    // Combat tracking
    uint32 _runicPowerSpent;
    uint32 _runesUsed;
    uint32 _diseasesApplied;
    uint32 _lastPresence;
    uint32 _lastHorn;
    uint32 _lastDeathGrip;
    uint32 _lastDarkCommand;
    uint32 _successfulInterrupts;

    // Ability usage tracking
    ::std::unordered_map<uint32, uint32> _abilityUsage;
};

} // namespace Playerbot

#endif // DEATHKNIGHT_AI_H