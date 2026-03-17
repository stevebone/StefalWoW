/*
 * Copyright (C) 2024 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef TRINITYCORE_BOT_GROUP_COORDINATOR_H
#define TRINITYCORE_BOT_GROUP_COORDINATOR_H

#include "Define.h"
#include "ObjectGuid.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include <chrono>

class Player;
class Group;
class Unit;
class Creature;
enum Difficulty : int16;

namespace Playerbot
{
    // Forward declare GroupEvent and BotAI from Playerbot namespace
    struct GroupEvent;
    class BotAI;  // Forward declaration in Playerbot namespace

namespace Advanced
{
    // BotAI is in parent Playerbot namespace, not Advanced namespace
    using Playerbot::BotAI;
    class TacticalCoordinator;

    /**
     * GroupCoordinator - Advanced group and raid coordination for PlayerBots
     *
     * Handles all group/raid mechanics including:
     * - Group formation and role assignment
     * - Raid coordination and boss strategies
     * - Loot distribution and rolling
     * - Group quest sharing and completion
     * - Tactical coordination (via TacticalCoordinator subsystem)
     *
     * ARCHITECTURE:
     * - Per-bot instance managing this bot's group participation
     * - Uses shared TacticalCoordinator for cross-bot tactical decisions
     * - Integrates with BotAI event system
     *
     * SUBSYSTEMS:
     * - TacticalCoordinator: Handles interrupt/dispel/focus coordination (shared)
     * - Social coordination: Invites, quests, loot (per-bot)
     * - Raid mechanics: Boss strategies, positioning (per-bot)
     */
    class GroupCoordinator
    {
    public:
        explicit GroupCoordinator(Player* bot, BotAI* ai);
        ~GroupCoordinator();

        // Core lifecycle
        void Initialize();
        void Update(uint32 diff);
        void Reset();
        void Shutdown();

        // Group management
        bool JoinGroup(Group* group);
        bool LeaveGroup();
        bool InviteToGroup(Player* player);
        bool AcceptGroupInvite(Player* inviter);
        bool DeclineGroupInvite(Player* inviter);
        Group* GetGroup() const;
        bool IsInGroup() const;

        // Role management
        enum class GroupRole : uint8
        {
            TANK,
            HEALER,
            DPS_MELEE,
            DPS_RANGED,
            SUPPORT,
            UNDEFINED
        };

        GroupRole DetermineRole() const;
        void SetRole(GroupRole role) { m_assignedRole = role; }
        GroupRole GetRole() const { return m_assignedRole; }
        bool CanFillRole(GroupRole role) const;

        // Raid coordination
        bool IsInRaid() const;
        uint32 GetRaidSize() const;
        uint32 GetGroupSize() const;
        Player* GetLeader() const;

        // Role queries
        bool IsTank() const { return GetRole() == GroupRole::TANK; }
        bool IsHealer() const { return GetRole() == GroupRole::HEALER; }
        bool IsDPS() const
        {
            return GetRole() == GroupRole::DPS_MELEE || GetRole() == GroupRole::DPS_RANGED;
        }

        // Combat queries
        bool IsInCombat() const;
        bool IsRaidLeader() const;
        bool AssignRaidRole(Player* target, GroupRole role);
        void CoordinateRaidPositions();
        void ExecuteBossStrategy(Creature* boss);

        // Loot management
        enum class LootDecision : uint8
        {
            PASS,
            NEED,
            GREED,
            DISENCHANT
        };

        LootDecision DecideLootRoll(uint32 itemId);
        bool RollForLoot(uint32 itemId, LootDecision decision);
        void ConfigureLootSettings(Group* group);
        bool NeedItem(uint32 itemId) const;
        bool CanGreedItem(uint32 itemId) const;

        // Quest coordination
        bool ShareQuest(uint32 questId);
        bool AcceptSharedQuest(uint32 questId, Player* sharer);
        void SyncGroupQuests();
        ::std::vector<uint32> GetShareableQuests() const;

        // Group composition
        struct GroupComposition
        {
            uint32 tanks = 0;
            uint32 healers = 0;
            uint32 dps = 0;
            uint32 total = 0;
            bool isBalanced = false;
            bool canRaid = false;
        };

        GroupComposition AnalyzeGroupComposition() const;
        bool IsGroupBalanced() const;
        GroupRole GetNeededRole() const;
        ::std::vector<Player*> GetGroupMembers() const;

        // Ready checks
        bool PerformReadyCheck();
        bool RespondToReadyCheck(bool ready);
        bool IsGroupReady() const;
        void WaitForGroupReady();

        // Dungeon/Raid finder
        bool QueueForDungeon(uint32 dungeonId);
        bool QueueForRaid(uint32 raidId);
        bool AcceptDungeonInvite();
        void LeaveDungeonQueue();

        // Combat coordination
        void AssignTarget(Unit* target);
        void FocusTarget(Unit* target);
        Unit* GetGroupTarget() const;
        void CoordinateCrowdControl(Unit* target);
        void CallForHelp(Unit* attacker);

        // Tactical coordination (delegates to TacticalCoordinator)
        TacticalCoordinator* GetTacticalCoordinator() const { return m_tacticalCoordinator.get(); }
        void RequestInterrupt(ObjectGuid targetGuid);
        void RequestDispel(ObjectGuid targetGuid);
        bool IsGroupCooldownAvailable(std::string const& cooldownName) const;
        void UseGroupCooldown(std::string const& cooldownName, uint32 durationMs);

        // Resurrection and recovery
        void RequestResurrection();
        bool OfferResurrection(Player* target);
        void CoordinateGroupRecovery();
        bool ShouldWaitForGroup() const;

        // Group event handlers (called from BotAI_EventHandlers.cpp)
        void OnTargetIconChanged(struct GroupEvent const& event);
        void OnGroupCompositionChanged(struct GroupEvent const& event);

        // Configuration
        bool IsEnabled() const { return m_enabled; }
        void SetEnabled(bool enabled) { m_enabled = enabled; }

        void SetAutoAcceptInvites(bool enable) { m_autoAcceptInvites = enable; }
        void SetAutoShareQuests(bool enable) { m_autoShareQuests = enable; }
        void SetFollowGroupStrategy(bool enable) { m_followGroupStrategy = enable; }
        void SetSmartLootRolls(bool enable) { m_smartLootRolls = enable; }

        // Statistics
        struct Statistics
        {
            uint32 groupsJoined = 0;
            uint32 raidsCompleted = 0;
            uint32 dungeonsCompleted = 0;
            uint32 questsShared = 0;
            uint32 lootRolls = 0;
            uint32 lootWon = 0;
            uint32 resurrectionsGiven = 0;
            uint32 resurrectionsReceived = 0;
        };

        Statistics const& GetStatistics() const { return m_stats; }

        // Performance metrics
        float GetCPUUsage() const { return m_cpuUsage; }
        size_t GetMemoryUsage() const;

    private:
        // Group state management
        enum class GroupState : uint8
        {
            IDLE,
            FORMING,
            ACTIVE,
            IN_COMBAT,
            DUNGEON,
            RAID,
            DISBANDING
        };

        GroupState m_currentState;
        void UpdateGroupState(uint32 diff);
        void TransitionToState(GroupState newState);

        // Role analysis
        struct RoleCapability
        {
            GroupRole role;
            float suitability;
            bool canPerform;
        };

        ::std::vector<RoleCapability> AnalyzeRoleCapabilities() const;
        GroupRole GetBestRole() const;
        bool HasTankingAbilities() const;
        bool HasHealingAbilities() const;

        // Loot evaluation
        struct LootPriority
        {
            uint32 itemId;
            float priority;
            LootDecision decision;
            bool isUpgrade;
            bool isTransmog;
        };

        LootPriority EvaluateLootItem(uint32 itemId) const;
        float CalculateItemValue(uint32 itemId) const;
        bool IsItemUpgrade(uint32 itemId) const;

        // Quest sharing logic
        struct ShareableQuest
        {
            uint32 questId;
            uint32 membersWithQuest;
            uint32 membersCanAccept;
            float sharePriority;
        };

        ::std::vector<ShareableQuest> EvaluateQuestsToShare() const;
        bool ShouldShareQuest(uint32 questId) const;
        bool CanMemberAcceptQuest(Player* member, uint32 questId) const;

        // Raid strategy
        struct BossStrategy
        {
            uint32 bossEntry;
            ::std::string strategyName;
            ::std::vector<::std::string> phases;
            ::std::unordered_map<::std::string, ::std::string> assignments;
        };

        BossStrategy* GetBossStrategy(uint32 bossEntry);
        void LoadBossStrategies();
        void ExecutePhaseStrategy(BossStrategy const* strategy, uint32 phase);

        // Group finder logic
        struct QueueInfo
        {
            uint32 dungeonId;
            uint32 queueTime;
            uint32 estimatedWait;
            bool isQueued;
        };

        QueueInfo m_queueInfo;
        void UpdateQueueStatus();

        // Invitation tracking
        struct PendingInvite
        {
            ObjectGuid inviter;
            uint32 inviteTime;
            bool responded;
        };

        ::std::unordered_map<ObjectGuid, PendingInvite> m_pendingInvites;
        void ProcessPendingInvites(uint32 diff);
        bool ShouldAcceptInvite(Player* inviter) const;

        // Target coordination
        ObjectGuid m_groupTarget;
        uint32 m_targetUpdateTime;
        void UpdateGroupTarget();
        void SyncWithGroupTarget();

        // Ready check tracking
        bool m_readyCheckActive;
        uint32 m_readyCheckTime;
        ::std::unordered_set<ObjectGuid> m_readyMembers;
        void ProcessReadyCheck(uint32 diff);

        // Statistics tracking
        void RecordGroupJoin();
        void RecordRaidComplete();
        void RecordQuestShare(uint32 questId);
        void RecordLootRoll(uint32 itemId, LootDecision decision);
        void RecordLootWon(uint32 itemId);

        // Performance tracking
        void StartPerformanceTimer();
        void EndPerformanceTimer();
        void UpdatePerformanceMetrics();

    private:
        Player* m_bot;
        BotAI* m_ai;
        bool m_enabled;

        // Tactical coordination subsystem (shared across group)
        std::shared_ptr<TacticalCoordinator> m_tacticalCoordinator;

        // Role assignment
        GroupRole m_assignedRole;
        GroupRole m_preferredRole;

        // Group state
        Group* m_currentGroup;
        uint32 m_lastGroupUpdate;

        // Configuration
        bool m_autoAcceptInvites;
        bool m_autoShareQuests;
        bool m_followGroupStrategy;
        bool m_smartLootRolls;
        uint32 m_inviteResponseDelay;

        // Loot tracking
        ::std::unordered_map<uint32, LootDecision> m_lootDecisions;
        uint32 m_lastLootRoll;

        // Boss strategies
        ::std::unordered_map<uint32, BossStrategy> m_bossStrategies;

        // Statistics
        Statistics m_stats;

        // Performance metrics
        ::std::chrono::high_resolution_clock::time_point m_performanceStart;
        ::std::chrono::microseconds m_lastUpdateDuration;
        ::std::chrono::microseconds m_totalUpdateTime;
        uint32 m_updateCount;
        float m_cpuUsage;
    };

} // namespace Advanced
} // namespace Playerbot

#endif // TRINITYCORE_BOT_GROUP_COORDINATOR_H
