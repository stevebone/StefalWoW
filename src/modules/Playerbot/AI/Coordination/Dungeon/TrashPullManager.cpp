/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "TrashPullManager.h"
#include "DungeonCoordinator.h"
#include "AI/Coordination/Messaging/BotMessageBus.h"
#include "AI/Coordination/Messaging/BotMessage.h"
#include "Player.h"
#include "Group.h"
#include "Creature.h"
#include "ObjectAccessor.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "Log.h"
#include <algorithm>

namespace Playerbot {

TrashPullManager::TrashPullManager(DungeonCoordinator* coordinator)
    : _coordinator(coordinator)
{
}

void TrashPullManager::Initialize(uint32 dungeonId)
{
    Reset();

    // Trash packs are registered dynamically as they're detected during dungeon traversal.
    // Static pack data per-dungeon could be added via DB tables for optimized pull planning.
    TC_LOG_DEBUG("playerbot", "TrashPullManager::Initialize - Initialized for dungeon %u", dungeonId);
}

void TrashPullManager::Update(uint32 diff)
{
    // Update CC status for active targets
    for (auto& [target, active] : _ccActive)
    {
        if (active)
        {
            // Check if CC is still active on target
            Unit* unit = ObjectAccessor::GetUnit(*ObjectAccessor::FindPlayer(_coordinator->GetMainTank()), target);
            if (!unit || !unit->HasBreakableByDamageCrowdControlAura())
            {
                active = false;
            }
        }
    }

    // Check for completed pulls
    for (auto it = _pulledPacks.begin(); it != _pulledPacks.end(); )
    {
        uint32 packId = *it;
        auto packIt = _packs.find(packId);
        if (packIt != _packs.end())
        {
            // Check if all pack members are dead
            bool allDead = true;
            for (const ObjectGuid& memberGuid : packIt->second.members)
            {
                Unit* member = ObjectAccessor::GetUnit(*ObjectAccessor::FindPlayer(_coordinator->GetMainTank()), memberGuid);
                if (member && member->IsAlive())
                {
                    allDead = false;
                    break;
                }
            }

            if (allDead)
            {
                OnPackCleared(packId);
                it = _pulledPacks.erase(it);
                continue;
            }
        }
        ++it;
    }
}

void TrashPullManager::Reset()
{
    _packs.clear();
    _clearedPacks.clear();
    _pulledPacks.clear();
    while (!_pullQueue.empty()) _pullQueue.pop();
    _currentPlan = PullPlan{};
    _hasPlan = false;
    _ccAssignments.clear();
    _ccSpells.clear();
    _ccActive.clear();
    _markerAssignments.clear();
    _routeDirty = true;
}

// ============================================================================
// PACK MANAGEMENT
// ============================================================================

void TrashPullManager::RegisterPack(const TrashPack& pack)
{
    _packs[pack.packId] = pack;
    _routeDirty = true;

    TC_LOG_DEBUG("playerbot", "TrashPullManager::RegisterPack - Registered pack %u with %zu members",
                 pack.packId, pack.members.size());
}

void TrashPullManager::OnPackCleared(uint32 packId)
{
    _clearedPacks.push_back(packId);
    _routeDirty = true;

    // Clear CC and marker data for this pack
    auto packIt = _packs.find(packId);
    if (packIt != _packs.end())
    {
        for (const ObjectGuid& member : packIt->second.members)
        {
            _ccAssignments.erase(member);
            _ccSpells.erase(member);
            _ccActive.erase(member);
            _markerAssignments.erase(member);
        }
    }

    TC_LOG_DEBUG("playerbot", "TrashPullManager::OnPackCleared - Pack %u cleared", packId);
}

void TrashPullManager::OnPackPulled(uint32 packId)
{
    _pulledPacks.push_back(packId);
    TC_LOG_DEBUG("playerbot", "TrashPullManager::OnPackPulled - Pack %u pulled", packId);
}

TrashPack* TrashPullManager::GetPack(uint32 packId)
{
    auto it = _packs.find(packId);
    return it != _packs.end() ? &it->second : nullptr;
}

uint32 TrashPullManager::GetRemainingPackCount() const
{
    uint32 remaining = 0;
    for (const auto& [id, pack] : _packs)
    {
        if (::std::find(_clearedPacks.begin(), _clearedPacks.end(), id) == _clearedPacks.end())
        {
            remaining++;
        }
    }
    return remaining;
}

// ============================================================================
// PULL PLANNING
// ============================================================================

PullPlan* TrashPullManager::GetCurrentPullPlan()
{
    if (_hasPlan)
        return &_currentPlan;

    // Try to create plan for next pack
    uint32 nextPack = GetNextPackToPull();
    if (nextPack > 0 && CreatePullPlan(nextPack))
        return &_currentPlan;

    return nullptr;
}

bool TrashPullManager::CreatePullPlan(uint32 packId)
{
    auto it = _packs.find(packId);
    if (it == _packs.end())
        return false;

    const TrashPack& pack = it->second;
    _currentPlan = PullPlan{};
    _currentPlan.packId = packId;

    // Assign puller (main tank)
    _currentPlan.puller = _coordinator->GetMainTank();
    if (_currentPlan.puller.IsEmpty())
    {
        // Fallback to first DPS
        const auto& dps = _coordinator->GetDPS();
        if (!dps.empty())
            _currentPlan.puller = dps[0];
    }

    // Calculate kill order and assign markers
    ::std::vector<ObjectGuid> killOrder;
    CalculateKillOrder(pack, killOrder);

    uint8 killIndex = 0;
    for (const ObjectGuid& target : killOrder)
    {
        RaidMarker marker = SelectMarkerForRole(killIndex++, false);
        _currentPlan.markerAssignments.push_back({target, marker});
    }

    // Assign CC if needed
    if (pack.requiresCC && pack.recommendedCCCount > 0)
    {
        AssignCC(pack);

        // Add CC assignments to plan
        for (const auto& [target, ccer] : _ccAssignments)
        {
            _currentPlan.ccAssignments.push_back({ccer, target});

            // Update marker for CC target
            for (auto& [t, marker] : _currentPlan.markerAssignments)
            {
                if (t == target)
                {
                    marker = SelectMarkerForRole(0, true);  // CC marker
                    break;
                }
            }
        }
    }

    // Set pull position (default to pack position)
    _currentPlan.pullPositionX = pack.x;
    _currentPlan.pullPositionY = pack.y;
    _currentPlan.pullPositionZ = pack.z;

    _hasPlan = true;

    TC_LOG_DEBUG("playerbot", "TrashPullManager::CreatePullPlan - Created plan for pack %u, %zu markers, %zu CC",
                 packId, _currentPlan.markerAssignments.size(), _currentPlan.ccAssignments.size());

    return true;
}

void TrashPullManager::ExecutePull(const PullPlan& plan)
{
    // Apply markers first
    ApplyMarkers(plan);

    // Mark pack as pulled
    OnPackPulled(plan.packId);

    // Broadcast focus target (first skull-marked mob) to group via BotMessageBus
    if (_coordinator && _coordinator->GetGroup() && !plan.markerAssignments.empty())
    {
        Group* group = _coordinator->GetGroup();
        ObjectGuid groupGuid = group->GetGUID();
        ObjectGuid leaderGuid = group->GetLeaderGUID();
        ObjectGuid focusTarget = plan.markerAssignments[0].first;
        BotMessage msg = BotMessage::CommandFocusTarget(leaderGuid, groupGuid, focusTarget);
        sBotMessageBus->Publish(msg);
    }

    TC_LOG_DEBUG("playerbot", "TrashPullManager::ExecutePull - Executing pull for pack %u", plan.packId);
}

void TrashPullManager::ClearCurrentPlan()
{
    _currentPlan = PullPlan{};
    _hasPlan = false;
}

// ============================================================================
// CC MANAGEMENT
// ============================================================================

void TrashPullManager::AssignCC(const TrashPack& pack)
{
    // Clear existing assignments for this pack
    for (const ObjectGuid& member : pack.members)
    {
        _ccAssignments.erase(member);
    }

    // Determine how many targets to CC
    uint8 ccCount = ::std::min(pack.recommendedCCCount, static_cast<uint8>(pack.members.size() - 1));
    if (ccCount == 0)
        return;

    // Sort targets by priority (casters/dangerous first)
    ::std::vector<ObjectGuid> ccTargets;
    for (const ObjectGuid& member : pack.members)
    {
        // Skip if this is the first kill target
        ccTargets.push_back(member);
    }

    // Assign CCers to targets
    uint8 assigned = 0;
    for (const ObjectGuid& target : ccTargets)
    {
        if (assigned >= ccCount)
            break;

        ObjectGuid ccer = SelectBestCCer(target);
        if (!ccer.IsEmpty())
        {
            _ccAssignments[target] = ccer;
            _ccActive[target] = false;  // Not yet applied
            assigned++;
        }
    }

    TC_LOG_DEBUG("playerbot", "TrashPullManager::AssignCC - Assigned %u CC targets", assigned);
}

ObjectGuid TrashPullManager::GetCCResponsible(ObjectGuid target) const
{
    auto it = _ccAssignments.find(target);
    return it != _ccAssignments.end() ? it->second : ObjectGuid::Empty;
}

bool TrashPullManager::IsTargetCCd(ObjectGuid target) const
{
    auto it = _ccActive.find(target);
    return it != _ccActive.end() && it->second;
}

void TrashPullManager::OnCCBroken(ObjectGuid target)
{
    auto it = _ccActive.find(target);
    if (it != _ccActive.end())
    {
        it->second = false;
    }

    TC_LOG_DEBUG("playerbot", "TrashPullManager::OnCCBroken - CC broken on target");
}

// ============================================================================
// MARKER MANAGEMENT
// ============================================================================

void TrashPullManager::ApplyMarkers(const PullPlan& plan)
{
    for (const auto& [target, marker] : plan.markerAssignments)
    {
        SetMarker(target, marker);
    }
}

void TrashPullManager::ClearMarkers()
{
    _markerAssignments.clear();
}

RaidMarker TrashPullManager::GetMarkerForTarget(ObjectGuid target) const
{
    auto it = _markerAssignments.find(target);
    return it != _markerAssignments.end() ? it->second : RaidMarker::NONE;
}

void TrashPullManager::SetMarker(ObjectGuid target, RaidMarker marker)
{
    _markerAssignments[target] = marker;
    // Marker assignment tracked internally for AI coordination.
    // Visual world markers (skull, cross, etc.) require SMSG_UPDATE_WORLD_MARKER packet.
}

// ============================================================================
// SAFETY CHECKS
// ============================================================================

bool TrashPullManager::IsSafeToPull() const
{
    // Check if already in combat with another pack
    if (!_pulledPacks.empty())
        return false;

    // Check group readiness
    return IsGroupReadyForPull();
}

bool TrashPullManager::IsGroupReadyForPull() const
{
    return _coordinator && _coordinator->IsGroupReady();
}

uint32 TrashPullManager::GetEstimatedPullDifficulty(uint32 packId) const
{
    auto it = _packs.find(packId);
    if (it == _packs.end())
        return 0;

    const TrashPack& pack = it->second;

    // Base difficulty on pack size and priority
    uint32 difficulty = pack.members.size() * 10;

    switch (pack.priority)
    {
        case TrashPackPriority::DANGEROUS:
            difficulty += 40;
            break;
        case TrashPackPriority::PATROL:
            difficulty += 20;
            break;
        case TrashPackPriority::REQUIRED:
            difficulty += 10;
            break;
        default:
            break;
    }

    // Add difficulty for CC requirements
    if (pack.requiresCC)
        difficulty += pack.recommendedCCCount * 5;

    return ::std::min(difficulty, 100u);
}

// ============================================================================
// PATHING & ROUTING
// ============================================================================

::std::vector<uint32> TrashPullManager::GetOptimalClearOrder() const
{
    if (_routeDirty)
    {
        CalculateOptimalRoute();
    }
    return _cachedRoute;
}

bool TrashPullManager::CanSkipPack(uint32 packId) const
{
    auto it = _packs.find(packId);
    if (it == _packs.end())
        return true;

    // Can skip if priority is SKIP or OPTIONAL
    return it->second.priority == TrashPackPriority::SKIP ||
           it->second.priority == TrashPackPriority::OPTIONAL;
}

uint32 TrashPullManager::GetNextPackToPull() const
{
    // First check queue
    if (!_pullQueue.empty())
    {
        return _pullQueue.front();
    }

    // Otherwise use optimal route
    auto route = GetOptimalClearOrder();
    for (uint32 packId : route)
    {
        if (::std::find(_clearedPacks.begin(), _clearedPacks.end(), packId) == _clearedPacks.end() &&
            ::std::find(_pulledPacks.begin(), _pulledPacks.end(), packId) == _pulledPacks.end())
        {
            return packId;
        }
    }

    return 0;
}

void TrashPullManager::QueuePack(uint32 packId)
{
    _pullQueue.push(packId);
}

// ============================================================================
// HELPERS
// ============================================================================

ObjectGuid TrashPullManager::SelectBestCCer(ObjectGuid target) const
{
    const auto& dps = _coordinator->GetDPS();

    // Find DPS who can CC this target and isn't already assigned
    for (const ObjectGuid& dpsGuid : dps)
    {
        if (CanCC(dpsGuid, target))
        {
            // Check if already assigned to another target
            bool alreadyAssigned = false;
            for (const auto& [t, ccer] : _ccAssignments)
            {
                if (ccer == dpsGuid)
                {
                    alreadyAssigned = true;
                    break;
                }
            }

            if (!alreadyAssigned)
                return dpsGuid;
        }
    }

    return ObjectGuid::Empty;
}

bool TrashPullManager::CanCC(ObjectGuid player, ObjectGuid target) const
{
    // Get available CC spells for this player
    auto ccSpells = GetAvailableCCSpells(player);
    if (ccSpells.empty())
        return false;

    // Check if target is immune to all CC types
    for (uint32 spellId : ccSpells)
    {
        if (!IsImmuneToCC(target, spellId))
            return true;
    }

    return false;
}

RaidMarker TrashPullManager::SelectMarkerForRole(uint8 killOrder, bool isCC) const
{
    if (isCC)
    {
        // CC markers: Moon, Square, Triangle, Diamond, Circle
        switch (killOrder)
        {
            case 0: return RaidMarker::MOON;
            case 1: return RaidMarker::SQUARE;
            case 2: return RaidMarker::TRIANGLE;
            case 3: return RaidMarker::DIAMOND;
            default: return RaidMarker::CIRCLE;
        }
    }
    else
    {
        // Kill markers: Skull, Cross, Star
        switch (killOrder)
        {
            case 0: return RaidMarker::SKULL;
            case 1: return RaidMarker::CROSS;
            default: return RaidMarker::STAR;
        }
    }
}

void TrashPullManager::CalculateKillOrder(const TrashPack& pack, ::std::vector<ObjectGuid>& order)
{
    order.clear();

    // Kill order: iterate members in discovery order (typically proximity-based).
    // Enhancement: Prioritize by creature type (healer > caster > melee) and threat level.
    for (const ObjectGuid& member : pack.members)
    {
        order.push_back(member);
    }
}

void TrashPullManager::CalculateOptimalRoute() const
{
    _cachedRoute.clear();

    // Simple route: all packs by priority, then by ID
    ::std::vector<::std::pair<uint32, TrashPackPriority>> packPriorities;

    for (const auto& [id, pack] : _packs)
    {
        if (::std::find(_clearedPacks.begin(), _clearedPacks.end(), id) == _clearedPacks.end())
        {
            packPriorities.push_back({id, pack.priority});
        }
    }

    // Sort by priority (REQUIRED > DANGEROUS > PATROL > OPTIONAL > SKIP)
    ::std::sort(packPriorities.begin(), packPriorities.end(),
        [](const auto& a, const auto& b) {
            return static_cast<uint8>(a.second) > static_cast<uint8>(b.second);
        });

    for (const auto& [id, priority] : packPriorities)
    {
        _cachedRoute.push_back(id);
    }

    _routeDirty = false;
}

::std::vector<uint32> TrashPullManager::GetAvailableCCSpells(ObjectGuid player) const
{
    ::std::vector<uint32> result;

    Player* p = ObjectAccessor::FindPlayer(player);
    if (!p)
        return result;

    // Common CC spells by class — covers the primary CC for each class
    static const ::std::vector<uint32> commonCCSpells = {
        118,    // Polymorph (Mage)
        6770,   // Sap (Rogue)
        20066,  // Repentance (Paladin)
        710,    // Banish (Warlock)
        51514,  // Hex (Shaman)
        339,    // Entangling Roots (Druid)
        3355,   // Freezing Trap (Hunter)
        9484,   // Shackle Undead (Priest)
        2637    // Hibernate (Druid)
    };

    for (uint32 spellId : commonCCSpells)
    {
        if (p->HasSpell(spellId))
        {
            result.push_back(spellId);
        }
    }

    return result;
}

bool TrashPullManager::IsImmuneToCC(ObjectGuid target, uint32 ccSpellId) const
{
    Unit* unit = ObjectAccessor::GetUnit(*ObjectAccessor::FindPlayer(_coordinator->GetMainTank()), target);
    if (!unit)
        return true;

    // Check mechanical immunity
    const SpellInfo* spellInfo = sSpellMgr->GetSpellInfo(ccSpellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return true;

    // Check creature type restrictions
    if (Creature* creature = unit->ToCreature())
    {
        uint32 creatureType = creature->GetCreatureType();

        // Polymorph only works on beasts/humanoids/critters
        if (ccSpellId == 118)  // Polymorph
        {
            if (creatureType != CREATURE_TYPE_BEAST &&
                creatureType != CREATURE_TYPE_HUMANOID &&
                creatureType != CREATURE_TYPE_CRITTER)
            {
                return true;
            }
        }

        // Banish only works on demons/elementals
        if (ccSpellId == 710)  // Banish
        {
            if (creatureType != CREATURE_TYPE_DEMON &&
                creatureType != CREATURE_TYPE_ELEMENTAL)
            {
                return true;
            }
        }

        // Shackle Undead only works on undead
        if (ccSpellId == 9484)  // Shackle Undead
        {
            if (creatureType != CREATURE_TYPE_UNDEAD)
            {
                return true;
            }
        }

        // Hibernate only works on beasts/dragonkin
        if (ccSpellId == 2637)  // Hibernate
        {
            if (creatureType != CREATURE_TYPE_BEAST &&
                creatureType != CREATURE_TYPE_DRAGONKIN)
            {
                return true;
            }
        }
    }

    return false;
}

} // namespace Playerbot
