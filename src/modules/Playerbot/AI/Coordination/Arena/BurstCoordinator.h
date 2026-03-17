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

namespace Playerbot {

class ArenaCoordinator;

/**
 * @struct BurstOpportunity
 * @brief Represents a potential opportunity to initiate a burst window
 */
struct BurstOpportunity
{
    ObjectGuid target;
    float score;
    uint32 expectedDuration;
    bool trinketDown;
    bool defensivesDown;
    bool inCC;
    uint32 ccRemainingMs;
    ::std::vector<ObjectGuid> readyBursters;
    ::std::string reason;

    BurstOpportunity()
        : target(), score(0), expectedDuration(0),
          trinketDown(false), defensivesDown(false),
          inCC(false), ccRemainingMs(0) {}
};

/**
 * @struct BurstCooldown
 * @brief Tracks a player's burst cooldown availability
 */
struct BurstCooldown
{
    ObjectGuid player;
    uint32 spellId;
    uint32 readyTime;
    uint32 duration;
    float damageMultiplier;
    bool isActive;

    BurstCooldown()
        : player(), spellId(0), readyTime(0),
          duration(0), damageMultiplier(1.0f), isActive(false) {}
};

/**
 * @class BurstCoordinator
 * @brief Coordinates burst windows across the team in arena
 *
 * Manages the timing and execution of burst windows including:
 * - Evaluating burst opportunities
 * - Coordinating cooldown usage across team
 * - Tracking burst success/failure
 * - Managing burst phases (preparing, executing, sustaining, retreating)
 */
class BurstCoordinator
{
public:
    BurstCoordinator(ArenaCoordinator* coordinator);

    void Initialize();
    void Update(uint32 diff);
    void Reset();

    // ========================================================================
    // BURST WINDOW MANAGEMENT
    // ========================================================================

    bool StartBurst(ObjectGuid target);
    void EndBurst();
    bool IsBurstActive() const { return _currentBurst.isActive; }
    const BurstWindow& GetCurrentBurst() const { return _currentBurst; }
    BurstPhase GetPhase() const { return _currentBurst.phase; }

    // ========================================================================
    // BURST OPPORTUNITY EVALUATION
    // ========================================================================

    ::std::vector<BurstOpportunity> EvaluateOpportunities() const;
    BurstOpportunity EvaluateTarget(ObjectGuid target) const;
    float CalculateBurstScore(const ArenaEnemy& enemy) const;
    bool ShouldInitiateBurst() const;
    ObjectGuid GetBestBurstTarget() const;

    // ========================================================================
    // COOLDOWN TRACKING
    // ========================================================================

    bool IsCooldownReady(ObjectGuid player, uint32 spellId) const;
    bool HasBurstReady(ObjectGuid player) const;
    uint32 GetReadyBursterCount() const;
    ::std::vector<ObjectGuid> GetReadyBursters() const;
    void OnCooldownUsed(ObjectGuid player, uint32 spellId);
    void OnCooldownReady(ObjectGuid player, uint32 spellId);

    // ========================================================================
    // BURST PARTICIPATION
    // ========================================================================

    bool ShouldUseCooldowns(ObjectGuid player) const;
    bool IsParticipatingInBurst(ObjectGuid player) const;
    void OnPlayerJoinedBurst(ObjectGuid player);
    void OnPlayerLeftBurst(ObjectGuid player);
    uint32 GetBurstDuration() const;

    // ========================================================================
    // BURST SUCCESS TRACKING
    // ========================================================================

    float GetBurstProgress() const;  // 0.0-1.0 based on target health
    bool IsBurstSuccessful() const;
    bool IsBurstFailing() const;
    void OnTargetUsedDefensive(ObjectGuid target);
    void OnTargetUsedTrinket(ObjectGuid target);
    void OnTargetDied(ObjectGuid target);

    // ========================================================================
    // BURST HISTORY
    // ========================================================================

    uint32 GetBurstWindowCount() const { return _burstWindowCount; }
    uint32 GetSuccessfulBurstCount() const { return _successfulBurstCount; }
    float GetBurstSuccessRate() const;
    uint32 GetLastBurstTime() const { return _lastBurstEndTime; }

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    void SetMinBursters(uint8 count) { _minBurstersRequired = count; }
    void SetBurstDurationThreshold(uint32 ms) { _burstDurationThreshold = ms; }
    void SetHealthTargetThreshold(float percent) { _healthTargetThreshold = percent; }

private:
    ArenaCoordinator* _coordinator;

    // ========================================================================
    // CURRENT BURST
    // ========================================================================

    BurstWindow _currentBurst;
    uint32 _phaseStartTime = 0;

    // ========================================================================
    // COOLDOWN TRACKING
    // ========================================================================

    ::std::map<ObjectGuid, ::std::vector<BurstCooldown>> _playerCooldowns;

    // ========================================================================
    // HISTORY
    // ========================================================================

    uint32 _burstWindowCount = 0;
    uint32 _successfulBurstCount = 0;
    uint32 _lastBurstEndTime = 0;

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    uint8 _minBurstersRequired = 2;
    uint32 _burstDurationThreshold = 10000;  // Max 10s burst window
    float _healthTargetThreshold = 30.0f;    // Success = target below 30%
    uint32 _phaseDurations[5] = { 0, 2000, 6000, 4000, 2000 };  // Phase durations

    // ========================================================================
    // SCORING WEIGHTS
    // ========================================================================

    float _weightTrinketDown = 2.0f;
    float _weightDefensivesDown = 1.5f;
    float _weightInCC = 1.5f;
    float _weightLowHealth = 1.0f;
    float _weightHealer = 1.2f;

    // ========================================================================
    // PHASE MANAGEMENT
    // ========================================================================

    void UpdatePhase(uint32 diff);
    void TransitionToPhase(BurstPhase newPhase);
    bool ShouldAdvancePhase() const;
    bool ShouldAbortBurst() const;

    // ========================================================================
    // COOLDOWN MANAGEMENT
    // ========================================================================

    void InitializePlayerCooldowns(ObjectGuid player);
    void UpdateCooldownTimers(uint32 diff);
    ::std::vector<uint32> GetBurstSpellsForClass(uint32 classId) const;
    float GetCooldownDamageMultiplier(uint32 spellId) const;
    uint32 GetCooldownDuration(uint32 spellId) const;

    // ========================================================================
    // SCORING
    // ========================================================================

    float ScoreTrinketStatus(const ArenaEnemy& enemy) const;
    float ScoreDefensiveStatus(const ArenaEnemy& enemy) const;
    float ScoreCCStatus(const ArenaEnemy& enemy) const;
    float ScoreHealthStatus(const ArenaEnemy& enemy) const;
    float ScoreRole(const ArenaEnemy& enemy) const;
    float ScoreTeamReadiness() const;
};

} // namespace Playerbot
