/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Define.h"
#include "Position.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <chrono>

// Forward declarations
class Player;
class Unit;
class Creature;
class SpellInfo;
class Aura;

namespace Playerbot
{

// Combat role types for positioning and behavior
enum class CombatRole : uint8
{
    TANK      = 0,
    HEALER    = 1,
    MELEE_DPS = 2,
    RANGED_DPS = 3,
    HYBRID    = 4
};

// Resource types for unified resource management
enum class ResourceType : uint8
{
    MANA        = 0,
    RAGE        = 1,
    FOCUS       = 2,
    ENERGY      = 3,
    COMBO_POINTS = 4,
    RUNES       = 5,
    RUNIC_POWER = 6,
    SOUL_SHARDS = 7,
    LUNAR_POWER = 8,
    HOLY_POWER  = 9,
    MAELSTROM   = 10,
    CHI         = 11,
    INSANITY    = 12,
    BURNING_EMBERS = 13,
    DEMONIC_FURY = 14,
    ARCANE_CHARGES = 15,
    FURY        = 16,
    PAIN        = 17
};

// Performance metrics tracking
struct PerformanceMetrics
{
    uint32 totalCasts = 0;
    uint32 failedCasts = 0;
    uint32 resourceWasted = 0;
    uint32 interruptsSuccessful = 0;
    uint32 interruptsFailed = 0;
    uint32 positioningUpdates = 0;
    uint32 emergencyActions = 0;
    uint64 totalDamageDealt = 0;
    uint64 totalHealingDone = 0;
    uint64 totalDamageTaken = 0;
    ::std::chrono::milliseconds totalCombatTime{0};
    ::std::chrono::steady_clock::time_point combatStartTime;
};

// Base class for ALL combat specializations
class TC_GAME_API CombatSpecializationBase
{
public:
    explicit CombatSpecializationBase(Player* bot, CombatRole role, ResourceType primaryResource);
    virtual ~CombatSpecializationBase() = default;

    // Core specialization interface (pure virtual - must be implemented)
    virtual void UpdateRotation(::Unit* target) = 0;

    // Common implementations that can be overridden if needed
    virtual void UpdateBuffs();
    virtual void UpdateCooldowns(uint32 diff);
    virtual bool CanUseAbility(uint32 spellId);

    // Combat lifecycle management
    virtual void OnCombatStart(::Unit* target);
    virtual void OnCombatEnd();
    virtual void OnTargetSwitch(::Unit* oldTarget, ::Unit* newTarget);
    virtual void OnDamageTaken(::Unit* attacker, uint32 damage);
    virtual void OnDamageDealt(::Unit* target, uint32 damage);
    virtual void OnHealingReceived(::Unit* healer, uint32 amount);
    virtual void OnHealingDone(::Unit* target, uint32 amount);

    // Resource management (common implementation)
    virtual bool HasEnoughResource(uint32 spellId);
    virtual void ConsumeResource(uint32 spellId);
    virtual uint32 GetCurrentResource() const;
    virtual uint32 GetMaxResource() const;
    virtual float GetResourcePercent() const;
    virtual void RegenerateResource(uint32 diff);

    // Positioning (common implementation with role-based defaults)
    virtual Position GetOptimalPosition(::Unit* target);
    virtual float GetOptimalRange(::Unit* target);
    virtual bool IsInOptimalPosition(::Unit* target) const;
    virtual void UpdatePositioning(::Unit* target);
    virtual bool ShouldReposition(::Unit* target) const;

    // Buff management
    virtual bool ShouldRefreshBuff(uint32 spellId, uint32 remainingTime = 0) const;
    virtual void RefreshExpiringBuffs();
    virtual bool HasBuff(uint32 spellId) const;
    virtual uint32 GetBuffRemainingTime(uint32 spellId) const;
    virtual void ApplyBuff(uint32 spellId);

    // Cooldown management
    virtual bool IsSpellReady(uint32 spellId) const;
    virtual void SetSpellCooldown(uint32 spellId, uint32 cooldownMs);
    virtual uint32 GetSpellCooldown(uint32 spellId) const;
    virtual void ResetCooldown(uint32 spellId);
    virtual void ResetAllCooldowns();
    virtual bool HasGlobalCooldown() const;

    // Interrupt coordination
    virtual bool ShouldInterrupt(::Unit* target);
    virtual bool TryInterrupt(::Unit* target);
    virtual uint32 GetInterruptSpellId() const;
    virtual bool IsInterruptReady() const;
    virtual void OnInterruptSuccess(::Unit* target);
    virtual void OnInterruptFailed(::Unit* target);

    // Target management
    virtual ::Unit* SelectBestTarget();
    virtual ::std::vector<::Unit*> GetNearbyEnemies(float range = 40.0f) const;
    virtual ::std::vector<::Unit*> GetNearbyAllies(float range = 40.0f) const;
    virtual bool IsValidTarget(::Unit* target) const;
    virtual float CalculateThreatLevel(::Unit* target) const;

    // Emergency response
    virtual void HandleEmergencySituation();
    virtual bool IsInEmergencyState() const;
    virtual void UseDefensiveCooldowns();
    virtual void UseOffensiveCooldowns();
    virtual bool ShouldUsePotions() const;
    virtual void UsePotions();

    // Performance tracking
    const PerformanceMetrics& GetMetrics() const { return _metrics; }
    void ResetMetrics();
    void LogPerformance() const;

    // Utility functions
    bool CastSpell(uint32 spellId, ::Unit* target = nullptr);
    bool IsCasting() const;
    bool IsChanneling() const;
    bool IsMoving() const;
    bool IsBehindTarget(::Unit* target) const;
    bool IsInMeleeRange(::Unit* target) const;
    bool IsInCastRange(::Unit* target, uint32 spellId) const;
    float GetDistance(::Unit* target) const;

    // Configuration
    void SetCombatRole(CombatRole role) { _role = role; }
    CombatRole GetCombatRole() const { return _role; }
    void SetPrimaryResource(ResourceType type) { _primaryResource = type; }
    ResourceType GetPrimaryResource() const { return _primaryResource; }

protected:
    // Protected helper methods for specializations
    Player* GetBot() const { return _bot; }
    bool HasSpell(uint32 spellId) const;
    SpellInfo const* GetSpellInfo(uint32 spellId) const;
    uint32 GetSpellCastTime(uint32 spellId) const;
    uint32 GetSpellManaCost(uint32 spellId) const;

    // Common rotation helpers
    void UpdateDoTTracking(::Unit* target);
    bool ShouldRefreshDoT(::Unit* target, uint32 spellId, uint32 threshold = 3000) const;
    uint32 GetDoTRemainingTime(::Unit* target, uint32 spellId) const;

    // AoE management
    bool ShouldUseAoE() const;
    uint32 GetEnemiesInRange(float range) const;
    Position GetBestAoELocation(float radius) const;

    // Proc management
    void CheckForProcs();
    bool HasProc(uint32 procId) const;
    void ConsumeProc(uint32 procId);
    uint32 GetProcRemainingTime(uint32 procId) const;

    // Movement prediction
    Position PredictTargetPosition(::Unit* target, uint32 timeMs) const;
    bool WillTargetMoveOutOfRange(::Unit* target, uint32 timeMs) const;

    // Threat management
    void UpdateThreatTable();
    bool IsMainTank() const;
    bool ShouldReduceThreat() const;
    void ReduceThreat();

    // Group coordination
    bool IsInGroup() const;
    bool IsInRaid() const;
    Player* GetGroupTank() const;
    Player* GetGroupHealer() const;
    ::std::vector<Player*> GetGroupMembers() const;

    // Constants for common use
    static constexpr float BOT_MELEE_RANGE = 5.0f;
    static constexpr float RANGED_MIN_DISTANCE = 8.0f;
    static constexpr float RANGED_OPTIMAL_DISTANCE = 25.0f;
    static constexpr float MAX_SPELL_RANGE = 40.0f;
    static constexpr uint32 GLOBAL_COOLDOWN_MS = 1500;
    static constexpr uint32 BUFF_REFRESH_THRESHOLD_MS = 5000;
    static constexpr uint32 DOT_REFRESH_THRESHOLD_MS = 3000;
    static constexpr float EMERGENCY_HEALTH_PCT = 30.0f;
    static constexpr float LOW_RESOURCE_PCT = 20.0f;

protected:
    Player* _bot;
    CombatRole _role;
    ResourceType _primaryResource;

    // Cooldown tracking
    ::std::unordered_map<uint32, uint32> _cooldowns;
    uint32 _globalCooldownEnd;

    // Buff tracking
    ::std::unordered_map<uint32, uint32> _buffExpirationTimes;

    // DoT tracking (targetGuid -> (spellId -> expirationTime))
    ::std::unordered_map<uint64, ::std::unordered_map<uint32, uint32>> _dotTracking;

    // Proc tracking
    ::std::unordered_map<uint32, uint32> _procExpirationTimes;

    // Threat tracking
    ::std::unordered_map<uint64, float> _threatTable;
    uint32 _lastThreatUpdate;

    // Performance metrics
    PerformanceMetrics _metrics;

    // State tracking
    ::Unit* _currentTarget;
    bool _inCombat;
    uint32 _combatStartTime;
    uint32 _lastPositionUpdate;
    uint32 _lastBuffCheck;
    uint32 _lastResourceRegen;
    uint32 _lastEmergencyCheck;
    uint32 _consecutiveFailedCasts;

    // Position caching
    Position _lastOptimalPosition;
    uint32 _lastOptimalPositionCheck;

    // Group information caching
    ::std::vector<Player*> _cachedGroupMembers;
    uint32 _lastGroupUpdate;
    Player* _cachedTank;
    Player* _cachedHealer;

    // QW-4 FIX: Per-instance cleanup throttle (was static - caused cross-bot contamination)
    uint32 _lastCooldownCleanup = 0;

private:
    // Internal update methods
    void UpdateGlobalCooldown(uint32 diff);
    void UpdateBuffTimers(uint32 diff);
    void UpdateDotTimers(uint32 diff);
    void UpdateProcTimers(uint32 diff);
    void CleanupExpiredData();
    void ValidateCooldowns();
    void ValidateBuffs();
    void UpdateMetrics(uint32 diff);
};

} // namespace Playerbot