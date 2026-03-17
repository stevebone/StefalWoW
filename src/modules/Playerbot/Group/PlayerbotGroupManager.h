/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include "Group.h"
#include "Player.h"
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <queue>
#include <atomic>
#include <mutex>
#include <chrono>

class Player;
class Group;

namespace Playerbot
{

enum class GroupRole : uint8
{
    TANK     = 0,
    HEALER   = 1,
    DPS      = 2,
    SUPPORT  = 3
};

enum class GroupFormationType : uint8
{
    RANDOM_DUNGEON   = 0,
    GUILD_GROUP      = 1,
    ZONE_BASED       = 2,
    LEVEL_RANGE      = 3,
    QUEST_BASED      = 4,
    ARENA_TEAM       = 5,
    RAID_GROUP       = 6,
    MANUAL           = 7
};

enum class GroupCoordinationMode : uint8
{
    LEADER_FOLLOW    = 0,  // Follow group leader
    FORMATION_BASED  = 1,  // Maintain formation
    OBJECTIVE_BASED  = 2,  // Focus on objectives
    ADAPTIVE         = 3   // Adapt to situation
};

struct GroupMemberInfo
{
    uint32 playerGuid;
    GroupRole role;
    uint32 level;
    uint8 classId;
    uint8 specId;
    float groupContribution;
    bool isOnline;
    bool isBot;
    Position lastKnownPosition;
    uint32 lastUpdateTime;

    GroupMemberInfo(uint32 guid, GroupRole r, uint32 lvl, uint8 cls, uint8 spec)
        : playerGuid(guid), role(r), level(lvl), classId(cls), specId(spec)
        , groupContribution(1.0f), isOnline(true), isBot(false)
        , lastUpdateTime(GameTime::GetGameTimeMS()) {}
};

struct GroupFormationData
{
    struct Position
    {
        float x, y, z;
        float distance;
        float angle;

        Position(float x_ = 0, float y_ = 0, float z_ = 0, float dist = 0, float ang = 0)
            : x(x_), y(y_), z(z_), distance(dist), angle(ang) {}
    };

    Position leaderOffset;
    ::std::unordered_map<GroupRole, Position> rolePositions;
    float maxFormationDistance;
    bool maintainFormation;

    GroupFormationData() : maxFormationDistance(15.0f), maintainFormation(true)
    {
        // Default formation positions
        rolePositions[GroupRole::TANK] = Position(0, 2, 0, 3.0f, 0);
        rolePositions[GroupRole::HEALER] = Position(0, -8, 0, 10.0f, static_cast<float>(M_PI));
        rolePositions[GroupRole::DPS] = Position(-3, -3, 0, 5.0f, static_cast<float>(M_PI) * 0.75f);
        rolePositions[GroupRole::SUPPORT] = Position(3, -3, 0, 5.0f, static_cast<float>(M_PI) * 0.25f);
    }
};

struct GroupObjective
{
    enum Type
    {
        KILL_TARGET     = 0,
        REACH_LOCATION  = 1,
        COMPLETE_QUEST  = 2,
        DEFEND_AREA     = 3,
        COLLECT_ITEM    = 4,
        ESCORT_NPC      = 5,
        SURVIVE_TIME    = 6
    };

    Type type;
    uint32 targetId;
    Position targetLocation;
    float completionRadius;
    uint32 timeLimit;
    uint32 priority;
    bool isCompleted;
    uint32 assignedTime;

    GroupObjective(Type t, uint32 id = 0, Position pos = Position(), float radius = 5.0f)
        : type(t), targetId(id), targetLocation(pos), completionRadius(radius)
        , timeLimit(0), priority(100), isCompleted(false), assignedTime(GameTime::GetGameTimeMS()) {}
};

class TC_GAME_API PlayerbotGroupManager final 
{
public:
    static PlayerbotGroupManager* instance();

    // IPlayerbotGroupManager interface implementation

    // Core group management
    bool CreateGroup(Player* leader, GroupFormationType type = GroupFormationType::MANUAL);
    bool AddMemberToGroup(uint32 groupId, Player* member, GroupRole preferredRole = GroupRole::DPS);
    bool RemoveMemberFromGroup(uint32 groupId, uint32 memberGuid);
    bool DisbandGroup(uint32 groupId);

    // Group finder and matching
    uint32 FindSuitableGroup(Player* player, GroupRole role);
    ::std::vector<uint32> FindMembersForGroup(uint32 groupId, GroupRole role, uint32 minLevel, uint32 maxLevel);
    bool CanJoinGroup(Player* player, uint32 groupId, GroupRole role);

    // Group coordination
    void UpdateGroupCoordination(uint32 groupId);
    void SetGroupObjective(uint32 groupId, GroupObjective const& objective);
    void UpdateGroupFormation(uint32 groupId, GroupFormationData const& formation);
    Position GetOptimalPositionForMember(uint32 groupId, uint32 memberGuid);

    // Leadership and decision making
    void AssignGroupLeader(uint32 groupId, uint32 newLeaderGuid);
    void HandleLeaderDisconnect(uint32 groupId);
    void MakeGroupDecision(uint32 groupId, ::std::string const& decision);

    // Combat coordination
    void OnCombatStart(uint32 groupId, Unit* target);
    void OnCombatEnd(uint32 groupId);
    void CoordinateGroupAttack(uint32 groupId, Unit* target);
    void HandleGroupThreat(uint32 groupId);

    // Movement and positioning
    void UpdateGroupMovement(uint32 groupId);
    void MoveGroupToLocation(uint32 groupId, Position const& destination);
    void FormationMove(uint32 groupId, Position const& destination);

    // Communication and chat
    void BroadcastToGroup(uint32 groupId, ::std::string const& message, ChatMsg type = CHAT_MSG_PARTY);
    void HandleGroupChat(uint32 groupId, Player* sender, ::std::string const& message);

    // Statistics and monitoring
    using GroupStatistics = Playerbot::GroupStatistics;

    GroupStatistics GetGroupStatistics(uint32 groupId);
    void UpdateGroupStatistics(uint32 groupId, GroupStatistics const& stats);

    // Automated group management
    void ProcessGroupQueue();
    void AutoFormGroups();
    void AutoDisbandInactiveGroups();
    void RebalanceGroups();

    // Configuration and settings
    void SetGroupCoordinationMode(uint32 groupId, GroupCoordinationMode mode);
    void EnableAutoGrouping(bool enable) override { _autoGroupingEnabled = enable; }
    void SetMaxGroupsPerMap(uint32 mapId, uint32 maxGroups);

    // Update and maintenance
    void Update(uint32 diff);
    void CleanupInactiveGroups();

private:
    PlayerbotGroupManager();
    ~PlayerbotGroupManager() = default;

    struct PlayerbotGroup
    {
        uint32 groupId;
        ObjectGuid leaderGuid;
        Group* coreGroup;
        GroupFormationType formationType;
        GroupCoordinationMode coordinationMode;
        ::std::vector<GroupMemberInfo> members;
        GroupFormationData formation;
        ::std::queue<GroupObjective> objectives;
        GroupObjective currentObjective;
        GroupStatistics statistics;
        uint32 creationTime;
        uint32 lastActivityTime;
        bool isActive;
        bool inCombat;
        Position lastKnownLeaderPos;

        PlayerbotGroup(uint32 id, Player* leader, GroupFormationType type)
            : groupId(id), leaderGuid(leader->GetGUID()), coreGroup(nullptr)
            , formationType(type), coordinationMode(GroupCoordinationMode::LEADER_FOLLOW)
            , currentObjective(GroupObjective::REACH_LOCATION)
            , creationTime(GameTime::GetGameTimeMS()), lastActivityTime(GameTime::GetGameTimeMS())
            , isActive(true), inCombat(false) {}
    };

    // Core data structures
    ::std::unordered_map<uint32, ::std::unique_ptr<PlayerbotGroup>> _groups;
    ::std::unordered_map<uint32, uint32> _playerToGroup; // player guid -> group id
    ::std::atomic<uint32> _nextGroupId{1};
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::GROUP_MANAGER> _groupsMutex;

    // Group formation queue
    struct GroupFormationRequest
    {
        uint32 playerGuid;
        GroupRole preferredRole;
        GroupFormationType formationType;
        uint32 minLevel;
        uint32 maxLevel;
        uint32 requestTime;

        GroupFormationRequest(uint32 guid, GroupRole role, GroupFormationType type, uint32 minLvl, uint32 maxLvl)
            : playerGuid(guid), preferredRole(role), formationType(type)
            , minLevel(minLvl), maxLevel(maxLvl), requestTime(GameTime::GetGameTimeMS()) {}
    };

    ::std::queue<GroupFormationRequest> _formationQueue;
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::GROUP_MANAGER> _queueMutex;

    // Configuration
    ::std::atomic<bool> _autoGroupingEnabled{true};
    ::std::unordered_map<uint32, uint32> _maxGroupsPerMap;

    // Helper functions
    uint32 GenerateGroupId() { return _nextGroupId++; }
    GroupRole DetermineOptimalRole(Player* player);
    bool IsGroupBalanced(const PlayerbotGroup& group);
    float CalculateGroupCompatibility(const ::std::vector<Player*>& players);
    void OptimizeGroupComposition(PlayerbotGroup& group);
    Position CalculateFormationPosition(const PlayerbotGroup& group, const GroupMemberInfo& member);
    void ExecuteGroupObjective(PlayerbotGroup& group);
    void HandleGroupMovementLogic(PlayerbotGroup& group);
    void UpdateCombatCoordination(PlayerbotGroup& group);
    void ProcessGroupCommunication(PlayerbotGroup& group);

    // Constants
    static constexpr uint32 MAX_GROUPS_GLOBAL = 1000;
    static constexpr uint32 GROUP_UPDATE_INTERVAL = 1000; // 1 second
    static constexpr uint32 FORMATION_UPDATE_INTERVAL = 500; // 0.5 seconds
    static constexpr uint32 INACTIVE_GROUP_TIMEOUT = 300000; // 5 minutes
    static constexpr float DEFAULT_FORMATION_DISTANCE = 15.0f;
    static constexpr float MIN_GROUP_EFFICIENCY = 0.3f;
    static constexpr uint32 QUEUE_PROCESSING_INTERVAL = 2000; // 2 seconds
    static constexpr uint32 MAX_GROUP_SIZE = 5;
    static constexpr uint32 MAX_RAID_SIZE = 25;
};

} // namespace Playerbot