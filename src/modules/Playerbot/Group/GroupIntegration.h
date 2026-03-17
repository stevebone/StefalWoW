/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Group.h"
#include "Threading/LockHierarchy.h"
#include "PlayerbotGroupManager.h"
#include "Player.h"

namespace Playerbot
{

/**
 * @brief Integration layer between TrinityCore's Group system and Playerbot coordination
 *
 * This class bridges the core TrinityCore Group functionality with enhanced
 * playerbot coordination features. It works WITH the existing group system,
 * not against it.
 */
class TC_GAME_API GroupIntegration
{
public:
    // Core integration with TrinityCore Group system
    static void OnGroupCreated(Group* group);
    static void OnGroupDisbanded(Group* group);
    static void OnMemberAdded(Group* group, Player* member);
    static void OnMemberRemoved(Group* group, Player* member);
    static void OnLeaderChanged(Group* group, Player* newLeader);

    // Enhanced functionality for mixed groups (humans + bots)
    static bool IsMixedGroup(Group* group);
    static bool IsFullBotGroup(Group* group);
    static uint32 GetBotCount(Group* group);
    static uint32 GetHumanCount(Group* group);

    // Bot-specific group operations
    static bool AddBotToExistingGroup(Group* group, uint32 botGuid, GroupRole preferredRole);
    static void RemoveBotFromGroup(Group* group, uint32 botGuid);
    static void HandleBotGroupInvite(Player* inviter, Player* botPlayer);

    // Regular group quest/elite quest support
    static void HandleQuestSharing(Group* group, uint32 questId, Player* questGiver);
    static void HandleEliteQuestCoordination(Group* group, uint32 questId);
    static void HandleDungeonGroupFormation(Group* group, uint32 dungeonId);

    // Group coordination enhancement
    static void EnableCoordinationForGroup(Group* group, GroupCoordinationMode mode = GroupCoordinationMode::ADAPTIVE);
    static void DisableCoordinationForGroup(Group* group);
    static void UpdateGroupCoordination(Group* group);

    // Regular group scenarios
    struct RegularGroupScenario
    {
        enum Type
        {
            QUEST_GROUP         = 0,  // 2-5 players doing regular quests
            ELITE_QUEST_GROUP   = 1,  // 3-5 players for elite quests
            DUNGEON_GROUP       = 2,  // 5 players for dungeon
            WORLD_GROUP         = 3,  // Casual grouping for world content
            MIXED_HUMAN_BOT     = 4   // Humans + bots together
        };

        Type scenarioType;
        Group* coreGroup;
        ::std::vector<uint32> objectives;
        Position meetingPoint;
        uint32 estimatedDuration;
        bool requiresCoordination;

        RegularGroupScenario(Type type, Group* group)
            : scenarioType(type), coreGroup(group), estimatedDuration(3600000) // 1 hour default
            , requiresCoordination(type != WORLD_GROUP) {}
    };

    // Scenario management
    static void SetupRegularGroupScenario(Group* group, RegularGroupScenario::Type type);
    static void HandleScenarioProgress(Group* group, uint32 objectiveId);
    static void CompleteScenario(Group* group);

private:
    // Integration data
    static ::std::unordered_map<uint32, uint32> _groupToPlayerbotGroup; // core group id -> playerbot group id
    static ::std::unordered_map<uint32, RegularGroupScenario> _groupScenarios;
    static Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::GROUP_MANAGER> _integrationMutex;

    // Helper functions
    static uint32 CreatePlayerbotGroupForCoreGroup(Group* coreGroup);
    static void SyncGroupMembership(Group* coreGroup, uint32 playbotGroupId);
    static GroupRole DetermineRoleFromClass(uint8 classId, uint8 spec);
    static void UpdateGroupObjectives(Group* group, const RegularGroupScenario& scenario);
};

/**
 * @brief Hook integration points for TrinityCore Group events
 *
 * These hooks are registered with TrinityCore's Group system to automatically
 * enhance groups with playerbot coordination when appropriate.
 */
class GroupHooks
{
public:
    // TrinityCore Group event hooks
    static void RegisterGroupHooks();

    // Hook implementations
    static void Hook_OnGroupCreate(Group* group, Player* leader);
    static void Hook_OnGroupDisband(Group* group);
    static void Hook_OnAddMember(Group* group, Player* player);
    static void Hook_OnRemoveMember(Group* group, ObjectGuid playerGuid);
    static void Hook_OnGroupUpdate(Group* group, uint32 diff);

private:
    static bool _hooksRegistered;
};

/**
 * @brief Enhanced group functionality specifically for quest groups
 *
 * Provides smart coordination for common group scenarios:
 * - Regular quest groups (2-3 players helping each other)
 * - Elite quest groups (3-5 players for challenging content)
 * - Dungeon groups (5 players with defined roles)
 */
class QuestGroupCoordination
{
public:
    // Quest group formation
    static bool FormQuestGroup(Player* initiator, uint32 questId, uint32 maxMembers = 5);
    static bool JoinQuestGroup(Player* player, uint32 questId);
    static void DisbandQuestGroup(uint32 questId);

    // Quest sharing and coordination
    static void ShareQuestWithGroup(Group* group, uint32 questId, Player* questGiver);
    static void CoordinateQuestObjectives(Group* group, uint32 questId);
    static void HandleQuestTurnIn(Group* group, uint32 questId);

    // Elite quest specific functionality
    static void SetupEliteQuestStrategy(Group* group, uint32 questId);
    static void HandleEliteQuestCombat(Group* group, Unit* eliteTarget);
    static void AdaptToEliteQuestChallenges(Group* group);

    // Smart member recruitment for quest groups
    static ::std::vector<Player*> FindSuitableMembersForQuest(uint32 questId, uint32 requesterLevel);
    static bool InviteBotForQuest(Group* group, uint32 questId, GroupRole neededRole);

private:
    struct QuestGroupData
    {
        uint32 questId;
        Group* group;
        ::std::vector<Position> questObjectiveLocations;
        ::std::vector<uint32> requiredKills;
        ::std::vector<uint32> requiredItems;
        uint32 estimatedCompletionTime;
        bool isEliteQuest;
        bool requiresSpecificRoles;

        QuestGroupData(uint32 qId, Group* g) : questId(qId), group(g), estimatedCompletionTime(1800000) // 30 min
            , isEliteQuest(false), requiresSpecificRoles(false) {}
    };

    static ::std::unordered_map<uint32, QuestGroupData> _activeQuestGroups; // questId -> data
    static Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::GROUP_MANAGER> _questGroupMutex;
};

} // namespace Playerbot

/**
 * INTEGRATION SUMMARY:
 *
 * Regular groups in TrinityCore work exactly as before:
 * 1. Players use /invite to create groups (max 5 members)
 * 2. Core Group class handles membership, loot, XP sharing
 * 3. Groups can promote to raids (max 40 members)
 * 4. Standard group features work unchanged
 *
 * Playerbot enhancement adds:
 * 1. Optional coordination layer for mixed human/bot groups
 * 2. Smart bot recruitment for quest groups
 * 3. Enhanced coordination for elite quests and dungeons
 * 4. Automatic strategy adaptation based on group composition
 *
 * Key Integration Points:
 * - Hook into Group creation/destruction events
 * - Extend existing groups with coordination features
 * - Maintain full compatibility with existing Group functionality
 * - Allow humans and bots to group together naturally
 *
 * Example Scenarios:
 * 1. Human creates group, invites 2 other humans + 2 bots for dungeon
 * 2. Human needs help with elite quest, system finds suitable bots
 * 3. Mixed group gets enhanced coordination for challenging content
 * 4. All standard group features (chat, loot, XP) work normally
 */