/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef _BGBOTMANAGER_H
#define _BGBOTMANAGER_H

#include "Common.h"
#include "Threading/LockHierarchy.h"
#include "ObjectGuid.h"
#include "Battleground.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <mutex>
#include <atomic>

class Player;
class Group;

namespace Playerbot
{

/**
 * @brief Manages automatic bot recruitment for Battleground queues
 *
 * This manager monitors human player BG queue joins and automatically populates
 * teams with appropriate bots based on faction requirements. It handles:
 * - Detection of human players in BG queue
 * - Selection and queueing of suitable bots (faction-appropriate)
 * - Automatic invitation acceptance for bots
 * - Teleportation to battleground on match start
 * - Tracking of bot assignments to prevent double-queueing
 *
 * Thread-safe singleton implementation using Meyer's singleton pattern.
 */
class TC_GAME_API BGBotManager final
{
private:
    BGBotManager();
    ~BGBotManager();

public:
    // Singleton access (Meyer's singleton - thread-safe in C++11+)
    static BGBotManager* instance();

    // Delete copy/move constructors
    BGBotManager(BGBotManager const&) = delete;
    BGBotManager(BGBotManager&&) = delete;
    BGBotManager& operator=(BGBotManager const&) = delete;
    BGBotManager& operator=(BGBotManager&&) = delete;

    // ============================================================================
    // IBGBotManager INTERFACE
    // ============================================================================

    void Initialize();
    void Shutdown();
    void Update(uint32 diff);

    void OnPlayerJoinQueue(Player* player, BattlegroundTypeId bgTypeId,
                           BattlegroundBracketId bracket, bool asGroup);
    void OnPlayerLeaveQueue(ObjectGuid playerGuid);
    void OnInvitationReceived(ObjectGuid playerGuid, uint32 bgInstanceGuid);
    void OnBattlegroundStart(Battleground* bg);
    void OnBattlegroundEnd(Battleground* bg, Team winnerTeam);

    /**
     * @brief Populate a battleground with bots (teleport invited + fill empty slots)
     * @param bg The battleground instance
     *
     * Can be called during WAIT_JOIN (prep phase) so bots are present before
     * gates open. Also called by OnBattlegroundStart for late population.
     * Safe to call multiple times - won't duplicate bots.
     */
    void PopulateBattleground(Battleground* bg);

    uint32 PopulateQueue(ObjectGuid playerGuid, BattlegroundTypeId bgTypeId,
                         BattlegroundBracketId bracket,
                         uint32 neededAlliance, uint32 neededHorde);

    bool IsBotQueued(ObjectGuid botGuid) const;
    void GetStatistics(uint32& totalQueued, uint32& totalAssignments) const;
    void SetEnabled(bool enable);
    bool IsEnabled() const { return _enabled; }
    void CleanupStaleAssignments();

    // ============================================================================
    // ADDITIONAL METHODS
    // ============================================================================

    /**
     * @brief Get the team size for a battleground type
     * @param bgTypeId The battleground type
     * @return Team size (10 for WSG/AB, 15 for AV, etc.)
     */
    uint32 GetBGTeamSize(BattlegroundTypeId bgTypeId) const;

    /**
     * @brief Get the minimum players needed to start
     * @param bgTypeId The battleground type
     * @return Minimum player count
     */
    uint32 GetBGMinPlayers(BattlegroundTypeId bgTypeId) const;

    /**
     * @brief Queue a specific bot for battleground (used by InstanceBotHooks)
     * @param bot The bot player object (must be logged in and in world)
     * @param bgTypeId The battleground type
     * @param bracket The BG bracket
     * @return true if successfully queued
     *
     * NOTE: This is a public wrapper for JIT bot integration.
     * The bot must already be logged in before calling this.
     */
    bool QueueBotForBG(Player* bot, BattlegroundTypeId bgTypeId, BattlegroundBracketId bracket);

    /**
     * @brief Queue a specific bot for battleground with human player tracking
     * @param bot The bot player object (must be logged in and in world)
     * @param bgTypeId The battleground type
     * @param bracket The BG bracket
     * @param humanPlayerGuid The human player this bot is associated with (for invitation tracking)
     * @return true if successfully queued
     *
     * This version registers the bot in _queuedBots so that OnInvitationReceived
     * will properly auto-accept the BG invitation and the bot will enter the BG.
     */
    bool QueueBotForBGWithTracking(Player* bot, BattlegroundTypeId bgTypeId,
                                    BattlegroundBracketId bracket, ObjectGuid humanPlayerGuid);

    /**
     * @brief Get the first human player queued for a specific BG type and bracket
     * @param bgTypeId The battleground type
     * @param bracket The BG bracket
     * @return Human player GUID if found, otherwise Empty
     *
     * Used by QueueStatePoller/InstanceBotPool to associate warm pool bots
     * with the human player that triggered the queue (for invitation tracking).
     */
    ObjectGuid GetQueuedHumanForBG(BattlegroundTypeId bgTypeId, BattlegroundBracketId bracket) const;

private:
    // ============================================================================
    // HELPER METHODS
    // ============================================================================

    /**
     * @brief Calculate how many bots are needed for each faction
     */
    void CalculateNeededBots(BattlegroundTypeId bgTypeId, Team humanTeam,
                              uint32& allianceNeeded, uint32& hordeNeeded) const;

    /**
     * @brief Queue a bot for battleground
     */
    bool QueueBot(Player* bot, BattlegroundTypeId bgTypeId, BattlegroundBracketId bracket);

    /**
     * @brief Remove a bot from BG queue
     */
    void RemoveBotFromQueue(Player* bot);

    /**
     * @brief Find available bots for a faction
     */
    std::vector<Player*> FindAvailableBots(Team team, uint8 minLevel, uint8 maxLevel,
                                            uint32 count) const;

    /**
     * @brief Get level range for bracket
     */
    void GetBracketLevelRange(BattlegroundBracketId bracket, uint8& minLevel, uint8& maxLevel) const;

    /**
     * @brief Register bot assignment
     */
    void RegisterBotAssignment(ObjectGuid humanGuid, ObjectGuid botGuid,
                                BattlegroundTypeId bgTypeId, Team team);

    /**
     * @brief Unregister bot assignment
     */
    void UnregisterBotAssignment(ObjectGuid botGuid);

    /**
     * @brief Unregister all bots for a player
     */
    void UnregisterAllBotsForPlayer(ObjectGuid humanGuid);

    /**
     * @brief Check if bot is available for BG
     */
    bool IsBotAvailable(Player* bot) const;

    /**
     * @brief Add a bot directly to an active battleground
     * @param bot The bot to add
     * @param bg The battleground instance
     * @param team The team to join
     * @return true if successfully added
     */
    bool AddBotDirectlyToBG(Player* bot, Battleground* bg, Team team);

    /**
     * @brief Internal populate logic - caller must hold _mutex
     */
    void PopulateBattlegroundLocked(Battleground* bg);

    /**
     * @brief Remove excess bots from an overpopulated team - caller must hold _mutex
     * @param bg The battleground instance
     * @param team The overpopulated team
     * @param excessCount How many bots to remove
     */
    void TrimExcessBotsLocked(Battleground* bg, Team team, uint32 excessCount);

    // ============================================================================
    // DATA STRUCTURES
    // ============================================================================

    /**
     * @brief Information about a bot queued for BG
     */
    struct BotQueueInfo
    {
        ObjectGuid humanPlayerGuid;     ///< Human player this bot is associated with
        BattlegroundTypeId bgTypeId;    ///< Battleground type
        Team team;                       ///< Faction
        time_t queueTime;                ///< When queued
        uint32 bgInstanceGuid;           ///< BG instance if invited
        bool needsTeleport;              ///< True if bot received invitation and needs teleport

        BotQueueInfo() : bgTypeId(BATTLEGROUND_TYPE_NONE), team(TEAM_OTHER),
                         queueTime(0), bgInstanceGuid(0), needsTeleport(false) {}

        BotQueueInfo(ObjectGuid humanGuid, BattlegroundTypeId bgType, Team t)
            : humanPlayerGuid(humanGuid), bgTypeId(bgType), team(t),
              queueTime(time(nullptr)), bgInstanceGuid(0), needsTeleport(false) {}
    };

    /**
     * @brief Information about a human player with bot assignments
     */
    struct HumanPlayerQueueInfo
    {
        std::vector<ObjectGuid> assignedBots;
        BattlegroundTypeId bgTypeId;
        BattlegroundBracketId bracket;
        Team team;
        time_t queueTime;

        HumanPlayerQueueInfo() : bgTypeId(BATTLEGROUND_TYPE_NONE),
                                  bracket(BG_BRACKET_ID_FIRST),
                                  team(TEAM_OTHER), queueTime(0) {}

        HumanPlayerQueueInfo(BattlegroundTypeId bgType, BattlegroundBracketId b, Team t)
            : bgTypeId(bgType), bracket(b), team(t), queueTime(time(nullptr)) {}
    };

    // ============================================================================
    // MEMBER VARIABLES
    // ============================================================================

    mutable OrderedRecursiveMutex<LockOrder::GROUP_MANAGER> _mutex;

    /// Map of bot GUID -> queue information
    std::unordered_map<ObjectGuid, BotQueueInfo> _queuedBots;

    /// Map of human player GUID -> queue information
    std::unordered_map<ObjectGuid, HumanPlayerQueueInfo> _humanPlayers;

    /// Map of BG instance GUID -> set of bot GUIDs
    std::unordered_map<uint32, std::unordered_set<ObjectGuid>> _bgInstanceBots;

    /// Map of BG instance GUID -> time when first human entered (for delayed bot teleport)
    std::unordered_map<uint32, uint32> _bgHumanEntryTime;

    /// Whether the system is enabled
    std::atomic<bool> _enabled;

    /// Update accumulator for periodic cleanup
    uint32 _updateAccumulator;

    /// Update accumulator for invitation polling
    uint32 _invitationCheckAccumulator;

    /// Cleanup interval (5 minutes)
    static constexpr uint32 CLEANUP_INTERVAL = 5 * MINUTE * IN_MILLISECONDS;

    /// Invitation check interval (1 second - frequent check for quick BG entry)
    static constexpr uint32 INVITATION_CHECK_INTERVAL = 1 * IN_MILLISECONDS;

    /// Delay before bots teleport after human enters BG (5 seconds)
    static constexpr uint32 BOT_TELEPORT_DELAY = 5 * IN_MILLISECONDS;

    /// Maximum queue time before considered stale (30 minutes)
    static constexpr time_t MAX_QUEUE_TIME = 30 * MINUTE;

    /// Whether initialized
    bool _initialized;

    // ============================================================================
    // POPULATION RETRY SYSTEM
    // ============================================================================

    struct PendingPopulationInfo
    {
        uint32 startTime;
        BattlegroundTypeId bgTypeId;
    };

    /// Map of BG instance GUID -> population retry info
    std::unordered_map<uint32, PendingPopulationInfo> _pendingPopulations;

    /// Accumulator for population retry checks
    uint32 _populationRetryAccumulator = 0;

    /// How often to retry populating BG instances (5 seconds)
    static constexpr uint32 POPULATION_RETRY_INTERVAL = 5 * IN_MILLISECONDS;

    /// Maximum time to keep retrying population (2 minutes - covers full prep phase)
    static constexpr uint32 POPULATION_RETRY_MAX_DURATION = 120 * IN_MILLISECONDS;

    /**
     * @brief Process BG instances that need population retries
     *
     * Warm pool bots may not be fully logged in when OnBattlegroundStart fires.
     * This periodically re-checks and fills missing team slots during the prep phase.
     */
    void ProcessPendingPopulations();

    // ============================================================================
    // INVITATION PROCESSING
    // ============================================================================

    /**
     * @brief Process pending BG invitations for queued bots
     *
     * Since the core BG system doesn't notify the bot module when bots are invited,
     * we poll all queued bots to check if they have pending invitations and auto-accept.
     */
    void ProcessPendingInvitations();
};

} // namespace Playerbot

#define sBGBotManager Playerbot::BGBotManager::instance()

#endif // _BGBOTMANAGER_H
