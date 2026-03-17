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

#include "InstanceBotHooks.h"
#include "InstanceBotOrchestrator.h"
#include "InstanceBotPool.h"
#include "QueueStatePoller.h"
#include "ContentRequirements.h"
#include "JITBotFactory.h"
#include "Player.h"
#include "Group.h"
#include "Battleground.h"
#include "BattlegroundMgr.h"
#include "ObjectAccessor.h"
#include "Log.h"
#include "LFGMgr.h"
#include "Session/BotWorldSessionMgr.h"
#include "PvP/BGBotManager.h"
#include "DatabaseEnv.h"

namespace Playerbot
{

// ============================================================================
// STATIC MEMBER INITIALIZATION
// ============================================================================

std::atomic<bool> InstanceBotHooks::_initialized{false};
std::atomic<bool> InstanceBotHooks::_enabled{false};
std::mutex InstanceBotHooks::_callbackMutex;
std::unordered_map<ObjectGuid, InstanceBotHooks::BotAssignmentCallback> InstanceBotHooks::_dungeonCallbacks;
std::unordered_map<uint64, InstanceBotHooks::PvPBotAssignmentCallback> InstanceBotHooks::_bgCallbacks;
std::mutex InstanceBotHooks::_pendingBGMutex;
std::vector<InstanceBotHooks::PendingBGQueueEntry> InstanceBotHooks::_pendingBGQueue;
uint32 InstanceBotHooks::_updateAccumulator{0};

// ============================================================================
// INITIALIZATION
// ============================================================================

bool InstanceBotHooks::Initialize()
{
    if (_initialized.exchange(true))
    {
        TC_LOG_WARN("playerbots.instance", "InstanceBotHooks::Initialize called multiple times");
        return true;
    }

    TC_LOG_INFO("playerbots.instance", "Initializing Instance Bot Hooks...");

    // Check if orchestrator is available and enabled
    if (sInstanceBotOrchestrator)
    {
        _enabled.store(sInstanceBotOrchestrator->GetConfig().enabled);
    }
    else
    {
        TC_LOG_ERROR("playerbots.instance", "InstanceBotOrchestrator not available for hooks");
        _enabled.store(false);
        _initialized.store(false);
        return false;
    }

    TC_LOG_INFO("playerbots.instance", "Instance Bot Hooks initialized (enabled: {})",
        _enabled.load() ? "yes" : "no");

    return true;
}

void InstanceBotHooks::Shutdown()
{
    if (!_initialized.exchange(false))
        return;

    TC_LOG_INFO("playerbots.instance", "Shutting down Instance Bot Hooks...");

    _enabled.store(false);

    // Clear all pending callbacks
    {
        std::lock_guard<std::mutex> lock(_callbackMutex);
        _dungeonCallbacks.clear();
        _bgCallbacks.clear();
    }

    // Clear pending BG queue
    {
        std::lock_guard<std::mutex> lock(_pendingBGMutex);
        if (!_pendingBGQueue.empty())
        {
            TC_LOG_INFO("playerbots.instance", "Clearing {} pending BG queue entries", _pendingBGQueue.size());
            _pendingBGQueue.clear();
        }
    }

    TC_LOG_INFO("playerbots.instance", "Instance Bot Hooks shutdown complete");
}

bool InstanceBotHooks::IsEnabled()
{
    return _enabled.load() && _initialized.load();
}

// ============================================================================
// LFG HOOKS
// ============================================================================

void InstanceBotHooks::OnPlayerJoinLfg(
    Player* player,
    std::set<uint32> const& dungeons,
    uint8 roles)
{
    if (!IsEnabled() || !player || dungeons.empty())
        return;

    TC_LOG_DEBUG("playerbots.instance", "OnPlayerJoinLfg: Player {} queued for {} dungeons with role {}",
        player->GetName(), dungeons.size(), roles);

    // Get the first dungeon (typically only one for random)
    uint32 dungeonId = *dungeons.begin();

    // Create dungeon request for orchestrator
    DungeonRequest request;
    request.playerGuid = player->GetGUID();
    request.dungeonId = dungeonId;
    request.playerRole = roles;
    request.playerLevel = player->GetLevel();  // Use actual player level for bot matching
    request.playerFaction = player->GetTeam() == ALLIANCE ? Faction::Alliance : Faction::Horde;

    TC_LOG_INFO("playerbots.instance", "OnPlayerJoinLfg: Player {} (level {}, {}) queued for dungeon {}",
        player->GetName(), request.playerLevel,
        request.playerFaction == Faction::Alliance ? "Alliance" : "Horde", dungeonId);

    // Set up callbacks - capture context needed for LFG queue join
    request.onBotsReady = [playerGuid = player->GetGUID(), dungeonId, roles](std::vector<ObjectGuid> const& bots)
    {
        TC_LOG_INFO("playerbots.instance", "Dungeon bots ready for player {}: {} bots assigned",
            playerGuid.ToString(), bots.size());

        // Add each bot to the LFG queue
        uint32 botsAdded = 0;
        for (ObjectGuid const& botGuid : bots)
        {
            Player* bot = ObjectAccessor::FindPlayer(botGuid);
            if (!bot)
            {
                TC_LOG_DEBUG("playerbots.instance", "Bot {} not online yet, skipping LFG queue add",
                    botGuid.ToString());
                continue;
            }

            // Determine bot role based on spec (opposite of what player needs)
            uint8 botRoles = lfg::PLAYER_ROLE_DAMAGE; // Default to DPS

            // If player is tank/healer, bot should be DPS; if player is DPS, bots fill tank/healer
            if (!(roles & lfg::PLAYER_ROLE_TANK))
                botRoles |= lfg::PLAYER_ROLE_TANK;
            if (!(roles & lfg::PLAYER_ROLE_HEALER))
                botRoles |= lfg::PLAYER_ROLE_HEALER;

            // Create dungeon set for the bot
            lfg::LfgDungeonSet dungeonSet;
            dungeonSet.insert(dungeonId);

            try
            {
                // Add bot to LFG queue
                sLFGMgr->JoinLfg(bot, botRoles, dungeonSet);
                botsAdded++;

                TC_LOG_DEBUG("playerbots.instance", "Added bot {} to LFG queue for dungeon {} with roles {}",
                    bot->GetName(), dungeonId, botRoles);
            }
            catch (std::exception const& e)
            {
                TC_LOG_ERROR("playerbots.instance", "Failed to add bot {} to LFG queue: {}",
                    bot->GetName(), e.what());
            }
        }

        TC_LOG_INFO("playerbots.instance", "Added {}/{} bots to LFG queue for dungeon {}",
            botsAdded, bots.size(), dungeonId);

        // Notify any registered callbacks
        std::lock_guard<std::mutex> lock(_callbackMutex);
        auto it = _dungeonCallbacks.find(playerGuid);
        if (it != _dungeonCallbacks.end())
        {
            it->second(true, bots);
            _dungeonCallbacks.erase(it);
        }
    };

    request.onFailed = [playerGuid = player->GetGUID()](std::string const& error)
    {
        TC_LOG_WARN("playerbots.instance", "Failed to get dungeon bots for player {}: {}",
            playerGuid.ToString(), error);

        // Notify any registered callbacks
        std::lock_guard<std::mutex> lock(_callbackMutex);
        auto it = _dungeonCallbacks.find(playerGuid);
        if (it != _dungeonCallbacks.end())
        {
            it->second(false, {});
            _dungeonCallbacks.erase(it);
        }
    };

    // Submit request to orchestrator
    uint32 requestId = sInstanceBotOrchestrator->RequestDungeonBots(request);
    TC_LOG_DEBUG("playerbots.instance", "Dungeon bot request {} submitted for player {}",
        requestId, player->GetName());
}

void InstanceBotHooks::OnPlayerLeaveLfg(Player* player)
{
    if (!IsEnabled() || !player)
        return;

    TC_LOG_DEBUG("playerbots.instance", "OnPlayerLeaveLfg: Player {} left LFG queue",
        player->GetName());

    // Cancel any pending requests for this player
    // The orchestrator tracks requests by player GUID
    sInstanceBotOrchestrator->CancelRequestsForPlayer(player->GetGUID());

    // Clear any registered callbacks
    {
        std::lock_guard<std::mutex> lock(_callbackMutex);
        _dungeonCallbacks.erase(player->GetGUID());
    }
}

void InstanceBotHooks::OnLfgGroupFormed(
    ObjectGuid groupGuid,
    uint32 dungeonId,
    std::vector<ObjectGuid> const& players)
{
    if (!IsEnabled())
        return;

    TC_LOG_DEBUG("playerbots.instance", "OnLfgGroupFormed: Group {} formed for dungeon {} with {} players",
        groupGuid.ToString(), dungeonId, players.size());

    // Get content requirements
    auto const* requirement = sContentRequirementDb->GetDungeonRequirement(dungeonId);
    if (!requirement)
    {
        TC_LOG_WARN("playerbots.instance", "No content requirement found for dungeon {}", dungeonId);
        return;
    }

    // Calculate how many bots we need
    uint32 maxPlayers = requirement->maxPlayers;
    uint32 currentPlayers = static_cast<uint32>(players.size());

    if (currentPlayers >= maxPlayers)
    {
        TC_LOG_DEBUG("playerbots.instance", "Group {} already full ({}/{})",
            groupGuid.ToString(), currentPlayers, maxPlayers);
        return;
    }

    // Find first human player for faction/level reference
    Player* refPlayer = nullptr;
    for (ObjectGuid const& guid : players)
    {
        if (Player* player = ObjectAccessor::FindPlayer(guid))
        {
            if (!sInstanceBotPool->IsPoolBot(guid))
            {
                refPlayer = player;
                break;
            }
        }
    }

    if (!refPlayer)
    {
        TC_LOG_WARN("playerbots.instance", "No human player found in LFG group {}", groupGuid.ToString());
        return;
    }

    // The group should already have bots from the initial queue request
    // This hook is for verification and late additions
    TC_LOG_DEBUG("playerbots.instance", "LFG group {} ready with {}/{} players",
        groupGuid.ToString(), currentPlayers, maxPlayers);
}

void InstanceBotHooks::OnLfgProposalAccepted(
    ObjectGuid groupGuid,
    uint32 dungeonId)
{
    if (!IsEnabled())
        return;

    TC_LOG_DEBUG("playerbots.instance", "OnLfgProposalAccepted: Group {} accepted for dungeon {}",
        groupGuid.ToString(), dungeonId);

    // Mark assigned bots as entering instance
    sInstanceBotOrchestrator->OnInstanceCreated(
        groupGuid.GetCounter(),
        InstanceType::Dungeon,
        dungeonId
    );
}

// ============================================================================
// BATTLEGROUND HOOKS
// ============================================================================

void InstanceBotHooks::OnPlayerJoinBattleground(
    Player* player,
    uint32 bgTypeId,
    uint32 bracketId,
    bool asGroup)
{
    if (!IsEnabled() || !player)
        return;

    TC_LOG_DEBUG("playerbots.instance", "OnPlayerJoinBattleground: Player {} queued for BG {} bracket {} (group: {})",
        player->GetName(), bgTypeId, bracketId, asGroup ? "yes" : "no");

    // Get content requirements
    auto const* requirement = sContentRequirementDb->GetBattlegroundRequirement(bgTypeId);
    if (!requirement)
    {
        TC_LOG_WARN("playerbots.instance", "No content requirement found for battleground {}", bgTypeId);
        return;
    }

    // For ALL BGs that require both factions, start JIT creation immediately
    // This ensures bots are available for BGs of all sizes (10v10, 15v15, 40v40)
    if (requirement->requiresBothFactions)
    {
        TC_LOG_INFO("playerbots.instance", "Battleground {} detected - preparing JIT bots for {}/faction",
            bgTypeId, requirement->playersPerFaction);

        // CRITICAL FIX: Register queue with QueueStatePoller to enable warm pool
        // The QueueStatePoller will poll this queue every 5 seconds and check if
        // warm pool bots are available before falling back to JIT creation.
        // This ensures warm pool bots (with bypassMaxBotsLimit=true) are used first.
        sQueueStatePoller->RegisterActiveBGQueue(
            static_cast<BattlegroundTypeId>(bgTypeId),
            static_cast<BattlegroundBracketId>(bracketId)
        );

        TC_LOG_INFO("playerbots.instance", "Registered BG {} bracket {} with QueueStatePoller for warm pool integration",
            bgTypeId, bracketId);

        // Create battleground request for orchestrator
        BattlegroundRequest request;
        request.bgTypeId = bgTypeId;
        request.bracketId = bracketId;
        request.playerLevel = player->GetLevel();
        request.currentAlliancePlayers = player->GetTeam() == ALLIANCE ? 1 : 0;
        request.currentHordePlayers = player->GetTeam() == HORDE ? 1 : 0;
        request.playerFaction = player->GetTeam() == ALLIANCE ? Faction::Alliance : Faction::Horde;

        // CRITICAL FIX: Capture player GUID for bot tracking (enables invitation auto-accept)
        ObjectGuid playerGuid = player->GetGUID();

        request.onBotsReady = [bgTypeId, bracketId, playerGuid](
            std::vector<ObjectGuid> const& alliance,
            std::vector<ObjectGuid> const& horde)
        {
            TC_LOG_INFO("playerbots.instance", "BG {} bots ready: {} Alliance, {} Horde - Adding to login queue (tracking human {})",
                bgTypeId, alliance.size(), horde.size(), playerGuid.ToString());

            // ================================================================
            // CRITICAL FIX: JIT creates DATABASE RECORDS, not logged-in Player objects
            // We must:
            // 1. Get account ID for each bot
            // 2. Add to pending queue for login
            // 3. ProcessPendingBGQueues will login them and queue for BG
            // 4. Store humanPlayerGuid so bots are tracked for invitation handling
            // ================================================================

            auto now = std::chrono::steady_clock::now();
            uint32 addedCount = 0;

            // Add Alliance bots to pending queue
            for (auto const& botGuid : alliance)
            {
                // Get account ID from JITBotFactory (stored during creation)
                // We use this instead of database query because commits are async
                uint32 accountId = sJITBotFactory->GetAccountForBot(botGuid);

                if (accountId == 0)
                {
                    TC_LOG_WARN("playerbots.instance", "BG {} - Could not find account for bot {} in JITBotFactory",
                        bgTypeId, botGuid.ToString());
                    continue;
                }

                PendingBGQueueEntry entry;
                entry.botGuid = botGuid;
                entry.humanPlayerGuid = playerGuid;  // CRITICAL: Track human for invitation handling
                entry.accountId = accountId;
                entry.bgTypeId = bgTypeId;
                entry.bracketId = bracketId;
                entry.team = TEAM_ALLIANCE;
                entry.createdAt = now;
                entry.loginQueued = false;
                entry.retryCount = 0;

                {
                    std::lock_guard<std::mutex> lock(_pendingBGMutex);
                    _pendingBGQueue.push_back(entry);
                }
                ++addedCount;
            }

            // Add Horde bots to pending queue
            for (auto const& botGuid : horde)
            {
                // Get account ID from JITBotFactory (stored during creation)
                // We use this instead of database query because commits are async
                uint32 accountId = sJITBotFactory->GetAccountForBot(botGuid);

                if (accountId == 0)
                {
                    TC_LOG_WARN("playerbots.instance", "BG {} - Could not find account for bot {} in JITBotFactory",
                        bgTypeId, botGuid.ToString());
                    continue;
                }

                PendingBGQueueEntry entry;
                entry.botGuid = botGuid;
                entry.humanPlayerGuid = playerGuid;  // CRITICAL: Track human for invitation handling
                entry.accountId = accountId;
                entry.bgTypeId = bgTypeId;
                entry.bracketId = bracketId;
                entry.team = TEAM_HORDE;
                entry.createdAt = now;
                entry.loginQueued = false;
                entry.retryCount = 0;

                {
                    std::lock_guard<std::mutex> lock(_pendingBGMutex);
                    _pendingBGQueue.push_back(entry);
                }
                ++addedCount;
            }

            TC_LOG_INFO("playerbots.instance", "BG {} - Added {} bots to pending login queue (total pending: {})",
                bgTypeId, addedCount, _pendingBGQueue.size());

            // Notify any registered callbacks (for legacy compatibility)
            std::lock_guard<std::mutex> lock(_callbackMutex);
            uint64 key = MakeBGCallbackKey(bgTypeId, bracketId);
            auto it = _bgCallbacks.find(key);
            if (it != _bgCallbacks.end())
            {
                it->second(true, alliance, horde);
                _bgCallbacks.erase(it);
            }
        };

        request.onFailed = [bgTypeId, bracketId](std::string const& error)
        {
            TC_LOG_WARN("playerbots.instance", "Failed to get BG {} bots: {}", bgTypeId, error);

            std::lock_guard<std::mutex> lock(_callbackMutex);
            uint64 key = MakeBGCallbackKey(bgTypeId, bracketId);
            auto it = _bgCallbacks.find(key);
            if (it != _bgCallbacks.end())
            {
                it->second(false, {}, {});
                _bgCallbacks.erase(it);
            }
        };

        sInstanceBotOrchestrator->RequestBattlegroundBots(request);
    }
}

void InstanceBotHooks::OnPlayerLeaveBattlegroundQueue(
    Player* player,
    uint32 bgTypeId)
{
    if (!IsEnabled() || !player)
        return;

    TC_LOG_DEBUG("playerbots.instance", "OnPlayerLeaveBattlegroundQueue: Player {} left BG {} queue",
        player->GetName(), bgTypeId);

    // Don't cancel BG bot creation - other players may still be queued
    // The queue system handles this naturally
}

bool InstanceBotHooks::OnBattlegroundQueueUpdate(
    uint32 bgTypeId,
    uint32 bracketId,
    uint32 allianceInQueue,
    uint32 hordeInQueue,
    uint32 minPlayersPerTeam,
    uint32 maxPlayersPerTeam)
{
    if (!IsEnabled())
        return false;

    // Only process if at least one human is queued
    if (allianceInQueue == 0 && hordeInQueue == 0)
        return false;

    auto const* requirement = sContentRequirementDb->GetBattlegroundRequirement(bgTypeId);
    if (!requirement)
        return false;

    // Check if we should fill with bots
    uint32 totalQueued = allianceInQueue + hordeInQueue;

    // For PvP BGs requiring both factions
    if (requirement->requiresBothFactions)
    {
        // Check if we can start the BG with bots
        bool allianceReady = allianceInQueue >= 1;  // At least 1 human
        bool hordeReady = hordeInQueue >= 1 || allianceInQueue >= 1;  // Can fill with bots

        if (allianceReady || hordeReady)
        {
            uint32 neededAlliance = maxPlayersPerTeam - allianceInQueue;
            uint32 neededHorde = maxPlayersPerTeam - hordeInQueue;

            TC_LOG_DEBUG("playerbots.instance",
                "BG {} queue update: Alliance {}/{}, Horde {}/{} - need {} Alliance bots, {} Horde bots",
                bgTypeId, allianceInQueue, maxPlayersPerTeam, hordeInQueue, maxPlayersPerTeam,
                neededAlliance, neededHorde);

            // Check if orchestrator can provide bots
            return sInstanceBotOrchestrator->CanProvideBotsFor(InstanceType::Battleground, bgTypeId);
        }
    }

    return false;
}

void InstanceBotHooks::OnBattlegroundStarting(
    Battleground* bg,
    uint32 allianceCount,
    uint32 hordeCount)
{
    if (!IsEnabled() || !bg)
        return;

    uint32 bgTypeId = bg->GetTypeID();
    uint32 instanceId = bg->GetInstanceID();

    TC_LOG_DEBUG("playerbots.instance",
        "OnBattlegroundStarting: BG {} instance {} starting with {} Alliance, {} Horde",
        bgTypeId, instanceId, allianceCount, hordeCount);

    // Register this instance with the orchestrator
    sInstanceBotOrchestrator->OnInstanceCreated(
        instanceId,
        InstanceType::Battleground,
        bgTypeId
    );

    // Get content requirements
    auto const* requirement = sContentRequirementDb->GetBattlegroundRequirement(bgTypeId);
    if (!requirement)
        return;

    uint32 maxPerTeam = requirement->playersPerFaction;
    uint32 neededAlliance = maxPerTeam > allianceCount ? maxPerTeam - allianceCount : 0;
    uint32 neededHorde = maxPerTeam > hordeCount ? maxPerTeam - hordeCount : 0;

    if (neededAlliance == 0 && neededHorde == 0)
    {
        TC_LOG_DEBUG("playerbots.instance", "BG {} fully staffed, no bots needed", instanceId);
        return;
    }

    TC_LOG_INFO("playerbots.instance", "BG {} needs {} Alliance bots, {} Horde bots",
        instanceId, neededAlliance, neededHorde);

    // Request bots from orchestrator (already should be reserved)
    BattlegroundRequest request;
    request.bgTypeId = bgTypeId;
    request.bracketId = bg->GetBracketId();
    // Player level derived from bracket — bracket min level is used by the orchestrator
    request.currentAlliancePlayers = allianceCount;
    request.currentHordePlayers = hordeCount;
    request.playerFaction = allianceCount > 0 ? Faction::Alliance : Faction::Horde;

    request.onBotsReady = [instanceId, bgTypeId](
        std::vector<ObjectGuid> const& alliance,
        std::vector<ObjectGuid> const& horde)
    {
        TC_LOG_INFO("playerbots.instance",
            "BG {} instance {} - adding {} Alliance bots, {} Horde bots",
            bgTypeId, instanceId, alliance.size(), horde.size());

        // Bots are added to the BG through the normal queue mechanism
        // The orchestrator handles teleporting bots to the BG
    };

    request.onFailed = [instanceId, bgTypeId](std::string const& error)
    {
        TC_LOG_ERROR("playerbots.instance",
            "Failed to fill BG {} instance {}: {}",
            bgTypeId, instanceId, error);
    };

    sInstanceBotOrchestrator->RequestBattlegroundBots(request);
}

void InstanceBotHooks::OnBattlegroundEnded(
    Battleground* bg,
    uint32 winnerTeam)
{
    if (!IsEnabled() || !bg)
        return;

    uint32 instanceId = bg->GetInstanceID();
    uint32 bgTypeId = bg->GetTypeID();

    TC_LOG_DEBUG("playerbots.instance", "OnBattlegroundEnded: BG {} instance {} ended, winner: {}",
        bgTypeId, instanceId, winnerTeam == ALLIANCE ? "Alliance" : "Horde");

    // Release all bots from this BG back to pool
    sInstanceBotOrchestrator->OnInstanceEnded(instanceId);

    // Get bots in this instance for recycling
    std::vector<ObjectGuid> bots = sInstanceBotOrchestrator->GetBotsInInstance(instanceId);
    if (!bots.empty())
    {
        sJITBotFactory->RecycleBots(bots);
    }

    TC_LOG_DEBUG("playerbots.instance", "Released {} bots from BG {} instance {}",
        bots.size(), bgTypeId, instanceId);
}

void InstanceBotHooks::OnPlayerLeftBattleground(
    Player* player,
    Battleground* bg)
{
    if (!IsEnabled() || !player || !bg)
        return;

    // If this was the last human player, end the BG
    // (Let the core handle this - we just track for logging)
    TC_LOG_DEBUG("playerbots.instance", "Player {} left BG {} instance {}",
        player->GetName(), bg->GetTypeID(), bg->GetInstanceID());
}

// ============================================================================
// ARENA HOOKS
// ============================================================================

void InstanceBotHooks::OnPlayerJoinArena(
    Player* player,
    uint32 arenaType,
    uint32 bracketId,
    bool isRated,
    std::vector<ObjectGuid> const& teamMembers)
{
    if (!IsEnabled() || !player)
        return;

    TC_LOG_DEBUG("playerbots.instance", "OnPlayerJoinArena: Player {} queued for {}v{} arena (rated: {})",
        player->GetName(), arenaType, arenaType, isRated ? "yes" : "no");

    // Create arena request
    ArenaRequest request;
    request.arenaType = arenaType;
    request.bracketId = bracketId;
    request.playerLevel = player->GetLevel();
    request.playerGuid = player->GetGUID();
    request.playerFaction = player->GetTeam() == ALLIANCE ? Faction::Alliance : Faction::Horde;
    request.existingTeammates = teamMembers;
    request.needOpponents = true;

    request.onBotsReady = [arenaType, playerName = player->GetName()](
        std::vector<ObjectGuid> const& teammates,
        std::vector<ObjectGuid> const& opponents)
    {
        TC_LOG_DEBUG("playerbots.instance", "Arena {} bots ready for {}: {} teammates, {} opponents",
            arenaType, playerName, teammates.size(), opponents.size());
    };

    request.onFailed = [arenaType, playerName = player->GetName()](std::string const& error)
    {
        TC_LOG_WARN("playerbots.instance", "Failed to get arena {} bots for {}: {}",
            arenaType, playerName, error);
    };

    sInstanceBotOrchestrator->RequestArenaBots(request);
}

void InstanceBotHooks::OnPlayerLeaveArenaQueue(
    Player* player,
    uint32 arenaType)
{
    if (!IsEnabled() || !player)
        return;

    TC_LOG_DEBUG("playerbots.instance", "OnPlayerLeaveArenaQueue: Player {} left arena {} queue",
        player->GetName(), arenaType);

    sInstanceBotOrchestrator->CancelRequestsForPlayer(player->GetGUID());
}

bool InstanceBotHooks::OnArenaMatchPreparing(
    uint32 arenaType,
    uint32 bracketId,
    std::vector<ObjectGuid>& team1Players,
    std::vector<ObjectGuid>& team2Players,
    uint32 team1NeedsPlayers,
    uint32 team2NeedsPlayers)
{
    if (!IsEnabled())
        return false;

    TC_LOG_DEBUG("playerbots.instance",
        "OnArenaMatchPreparing: {}v{} - Team1 needs {}, Team2 needs {}",
        arenaType, arenaType, team1NeedsPlayers, team2NeedsPlayers);

    if (team1NeedsPlayers == 0 && team2NeedsPlayers == 0)
        return true;  // Match is ready

    // Get arena requirements
    auto const* requirement = sContentRequirementDb->GetArenaRequirement(arenaType);
    if (!requirement)
        return false;

    // Determine factions (for mixed arena, use Alliance for team1, Horde for team2)
    Faction team1Faction = Faction::Alliance;
    Faction team2Faction = Faction::Horde;

    // Try to get bots from pool
    if (team1NeedsPlayers > 0)
    {
        ArenaAssignment assignment = sInstanceBotPool->AssignForArena(
            arenaType,
            bracketId,
            team1Faction,
            team1NeedsPlayers,
            0  // No opponents from this call
        );

        for (ObjectGuid const& guid : assignment.teammates)
        {
            team1Players.push_back(guid);
        }
    }

    if (team2NeedsPlayers > 0)
    {
        ArenaAssignment assignment = sInstanceBotPool->AssignForArena(
            arenaType,
            bracketId,
            team2Faction,
            team2NeedsPlayers,
            0
        );

        for (ObjectGuid const& guid : assignment.teammates)
        {
            team2Players.push_back(guid);
        }
    }

    // Check if we now have enough
    bool team1Ready = team1Players.size() >= arenaType;
    bool team2Ready = team2Players.size() >= arenaType;

    TC_LOG_DEBUG("playerbots.instance", "Arena match prep: Team1 {}/{}, Team2 {}/{}",
        team1Players.size(), arenaType, team2Players.size(), arenaType);

    return team1Ready && team2Ready;
}

void InstanceBotHooks::OnArenaMatchEnded(
    uint32 arenaType,
    uint32 winnerTeam,
    std::vector<ObjectGuid> const& team1Players,
    std::vector<ObjectGuid> const& team2Players)
{
    if (!IsEnabled())
        return;

    TC_LOG_DEBUG("playerbots.instance", "OnArenaMatchEnded: {}v{} - Winner: Team {}",
        arenaType, arenaType, winnerTeam + 1);

    // Release bots back to pool
    for (ObjectGuid const& guid : team1Players)
    {
        if (sInstanceBotPool->IsPoolBot(guid))
        {
            sInstanceBotPool->ReleaseBots({guid});
        }
    }

    for (ObjectGuid const& guid : team2Players)
    {
        if (sInstanceBotPool->IsPoolBot(guid))
        {
            sInstanceBotPool->ReleaseBots({guid});
        }
    }
}

// ============================================================================
// RAID/INSTANCE HOOKS
// ============================================================================

void InstanceBotHooks::OnPlayerEnterInstance(
    Player* player,
    uint32 mapId,
    uint32 instanceId,
    bool isRaid)
{
    if (!IsEnabled() || !player)
        return;

    TC_LOG_DEBUG("playerbots.instance", "OnPlayerEnterInstance: Player {} entered {} instance {} (map {})",
        player->GetName(), isRaid ? "raid" : "dungeon", instanceId, mapId);

    // Track the instance
    sInstanceBotOrchestrator->OnInstanceCreated(
        instanceId,
        isRaid ? InstanceType::Raid : InstanceType::Dungeon,
        mapId
    );
}

void InstanceBotHooks::OnPlayerLeaveInstance(
    Player* player,
    uint32 mapId,
    uint32 instanceId)
{
    if (!IsEnabled() || !player)
        return;

    TC_LOG_DEBUG("playerbots.instance", "OnPlayerLeaveInstance: Player {} left instance {} (map {})",
        player->GetName(), instanceId, mapId);

    sInstanceBotOrchestrator->OnPlayerLeftInstance(player->GetGUID(), instanceId);
}

void InstanceBotHooks::OnInstanceReset(
    uint32 mapId,
    uint32 instanceId)
{
    if (!IsEnabled())
        return;

    TC_LOG_DEBUG("playerbots.instance", "OnInstanceReset: Instance {} (map {}) reset",
        instanceId, mapId);

    // Release all bots from this instance
    sInstanceBotOrchestrator->OnInstanceEnded(instanceId);
}

void InstanceBotHooks::OnRaidNeedsBots(
    Player* leader,
    uint32 raidId,
    std::vector<ObjectGuid> const& currentMembers,
    std::map<ObjectGuid, uint8> const& memberRoles)
{
    if (!IsEnabled() || !leader)
        return;

    TC_LOG_DEBUG("playerbots.instance", "OnRaidNeedsBots: Leader {} needs bots for raid {} ({} current members)",
        leader->GetName(), raidId, currentMembers.size());

    // Create raid request
    RaidRequest request;
    request.leaderGuid = leader->GetGUID();
    request.raidId = raidId;
    request.playerLevel = leader->GetLevel();
    request.playerFaction = leader->GetTeam() == ALLIANCE ? Faction::Alliance : Faction::Horde;
    request.currentGroupMembers = currentMembers;
    request.memberRoles = memberRoles;

    request.onBotsReady = [raidId, leaderName = leader->GetName()](std::vector<ObjectGuid> const& bots)
    {
        TC_LOG_INFO("playerbots.instance", "Raid {} bots ready for {}: {} bots assigned",
            raidId, leaderName, bots.size());
    };

    request.onFailed = [raidId, leaderName = leader->GetName()](std::string const& error)
    {
        TC_LOG_WARN("playerbots.instance", "Failed to get raid {} bots for {}: {}",
            raidId, leaderName, error);
    };

    sInstanceBotOrchestrator->RequestRaidBots(request);
}

// ============================================================================
// GROUP HOOKS
// ============================================================================

void InstanceBotHooks::OnGroupDisbanded(ObjectGuid groupGuid)
{
    if (!IsEnabled())
        return;

    TC_LOG_DEBUG("playerbots.instance", "OnGroupDisbanded: Group {} disbanded",
        groupGuid.ToString());

    // Release any bots assigned to this group
    // The instance cleanup will handle the bot release
}

void InstanceBotHooks::OnGroupLeaderChanged(
    Group* group,
    Player* newLeader)
{
    if (!IsEnabled() || !group || !newLeader)
        return;

    TC_LOG_DEBUG("playerbots.instance", "OnGroupLeaderChanged: Group {} new leader {}",
        group->GetGUID().ToString(), newLeader->GetName());

    // Update bot master reference if needed
    // Bots should follow the new leader's commands
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

bool InstanceBotHooks::IsPoolBot(ObjectGuid guid)
{
    if (!IsEnabled())
        return false;

    return sInstanceBotPool->IsPoolBot(guid);
}

uint32 InstanceBotHooks::GetEstimatedWaitTime(
    uint8 contentType,
    uint32 contentId)
{
    if (!IsEnabled())
        return 0;

    InstanceType type = static_cast<InstanceType>(contentType);
    auto waitTime = sInstanceBotOrchestrator->GetEstimatedWaitTime(type, contentId, 1);

    return static_cast<uint32>(waitTime.count());
}

void InstanceBotHooks::GetPoolStats(
    uint32& outReady,
    uint32& outAssigned,
    uint32& outTotal)
{
    if (!IsEnabled())
    {
        outReady = 0;
        outAssigned = 0;
        outTotal = 0;
        return;
    }

    PoolStatistics stats = sInstanceBotPool->GetStatistics();
    outReady = stats.slotStats.readySlots;
    outAssigned = stats.slotStats.assignedSlots;
    outTotal = stats.slotStats.GetTotal();
}

// ============================================================================
// CALLBACKS
// ============================================================================

void InstanceBotHooks::RegisterDungeonCallback(
    ObjectGuid playerGuid,
    BotAssignmentCallback callback)
{
    if (!IsEnabled())
        return;

    std::lock_guard<std::mutex> lock(_callbackMutex);
    _dungeonCallbacks[playerGuid] = std::move(callback);
}

void InstanceBotHooks::RegisterBattlegroundCallback(
    uint32 bgTypeId,
    uint32 bracketId,
    PvPBotAssignmentCallback callback)
{
    if (!IsEnabled())
        return;

    std::lock_guard<std::mutex> lock(_callbackMutex);
    uint64 key = MakeBGCallbackKey(bgTypeId, bracketId);
    _bgCallbacks[key] = std::move(callback);
}

// ============================================================================
// UPDATE / PROCESSING
// ============================================================================

void InstanceBotHooks::Update(uint32 diff)
{
    if (!IsEnabled())
        return;

    _updateAccumulator += diff;
    if (_updateAccumulator < UPDATE_INTERVAL_MS)
        return;

    _updateAccumulator = 0;

    // Process bots waiting to be logged in and queued for BG
    ProcessPendingBGQueues();
}

void InstanceBotHooks::ProcessPendingBGQueues()
{
    std::lock_guard<std::mutex> lock(_pendingBGMutex);

    if (_pendingBGQueue.empty())
        return;

    uint32 processed = 0;
    uint32 loginsQueued = 0;
    uint32 botsQueued = 0;
    uint32 expired = 0;

    // Process pending entries
    auto it = _pendingBGQueue.begin();
    while (it != _pendingBGQueue.end())
    {
        auto& entry = *it;

        // Check for expiration
        if (entry.IsExpired())
        {
            TC_LOG_WARN("playerbots.instance", "BG pending entry expired for bot {} (BG {})",
                entry.botGuid.ToString(), entry.bgTypeId);
            it = _pendingBGQueue.erase(it);
            ++expired;
            continue;
        }

        // Step 1: If login not yet queued, queue login
        if (!entry.loginQueued)
        {
            // CRITICAL: Wait for database commit before attempting login
            // JIT bot creation uses async CommitTransaction, so we must wait
            // for the transaction to be applied before querying character data
            constexpr auto DB_COMMIT_DELAY = std::chrono::milliseconds(500);
            auto timeSinceCreation = std::chrono::steady_clock::now() - entry.createdAt;
            if (timeSinceCreation < DB_COMMIT_DELAY)
            {
                // Not enough time has passed for DB commit, skip for now
                ++it;
                continue;
            }

            // Check if BotWorldSessionMgr is available
            if (!sBotWorldSessionMgr || !sBotWorldSessionMgr->IsEnabled())
            {
                TC_LOG_DEBUG("playerbots.instance", "BotWorldSessionMgr not available for bot {}",
                    entry.botGuid.ToString());
                ++it;
                continue;
            }

            // Queue the bot for login
            bool queued = sBotWorldSessionMgr->AddPlayerBot(entry.botGuid, entry.accountId, true /* bypassLimit */);
            if (queued)
            {
                entry.loginQueued = true;
                entry.loginQueuedAt = std::chrono::steady_clock::now();
                ++loginsQueued;
                TC_LOG_DEBUG("playerbots.instance", "Queued bot {} for login (BG {}, team {})",
                    entry.botGuid.ToString(), entry.bgTypeId, entry.team == TEAM_ALLIANCE ? "Alliance" : "Horde");
            }
            else
            {
                // CRITICAL FIX: Check if bot is already in the loading queue (queued by JITBotFactory)
                // If so, treat it as "already queued" instead of a failure
                if (sBotWorldSessionMgr->IsBotLoading(entry.botGuid))
                {
                    entry.loginQueued = true;
                    entry.loginQueuedAt = std::chrono::steady_clock::now();
                    TC_LOG_DEBUG("playerbots.instance", "Bot {} already in loading queue - treating as queued (BG {}, team {})",
                        entry.botGuid.ToString(), entry.bgTypeId, entry.team == TEAM_ALLIANCE ? "Alliance" : "Horde");
                }
                else
                {
                    ++entry.retryCount;
                    if (entry.retryCount > 10)
                    {
                        TC_LOG_WARN("playerbots.instance", "Failed to queue bot {} for login after 10 retries",
                            entry.botGuid.ToString());
                        it = _pendingBGQueue.erase(it);
                        continue;
                    }
                }
            }
            ++it;
            continue;
        }

        // Step 2: Check if login timed out
        if (entry.IsLoginTimedOut())
        {
            TC_LOG_WARN("playerbots.instance", "Login timeout for bot {} (BG {})",
                entry.botGuid.ToString(), entry.bgTypeId);
            it = _pendingBGQueue.erase(it);
            continue;
        }

        // Step 3: Check if bot is now logged in (in world)
        Player* bot = ObjectAccessor::FindPlayer(entry.botGuid);
        if (!bot || !bot->IsInWorld())
        {
            // Not yet logged in, check next iteration
            ++it;
            continue;
        }

        // Step 3.5: Mark login as completed if this is first time we see the bot in world
        if (!entry.loginCompleted)
        {
            entry.loginCompleted = true;
            entry.loginCompletedAt = std::chrono::steady_clock::now();
            TC_LOG_DEBUG("playerbots.instance", "Bot {} appeared in world, starting stabilization delay",
                entry.botGuid.ToString());
            ++it;
            continue;
        }

        // Step 3.6: Wait for stabilization delay before queueing
        // This ensures bot AI, strategies, and session state are fully initialized
        if (!entry.IsReadyForBGQueue())
        {
            // Still stabilizing, check next iteration
            ++it;
            continue;
        }

        // Step 4: Bot is logged in AND stabilized - queue for BG!
        TC_LOG_INFO("playerbots.instance", "Bot {} is now logged in, queueing for BG {} (team {}, tracking human {})",
            bot->GetName(), entry.bgTypeId, entry.team == TEAM_ALLIANCE ? "Alliance" : "Horde",
            entry.humanPlayerGuid.ToString());

        // Use BGBotManager to queue the bot WITH TRACKING for invitation auto-accept
        BattlegroundTypeId bgTypeId = static_cast<BattlegroundTypeId>(entry.bgTypeId);
        BattlegroundBracketId bracketId = static_cast<BattlegroundBracketId>(entry.bracketId);

        // CRITICAL FIX: Use QueueBotForBGWithTracking to register in _queuedBots
        // This ensures OnInvitationReceived will process the invitation and add bot to BG
        if (sBGBotManager->QueueBotForBGWithTracking(bot, bgTypeId, bracketId, entry.humanPlayerGuid))
        {
            TC_LOG_INFO("playerbots.instance", "Successfully queued bot {} for BG {} bracket {} (with invitation tracking)",
                bot->GetName(), entry.bgTypeId, entry.bracketId);
            ++botsQueued;
        }
        else
        {
            TC_LOG_WARN("playerbots.instance", "Failed to queue bot {} for BG {} bracket {}",
                bot->GetName(), entry.bgTypeId, entry.bracketId);
        }

        // Remove from pending queue (success or fail, we're done with this entry)
        it = _pendingBGQueue.erase(it);
        ++processed;
    }

    if (processed > 0 || loginsQueued > 0 || expired > 0)
    {
        TC_LOG_INFO("playerbots.instance",
            "ProcessPendingBGQueues: processed={}, loginsQueued={}, botsQueued={}, expired={}, remaining={}",
            processed, loginsQueued, botsQueued, expired, _pendingBGQueue.size());
    }
}

} // namespace Playerbot
