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
#include <queue>
#include <vector>
#include <map>

namespace Playerbot {

class ArenaCoordinator;
class CrowdControlManager;  // From Phase 2

/**
 * @enum CCCategory
 * @brief Categories of CC for DR tracking
 */
enum class CCCategory : uint8
{
    STUN = 0,
    INCAPACITATE = 1,
    DISORIENT = 2,
    SILENCE = 3,
    FEAR = 4,
    ROOT = 5,
    SLOW = 6,
    KNOCKBACK = 7
};

/**
 * @struct CCLink
 * @brief Represents a single CC in a chain
 */
struct CCLink
{
    ObjectGuid caster;
    uint32 spellId;
    CCCategory category;
    uint32 baseDuration;
    uint32 expectedDuration;
    uint8 drStack;          // DR stack when applied (0, 1, 2)
    uint32 scheduledTime;   // When to cast
    bool executed;
    bool successful;

    CCLink()
        : caster(), spellId(0), category(CCCategory::STUN),
          baseDuration(0), expectedDuration(0), drStack(0),
          scheduledTime(0), executed(false), successful(false) {}
};

/**
 * @struct CCChain
 * @brief Represents a planned chain of CC abilities
 */
struct CCChain
{
    ObjectGuid target;
    ::std::vector<CCLink> links;
    uint32 totalDuration;
    uint32 startTime;
    uint32 expectedEndTime;
    bool isActive;
    uint8 currentLinkIndex;

    // Chain purpose
    bool isForBurst;
    bool isForPeel;
    bool isForKill;

    CCChain()
        : target(), totalDuration(0), startTime(0), expectedEndTime(0),
          isActive(false), currentLinkIndex(0),
          isForBurst(false), isForPeel(false), isForKill(false) {}

    void Reset()
    {
        target = ObjectGuid::Empty;
        links.clear();
        totalDuration = 0;
        startTime = 0;
        expectedEndTime = 0;
        isActive = false;
        currentLinkIndex = 0;
        isForBurst = false;
        isForPeel = false;
        isForKill = false;
    }
};

/**
 * @struct PlayerCCAbility
 * @brief Represents a player's CC ability
 */
struct PlayerCCAbility
{
    uint32 spellId;
    CCCategory category;
    uint32 baseDuration;
    uint32 cooldown;
    uint32 readyTime;
    float range;
    bool requiresLOS;
    bool isMelee;

    PlayerCCAbility()
        : spellId(0), category(CCCategory::STUN), baseDuration(0),
          cooldown(0), readyTime(0), range(30.0f),
          requiresLOS(true), isMelee(false) {}
};

/**
 * @class CCChainManager
 * @brief Manages CC chains with DR-aware planning
 *
 * Coordinates crowd control abilities across the team, planning
 * chains that account for diminishing returns. Uses the Phase 2
 * CrowdControlManager for DR tracking.
 */
class CCChainManager
{
public:
    CCChainManager(ArenaCoordinator* coordinator, CrowdControlManager* ccManager);

    void Initialize();
    void Update(uint32 diff);
    void Reset();

    // ========================================================================
    // CHAIN MANAGEMENT
    // ========================================================================

    bool StartChain(ObjectGuid target, bool forBurst = false, bool forPeel = false);
    void EndChain();
    bool IsChainActive() const { return _activeChain.isActive; }
    const CCChain& GetActiveChain() const { return _activeChain; }
    ObjectGuid GetChainTarget() const { return _activeChain.target; }

    // ========================================================================
    // CHAIN PLANNING
    // ========================================================================

    CCChain PlanChain(ObjectGuid target, uint32 desiredDuration) const;
    CCChain PlanOptimalChain(ObjectGuid target) const;
    bool CanChainTarget(ObjectGuid target) const;
    uint32 GetMaxChainDuration(ObjectGuid target) const;

    // ========================================================================
    // DR-AWARE CC
    // ========================================================================

    uint32 GetExpectedDuration(ObjectGuid target, uint32 spellId) const;
    uint8 GetDRStacks(ObjectGuid target, CCCategory category) const;
    bool IsImmune(ObjectGuid target, CCCategory category) const;
    bool WillDRExpireSoon(ObjectGuid target, CCCategory category) const;
    uint32 GetTimeUntilDRReset(ObjectGuid target, CCCategory category) const;

    // ========================================================================
    // CHAIN EXECUTION
    // ========================================================================

    ObjectGuid GetNextCCer() const;
    uint32 GetNextCCSpell() const;
    uint32 GetTimeUntilNextCC() const;
    void OnCCApplied(ObjectGuid caster, ObjectGuid target, uint32 spellId);
    void OnCCBroken(ObjectGuid target);
    void OnCCExpired(ObjectGuid target);
    void OnCCResisted(ObjectGuid caster, ObjectGuid target, uint32 spellId);

    // ========================================================================
    // CC REQUESTS
    // ========================================================================

    void RequestCC(ObjectGuid requester, ObjectGuid target, uint32 minDuration, uint8 priority);
    ::std::vector<CCRequest> GetPendingRequests() const { return _pendingRequests; }
    void FulfillRequest(const CCRequest& request);
    void CancelRequest(ObjectGuid target);

    // ========================================================================
    // PLAYER CC AVAILABILITY
    // ========================================================================

    bool HasCCAvailable(ObjectGuid player) const;
    ::std::vector<PlayerCCAbility> GetAvailableCCSpells(ObjectGuid player) const;
    bool CanCast(ObjectGuid player, uint32 spellId, ObjectGuid target) const;
    uint32 GetCooldownRemaining(ObjectGuid player, uint32 spellId) const;
    void OnCooldownUsed(ObjectGuid player, uint32 spellId);

    // ========================================================================
    // CC OVERLAP PREVENTION
    // ========================================================================

    bool ShouldOverlapCC() const;
    float GetOverlapWindow() const { return _overlapWindow; }
    void SetOverlapWindow(float ms) { _overlapWindow = ms; }

    // ========================================================================
    // STATISTICS
    // ========================================================================

    uint32 GetChainsStarted() const { return _chainsStarted; }
    uint32 GetChainsCompleted() const { return _chainsCompleted; }
    uint32 GetCCApplied() const { return _ccApplied; }
    uint32 GetCCBroken() const { return _ccBroken; }
    float GetChainSuccessRate() const;

private:
    ArenaCoordinator* _coordinator;
    CrowdControlManager* _ccManager;  // For DR tracking from Phase 2

    // ========================================================================
    // ACTIVE CHAIN
    // ========================================================================

    CCChain _activeChain;
    uint32 _chainUpdateTimer = 0;

    // ========================================================================
    // PENDING REQUESTS
    // ========================================================================

    ::std::vector<CCRequest> _pendingRequests;

    // ========================================================================
    // PLAYER CC ABILITIES
    // ========================================================================

    ::std::map<ObjectGuid, ::std::vector<PlayerCCAbility>> _playerCCAbilities;

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    float _overlapWindow = 300.0f;  // 300ms overlap to prevent gaps
    uint32 _maxChainDuration = 20000;  // Max 20s chain
    uint32 _minCCDuration = 500;  // Don't use CC for <500ms

    // ========================================================================
    // STATISTICS
    // ========================================================================

    uint32 _chainsStarted = 0;
    uint32 _chainsCompleted = 0;
    uint32 _ccApplied = 0;
    uint32 _ccBroken = 0;

    // ========================================================================
    // CHAIN PLANNING
    // ========================================================================

    void LoadPlayerCCAbilities(ObjectGuid player);
    void UpdateCooldowns(uint32 diff);
    CCLink SelectBestNextCC(ObjectGuid target, uint8 maxDRStack) const;
    ObjectGuid FindBestCCer(ObjectGuid target, CCCategory category, uint8 maxDR) const;

    // ========================================================================
    // CHAIN EXECUTION
    // ========================================================================

    void UpdateActiveChain(uint32 diff);
    void AdvanceToNextLink();
    void OnLinkExecuted(CCLink& link, bool success);
    bool IsChainBroken() const;

    // ========================================================================
    // DR CALCULATIONS
    // ========================================================================

    uint32 CalculateDRDuration(uint32 baseDuration, uint8 drStack) const;
    CCCategory GetSpellCategory(uint32 spellId) const;

    // ========================================================================
    // UTILITY
    // ========================================================================

    bool IsInRange(ObjectGuid caster, ObjectGuid target, float range) const;
    bool HasLOS(ObjectGuid caster, ObjectGuid target) const;
    float GetDistance(ObjectGuid a, ObjectGuid b) const;
};

} // namespace Playerbot
