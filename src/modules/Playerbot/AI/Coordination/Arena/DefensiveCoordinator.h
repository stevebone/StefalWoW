/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "ArenaState.h"
#include <vector>
#include <map>
#include <queue>

namespace Playerbot {

class ArenaCoordinator;

/**
 * @enum DefensiveType
 * @brief Types of defensive abilities
 */
enum class DefensiveType : uint8
{
    PERSONAL_IMMUNITY = 0,      // Ice Block, Divine Shield
    PERSONAL_WALL = 1,          // Icebound Fortitude, Barkskin
    PERSONAL_ABSORB = 2,        // Power Word: Shield, Ice Barrier
    EXTERNAL_HEAL = 3,          // External healing CDs
    EXTERNAL_SHIELD = 4,        // Pain Suppression, Ironbark
    EXTERNAL_IMMUNITY = 5,      // Blessing of Protection
    CC_BREAK = 6,               // Trinket, Every Man for Himself
    MOBILITY = 7,               // Blink, Sprint
    THREAT_DROP = 8             // Vanish, Feign Death
};

/**
 * @struct DefensiveCooldown
 * @brief Represents a defensive cooldown
 */
struct DefensiveCooldown
{
    uint32 spellId;
    DefensiveType type;
    uint32 cooldownDuration;
    uint32 effectDuration;
    float damageReduction;
    bool isExternal;
    bool breaksCC;
    bool providesImmunity;
    uint32 readyTime;
    bool isActive;
    uint32 activeEndTime;

    DefensiveCooldown()
        : spellId(0), type(DefensiveType::PERSONAL_WALL),
          cooldownDuration(0), effectDuration(0), damageReduction(0),
          isExternal(false), breaksCC(false), providesImmunity(false),
          readyTime(0), isActive(false), activeEndTime(0) {}
};

/**
 * @struct ThreatAssessment
 * @brief Assesses the threat level on a teammate
 */
struct ThreatAssessment
{
    ObjectGuid target;
    float healthPercent;
    float healthDeficit;
    float incomingDamageRate;  // DPS taken
    uint32 attackerCount;
    bool isBeingFocused;
    bool hasActiveCCs;
    bool hasDefensivesUp;
    DefensiveState state;
    uint8 urgencyLevel;  // 0-3

    ThreatAssessment()
        : target(), healthPercent(100.0f), healthDeficit(0),
          incomingDamageRate(0), attackerCount(0),
          isBeingFocused(false), hasActiveCCs(false), hasDefensivesUp(false),
          state(DefensiveState::HEALTHY), urgencyLevel(0) {}
};

/**
 * @struct PeelAssignment
 * @brief Represents a peel assignment for a teammate
 */
struct PeelAssignment
{
    ObjectGuid peeler;
    ObjectGuid target;      // Teammate being protected
    ObjectGuid threat;      // Enemy to peel off
    uint32 assignTime;
    uint32 duration;
    bool isActive;

    PeelAssignment()
        : peeler(), target(), threat(),
          assignTime(0), duration(0), isActive(false) {}
};

/**
 * @class DefensiveCoordinator
 * @brief Coordinates defensive cooldowns and peeling in arena
 *
 * Manages team survivability including:
 * - Monitoring teammate health and threat
 * - Coordinating defensive cooldown usage
 * - Assigning peels to protect endangered teammates
 * - Tracking enemy offensive pressure
 */
class DefensiveCoordinator
{
public:
    DefensiveCoordinator(ArenaCoordinator* coordinator);

    void Initialize();
    void Update(uint32 diff);
    void Reset();

    // ========================================================================
    // THREAT ASSESSMENT
    // ========================================================================

    ThreatAssessment AssessTeammate(ObjectGuid teammate) const;
    DefensiveState GetTeammateState(ObjectGuid teammate) const;
    ObjectGuid GetMostEndangeredTeammate() const;
    float GetTeamThreatLevel() const;
    ::std::vector<ThreatAssessment> GetAllAssessments() const;

    // ========================================================================
    // PEEL MANAGEMENT
    // ========================================================================

    void RequestPeel(ObjectGuid teammate, ObjectGuid threat, uint8 urgency);
    void AssignPeel(ObjectGuid peeler, ObjectGuid teammate, ObjectGuid threat);
    void CancelPeel(ObjectGuid peeler);
    const PeelAssignment* GetPeelAssignment(ObjectGuid peeler) const;
    ObjectGuid GetPeelTarget() const;  // Who needs peel most
    ObjectGuid GetBestPeeler(ObjectGuid target, ObjectGuid threat) const;
    bool IsPeeling(ObjectGuid player) const;
    ::std::vector<PeelRequest> GetPendingPeelRequests() const { return _pendingPeels; }

    // ========================================================================
    // DEFENSIVE COOLDOWN MANAGEMENT
    // ========================================================================

    bool ShouldUseDefensive(ObjectGuid player) const;
    uint32 GetRecommendedDefensive(ObjectGuid player) const;
    bool HasDefensivesAvailable(ObjectGuid player) const;
    ::std::vector<DefensiveCooldown> GetAvailableDefensives(ObjectGuid player) const;
    void OnDefensiveUsed(ObjectGuid player, uint32 spellId);
    void OnDefensiveExpired(ObjectGuid player, uint32 spellId);

    // ========================================================================
    // EXTERNAL DEFENSIVE COORDINATION
    // ========================================================================

    ObjectGuid GetExternalDefensiveTarget() const;
    bool ShouldUseExternalDefensive(ObjectGuid healer, ObjectGuid target) const;
    uint32 GetRecommendedExternalDefensive(ObjectGuid healer, ObjectGuid target) const;
    void RequestExternalDefensive(ObjectGuid requester, uint8 urgency);

    // ========================================================================
    // CC BREAK COORDINATION
    // ========================================================================

    bool ShouldTrinket(ObjectGuid player) const;
    bool ShouldBreakCC(ObjectGuid player) const;
    uint8 GetCCBreakPriority(ObjectGuid player) const;
    void OnTrinketUsed(ObjectGuid player);

    // ========================================================================
    // DAMAGE TRACKING
    // ========================================================================

    void OnDamageTaken(ObjectGuid target, ObjectGuid attacker, uint32 damage);
    float GetRecentDamageTaken(ObjectGuid player) const;
    float GetDamageRate(ObjectGuid player) const;  // DPS taken
    ObjectGuid GetPrimaryAttacker(ObjectGuid target) const;

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    void SetHealthThresholds(float pressured, float danger, float critical);
    void SetPeelDuration(uint32 ms) { _peelDuration = ms; }

private:
    ArenaCoordinator* _coordinator;

    // ========================================================================
    // PEEL TRACKING
    // ========================================================================

    ::std::vector<PeelRequest> _pendingPeels;
    ::std::map<ObjectGuid, PeelAssignment> _activePeels;  // peeler -> assignment

    // ========================================================================
    // DEFENSIVE TRACKING
    // ========================================================================

    ::std::map<ObjectGuid, ::std::vector<DefensiveCooldown>> _playerDefensives;

    // ========================================================================
    // DAMAGE TRACKING
    // ========================================================================

    struct DamageRecord
    {
        uint32 timestamp;
        ObjectGuid attacker;
        uint32 damage;
    };
    ::std::map<ObjectGuid, ::std::vector<DamageRecord>> _recentDamage;
    uint32 _damageTrackingWindow = 5000;  // 5 seconds

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    float _healthThresholdPressured = 80.0f;
    float _healthThresholdDanger = 50.0f;
    float _healthThresholdCritical = 30.0f;
    uint32 _peelDuration = 5000;  // 5 second peel assignments
    uint32 _damageRateThreshold = 10000;  // 10k DPS = danger

    // ========================================================================
    // THREAT ASSESSMENT
    // ========================================================================

    DefensiveState CalculateDefensiveState(ObjectGuid player) const;
    uint8 CalculateUrgency(const ThreatAssessment& assessment) const;
    bool IsBeingFocused(ObjectGuid player) const;

    // ========================================================================
    // PEEL LOGIC
    // ========================================================================

    void UpdatePeelAssignments(uint32 diff);
    void ProcessPeelRequests();
    bool CanPeel(ObjectGuid peeler, ObjectGuid threat) const;
    float ScorePeeler(ObjectGuid peeler, ObjectGuid target, ObjectGuid threat) const;

    // ========================================================================
    // DEFENSIVE COOLDOWN LOGIC
    // ========================================================================

    void LoadPlayerDefensives(ObjectGuid player);
    void UpdateCooldownTimers(uint32 diff);
    void UpdateActiveDefensives(uint32 diff);
    float ScoreDefensiveValue(const DefensiveCooldown& defensive, const ThreatAssessment& threat) const;

    // ========================================================================
    // CC BREAK LOGIC
    // ========================================================================

    bool IsCCDangerous(ObjectGuid player) const;
    bool WillDieInCC(ObjectGuid player, uint32 ccDuration) const;
    bool IsHealerCCed() const;

    // ========================================================================
    // DAMAGE TRACKING
    // ========================================================================

    void CleanOldDamageRecords(uint32 currentTime);
    void RecordDamage(ObjectGuid target, ObjectGuid attacker, uint32 damage);
};

} // namespace Playerbot
