/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "BGRoleManager.h"
#include "BattlegroundCoordinator.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "GameTime.h"
#include "Log.h"
#include <algorithm>

namespace Playerbot {

BGRoleManager::BGRoleManager(BattlegroundCoordinator* coordinator)
    : _coordinator(coordinator)
{
}

void BGRoleManager::Initialize()
{
    Reset();
    TC_LOG_DEBUG("playerbot", "BGRoleManager::Initialize - Initialized");
}

void BGRoleManager::Update(uint32 diff)
{
    (void)diff;
    UpdateRoleCounts();
}

void BGRoleManager::Reset()
{
    _assignments.clear();
    _requirements.clear();
    _lastReassignmentTime = 0;
}

// ============================================================================
// ROLE ASSIGNMENT
// ============================================================================

void BGRoleManager::AssignRole(ObjectGuid player, BGRole role)
{
    BGRoleAssignment assignment;
    assignment.player = player;
    assignment.role = role;
    assignment.assignTime = GameTime::GetGameTimeMS();
    assignment.efficiency = 0.5f;

    _assignments[player] = assignment;

    TC_LOG_DEBUG("playerbot", "BGRoleManager: Assigned {} to player",
                 BGRoleToString(role));

    UpdateRoleCounts();
}

void BGRoleManager::ClearRole(ObjectGuid player)
{
    _assignments.erase(player);
    UpdateRoleCounts();
}

BGRole BGRoleManager::GetRole(ObjectGuid player) const
{
    auto it = _assignments.find(player);
    return it != _assignments.end() ? it->second.role : BGRole::UNASSIGNED;
}

bool BGRoleManager::HasRole(ObjectGuid player) const
{
    auto it = _assignments.find(player);
    return it != _assignments.end() && it->second.role != BGRole::UNASSIGNED;
}

// ============================================================================
// ROLE QUERIES
// ============================================================================

::std::vector<ObjectGuid> BGRoleManager::GetPlayersWithRole(BGRole role) const
{
    ::std::vector<ObjectGuid> result;
    for (const auto& [player, assignment] : _assignments)
    {
        if (assignment.role == role)
            result.push_back(player);
    }
    return result;
}

uint32 BGRoleManager::GetRoleCount(BGRole role) const
{
    auto it = _requirements.find(role);
    return it != _requirements.end() ? it->second.currentCount : 0;
}

bool BGRoleManager::IsRoleFilled(BGRole role) const
{
    auto it = _requirements.find(role);
    if (it == _requirements.end())
        return true;

    return it->second.currentCount >= it->second.minCount;
}

bool BGRoleManager::IsRoleNeeded(BGRole role) const
{
    auto it = _requirements.find(role);
    if (it == _requirements.end())
        return false;

    return it->second.currentCount < it->second.idealCount;
}

// ============================================================================
// ROLE SUITABILITY
// ============================================================================

::std::vector<RoleSuitability> BGRoleManager::EvaluateAllSuitabilities(ObjectGuid player) const
{
    ::std::vector<RoleSuitability> result;

    for (const auto& [role, req] : _requirements)
    {
        result.push_back(EvaluateSuitability(player, role));
    }

    // Sort by score (highest first)
    ::std::sort(result.begin(), result.end(),
        [](const RoleSuitability& a, const RoleSuitability& b)
        {
            return a.score > b.score;
        });

    return result;
}

RoleSuitability BGRoleManager::EvaluateSuitability(ObjectGuid player, BGRole role) const
{
    RoleSuitability suit;
    suit.player = player;
    suit.role = role;
    suit.score = GetRoleSuitabilityScore(player, role);

    return suit;
}

float BGRoleManager::GetRoleSuitabilityScore(ObjectGuid player, BGRole role) const
{
    switch (role)
    {
        case BGRole::FLAG_CARRIER:
            return ScoreFlagCarrierSuitability(player);
        case BGRole::FLAG_ESCORT:
            return ScoreFlagEscortSuitability(player);
        case BGRole::FLAG_HUNTER:
            return ScoreFlagHunterSuitability(player);
        case BGRole::NODE_ATTACKER:
            return ScoreNodeAttackerSuitability(player);
        case BGRole::NODE_DEFENDER:
            return ScoreNodeDefenderSuitability(player);
        case BGRole::ROAMER:
            return ScoreRoamerSuitability(player);
        case BGRole::HEALER_OFFENSE:
            return ScoreHealerOffenseSuitability(player);
        case BGRole::HEALER_DEFENSE:
            return ScoreHealerDefenseSuitability(player);
        default:
            return 0.5f;
    }
}

BGRole BGRoleManager::GetBestRole(ObjectGuid player) const
{
    auto suitabilities = EvaluateAllSuitabilities(player);
    if (suitabilities.empty())
        return BGRole::ROAMER;

    return suitabilities[0].role;
}

ObjectGuid BGRoleManager::GetBestPlayerForRole(BGRole role) const
{
    ObjectGuid best;
    float bestScore = 0.0f;

    for (const auto& bot : _coordinator->GetAliveBots())
    {
        if (HasRole(bot.guid))
        {
            BGRole currentRole = GetRole(bot.guid);
            // Skip players already assigned to the target role (prevents infinite loop)
            if (currentRole == role)
                continue;
            // Skip players in high-priority roles that shouldn't be reassigned
            if (currentRole == BGRole::FLAG_CARRIER)
                continue;
        }

        float score = GetRoleSuitabilityScore(bot.guid, role);
        if (score > bestScore)
        {
            bestScore = score;
            best = bot.guid;
        }
    }

    return best;
}

// ============================================================================
// AUTOMATIC ASSIGNMENT
// ============================================================================

void BGRoleManager::AssignAllRoles()
{
    // Clear existing assignments
    _assignments.clear();

    auto aliveBots = _coordinator->GetAliveBots();
    auto allBots = _coordinator->GetAllBots();

    TC_LOG_INFO("playerbot.bg", "BGRoleManager::AssignAllRoles - Total bots: {}, Alive bots: {}, Requirements: {}",
        allBots.size(), aliveBots.size(), _requirements.size());

    // Use all bots if no alive bots (they might not be flagged as alive yet during init)
    auto& bots = aliveBots.empty() ? allBots : aliveBots;

    if (bots.empty())
    {
        TC_LOG_WARN("playerbot.bg", "BGRoleManager::AssignAllRoles - No bots to assign roles to!");
        return;
    }

    // First pass: Assign healers
    uint32 healersAssigned = 0;
    for (const auto& bot : bots)
    {
        if (IsHealer(bot.guid))
        {
            AssignRole(bot.guid, BGRole::HEALER_DEFENSE);
            healersAssigned++;
        }
    }
    TC_LOG_DEBUG("playerbot.bg", "BGRoleManager: Pass 1 - Assigned {} healers", healersAssigned);

    // Second pass: Fill needed roles by suitability
    // Safety limit: no role should need more iterations than total bot count
    uint32 maxIterations = static_cast<uint32>(bots.size()) + 1;
    for (const auto& [role, req] : _requirements)
    {
        uint32 assigned = 0;
        uint32 iterations = 0;
        while (GetRoleCount(role) < req.idealCount && iterations < maxIterations)
        {
            ++iterations;
            ObjectGuid best = GetBestPlayerForRole(role);
            if (best.IsEmpty())
            {
                TC_LOG_DEBUG("playerbot.bg", "BGRoleManager: No suitable player for role {}", BGRoleToString(role));
                break;
            }

            AssignRole(best, role);
            assigned++;
        }
        if (iterations >= maxIterations)
        {
            TC_LOG_WARN("playerbot.bg",
                "BGRoleManager: Hit iteration limit for role {} (assigned {}/{})",
                BGRoleToString(role), GetRoleCount(role), req.idealCount);
        }
        TC_LOG_DEBUG("playerbot.bg", "BGRoleManager: Pass 2 - Assigned {} players to role {} (need {})",
            assigned, BGRoleToString(role), req.idealCount);
    }

    // Final pass: Assign remaining to roamer
    uint32 roamersAssigned = 0;
    for (const auto& bot : bots)
    {
        if (!HasRole(bot.guid))
        {
            AssignRole(bot.guid, BGRole::ROAMER);
            roamersAssigned++;
        }
    }
    TC_LOG_DEBUG("playerbot.bg", "BGRoleManager: Pass 3 - Assigned {} roamers", roamersAssigned);

    TC_LOG_INFO("playerbot.bg", "BGRoleManager: Assigned roles to {} players (healers: {}, roamers: {})",
                 _assignments.size(), healersAssigned, roamersAssigned);
}

void BGRoleManager::RebalanceRoles()
{
    uint32 now = GameTime::GetGameTimeMS();

    // Cooldown check
    if (now - _lastReassignmentTime < _reassignmentCooldown)
        return;

    _lastReassignmentTime = now;

    // Fill missing roles
    FillMissingRoles();

    // Check for role swaps that would improve efficiency
    if (ShouldSwapRoles())
    {
        // Would implement swap logic
    }
}

void BGRoleManager::FillMissingRoles()
{
    auto neededRoles = GetNeededRoles();
    auto overfilledRoles = GetOverfilledRoles();

    for (BGRole neededRole : neededRoles)
    {
        // Try to reassign from overfilled roles first
        for (BGRole overfilledRole : overfilledRoles)
        {
            auto players = GetPlayersWithRole(overfilledRole);
            if (players.empty())
                continue;

            // Find best player to reassign
            ObjectGuid best;
            float bestScore = 0.0f;

            for (const ObjectGuid& player : players)
            {
                float score = GetRoleSuitabilityScore(player, neededRole);
                if (score > bestScore)
                {
                    bestScore = score;
                    best = player;
                }
            }

            if (!best.IsEmpty())
            {
                AssignRole(best, neededRole);
                break;
            }
        }
    }
}

// ============================================================================
// ROLE REQUIREMENTS
// ============================================================================

void BGRoleManager::SetRoleRequirement(BGRole role, uint8 min, uint8 max, uint8 ideal)
{
    RoleRequirement req;
    req.role = role;
    req.minCount = min;
    req.maxCount = max;
    req.idealCount = ideal;
    req.currentCount = 0;

    _requirements[role] = req;
}

RoleRequirement BGRoleManager::GetRoleRequirement(BGRole role) const
{
    auto it = _requirements.find(role);
    return it != _requirements.end() ? it->second : RoleRequirement();
}

::std::vector<RoleRequirement> BGRoleManager::GetAllRequirements() const
{
    ::std::vector<RoleRequirement> result;
    for (const auto& [role, req] : _requirements)
    {
        result.push_back(req);
    }
    return result;
}

bool BGRoleManager::AreRequirementsMet() const
{
    for (const auto& [role, req] : _requirements)
    {
        if (req.currentCount < req.minCount)
            return false;
    }
    return true;
}

// ============================================================================
// ROLE EFFICIENCY
// ============================================================================

void BGRoleManager::UpdateRoleEfficiency(ObjectGuid player, float efficiency)
{
    auto it = _assignments.find(player);
    if (it != _assignments.end())
    {
        it->second.efficiency = efficiency;
    }
}

float BGRoleManager::GetRoleEfficiency(ObjectGuid player) const
{
    auto it = _assignments.find(player);
    return it != _assignments.end() ? it->second.efficiency : 0.5f;
}

float BGRoleManager::GetAverageRoleEfficiency(BGRole role) const
{
    float total = 0.0f;
    uint32 count = 0;

    for (const auto& [player, assignment] : _assignments)
    {
        if (assignment.role == role)
        {
            total += assignment.efficiency;
            count++;
        }
    }

    return count > 0 ? total / count : 0.5f;
}

// ============================================================================
// ROLE SWAPPING
// ============================================================================

bool BGRoleManager::CanSwapRoles(ObjectGuid player1, ObjectGuid player2) const
{
    auto it1 = _assignments.find(player1);
    auto it2 = _assignments.find(player2);

    if (it1 == _assignments.end() || it2 == _assignments.end())
        return false;

    // Can't swap with self
    if (player1 == player2)
        return false;

    return true;
}

void BGRoleManager::SwapRoles(ObjectGuid player1, ObjectGuid player2)
{
    if (!CanSwapRoles(player1, player2))
        return;

    BGRole role1 = GetRole(player1);
    BGRole role2 = GetRole(player2);

    AssignRole(player1, role2);
    AssignRole(player2, role1);
}

bool BGRoleManager::ShouldSwapRoles() const
{
    // Check if any swaps would improve overall efficiency
    return false;  // Simplified
}

// ============================================================================
// SUITABILITY SCORING
// ============================================================================

float BGRoleManager::ScoreFlagCarrierSuitability(ObjectGuid player) const
{
    float score = 0.0f;

    score += ScoreFCHealth(player);
    score += ScoreFCClass(player);
    score += ScoreFCMobility(player);
    score += ScoreFCSurvivability(player);

    // Class bonuses
    Player* p = ObjectAccessor::FindPlayer(player);
    if (p)
    {
        score += GetClassRoleBonus(p->GetClass(), BGRole::FLAG_CARRIER);
    }

    return score;
}

float BGRoleManager::ScoreFlagEscortSuitability(ObjectGuid player) const
{
    float score = 0.5f;

    // Healers are good escorts
    if (IsHealer(player))
        score += 0.3f;

    // Melee DPS with CC are good
    if (IsMeleeDPS(player))
        score += 0.2f;

    return score;
}

float BGRoleManager::ScoreFlagHunterSuitability(ObjectGuid player) const
{
    float score = 0.5f;

    // High mobility classes
    Player* p = ObjectAccessor::FindPlayer(player);
    if (p)
    {
        uint8 classId = p->GetClass();
        if (classId == CLASS_ROGUE || classId == CLASS_DRUID ||
            classId == CLASS_DEMON_HUNTER)
        {
            score += 0.3f;
        }
    }

    return score;
}

float BGRoleManager::ScoreNodeAttackerSuitability(ObjectGuid player) const
{
    float score = 0.5f;

    // DPS classes are better attackers
    if (IsMeleeDPS(player) || IsRangedDPS(player))
        score += 0.3f;

    return score;
}

float BGRoleManager::ScoreNodeDefenderSuitability(ObjectGuid player) const
{
    float score = 0.5f;

    // Tanks are good defenders
    if (IsTank(player))
        score += 0.2f;

    // Classes with CC are good
    Player* p = ObjectAccessor::FindPlayer(player);
    if (p)
    {
        uint8 classId = p->GetClass();
        if (classId == CLASS_MAGE || classId == CLASS_WARLOCK ||
            classId == CLASS_PRIEST)
        {
            score += 0.2f;
        }
    }

    return score;
}

float BGRoleManager::ScoreRoamerSuitability(ObjectGuid player) const
{
    float score = 0.5f;

    // Mobile classes
    Player* p = ObjectAccessor::FindPlayer(player);
    if (p)
    {
        uint8 classId = p->GetClass();
        if (classId == CLASS_ROGUE || classId == CLASS_DRUID ||
            classId == CLASS_MONK)
        {
            score += 0.2f;
        }
    }

    return score;
}

float BGRoleManager::ScoreHealerOffenseSuitability(ObjectGuid player) const
{
    if (!IsHealer(player))
        return 0.0f;

    return 0.7f;  // All healers can do offense
}

float BGRoleManager::ScoreHealerDefenseSuitability(ObjectGuid player) const
{
    if (!IsHealer(player))
        return 0.0f;

    return 0.8f;  // All healers can do defense
}

// ============================================================================
// CLASS/SPEC SCORING
// ============================================================================

float BGRoleManager::GetClassRoleBonus(uint32 classId, BGRole role) const
{
    if (role == BGRole::FLAG_CARRIER)
    {
        // Druids are best FCs
        if (classId == CLASS_DRUID) return 0.4f;
        // Monks are good FCs
        if (classId == CLASS_MONK) return 0.3f;
        // DH are fast
        if (classId == CLASS_DEMON_HUNTER) return 0.25f;
    }

    return 0.0f;
}

float BGRoleManager::GetSpecRoleBonus(uint32 /*specId*/, BGRole /*role*/) const
{
    // Would have spec-specific bonuses
    return 0.0f;
}

bool BGRoleManager::IsHealer(ObjectGuid player) const
{
    Player* p = ObjectAccessor::FindPlayer(player);
    if (!p)
        return false;

    // Simplified - would check spec
    uint8 classId = p->GetClass();
    return classId == CLASS_PRIEST || classId == CLASS_PALADIN ||
           classId == CLASS_SHAMAN || classId == CLASS_DRUID ||
           classId == CLASS_MONK || classId == CLASS_EVOKER;
}

bool BGRoleManager::IsTank(ObjectGuid player) const
{
    Player* p = ObjectAccessor::FindPlayer(player);
    if (!p)
        return false;

    // Simplified
    uint8 classId = p->GetClass();
    return classId == CLASS_WARRIOR || classId == CLASS_PALADIN ||
           classId == CLASS_DEATH_KNIGHT || classId == CLASS_DRUID ||
           classId == CLASS_MONK || classId == CLASS_DEMON_HUNTER;
}

bool BGRoleManager::IsMeleeDPS(ObjectGuid player) const
{
    Player* p = ObjectAccessor::FindPlayer(player);
    if (!p)
        return false;

    uint8 classId = p->GetClass();
    return classId == CLASS_WARRIOR || classId == CLASS_ROGUE ||
           classId == CLASS_DEATH_KNIGHT || classId == CLASS_PALADIN ||
           classId == CLASS_MONK || classId == CLASS_DEMON_HUNTER;
}

bool BGRoleManager::IsRangedDPS(ObjectGuid player) const
{
    Player* p = ObjectAccessor::FindPlayer(player);
    if (!p)
        return false;

    uint8 classId = p->GetClass();
    return classId == CLASS_MAGE || classId == CLASS_WARLOCK ||
           classId == CLASS_HUNTER || classId == CLASS_EVOKER;
}

// ============================================================================
// UTILITY
// ============================================================================

void BGRoleManager::UpdateRoleCounts()
{
    // Reset counts
    for (auto& [role, req] : _requirements)
    {
        req.currentCount = 0;
    }

    // Count current assignments
    for (const auto& [player, assignment] : _assignments)
    {
        auto it = _requirements.find(assignment.role);
        if (it != _requirements.end())
        {
            it->second.currentCount++;
        }
    }
}

::std::vector<BGRole> BGRoleManager::GetNeededRoles() const
{
    ::std::vector<BGRole> result;
    for (const auto& [role, req] : _requirements)
    {
        if (req.currentCount < req.idealCount)
            result.push_back(role);
    }
    return result;
}

::std::vector<BGRole> BGRoleManager::GetOverfilledRoles() const
{
    ::std::vector<BGRole> result;
    for (const auto& [role, req] : _requirements)
    {
        if (req.currentCount > req.maxCount)
            result.push_back(role);
    }
    return result;
}

float BGRoleManager::ScoreFCHealth(ObjectGuid player) const
{
    const BGPlayer* bot = _coordinator->GetBot(player);
    return bot ? bot->healthPercent / 100.0f * 0.3f : 0.0f;
}

float BGRoleManager::ScoreFCClass(ObjectGuid player) const
{
    Player* p = ObjectAccessor::FindPlayer(player);
    if (!p)
        return 0.0f;

    return GetClassRoleBonus(p->GetClass(), BGRole::FLAG_CARRIER);
}

float BGRoleManager::ScoreFCMobility(ObjectGuid player) const
{
    Player* p = ObjectAccessor::FindPlayer(player);
    if (!p)
        return 0.0f;

    uint8 classId = p->GetClass();
    if (classId == CLASS_DRUID || classId == CLASS_DEMON_HUNTER)
        return 0.3f;
    if (classId == CLASS_MONK || classId == CLASS_ROGUE)
        return 0.2f;

    return 0.1f;
}

float BGRoleManager::ScoreFCSurvivability(ObjectGuid player) const
{
    if (IsTank(player))
        return 0.2f;
    if (IsHealer(player))
        return 0.1f;
    return 0.0f;
}

} // namespace Playerbot
