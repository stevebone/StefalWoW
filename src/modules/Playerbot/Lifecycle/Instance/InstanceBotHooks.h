/*
 * Copyright (C) 2008-2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PLAYERBOT_INSTANCE_BOT_HOOKS_H
#define PLAYERBOT_INSTANCE_BOT_HOOKS_H

#include "Define.h"
#include "ObjectGuid.h"
#include "SharedDefines.h"  // For TeamId enum (TEAM_ALLIANCE, TEAM_HORDE)
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <set>
#include <unordered_map>
#include <vector>

class Player;
class Group;
class Battleground;
class BattlegroundQueue;

namespace Playerbot
{

/**
 * @class InstanceBotHooks
 * @brief Static hook interface for minimal core integration
 *
 * This class provides the module-side hook implementations that are called
 * from minimal #ifdef BUILD_PLAYERBOT blocks in TrinityCore core files.
 *
 * Design Philosophy:
 * - All hooks are static methods (no instance required)
 * - All logic stays in the module (hooks just forward to orchestrator)
 * - Core changes are minimal (just hook calls)
 * - Thread-safe (called from various core contexts)
 *
 * Integration Points:
 * 1. LFG System (LFGMgr.cpp)
 *    - OnPlayerJoinLfg: When player queues for dungeon
 *    - OnPlayerLeaveLfg: When player leaves queue
 *    - OnLfgGroupFormed: When LFG group is ready
 *
 * 2. Battleground System (BattlegroundQueue.cpp, BattlegroundMgr.cpp)
 *    - OnPlayerJoinBattleground: When player queues for BG
 *    - OnPlayerLeaveBattleground: When player leaves BG queue
 *    - OnBattlegroundStarting: When BG is about to start
 *    - OnBattlegroundEnded: When BG ends
 *
 * 3. Arena System (ArenaTeam.cpp, ArenaQueue)
 *    - OnPlayerJoinArena: When player queues for arena
 *    - OnArenaMatchStarting: When arena match is starting
 *    - OnArenaMatchEnded: When arena match ends
 *
 * 4. Raid/Instance System
 *    - OnPlayerEnterInstance: When player enters instance
 *    - OnPlayerLeaveInstance: When player leaves instance
 *    - OnInstanceReset: When instance is reset
 *
 * Usage in Core (minimal change):
 * @code
 * #ifdef BUILD_PLAYERBOT
 * #include "Lifecycle/Instance/InstanceBotHooks.h"
 * #endif
 *
 * void LFGMgr::JoinLfg(Player* player, ...)
 * {
 *     // ... existing code ...
 *
 *     #ifdef BUILD_PLAYERBOT
 *     Playerbot::InstanceBotHooks::OnPlayerJoinLfg(player, dungeons, roles);
 *     #endif
 * }
 * @endcode
 */
class TC_GAME_API InstanceBotHooks
{
public:
    // ========================================================================
    // INITIALIZATION
    // ========================================================================

    /**
     * @brief Initialize the hook system
     * @return true if initialization successful
     *
     * Called during module initialization to set up callbacks.
     */
    static bool Initialize();

    /**
     * @brief Shutdown the hook system
     *
     * Called during module shutdown to clean up.
     */
    static void Shutdown();

    /**
     * @brief Check if hooks are enabled
     * @return true if instance bot system is enabled
     */
    static bool IsEnabled();

    // ========================================================================
    // LFG HOOKS (Called from LFGMgr.cpp)
    // ========================================================================

    /**
     * @brief Called when a player joins the LFG queue
     * @param player The player joining the queue
     * @param dungeons Set of dungeon IDs the player queued for
     * @param roles The roles the player selected (PLAYER_ROLE_TANK/HEALER/DAMAGE)
     *
     * This hook triggers bot reservation for the dungeon.
     * The orchestrator will prepare bots based on the content requirements.
     */
    static void OnPlayerJoinLfg(
        Player* player,
        std::set<uint32> const& dungeons,
        uint8 roles
    );

    /**
     * @brief Called when a player leaves the LFG queue
     * @param player The player leaving the queue
     *
     * This cancels any pending bot reservations for this player.
     */
    static void OnPlayerLeaveLfg(Player* player);

    /**
     * @brief Called when an LFG group is formed and ready
     * @param groupGuid The GUID of the formed group
     * @param dungeonId The dungeon ID the group will enter
     * @param players The player GUIDs in the group
     *
     * At this point, bots should be added to fill remaining slots.
     */
    static void OnLfgGroupFormed(
        ObjectGuid groupGuid,
        uint32 dungeonId,
        std::vector<ObjectGuid> const& players
    );

    /**
     * @brief Called when LFG proposal is accepted and group teleports
     * @param groupGuid The group GUID
     * @param dungeonId The dungeon ID
     *
     * Final hook before group enters dungeon - ensure bots are ready.
     */
    static void OnLfgProposalAccepted(
        ObjectGuid groupGuid,
        uint32 dungeonId
    );

    // ========================================================================
    // BATTLEGROUND HOOKS (Called from BattlegroundQueue.cpp)
    // ========================================================================

    /**
     * @brief Called when a player joins a battleground queue
     * @param player The player joining
     * @param bgTypeId The battleground type ID
     * @param bracketId The level bracket
     * @param asGroup Whether joining as a group
     *
     * This triggers bot preparation for the battleground.
     * For large BGs (AV, IoC), this may trigger JIT bot creation.
     */
    static void OnPlayerJoinBattleground(
        Player* player,
        uint32 bgTypeId,
        uint32 bracketId,
        bool asGroup
    );

    /**
     * @brief Called when a player leaves battleground queue
     * @param player The player leaving
     * @param bgTypeId The battleground type
     */
    static void OnPlayerLeaveBattlegroundQueue(
        Player* player,
        uint32 bgTypeId
    );

    /**
     * @brief Called when battleground queue is checking for matches
     * @param bgTypeId The battleground type
     * @param bracketId The level bracket
     * @param allianceInQueue Number of Alliance players in queue
     * @param hordeInQueue Number of Horde players in queue
     * @param minPlayersPerTeam Minimum players needed per team
     * @param maxPlayersPerTeam Maximum players per team
     * @return true if bots should be added to start the BG
     *
     * This is called periodically to check if bots should fill the BG.
     */
    static bool OnBattlegroundQueueUpdate(
        uint32 bgTypeId,
        uint32 bracketId,
        uint32 allianceInQueue,
        uint32 hordeInQueue,
        uint32 minPlayersPerTeam,
        uint32 maxPlayersPerTeam
    );

    /**
     * @brief Called when a battleground is about to start
     * @param bg The battleground instance
     * @param allianceCount Current Alliance player count
     * @param hordeCount Current Horde player count
     *
     * Final opportunity to add bots before BG starts.
     */
    static void OnBattlegroundStarting(
        Battleground* bg,
        uint32 allianceCount,
        uint32 hordeCount
    );

    /**
     * @brief Called when a battleground ends
     * @param bg The battleground instance
     * @param winnerTeam The winning team (ALLIANCE/HORDE)
     *
     * Release bots back to pool or for recycling.
     */
    static void OnBattlegroundEnded(
        Battleground* bg,
        uint32 winnerTeam
    );

    /**
     * @brief Called when a player leaves a battleground
     * @param player The player leaving
     * @param bg The battleground
     */
    static void OnPlayerLeftBattleground(
        Player* player,
        Battleground* bg
    );

    // ========================================================================
    // ARENA HOOKS (Called from ArenaTeam.cpp / Arena system)
    // ========================================================================

    /**
     * @brief Called when a player/team joins arena queue
     * @param player The player joining (team leader for rated)
     * @param arenaType The arena type (2v2, 3v3, 5v5, Solo Shuffle)
     * @param bracketId The bracket ID
     * @param isRated Whether this is rated arena
     * @param teamMembers Current team members (for rated)
     */
    static void OnPlayerJoinArena(
        Player* player,
        uint32 arenaType,
        uint32 bracketId,
        bool isRated,
        std::vector<ObjectGuid> const& teamMembers
    );

    /**
     * @brief Called when player leaves arena queue
     * @param player The player leaving
     * @param arenaType The arena type
     */
    static void OnPlayerLeaveArenaQueue(
        Player* player,
        uint32 arenaType
    );

    /**
     * @brief Called when arena match is being prepared
     * @param arenaType The arena type
     * @param bracketId The bracket
     * @param team1Players Team 1 player GUIDs
     * @param team2Players Team 2 player GUIDs
     * @param team1NeedsPlayers How many more players team 1 needs
     * @param team2NeedsPlayers How many more players team 2 needs
     * @return true if bots were successfully assigned
     */
    static bool OnArenaMatchPreparing(
        uint32 arenaType,
        uint32 bracketId,
        std::vector<ObjectGuid>& team1Players,
        std::vector<ObjectGuid>& team2Players,
        uint32 team1NeedsPlayers,
        uint32 team2NeedsPlayers
    );

    /**
     * @brief Called when arena match ends
     * @param arenaType The arena type
     * @param winnerTeam The winning team (0 or 1)
     * @param team1Players Team 1 players
     * @param team2Players Team 2 players
     */
    static void OnArenaMatchEnded(
        uint32 arenaType,
        uint32 winnerTeam,
        std::vector<ObjectGuid> const& team1Players,
        std::vector<ObjectGuid> const& team2Players
    );

    // ========================================================================
    // RAID/INSTANCE HOOKS
    // ========================================================================

    /**
     * @brief Called when a player enters an instance
     * @param player The player entering
     * @param mapId The map ID
     * @param instanceId The instance ID
     * @param isRaid Whether this is a raid instance
     */
    static void OnPlayerEnterInstance(
        Player* player,
        uint32 mapId,
        uint32 instanceId,
        bool isRaid
    );

    /**
     * @brief Called when a player leaves an instance
     * @param player The player leaving
     * @param mapId The map ID
     * @param instanceId The instance ID
     */
    static void OnPlayerLeaveInstance(
        Player* player,
        uint32 mapId,
        uint32 instanceId
    );

    /**
     * @brief Called when an instance is being reset
     * @param mapId The map ID
     * @param instanceId The instance ID
     *
     * All bots in this instance should be released.
     */
    static void OnInstanceReset(
        uint32 mapId,
        uint32 instanceId
    );

    /**
     * @brief Called when raid group needs bots filled
     * @param leader The raid leader
     * @param raidId The raid content ID
     * @param currentMembers Current group member GUIDs
     * @param memberRoles Map of member GUID to role
     */
    static void OnRaidNeedsBots(
        Player* leader,
        uint32 raidId,
        std::vector<ObjectGuid> const& currentMembers,
        std::map<ObjectGuid, uint8> const& memberRoles
    );

    // ========================================================================
    // GROUP HOOKS
    // ========================================================================

    /**
     * @brief Called when a group is disbanded
     * @param groupGuid The group GUID
     *
     * Release any bots assigned to this group.
     */
    static void OnGroupDisbanded(ObjectGuid groupGuid);

    /**
     * @brief Called when group leader changes
     * @param group The group
     * @param newLeader The new leader
     */
    static void OnGroupLeaderChanged(
        Group* group,
        Player* newLeader
    );

    // ========================================================================
    // UTILITY FUNCTIONS
    // ========================================================================

    /**
     * @brief Check if a GUID belongs to a pool bot
     * @param guid The GUID to check
     * @return true if this is a pool bot
     */
    static bool IsPoolBot(ObjectGuid guid);

    /**
     * @brief Get estimated wait time for content
     * @param contentType The content type (0=Dungeon, 1=Raid, 2=BG, 3=Arena)
     * @param contentId The content ID
     * @return Estimated wait time in seconds
     */
    static uint32 GetEstimatedWaitTime(
        uint8 contentType,
        uint32 contentId
    );

    /**
     * @brief Get pool statistics for display
     * @param outReady Output: ready bot count
     * @param outAssigned Output: assigned bot count
     * @param outTotal Output: total pool size
     */
    static void GetPoolStats(
        uint32& outReady,
        uint32& outAssigned,
        uint32& outTotal
    );

    // ========================================================================
    // CALLBACKS FOR ASYNC OPERATIONS
    // ========================================================================

    /**
     * @brief Callback type for bot assignment completion
     */
    using BotAssignmentCallback = std::function<void(
        bool success,
        std::vector<ObjectGuid> const& assignedBots
    )>;

    /**
     * @brief Callback type for dual-faction bot assignment (PvP)
     */
    using PvPBotAssignmentCallback = std::function<void(
        bool success,
        std::vector<ObjectGuid> const& allianceBots,
        std::vector<ObjectGuid> const& hordeBots
    )>;

    /**
     * @brief Register callback for pending dungeon request
     * @param playerGuid The requesting player
     * @param callback The callback to invoke when bots are ready
     */
    static void RegisterDungeonCallback(
        ObjectGuid playerGuid,
        BotAssignmentCallback callback
    );

    /**
     * @brief Register callback for pending battleground request
     * @param bgTypeId The battleground type
     * @param bracketId The bracket
     * @param callback The callback to invoke when bots are ready
     */
    static void RegisterBattlegroundCallback(
        uint32 bgTypeId,
        uint32 bracketId,
        PvPBotAssignmentCallback callback
    );

    // ========================================================================
    // UPDATE / PROCESSING
    // ========================================================================

    /**
     * @brief Update method called periodically to process pending bot queues
     * @param diff Time since last update in ms
     *
     * This processes bots that have been created by JIT and need to:
     * 1. Be logged in
     * 2. Be queued for their respective BG after login completes
     */
    static void Update(uint32 diff);

    /**
     * @brief Process pending BG queue entries
     *
     * Checks if JIT-created bots are now logged in and queues them for BG.
     */
    static void ProcessPendingBGQueues();

private:
    InstanceBotHooks() = delete;
    ~InstanceBotHooks() = delete;

    // Internal state
    static std::atomic<bool> _initialized;
    static std::atomic<bool> _enabled;

    // Callback storage
    static std::mutex _callbackMutex;
    static std::unordered_map<ObjectGuid, BotAssignmentCallback> _dungeonCallbacks;
    static std::unordered_map<uint64, PvPBotAssignmentCallback> _bgCallbacks;

    // Helper to create BG callback key
    static uint64 MakeBGCallbackKey(uint32 bgTypeId, uint32 bracketId)
    {
        return (static_cast<uint64>(bgTypeId) << 32) | bracketId;
    }

    // ========================================================================
    // PENDING BG QUEUE - Bots waiting to login and be queued for BG
    // ========================================================================

    /**
     * @struct PendingBGQueueEntry
     * @brief Tracks a bot that needs to be logged in and queued for BG
     *
     * JIT creates bot character records, but they need to be:
     * 1. Logged in via BotWorldSessionMgr
     * 2. Queued for BG once they're in world
     */
    struct PendingBGQueueEntry
    {
        ObjectGuid botGuid;                         ///< Bot's character GUID
        ObjectGuid humanPlayerGuid;                 ///< Human player this bot is assigned to (for tracking)
        uint32 accountId = 0;                       ///< Bot's account ID
        uint32 bgTypeId = 0;                        ///< Target battleground type
        uint32 bracketId = 0;                       ///< BG bracket
        TeamId team = TEAM_ALLIANCE;                ///< Bot's team
        std::chrono::steady_clock::time_point createdAt;  ///< When entry was created
        std::chrono::steady_clock::time_point loginQueuedAt;  ///< When login was queued
        std::chrono::steady_clock::time_point loginCompletedAt;  ///< When bot was first seen in-world
        bool loginQueued = false;                   ///< Has login been queued?
        bool loginCompleted = false;                ///< Has bot appeared in world?
        uint32 retryCount = 0;                      ///< Number of queue attempts

        bool IsExpired() const
        {
            auto elapsed = std::chrono::steady_clock::now() - createdAt;
            return elapsed > std::chrono::minutes(2);  // 2 minute timeout
        }

        bool IsLoginTimedOut() const
        {
            if (!loginQueued) return false;
            auto elapsed = std::chrono::steady_clock::now() - loginQueuedAt;
            return elapsed > std::chrono::seconds(30);  // 30 second login timeout
        }

        /// Check if bot has stabilized after login (ready for BG queue)
        /// Wait 1 second after bot appears in world to ensure full initialization
        bool IsReadyForBGQueue() const
        {
            if (!loginCompleted) return false;
            auto elapsed = std::chrono::steady_clock::now() - loginCompletedAt;
            return elapsed > std::chrono::milliseconds(1000);  // 1 second stabilization delay
        }
    };

    static std::mutex _pendingBGMutex;
    static std::vector<PendingBGQueueEntry> _pendingBGQueue;
    static uint32 _updateAccumulator;
    static constexpr uint32 UPDATE_INTERVAL_MS = 500;  // Check every 500ms
};

} // namespace Playerbot

#endif // PLAYERBOT_INSTANCE_BOT_HOOKS_H
