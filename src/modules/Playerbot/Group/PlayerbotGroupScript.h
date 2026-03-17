/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_GROUP_SCRIPT_H
#define PLAYERBOT_GROUP_SCRIPT_H

#include "ScriptMgr.h"
#include "Threading/LockHierarchy.h"
#include "ObjectGuid.h"
#include <unordered_map>
#include <array>
#include <chrono>
#include <mutex>

// Forward declarations
class Group;
// Note: LootMethod, ItemQualities, and Difficulty are already defined in TrinityCore headers - don't re-declare

namespace Playerbot
{

// Forward declaration for GroupEvent (defined in GroupEventBus.h)
struct GroupEvent;

/**
 * @class PlayerbotGroupScript
 * @brief TrinityCore GroupScript implementation for playerbot group event handling
 *
 * This class implements the Observer pattern using TrinityCore's existing GroupScript hooks.
 * It subscribes to the 5 core group lifecycle events that Group.cpp already calls:
 * - OnAddMember
 * - OnInviteMember
 * - OnRemoveMember
 * - OnChangeLeader
 * - OnDisband
 *
 * For events NOT covered by GroupScript (ready checks, loot changes, etc.),
 * we use polling in the WorldScript update loop to detect state changes.
 *
 * **Architecture Decision** (from SCRIPTMGR_VS_HOOKS_ANALYSIS.md):
 * - ZERO core file modifications (perfect CLAUDE.md compliance)
 * - Uses existing ScriptMgr infrastructure (no custom hooks)
 * - Polling for missing events (<0.1% CPU overhead)
 * - 100ms detection latency (acceptable for bot AI)
 *
 * **Performance Targets**:
 * - <0.03% CPU for 500 bot groups
 * - ~200 CPU cycles per group per poll
 * - Polling interval: 100ms (10 checks per second)
 *
 * **Integration Pattern**:
 * ```cpp
 * // Group.cpp already calls (NO modifications needed):
 * sScriptMgr->OnGroupAddMember(this, player->GetGUID());     // Line 500
 * sScriptMgr->OnGroupInviteMember(this, player->GetGUID());  // Line 375
 * sScriptMgr->OnGroupRemoveMember(this, guid, method, ...);  // Line 575
 * sScriptMgr->OnGroupChangeLeader(this, newGuid, oldGuid);   // Line 700
 * sScriptMgr->OnGroupDisband(this);                          // Line 734
 * ```
 */
class TC_GAME_API PlayerbotGroupScript : public GroupScript
{
public:
    PlayerbotGroupScript();
    ~PlayerbotGroupScript();

    // ========================================================================
    // SCRIPTMGR HOOKS (Already called by Group.cpp)
    // ========================================================================

    /**
     * Hook: Member added to group
     * Called from: Group::AddMember() line 500
     * Publishes: GroupEventType::MEMBER_JOINED
     */
    void OnAddMember(Group* group, ObjectGuid guid);

    /**
     * Hook: Member invited to group
     * Called from: Group::AddInvite() line 375
     * Publishes: GroupEventType::MEMBER_JOINED (when invite accepted)
     */
    void OnInviteMember(Group* group, ObjectGuid guid);

    /**
     * Hook: Member removed from group
     * Called from: Group::RemoveMember() line 575
     * Publishes: GroupEventType::MEMBER_LEFT
     */
    void OnRemoveMember(Group* group, ObjectGuid guid, RemoveMethod method, ObjectGuid kicker, char const* reason);

    /**
     * Hook: Group leadership changed
     * Called from: Group::ChangeLeader() line 700
     * Publishes: GroupEventType::LEADER_CHANGED
     */
    void OnChangeLeader(Group* group, ObjectGuid newLeaderGuid, ObjectGuid oldLeaderGuid);

    /**
     * Hook: Group disbanded
     * Called from: Group::Disband() line 734
     * Publishes: GroupEventType::GROUP_DISBANDED
     */
    void OnDisband(Group* group);

    // ========================================================================
    // STATE POLLING (Called from PlayerbotWorldScript)
    // ========================================================================

    /**
     * Poll group state changes
     * Called from: PlayerbotWorldScript::OnUpdate() every 100ms
     *
     * Detects and publishes events for:
     * - Ready check started/completed
     * - Loot method/threshold/master looter changed
     * - Target icons changed
     * - Raid markers changed
     * - Subgroup assignments changed
     * - Difficulty changed
     * - Raid conversion (party ↔ raid)
     *
     * @param group Group to poll
     * @param diff Time since last poll (milliseconds)
     */
    static void PollGroupStateChanges(Group* group, uint32 diff);

private:
    /**
     * @struct GroupState
     * @brief Cached group state for change detection
     *
     * Stores previous state to detect changes via polling.
     * Memory overhead: ~150 bytes per group
     */
    struct GroupState
    {
        // Loot settings
        uint8 lootMethod{0};              // LootMethod enum
        uint8 lootThreshold{0};           // ItemQualities enum
        ObjectGuid masterLooterGuid;

        // Target icons (8 raid markers)
        ::std::array<ObjectGuid, 8> targetIcons;

        // Difficulty settings (WoW 12.0: Changed from uint8 to int16)
        int16 dungeonDifficulty{0};
        int16 raidDifficulty{0};
        int16 legacyRaidDifficulty{0};

        // Ready check
        bool readyCheckActive{false};

        // Group type
        bool isRaid{false};

        // Member subgroups (for change detection)
        ::std::unordered_map<ObjectGuid, uint8> memberSubgroups;

        // Last update timestamp
        ::std::chrono::steady_clock::time_point lastUpdate;
    };

    // State cache: GroupGUID → GroupState
    static inline ::std::unordered_map<ObjectGuid, GroupState> _groupStates;
    static inline Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::GROUP_MANAGER> _groupStatesMutex;

    // Polling helper functions
    static void CheckLootMethodChange(Group* group, GroupState& state);
    static void CheckLootThresholdChange(Group* group, GroupState& state);
    static void CheckMasterLooterChange(Group* group, GroupState& state);
    static void CheckTargetIconChanges(Group* group, GroupState& state);
    static void CheckDifficultyChanges(Group* group, GroupState& state);
    static void CheckRaidConversion(Group* group, GroupState& state);
    static void CheckReadyCheckState(Group* group, GroupState& state);
    static void CheckSubgroupChanges(Group* group, GroupState& state);

    // State management
    static GroupState& GetOrCreateGroupState(ObjectGuid groupGuid);
    static void RemoveGroupState(ObjectGuid groupGuid);
    static void InitializeGroupState(Group* group, GroupState& state);

    // Event publishing
    static void PublishEvent(GroupEvent const& event);

    // Statistics
    struct PollStatistics
    {
        uint64_t totalPolls{0};
        uint64_t eventsDetected{0};
        uint64_t averagePollTimeUs{0};
        ::std::chrono::steady_clock::time_point startTime;  // Initialized in constructor

        PollStatistics() : startTime(::std::chrono::steady_clock::now()) {}

        void Reset()
        {
            totalPolls = 0;
            eventsDetected = 0;
            averagePollTimeUs = 0;
            startTime = ::std::chrono::steady_clock::now();
        }

        ::std::string ToString() const
        {
            auto now = ::std::chrono::steady_clock::now();
            auto uptime = ::std::chrono::duration_cast<::std::chrono::seconds>(now - startTime).count();

            return fmt::format(
                "PlayerbotGroupScript Poll Statistics:\n"
                "  Total Polls: {}\n"
                "  Events Detected: {}\n"
                "  Average Poll Time: {} μs\n"
                "  Polls Per Second: {:.2f}\n"
                "  Uptime: {} seconds",
                totalPolls,
                eventsDetected,
                averagePollTimeUs,
                uptime > 0 ? static_cast<double>(totalPolls) / uptime : 0.0,
                uptime);
        }
    };

    static inline PollStatistics _pollStats;
    static inline Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::GROUP_MANAGER> _pollStatsMutex;
};

/**
 * @class PlayerbotWorldScript
 * @brief WorldScript for global group state polling
 *
 * This script runs the polling loop for all active groups with bots.
 * It's the alternative to having a GroupScript::OnUpdate() hook
 * (which doesn't exist in TrinityCore's GroupScript).
 *
 * **Polling Strategy**:
 * - Only poll groups that have at least one bot member
 * - Poll interval: 100ms (configurable)
 * - Batch processing: Multiple groups per update
 *
 * **Performance**:
 * - 500 bot groups: ~100,000 cycles/second
 * - Modern CPU @ 3GHz: ~0.03% CPU usage
 * - <10MB memory for state cache
 */
class TC_GAME_API PlayerbotWorldScript : public WorldScript
{
public:
    PlayerbotWorldScript();
    ~PlayerbotWorldScript();

    /**
     * World update hook
     * Called from: World::Update() every tick
     *
     * Polls all active bot groups for state changes
     */
    void OnUpdate(uint32 diff);

private:
    uint32 _pollTimer{0};
    static constexpr uint32 POLL_INTERVAL_MS = 100;  // Poll every 100ms

    // Helper: Check if group has any bots
    static bool GroupHasBots(Group const* group);
};

} // namespace Playerbot

#endif // PLAYERBOT_GROUP_SCRIPT_H
