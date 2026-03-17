/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "MythicPlusManager.h"
#include "DungeonCoordinator.h"
#include "TrashPullManager.h"
#include "AI/Coordination/Messaging/BotMessageBus.h"
#include "AI/Coordination/Messaging/BotMessage.h"
#include "Player.h"
#include "Group.h"
#include "GameTime.h"
#include "Log.h"
#include <algorithm>
#include <cmath>

namespace Playerbot {

MythicPlusManager::MythicPlusManager(DungeonCoordinator* coordinator)
    : _coordinator(coordinator)
{
}

void MythicPlusManager::Initialize(const KeystoneInfo& keystone)
{
    Reset();

    _keystone = keystone;
    LoadForcesTable(keystone.dungeonId);
    CalculateOptimalRoute();

    TC_LOG_DEBUG("playerbot", "MythicPlusManager::Initialize - Initialized for +%u %u with %zu affixes",
                 keystone.level, keystone.dungeonId, keystone.affixes.size());
}

void MythicPlusManager::Update(uint32 diff)
{
    if (!IsActive())
        return;

    // Update quaking status
    if (_quakingActive)
    {
        uint32 now = GameTime::GetGameTimeMS();
        if (now >= _quakingEndTime)
        {
            _quakingActive = false;
        }
    }

    // Clean up expired/dead sanguine pools and explosive orbs
    // (In a real implementation, we'd validate these against actual game objects)
}

void MythicPlusManager::Reset()
{
    _keystone = KeystoneInfo{};
    _startTime = 0;
    _enemyForces = 0.0f;
    _deathCount = 0;
    _forcesTable.clear();
    _quakingActive = false;
    _quakingEndTime = 0;
    _sanguinePools.clear();
    _explosiveOrbs.clear();
    _volcanicPools.clear();
    _plannedRoute.clear();
    _currentRouteIndex = 0;
    _routeDirty = true;
}

void MythicPlusManager::StartTimer()
{
    _startTime = GameTime::GetGameTimeMS();

    TC_LOG_DEBUG("playerbot", "MythicPlusManager::StartTimer - Timer started for +%u, time limit %u ms",
                 _keystone.level, _keystone.timeLimit);
}

// ============================================================================
// TIMER
// ============================================================================

uint32 MythicPlusManager::GetElapsedTime() const
{
    if (_startTime == 0)
        return 0;

    return GameTime::GetGameTimeMS() - _startTime;
}

uint32 MythicPlusManager::GetRemainingTime() const
{
    uint32 elapsed = GetElapsedTime();
    uint32 penalty = GetTimePenalty();
    uint32 totalUsed = elapsed + penalty;

    if (totalUsed >= _keystone.timeLimit)
        return 0;

    return _keystone.timeLimit - totalUsed;
}

bool MythicPlusManager::IsOnTime() const
{
    if (_keystone.timeLimit == 0)
        return true;

    // Calculate expected progress based on time
    float timeProgress = GetTimeProgress();
    float completionProgress = _coordinator->GetCompletionPercent() / 100.0f;

    // On time if completion >= time progress
    return completionProgress >= timeProgress * 0.9f;  // 10% buffer
}

bool MythicPlusManager::CanTwoChest() const
{
    uint32 remaining = GetRemainingTime();
    float completionNeeded = 100.0f - _coordinator->GetCompletionPercent();

    // Estimate time needed for remaining content
    float timeProgress = GetTimeProgress();
    if (timeProgress <= 0.0f)
        return true;

    float rateOfProgress = _coordinator->GetCompletionPercent() / (timeProgress * 100.0f);
    if (rateOfProgress <= 0.0f)
        return false;

    float estimatedTimeNeeded = (completionNeeded / rateOfProgress) * _keystone.timeLimit;

    return remaining > estimatedTimeNeeded * TWO_CHEST_TIME_MOD;
}

bool MythicPlusManager::CanThreeChest() const
{
    uint32 remaining = GetRemainingTime();
    float completionNeeded = 100.0f - _coordinator->GetCompletionPercent();

    float timeProgress = GetTimeProgress();
    if (timeProgress <= 0.0f)
        return true;

    float rateOfProgress = _coordinator->GetCompletionPercent() / (timeProgress * 100.0f);
    if (rateOfProgress <= 0.0f)
        return false;

    float estimatedTimeNeeded = (completionNeeded / rateOfProgress) * _keystone.timeLimit;

    return remaining > estimatedTimeNeeded * THREE_CHEST_TIME_MOD;
}

float MythicPlusManager::GetTimeProgress() const
{
    if (_keystone.timeLimit == 0)
        return 0.0f;

    uint32 elapsed = GetElapsedTime() + GetTimePenalty();
    return static_cast<float>(elapsed) / _keystone.timeLimit;
}

uint32 MythicPlusManager::GetExpectedCompletionTime() const
{
    float completion = _coordinator->GetCompletionPercent();
    if (completion <= 0.0f)
        return _keystone.timeLimit;

    uint32 elapsed = GetElapsedTime();
    return static_cast<uint32>(elapsed / (completion / 100.0f));
}

// ============================================================================
// ENEMY FORCES
// ============================================================================

void MythicPlusManager::OnEnemyKilled(uint32 creatureId)
{
    float forces = GetForcesValue(creatureId);
    _enemyForces += forces;

    TC_LOG_DEBUG("playerbot", "MythicPlusManager::OnEnemyKilled - Killed creature %u, +%.2f%% forces, total %.2f%%",
                 creatureId, forces, _enemyForces);

    // Update route if we've gained enough forces
    if (_enemyForces >= 100.0f)
    {
        _routeDirty = true;
    }
}

float MythicPlusManager::GetForcesValue(uint32 creatureId) const
{
    auto it = _forcesTable.find(creatureId);
    return it != _forcesTable.end() ? it->second.forcesValue : 0.0f;
}

void MythicPlusManager::RegisterEnemyForces(uint32 creatureId, const EnemyForces& forces)
{
    _forcesTable[creatureId] = forces;
}

// ============================================================================
// DEATH COUNTER
// ============================================================================

void MythicPlusManager::OnPlayerDied()
{
    _deathCount++;

    TC_LOG_DEBUG("playerbot", "MythicPlusManager::OnPlayerDied - Death count: %u, penalty: %u ms",
                 _deathCount, GetTimePenalty());

    // Check if this death would cause deplete
    if (WouldDeplete())
    {
        TC_LOG_DEBUG("playerbot", "MythicPlusManager: Warning - Key will deplete!");
    }
}

bool MythicPlusManager::WouldDeplete() const
{
    return GetRemainingTime() == 0;
}

// ============================================================================
// ROUTE OPTIMIZATION
// ============================================================================

bool MythicPlusManager::ShouldSkipPack(uint32 packId) const
{
    // Can't skip if we need more forces
    if (_enemyForces < 100.0f)
    {
        // Check if this pack contributes forces
        TrashPullManager* trashMgr = _coordinator->GetTrashManager();
        if (trashMgr)
        {
            if (!trashMgr->CanSkipPack(packId))
                return false;
        }
    }
    else
    {
        // Already have enough forces - skip non-required packs
        TrashPullManager* trashMgr = _coordinator->GetTrashManager();
        if (trashMgr)
        {
            return trashMgr->CanSkipPack(packId);
        }
    }

    return false;
}

bool MythicPlusManager::ShouldPullExtra() const
{
    // Pull extra if we're running low on time but need forces
    if (!IsOnTime() && _enemyForces < 100.0f)
        return true;

    // Pull extra in Fortified weeks (trash is harder but gives same forces)
    if (_keystone.IsFortified())
        return false;

    return false;
}

::std::vector<uint32> MythicPlusManager::GetOptimalRoute() const
{
    if (_routeDirty)
    {
        const_cast<MythicPlusManager*>(this)->CalculateOptimalRoute();
    }
    return _plannedRoute;
}

float MythicPlusManager::GetRouteProgress() const
{
    if (_plannedRoute.empty())
        return 0.0f;

    return static_cast<float>(_currentRouteIndex) / _plannedRoute.size();
}

// ============================================================================
// AFFIX HANDLING
// ============================================================================

void MythicPlusManager::OnAffixTriggered(MythicPlusAffix affix, ObjectGuid source)
{
    switch (affix)
    {
        case MythicPlusAffix::QUAKING:
            _quakingActive = true;
            _quakingEndTime = GameTime::GetGameTimeMS() + QUAKING_DURATION_MS;
            TC_LOG_DEBUG("playerbot", "MythicPlusManager: Quaking active for %u ms", QUAKING_DURATION_MS);
            // Broadcast spread command for quaking
            if (_coordinator && _coordinator->GetGroup())
            {
                Group* group = _coordinator->GetGroup();
                ObjectGuid groupGuid = group->GetGUID();
                ObjectGuid leaderGuid = group->GetLeaderGUID();
                BotMessage msg = BotMessage::CommandSpread(leaderGuid, groupGuid);
                sBotMessageBus->Publish(msg);
            }
            break;

        case MythicPlusAffix::EXPLOSIVE:
            AddExplosiveOrb(source);
            // Broadcast focus target for explosive orb
            if (_coordinator && _coordinator->GetGroup())
            {
                Group* group = _coordinator->GetGroup();
                ObjectGuid groupGuid = group->GetGUID();
                ObjectGuid leaderGuid = group->GetLeaderGUID();
                BotMessage msg = BotMessage::CommandFocusTarget(leaderGuid, groupGuid, source);
                sBotMessageBus->Publish(msg);
            }
            break;

        case MythicPlusAffix::VOLCANIC:
            _volcanicPools.insert(source);
            break;

        case MythicPlusAffix::SANGUINE:
            AddSanguinePool(source);
            break;

        default:
            break;
    }
}

bool MythicPlusManager::ShouldKillExplosive(ObjectGuid explosive) const
{
    // Always kill explosives - they're dangerous
    return _explosiveOrbs.find(explosive) != _explosiveOrbs.end();
}

bool MythicPlusManager::ShouldAvoidSanguine(float x, float y, float z) const
{
    // In a real implementation, we'd check distance to pool positions
    // For now, just check if any pools exist
    return !_sanguinePools.empty();
}

void MythicPlusManager::AddSanguinePool(ObjectGuid pool)
{
    _sanguinePools.insert(pool);
}

void MythicPlusManager::RemoveSanguinePool(ObjectGuid pool)
{
    _sanguinePools.erase(pool);
}

void MythicPlusManager::AddExplosiveOrb(ObjectGuid orb)
{
    _explosiveOrbs.insert(orb);
}

void MythicPlusManager::RemoveExplosiveOrb(ObjectGuid orb)
{
    _explosiveOrbs.erase(orb);
}

// ============================================================================
// STRATEGY ADJUSTMENTS
// ============================================================================

float MythicPlusManager::GetDamageModifier() const
{
    float modifier = 1.0f;

    // Base scaling per level (8% per level after 2)
    if (_keystone.level > 2)
    {
        modifier += (_keystone.level - 2) * 0.08f;
    }

    // Tyrannical: Bosses deal 30% more damage
    // Fortified: Trash deals 30% more damage
    // (Applied separately based on target type)

    return modifier;
}

float MythicPlusManager::GetHealthModifier() const
{
    float modifier = 1.0f;

    // Base scaling per level (8% per level after 2)
    if (_keystone.level > 2)
    {
        modifier += (_keystone.level - 2) * 0.08f;
    }

    // Tyrannical/Fortified add 30% HP to respective targets
    return modifier;
}

bool MythicPlusManager::ShouldUseCooldowns() const
{
    // Use CDs on tough packs in Fortified
    if (_keystone.IsFortified())
    {
        // Check if current pack is difficult
        return true;  // Simplified
    }

    // Use CDs on bosses in Tyrannical
    if (_keystone.IsTyrannical() && _coordinator->IsInBossEncounter())
    {
        return true;
    }

    return false;
}

bool MythicPlusManager::ShouldLust() const
{
    // Tyrannical: Lust on bosses
    if (_keystone.IsTyrannical() && _coordinator->IsInBossEncounter())
    {
        return true;
    }

    // Fortified: Lust on dangerous packs
    if (_keystone.IsFortified())
    {
        // Check if current pack is difficult
        TrashPullManager* trashMgr = _coordinator->GetTrashManager();
        if (trashMgr)
        {
            PullPlan* plan = trashMgr->GetCurrentPullPlan();
            if (plan)
            {
                return trashMgr->GetEstimatedPullDifficulty(plan->packId) > 70;
            }
        }
    }

    return false;
}

uint8 MythicPlusManager::GetRecommendedPullSize() const
{
    // In Fortified, smaller pulls
    if (_keystone.IsFortified())
        return 1;

    // In Tyrannical, can do bigger pulls
    if (_keystone.IsTyrannical())
        return 2;

    // Bolstering: Keep pulls even-sized to avoid buffing
    if (HasAffix(MythicPlusAffix::BOLSTERING))
        return 1;

    // Bursting: Smaller pulls to manage stacks
    if (HasAffix(MythicPlusAffix::BURSTING))
        return 1;

    return 2;  // Default: Pull 2 packs if possible
}

// ============================================================================
// HELPERS
// ============================================================================

void MythicPlusManager::LoadForcesTable(uint32 dungeonId)
{
    // LIMITATION: M+ forces-per-creature data is not available in TrinityCore DB.
    // Forces values would need a custom `playerbot_mythic_forces` table.
    // Currently, forces tracking relies on creature kill count as a proxy.

    TC_LOG_DEBUG("playerbot", "MythicPlusManager::LoadForcesTable - Loading forces for dungeon %u", dungeonId);
}

void MythicPlusManager::CalculateOptimalRoute()
{
    _plannedRoute.clear();

    // Get trash manager's route as base
    TrashPullManager* trashMgr = _coordinator->GetTrashManager();
    if (!trashMgr)
        return;

    auto baseRoute = trashMgr->GetOptimalClearOrder();

    // Filter based on forces needed
    float forcesGained = 0.0f;
    bool hasEnoughForces = _enemyForces >= 100.0f;

    for (uint32 packId : baseRoute)
    {
        // Skip if we have enough forces and pack is optional
        if (hasEnoughForces && ShouldSkipPack(packId))
            continue;

        _plannedRoute.push_back(packId);

        // Calculate forces this pack would give
        TrashPack* pack = trashMgr->GetPack(packId);
        if (pack)
        {
            for (const ObjectGuid& member : pack->members)
            {
                // Estimate forces (in real implementation, look up creature ID)
                forcesGained += 1.0f;  // Simplified
            }
        }

        // Update forces check
        if (!hasEnoughForces && (_enemyForces + forcesGained) >= 100.0f)
        {
            hasEnoughForces = true;
        }
    }

    _routeDirty = false;

    TC_LOG_DEBUG("playerbot", "MythicPlusManager::CalculateOptimalRoute - Route calculated with %zu packs",
                 _plannedRoute.size());
}

float MythicPlusManager::GetAffixScaling() const
{
    // Affixes scale with key level
    // Level 4+: First affix
    // Level 7+: Second affix
    // Level 10+: Third affix

    float scaling = 1.0f;

    if (_keystone.level >= 4)
        scaling += 0.1f;
    if (_keystone.level >= 7)
        scaling += 0.1f;
    if (_keystone.level >= 10)
        scaling += 0.1f;

    return scaling;
}

} // namespace Playerbot
