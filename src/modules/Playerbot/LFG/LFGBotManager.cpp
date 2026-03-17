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

#include "LFGBotManager.h"
#include "Core/PlayerBotHelpers.h"  // GetBotAI, GetGameSystems
#include "../Core/Diagnostics/BotOperationTracker.h"
#include "../Core/BotReadinessChecker.h"  // Comprehensive bot readiness validation
#include "LFGBotSelector.h"
#include "LFGRoleDetector.h"
#include "LFGGroupCoordinator.h"
#include "../Lifecycle/Instance/JITBotFactory.h"
#include "../Lifecycle/Instance/PoolSlotState.h"
#include "../Lifecycle/Instance/QueueStatePoller.h"
#include "Player.h"
#include "LFGMgr.h"
#include "LFG.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "World.h"
#include "Group.h"
#include "GroupMgr.h"
#include "DB2Stores.h"  // For ContentTuning level queries
#include "../Core/PlayerBotHooks.h"

// Use Playerbot namespace for error tracking enums and macros
using namespace Playerbot;

// Constructor
LFGBotManager::LFGBotManager()
    : _enabled(false)
    , _updateAccumulator(0)
    , _initialized(false)
{
}

// Destructor
LFGBotManager::~LFGBotManager()
{
    Shutdown();
}

// Singleton instance
LFGBotManager* LFGBotManager::instance()
{
    static LFGBotManager instance;
    return &instance;
}

void LFGBotManager::Initialize()
{
    std::lock_guard lock(_mutex);

    if (_initialized)
    {
        TC_LOG_WARN("module.playerbot", "LFGBotManager::Initialize - Already initialized, ignoring");
        return;
    }

    TC_LOG_INFO("module.playerbot", "Initializing LFG Bot Manager...");

    // Clear any existing data
    _queuedBots.clear();
    _humanPlayers.clear();
    _proposalBots.clear();
    _updateAccumulator = 0;

    // LFG bot filling enabled by default. Can be controlled via Playerbot.LFG.Enable config.
    _enabled = true;

    _initialized = true;

    TC_LOG_INFO("module.playerbot", "LFG Bot Manager initialized successfully (Enabled: {})", _enabled.load());
}

void LFGBotManager::Shutdown()
{
    std::lock_guard lock(_mutex);

    if (!_initialized)
        return;

    TC_LOG_INFO("module.playerbot", "Shutting down LFG Bot Manager...");

    // Remove all bots from queues
    for (auto const& [botGuid, queueInfo] : _queuedBots)
    {
        if (Player* bot = ObjectAccessor::FindPlayer(botGuid))
        {
            RemoveBotFromQueue(bot);
        }
    }

    // Clear all data structures
    _queuedBots.clear();
    _humanPlayers.clear();
    _proposalBots.clear();

    _initialized = false;
    _enabled = false;

    TC_LOG_INFO("module.playerbot", "LFG Bot Manager shut down successfully");
}

void LFGBotManager::Update(uint32 diff)
{
    if (!_enabled || !_initialized)
        return;

    _updateAccumulator += diff;
    _pendingCheckAccumulator += diff;

    // Process pending JIT bots frequently (every 100ms)
    if (_pendingCheckAccumulator >= PENDING_CHECK_INTERVAL)
    {
        ProcessPendingJITBots();
        _pendingCheckAccumulator = 0;
    }

    // Periodic cleanup every CLEANUP_INTERVAL
    if (_updateAccumulator >= CLEANUP_INTERVAL)
    {
        CleanupStaleAssignments();
        _updateAccumulator = 0;
    }
}

void LFGBotManager::ProcessPendingJITBots()
{
    if (_pendingJITBots.empty())
        return;

    std::lock_guard lock(_mutex);

    auto it = _pendingJITBots.begin();
    while (it != _pendingJITBots.end())
    {
        PendingJITBot& pending = *it;
        pending.retryCount++;

        // ========================================================================
        // COMPREHENSIVE BOT READINESS CHECK
        // ========================================================================
        // Instead of just checking ObjectAccessor::FindPlayer(), we now use
        // BotReadinessChecker to verify ALL conditions are met:
        // - Bot is in ObjectAccessor (HashMapHolder)
        // - Bot is in CharacterCache (prevents "??" level display)
        // - Bot is in world with valid map and session
        // - Bot is not being teleported
        // - BotAI is initialized
        // - Bot is not already in a queue
        //
        // This prevents race conditions where bots are queued before they're
        // fully initialized, causing issues like "offline" display in group UI.
        // ========================================================================

        BotReadinessResult readiness = BotReadinessChecker::Check(pending.botGuid, BotReadinessFlag::LFG_READY);

        if (readiness.IsLFGReady())
        {
            Player* bot = readiness.player;

            // Bot is fully ready! Queue it for LFG
            uint8 botRole = sLFGRoleDetector->DetectBotRole(bot);

            if (QueueBot(bot, botRole, pending.dungeons))
            {
                RegisterBotAssignment(pending.humanPlayerGuid, pending.botGuid, botRole, pending.dungeons);
                TC_LOG_INFO("playerbot.lfg", "ProcessPendingJITBots: ✅ Queued bot {} (level {}) as role {} for player {} after {} retries",
                    bot->GetName(), bot->GetLevel(), botRole, pending.humanPlayerGuid.ToString(), pending.retryCount);
            }
            else
            {
                TC_LOG_WARN("playerbot.lfg", "ProcessPendingJITBots: Failed to queue bot {} for player {}",
                    bot->GetName(), pending.humanPlayerGuid.ToString());

                // Track queue failure (specific error already logged in QueueBot)
                BOT_TRACK_LFG_ERROR(
                    LFGQueueErrorCode::JOIN_LFG_FAILED,
                    fmt::format("JIT bot {} failed to queue after loading for player {}",
                        bot->GetName(), pending.humanPlayerGuid.ToString()),
                    pending.botGuid,
                    pending.humanPlayerGuid,
                    pending.dungeons.empty() ? 0 : *pending.dungeons.begin());
            }

            it = _pendingJITBots.erase(it);
        }
        else if (pending.retryCount >= MAX_PENDING_RETRIES)
        {
            // Too many retries, give up
            TC_LOG_WARN("playerbot.lfg", "ProcessPendingJITBots: Gave up on bot {} after {} retries ({}ms) - not ready. {}",
                pending.botGuid.ToString(), pending.retryCount, pending.retryCount * PENDING_CHECK_INTERVAL,
                readiness.GetFailureReport());

            // Track this failure for diagnostics
            BOT_TRACK_LFG_ERROR(
                LFGQueueErrorCode::JIT_BOT_TIMEOUT,
                fmt::format("JIT bot {} not ready after {}ms ({} retries) for player {}: {}",
                    pending.botGuid.ToString(),
                    pending.retryCount * PENDING_CHECK_INTERVAL,
                    pending.retryCount,
                    pending.humanPlayerGuid.ToString(),
                    readiness.GetSummary()),
                pending.botGuid,
                pending.humanPlayerGuid,
                pending.dungeons.empty() ? 0 : *pending.dungeons.begin());

            it = _pendingJITBots.erase(it);
        }
        else
        {
            // Not ready yet, log debug info periodically
            if (pending.retryCount % 5 == 0)  // Every 5 retries (~1.5 seconds)
            {
                TC_LOG_DEBUG("playerbot.lfg", "ProcessPendingJITBots: Bot {} retry {}/{} - {}",
                    pending.botGuid.ToString(), pending.retryCount, MAX_PENDING_RETRIES,
                    readiness.GetSummary());
            }
            ++it;
        }
    }
}

void LFGBotManager::OnPlayerJoinQueue(Player* player, uint8 playerRole, lfg::LfgDungeonSet const& dungeons)
{
    if (!_enabled || !_initialized)
        return;

    // Only process human players
    if (Playerbot::PlayerBotHooks::IsPlayerBot(player))
        return;

    if (dungeons.empty())
    {
        TC_LOG_WARN("module.playerbot", "LFGBotManager::OnPlayerJoinQueue - Player {} queued with no dungeons",
                    player->GetName());
        return;
    }

    ObjectGuid playerGuid = player->GetGUID();

    // Debug: Log raw role value and which flags are set
    TC_LOG_INFO("module.playerbot.lfg", "LFGBotManager::OnPlayerJoinQueue - Player {} role bitmask: {} (TANK={}, HEALER={}, DPS={})",
                player->GetName(), playerRole,
                (playerRole & lfg::PLAYER_ROLE_TANK) ? "YES" : "no",
                (playerRole & lfg::PLAYER_ROLE_HEALER) ? "YES" : "no",
                (playerRole & lfg::PLAYER_ROLE_DAMAGE) ? "YES" : "no");

    // Calculate needed roles (assuming 5-man dungeon composition)
    uint8 tanksNeeded = 0, healersNeeded = 0, dpsNeeded = 0;
    CalculateNeededRoles(playerRole, tanksNeeded, healersNeeded, dpsNeeded);

    uint8 totalNeeded = tanksNeeded + healersNeeded + dpsNeeded;

    if (totalNeeded == 0)
    {
        TC_LOG_DEBUG("module.playerbot", "LFGBotManager::OnPlayerJoinQueue - Player {} has full group, no bots needed",
                     player->GetName());
        return;
    }

    TC_LOG_INFO("module.playerbot", "LFGBotManager::OnPlayerJoinQueue - Player {} queued, need {} tanks, {} healers, {} DPS",
                player->GetName(), tanksNeeded, healersNeeded, dpsNeeded);

    // Build role mask for bot selection
    uint8 neededRoles = 0;
    if (tanksNeeded > 0)
        neededRoles |= lfg::PLAYER_ROLE_TANK;
    if (healersNeeded > 0)
        neededRoles |= lfg::PLAYER_ROLE_HEALER;
    if (dpsNeeded > 0)
        neededRoles |= lfg::PLAYER_ROLE_DAMAGE;

    // Populate queue with bots
    uint32 botsQueued = PopulateQueue(playerGuid, neededRoles, dungeons);

    if (botsQueued < totalNeeded)
    {
        TC_LOG_WARN("module.playerbot", "LFGBotManager::OnPlayerJoinQueue - Only queued {}/{} bots for player {}",
                    botsQueued, totalNeeded, player->GetName());
    }
    else
    {
        TC_LOG_INFO("module.playerbot", "LFGBotManager::OnPlayerJoinQueue - Successfully queued {} bots for player {}",
                    botsQueued, player->GetName());
    }

    // ========================================================================
    // Register dungeons with QueueStatePoller for shortage detection
    // CRITICAL: Pass the human player's level so JIT bots are created at
    // the correct level to match the human, not at dungeon average level.
    // ========================================================================
    uint8 playerLevel = player->GetLevel();
    for (uint32 dungeonId : dungeons)
    {
        // Get dungeon level range for validation
        uint8 minLevel = 0, maxLevel = 0;
        if (GetDungeonLevelRange(dungeonId, minLevel, maxLevel))
        {
            sQueueStatePoller->RegisterActiveLFGQueue(dungeonId, minLevel, maxLevel, playerLevel);
            TC_LOG_INFO("module.playerbot.lfg", "Registered LFG queue for dungeon {} with human player level {}",
                dungeonId, playerLevel);
        }
        else
        {
            // Fallback if level range not found (shouldn't happen)
            sQueueStatePoller->RegisterActiveLFGQueue(dungeonId);
            TC_LOG_WARN("module.playerbot.lfg", "Could not get level range for dungeon {}, using fallback registration",
                dungeonId);
        }
    }

    // Trigger immediate poll to detect any remaining shortages
    // This allows the JIT system to create additional bots if needed
    sQueueStatePoller->PollLFGQueues();
}

void LFGBotManager::OnPlayerLeaveQueue(ObjectGuid playerGuid)
{
    if (!_enabled || !_initialized)
        return;

    std::lock_guard lock(_mutex);

    // Check if this is a human player with assigned bots
    auto humanItr = _humanPlayers.find(playerGuid);
    if (humanItr != _humanPlayers.end())
    {
        TC_LOG_DEBUG("module.playerbot", "LFGBotManager::OnPlayerLeaveQueue - Human player left queue, removing {} assigned bots",
                     humanItr->second.assignedBots.size());

        // Remove all assigned bots from queue
    for (ObjectGuid botGuid : humanItr->second.assignedBots)
        {
            if (Player* bot = ObjectAccessor::FindPlayer(botGuid))
            {
                RemoveBotFromQueue(bot);
            }
            _queuedBots.erase(botGuid);
        }

        _humanPlayers.erase(humanItr);
    }
    // Check if this is a bot
    else
    {
        auto botItr = _queuedBots.find(playerGuid);
        if (botItr != _queuedBots.end())
        {
            // Remove bot from its human player's assignment list
            ObjectGuid humanGuid = botItr->second.humanPlayerGuid;
            auto humanPlayerItr = _humanPlayers.find(humanGuid);
            if (humanPlayerItr != _humanPlayers.end())
            {
                auto& bots = humanPlayerItr->second.assignedBots;
                bots.erase(std::remove(bots.begin(), bots.end(), playerGuid), bots.end());

                if (bots.empty())
                {
                    _humanPlayers.erase(humanPlayerItr);
                }
            }

            _queuedBots.erase(botItr);
        }
    }
}

void LFGBotManager::OnProposalReceived(uint32 proposalId, lfg::LfgProposal const& proposal)
{
    if (!_enabled || !_initialized)
        return;

    std::lock_guard lock(_mutex);

    // Find all bots in this proposal
    std::unordered_set<ObjectGuid> botsInProposal;

    for (auto const& [playerGuid, proposalPlayer] : proposal.players)
    {
        // Check if this is a queued bot
    if (_queuedBots.find(playerGuid) != _queuedBots.end())
        {
            botsInProposal.insert(playerGuid);

            // Update bot's proposal ID
            _queuedBots[playerGuid].proposalId = proposalId;

            // Automatically accept the proposal for this bot
            TC_LOG_DEBUG("module.playerbot", "LFGBotManager::OnProposalReceived - Bot {} auto-accepting proposal {}",
                         playerGuid.ToString(), proposalId);

            // Use LFGMgr to update proposal acceptance
            sLFGMgr->UpdateProposal(proposalId, playerGuid, true);
        }
    }

    if (!botsInProposal.empty())
    {
        _proposalBots[proposalId] = botsInProposal;
        TC_LOG_DEBUG("module.playerbot", "LFGBotManager::OnProposalReceived - Proposal {} has {} bots",
                     proposalId, botsInProposal.size());
    }
}

void LFGBotManager::OnRoleCheckReceived(ObjectGuid groupGuid, ObjectGuid botGuid)
{
    if (!_enabled || !_initialized)
        return;

    std::lock_guard lock(_mutex);

    // If specific bot GUID provided, handle just that bot
    if (!botGuid.IsEmpty())
    {
        auto itr = _queuedBots.find(botGuid);
        if (itr != _queuedBots.end())
        {
            uint8 role = itr->second.assignedRole;
            TC_LOG_DEBUG("module.playerbot", "LFGBotManager::OnRoleCheckReceived - Bot {} confirming role {} for group {}",
                         botGuid.ToString(), role, groupGuid.ToString());

            sLFGMgr->UpdateRoleCheck(groupGuid, botGuid, role);
        }
        return;
    }

    // Handle all bots in the group
    for (auto const& [queuedBotGuid, queueInfo] : _queuedBots)
    {
        // Check if bot is part of this group's proposal
        // We need to verify the bot is actually in this group
    if (Player* bot = ObjectAccessor::FindPlayer(queuedBotGuid))
        {
            uint8 role = queueInfo.assignedRole;
            TC_LOG_DEBUG("module.playerbot", "LFGBotManager::OnRoleCheckReceived - Bot {} confirming role {} for group {}",
                         queuedBotGuid.ToString(), role, groupGuid.ToString());

            sLFGMgr->UpdateRoleCheck(groupGuid, queuedBotGuid, role);
        }
    }
}

void LFGBotManager::OnGroupFormed(ObjectGuid groupGuid)
{
    if (!_enabled || !_initialized)
    {
        TC_LOG_WARN("module.playerbot", "LFGBotManager::OnGroupFormed - SKIPPED (enabled={}, initialized={})",
            _enabled.load(), _initialized);
        return;
    }

    std::lock_guard lock(_mutex);

    TC_LOG_INFO("module.playerbot", ">>> LFGBotManager::OnGroupFormed CALLED - Group {}", groupGuid.ToString());

    // Get the group
    Group* group = sGroupMgr->GetGroupByGUID(groupGuid);
    if (!group)
    {
        TC_LOG_ERROR("module.playerbot", "LFGBotManager::OnGroupFormed - Group {} not found", groupGuid.ToString());
        return;
    }

    // Get the dungeon ID for this group
    uint32 dungeonId = sLFGMgr->GetDungeon(groupGuid);
    TC_LOG_INFO("module.playerbot", "LFGBotManager::OnGroupFormed - Group {} has dungeonId {} from LFGMgr",
        groupGuid.ToString(), dungeonId);

    if (dungeonId == 0)
    {
        TC_LOG_WARN("module.playerbot", "LFGBotManager::OnGroupFormed - No dungeon ID for group {} - ABORTING", groupGuid.ToString());
        return;
    }

    // Notify the group coordinator about group formation
    bool coordinatorEnabled = sLFGGroupCoordinator->IsEnabled();
    TC_LOG_INFO("module.playerbot", "LFGBotManager::OnGroupFormed - Coordinator enabled: {}", coordinatorEnabled);

    if (coordinatorEnabled)
    {
        if (sLFGGroupCoordinator->OnGroupFormed(groupGuid, dungeonId))
        {
            TC_LOG_DEBUG("module.playerbot", "LFGBotManager::OnGroupFormed - Group coordinator notified for group {}",
                groupGuid.ToString());

            // Teleport the group to the dungeon
    if (sLFGGroupCoordinator->TeleportGroupToDungeon(group, dungeonId))
            {
                TC_LOG_INFO("module.playerbot", "LFGBotManager::OnGroupFormed - Group {} teleported to dungeon {}",
                    groupGuid.ToString(), dungeonId);
            }
            else
            {
                TC_LOG_ERROR("module.playerbot", "LFGBotManager::OnGroupFormed - Failed to teleport group {} to dungeon {}",
                    groupGuid.ToString(), dungeonId);
            }
        }
        else
        {
            TC_LOG_ERROR("module.playerbot", "LFGBotManager::OnGroupFormed - Failed to register group {} with coordinator",
                groupGuid.ToString());
        }
    }

    // Find and cleanup all bots and humans associated with this group
    // This is done by checking which bots/humans were successfully grouped
    // The actual cleanup will happen when they complete/leave the dungeon
}

void LFGBotManager::OnProposalFailed(uint32 proposalId)
{
    if (!_enabled || !_initialized)
        return;

    std::lock_guard lock(_mutex);

    auto itr = _proposalBots.find(proposalId);
    if (itr == _proposalBots.end())
        return;

    TC_LOG_DEBUG("module.playerbot", "LFGBotManager::OnProposalFailed - Proposal {} failed, removing {} bots from queue",
                 proposalId, itr->second.size());

    // Remove all bots from this failed proposal
    for (ObjectGuid botGuid : itr->second)
    {
        if (Player* bot = ObjectAccessor::FindPlayer(botGuid))
        {
            RemoveBotFromQueue(bot);
        }

        UnregisterBotAssignment(botGuid);
    }

    _proposalBots.erase(itr);
}

uint32 LFGBotManager::PopulateQueue(ObjectGuid playerGuid, uint8 neededRoles, lfg::LfgDungeonSet const& dungeons)
{
    if (dungeons.empty())
        return 0;

    std::lock_guard lock(_mutex);

    uint32 dungeonId = *dungeons.begin();
    uint8 minLevel = 0, maxLevel = 0;

    if (!GetDungeonLevelRange(dungeonId, minLevel, maxLevel))
    {
        TC_LOG_ERROR("module.playerbot", "LFGBotManager::PopulateQueue - Could not get level range for dungeon {}", dungeonId);
        BOT_TRACK_LFG_ERROR(
            LFGQueueErrorCode::DUNGEON_NOT_FOUND,
            fmt::format("Could not get level range for dungeon {}", dungeonId),
            ObjectGuid::Empty,
            playerGuid,
            dungeonId);
        return 0;
    }

    TC_LOG_INFO("module.playerbot", "LFGBotManager::PopulateQueue - Dungeon {} allows level {}-{}", dungeonId, minLevel, maxLevel);

    // Determine what roles the human player has
    Player* humanPlayer = ObjectAccessor::FindPlayer(playerGuid);
    if (!humanPlayer)
    {
        TC_LOG_ERROR("module.playerbot", "LFGBotManager::PopulateQueue - Could not find player {}", playerGuid.ToString());
        BOT_TRACK_LFG_ERROR(
            LFGQueueErrorCode::HUMAN_PLAYER_NOT_FOUND,
            fmt::format("Could not find player {} for LFG queue", playerGuid.ToString()),
            ObjectGuid::Empty,
            playerGuid,
            dungeonId);
        return 0;
    }

    // ========================================================================
    // CRITICAL: Calculate bot level bracket based on HUMAN PLAYER's level
    // ========================================================================
    // Bots must be within the player's level bracket, NOT the dungeon's full range.
    // LFG typically groups players within ±5 levels of each other.
    // We clamp to the dungeon's allowed range to ensure bots can enter.
    uint8 playerLevel = humanPlayer->GetLevel();
    constexpr uint8 LEVEL_BRACKET_RANGE = 5;  // ±5 levels like standard LFG

    // Calculate bracket around player's level
    uint8 bracketMinLevel = (playerLevel > LEVEL_BRACKET_RANGE) ? (playerLevel - LEVEL_BRACKET_RANGE) : 1;
    uint8 bracketMaxLevel = (playerLevel + LEVEL_BRACKET_RANGE < MAX_LEVEL) ? (playerLevel + LEVEL_BRACKET_RANGE) : MAX_LEVEL;

    // Clamp bracket to dungeon's allowed range (bots must be able to enter)
    uint8 effectiveMinLevel = std::max(bracketMinLevel, minLevel);
    uint8 effectiveMaxLevel = std::min(bracketMaxLevel, maxLevel);

    // Sanity check: ensure we have a valid range
    if (effectiveMinLevel > effectiveMaxLevel)
    {
        TC_LOG_WARN("module.playerbot", "LFGBotManager::PopulateQueue - Player level {} outside dungeon range {}-{}, expanding bracket",
                    playerLevel, minLevel, maxLevel);
        // Player is outside dungeon range - use dungeon's range as fallback
        effectiveMinLevel = minLevel;
        effectiveMaxLevel = maxLevel;
    }

    TC_LOG_INFO("module.playerbot", "LFGBotManager::PopulateQueue - Player level {}, selecting bots in bracket {}-{} (dungeon allows {}-{})",
                playerLevel, effectiveMinLevel, effectiveMaxLevel, minLevel, maxLevel);

    // Calculate exact numbers needed (assuming 5-man dungeon composition)
    uint8 tanksNeeded = 0, healersNeeded = 0, dpsNeeded = 0;
    uint8 humanRole = sLFGMgr->GetRoles(playerGuid);
    CalculateNeededRoles(humanRole, tanksNeeded, healersNeeded, dpsNeeded);

    // Update QueueStatePoller with the needed role counts
    // This allows the poller to track fill status accurately
    sQueueStatePoller->SetLFGNeededCounts(dungeonId, tanksNeeded, healersNeeded, dpsNeeded);

    // Also count the human player as filling their primary role
    // Human is queued with their selected role, so increment that count
    if (humanRole & lfg::PLAYER_ROLE_TANK)
        sQueueStatePoller->UpdateLFGRoleCount(dungeonId, lfg::PLAYER_ROLE_TANK, true);
    else if (humanRole & lfg::PLAYER_ROLE_HEALER)
        sQueueStatePoller->UpdateLFGRoleCount(dungeonId, lfg::PLAYER_ROLE_HEALER, true);
    else if (humanRole & lfg::PLAYER_ROLE_DAMAGE)
        sQueueStatePoller->UpdateLFGRoleCount(dungeonId, lfg::PLAYER_ROLE_DAMAGE, true);

    uint32 botsQueued = 0;
    uint32 tanksQueued = 0;
    uint32 healersQueued = 0;
    uint32 dpsQueued = 0;

    // ========================================================================
    // PHASE 1: Try to find existing online bots within player's level bracket
    // ========================================================================

    // Queue tanks
    if ((neededRoles & lfg::PLAYER_ROLE_TANK) && tanksNeeded > 0)
    {
        // Use static method for system-wide bot discovery (Phase 7 compliant)
        // IMPORTANT: Use effectiveMinLevel/effectiveMaxLevel (player's bracket), not dungeon's full range
        std::vector<Player*> tanks = LFGBotSelector::FindAvailableTanks(effectiveMinLevel, effectiveMaxLevel, tanksNeeded, humanPlayer);
        for (Player* tank : tanks)
        {
            if (QueueBot(tank, lfg::PLAYER_ROLE_TANK, dungeons))
            {
                RegisterBotAssignment(playerGuid, tank->GetGUID(), lfg::PLAYER_ROLE_TANK, dungeons);
                ++botsQueued;
                ++tanksQueued;
                TC_LOG_INFO("playerbot.lfg", "Queued tank bot {} (level {}) for human player {}",
                    tank->GetName(), tank->GetLevel(), humanPlayer->GetName());
            }
        }
    }

    // Queue healers
    if ((neededRoles & lfg::PLAYER_ROLE_HEALER) && healersNeeded > 0)
    {
        // Use static method for system-wide bot discovery (Phase 7 compliant)
        // IMPORTANT: Use effectiveMinLevel/effectiveMaxLevel (player's bracket), not dungeon's full range
        std::vector<Player*> healers = LFGBotSelector::FindAvailableHealers(effectiveMinLevel, effectiveMaxLevel, healersNeeded, humanPlayer);
        for (Player* healer : healers)
        {
            if (QueueBot(healer, lfg::PLAYER_ROLE_HEALER, dungeons))
            {
                RegisterBotAssignment(playerGuid, healer->GetGUID(), lfg::PLAYER_ROLE_HEALER, dungeons);
                ++botsQueued;
                ++healersQueued;
                TC_LOG_INFO("playerbot.lfg", "Queued healer bot {} (level {}) for human player {}",
                    healer->GetName(), healer->GetLevel(), humanPlayer->GetName());
            }
        }
    }

    // Queue DPS
    if ((neededRoles & lfg::PLAYER_ROLE_DAMAGE) && dpsNeeded > 0)
    {
        // Use static method for system-wide bot discovery (Phase 7 compliant)
        // IMPORTANT: Use effectiveMinLevel/effectiveMaxLevel (player's bracket), not dungeon's full range
        std::vector<Player*> dps = LFGBotSelector::FindAvailableDPS(effectiveMinLevel, effectiveMaxLevel, dpsNeeded, humanPlayer);
        for (Player* dpsPlayer : dps)
        {
            if (QueueBot(dpsPlayer, lfg::PLAYER_ROLE_DAMAGE, dungeons))
            {
                RegisterBotAssignment(playerGuid, dpsPlayer->GetGUID(), lfg::PLAYER_ROLE_DAMAGE, dungeons);
                ++botsQueued;
                ++dpsQueued;
                TC_LOG_INFO("playerbot.lfg", "Queued DPS bot {} (level {}) for human player {}",
                    dpsPlayer->GetName(), dpsPlayer->GetLevel(), humanPlayer->GetName());
            }
        }
    }

    // ========================================================================
    // PHASE 2: JIT Bot Creation - Create new bots if not enough found
    // ========================================================================

    uint32 tanksStillNeeded = (tanksNeeded > tanksQueued) ? (tanksNeeded - tanksQueued) : 0;
    uint32 healersStillNeeded = (healersNeeded > healersQueued) ? (healersNeeded - healersQueued) : 0;
    uint32 dpsStillNeeded = (dpsNeeded > dpsQueued) ? (dpsNeeded - dpsQueued) : 0;

    if (tanksStillNeeded > 0 || healersStillNeeded > 0 || dpsStillNeeded > 0)
    {
        TC_LOG_INFO("playerbot.lfg", "LFGBotManager::PopulateQueue - Not enough online bots found. "
            "Still need: {} tanks, {} healers, {} DPS. Submitting JIT creation request.",
            tanksStillNeeded, healersStillNeeded, dpsStillNeeded);

        // Create JIT request with the human player's level (centered in the dungeon's level range)
        uint32 targetLevel = humanPlayer->GetLevel();

        // Capture necessary data for the callbacks
        ObjectGuid capturedPlayerGuid = playerGuid;
        lfg::LfgDungeonSet capturedDungeons = dungeons;

        Playerbot::FactoryRequest jitRequest;
        jitRequest.instanceType = Playerbot::InstanceType::Dungeon;
        jitRequest.contentId = dungeonId;
        jitRequest.playerLevel = targetLevel;
        jitRequest.playerFaction = (humanPlayer->GetTeam() == ALLIANCE) ? Playerbot::Faction::Alliance : Playerbot::Faction::Horde;
        jitRequest.tanksNeeded = tanksStillNeeded;
        jitRequest.healersNeeded = healersStillNeeded;
        jitRequest.dpsNeeded = dpsStillNeeded;
        jitRequest.priority = 1;  // High priority for LFG
        jitRequest.timeout = std::chrono::milliseconds(60000);  // 60 second timeout
        jitRequest.createdAt = std::chrono::system_clock::now();

        // Callback when bots are created - queue them for the human player
        jitRequest.onComplete = [this, capturedPlayerGuid, capturedDungeons](std::vector<ObjectGuid> const& createdBots)
        {
            TC_LOG_INFO("playerbot.lfg", "LFGBotManager JIT callback - {} bots created for player {}",
                createdBots.size(), capturedPlayerGuid.ToString());

            // Add bots to pending list - they need time to login before we can queue them
            // The Update() function will process pending bots once they're fully loaded
            {
                std::lock_guard lock(_mutex);
                for (ObjectGuid botGuid : createdBots)
                {
                    PendingJITBot pending;
                    pending.botGuid = botGuid;
                    pending.humanPlayerGuid = capturedPlayerGuid;
                    pending.dungeons = capturedDungeons;
                    pending.createdAt = std::chrono::steady_clock::now();
                    pending.retryCount = 0;
                    _pendingJITBots.push_back(std::move(pending));
                    
                    TC_LOG_INFO("playerbot.lfg", "JIT: Added bot {} to pending queue for player {}",
                        botGuid.ToString(), capturedPlayerGuid.ToString());
                }
            }
        };

        // Callback on failure
        jitRequest.onFailed = [capturedPlayerGuid](std::string const& error)
        {
            TC_LOG_ERROR("playerbot.lfg", "LFGBotManager JIT failed for player {}: {}",
                capturedPlayerGuid.ToString(), error);
            BOT_TRACK_LFG_ERROR(
                LFGQueueErrorCode::JIT_BOT_TIMEOUT,
                fmt::format("JIT bot creation failed for player {}: {}", capturedPlayerGuid.ToString(), error),
                ObjectGuid::Empty,
                capturedPlayerGuid,
                0);
        };

        // Callback on progress (optional, just for logging)
        jitRequest.onProgress = [capturedPlayerGuid](float progress)
        {
            TC_LOG_DEBUG("playerbot.lfg", "LFGBotManager JIT progress for player {}: {:.1f}%",
                capturedPlayerGuid.ToString(), progress * 100.0f);
        };

        // Submit the JIT request
        uint32 requestId = sJITBotFactory->SubmitRequest(std::move(jitRequest));
        TC_LOG_INFO("playerbot.lfg", "LFGBotManager::PopulateQueue - Submitted JIT request {} for player {}",
            requestId, humanPlayer->GetName());
    }

    // Register human player if we queued any bots (or if JIT request was submitted)
    if (botsQueued > 0 || tanksStillNeeded > 0 || healersStillNeeded > 0 || dpsStillNeeded > 0)
    {
        auto& humanInfo = _humanPlayers[playerGuid];
        humanInfo.playerRole = humanRole;
        humanInfo.dungeons = dungeons;
    }

    return botsQueued;
}

bool LFGBotManager::IsBotQueued(ObjectGuid botGuid) const
{
    std::lock_guard lock(_mutex);
    return _queuedBots.find(botGuid) != _queuedBots.end();
}

void LFGBotManager::GetStatistics(uint32& totalQueued, uint32& totalAssignments) const
{
    std::lock_guard lock(_mutex);
    totalQueued = static_cast<uint32>(_queuedBots.size());
    totalAssignments = static_cast<uint32>(_humanPlayers.size());
}

void LFGBotManager::SetEnabled(bool enable)
{
    _enabled = enable;
    TC_LOG_INFO("module.playerbot", "LFG Bot Manager {}", enable ? "enabled" : "disabled");

    if (!enable)
    {
        // Remove all bots from queues when disabled
        std::lock_guard lock(_mutex);
        for (auto const& [botGuid, queueInfo] : _queuedBots)
        {
            if (Player* bot = ObjectAccessor::FindPlayer(botGuid))
            {
                RemoveBotFromQueue(bot);
            }
        }
        _queuedBots.clear();
        _humanPlayers.clear();
        _proposalBots.clear();
    }
}

void LFGBotManager::CleanupStaleAssignments()
{
    std::lock_guard lock(_mutex);

    time_t currentTime = time(nullptr);
    std::vector<ObjectGuid> staleHumans;
    std::vector<ObjectGuid> staleBots;

    // Find stale bot assignments
    for (auto const& [botGuid, queueInfo] : _queuedBots)
    {
        // Check if bot has been queued too long
    if ((currentTime - queueInfo.queueTime) > MAX_QUEUE_TIME)
        {
            TC_LOG_DEBUG("module.playerbot", "LFGBotManager::CleanupStaleAssignments - Bot {} queue time exceeded, removing",
                         botGuid.ToString());
            staleBots.push_back(botGuid);
            continue;
        }

        // Check if bot still exists
        Player* bot = ObjectAccessor::FindPlayer(botGuid);
        if (!bot)
        {
            TC_LOG_DEBUG("module.playerbot", "LFGBotManager::CleanupStaleAssignments - Bot {} no longer exists, removing",
                         botGuid.ToString());
            staleBots.push_back(botGuid);
            continue;
        }

        // Check if bot is still actually in queue via LFGMgr
        lfg::LfgState botState = sLFGMgr->GetState(botGuid);
        if (botState != lfg::LFG_STATE_QUEUED && botState != lfg::LFG_STATE_PROPOSAL && botState != lfg::LFG_STATE_ROLECHECK)
        {
            TC_LOG_DEBUG("module.playerbot", "LFGBotManager::CleanupStaleAssignments - Bot {} not in valid LFG state ({}), removing",
                         botGuid.ToString(), botState);
            staleBots.push_back(botGuid);
        }
    }

    // Find stale human assignments
    for (auto const& [humanGuid, playerInfo] : _humanPlayers)
    {
        // Check if human still exists
        Player* human = ObjectAccessor::FindPlayer(humanGuid);
        if (!human)
        {
            TC_LOG_DEBUG("module.playerbot", "LFGBotManager::CleanupStaleAssignments - Human {} no longer exists, removing",
                         humanGuid.ToString());
            staleHumans.push_back(humanGuid);
            continue;
        }

        // Check if human is still in queue
        lfg::LfgState humanState = sLFGMgr->GetState(humanGuid);
        if (humanState != lfg::LFG_STATE_QUEUED && humanState != lfg::LFG_STATE_PROPOSAL && humanState != lfg::LFG_STATE_ROLECHECK)
        {
            TC_LOG_DEBUG("module.playerbot", "LFGBotManager::CleanupStaleAssignments - Human {} not in valid LFG state ({}), removing",
                         humanGuid.ToString(), humanState);
            staleHumans.push_back(humanGuid);
        }
    }

    // Remove stale bots
    for (ObjectGuid botGuid : staleBots)
    {
        if (Player* bot = ObjectAccessor::FindPlayer(botGuid))
        {
            RemoveBotFromQueue(bot);
        }
        UnregisterBotAssignment(botGuid);
    }

    // Remove stale humans
    for (ObjectGuid humanGuid : staleHumans)
    {
        UnregisterAllBotsForPlayer(humanGuid);
    }

    if (!staleBots.empty() || !staleHumans.empty())
    {
        TC_LOG_DEBUG("module.playerbot", "LFGBotManager::CleanupStaleAssignments - Removed {} stale bots and {} stale humans",
                     staleBots.size(), staleHumans.size());
    }
}

bool LFGBotManager::QueueJITBot(Player* bot, uint32 dungeonId)
{
    if (!bot)
    {
        TC_LOG_WARN("module.playerbot.lfg", "LFGBotManager::QueueJITBot - Null bot pointer");
        return false;
    }

    // ========================================================================
    // ENHANCED JIT BOT QUEUE LOGGING
    // ========================================================================
    // Log comprehensive information to help diagnose LFG queue failures.
    // The most common issue is level mismatch between bot and dungeon requirements.
    // ========================================================================

    uint8 botLevel = bot->GetLevel();
    uint8 minLevel = 0, maxLevel = 0;
    bool hasLevelRange = GetDungeonLevelRange(dungeonId, minLevel, maxLevel);

    TC_LOG_INFO("module.playerbot.lfg",
        "LFGBotManager::QueueJITBot - Attempting to queue bot {} (level {}) for dungeon {} (requires level {}-{})",
        bot->GetName(), botLevel, dungeonId,
        hasLevelRange ? minLevel : 0,
        hasLevelRange ? maxLevel : 0);

    // Pre-queue validation: Check if bot's level is within dungeon range
    if (hasLevelRange)
    {
        if (botLevel < minLevel)
        {
            TC_LOG_WARN("module.playerbot.lfg",
                "⚠️ QueueJITBot: Bot {} level {} is BELOW dungeon {} minimum level {}!",
                bot->GetName(), botLevel, dungeonId, minLevel);
            // Continue anyway - the LFGMgr will do the final check
        }
        else if (botLevel > maxLevel)
        {
            TC_LOG_WARN("module.playerbot.lfg",
                "⚠️ QueueJITBot: Bot {} level {} is ABOVE dungeon {} maximum level {}!",
                bot->GetName(), botLevel, dungeonId, maxLevel);
            // Continue anyway - the LFGMgr will do the final check
        }
        else
        {
            TC_LOG_INFO("module.playerbot.lfg",
                "✅ QueueJITBot: Bot {} level {} is within dungeon {} range ({}-{})",
                bot->GetName(), botLevel, dungeonId, minLevel, maxLevel);
        }
    }

    // Pre-check: Get locked dungeons to see if this dungeon is accessible
    lfg::LfgLockMap locks = sLFGMgr->GetLockedDungeons(bot->GetGUID());
    auto lockIt = locks.find(dungeonId);
    if (lockIt != locks.end())
    {
        // LfgLockInfoData members: lockStatus, requiredItemLevel, currentItemLevel
        TC_LOG_WARN("module.playerbot.lfg",
            "⚠️ QueueJITBot: Bot {} has LOCK on dungeon {} - Status: {}, RequiredILvl: {}, CurrentILvl: {:.1f}",
            bot->GetName(), dungeonId,
            lockIt->second.lockStatus,
            lockIt->second.requiredItemLevel,
            lockIt->second.currentItemLevel);
    }
    else
    {
        TC_LOG_INFO("module.playerbot.lfg",
            "✅ QueueJITBot: Bot {} has NO lock on dungeon {} - eligible to queue",
            bot->GetName(), dungeonId);
    }

    // Determine role based on bot's spec
    uint8 role = sLFGRoleDetector->DetectBotRole(bot);

    // Create dungeon set
    lfg::LfgDungeonSet dungeons;
    dungeons.insert(dungeonId);

    TC_LOG_INFO("module.playerbot.lfg",
        "LFGBotManager::QueueJITBot - Queueing JIT bot {} (level {}, role={}) for dungeon {}",
        bot->GetName(), botLevel, role, dungeonId);

    bool success = QueueBot(bot, role, dungeons);

    if (success)
    {
        TC_LOG_INFO("module.playerbot.lfg",
            "✅ QueueJITBot: Successfully queued bot {} (level {}) for dungeon {}",
            bot->GetName(), botLevel, dungeonId);
    }
    else
    {
        TC_LOG_ERROR("module.playerbot.lfg",
            "❌ QueueJITBot: FAILED to queue bot {} (level {}) for dungeon {}",
            bot->GetName(), botLevel, dungeonId);
    }

    return success;
}

void LFGBotManager::CalculateNeededRoles(uint8 humanRoles,
                                          uint8& tanksNeeded, uint8& healersNeeded, uint8& dpsNeeded) const
{
    // Standard 5-man dungeon composition — raid composition uses separate logic
    tanksNeeded = lfg::LFG_TANKS_NEEDED;      // 1
    healersNeeded = lfg::LFG_HEALERS_NEEDED;  // 1
    dpsNeeded = lfg::LFG_DPS_NEEDED;          // 3

    // CRITICAL FIX: Human can only fill ONE role, not multiple!
    // When human selects multiple roles (e.g., tank+DPS), they will be assigned
    // exactly ONE role when the group is formed. We must only subtract ONE role.
    // Priority: Tank > Healer > DPS (rarest roles filled first)
    bool humanRoleFilled = false;

    if (!humanRoleFilled && (humanRoles & lfg::PLAYER_ROLE_TANK))
    {
        tanksNeeded = (tanksNeeded > 0) ? tanksNeeded - 1 : 0;
        humanRoleFilled = true;
        TC_LOG_DEBUG("module.playerbot.lfg", "CalculateNeededRoles: Human assigned as TANK");
    }
    if (!humanRoleFilled && (humanRoles & lfg::PLAYER_ROLE_HEALER))
    {
        healersNeeded = (healersNeeded > 0) ? healersNeeded - 1 : 0;
        humanRoleFilled = true;
        TC_LOG_DEBUG("module.playerbot.lfg", "CalculateNeededRoles: Human assigned as HEALER");
    }
    if (!humanRoleFilled && (humanRoles & lfg::PLAYER_ROLE_DAMAGE))
    {
        dpsNeeded = (dpsNeeded > 0) ? dpsNeeded - 1 : 0;
        humanRoleFilled = true;
        TC_LOG_DEBUG("module.playerbot.lfg", "CalculateNeededRoles: Human assigned as DPS");
    }

    TC_LOG_INFO("module.playerbot.lfg", "CalculateNeededRoles: Need {} tanks, {} healers, {} DPS (human roles mask: {})",
                tanksNeeded, healersNeeded, dpsNeeded, humanRoles);
}

bool LFGBotManager::QueueBot(Player* bot, uint8 role, lfg::LfgDungeonSet const& dungeons)
{
    if (!bot)
        return false;

    // Minimum level 10 required for LFG (same as retail)
    // This prevents low-level bots (like Death Knights in starting zone) from joining
    constexpr uint8 MIN_LFG_LEVEL = 10;
    if (bot->GetLevel() < MIN_LFG_LEVEL)
    {
        TC_LOG_DEBUG("module.playerbot", "LFGBotManager::QueueBot - Bot {} is level {} (minimum {} required for LFG)",
                     bot->GetName(), bot->GetLevel(), MIN_LFG_LEVEL);
        BOT_TRACK_LFG_ERROR(
            LFGQueueErrorCode::ROLE_VALIDATION_FAILED,  // Using role validation as closest error type
            fmt::format("Bot {} level {} below minimum {} for LFG", bot->GetName(), bot->GetLevel(), MIN_LFG_LEVEL),
            bot->GetGUID(),
            ObjectGuid::Empty,
            dungeons.empty() ? 0 : *dungeons.begin());
        return false;
    }

    if (bot->GetGroup())
    {
        TC_LOG_WARN("module.playerbot", "LFGBotManager::QueueBot - Bot {} is already in a group", bot->GetName());
        BOT_TRACK_LFG_ERROR(
            LFGQueueErrorCode::BOT_IN_GROUP,
            fmt::format("Bot {} is already in a group", bot->GetName()),
            bot->GetGUID(),
            ObjectGuid::Empty,
            dungeons.empty() ? 0 : *dungeons.begin());
        return false;
    }

    // Check if bot has deserter debuff
    if (bot->HasAura(lfg::LFG_SPELL_DUNGEON_DESERTER))
    {
        TC_LOG_DEBUG("module.playerbot", "LFGBotManager::QueueBot - Bot {} has deserter debuff", bot->GetName());
        BOT_TRACK_LFG_ERROR(
            LFGQueueErrorCode::BOT_HAS_DESERTER,
            fmt::format("Bot {} has deserter debuff", bot->GetName()),
            bot->GetGUID(),
            ObjectGuid::Empty,
            dungeons.empty() ? 0 : *dungeons.begin());
        return false;
    }

    // Check if bot is already queued via LFGMgr
    lfg::LfgState botState = sLFGMgr->GetState(bot->GetGUID());
    if (botState == lfg::LFG_STATE_QUEUED || botState == lfg::LFG_STATE_PROPOSAL || botState == lfg::LFG_STATE_ROLECHECK)
    {
        TC_LOG_DEBUG("module.playerbot", "LFGBotManager::QueueBot - Bot {} is already queued (state: {})", bot->GetName(), botState);
        // Track as info (not error) - this is expected in some race conditions
        BOT_TRACK_SUCCESS(BotOperationCategory::LFG_QUEUE, "BotAlreadyQueued", bot->GetGUID());
        return false;
    }

    // Validate role for bot's class using LFGRoleDetector
    if (!sLFGRoleDetector->CanPerformRole(bot, role))
    {
        TC_LOG_WARN("module.playerbot", "LFGBotManager::QueueBot - Bot {} cannot perform role {}", bot->GetName(), role);
        BOT_TRACK_LFG_ERROR(
            LFGQueueErrorCode::ROLE_VALIDATION_FAILED,
            fmt::format("Bot {} (class {}) cannot perform role {}", bot->GetName(), bot->GetClass(), role),
            bot->GetGUID(),
            ObjectGuid::Empty,
            dungeons.empty() ? 0 : *dungeons.begin());
        return false;
    }
    uint8 validatedRole = role;

    // Create a mutable copy of dungeons for JoinLfg
    lfg::LfgDungeonSet dungeonsCopy = dungeons;

    // CRITICAL FIX: Set bot's team/faction in LFGMgr before queueing
    // The LFG system uses separate queues per faction (GetQueueId returns GetTeam)
    // Without this, bots have Team=0 and go into a different queue than human players
    // This mirrors what LFGPlayerScript::OnLogin does for normal players
    sLFGMgr->SetTeam(bot->GetGUID(), bot->GetTeam());

    // Queue the bot via LFGMgr
    TC_LOG_DEBUG("module.playerbot", "LFGBotManager::QueueBot - Queueing bot {} as role {} for {} dungeons (Team: {})",
                 bot->GetName(), validatedRole, dungeonsCopy.size(), bot->GetTeam());

    sLFGMgr->JoinLfg(bot, validatedRole, dungeonsCopy);

    // Update QueueStatePoller role counts so it knows bots are queued
    // This prevents unnecessary JIT requests for roles that are already filled
    for (uint32 dungeonId : dungeonsCopy)
    {
        sQueueStatePoller->UpdateLFGRoleCount(dungeonId, validatedRole, true);
    }

    // Track successful queue operation
    BOT_TRACK_SUCCESS(BotOperationCategory::LFG_QUEUE, "QueueBot", bot->GetGUID());

    return true;
}

void LFGBotManager::RemoveBotFromQueue(Player* bot)
{
    if (!bot)
        return;

    TC_LOG_DEBUG("module.playerbot", "LFGBotManager::RemoveBotFromQueue - Removing bot {} from LFG queue", bot->GetName());

    // Get the bot's queue info before removing so we can update role counts
    // Note: _mutex is already held by caller or this is called from context where we have the info
    ObjectGuid botGuid = bot->GetGUID();
    auto itr = _queuedBots.find(botGuid);
    if (itr != _queuedBots.end())
    {
        BotQueueInfo const& info = itr->second;
        // Decrement role counts for all dungeons this bot was queued for
        for (uint32 dungeonId : info.dungeons)
        {
            sQueueStatePoller->UpdateLFGRoleCount(dungeonId, info.assignedRole, false);
        }
    }

    sLFGMgr->LeaveLfg(botGuid);
}

bool LFGBotManager::GetDungeonLevelRange(uint32 dungeonId, uint8& minLevel, uint8& maxLevel) const
{
    // Access dungeon data directly from DB2 store (LFGMgr::GetLFGDungeon is private)
    LFGDungeonsEntry const* dungeon = sLFGDungeonsStore.LookupEntry(dungeonId);
    if (!dungeon)
    {
        TC_LOG_ERROR("module.playerbot", "LFGBotManager::GetDungeonLevelRange - Dungeon {} not found in LFGDungeons.db2", dungeonId);
        return false;
    }

    // Query ContentTuning for actual level requirements
    // This is the same method TrinityCore uses in LFGMgr::InitializeLockedDungeons
    if (Optional<ContentTuningLevels> levels = sDB2Manager.GetContentTuningData(dungeon->ContentTuningID, {}))
    {
        minLevel = static_cast<uint8>(std::max<int16>(1, levels->MinLevel));
        maxLevel = static_cast<uint8>(std::min<int16>(MAX_LEVEL, levels->MaxLevel));

        TC_LOG_DEBUG("module.playerbot", "LFGBotManager::GetDungeonLevelRange - Dungeon {} '{}' requires level {}-{} (ContentTuning {})",
                     dungeonId, dungeon->Name[sWorld->GetDefaultDbcLocale()], minLevel, maxLevel, dungeon->ContentTuningID);
        return true;
    }

    // ContentTuning not found - this is a data error, fail explicitly
    TC_LOG_ERROR("module.playerbot", "LFGBotManager::GetDungeonLevelRange - ContentTuning {} not found for dungeon {} '{}'",
                 dungeon->ContentTuningID, dungeonId, dungeon->Name[sWorld->GetDefaultDbcLocale()]);
    return false;
}

void LFGBotManager::RegisterBotAssignment(ObjectGuid humanGuid, ObjectGuid botGuid, uint8 role,
                                           lfg::LfgDungeonSet const& dungeons)
{
    // Add to human player's bot list
    auto& humanInfo = _humanPlayers[humanGuid];
    humanInfo.assignedBots.push_back(botGuid);

    // Add to bot queue info
    uint32 primaryDungeon = dungeons.empty() ? 0 : *dungeons.begin();
    _queuedBots[botGuid] = BotQueueInfo(humanGuid, role, dungeons, primaryDungeon);

    TC_LOG_DEBUG("module.playerbot", "LFGBotManager::RegisterBotAssignment - Bot {} assigned to human {} with role {}",
                 botGuid.ToString(), humanGuid.ToString(), role);
}

void LFGBotManager::UnregisterBotAssignment(ObjectGuid botGuid)
{
    auto itr = _queuedBots.find(botGuid);
    if (itr == _queuedBots.end())
        return;

    ObjectGuid humanGuid = itr->second.humanPlayerGuid;

    // Remove from bot queue
    _queuedBots.erase(itr);

    // Remove from human player's bot list
    auto humanItr = _humanPlayers.find(humanGuid);
    if (humanItr != _humanPlayers.end())
    {
        auto& bots = humanItr->second.assignedBots;
        bots.erase(std::remove(bots.begin(), bots.end(), botGuid), bots.end());

        if (bots.empty())
        {
            _humanPlayers.erase(humanItr);
        }
    }

    TC_LOG_DEBUG("module.playerbot", "LFGBotManager::UnregisterBotAssignment - Bot {} unregistered", botGuid.ToString());
}

std::vector<ObjectGuid> LFGBotManager::GetAssignedBots(ObjectGuid humanGuid) const
{
    auto itr = _humanPlayers.find(humanGuid);
    if (itr != _humanPlayers.end())
        return itr->second.assignedBots;

    return std::vector<ObjectGuid>();
}

void LFGBotManager::UnregisterAllBotsForPlayer(ObjectGuid humanGuid)
{
    auto itr = _humanPlayers.find(humanGuid);
    if (itr == _humanPlayers.end())
        return;

    TC_LOG_DEBUG("module.playerbot", "LFGBotManager::UnregisterAllBotsForPlayer - Removing {} bots for player {}",
                 itr->second.assignedBots.size(), humanGuid.ToString());

    for (ObjectGuid botGuid : itr->second.assignedBots)
    {
        if (Player* bot = ObjectAccessor::FindPlayer(botGuid))
        {
            RemoveBotFromQueue(bot);
        }
        _queuedBots.erase(botGuid);
    }

    _humanPlayers.erase(itr);
}
