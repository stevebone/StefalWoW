/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * GroupMemberResolver - Implementation
 */

#include "GroupMemberResolver.h"
#include "GroupRoleEnums.h"
#include "../Core/Diagnostics/GroupMemberDiagnostics.h"
#include "../Session/BotWorldSessionMgr.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "Group.h"
#include "Log.h"
#include <algorithm>

namespace Playerbot
{

// =====================================================================
// CORE LOOKUP METHODS
// =====================================================================

Player* GroupMemberResolver::ResolveMember(ObjectGuid guid)
{
    if (guid.IsEmpty())
        return nullptr;

    // Method 1: ObjectAccessor::FindPlayer (fastest, same-map players)
    if (Player* player = ObjectAccessor::FindPlayer(guid))
        return player;

    // Method 2: ObjectAccessor::FindConnectedPlayer (connected players, any map)
    if (Player* player = ObjectAccessor::FindConnectedPlayer(guid))
        return player;

    // Method 3: BotWorldSessionMgr (bot registry - always works for bots)
    if (Player* bot = sBotWorldSessionMgr->GetPlayerBot(guid))
        return bot;

    // All methods failed
    TC_LOG_DEBUG("module.playerbot.group",
        "GroupMemberResolver: Failed to resolve GUID {} (all methods failed)",
        guid.ToString());
    
    return nullptr;
}

Player* GroupMemberResolver::ResolveMemberDiag(
    ObjectGuid guid,
    const char* callerFunc,
    const char* callerFile,
    uint32 callerLine)
{
    // Use diagnostics system if enabled
    if (sGroupMemberDiagnostics->IsEnabled())
    {
        return sGroupMemberDiagnostics->DiagnosticLookup(
            guid, callerFunc, callerFile, callerLine);
    }
    
    // Fall back to normal resolution
    return ResolveMember(guid);
}

// =====================================================================
// GROUP ITERATION METHODS
// =====================================================================

std::vector<Player*> GroupMemberResolver::GetGroupMembers(Group* group)
{
    std::vector<Player*> members;
    
    if (!group)
        return members;

    // Reserve space for typical group size
    members.reserve(group->GetMembersCount());

    // Use GetMemberSlots() for GUIDs, then resolve each
    for (auto const& slot : group->GetMemberSlots())
    {
        if (Player* member = ResolveMember(slot.guid))
            members.push_back(member);
    }

    return members;
}

std::vector<Player*> GroupMemberResolver::GetGroupMembersExcept(Group* group, Player* exclude)
{
    std::vector<Player*> members;
    
    if (!group)
        return members;

    members.reserve(group->GetMembersCount());

    for (auto const& slot : group->GetMemberSlots())
    {
        if (Player* member = ResolveMember(slot.guid))
        {
            if (member != exclude)
                members.push_back(member);
        }
    }

    return members;
}

std::vector<Player*> GroupMemberResolver::GetGroupMembersInRange(
    Player* player,
    float range,
    bool includeSelf)
{
    std::vector<Player*> members;
    
    if (!player)
        return members;

    Group* group = player->GetGroup();
    if (!group)
    {
        // Solo player - return self if requested
        if (includeSelf)
            members.push_back(player);
        return members;
    }

    float rangeSq = range * range;
    members.reserve(group->GetMembersCount());

    for (auto const& slot : group->GetMemberSlots())
    {
        Player* member = ResolveMember(slot.guid);
        if (!member)
            continue;

        // Check if this is self
        if (member == player)
        {
            if (includeSelf)
                members.push_back(member);
            continue;
        }

        // Check range (must be on same map)
        if (member->GetMapId() != player->GetMapId())
            continue;

        float distSq = player->GetExactDistSq(member);
        if (distSq <= rangeSq)
            members.push_back(member);
    }

    return members;
}

std::vector<Player*> GroupMemberResolver::GetGroupMembersFiltered(
    Group* group,
    std::function<bool(Player*)> predicate)
{
    std::vector<Player*> members;
    
    if (!group || !predicate)
        return members;

    members.reserve(group->GetMembersCount());

    for (auto const& slot : group->GetMemberSlots())
    {
        if (Player* member = ResolveMember(slot.guid))
        {
            if (predicate(member))
                members.push_back(member);
        }
    }

    return members;
}

// =====================================================================
// COMBAT-SPECIFIC HELPERS
// =====================================================================

bool GroupMemberResolver::IsGroupInCombat(Group* group, Player* excludePlayer)
{
    if (!group)
        return false;

    for (auto const& slot : group->GetMemberSlots())
    {
        Player* member = ResolveMember(slot.guid);
        if (!member || member == excludePlayer)
            continue;

        if (member->IsInCombat())
            return true;
    }

    return false;
}

Player* GroupMemberResolver::FindGroupMemberInCombat(Group* group, Player* excludePlayer)
{
    if (!group)
        return nullptr;

    for (auto const& slot : group->GetMemberSlots())
    {
        Player* member = ResolveMember(slot.guid);
        if (!member || member == excludePlayer)
            continue;

        if (member->IsInCombat())
            return member;
    }

    return nullptr;
}

std::vector<Player*> GroupMemberResolver::GetGroupMembersInCombat(Group* group)
{
    return GetGroupMembersFiltered(group, [](Player* p) {
        return p->IsInCombat();
    });
}

std::vector<Player*> GroupMemberResolver::GetGroupMembersNeedingHealing(
    Group* group,
    float healthThreshold)
{
    std::vector<Player*> needHealing;
    
    if (!group)
        return needHealing;

    needHealing.reserve(group->GetMembersCount());

    for (auto const& slot : group->GetMemberSlots())
    {
        Player* member = ResolveMember(slot.guid);
        if (!member || member->isDead())
            continue;

        if (member->GetHealthPct() < healthThreshold)
            needHealing.push_back(member);
    }

    // Sort by health (lowest first)
    std::sort(needHealing.begin(), needHealing.end(),
        [](Player* a, Player* b) {
            return a->GetHealthPct() < b->GetHealthPct();
        });

    return needHealing;
}

// =====================================================================
// ROLE-BASED HELPERS
// =====================================================================

std::vector<Player*> GroupMemberResolver::GetGroupTanks(Group* group)
{
    return GetGroupMembersFiltered(group, [](Player* p) {
        return IsPlayerTank(p);
    });
}

std::vector<Player*> GroupMemberResolver::GetGroupHealers(Group* group)
{
    return GetGroupMembersFiltered(group, [](Player* p) {
        return IsPlayerHealer(p);
    });
}

std::vector<Player*> GroupMemberResolver::GetGroupDPS(Group* group)
{
    return GetGroupMembersFiltered(group, [](Player* p) {
        return !IsPlayerTank(p) && !IsPlayerHealer(p);
    });
}

// =====================================================================
// UTILITY METHODS
// =====================================================================

bool GroupMemberResolver::IsBot(ObjectGuid guid)
{
    return sBotWorldSessionMgr->GetPlayerBot(guid) != nullptr;
}

bool GroupMemberResolver::IsBot(Player* player)
{
    if (!player)
        return false;
    return IsBot(player->GetGUID());
}

Player* GroupMemberResolver::GetGroupLeader(Group* group)
{
    if (!group)
        return nullptr;

    return ResolveMember(group->GetLeaderGUID());
}

uint32 GroupMemberResolver::GetResolvedMemberCount(Group* group)
{
    if (!group)
        return 0;

    uint32 count = 0;
    for (auto const& slot : group->GetMemberSlots())
    {
        if (ResolveMember(slot.guid))
            ++count;
    }

    return count;
}

} // namespace Playerbot
