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
 * @struct RoleSuitability
 * @brief Evaluates how suitable a player is for a role
 */
struct RoleSuitability
{
    ObjectGuid player;
    BGRole role;
    float score;
    ::std::string reason;

    RoleSuitability()
        : player(), role(BGRole::UNASSIGNED), score(0) {}
};

/**
 * @struct RoleRequirement
 * @brief Defines requirements for a battleground strategy
 */
struct RoleRequirement
{
    BGRole role;
    uint8 minCount;
    uint8 maxCount;
    uint8 idealCount;
    uint8 currentCount;

    RoleRequirement()
        : role(BGRole::UNASSIGNED),
          minCount(0), maxCount(0), idealCount(0), currentCount(0) {}
};

/**
 * @class BGRoleManager
 * @brief Manages role assignments for bots in battlegrounds
 *
 * Handles:
 * - Role suitability evaluation
 * - Dynamic role assignment
 * - Role balancing
 * - Role-specific behavior coordination
 */
class BGRoleManager
{
public:
    BGRoleManager(BattlegroundCoordinator* coordinator);

    void Initialize();
    void Update(uint32 diff);
    void Reset();

    // ========================================================================
    // ROLE ASSIGNMENT
    // ========================================================================

    void AssignRole(ObjectGuid player, BGRole role);
    void ClearRole(ObjectGuid player);
    BGRole GetRole(ObjectGuid player) const;
    bool HasRole(ObjectGuid player) const;

    // ========================================================================
    // ROLE QUERIES
    // ========================================================================

    ::std::vector<ObjectGuid> GetPlayersWithRole(BGRole role) const;
    uint32 GetRoleCount(BGRole role) const;
    bool IsRoleFilled(BGRole role) const;
    bool IsRoleNeeded(BGRole role) const;
    size_t GetAssignmentCount() const { return _assignments.size(); }

    // ========================================================================
    // ROLE SUITABILITY
    // ========================================================================

    ::std::vector<RoleSuitability> EvaluateAllSuitabilities(ObjectGuid player) const;
    RoleSuitability EvaluateSuitability(ObjectGuid player, BGRole role) const;
    float GetRoleSuitabilityScore(ObjectGuid player, BGRole role) const;
    BGRole GetBestRole(ObjectGuid player) const;
    ObjectGuid GetBestPlayerForRole(BGRole role) const;

    // ========================================================================
    // AUTOMATIC ASSIGNMENT
    // ========================================================================

    void AssignAllRoles();
    void RebalanceRoles();
    void FillMissingRoles();

    // ========================================================================
    // ROLE REQUIREMENTS
    // ========================================================================

    void SetRoleRequirement(BGRole role, uint8 min, uint8 max, uint8 ideal);
    RoleRequirement GetRoleRequirement(BGRole role) const;
    ::std::vector<RoleRequirement> GetAllRequirements() const;
    bool AreRequirementsMet() const;

    // ========================================================================
    // ROLE EFFICIENCY
    // ========================================================================

    void UpdateRoleEfficiency(ObjectGuid player, float efficiency);
    float GetRoleEfficiency(ObjectGuid player) const;
    float GetAverageRoleEfficiency(BGRole role) const;

    // ========================================================================
    // ROLE SWAPPING
    // ========================================================================

    bool CanSwapRoles(ObjectGuid player1, ObjectGuid player2) const;
    void SwapRoles(ObjectGuid player1, ObjectGuid player2);
    bool ShouldSwapRoles() const;

private:
    BattlegroundCoordinator* _coordinator;

    // ========================================================================
    // ASSIGNMENTS
    // ========================================================================

    ::std::map<ObjectGuid, BGRoleAssignment> _assignments;

    // ========================================================================
    // REQUIREMENTS
    // ========================================================================

    ::std::map<BGRole, RoleRequirement> _requirements;

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    uint32 _reassignmentCooldown = 30000;  // 30 seconds
    uint32 _lastReassignmentTime = 0;

    // ========================================================================
    // SUITABILITY SCORING
    // ========================================================================

    float ScoreFlagCarrierSuitability(ObjectGuid player) const;
    float ScoreFlagEscortSuitability(ObjectGuid player) const;
    float ScoreFlagHunterSuitability(ObjectGuid player) const;
    float ScoreNodeAttackerSuitability(ObjectGuid player) const;
    float ScoreNodeDefenderSuitability(ObjectGuid player) const;
    float ScoreRoamerSuitability(ObjectGuid player) const;
    float ScoreHealerOffenseSuitability(ObjectGuid player) const;
    float ScoreHealerDefenseSuitability(ObjectGuid player) const;

    // ========================================================================
    // CLASS/SPEC SCORING
    // ========================================================================

    float GetClassRoleBonus(uint32 classId, BGRole role) const;
    float GetSpecRoleBonus(uint32 specId, BGRole role) const;
    bool IsHealer(ObjectGuid player) const;
    bool IsTank(ObjectGuid player) const;
    bool IsMeleeDPS(ObjectGuid player) const;
    bool IsRangedDPS(ObjectGuid player) const;

    // ========================================================================
    // FC SCORING HELPERS
    // ========================================================================

    float ScoreFCHealth(ObjectGuid player) const;
    float ScoreFCClass(ObjectGuid player) const;
    float ScoreFCMobility(ObjectGuid player) const;
    float ScoreFCSurvivability(ObjectGuid player) const;

    // ========================================================================
    // UTILITY
    // ========================================================================

    void UpdateRoleCounts();
    ::std::vector<BGRole> GetNeededRoles() const;
    ::std::vector<BGRole> GetOverfilledRoles() const;
};

} // namespace Playerbot
