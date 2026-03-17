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
#include "Threading/LockHierarchy.h"
#include "Position.h"
#include "../../Combat/InterruptManager.h"
#include "../../Combat/BotThreatManager.h"
#include "../../Combat/TargetSelector.h"
#include "../../Combat/PositionManager.h"
#include <unordered_map>
#include <memory>
#include <atomic>
#include <chrono>
#include <mutex>
#include <queue>
#include <map>

// Forward declarations

namespace Playerbot
{

// Forward declarations for specialization classes (QW-4 FIX)
class AfflictionWarlockRefactored;
class DemonologyWarlockRefactored;
class DestructionWarlockRefactored;

// Type aliases for consistency with base naming
using AfflictionWarlock = AfflictionWarlockRefactored;
using DemonologyWarlock = DemonologyWarlockRefactored;
using DestructionWarlock = DestructionWarlockRefactored;

// Warlock AI implementation with specialization pattern and full CombatBehaviorIntegration
class TC_GAME_API WarlockAI : public ClassAI
{
public:
    explicit WarlockAI(Player* bot);
    ~WarlockAI(); // Implemented in cpp file

    // ClassAI interface implementation
    void UpdateRotation(::Unit* target);
    void UpdateBuffs();
    void UpdateCooldowns(uint32 diff);
    bool CanUseAbility(uint32 spellId);

    // Combat state callbacks
    void OnCombatStart(::Unit* target);
    void OnCombatEnd();

    // Non-combat update (called when NOT in combat)
    // Used for pet summoning, buff maintenance, preparation
    void OnNonCombatUpdate(uint32 diff);

protected:
    // Resource management
    bool HasEnoughResource(uint32 spellId);
    void ConsumeResource(uint32 spellId);

    // Positioning
    Position GetOptimalPosition(::Unit* target);
    float GetOptimalRange(::Unit* target);

private:
    // ========================================================================
    // CombatBehaviorIntegration Priority System
    // ========================================================================

    // Priority 1: Interrupts
    bool HandleInterrupt(Unit* target);

    // Priority 2: Defensives
    bool HandleDefensives();

    // Priority 3: Movement (instant casts while moving)
    bool HandleInstantCasts(Unit* target);

    // Priority 4: Pet Management
    bool HandlePetManagement();
    bool SummonPet();
    void HandlePetSpecialAbilities();

    // Priority 5: Target Switching (DoT application)
    bool ApplyDoTToTarget(Unit* target);

    // Priority 6: Crowd Control
    bool HandleCrowdControl(Unit* target);

    // Priority 7: AoE
    bool HandleAoERotation(Unit* target);

    // Priority 8: Offensive Cooldowns
    bool HandleOffensiveCooldowns(Unit* target);

    // Priority 9: Soul Shard Management
    void HandleSoulShardManagement();
    bool HasHealthstone();
    bool UseHealthstone();
    bool HasSoulstone();

    // Priority 10: Basic Rotation (Shadow Bolt / Corruption)
    void ExecuteBasicRotation(Unit* target);

    // ========================================================================
    // Combat Utilities
    // ========================================================================

    // Enemy detection
    Unit* GetNearestEnemy(float range);
    uint32 GetNearbyEnemyCount(float range);

    // DoT management
    bool ApplyCurse(Unit* target);
    ::std::map<ObjectGuid, ::std::map<uint32, uint32>> _dotTracker; // target -> spell -> timestamp

    // Pet ability tracking
    ::std::map<uint32, uint32> _petAbilityCooldowns; // ability -> last use time

    // Combat metrics
    void UpdateCombatMetrics();

    // ========================================================================
    // Specialization System
    // ========================================================================
    // ========================================================================
    // Performance Metrics
    // ========================================================================

    struct WarlockMetrics {
        ::std::atomic<uint32> manaSpent{0};
        ::std::atomic<uint32> damageDealt{0};
        ::std::atomic<uint32> dotDamage{0};
        ::std::atomic<uint32> petDamage{0};
        ::std::atomic<uint32> soulShardsUsed{0};
        ::std::atomic<uint32> lifeTapsCast{0};
        ::std::atomic<uint32> spellsCast{0};
        ::std::atomic<float> manaEfficiency{0.0f};
        ::std::atomic<float> petUptime{0.0f};
        ::std::atomic<float> dotUptime{0.0f};
        ::std::chrono::steady_clock::time_point combatStartTime;
        ::std::chrono::steady_clock::time_point lastUpdate;
        void Reset()
        {
            manaSpent = 0; damageDealt = 0; dotDamage = 0; petDamage = 0;
            soulShardsUsed = 0; lifeTapsCast = 0; spellsCast = 0;
            manaEfficiency = 0.0f; petUptime = 0.0f; dotUptime = 0.0f;
            combatStartTime = ::std::chrono::steady_clock::now();
            lastUpdate = combatStartTime;
        }
    } _warlockMetrics;

    ::std::unordered_map<uint32, ::std::atomic<uint32>> _abilityUsage;

    // ========================================================================
    // Warlock-Specific Utilities
    // ========================================================================

    void UpdateWarlockBuffs();
    void UpdatePetCheck();
    void UpdateSoulShardCheck();

    // Enhanced warlock mechanics
    void OptimizeManaManagement();
    void ManageLifeTapTiming();
    void OptimizePetPositioning();
    void ManageWarlockCooldowns();
    void OptimizeSoulShardUsage();
    void HandleAoESituations();
    void ManageCurseApplication();
    void OptimizeDoTRotation();

    // ========================================================================
    // Combat System Integration
    // ========================================================================

    ::std::unique_ptr<BotThreatManager> _threatManager;
    ::std::unique_ptr<TargetSelector> _targetSelector;
    ::std::unique_ptr<PositionManager> _positionManager;
    ::std::unique_ptr<InterruptManager> _interruptManager;

    // ========================================================================
    // QW-4 FIX: Per-instance specialization objects
    // Each bot has its own specialization object initialized with correct bot pointer
    // ========================================================================

    ::std::unique_ptr<AfflictionWarlock> _afflictionSpec;
    ::std::unique_ptr<DemonologyWarlock> _demonologySpec;
    ::std::unique_ptr<DestructionWarlock> _destructionSpec;

    // Delegation to specialization
    void DelegateToSpecialization(::Unit* target);

    // ========================================================================
    // Resource Tracking
    // ========================================================================

    // Soul shard tracking
    ::std::atomic<uint32> _currentSoulShards{0};
    ::std::queue<uint32> _soulShardHistory;
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE> _soulShardMutex;

    // Pet management
    ::std::atomic<bool> _petActive{false};
    ::std::atomic<uint32> _petHealthPercent{0};
    ::std::chrono::steady_clock::time_point _lastPetCheck;

    // Mana management
    float _optimalManaThreshold;
    ::std::atomic<bool> _lowManaMode{false};
    uint32 _lastLifeTapTime;

    // ========================================================================
    // Constants
    // ========================================================================

    static constexpr uint32 SOUL_SHARD_UPDATE_INTERVAL = 1000; // 1 second
    static constexpr uint32 PET_CHECK_INTERVAL = 2000; // 2 seconds
    static constexpr float LOW_MANA_THRESHOLD = 0.3f; // 30%
    static constexpr float LIFE_TAP_THRESHOLD = 0.8f; // 80% health
    static constexpr uint32 COMBAT_METRICS_UPDATE_INTERVAL = 500; // 0.5 seconds

    // ========================================================================
    // Helper Methods
    // ========================================================================

    bool HasEnoughMana(uint32 amount);
    uint32 GetMana();
    uint32 GetMaxMana();
    float GetManaPercent();
    void UseDefensiveAbilities();
    void UseCrowdControl(::Unit* target);
    void UpdatePetManagement();
    bool ShouldConserveMana();

    // ========================================================================
    // Tracking Variables
    // ========================================================================

    uint32 _manaSpent;
    uint32 _damageDealt;
    uint32 _soulshardsUsed;
    uint32 _fearsUsed;
    uint32 _petsSpawned;
    uint32 _lastFear;
    uint32 _lastPetSummon;
};

} // namespace Playerbot
