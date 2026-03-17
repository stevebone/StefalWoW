/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
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

#ifndef _LFGBOTMANAGER_H
#define _LFGBOTMANAGER_H

#include <chrono>
#include "Common.h"
#include "Threading/LockHierarchy.h"
#include "ObjectGuid.h"
#include "LFG.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <mutex>
#include <atomic>

class Player;

namespace lfg
{
    struct LfgProposal;
}

/**
 * @brief Manages automatic bot recruitment for LFG (Looking For Group) system
 *
 * This manager monitors human player queue joins and automatically populates groups
 * with appropriate bots based on role requirements. It handles:
 * - Detection of missing roles in queued groups
 * - Selection and queueing of suitable bots
 * - Automatic proposal acceptance for bots
 * - Role check confirmation for bots
 * - Tracking of bot assignments to prevent double-queueing
 *
 * Thread-safe singleton implementation using Meyer's singleton pattern.
 */
class TC_GAME_API LFGBotManager final 
{
private:
    LFGBotManager();
    ~LFGBotManager();

public:
    // Singleton access (Meyer's singleton - thread-safe in C++11+)
    static LFGBotManager* instance();

    // Delete copy/move constructors and assignment operators
    LFGBotManager(LFGBotManager const&) = delete;
    LFGBotManager(LFGBotManager&&) = delete;
    LFGBotManager& operator=(LFGBotManager const&) = delete;
    LFGBotManager& operator=(LFGBotManager&&) = delete;

    // ILFGBotManager interface implementation

    /**
     * @brief Initialize the LFG Bot Manager
     * Must be called once during server startup
     */
    void Initialize();

    /**
     * @brief Shutdown and cleanup the LFG Bot Manager
     * Called during server shutdown
     */
    void Shutdown();

    /**
     * @brief Update manager state (called from world update loop)
     * @param diff Time elapsed since last update in milliseconds
     */
    void Update(uint32 diff);

    /**
     * @brief Called when a human player joins the LFG queue
     * Triggers bot recruitment if needed
     *
     * @param player The human player joining queue
     * @param playerRole The role(s) the player selected (PLAYER_ROLE_TANK/HEALER/DAMAGE)
     * @param dungeons Set of dungeon IDs the player queued for
     */
    void OnPlayerJoinQueue(Player* player, uint8 playerRole, lfg::LfgDungeonSet const& dungeons);

    /**
     * @brief Called when a player (human or bot) leaves the LFG queue
     * Cleanup bot assignments if needed
     *
     * @param playerGuid GUID of the player leaving queue
     */
    void OnPlayerLeaveQueue(ObjectGuid playerGuid);

    /**
     * @brief Called when an LFG proposal is received
     * Bots automatically accept proposals
     *
     * @param proposalId The proposal ID
     * @param proposal The proposal data structure
     */
    void OnProposalReceived(uint32 proposalId, lfg::LfgProposal const& proposal);

    /**
     * @brief Called when a role check begins
     * Bots automatically confirm their assigned roles
     *
     * @param groupGuid The group GUID
     * @param botGuid The bot GUID (if specific bot, otherwise empty for all bots)
     */
    void OnRoleCheckReceived(ObjectGuid groupGuid, ObjectGuid botGuid = ObjectGuid::Empty);

    /**
     * @brief Called when a group is formed successfully
     * Cleanup tracking data for the group
     *
     * @param groupGuid The group GUID
     */
    void OnGroupFormed(ObjectGuid groupGuid);

    /**
     * @brief Called when a proposal fails or is declined
     * Remove bots from queue and allow them to be selected again
     *
     * @param proposalId The failed proposal ID
     */
    void OnProposalFailed(uint32 proposalId);

    /**
     * @brief Manually populate queue with bots for a specific player
     * Used for testing or manual control
     *
     * @param playerGuid The player to populate bots for
     * @param neededRoles Bitmask of needed roles (PLAYER_ROLE_TANK | PLAYER_ROLE_HEALER | PLAYER_ROLE_DAMAGE)
     * @param dungeons Set of dungeons to queue for
     * @return Number of bots successfully queued
     */
    uint32 PopulateQueue(ObjectGuid playerGuid, uint8 neededRoles, lfg::LfgDungeonSet const& dungeons);

    /**
     * @brief Check if a bot is currently assigned to an LFG queue
     *
     * @param botGuid The bot's GUID
     * @return true if bot is queued, false otherwise
     */
    bool IsBotQueued(ObjectGuid botGuid) const;

    /**
     * @brief Get statistics about current bot assignments
     *
     * @param totalQueued Output: Total number of bots currently queued
     * @param totalAssignments Output: Total number of active human->bot assignments
     */
    void GetStatistics(uint32& totalQueued, uint32& totalAssignments) const;

    /**
     * @brief Enable or disable the LFG bot system
     *
     * @param enable true to enable, false to disable
     */
    void SetEnabled(bool enable);

    /**
     * @brief Check if the LFG bot system is enabled
     *
     * @return true if enabled, false otherwise
     */
    bool IsEnabled() const { return _enabled; }

    /**
     * @brief Clean up stale queue assignments
     * Removes bots that have been queued for too long without forming a group
     */
    void CleanupStaleAssignments();

    /**
     * @brief Queue a JIT-created bot for a specific dungeon
     * Public interface for QueueStatePoller to queue bots after JIT creation
     *
     * @param bot The bot player object
     * @param dungeonId The dungeon ID to queue for
     * @return true if successfully queued, false otherwise
     */
    bool QueueJITBot(Player* bot, uint32 dungeonId);

private:
    /**
     * @brief Calculate which roles are needed to complete a group
     *
     * @param humanRoles The roles covered by human player(s)
     * @param tanksNeeded Output: Number of tanks needed
     * @param healersNeeded Output: Number of healers needed
     * @param dpsNeeded Output: Number of DPS needed
     */
    void CalculateNeededRoles(uint8 humanRoles,
                              uint8& tanksNeeded, uint8& healersNeeded, uint8& dpsNeeded) const;

    /**
     * @brief Queue a bot for LFG with specific role
     *
     * @param bot The bot player object
     * @param role The role to queue as (PLAYER_ROLE_TANK/HEALER/DAMAGE)
     * @param dungeons Set of dungeons to queue for
     * @return true if successfully queued, false otherwise
     */
    bool QueueBot(Player* bot, uint8 role, lfg::LfgDungeonSet const& dungeons);

    /**
     * @brief Remove a bot from the LFG queue
     *
     * @param bot The bot player object
     */
    void RemoveBotFromQueue(Player* bot);

    /**
     * @brief Get the level range for a dungeon
     *
     * @param dungeonId The dungeon ID
     * @param minLevel Output: Minimum level
     * @param maxLevel Output: Maximum level
     * @return true if dungeon found, false otherwise
     */
    bool GetDungeonLevelRange(uint32 dungeonId, uint8& minLevel, uint8& maxLevel) const;

    /**
     * @brief Register a bot assignment to a human player
     *
     * @param humanGuid The human player's GUID
     * @param botGuid The bot's GUID
     * @param role The role assigned to the bot
     * @param dungeons The dungeons queued for
     */
    void RegisterBotAssignment(ObjectGuid humanGuid, ObjectGuid botGuid, uint8 role,
                               lfg::LfgDungeonSet const& dungeons);

    /**
     * @brief Unregister a bot assignment
     *
     * @param botGuid The bot's GUID
     */
    void UnregisterBotAssignment(ObjectGuid botGuid);

    /**
     * @brief Get all bots assigned to a specific human player
     *
     * @param humanGuid The human player's GUID
     * @return Vector of bot GUIDs
     */
    std::vector<ObjectGuid> GetAssignedBots(ObjectGuid humanGuid) const;

    /**
     * @brief Unregister all bots assigned to a human player
     *
     * @param humanGuid The human player's GUID
     */
    void UnregisterAllBotsForPlayer(ObjectGuid humanGuid);

    // Data structures

    /**
     * @brief Information about a bot queued for LFG
     */
    struct BotQueueInfo
    {
        ObjectGuid humanPlayerGuid;     ///< The human player this bot is grouped with
        uint8 assignedRole;              ///< Role assigned (PLAYER_ROLE_TANK/HEALER/DAMAGE)
        uint32 primaryDungeonId;         ///< Primary dungeon ID from the set
        time_t queueTime;                ///< When the bot was queued
        lfg::LfgDungeonSet dungeons;    ///< Full set of dungeons queued for
        uint32 proposalId;               ///< Current proposal ID (0 if no active proposal)

        BotQueueInfo() : assignedRole(0), primaryDungeonId(0), queueTime(0), proposalId(0) {}

        BotQueueInfo(ObjectGuid humanGuid, uint8 role, lfg::LfgDungeonSet const& dungs, uint32 dungId)
            : humanPlayerGuid(humanGuid), assignedRole(role), primaryDungeonId(dungId),
              queueTime(time(nullptr)), dungeons(dungs), proposalId(0) {}
    };

    /**
     * @brief Information about a human player with bot assignments
     */
    struct HumanPlayerQueueInfo
    {
        std::vector<ObjectGuid> assignedBots;  ///< Bots assigned to this player
        uint8 playerRole;                       ///< The human's role
        lfg::LfgDungeonSet dungeons;           ///< Dungeons queued for
        time_t queueTime;                       ///< When the player queued

        HumanPlayerQueueInfo() : playerRole(0), queueTime(0) {}

        HumanPlayerQueueInfo(uint8 role, lfg::LfgDungeonSet const& dungs)
            : playerRole(role), dungeons(dungs), queueTime(time(nullptr)) {}
    };

    // Member variables

    /// Mutex for thread-safe access to all data structures
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::GROUP_MANAGER> _mutex;

    /// Map of bot GUID -> queue information
    std::unordered_map<ObjectGuid, BotQueueInfo> _queuedBots;

    /// Map of human player GUID -> queue information with assigned bots
    std::unordered_map<ObjectGuid, HumanPlayerQueueInfo> _humanPlayers;

    /// Map of proposal ID -> set of bot GUIDs involved
    std::unordered_map<uint32, std::unordered_set<ObjectGuid>> _proposalBots;

    /// Whether the LFG bot system is enabled
    std::atomic<bool> _enabled;

    /// Update accumulator for periodic cleanup
    uint32 _updateAccumulator;

    /// Pending JIT bot info - bots that were created but need to be queued once loaded
    struct PendingJITBot
    {
        ObjectGuid botGuid;
        ObjectGuid humanPlayerGuid;
        lfg::LfgDungeonSet dungeons;
        std::chrono::steady_clock::time_point createdAt;
        uint32 retryCount = 0;
    };

    /// Pending JIT bots waiting to be queued
    std::vector<PendingJITBot> _pendingJITBots;

    /// Process pending JIT bots in Update
    void ProcessPendingJITBots();

    /// Max retries for pending bot queue
    // RELIABILITY FIX: Increased from 30 (3s) to 100 (10s) to allow JIT bots more time to load
    // JIT bots need time to: create character -> login -> apply gear -> be findable via ObjectAccessor
    static constexpr uint32 MAX_PENDING_RETRIES = 100;  // ~10 seconds at 100ms interval

    /// Pending check interval
    static constexpr uint32 PENDING_CHECK_INTERVAL = 100;  // 100ms

    /// Pending check accumulator
    uint32 _pendingCheckAccumulator = 0;

    /// Cleanup interval in milliseconds (5 minutes)
    static constexpr uint32 CLEANUP_INTERVAL = 5 * MINUTE * IN_MILLISECONDS;

    /// Maximum time a bot can be queued before being considered stale (15 minutes)
    static constexpr time_t MAX_QUEUE_TIME = 15 * MINUTE;

    /// Whether the manager has been initialized
    bool _initialized;
};

#define sLFGBotManager LFGBotManager::instance()

#endif // _LFGBOTMANAGER_H
