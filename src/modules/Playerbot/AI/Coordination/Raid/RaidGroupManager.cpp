/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 */

#include "RaidGroupManager.h"
#include "RaidCoordinator.h"
#include "Player.h"
#include "Log.h"

namespace Playerbot {

// ============================================================================
// ROLE DETECTION HELPERS
// ============================================================================

/**
 * Checks if a player has a tank specialization.
 */
static bool IsTankSpecialization(Player* player)
{
    if (!player)
        return false;

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

/**
 * Checks if a player has a healer specialization.
 */
static bool IsHealerSpecialization(Player* player)
{
    if (!player)
        return false;

    ChrSpecialization spec = player->GetPrimarySpecialization();

    switch (spec)
    {
        case ChrSpecialization::PriestDiscipline:
        case ChrSpecialization::PriestHoly:
        case ChrSpecialization::PaladinHoly:
        case ChrSpecialization::DruidRestoration:
        case ChrSpecialization::ShamanRestoration:
        case ChrSpecialization::MonkMistweaver:
        case ChrSpecialization::EvokerPreservation:
            return true;
        default:
            return false;
    }
}

// ============================================================================
// CONSTRUCTOR
// ============================================================================

RaidGroupManager::RaidGroupManager(RaidCoordinator* coordinator)
    : _coordinator(coordinator)
{
}

void RaidGroupManager::Initialize()
{
    Reset();
    InitializeGroups();
    TC_LOG_DEBUG("playerbots.raid", "RaidGroupManager::Initialize - Initialized with %u groups",
        GetGroupCount());
}

void RaidGroupManager::Update(uint32 /*diff*/)
{
    UpdateGroupComposition();
}

void RaidGroupManager::Reset()
{
    _subGroups.clear();
    _playerGroups.clear();
    _isSplit = false;
    _splitGroups = 1;
}

RaidSubGroup* RaidGroupManager::GetSubGroup(uint8 groupNum)
{
    if (groupNum >= _subGroups.size())
        return nullptr;
    return &_subGroups[groupNum];
}

const RaidSubGroup* RaidGroupManager::GetSubGroup(uint8 groupNum) const
{
    if (groupNum >= _subGroups.size())
        return nullptr;
    return &_subGroups[groupNum];
}

uint8 RaidGroupManager::GetPlayerGroup(ObjectGuid player) const
{
    auto it = _playerGroups.find(player);
    return it != _playerGroups.end() ? it->second : 0;
}

std::vector<ObjectGuid> RaidGroupManager::GetGroupMembers(uint8 groupNum) const
{
    const RaidSubGroup* group = GetSubGroup(groupNum);
    if (!group)
        return {};
    return group->members;
}

void RaidGroupManager::AssignToSubGroup(ObjectGuid player, uint8 groupNum)
{
    if (groupNum >= _subGroups.size())
        return;

    // Remove from old group
    RemoveFromSubGroup(player);

    // Add to new group
    _subGroups[groupNum].members.push_back(player);
    _playerGroups[player] = groupNum;

    TC_LOG_DEBUG("playerbots.raid", "RaidGroupManager::AssignToSubGroup - Player assigned to group %u", groupNum);
}

void RaidGroupManager::RemoveFromSubGroup(ObjectGuid player)
{
    auto it = _playerGroups.find(player);
    if (it == _playerGroups.end())
        return;

    uint8 oldGroup = it->second;
    if (oldGroup < _subGroups.size())
    {
        auto& members = _subGroups[oldGroup].members;
        members.erase(std::remove(members.begin(), members.end(), player), members.end());
    }

    _playerGroups.erase(player);
}

void RaidGroupManager::SwapPlayers(ObjectGuid player1, ObjectGuid player2)
{
    uint8 group1 = GetPlayerGroup(player1);
    uint8 group2 = GetPlayerGroup(player2);

    RemoveFromSubGroup(player1);
    RemoveFromSubGroup(player2);

    AssignToSubGroup(player1, group2);
    AssignToSubGroup(player2, group1);
}

void RaidGroupManager::SplitRaid(uint8 numGroups)
{
    _isSplit = true;
    _splitGroups = numGroups;
    TC_LOG_DEBUG("playerbots.raid", "RaidGroupManager::SplitRaid - Raid split into %u groups", numGroups);
}

void RaidGroupManager::MergeRaid()
{
    _isSplit = false;
    _splitGroups = 1;
    TC_LOG_DEBUG("playerbots.raid", "RaidGroupManager::MergeRaid - Raid merged");
}

uint8 RaidGroupManager::GetSplitGroup(ObjectGuid player) const
{
    if (!_isSplit)
        return 0;

    uint8 playerGroup = GetPlayerGroup(player);
    return playerGroup % _splitGroups;
}

void RaidGroupManager::OptimizeSubGroups()
{
    BalanceTanksAndHealers();
}

void RaidGroupManager::BalanceTanksAndHealers()
{
    // Ensure each group has at least one tank and healer where possible
    for (auto& group : _subGroups)
    {
        group.hasTank = false;
        group.hasHealer = false;
        group.meleeCount = 0;
        group.rangedCount = 0;

        for (ObjectGuid guid : group.members)
        {
            Player* player = _coordinator->GetPlayer(guid);
            if (!player)
                continue;

            if (IsTankSpecialization(player))
                group.hasTank = true;
            else if (IsHealerSpecialization(player))
                group.hasHealer = true;
        }
    }
}

bool RaidGroupManager::NeedsOptimization() const
{
    for (const auto& group : _subGroups)
    {
        if (!group.members.empty() && !group.hasTank && !group.hasHealer)
            return true;
    }
    return false;
}

uint32 RaidGroupManager::GetTotalMembers() const
{
    uint32 total = 0;
    for (const auto& group : _subGroups)
    {
        total += static_cast<uint32>(group.members.size());
    }
    return total;
}

void RaidGroupManager::InitializeGroups()
{
    _subGroups.resize(8);

    for (uint8 i = 0; i < 8; ++i)
    {
        _subGroups[i].groupNumber = i + 1;
    }

    // Distribute players evenly
    uint8 currentGroup = 0;
    for (ObjectGuid guid : _coordinator->GetAllMembers())
    {
        AssignToSubGroup(guid, currentGroup);
        currentGroup = (currentGroup + 1) % 8;
    }

    UpdateGroupComposition();
}

void RaidGroupManager::UpdateGroupComposition()
{
    for (auto& group : _subGroups)
    {
        group.hasTank = false;
        group.hasHealer = false;
        group.meleeCount = 0;
        group.rangedCount = 0;

        for (ObjectGuid guid : group.members)
        {
            Player* player = _coordinator->GetPlayer(guid);
            if (!player)
                continue;

            if (IsTankSpecialization(player))
                group.hasTank = true;
            else if (IsHealerSpecialization(player))
                group.hasHealer = true;
        }
    }
}

} // namespace Playerbot
