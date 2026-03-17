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

#include "BGState.h"
#include <vector>
#include <map>

namespace Playerbot {

class BattlegroundCoordinator;

/**
 * @enum FlagState
 * @brief Current state of a flag
 */
enum class FlagState : uint8
{
    AT_BASE = 0,
    CARRIED = 1,
    DROPPED = 2,
    RESPAWNING = 3
};

/**
 * @struct FlagCarrierStatus
 * @brief Status of the flag carrier
 */
struct FlagCarrierStatus
{
    ObjectGuid carrier;
    float healthPercent;
    bool isUnderAttack;
    uint8 attackerCount;
    float distanceToCapture;
    uint8 debuffStacks;
    bool hasEscorts;
    uint8 escortCount;

    FlagCarrierStatus()
        : carrier(), healthPercent(100.0f), isUnderAttack(false),
          attackerCount(0), distanceToCapture(0), debuffStacks(0),
          hasEscorts(false), escortCount(0) {}
};

/**
 * @struct EscortAssignment
 * @brief Assignment of an escort to flag carrier
 */
struct EscortAssignment
{
    ObjectGuid escort;
    ObjectGuid flagCarrier;
    uint32 assignTime;
    float distanceToFC;

    EscortAssignment()
        : escort(), flagCarrier(), assignTime(0), distanceToFC(0) {}
};

/**
 * @class FlagCarrierManager
 * @brief Manages flag carrier behavior in CTF battlegrounds
 *
 * Handles:
 * - Flag carrier selection
 * - Escort coordination
 * - Capture timing
 * - Flag defense and return
 * - EFC hunting
 */
class FlagCarrierManager
{
public:
    FlagCarrierManager(BattlegroundCoordinator* coordinator);

    void Initialize();
    void Update(uint32 diff);
    void Reset();

    // ========================================================================
    // FLAG STATE
    // ========================================================================

    FlagState GetFriendlyFlagState() const;
    FlagState GetEnemyFlagState() const;
    const FlagInfo& GetFriendlyFlag() const { return _friendlyFlag; }
    const FlagInfo& GetEnemyFlag() const { return _enemyFlag; }

    // ========================================================================
    // FLAG CARRIER
    // ========================================================================

    ObjectGuid GetFriendlyFC() const { return _friendlyFlag.carrierGuid; }
    ObjectGuid GetEnemyFC() const { return _enemyFlag.carrierGuid; }
    bool HasFriendlyFC() const { return !_friendlyFlag.carrierGuid.IsEmpty(); }
    bool HasEnemyFC() const { return !_enemyFlag.carrierGuid.IsEmpty(); }
    FlagCarrierStatus GetFCStatus() const;
    bool IsFCInDanger() const;
    bool IsFCNearCapture() const;

    // ========================================================================
    // FLAG EVENTS
    // ========================================================================

    void OnFlagPickedUp(ObjectGuid player, bool isEnemyFlag);
    void OnFlagDropped(ObjectGuid player, float x, float y, float z);
    void OnFlagCaptured(ObjectGuid player);
    void OnFlagReturned(ObjectGuid player);
    void OnFlagReset(bool isEnemyFlag);

    // ========================================================================
    // FLAG CARRIER SELECTION
    // ========================================================================

    ObjectGuid GetBestFCCandidate() const;
    float GetFCSuitabilityScore(ObjectGuid player) const;
    bool ShouldPickUpFlag(ObjectGuid player) const;

    // ========================================================================
    // ESCORT MANAGEMENT
    // ========================================================================

    void AssignEscort(ObjectGuid escort);
    void UnassignEscort(ObjectGuid escort);
    ::std::vector<ObjectGuid> GetEscorts() const;
    uint32 GetEscortCount() const;
    bool NeedsMoreEscorts() const;
    uint32 GetIdealEscortCount() const;
    ObjectGuid GetBestEscortCandidate() const;

    // ========================================================================
    // CAPTURE TIMING
    // ========================================================================

    bool CanCapture() const;
    bool ShouldCapture() const;
    bool ShouldWaitForFriendlyFlag() const;
    uint32 GetEstimatedCaptureTime() const;

    // ========================================================================
    // EFC HUNTING
    // ========================================================================

    void AssignHunter(ObjectGuid hunter);
    void UnassignHunter(ObjectGuid hunter);
    ::std::vector<ObjectGuid> GetHunters() const;
    uint32 GetHunterCount() const;
    bool NeedsMoreHunters() const;
    ObjectGuid GetBestHunterCandidate() const;
    float GetEFCThreatLevel() const;

    // ========================================================================
    // FLAG DEFENSE
    // ========================================================================

    void AssignDefender(ObjectGuid defender);
    void UnassignDefender(ObjectGuid defender);
    ::std::vector<ObjectGuid> GetDefenders() const;
    bool IsFlagUndefended() const;
    bool ShouldReturnToDefense(ObjectGuid player) const;

    // ========================================================================
    // DROPPED FLAG
    // ========================================================================

    bool IsFriendlyFlagDropped() const;
    bool IsEnemyFlagDropped() const;
    void GetDroppedFlagPosition(bool isEnemy, float& x, float& y, float& z) const;
    uint32 GetDroppedFlagTimer(bool isEnemy) const;
    bool ShouldPickUpDroppedFlag(ObjectGuid player) const;
    ObjectGuid GetClosestToDroppedFlag(bool isEnemy) const;

    // ========================================================================
    // DEBUFF TRACKING
    // ========================================================================

    uint8 GetFCDebuffStacks() const;
    float GetFCDamageTakenMultiplier() const;
    float GetFCHealingReceivedMultiplier() const;
    bool IsFCDebuffCritical() const;

private:
    BattlegroundCoordinator* _coordinator;

    // ========================================================================
    // FLAG STATE
    // ========================================================================

    FlagInfo _friendlyFlag;
    FlagInfo _enemyFlag;

    // ========================================================================
    // ASSIGNMENTS
    // ========================================================================

    ::std::vector<EscortAssignment> _escorts;
    ::std::vector<ObjectGuid> _hunters;
    ::std::vector<ObjectGuid> _defenders;

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    uint32 _idealEscortCount = 2;
    uint32 _maxEscortCount = 4;
    uint32 _idealHunterCount = 2;
    uint32 _minDefenderCount = 1;
    uint8 _criticalDebuffStacks = 7;
    float _captureRange = 30.0f;

    // ========================================================================
    // FC SUITABILITY SCORING
    // ========================================================================

    float ScoreFCHealth(ObjectGuid player) const;
    float ScoreFCClass(ObjectGuid player) const;
    float ScoreFCMobility(ObjectGuid player) const;
    float ScoreFCSurvivability(ObjectGuid player) const;

    // ========================================================================
    // POSITION TRACKING
    // ========================================================================

    float GetDistanceToEnemyBase() const;
    float GetDistanceToFriendlyBase() const;
    float GetDistanceToFC(ObjectGuid player) const;
    float GetDistanceToEFC(ObjectGuid player) const;

    // ========================================================================
    // UTILITY
    // ========================================================================

    void UpdateEscortDistances();
    void CleanupInvalidAssignments();
    bool IsValidFC(ObjectGuid player) const;
    bool IsValidEscort(ObjectGuid player) const;
    bool IsValidHunter(ObjectGuid player) const;
};

} // namespace Playerbot
