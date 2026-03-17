/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "PlayerbotGroupScript.h"
#include "Core/Events/GenericEventBus.h"
#include "GroupEvents.h"
#include "Group.h"
#include "GroupMgr.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "Log.h"
#include "Core/PlayerBotHooks.h"
#include <algorithm>

namespace Playerbot
{

// Static member definitions moved to header with inline to fix DLL linkage (C2491)

// ============================================================================
// PLAYERBOTGROUPSCRIPT IMPLEMENTATION
// ============================================================================

PlayerbotGroupScript::PlayerbotGroupScript() : GroupScript("PlayerbotGroupScript")
{
    TC_LOG_INFO("playerbot", "PlayerbotGroupScript: Registered (using existing ScriptMgr hooks)");
}

PlayerbotGroupScript::~PlayerbotGroupScript()
{
    ::std::lock_guard lock(_groupStatesMutex);
    _groupStates.clear();
}

// ========================================================================
// SCRIPTMGR HOOK IMPLEMENTATIONS
// ========================================================================

void PlayerbotGroupScript::OnAddMember(Group* group, ObjectGuid guid)
{
    if (!group)
        return;

    // Publish MEMBER_JOINED event
    GroupEvent event = GroupEvent::MemberJoined(group->GetGUID(), guid);
    PublishEvent(event);

    // Initialize or update group state for polling
    ::std::lock_guard lock(_groupStatesMutex);
    GroupState& state = GetOrCreateGroupState(group->GetGUID());

    // Update member subgroup tracking
    if (Player* player = ObjectAccessor::FindConnectedPlayer(guid))
        state.memberSubgroups[guid] = player->GetSubGroup();

    TC_LOG_DEBUG("playerbot.group", "PlayerbotGroupScript::OnAddMember: Group {} added member {}",
        group->GetGUID().ToString(), guid.ToString());
}

void PlayerbotGroupScript::OnInviteMember(Group* group, ObjectGuid guid)
{
    if (!group)
        return;

    // Note: OnInviteMember is called when invite is SENT, not when member joins
    // Actual join is handled by OnAddMember
    // We could publish an INVITE_SENT event if needed, but for now we skip it

    TC_LOG_DEBUG("playerbot.group", "PlayerbotGroupScript::OnInviteMember: Group {} invited {}",
        group->GetGUID().ToString(), guid.ToString());
}

void PlayerbotGroupScript::OnRemoveMember(Group* group, ObjectGuid guid, RemoveMethod method, ObjectGuid kicker, char const* reason)
{
    if (!group)
        return;

    // Publish MEMBER_LEFT event
    GroupEvent event = GroupEvent::MemberLeft(group->GetGUID(), guid, static_cast<uint32>(method));
    PublishEvent(event);

    // Update group state
    ::std::lock_guard lock(_groupStatesMutex);
    auto stateItr = _groupStates.find(group->GetGUID());
    if (stateItr != _groupStates.end())
    {
        // Remove member from subgroup tracking
        stateItr->second.memberSubgroups.erase(guid);
    }

    TC_LOG_DEBUG("playerbot.group", "PlayerbotGroupScript::OnRemoveMember: Group {} removed member {} (method: {}, kicker: {}, reason: {})",
        group->GetGUID().ToString(), guid.ToString(), static_cast<uint32>(method),
        kicker.ToString(), reason ? reason : "none");
}

void PlayerbotGroupScript::OnChangeLeader(Group* group, ObjectGuid newLeaderGuid, ObjectGuid oldLeaderGuid)
{
    if (!group)
        return;

    // Publish LEADER_CHANGED event
    GroupEvent event = GroupEvent::LeaderChanged(group->GetGUID(), newLeaderGuid);
    PublishEvent(event);

    TC_LOG_DEBUG("playerbot.group", "PlayerbotGroupScript::OnChangeLeader: Group {} leader changed from {} to {}",
        group->GetGUID().ToString(), oldLeaderGuid.ToString(), newLeaderGuid.ToString());
}

void PlayerbotGroupScript::OnDisband(Group* group)
{
    if (!group)
        return;

    // Publish GROUP_DISBANDED event
    GroupEvent event = GroupEvent::GroupDisbanded(group->GetGUID());
    PublishEvent(event);

    // Clean up group state
    RemoveGroupState(group->GetGUID());

    TC_LOG_DEBUG("playerbot.group", "PlayerbotGroupScript::OnDisband: Group {} disbanded",
        group->GetGUID().ToString());
}

// ========================================================================
// POLLING IMPLEMENTATION
// ========================================================================

void PlayerbotGroupScript::PollGroupStateChanges(Group* group, uint32 diff)
{
    if (!group)
        return;

    auto startTime = ::std::chrono::high_resolution_clock::now();

    ::std::lock_guard lock(_groupStatesMutex);
    GroupState& state = GetOrCreateGroupState(group->GetGUID());

    // Check all polled state changes
    CheckLootMethodChange(group, state);
    CheckLootThresholdChange(group, state);
    CheckMasterLooterChange(group, state);
    // NOTE: Target icon polling removed - Group::GetTargetIcon() is not accessible (private)
    CheckDifficultyChanges(group, state);
    CheckRaidConversion(group, state);
    CheckReadyCheckState(group, state);
    CheckSubgroupChanges(group, state);

    // Update statistics
    auto endTime = ::std::chrono::high_resolution_clock::now();
    auto duration = ::std::chrono::duration_cast<::std::chrono::microseconds>(endTime - startTime);

    ::std::lock_guard statsLock(_pollStatsMutex);
    _pollStats.totalPolls++;

    // Running average of poll time
    if (_pollStats.totalPolls == 1)
        _pollStats.averagePollTimeUs = duration.count();
    else
        _pollStats.averagePollTimeUs = (_pollStats.averagePollTimeUs * (_pollStats.totalPolls - 1) + duration.count()) / _pollStats.totalPolls;
}

void PlayerbotGroupScript::CheckLootMethodChange(Group* group, GroupState& state)
{
    uint8 currentMethod = static_cast<uint8>(group->GetLootMethod());
    if (currentMethod != state.lootMethod)
    {
        GroupEvent event = GroupEvent::LootMethodChanged(group->GetGUID(), currentMethod);
        PublishEvent(event);

        state.lootMethod = currentMethod;

        ::std::lock_guard lock(_pollStatsMutex);
        _pollStats.eventsDetected++;

        TC_LOG_DEBUG("playerbot.group", "PlayerbotGroupScript::CheckLootMethodChange: Group {} loot method changed to {}",
            group->GetGUID().ToString(), currentMethod);
    }
}

void PlayerbotGroupScript::CheckLootThresholdChange(Group* group, GroupState& state)
{
    uint8 currentThreshold = static_cast<uint8>(group->GetLootThreshold());
    if (currentThreshold != state.lootThreshold)
    {
        // Publish event to bus
        GroupEvent event;
        event.type = GroupEventType::LOOT_THRESHOLD_CHANGED;
        event.priority = EventPriority::MEDIUM;
        event.groupGuid = group->GetGUID();
        event.data1 = currentThreshold;
        event.timestamp = ::std::chrono::steady_clock::now();
        event.expiryTime = event.timestamp + ::std::chrono::seconds(30);
        PublishEvent(event);

        state.lootThreshold = currentThreshold;

        ::std::lock_guard lock(_pollStatsMutex);
        _pollStats.eventsDetected++;

        TC_LOG_DEBUG("playerbot.group", "PlayerbotGroupScript::CheckLootThresholdChange: Group {} loot threshold changed to {}",
            group->GetGUID().ToString(), currentThreshold);
    }
}

void PlayerbotGroupScript::CheckMasterLooterChange(Group* group, GroupState& state)
{
    ObjectGuid currentMasterLooter = group->GetMasterLooterGuid();
    if (currentMasterLooter != state.masterLooterGuid)
    {
        // Publish event to bus
        GroupEvent event;
        event.type = GroupEventType::MASTER_LOOTER_CHANGED;
        event.priority = EventPriority::MEDIUM;
        event.groupGuid = group->GetGUID();
        event.targetGuid = currentMasterLooter;
        event.timestamp = ::std::chrono::steady_clock::now();
        event.expiryTime = event.timestamp + ::std::chrono::seconds(30);
        PublishEvent(event);

        state.masterLooterGuid = currentMasterLooter;

        ::std::lock_guard lock(_pollStatsMutex);
        _pollStats.eventsDetected++;

        TC_LOG_DEBUG("playerbot.group", "PlayerbotGroupScript::CheckMasterLooterChange: Group {} master looter changed to {}",
            group->GetGUID().ToString(), currentMasterLooter.ToString());
    }
}

// NOTE: CheckTargetIconChanges() removed - Group::GetTargetIcon() is not accessible
// TrinityCore's Group class has m_targetIcons[] as private member with no public getter
// Options to restore:
//   1. Add Group::GetTargetIcon(uint8) accessor to TrinityCore (requires core modification)
//   2. Use packet sniffing for SMSG_RAID_TARGET_UPDATE
//   3. Accept limited event coverage (no target icon events)

void PlayerbotGroupScript::CheckDifficultyChanges(Group* group, GroupState& state)
{
    uint8 currentDungeon = static_cast<uint8>(group->GetDungeonDifficultyID());
    uint8 currentRaid = static_cast<uint8>(group->GetRaidDifficultyID());
    uint8 currentLegacy = static_cast<uint8>(group->GetLegacyRaidDifficultyID());

    bool changed = false;

    if (currentDungeon != state.dungeonDifficulty)
    {
        GroupEvent event = GroupEvent::DifficultyChanged(group->GetGUID(), currentDungeon);
        PublishEvent(event);
        state.dungeonDifficulty = currentDungeon;
        changed = true;
    }

    if (currentRaid != state.raidDifficulty)
    {
        GroupEvent event = GroupEvent::DifficultyChanged(group->GetGUID(), currentRaid);
        PublishEvent(event);
        state.raidDifficulty = currentRaid;
        changed = true;
    }

    if (currentLegacy != state.legacyRaidDifficulty)
    {
        GroupEvent event = GroupEvent::DifficultyChanged(group->GetGUID(), currentLegacy);
        PublishEvent(event);
        state.legacyRaidDifficulty = currentLegacy;
        changed = true;
    }

    if (changed)
    {
        ::std::lock_guard lock(_pollStatsMutex);
        _pollStats.eventsDetected++;

        TC_LOG_DEBUG("playerbot.group", "PlayerbotGroupScript::CheckDifficultyChanges: Group {} difficulty changed",
            group->GetGUID().ToString());
    }
}

void PlayerbotGroupScript::CheckRaidConversion(Group* group, GroupState& state)
{
    bool currentIsRaid = group->isRaidGroup();
    if (currentIsRaid != state.isRaid)
    {
        // Publish RAID_CONVERTED event
        GroupEvent event;
        event.type = GroupEventType::RAID_CONVERTED;
        event.priority = EventPriority::HIGH;
        event.groupGuid = group->GetGUID();
        event.data1 = currentIsRaid ? 1 : 0;
        event.timestamp = ::std::chrono::steady_clock::now();
        event.expiryTime = event.timestamp + ::std::chrono::seconds(30);
        PublishEvent(event);

        state.isRaid = currentIsRaid;

        ::std::lock_guard lock(_pollStatsMutex);
        _pollStats.eventsDetected++;

        TC_LOG_INFO("playerbot.group", "PlayerbotGroupScript::CheckRaidConversion: Group {} converted to {}",
            group->GetGUID().ToString(), currentIsRaid ? "RAID" : "PARTY");
    }
}

void PlayerbotGroupScript::CheckReadyCheckState(Group* group, GroupState& state)
{
    // Note: Group class doesn't expose ready check state publicly
    // This is a limitation of the polling approach
    // We would need Group::IsReadyCheckActive() or similar

    // For now, we skip ready check polling
    // Alternative: Track ready check via packet sniffing or add minimal Group.h accessor
}

void PlayerbotGroupScript::CheckSubgroupChanges(Group* group, GroupState& state)
{
    bool anyChanged = false;

    for (auto const& memberSlot : group->GetMemberSlots())
    {
        ObjectGuid memberGuid = memberSlot.guid;
        uint8 currentSubgroup = memberSlot.group;

        auto itr = state.memberSubgroups.find(memberGuid);
        if (itr == state.memberSubgroups.end())
        {
            // New member (shouldn't happen here, handled by OnAddMember)
            state.memberSubgroups[memberGuid] = currentSubgroup;
        }
        else if (itr->second != currentSubgroup)
        {
            // Subgroup changed
            GroupEvent event;
            event.type = GroupEventType::SUBGROUP_CHANGED;
            event.priority = EventPriority::MEDIUM;
            event.groupGuid = group->GetGUID();
            event.targetGuid = memberGuid;
            event.data1 = currentSubgroup;
            event.timestamp = ::std::chrono::steady_clock::now();
            event.expiryTime = event.timestamp + ::std::chrono::seconds(30);
            PublishEvent(event);

            itr->second = currentSubgroup;
            anyChanged = true;

            TC_LOG_DEBUG("playerbot.group", "PlayerbotGroupScript::CheckSubgroupChanges: Group {} member {} moved to subgroup {}",
                group->GetGUID().ToString(), memberGuid.ToString(), currentSubgroup);
        }
    }

    if (anyChanged)
    {
        ::std::lock_guard lock(_pollStatsMutex);
        _pollStats.eventsDetected++;
    }
}

// ========================================================================
// STATE MANAGEMENT
// ========================================================================

PlayerbotGroupScript::GroupState& PlayerbotGroupScript::GetOrCreateGroupState(ObjectGuid groupGuid)
{
    // Caller must hold _groupStatesMutex
    auto itr = _groupStates.find(groupGuid);
    if (itr != _groupStates.end())
        return itr->second;

    // Create new state
    GroupState newState;
    newState.lastUpdate = ::std::chrono::steady_clock::now();

    auto [insertItr, inserted] = _groupStates.emplace(groupGuid, ::std::move(newState));
    return insertItr->second;
}

void PlayerbotGroupScript::RemoveGroupState(ObjectGuid groupGuid)
{
    ::std::lock_guard lock(_groupStatesMutex);
    _groupStates.erase(groupGuid);
}

void PlayerbotGroupScript::InitializeGroupState(Group* group, GroupState& state)
{
    if (!group)
        return;

    // Initialize loot settings
    state.lootMethod = static_cast<uint8>(group->GetLootMethod());
    state.lootThreshold = static_cast<uint8>(group->GetLootThreshold());
    state.masterLooterGuid = group->GetMasterLooterGuid();

    // NOTE: Target icon initialization removed - Group::GetTargetIcon() is not accessible

    // Initialize difficulty (WoW 12.0: Difficulty is int16)
    state.dungeonDifficulty = static_cast<int16>(group->GetDungeonDifficultyID());
    state.raidDifficulty = static_cast<int16>(group->GetRaidDifficultyID());
    state.legacyRaidDifficulty = static_cast<int16>(group->GetLegacyRaidDifficultyID());

    // Initialize raid status
    state.isRaid = group->isRaidGroup();

    // Initialize member subgroups
    for (auto const& memberSlot : group->GetMemberSlots())
        state.memberSubgroups[memberSlot.guid] = memberSlot.group;

    state.lastUpdate = ::std::chrono::steady_clock::now();
}

// ========================================================================
// EVENT PUBLISHING
// ========================================================================

/*static*/ void PlayerbotGroupScript::PublishEvent(GroupEvent const& event)
{
    EventBus<GroupEvent>::instance()->PublishEvent(event);
}

// ========================================================================
// STATISTICS
// ========================================================================

// PollStatistics methods now defined inline in header

// ============================================================================
// PLAYERBOTWORLDSCRIPT IMPLEMENTATION
// ============================================================================

PlayerbotWorldScript::PlayerbotWorldScript() : WorldScript("PlayerbotWorldScript")
{
    TC_LOG_INFO("playerbot", "PlayerbotWorldScript: Registered (for group state polling)");
}

PlayerbotWorldScript::~PlayerbotWorldScript()
{
}

void PlayerbotWorldScript::OnUpdate(uint32 diff)
{
    // NOTE: Global group polling removed - GroupMgr::GetGroupMap() is not accessible
    // TrinityCore's GroupMgr has GroupStore as protected member with no public iteration method

    // Options to restore polling:
    //   1. OPTION A (Module-Only): Call PollGroupStateChanges() from BotAI::Update() for each bot's group
    //      - Each bot polls only its own group (not all groups)
    //      - Fully module-only approach
    //
    //   2. OPTION B (Core Modification): Add GroupMgr::GetGroupMap() accessor
    //      - Requires modifying TrinityCore GroupMgr.h (violates CLAUDE.md)
    //
    //   3. OPTION C (Hooks Only): Use only the 5 ScriptMgr hooks (OnAddMember, OnRemoveMember, etc.)
    //      - Limited event coverage (5 out of 24 events)
    //      - Zero polling overhead

    // Current choice: OPTION A will be implemented when BotAI is available
    // For now, polling is disabled - only ScriptMgr hooks are active
}

bool PlayerbotWorldScript::GroupHasBots(Group const* group)
{
    if (!group)
        return false;

    // Check if any member is a bot
    for (auto const& memberSlot : group->GetMemberSlots())
    {
        if (Player const* player = ObjectAccessor::FindConnectedPlayer(memberSlot.guid))
        {
            if (Playerbot::PlayerBotHooks::IsPlayerBot(player))
                return true;
        }
    }

    return false; // No bots found
}

} // namespace Playerbot

// ============================================================================
// SCRIPT REGISTRATION
// ============================================================================

void AddSC_PlayerbotGroupScripts()
{
    new Playerbot::PlayerbotGroupScript();
    new Playerbot::PlayerbotWorldScript();
}
