/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "DungeonAutonomyManager.h"
#include "AI/Coordination/Dungeon/DungeonCoordinator.h"
#include "DungeonScriptMgr.h"
#include "DungeonScript.h"
#include "AI/BotAI.h"
#include "Player.h"
#include "Group.h"
#include "Map.h"
#include "Creature.h"
#include "SpellAuras.h"
#include "ObjectAccessor.h"
#include "GameTime.h"
#include "Log.h"
#include "MotionMaster.h"
#include "MovementGenerator.h"
#include "DBCEnums.h"  // For ChrSpecialization enum
#include "Core/PlayerBotHelpers.h"

namespace Playerbot
{

// ============================================================================
// SINGLETON
// ============================================================================

DungeonAutonomyManager* DungeonAutonomyManager::instance()
{
    static DungeonAutonomyManager instance;
    return &instance;
}

DungeonAutonomyManager::DungeonAutonomyManager()
{
    TC_LOG_INFO("server.loading", "DungeonAutonomyManager initialized");
}

DungeonAutonomyManager::~DungeonAutonomyManager() = default;

// ============================================================================
// PAUSE/RESUME CONTROLS
// ============================================================================

bool DungeonAutonomyManager::PauseDungeonAutonomy(Group* group, Player* pausedBy, const std::string& reason)
{
    if (!group)
        return false;

    std::lock_guard<decltype(_mutex)> lock(_mutex);

    auto& state = GetOrCreateState(group);

    // Can only pause if currently active or waiting
    if (state.state != DungeonAutonomyState::ACTIVE &&
        state.state != DungeonAutonomyState::WAITING &&
        state.state != DungeonAutonomyState::RECOVERING)
    {
        if (state.state == DungeonAutonomyState::PAUSED)
        {
            TC_LOG_DEBUG("module.playerbot.dungeon", "DungeonAutonomy already paused for group {}",
                group->GetGUID().GetCounter());
            return true; // Already paused
        }

        TC_LOG_WARN("module.playerbot.dungeon", "Cannot pause dungeon autonomy - state is {} for group {}",
            static_cast<int>(state.state), group->GetGUID().GetCounter());
        return false;
    }

    DungeonAutonomyState oldState = state.state;
    state.state = DungeonAutonomyState::PAUSED;
    state.pausedByPlayerGuid = pausedBy ? pausedBy->GetGUID().GetCounter() : 0;
    state.pauseReason = reason.empty() ? "Player command" : reason;
    state.lastStateChangeTime = GameTime::GetGameTimeMS();

    TC_LOG_INFO("module.playerbot.dungeon", "🛑 DUNGEON AUTONOMY PAUSED for group {} by {} - Reason: {}",
        group->GetGUID().GetCounter(),
        pausedBy ? pausedBy->GetName() : "System",
        state.pauseReason);

    LogStateTransition(group, oldState, DungeonAutonomyState::PAUSED);

    return true;
}

bool DungeonAutonomyManager::ResumeDungeonAutonomy(Group* group, Player* resumedBy)
{
    if (!group)
        return false;

    std::lock_guard<decltype(_mutex)> lock(_mutex);

    auto& state = GetOrCreateState(group);

    if (state.state != DungeonAutonomyState::PAUSED)
    {
        TC_LOG_DEBUG("module.playerbot.dungeon", "Cannot resume - autonomy not paused for group {}",
            group->GetGUID().GetCounter());
        return false;
    }

    DungeonAutonomyState oldState = state.state;
    state.state = DungeonAutonomyState::ACTIVE;
    state.pausedByPlayerGuid = 0;
    state.pauseReason.clear();
    state.lastStateChangeTime = GameTime::GetGameTimeMS();

    TC_LOG_INFO("module.playerbot.dungeon", "▶️ DUNGEON AUTONOMY RESUMED for group {} by {}",
        group->GetGUID().GetCounter(),
        resumedBy ? resumedBy->GetName() : "System");

    LogStateTransition(group, oldState, DungeonAutonomyState::ACTIVE);

    return true;
}

bool DungeonAutonomyManager::ToggleDungeonPause(Group* group, Player* toggledBy)
{
    if (!group)
        return false;

    std::lock_guard<decltype(_mutex)> lock(_mutex);

    auto& state = GetOrCreateState(group);

    if (state.state == DungeonAutonomyState::PAUSED)
    {
        // Currently paused - resume
        ResumeDungeonAutonomy(group, toggledBy);
        return false; // Now active (not paused)
    }
    else if (state.state == DungeonAutonomyState::ACTIVE ||
             state.state == DungeonAutonomyState::WAITING ||
             state.state == DungeonAutonomyState::RECOVERING)
    {
        // Currently active - pause
        PauseDungeonAutonomy(group, toggledBy, "Toggle command");
        return true; // Now paused
    }

    return state.state == DungeonAutonomyState::PAUSED;
}

bool DungeonAutonomyManager::IsPaused(Group* group) const
{
    if (!group)
        return true; // No group = effectively paused

    std::lock_guard<decltype(_mutex)> lock(_mutex);

    auto* state = GetState(group);
    return state && state->state == DungeonAutonomyState::PAUSED;
}

bool DungeonAutonomyManager::IsAutonomyEnabled(Group* group) const
{
    if (!group)
        return false;

    std::lock_guard<decltype(_mutex)> lock(_mutex);

    auto* state = GetState(group);
    return state && state->state != DungeonAutonomyState::DISABLED;
}

// ============================================================================
// AUTONOMY CONTROL
// ============================================================================

void DungeonAutonomyManager::EnableAutonomy(Group* group, const DungeonAutonomyConfig& config)
{
    if (!group)
        return;

    std::lock_guard<decltype(_mutex)> lock(_mutex);

    auto& state = GetOrCreateState(group);
    state.config = config;
    state.state = DungeonAutonomyState::ACTIVE;
    state.lastStateChangeTime = GameTime::GetGameTimeMS();

    TC_LOG_INFO("module.playerbot.dungeon", "✅ DUNGEON AUTONOMY ENABLED for group {} - Aggression: {}",
        group->GetGUID().GetCounter(),
        static_cast<int>(config.aggressionLevel));

    // Create coordinator if needed
    GetOrCreateCoordinator(group);
}

void DungeonAutonomyManager::DisableAutonomy(Group* group)
{
    if (!group)
        return;

    std::lock_guard<decltype(_mutex)> lock(_mutex);

    uint32 groupId = group->GetGUID().GetCounter();

    auto it = _groupStates.find(groupId);
    if (it != _groupStates.end())
    {
        it->second.state = DungeonAutonomyState::DISABLED;
        it->second.lastStateChangeTime = GameTime::GetGameTimeMS();
    }

    TC_LOG_INFO("module.playerbot.dungeon", "❌ DUNGEON AUTONOMY DISABLED for group {}",
        groupId);
}

void DungeonAutonomyManager::SetAggressionLevel(Group* group, DungeonAggressionLevel level)
{
    if (!group)
        return;

    std::lock_guard<decltype(_mutex)> lock(_mutex);

    auto& state = GetOrCreateState(group);
    state.config.aggressionLevel = level;

    // Adjust config based on aggression
    switch (level)
    {
        case DungeonAggressionLevel::CONSERVATIVE:
            state.config.minHealthToPull = 0.9f;
            state.config.minManaToPull = 0.8f;
            state.config.recoveryTimeMs = 10000;
            state.config.maxPullSize = 1;
            state.config.allowChainPulling = false;
            break;

        case DungeonAggressionLevel::NORMAL:
            state.config.minHealthToPull = 0.7f;
            state.config.minManaToPull = 0.5f;
            state.config.recoveryTimeMs = 5000;
            state.config.maxPullSize = 1;
            state.config.allowChainPulling = false;
            break;

        case DungeonAggressionLevel::AGGRESSIVE:
            state.config.minHealthToPull = 0.5f;
            state.config.minManaToPull = 0.3f;
            state.config.recoveryTimeMs = 2000;
            state.config.maxPullSize = 2;
            state.config.allowChainPulling = true;
            break;

        case DungeonAggressionLevel::SPEED_RUN:
            state.config.minHealthToPull = 0.3f;
            state.config.minManaToPull = 0.2f;
            state.config.recoveryTimeMs = 0;
            state.config.maxPullSize = 3;
            state.config.allowChainPulling = true;
            state.config.waitForSlowMembers = false;
            break;
    }

    TC_LOG_INFO("module.playerbot.dungeon", "Aggression level set to {} for group {}",
        static_cast<int>(level), group->GetGUID().GetCounter());
}

DungeonAutonomyState DungeonAutonomyManager::GetAutonomyState(Group* group) const
{
    if (!group)
        return DungeonAutonomyState::DISABLED;

    std::lock_guard<decltype(_mutex)> lock(_mutex);

    auto* state = GetState(group);
    return state ? state->state : DungeonAutonomyState::DISABLED;
}

DungeonAutonomyConfig DungeonAutonomyManager::GetConfig(Group* group) const
{
    if (!group)
        return {};

    std::lock_guard<decltype(_mutex)> lock(_mutex);

    auto* state = GetState(group);
    return state ? state->config : DungeonAutonomyConfig{};
}

void DungeonAutonomyManager::UpdateConfig(Group* group, const DungeonAutonomyConfig& config)
{
    if (!group)
        return;

    std::lock_guard<decltype(_mutex)> lock(_mutex);

    auto& state = GetOrCreateState(group);
    state.config = config;
}

// ============================================================================
// MAIN UPDATE LOOP
// ============================================================================

bool DungeonAutonomyManager::UpdateBotInDungeon(Player* bot, BotAI* ai, uint32 diff)
{
    if (!bot || !ai)
        return false;

    // Must be in dungeon
    Map* map = bot->GetMap();
    if (!map || !map->IsDungeon())
        return false;

    // Must be in group
    Group* group = bot->GetGroup();
    if (!group)
        return false;

    std::lock_guard<decltype(_mutex)> lock(_mutex);

    // Check autonomy state
    auto* state = GetState(group);
    if (!state || state->state == DungeonAutonomyState::DISABLED)
        return false;

    // Paused - do nothing autonomous
    if (state->state == DungeonAutonomyState::PAUSED)
        return false;

    // Get or create coordinator
    DungeonCoordinator* coordinator = GetOrCreateCoordinator(group);
    if (!coordinator)
        return false;

    // Delegate to role-specific handlers
    if (IsTankRole(bot, group))
    {
        return UpdateTankAI(bot, ai, group, coordinator, diff);
    }
    else if (IsHealerRole(bot, group))
    {
        return UpdateHealerAI(bot, ai, group, coordinator, diff);
    }
    else
    {
        return UpdateDpsAI(bot, ai, group, coordinator, diff);
    }
}

// ============================================================================
// TANK DECISIONS
// ============================================================================

bool DungeonAutonomyManager::ShouldTankPull(Player* tank, Group* group, DungeonCoordinator* coordinator)
{
    if (!tank || !group || !coordinator)
        return false;

    auto* state = GetState(group);
    if (!state)
        return false;

    // Don't pull if paused
    if (state->state == DungeonAutonomyState::PAUSED)
        return false;

    // Don't pull if in combat (unless chain pulling allowed)
    if (coordinator->IsInCombat() && !state->config.allowChainPulling)
        return false;

    // Check recovery time
    uint32 now = GameTime::GetGameTimeMS();
    if (now - state->lastPullTime < state->config.recoveryTimeMs)
        return false;

    // Check if safe to pull
    if (!coordinator->IsSafeToPull())
        return false;

    // Check group health/mana
    if (!IsGroupReadyToPull(group))
        return false;

    // Check if all members in range (if configured)
    if (state->config.waitForSlowMembers)
    {
        if (!AreAllMembersInRange(group, tank->GetPosition(), state->config.maxMemberDistance))
            return false;
    }

    // Check if there's a target to pull
    TrashPack* nextPack = coordinator->GetCurrentPullTarget();
    if (!nextPack)
    {
        // No trash - check for boss
        BossInfo* boss = coordinator->GetCurrentBoss();
        return boss != nullptr;
    }

    return true;
}

void DungeonAutonomyManager::ExecuteTankPull(Player* tank, Creature* target)
{
    if (!tank || !target)
        return;

    Group* group = tank->GetGroup();
    if (!group)
        return;

    std::lock_guard<decltype(_mutex)> lock(_mutex);

    auto& state = GetOrCreateState(group);
    state.lastPullTime = GameTime::GetGameTimeMS();
    state.state = DungeonAutonomyState::PULLING;

    // Mark target
    if (state.config.autoMarkTargets)
    {
        MarkPullTarget(target);
    }

    TC_LOG_INFO("module.playerbot.dungeon", "🎯 Tank {} pulling target {} (entry {})",
        tank->GetName(), target->GetName(), target->GetEntry());

    // Move to target and attack
    tank->GetMotionMaster()->MoveChase(target);
    tank->Attack(target, true);
}

bool DungeonAutonomyManager::ShouldTankAdvance(Player* tank, Group* group, DungeonCoordinator* coordinator)
{
    if (!tank || !group || !coordinator)
        return false;

    // In combat - don't advance
    if (coordinator->IsInCombat())
        return false;

    // Get current pull target position
    TrashPack* pack = coordinator->GetCurrentPullTarget();
    if (pack)
    {
        // Check distance to pack
        float distance = tank->GetDistance(pack->x, pack->y, pack->z);
        if (distance > 30.0f)
        {
            // Need to move closer
            return true;
        }
    }

    return false;
}

// ============================================================================
// GROUP COORDINATION
// ============================================================================

bool DungeonAutonomyManager::IsGroupReadyToPull(Group* group) const
{
    if (!group)
        return false;

    auto* state = GetState(group);
    if (!state)
        return false;

    // Check health
    float healthPct = GetGroupHealthPercent(group);
    if (healthPct < state->config.minHealthToPull)
    {
        TC_LOG_DEBUG("module.playerbot.dungeon", "Group not ready - health {} < {}",
            healthPct, state->config.minHealthToPull);
        return false;
    }

    // Check healer mana
    float manaPct = GetHealerManaPercent(group);
    if (manaPct < state->config.minManaToPull)
    {
        TC_LOG_DEBUG("module.playerbot.dungeon", "Group not ready - healer mana {} < {}",
            manaPct, state->config.minManaToPull);
        return false;
    }

    return true;
}

bool DungeonAutonomyManager::AreAllMembersInRange(Group* group, const Position& position, float maxDistance) const
{
    if (!group)
        return false;

    for (auto const& memberSlot : group->GetMemberSlots())
    {
        Player* member = ObjectAccessor::FindPlayer(memberSlot.guid);
        if (!member || !member->IsInWorld() || !member->IsAlive())
            continue;

        float distance = member->GetExactDist(&position);
        if (distance > maxDistance)
        {
            TC_LOG_DEBUG("module.playerbot.dungeon", "Member {} too far: {} > {}",
                member->GetName(), distance, maxDistance);
            return false;
        }
    }

    return true;
}

float DungeonAutonomyManager::GetGroupHealthPercent(Group* group) const
{
    if (!group)
        return 0.0f;

    float totalHealth = 0.0f;
    uint32 memberCount = 0;

    for (auto const& memberSlot : group->GetMemberSlots())
    {
        Player* member = ObjectAccessor::FindPlayer(memberSlot.guid);
        if (!member || !member->IsInWorld() || !member->IsAlive())
            continue;

        totalHealth += member->GetHealthPct();
        memberCount++;
    }

    return memberCount > 0 ? totalHealth / memberCount / 100.0f : 0.0f;
}

float DungeonAutonomyManager::GetHealerManaPercent(Group* group) const
{
    if (!group)
        return 0.0f;

    float totalMana = 0.0f;
    uint32 healerCount = 0;

    for (auto const& memberSlot : group->GetMemberSlots())
    {
        Player* member = ObjectAccessor::FindPlayer(memberSlot.guid);
        if (!member || !member->IsInWorld())
            continue;

        // Check if healer role
        if (IsHealerRole(const_cast<Player*>(member), const_cast<Group*>(group)))
        {
            if (member->GetMaxPower(POWER_MANA) > 0)
            {
                totalMana += static_cast<float>(member->GetPower(POWER_MANA)) /
                             static_cast<float>(member->GetMaxPower(POWER_MANA));
                healerCount++;
            }
        }
    }

    return healerCount > 0 ? totalMana / healerCount : 1.0f; // No healers = assume full
}

// ============================================================================
// GLOBAL UPDATE
// ============================================================================

void DungeonAutonomyManager::Update(uint32 diff)
{
    _updateTimer += diff;
    if (_updateTimer < UPDATE_INTERVAL_MS)
        return;

    _updateTimer = 0;

    std::lock_guard<decltype(_mutex)> lock(_mutex);

    // Update all coordinators
    for (auto& [groupId, coordinator] : _coordinators)
    {
        if (coordinator)
        {
            coordinator->Update(diff);
        }
    }
}

void DungeonAutonomyManager::OnGroupDisbanded(Group* group)
{
    if (!group)
        return;

    std::lock_guard<decltype(_mutex)> lock(_mutex);

    uint32 groupId = group->GetGUID().GetCounter();

    _groupStates.erase(groupId);
    _coordinators.erase(groupId);

    TC_LOG_DEBUG("module.playerbot.dungeon", "Cleaned up autonomy state for disbanded group {}",
        groupId);
}

void DungeonAutonomyManager::OnLeaveDungeon(Group* group)
{
    if (!group)
        return;

    std::lock_guard<decltype(_mutex)> lock(_mutex);

    uint32 groupId = group->GetGUID().GetCounter();

    auto it = _groupStates.find(groupId);
    if (it != _groupStates.end())
    {
        it->second.state = DungeonAutonomyState::DISABLED;
    }

    // Shutdown coordinator
    auto coordIt = _coordinators.find(groupId);
    if (coordIt != _coordinators.end() && coordIt->second)
    {
        coordIt->second->Shutdown();
        _coordinators.erase(coordIt);
    }

    TC_LOG_DEBUG("module.playerbot.dungeon", "Reset autonomy state for group {} leaving dungeon",
        groupId);
}

// ============================================================================
// COORDINATOR INTEGRATION
// ============================================================================

DungeonCoordinator* DungeonAutonomyManager::GetOrCreateCoordinator(Group* group)
{
    if (!group)
        return nullptr;

    uint32 groupId = group->GetGUID().GetCounter();

    auto it = _coordinators.find(groupId);
    if (it != _coordinators.end() && it->second)
    {
        return it->second.get();
    }

    // Create new coordinator
    auto coordinator = std::make_unique<DungeonCoordinator>(group);
    coordinator->Initialize();

    DungeonCoordinator* ptr = coordinator.get();
    _coordinators[groupId] = std::move(coordinator);

    TC_LOG_INFO("module.playerbot.dungeon", "Created DungeonCoordinator for group {}",
        groupId);

    return ptr;
}

DungeonCoordinator* DungeonAutonomyManager::GetCoordinator(Group* group) const
{
    if (!group)
        return nullptr;

    uint32 groupId = group->GetGUID().GetCounter();

    auto it = _coordinators.find(groupId);
    return (it != _coordinators.end()) ? it->second.get() : nullptr;
}

// ============================================================================
// INTERNAL HELPERS
// ============================================================================

GroupAutonomyState& DungeonAutonomyManager::GetOrCreateState(Group* group)
{
    uint32 groupId = group->GetGUID().GetCounter();

    auto it = _groupStates.find(groupId);
    if (it == _groupStates.end())
    {
        GroupAutonomyState newState(group->GetLeaderGUID());
        _groupStates[groupId] = newState;
        return _groupStates[groupId];
    }

    return it->second;
}

const GroupAutonomyState* DungeonAutonomyManager::GetState(Group* group) const
{
    if (!group)
        return nullptr;

    uint32 groupId = group->GetGUID().GetCounter();

    auto it = _groupStates.find(groupId);
    return (it != _groupStates.end()) ? &it->second : nullptr;
}

void DungeonAutonomyManager::TransitionState(Group* group, DungeonAutonomyState newState)
{
    if (!group)
        return;

    auto& state = GetOrCreateState(group);
    DungeonAutonomyState oldState = state.state;

    if (oldState != newState)
    {
        state.state = newState;
        state.lastStateChangeTime = GameTime::GetGameTimeMS();
        LogStateTransition(group, oldState, newState);
    }
}

bool DungeonAutonomyManager::UpdateTankAI(Player* tank, BotAI* ai, Group* group, DungeonCoordinator* coordinator, uint32 diff)
{
    // Check if should pull
    if (ShouldTankPull(tank, group, coordinator))
    {
        Creature* target = GetNextPullTarget(group, coordinator);
        if (target)
        {
            ExecuteTankPull(tank, target);
            return true;
        }
    }

    // Check if should advance
    if (ShouldTankAdvance(tank, group, coordinator))
    {
        TrashPack* pack = coordinator->GetCurrentPullTarget();
        if (pack)
        {
            // Use validated pathfinding for pack navigation
            Position dest(pack->x, pack->y, pack->z, 0.0f);
            if (BotAI* ai = GetBotAI(tank))
            {
                if (!ai->MoveTo(dest, true))
                {
                    // Fallback to legacy if validation fails
                    tank->GetMotionMaster()->MovePoint(0, pack->x, pack->y, pack->z);
                }
            }
            else
            {
                // Non-bot player - use standard movement
                tank->GetMotionMaster()->MovePoint(0, pack->x, pack->y, pack->z);
            }
            return true;
        }
    }

    return false; // Let normal AI handle
}

bool DungeonAutonomyManager::UpdateHealerAI(Player* healer, BotAI* ai, Group* group, DungeonCoordinator* coordinator, uint32 diff)
{
    // Healer follows tank, healing is handled by normal AI
    Player* tank = GetGroupTank(group);
    if (!tank)
        return false;

    // Stay in range of tank
    float distance = healer->GetExactDist(tank);
    if (distance > 25.0f)
    {
        // Move closer to tank
        Position followPos;
        float angle = healer->GetAbsoluteAngle(tank);
        followPos.m_positionX = tank->GetPositionX() + 15.0f * cos(angle);
        followPos.m_positionY = tank->GetPositionY() + 15.0f * sin(angle);
        followPos.m_positionZ = tank->GetPositionZ();

        // Use validated pathfinding for healer positioning
        if (BotAI* ai = GetBotAI(healer))
        {
            if (!ai->MoveTo(followPos, true))
            {
                // Fallback to legacy if validation fails
                healer->GetMotionMaster()->MovePoint(0, followPos);
            }
        }
        else
        {
            // Non-bot player - use standard movement
            healer->GetMotionMaster()->MovePoint(0, followPos);
        }
        return true;
    }

    return false; // Let normal AI handle healing
}

bool DungeonAutonomyManager::UpdateDpsAI(Player* dps, BotAI* ai, Group* group, DungeonCoordinator* coordinator, uint32 diff)
{
    // DPS follows tank
    Player* tank = GetGroupTank(group);
    if (!tank)
        return false;

    // Stay in range of tank
    float distance = dps->GetExactDist(tank);
    if (distance > 30.0f)
    {
        // Move closer to tank
        Position followPos;
        float angle = dps->GetAbsoluteAngle(tank);
        followPos.m_positionX = tank->GetPositionX() + 20.0f * cos(angle);
        followPos.m_positionY = tank->GetPositionY() + 20.0f * sin(angle);
        followPos.m_positionZ = tank->GetPositionZ();

        // Use validated pathfinding for DPS positioning
        if (BotAI* ai = GetBotAI(dps))
        {
            if (!ai->MoveTo(followPos, true))
            {
                // Fallback to legacy if validation fails
                dps->GetMotionMaster()->MovePoint(0, followPos);
            }
        }
        else
        {
            // Non-bot player - use standard movement
            dps->GetMotionMaster()->MovePoint(0, followPos);
        }
        return true;
    }

    return false; // Let normal AI handle combat
}

bool DungeonAutonomyManager::IsTankRole(Player* player, Group* /*group*/) const
{
    if (!player)
        return false;

    // Use ChrSpecialization to determine tank role
    ChrSpecialization spec = player->GetPrimarySpecialization();
    switch (spec)
    {
        case ChrSpecialization::WarriorProtection:
        case ChrSpecialization::PaladinProtection:
        case ChrSpecialization::DeathKnightBlood:
        case ChrSpecialization::DruidGuardian:
        case ChrSpecialization::MonkBrewmaster:
        case ChrSpecialization::DemonHunterVengeance:
            return true;
        default:
            return false;
    }
}

bool DungeonAutonomyManager::IsHealerRole(Player* player, Group* /*group*/) const
{
    if (!player)
        return false;

    // Use ChrSpecialization to determine healer role
    ChrSpecialization spec = player->GetPrimarySpecialization();
    switch (spec)
    {
        case ChrSpecialization::PriestDiscipline:
        case ChrSpecialization::PriestHoly:
        case ChrSpecialization::PaladinHoly:
        case ChrSpecialization::ShamanRestoration:
        case ChrSpecialization::DruidRestoration:
        case ChrSpecialization::MonkMistweaver:
        case ChrSpecialization::EvokerPreservation:
            return true;
        default:
            return false;
    }
}

Player* DungeonAutonomyManager::GetGroupTank(Group* group) const
{
    if (!group)
        return nullptr;

    for (auto const& memberSlot : group->GetMemberSlots())
    {
        Player* member = ObjectAccessor::FindPlayer(memberSlot.guid);
        if (member && member->IsInWorld() && IsTankRole(member, group))
        {
            return member;
        }
    }

    // No tank found - use leader
    return ObjectAccessor::FindPlayer(group->GetLeaderGUID());
}

Creature* DungeonAutonomyManager::GetNextPullTarget(Group* group, DungeonCoordinator* coordinator) const
{
    if (!coordinator)
        return nullptr;

    TrashPack* pack = coordinator->GetCurrentPullTarget();
    if (pack && !pack->members.empty())
    {
        // Get first creature from pack
        for (const ObjectGuid& guid : pack->members)
        {
            Creature* creature = ObjectAccessor::GetCreature(*GetGroupTank(const_cast<Group*>(group)), guid);
            if (creature && creature->IsAlive() && !creature->IsInCombat())
            {
                return creature;
            }
        }
    }

    return nullptr;
}

void DungeonAutonomyManager::MarkPullTarget(Creature* target)
{
    if (!target)
        return;

    // Set raid target icon (skull = 8)
    // This requires group leader permissions
    // For now, just log
    TC_LOG_DEBUG("module.playerbot.dungeon", "Would mark target {} with skull",
        target->GetName());
}

void DungeonAutonomyManager::LogStateTransition(Group* group, DungeonAutonomyState oldState, DungeonAutonomyState newState)
{
    static const char* stateNames[] = {
        "DISABLED", "PAUSED", "ACTIVE", "WAITING", "PULLING", "COMBAT", "RECOVERING"
    };

    TC_LOG_INFO("module.playerbot.dungeon", "Group {} autonomy state: {} -> {}",
        group ? group->GetGUID().GetCounter() : 0,
        stateNames[static_cast<int>(oldState)],
        stateNames[static_cast<int>(newState)]);
}

} // namespace Playerbot
