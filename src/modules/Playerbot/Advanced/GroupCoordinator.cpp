/*
 * Copyright (C) 2024 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "GroupCoordinator.h"
#include "TacticalCoordinator.h"
#include "BotAI.h"
#include "Player.h"
#include "Group.h"
#include "GroupMgr.h"
#include "../Group/GroupEvents.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Log.h"
#include "GameTime.h"
#include "Item.h"
#include "ItemTemplate.h"
#include "Creature.h"
#include "WorldSession.h"
#include "LootItemType.h"
#include "QuestDef.h"
#include "SharedDefines.h"
#include "Loot/Loot.h"
#include "Core/Events/GenericEventBus.h"
#include "Group/GroupEvents.h"
#include "../Spatial/SpatialGridQueryHelpers.h"  // PHASE 5C: Thread-safe helpers
#include <algorithm>

namespace Playerbot
{
namespace Advanced
{
    // Configuration constants
    static constexpr uint32 GROUP_UPDATE_INTERVAL = 1000;  // 1 second
    static constexpr uint32 INVITE_RESPONSE_DELAY = 2000;  // 2 seconds
    static constexpr uint32 READY_CHECK_TIMEOUT = 30000;   // 30 seconds
    static constexpr uint32 LOOT_ROLL_TIMEOUT = 60000;     // 60 seconds
    static constexpr uint32 TARGET_UPDATE_INTERVAL = 500;  // 0.5 seconds

    GroupCoordinator::GroupCoordinator(Player* bot, BotAI* ai)
        : m_currentState(GroupState::IDLE)
        , m_targetUpdateTime(0)
        , m_readyCheckActive(false)
        , m_readyCheckTime(0)
        , m_bot(bot)
        , m_ai(ai)
        , m_enabled(true)
        , m_assignedRole(GroupRole::UNDEFINED)
        , m_preferredRole(GroupRole::UNDEFINED)
        , m_currentGroup(nullptr)
        , m_lastGroupUpdate(0)
        , m_autoAcceptInvites(false)
        , m_autoShareQuests(true)
        , m_followGroupStrategy(true)
        , m_smartLootRolls(true)
        , m_inviteResponseDelay(INVITE_RESPONSE_DELAY)
        , m_lastLootRoll(0)
        , m_totalUpdateTime(0)
        , m_updateCount(0)
        , m_cpuUsage(0.0f)
    {
        m_queueInfo.isQueued = false;
    }

    GroupCoordinator::~GroupCoordinator()
    {
        Shutdown();
    }

    void GroupCoordinator::Initialize()
    {
        // CRITICAL: Do NOT access bot data during initialization!
        // Bot may not be fully in world yet when Initialize() is called from
        // GameSystemsManager::Initialize() during BotAI constructor.
        // Defer all bot data access to first Update() call.

        // Just set defaults - actual initialization will happen in Update() when bot is in world
        m_assignedRole = GroupRole::UNDEFINED;
        m_preferredRole = GroupRole::UNDEFINED;

        // NOTE: TacticalCoordinator creation deferred to UpdateGroupState() when bot is in world
        // NOTE: Boss strategies loading deferred to first Update() when bot is in world
        // NOTE: Logging with bot name deferred to avoid crash on uninitialized m_name
    }

    void GroupCoordinator::Update(uint32 diff)
    {
        if (!m_enabled || !m_bot || !m_bot->IsInWorld())
            return;

        // Deferred initialization - now safe to access bot data
        if (m_assignedRole == GroupRole::UNDEFINED)
        {
            m_assignedRole = DetermineRole();
            m_preferredRole = m_assignedRole;
            LoadBossStrategies();
            TC_LOG_DEBUG("bot.playerbot", "GroupCoordinator deferred init for bot {} (Role: {})",
                m_bot->GetName(), static_cast<uint32>(m_assignedRole));
        }

        // Create TacticalCoordinator on-demand when bot joins a group
        if (!m_tacticalCoordinator && m_bot->GetGroup())
        {
            m_tacticalCoordinator = std::make_shared<TacticalCoordinator>(m_bot->GetGroup());
            m_tacticalCoordinator->Initialize();
        }

        StartPerformanceTimer();

        // Update group state
    if (GameTime::GetGameTimeMS() - m_lastGroupUpdate > GROUP_UPDATE_INTERVAL)
        {
            UpdateGroupState(diff);
            m_lastGroupUpdate = GameTime::GetGameTimeMS();
        }

        // Process pending invites
        ProcessPendingInvites(diff);

        // Process ready checks
    if (m_readyCheckActive)
            ProcessReadyCheck(diff);

        // Update group target
    if (IsInGroup() && GameTime::GetGameTimeMS() - m_targetUpdateTime > TARGET_UPDATE_INTERVAL)
        {
            UpdateGroupTarget();
            m_targetUpdateTime = GameTime::GetGameTimeMS();
        }

        // Update queue status
    if (m_queueInfo.isQueued)
            UpdateQueueStatus();

        // Update tactical coordinator
        if (m_tacticalCoordinator)
            m_tacticalCoordinator->Update(diff);

        EndPerformanceTimer();
        UpdatePerformanceMetrics();
    }

    void GroupCoordinator::Reset()
    {
        m_currentState = GroupState::IDLE;
        m_pendingInvites.clear();
        m_lootDecisions.clear();
        m_readyCheckActive = false;
        m_readyMembers.clear();
        m_queueInfo.isQueued = false;
    }

    void GroupCoordinator::Shutdown()
    {
        m_enabled = false;
        // CRITICAL: Do NOT call LeaveGroup() during destructor!
        // When a bot is being destroyed, the Group::RemoveMember() call can crash
        // because Group::BroadcastGroupUpdate() may try to access members that are
        // already being destroyed or have invalid state.
        //
        // The Player destructor will handle group cleanup automatically via
        // Group::RemoveMember called from Player::~Player() or RemoveFromWorld.
        // We should only call LeaveGroup() when explicitly requested (e.g., by command)
        // NOT during destruction shutdown.
        //
        // OLD CODE (CAUSES CRASH):
        // if (m_bot && m_bot->IsInWorld() && IsInGroup())
        //     LeaveGroup();
        //
        // Just clear our internal state - let TrinityCore handle group cleanup
        m_currentGroup = nullptr;
        m_currentState = GroupState::IDLE;
        Reset();
    }

    // Group Management
    bool GroupCoordinator::JoinGroup(Group* group)
    {
        if (!group || !m_bot)
            return false;
        m_currentGroup = group;
        m_currentState = GroupState::ACTIVE;
        RecordGroupJoin();
        TC_LOG_DEBUG("bot.playerbot", "Bot %s joined group", m_bot->GetName().c_str());
        return true;
    }

    bool GroupCoordinator::LeaveGroup()
    {
        // CRITICAL: Check bot validity before any group operations
        // During destruction, m_bot may not be in world and Group::RemoveMember
        // can crash when iterating the member list (std::list::begin() crash)
        if (!m_bot || !m_bot->IsInWorld())
            return false;

        if (!IsInGroup())
            return false;

        if (Group* group = m_bot->GetGroup())
            group->RemoveMember(m_bot->GetGUID());

        m_currentGroup = nullptr;
        m_currentState = GroupState::IDLE;

        TC_LOG_DEBUG("bot.playerbot", "Bot {} left group", m_bot->GetGUID().ToString());
        return true;
    }

    bool GroupCoordinator::InviteToGroup(Player* player)
    {
        if (!player || !m_bot)
            return false;

        if (IsInGroup())
        {
            // Invite to existing group (must be leader or have invite rights)
    if (Group* group = m_bot->GetGroup())
            {
                if (group->IsLeader(m_bot->GetGUID()) || group->IsAssistant(m_bot->GetGUID()))
                {
                    group->AddInvite(player);
                    TC_LOG_DEBUG("bot.playerbot", "Bot %s invited %s to group",
                        m_bot->GetName().c_str(), player->GetName().c_str());
                    return true;
                }
            }
        }
        else
        {
            // Create new group and invite
            Group* group = new Group;
            if (!group->Create(m_bot))
            {
                delete group;
                return false;
            }

            sGroupMgr->AddGroup(group);
            group->AddInvite(player);

            m_currentGroup = group;
            m_currentState = GroupState::FORMING;
            TC_LOG_DEBUG("bot.playerbot", "Bot %s created group and invited %s",
                m_bot->GetName().c_str(), player->GetName().c_str());
            return true;
        }
        return false;
    }

    bool GroupCoordinator::AcceptGroupInvite(Player* inviter)
    {
        if (!inviter || !m_bot)
            return false;

        Group* group = inviter->GetGroup();
        if (!group)
            return false;

        if (!group->AddMember(m_bot))
            return false;

        m_currentGroup = group;
        m_currentState = GroupState::ACTIVE;
        RecordGroupJoin();

        TC_LOG_DEBUG("bot.playerbot", "Bot %s accepted invite from %s",
            m_bot->GetName().c_str(), inviter->GetName().c_str());

        return true;
    }

    bool GroupCoordinator::DeclineGroupInvite(Player* inviter)
    {
        if (!inviter)
            return false;

        m_pendingInvites.erase(inviter->GetGUID());

        TC_LOG_DEBUG("bot.playerbot", "Bot %s declined invite from %s",
            m_bot->GetName().c_str(), inviter->GetName().c_str());

        return true;
    }

    Group* GroupCoordinator::GetGroup() const
    {
        return m_bot ? m_bot->GetGroup() : nullptr;
    }

    bool GroupCoordinator::IsInGroup() const
    {
        return m_bot && m_bot->GetGroup() != nullptr;
    }

    // Role Management
    GroupCoordinator::GroupRole GroupCoordinator::DetermineRole() const
    {
        if (!m_bot)
            return GroupRole::UNDEFINED;

        Classes botClass = static_cast<Classes>(m_bot->GetClass());
        // Tank classes
    if (botClass == CLASS_WARRIOR || botClass == CLASS_PALADIN ||
            botClass == CLASS_DEATH_KNIGHT || botClass == CLASS_DEMON_HUNTER)
        {
            if (HasTankingAbilities())
                return GroupRole::TANK;
        }

        // Healer classes
    if (botClass == CLASS_PRIEST || botClass == CLASS_PALADIN ||
            botClass == CLASS_SHAMAN || botClass == CLASS_DRUID ||
            botClass == CLASS_MONK || botClass == CLASS_EVOKER)
        {
            if (HasHealingAbilities())
                return GroupRole::HEALER;
        }

        // Melee DPS
    if (botClass == CLASS_WARRIOR || botClass == CLASS_ROGUE ||
            botClass == CLASS_DEATH_KNIGHT || botClass == CLASS_MONK ||
            botClass == CLASS_DEMON_HUNTER)
        {
            return GroupRole::DPS_MELEE;
        }

        // Ranged DPS
    if (botClass == CLASS_HUNTER || botClass == CLASS_MAGE ||
            botClass == CLASS_WARLOCK || botClass == CLASS_PRIEST ||
            botClass == CLASS_SHAMAN || botClass == CLASS_DRUID ||
            botClass == CLASS_EVOKER)
        {
            return GroupRole::DPS_RANGED;
        }

        return GroupRole::DPS_MELEE;
    }

    bool GroupCoordinator::CanFillRole(GroupRole role) const
    {
        ::std::vector<RoleCapability> capabilities = AnalyzeRoleCapabilities();

        for (auto const& cap : capabilities)
        {
            if (cap.role == role && cap.canPerform)
                return true;
        }

        return false;
    }

    // Raid Coordination
    bool GroupCoordinator::IsInRaid() const
    {
        Group* group = GetGroup();
        return group && group->isRaidGroup();
    }

    uint32 GroupCoordinator::GetRaidSize() const
    {
        Group* group = GetGroup();
        return group ? group->GetMembersCount() : 0;
    }

    uint32 GroupCoordinator::GetGroupSize() const
    {
        Group* group = GetGroup();
        return group ? group->GetMembersCount() : 0;
    }

    Player* GroupCoordinator::GetLeader() const
    {
        Group* group = GetGroup();
        if (!group)
            return nullptr;

        ObjectGuid leaderGuid = group->GetLeaderGUID();
        return ObjectAccessor::FindPlayer(leaderGuid);
    }

    bool GroupCoordinator::IsInCombat() const
    {
        if (m_bot && m_bot->IsInCombat())
            return true;

        // Check if any group member is in combat
        Group* group = GetGroup();
        if (!group)
            return false;

        for (GroupReference const& ref : group->GetMembers())
        {
            if (Player* member = ref.GetSource())
            {
                if (member->IsInCombat())
                    return true;
            }
        }

        return false;
    }

    bool GroupCoordinator::IsRaidLeader() const
    {
        if (!m_bot)
            return false;

        Group* group = GetGroup();
        return group && group->IsLeader(m_bot->GetGUID());
    }

    bool GroupCoordinator::AssignRaidRole(Player* target, GroupRole role)
    {
        if (!target || !IsRaidLeader())
            return false;

        // This would require raid role assignment API
        TC_LOG_DEBUG("bot.playerbot", "Bot %s assigned role %u to %s",
            m_bot->GetName().c_str(), static_cast<uint32>(role), target->GetName().c_str());

        return true;
    }

    void GroupCoordinator::CoordinateRaidPositions()
    {
        if (!IsInRaid())
            return;

        // Coordinate raid member positions based on roles
        // This integrates with PositionManager
        TC_LOG_DEBUG("bot.playerbot", "Bot %s coordinating raid positions", m_bot->GetName().c_str());
    }

    void GroupCoordinator::ExecuteBossStrategy(Creature* boss)
    {
        if (!boss)
            return;

        BossStrategy* strategy = GetBossStrategy(boss->GetEntry());
        if (!strategy)
            return;

        TC_LOG_DEBUG("bot.playerbot", "Bot %s executing strategy '%s' for boss %u",
            m_bot->GetName().c_str(), strategy->strategyName.c_str(), boss->GetEntry());

        // Execute phase 1 strategy (simplified)
        ExecutePhaseStrategy(strategy, 1);
    }

    // Loot Management
    GroupCoordinator::LootDecision GroupCoordinator::DecideLootRoll(uint32 itemId)
    {
        if (!m_smartLootRolls)
            return LootDecision::PASS;

        LootPriority priority = EvaluateLootItem(itemId);
        return priority.decision;
    }

    bool GroupCoordinator::RollForLoot(uint32 itemId, LootDecision decision)
    {
        m_lootDecisions[itemId] = decision;
        m_lastLootRoll = GameTime::GetGameTimeMS();
        RecordLootRoll(itemId, decision);

        TC_LOG_DEBUG("bot.playerbot", "Bot %s rolled %u for item %u",
            m_bot->GetName().c_str(), static_cast<uint32>(decision), itemId);

        return true;
    }

    void GroupCoordinator::ConfigureLootSettings(Group* group)
    {
        if (!group || !IsRaidLeader())
            return;

        // Set group loot to group loot
        group->SetLootMethod(GROUP_LOOT);
        group->SetLootThreshold(ITEM_QUALITY_UNCOMMON);
        TC_LOG_DEBUG("bot.playerbot", "Bot %s configured loot settings", m_bot->GetName().c_str());
    }

    bool GroupCoordinator::NeedItem(uint32 itemId) const
    {
        ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemId);
        if (!proto)
            return false;

        // Check if item is usable by class
    if (proto->GetAllowableClass() && !(proto->GetAllowableClass() & m_bot->GetClassMask()))
            return false;

        // Check if it's an upgrade
        return IsItemUpgrade(itemId);
    }

    bool GroupCoordinator::CanGreedItem(uint32 itemId) const
    {
        ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemId);
        if (!proto)
            return false;

        // Can greed anything we can't need
        return !NeedItem(itemId);
    }

    // Quest Coordination
    bool GroupCoordinator::ShareQuest(uint32 questId)
    {
        if (!IsInGroup() || !ShouldShareQuest(questId))
            return false;

        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        if (!quest || !quest->HasFlag(QUEST_FLAGS_SHARABLE))
            return false;

        RecordQuestShare(questId);
        TC_LOG_DEBUG("bot.playerbot", "Bot %s shared quest %u", m_bot->GetName().c_str(), questId);
        return true;
    }

    bool GroupCoordinator::AcceptSharedQuest(uint32 questId, Player* sharer)
    {
        if (!sharer)
            return false;

        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        if (!quest)
            return false;

        if (!m_bot->CanAddQuest(quest, true))
            return false;

        m_bot->AddQuest(quest, sharer);

        TC_LOG_DEBUG("bot.playerbot", "Bot %s accepted shared quest %u from %s",
            m_bot->GetName().c_str(), questId, sharer->GetName().c_str());
        return true;
    }

    void GroupCoordinator::SyncGroupQuests()
    {
        if (!IsInGroup() || !m_autoShareQuests)
            return;

        ::std::vector<uint32> shareable = GetShareableQuests();
        for (uint32 questId : shareable)
            ShareQuest(questId);
    }
    ::std::vector<uint32> GroupCoordinator::GetShareableQuests() const
    {
        ::std::vector<uint32> shareable;

        if (!m_bot)
            return shareable;

        for (uint8 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
        {
            uint32 questId = m_bot->GetQuestSlotQuestId(slot);
            if (questId == 0)
                continue;

            Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
            if (quest && quest->HasFlag(QUEST_FLAGS_SHARABLE))
                shareable.push_back(questId);
        }

        return shareable;
    }

    // Group Composition
    GroupCoordinator::GroupComposition GroupCoordinator::AnalyzeGroupComposition() const
    {
        GroupComposition comp;

        // CRITICAL: Early return if bot is not in world or not valid
        // During construction/initialization, m_bot may not be fully initialized
        // and accessing group->GetMembers() can cause ACCESS_VIOLATION crash
        if (!m_bot || !m_bot->IsInWorld())
            return comp;

        Group* group = GetGroup();
        if (!group)
            return comp;

        for (GroupReference const& ref : group->GetMembers())
        {
            Player* member = ref.GetSource();
            if (!member)
                continue;
            comp.total++;

            // Determine role (simplified)
            Classes memberClass = static_cast<Classes>(member->GetClass());
            if (memberClass == CLASS_WARRIOR || memberClass == CLASS_PALADIN ||
                memberClass == CLASS_DEATH_KNIGHT)
                comp.tanks++;
            else if (memberClass == CLASS_PRIEST || memberClass == CLASS_SHAMAN ||
                     memberClass == CLASS_DRUID)
                comp.healers++;
            else
                comp.dps++;
        }

        // Check balance (1 tank, 1 healer, rest DPS)
        comp.isBalanced = (comp.tanks >= 1 && comp.healers >= 1);
        comp.canRaid = (comp.total >= 10);
        return comp;
    }

    bool GroupCoordinator::IsGroupBalanced() const
    {
        return AnalyzeGroupComposition().isBalanced;
    }
    GroupCoordinator::GroupRole GroupCoordinator::GetNeededRole() const
    {
        GroupComposition comp = AnalyzeGroupComposition();

        if (comp.tanks == 0)
            return GroupRole::TANK;
        if (comp.healers == 0)
            return GroupRole::HEALER;
        return GroupRole::DPS_MELEE;
    }

    ::std::vector<Player*> GroupCoordinator::GetGroupMembers() const
    {
        ::std::vector<Player*> members;

        Group* group = GetGroup();
        if (!group)
            return members;
        for (GroupReference const& ref : group->GetMembers())
        {
            if (Player* member = ref.GetSource())
                members.push_back(member);
        }

        return members;
    }

    // Ready Checks
    bool GroupCoordinator::PerformReadyCheck()
    {
        if (!IsRaidLeader())
            return false;

        m_readyCheckActive = true;
        m_readyCheckTime = GameTime::GetGameTimeMS();
        m_readyMembers.clear();
        TC_LOG_DEBUG("bot.playerbot", "Bot %s initiated ready check", m_bot->GetName().c_str());
        return true;
    }
    bool GroupCoordinator::RespondToReadyCheck(bool ready)
    {
        if (!m_readyCheckActive)
            return false;
        if (ready)
            m_readyMembers.insert(m_bot->GetGUID());
        TC_LOG_DEBUG("bot.playerbot", "Bot %s responded to ready check: %s",
            m_bot->GetName().c_str(), ready ? "ready" : "not ready");

        return true;
    }

    bool GroupCoordinator::IsGroupReady() const
    {
        if (!m_readyCheckActive)
            return false;
        Group* group = GetGroup();
        if (!group)
            return false;

        return m_readyMembers.size() >= static_cast<size_t>(group->GetMembersCount());
    }

    void GroupCoordinator::WaitForGroupReady()
    {
        // Wait for all members to be ready
    while (m_readyCheckActive && !IsGroupReady())
        {
            // Process updates
            Update(100);
        }
    }

    // Dungeon/Raid Finder
    bool GroupCoordinator::QueueForDungeon(uint32 dungeonId)
    {
        m_queueInfo.dungeonId = dungeonId;
        m_queueInfo.queueTime = GameTime::GetGameTimeMS();
        m_queueInfo.isQueued = true;
        TC_LOG_DEBUG("bot.playerbot", "Bot %s queued for dungeon %u",
            m_bot->GetName().c_str(), dungeonId);

        return true;
    }

    bool GroupCoordinator::QueueForRaid(uint32 raidId)
    {
        m_queueInfo.dungeonId = raidId;
        m_queueInfo.queueTime = GameTime::GetGameTimeMS();
        m_queueInfo.isQueued = true;
        TC_LOG_DEBUG("bot.playerbot", "Bot %s queued for raid %u",
            m_bot->GetName().c_str(), raidId);

        return true;
    }

    bool GroupCoordinator::AcceptDungeonInvite()
    {
        if (!m_queueInfo.isQueued)
            return false;

        m_queueInfo.isQueued = false;
        m_currentState = GroupState::DUNGEON;
        TC_LOG_DEBUG("bot.playerbot", "Bot %s accepted dungeon invite", m_bot->GetName().c_str());
        return true;
    }

    void GroupCoordinator::LeaveDungeonQueue()
    {
        m_queueInfo.isQueued = false;
        TC_LOG_DEBUG("bot.playerbot", "Bot %s left dungeon queue", m_bot->GetName().c_str());
    }

    // Combat Coordination
    void GroupCoordinator::AssignTarget(Unit* target)
    {
        if (!target)
            return;

        m_groupTarget = target->GetGUID();
        TC_LOG_DEBUG("bot.playerbot", "Bot %s assigned target", m_bot->GetName().c_str());
    }

    void GroupCoordinator::FocusTarget(Unit* target)
    {
        if (!target || !m_bot)
            return;

        m_bot->SetTarget(target->GetGUID());
        TC_LOG_DEBUG("bot.playerbot", "Bot %s focusing target", m_bot->GetName().c_str());
    }

    Unit* GroupCoordinator::GetGroupTarget() const
    {
        // PHASE 5C: Thread-safe spatial grid validation (replaces ObjectAccessor::GetUnit)
        auto snapshot = SpatialGridQueryHelpers::FindCreatureByGuid(m_bot, m_groupTarget);
        if (snapshot)
        {
            // Get Unit* for return (validated via snapshot first)
            return ObjectAccessor::GetUnit(*m_bot, m_groupTarget);
        }
        return nullptr;
    }

    void GroupCoordinator::CoordinateCrowdControl(Unit* target)
    {
        if (!target)
            return;

        TC_LOG_DEBUG("bot.playerbot", "Bot %s coordinating CC on target", m_bot->GetName().c_str());
    }

    void GroupCoordinator::CallForHelp(Unit* attacker)
    {
        if (!attacker || !IsInGroup())
            return;

        TC_LOG_DEBUG("bot.playerbot", "Bot %s calling for help", m_bot->GetName().c_str());
    }

    // Resurrection
    void GroupCoordinator::RequestResurrection()
    {
        if (!m_bot || m_bot->IsAlive())
            return;
        TC_LOG_DEBUG("bot.playerbot", "Bot %s requesting resurrection", m_bot->GetName().c_str());
    }

    bool GroupCoordinator::OfferResurrection(Player* target)
    {
        if (!target || target->IsAlive())
            return false;

        m_stats.resurrectionsGiven++;

        TC_LOG_DEBUG("bot.playerbot", "Bot %s offering resurrection to %s",
            m_bot->GetName().c_str(), target->GetName().c_str());

        return true;
    }

    void GroupCoordinator::CoordinateGroupRecovery()
    {
        if (!IsInGroup())
            return;
        TC_LOG_DEBUG("bot.playerbot", "Bot %s coordinating group recovery", m_bot->GetName().c_str());
    }

    bool GroupCoordinator::ShouldWaitForGroup() const
    {
        return IsInGroup() && m_followGroupStrategy;
    }

    // Private Methods
    void GroupCoordinator::UpdateGroupState(uint32 /*diff*/)
    {
        Group* group = GetGroup();

        if (!group && m_currentState != GroupState::IDLE)
        {
            TransitionToState(GroupState::IDLE);
            return;
        }

        if (group && m_currentState == GroupState::IDLE)
        {
            TransitionToState(GroupState::ACTIVE);
        }

        if (m_bot && m_bot->IsInCombat() && m_currentState != GroupState::IN_COMBAT)
        {
            TransitionToState(GroupState::IN_COMBAT);
        }
        else if (m_bot && !m_bot->IsInCombat() && m_currentState == GroupState::IN_COMBAT)
        {
            TransitionToState(GroupState::ACTIVE);
        }
    }

    void GroupCoordinator::TransitionToState(GroupState newState)
    {
        GroupState oldState = m_currentState;
        m_currentState = newState;

        TC_LOG_DEBUG("bot.playerbot", "Bot %s group state: %u -> %u",
            m_bot->GetName().c_str(), static_cast<uint32>(oldState), static_cast<uint32>(newState));
    }

    ::std::vector<GroupCoordinator::RoleCapability> GroupCoordinator::AnalyzeRoleCapabilities() const
    {
        ::std::vector<RoleCapability> capabilities;

        RoleCapability tank;
        tank.role = GroupRole::TANK;
        tank.canPerform = HasTankingAbilities();
        tank.suitability = tank.canPerform ? 0.8f : 0.0f;
        capabilities.push_back(tank);

        RoleCapability healer;
        healer.role = GroupRole::HEALER;
        healer.canPerform = HasHealingAbilities();
        healer.suitability = healer.canPerform ? 0.8f : 0.0f;
        capabilities.push_back(healer);

        RoleCapability dps;
        dps.role = GroupRole::DPS_MELEE;
        dps.canPerform = true;
        dps.suitability = 0.9f;
        capabilities.push_back(dps);

        return capabilities;
    }

    GroupCoordinator::GroupRole GroupCoordinator::GetBestRole() const
    {
        auto capabilities = AnalyzeRoleCapabilities();

        float bestSuitability = 0.0f;
        GroupRole bestRole = GroupRole::DPS_MELEE;

        for (auto const& cap : capabilities)
        {
            if (cap.suitability > bestSuitability)
            {
                bestSuitability = cap.suitability;
                bestRole = cap.role;
            }
        }

        return bestRole;
    }

    bool GroupCoordinator::HasTankingAbilities() const
    {
        if (!m_bot)
            return false;

        Classes botClass = static_cast<Classes>(m_bot->GetClass());
        return (botClass == CLASS_WARRIOR || botClass == CLASS_PALADIN ||
                botClass == CLASS_DEATH_KNIGHT || botClass == CLASS_DEMON_HUNTER ||
                botClass == CLASS_DRUID || botClass == CLASS_MONK);
    }

    bool GroupCoordinator::HasHealingAbilities() const
    {
        if (!m_bot)
            return false;

        Classes botClass = static_cast<Classes>(m_bot->GetClass());
        return (botClass == CLASS_PRIEST || botClass == CLASS_PALADIN ||
                botClass == CLASS_SHAMAN || botClass == CLASS_DRUID ||
                botClass == CLASS_MONK || botClass == CLASS_EVOKER);
    }

    GroupCoordinator::LootPriority GroupCoordinator::EvaluateLootItem(uint32 itemId) const
    {
        LootPriority priority;
        priority.itemId = itemId;
        priority.priority = 0.0f;
        priority.decision = LootDecision::PASS;
        priority.isUpgrade = false;
        priority.isTransmog = false;

        if (NeedItem(itemId))
        {
            priority.decision = LootDecision::NEED;
            priority.priority = 100.0f;
            priority.isUpgrade = IsItemUpgrade(itemId);
        }
        else if (CanGreedItem(itemId))
        {
            priority.decision = LootDecision::GREED;
            priority.priority = 50.0f;
        }

        return priority;
    }

    float GroupCoordinator::CalculateItemValue(uint32 itemId) const
    {
        ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemId);
        if (!proto)
            return 0.0f;

        float value = 0.0f;

        // Base value from quality
    switch (proto->GetQuality())
        {
            case ITEM_QUALITY_POOR:     value = 1.0f; break;
            case ITEM_QUALITY_NORMAL:   value = 5.0f; break;
            case ITEM_QUALITY_UNCOMMON: value = 20.0f; break;
            case ITEM_QUALITY_RARE:     value = 50.0f; break;
            case ITEM_QUALITY_EPIC:     value = 100.0f; break;
            default: value = 1.0f; break;
        }

        return value;
    }

    bool GroupCoordinator::IsItemUpgrade(uint32 itemId) const
    {
        ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemId);
        if (!proto)
            return false;

        // Check if item is equipment
    if (proto->GetClass() != ITEM_CLASS_WEAPON && proto->GetClass() != ITEM_CLASS_ARMOR)
            return false;

        // Get current item in that slot
        uint8 slot = proto->GetInventoryType();
        if (slot == INVTYPE_NON_EQUIP)
            return false;

        Item* currentItem = m_bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (!currentItem)
            return true; // Empty slot = upgrade

        // Simple item level comparison
        return proto->GetBaseItemLevel() > currentItem->GetTemplate()->GetBaseItemLevel();
    }

    ::std::vector<GroupCoordinator::ShareableQuest> GroupCoordinator::EvaluateQuestsToShare() const
    {
        ::std::vector<ShareableQuest> quests;

        ::std::vector<uint32> shareable = GetShareableQuests();
        Group* group = GetGroup();
        if (!group)
            return quests;

        for (uint32 questId : shareable)
        {
            ShareableQuest sq;
            sq.questId = questId;
            sq.membersWithQuest = 0;
            sq.membersCanAccept = 0;

            for (GroupReference const& ref : group->GetMembers())
            {
                Player* member = ref.GetSource();
                if (!member || member == m_bot)
                    continue;

                if (member->GetQuestStatus(questId) != QUEST_STATUS_NONE)
                    sq.membersWithQuest++;
                else if (CanMemberAcceptQuest(member, questId))
                    sq.membersCanAccept++;
            }

            sq.sharePriority = static_cast<float>(sq.membersCanAccept) / static_cast<float>(group->GetMembersCount());
            quests.push_back(sq);
        }

        return quests;
    }

    bool GroupCoordinator::ShouldShareQuest(uint32 questId) const
    {
        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        if (!quest || !quest->HasFlag(QUEST_FLAGS_SHARABLE))
            return false;
        Group* group = GetGroup();
        if (!group)
            return false;

        // Check if at least one member can accept
    for (GroupReference const& ref : group->GetMembers())
        {
            Player* member = ref.GetSource();
            if (member && member != m_bot && CanMemberAcceptQuest(member, questId))
                return true;
        }

        return false;
    }

    bool GroupCoordinator::CanMemberAcceptQuest(Player* member, uint32 questId) const
    {
        if (!member)
            return false;

        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        if (!quest)
            return false;

        return member->CanAddQuest(quest, true);
    }

    GroupCoordinator::BossStrategy* GroupCoordinator::GetBossStrategy(uint32 bossEntry)
    {
        auto it = m_bossStrategies.find(bossEntry);
        if (it != m_bossStrategies.end())
            return &it->second;

        return nullptr;
    }

    void GroupCoordinator::LoadBossStrategies()
    {
        // Load predefined boss strategies (simplified)
        // In a real implementation, this would load from database/config

        BossStrategy example;
        example.bossEntry = 0;
        example.strategyName = "Generic";
        example.phases.push_back("Phase 1");
        m_bossStrategies[0] = example;
    }

    void GroupCoordinator::ExecutePhaseStrategy(BossStrategy const* strategy, uint32 phase)
    {
        if (!strategy || phase == 0 || phase > strategy->phases.size())
            return;

        TC_LOG_DEBUG("bot.playerbot", "Executing phase %u of strategy '%s'",
            phase, strategy->strategyName.c_str());
    }

    void GroupCoordinator::UpdateQueueStatus()
    {
        if (!m_queueInfo.isQueued)
            return;

        // Calculate time in queue for potential future use in estimated wait time
        // For now, use a simple 5-minute estimate
        m_queueInfo.estimatedWait = 300000; // 5 minutes estimate
    }

    void GroupCoordinator::ProcessPendingInvites(uint32 /*diff*/)
    {
        for (auto it = m_pendingInvites.begin(); it != m_pendingInvites.end();)
        {
            if (it->second.responded)
            {
                it = m_pendingInvites.erase(it);
                continue;
            }

            if (GameTime::GetGameTimeMS() - it->second.inviteTime > m_inviteResponseDelay)
            {
                // PHASE 5C: Thread-safe spatial grid validation (replaces ObjectAccessor::FindPlayer)
                auto snapshot = SpatialGridQueryHelpers::FindPlayerByGuid(m_bot, it->first);
                Player* inviter = nullptr;

                if (snapshot && snapshot->isAlive)
                {
                    // Get Player* for invite acceptance (validated via snapshot first)
                    inviter = ObjectAccessor::FindPlayer(it->first);
                }

                if (inviter && ShouldAcceptInvite(inviter))
                {
                    AcceptGroupInvite(inviter);
                    it->second.responded = true;
                }
                else
                {
                    DeclineGroupInvite(inviter);
                    it->second.responded = true;
                }
            }

            ++it;
        }
    }

    bool GroupCoordinator::ShouldAcceptInvite(Player* inviter) const
    {
        if (!inviter || !m_autoAcceptInvites)
            return false;

        // Accept invites from friends, guild members, etc.
        return true;
    }

    void GroupCoordinator::UpdateGroupTarget()
    {
        Group* group = GetGroup();
        if (!group)
            return;

        // PHASE 5C: Thread-safe spatial grid validation (replaces ObjectAccessor::FindPlayer)
        auto snapshot = SpatialGridQueryHelpers::FindPlayerByGuid(m_bot, group->GetLeaderGUID());
        Player* leader = nullptr;

        if (snapshot && snapshot->isAlive)
        {
            // Get Player* for target sync (validated via snapshot first)
            leader = ObjectAccessor::FindPlayer(group->GetLeaderGUID());
        }

        if (leader && !leader->GetTarget().IsEmpty())
        {
            m_groupTarget = leader->GetTarget();
        }
    }

    void GroupCoordinator::SyncWithGroupTarget()
    {
        Unit* target = GetGroupTarget();
        if (target)
            FocusTarget(target);
    }

    void GroupCoordinator::ProcessReadyCheck(uint32 /*diff*/)
    {
        if (GameTime::GetGameTimeMS() - m_readyCheckTime > READY_CHECK_TIMEOUT)
        {
            m_readyCheckActive = false;
            m_readyMembers.clear();
        }
    }

    void GroupCoordinator::RecordGroupJoin()
    {
        m_stats.groupsJoined++;
    }

    void GroupCoordinator::RecordRaidComplete()
    {
        m_stats.raidsCompleted++;
    }

    void GroupCoordinator::RecordQuestShare(uint32 /*questId*/)
    {
        m_stats.questsShared++;
    }

    void GroupCoordinator::RecordLootRoll(uint32 /*itemId*/, LootDecision /*decision*/)
    {
        m_stats.lootRolls++;
    }

    void GroupCoordinator::RecordLootWon(uint32 /*itemId*/)
    {
        m_stats.lootWon++;
    }

    void GroupCoordinator::StartPerformanceTimer()
    {
        m_performanceStart = ::std::chrono::high_resolution_clock::now();
    }

    void GroupCoordinator::EndPerformanceTimer()
    {
        auto end = ::std::chrono::high_resolution_clock::now();
        m_lastUpdateDuration = ::std::chrono::duration_cast<::std::chrono::microseconds>(end - m_performanceStart);
        m_totalUpdateTime += m_lastUpdateDuration;
        m_updateCount++;
    }

    void GroupCoordinator::UpdatePerformanceMetrics()
    {
        if (m_updateCount > 0)
        {
            auto avgMicros = m_totalUpdateTime.count() / m_updateCount;
            m_cpuUsage = avgMicros / 10000.0f;
        }
    }

    size_t GroupCoordinator::GetMemoryUsage() const
    {
        size_t memory = sizeof(*this);
        memory += m_pendingInvites.size() * (sizeof(ObjectGuid) + sizeof(PendingInvite));
        memory += m_lootDecisions.size() * (sizeof(uint32) + sizeof(LootDecision));
        memory += m_bossStrategies.size() * sizeof(BossStrategy);
        memory += m_readyMembers.size() * sizeof(ObjectGuid);
        return memory;
    }

    // Group Event Handlers (called from BotAI_EventHandlers.cpp)
    void GroupCoordinator::OnTargetIconChanged(GroupEvent const& event)
    {
        if (!m_enabled || !m_bot || !IsInGroup())
            return;

        // Handle raid target icon changes
        // Event contains targetGuid and iconId in data1
        uint8 targetIconId = static_cast<uint8>(event.data1);
        TC_LOG_DEBUG("bot.playerbot", "Bot %s received target icon change (Icon: %u)",
            m_bot->GetName().c_str(), targetIconId);

        // If this is our current target and icon was changed, update our targeting priority
    if (event.targetGuid == m_groupTarget)
        {
            // High priority icons (Skull, Cross) should be focused
    if (targetIconId == 0 || targetIconId == 1) // Skull or Cross
            {
                // PHASE 5C: Thread-safe spatial grid validation (replaces ObjectAccessor::GetUnit)
                auto snapshot = SpatialGridQueryHelpers::FindCreatureByGuid(m_bot, event.targetGuid);
                Unit* target = nullptr;

                if (snapshot && snapshot->IsAlive())
                {
                    // Get Unit* for targeting (validated via snapshot first)
                    target = ObjectAccessor::GetUnit(*m_bot, event.targetGuid);
                }

                if (target && target->IsAlive())
                {
                    FocusTarget(target);
                    TC_LOG_DEBUG("bot.playerbot", "Bot %s focusing high-priority marked target",
                        m_bot->GetName().c_str());
                }
            }
        }
    }

    void GroupCoordinator::OnGroupCompositionChanged(GroupEvent const& event)
    {
        // CRITICAL: Check bot validity AND IsInWorld() before ANY bot access
        // During destruction or initialization, m_bot may not be in world and
        // accessing GetName(), GetGroup(), or iterating group members can crash
        if (!m_enabled || !m_bot || !m_bot->IsInWorld() || !IsInGroup())
            return;

        // Handle member join/leave events
        TC_LOG_DEBUG("bot.playerbot", "Bot {} received group composition change (Type: {})",
            m_bot->GetGUID().ToString(), static_cast<uint32>(event.type));

        // Analyze new group composition
        GroupComposition comp = AnalyzeGroupComposition();

        // Log composition changes
        TC_LOG_DEBUG("bot.playerbot", "Bot {} group composition - Tanks: {}, Healers: {}, DPS: {}, Total: {}, Balanced: {}",
            m_bot->GetGUID().ToString(), comp.tanks, comp.healers, comp.dps, comp.total,
            comp.isBalanced ? "Yes" : "No");

        // If group became unbalanced, consider suggesting role changes
        if (!comp.isBalanced)
        {
            GroupRole neededRole = GetNeededRole();
            if (CanFillRole(neededRole) && neededRole != m_assignedRole)
            {
                TC_LOG_DEBUG("bot.playerbot", "Bot {} could fill needed role {} (current: {})",
                    m_bot->GetGUID().ToString(), static_cast<uint32>(neededRole),
                    static_cast<uint32>(m_assignedRole));
            }
        }

        // If new member joined, check if we should share quests
    if (event.type == GroupEventType::MEMBER_JOINED && m_autoShareQuests)
        {
            SyncGroupQuests();
        }
    }

    // ========================================================================
    // TACTICAL COORDINATION DELEGATION
    // ========================================================================

    void GroupCoordinator::RequestInterrupt(ObjectGuid targetGuid)
    {
        if (!m_tacticalCoordinator)
        {
            TC_LOG_WARN("bot.playerbot", "Bot %s: Cannot request interrupt - no tactical coordinator",
                m_bot->GetName().c_str());
            return;
        }

        ObjectGuid assignedBot = m_tacticalCoordinator->AssignInterrupt(targetGuid);

        if (!assignedBot.IsEmpty())
        {
            TC_LOG_DEBUG("bot.playerbot", "Bot %s: Interrupt assigned to %s for target %s",
                m_bot->GetName().c_str(),
                assignedBot.ToString().c_str(),
                targetGuid.ToString().c_str());
        }
    }

    void GroupCoordinator::RequestDispel(ObjectGuid targetGuid)
    {
        if (!m_tacticalCoordinator)
        {
            TC_LOG_WARN("bot.playerbot", "Bot %s: Cannot request dispel - no tactical coordinator",
                m_bot->GetName().c_str());
            return;
        }

        ObjectGuid assignedBot = m_tacticalCoordinator->AssignDispel(targetGuid);

        if (!assignedBot.IsEmpty())
        {
            TC_LOG_DEBUG("bot.playerbot", "Bot %s: Dispel assigned to %s for target %s",
                m_bot->GetName().c_str(),
                assignedBot.ToString().c_str(),
                targetGuid.ToString().c_str());
        }
    }

    bool GroupCoordinator::IsGroupCooldownAvailable(std::string const& cooldownName) const
    {
        if (!m_tacticalCoordinator)
            return true; // No coordinator = no cooldown tracking

        return m_tacticalCoordinator->IsGroupCooldownAvailable(cooldownName);
    }

    void GroupCoordinator::UseGroupCooldown(std::string const& cooldownName, uint32 durationMs)
    {
        if (!m_tacticalCoordinator)
        {
            TC_LOG_WARN("bot.playerbot", "Bot %s: Cannot use group cooldown - no tactical coordinator",
                m_bot->GetName().c_str());
            return;
        }

        m_tacticalCoordinator->UseGroupCooldown(cooldownName, durationMs);

        TC_LOG_DEBUG("bot.playerbot", "Bot %s: Group cooldown %s used (duration: %ums)",
            m_bot->GetName().c_str(), cooldownName.c_str(), durationMs);
    }

} // namespace Advanced
} // namespace Playerbot
