/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * LFG Bot Integration Script - Module-Only Approach
 *
 * This script integrates the LFGBotManager with TrinityCore's LFG system
 * using a polling approach that requires NO core file modifications.
 *
 * Design:
 * - Uses WorldScript::OnUpdate to periodically check LFG queue state
 * - Detects human players with LFG_STATE_QUEUED status
 * - Triggers bot recruitment via LFGBotManager::OnPlayerJoinQueue
 * - Monitors proposals and role checks for automatic bot responses
 * - Tracks processed players to avoid duplicate bot additions
 */

#include "ScriptMgr.h"
#include "Player.h"
#include "World.h"
#include "WorldSession.h"
#include "GameTime.h"
#include "LFGMgr.h"
#include "LFG.h"
#include "Group.h"
#include "Session/BotWorldSessionMgr.h"
#include "Core/PlayerBotHooks.h"
#include "LFG/LFGBotManager.h"
#include "Lifecycle/Instance/InstanceBotHooks.h"
#include "Log.h"
#include <unordered_set>
#include <unordered_map>

namespace Playerbot
{

/**
 * @brief LFG Bot Integration using polling approach
 *
 * This WorldScript polls the LFG system periodically to detect human players
 * who have joined the queue, then triggers bot recruitment to fill the group.
 */
class PlayerbotLFGScript : public WorldScript
{
public:
    PlayerbotLFGScript() : WorldScript("PlayerbotLFGScript") {}

    void OnUpdate(uint32 diff) override
    {
        // Throttle LFG polling to every 1 second
        _updateAccumulator += diff;
        if (_updateAccumulator < LFG_POLL_INTERVAL)
            return;
        _updateAccumulator = 0;

        // Skip if LFG bot manager is not enabled
        if (!sLFGBotManager || !sLFGBotManager->IsEnabled())
            return;

        // Update LFGBotManager (handles stale cleanup, etc.)
        sLFGBotManager->Update(diff);

        // Poll for new queued players
        PollQueuedPlayers();

        // Poll for proposals that need bot acceptance
        PollProposals();

        // CRITICAL FIX: Poll for newly formed groups to transfer leadership to human
        // When LFG forms a group, the first queued player becomes leader.
        // If a bot queued first, it becomes leader which breaks dungeon progression.
        // This detects group formation and triggers leadership transfer.
        PollGroupFormation();

        // CRITICAL FIX: Poll for bots that need to teleport to dungeon
        // When LFG group is formed, bots enter LFG_STATE_DUNGEON but may not
        // complete teleportation if they missed the initial TeleportPlayer() call
        PollDungeonTeleports();

        // Cleanup stale tracking data periodically
        CleanupStaleData();
    }

    void OnStartup() override
    {
        TC_LOG_INFO("module.playerbot.lfg", "PlayerbotLFGScript: Initializing LFG bot integration...");

        // Initialize LFGBotManager
        if (sLFGBotManager)
        {
            sLFGBotManager->Initialize();
            TC_LOG_INFO("module.playerbot.lfg", "PlayerbotLFGScript: LFGBotManager initialized");
        }
        else
        {
            TC_LOG_ERROR("module.playerbot.lfg", "PlayerbotLFGScript: LFGBotManager not available!");
        }
    }

    void OnShutdownInitiate(ShutdownExitCode /*code*/, ShutdownMask /*mask*/) override
    {
        TC_LOG_INFO("module.playerbot.lfg", "PlayerbotLFGScript: Shutting down LFG bot integration...");

        if (sLFGBotManager)
        {
            sLFGBotManager->Shutdown();
        }

        // Clear all tracking data
        _processedPlayers.clear();
        _processedProposals.clear();
        _lastQueueState.clear();
        _botProposalAcceptTimes.clear();
        _botDungeonTeleportTimes.clear();
        _processedGroups.clear();
        _processedGroupTimes.clear();
    }

private:
    /**
     * @brief Poll all online players to detect new LFG queue joins
     */
    void PollQueuedPlayers()
    {
        SessionMap const& sessions = sWorld->GetAllSessions();

        for (auto const& pair : sessions)
        {
            WorldSession* session = pair.second;
            if (!session)
                continue;

            Player* player = session->GetPlayer();
            if (!player || !player->IsInWorld())
                continue;

            // Skip bots - only process human players
            if (IsPlayerBotCheck(player))
                continue;

            ObjectGuid playerGuid = player->GetGUID();

            // Get player's current LFG state
            lfg::LfgState state = sLFGMgr->GetState(playerGuid);

            // Track state changes
            auto lastStateIt = _lastQueueState.find(playerGuid);
            lfg::LfgState lastState = (lastStateIt != _lastQueueState.end())
                ? lastStateIt->second
                : lfg::LFG_STATE_NONE;

            // Update stored state
            _lastQueueState[playerGuid] = state;

            // Handle state transitions
            if (state == lfg::LFG_STATE_QUEUED && lastState != lfg::LFG_STATE_QUEUED)
            {
                // Player just joined the queue
                HandlePlayerJoinedQueue(player);
            }
            else if (state == lfg::LFG_STATE_NONE && lastState == lfg::LFG_STATE_QUEUED)
            {
                // Player left the queue
                HandlePlayerLeftQueue(player);
            }
            else if (state == lfg::LFG_STATE_PROPOSAL && lastState != lfg::LFG_STATE_PROPOSAL)
            {
                // Proposal received - human player got a proposal
                // This means bots in the same proposal also need to accept
                TC_LOG_INFO("module.playerbot.lfg",
                    "PlayerbotLFGScript: Human player {} has a proposal pending - bots will auto-accept via PollProposals",
                    player->GetName());
            }
            else if (state == lfg::LFG_STATE_ROLECHECK && lastState != lfg::LFG_STATE_ROLECHECK)
            {
                // Role check started
                HandleRoleCheck(player);
            }
        }
    }

    /**
     * @brief Poll for proposals that bots need to accept
     *
     * CRITICAL FIX: This method now actively polls for bots in LFG_STATE_PROPOSAL
     * and auto-accepts their proposals. The packet intercept in BotSession is a
     * backup mechanism, but this polling approach is more reliable because:
     * 1. It doesn't depend on packet serialization/parsing
     * 2. It handles edge cases where packets might be dropped
     * 3. It provides better diagnostic visibility
     */
    void PollProposals()
    {
        // Get all online bots via BotWorldSessionMgr
        std::vector<Player*> bots = Playerbot::sBotWorldSessionMgr->GetAllBotPlayers();

        for (Player* bot : bots)
        {
            if (!bot || !bot->IsInWorld())
                continue;

            ObjectGuid botGuid = bot->GetGUID();

            // Check if bot is in proposal state
            lfg::LfgState state = sLFGMgr->GetState(botGuid);
            if (state != lfg::LFG_STATE_PROPOSAL)
                continue;

            // Check if we've already processed this proposal for this bot
            auto lastAcceptIt = _botProposalAcceptTimes.find(botGuid);
            uint32 now = GameTime::GetGameTimeMS();

            // Debounce: Only try to accept once every 2 seconds per bot
            if (lastAcceptIt != _botProposalAcceptTimes.end() &&
                (now - lastAcceptIt->second) < 2000)
            {
                continue;
            }

            // Bot is in proposal state - we need to find and accept the proposal
            // The LFGMgr doesn't expose proposal IDs directly, so we iterate through proposals
            // However, we can use a simpler approach: check the proposal store via internal access

            TC_LOG_INFO("module.playerbot.lfg",
                "PlayerbotLFGScript::PollProposals - Bot {} in LFG_STATE_PROPOSAL, attempting auto-accept",
                bot->GetName());

            // Mark this bot as having been processed
            _botProposalAcceptTimes[botGuid] = now;

            // Call LFGBotManager to handle the proposal acceptance
            // This will iterate through proposals and accept any that contain this bot
            AcceptBotProposal(bot);
        }
    }

    /**
     * @brief Accept any pending proposal for a bot
     *
     * Optimization: Instead of scanning all proposal IDs, we scan a window
     * around the highest proposal ID we've seen. This is efficient because
     * proposal IDs are sequential and we only need to check recent proposals.
     */
    void AcceptBotProposal(Player* bot)
    {
        if (!bot)
            return;

        ObjectGuid botGuid = bot->GetGUID();
        lfg::LfgState stateBefore = sLFGMgr->GetState(botGuid);

        // Calculate scanning window: scan from (highest - 100) to (highest + 100)
        // This handles both catching up on missed proposals and new ones
        uint32 scanStart = (_highestProposalIdSeen > 100) ? _highestProposalIdSeen - 100 : 1;
        uint32 scanEnd = _highestProposalIdSeen + 100;

        // If this is our first scan, start from 1 but limit to 500 attempts
        if (_highestProposalIdSeen == 0)
        {
            scanStart = 1;
            scanEnd = 500;
        }

        TC_LOG_DEBUG("module.playerbot.lfg",
            "PlayerbotLFGScript::AcceptBotProposal - Scanning proposals {} to {} for bot {}",
            scanStart, scanEnd, bot->GetName());

        for (uint32 proposalId = scanStart; proposalId <= scanEnd; ++proposalId)
        {
            // Skip already processed proposals
            if (_processedProposals.count(proposalId))
                continue;

            // Try to accept this proposal for the bot
            // UpdateProposal will silently fail if bot isn't in this proposal
            sLFGMgr->UpdateProposal(proposalId, botGuid, true);

            lfg::LfgState stateAfter = sLFGMgr->GetState(botGuid);

            // Track highest proposal ID we've attempted (optimization for next scan)
            if (proposalId > _highestProposalIdSeen)
                _highestProposalIdSeen = proposalId;

            // If state changed, we found and accepted the right proposal
            if (stateAfter != stateBefore || stateAfter != lfg::LFG_STATE_PROPOSAL)
            {
                TC_LOG_INFO("module.playerbot.lfg",
                    "PlayerbotLFGScript::AcceptBotProposal - Bot {} accepted proposal {} (state: {} -> {})",
                    bot->GetName(), proposalId,
                    static_cast<uint32>(stateBefore), static_cast<uint32>(stateAfter));

                _processedProposals.insert(proposalId);

                // Cleanup old processed proposals to prevent memory growth
                if (_processedProposals.size() > 1000)
                {
                    // Remove proposals older than current - 500
                    std::vector<uint32> toRemove;
                    for (uint32 oldId : _processedProposals)
                    {
                        if (oldId < proposalId - 500)
                            toRemove.push_back(oldId);
                    }
                    for (uint32 oldId : toRemove)
                        _processedProposals.erase(oldId);
                }
                return;
            }
        }

        // Only warn if we've scanned a reasonable range
        if (scanEnd > scanStart + 10)
        {
            TC_LOG_DEBUG("module.playerbot.lfg",
                "PlayerbotLFGScript::AcceptBotProposal - No proposal found for bot {} in range {}-{}",
                bot->GetName(), scanStart, scanEnd);
        }
    }

    /**
     * @brief CRITICAL FIX: Poll for bots that need to teleport to dungeon
     *
     * When LFG forms a group, bots transition to LFG_STATE_DUNGEON and should
     * be teleported. However, due to timing issues (async bot login, race conditions),
     * bots may miss the initial TeleportPlayer() call in MakeNewGroup().
     *
     * This polling mechanism ensures bots eventually teleport by:
     * 1. Detecting bots in LFG_STATE_DUNGEON state
     * 2. Checking if they're in an LFG group with a dungeon assignment
     * 3. Verifying they're not already in the correct dungeon
     * 4. Forcing teleport via sLFGMgr->TeleportPlayer()
     */
    void PollDungeonTeleports()
    {
        // Get all online bots via BotWorldSessionMgr
        std::vector<Player*> bots = Playerbot::sBotWorldSessionMgr->GetAllBotPlayers();

        for (Player* bot : bots)
        {
            if (!bot || !bot->IsInWorld())
                continue;

            ObjectGuid botGuid = bot->GetGUID();

            // Check if bot is in dungeon state
            lfg::LfgState state = sLFGMgr->GetState(botGuid);
            if (state != lfg::LFG_STATE_DUNGEON)
                continue;

            // Get the bot's group
            Group* group = bot->GetGroup();
            if (!group || !group->isLFGGroup())
                continue;

            // Get the dungeon map ID for this LFG group using public API
            uint32 dungeonMapId = sLFGMgr->GetDungeonMapId(group->GetGUID());
            if (dungeonMapId == 0)
                continue;

            // Check if bot is already in the correct dungeon
            if (bot->GetMapId() == dungeonMapId)
            {
                // Already in dungeon, nothing to do
                continue;
            }

            // Debounce: Check if we've tried to teleport this bot recently
            uint32 now = GameTime::GetGameTimeMS();
            auto lastTeleportIt = _botDungeonTeleportTimes.find(botGuid);
            if (lastTeleportIt != _botDungeonTeleportTimes.end() &&
                (now - lastTeleportIt->second) < 3000)  // 3 second cooldown between attempts
            {
                continue;
            }

            // Check if bot is currently being teleported (waiting for far teleport completion)
            if (bot->IsBeingTeleportedFar() || bot->IsBeingTeleportedNear())
            {
                TC_LOG_DEBUG("module.playerbot.lfg",
                    "PollDungeonTeleports: Bot {} is already being teleported, waiting...",
                    bot->GetName());
                continue;
            }

            // Additional state checks before teleporting
            if (!bot->IsAlive())
            {
                TC_LOG_DEBUG("module.playerbot.lfg",
                    "PollDungeonTeleports: Bot {} is dead, cannot teleport",
                    bot->GetName());
                continue;
            }

            // Record teleport attempt time
            _botDungeonTeleportTimes[botGuid] = now;

            TC_LOG_INFO("module.playerbot.lfg",
                "PollDungeonTeleports: Forcing dungeon teleport for bot {} (map {} -> {})",
                bot->GetName(), bot->GetMapId(), dungeonMapId);

            // Force teleport using LFGMgr (same method used in MakeNewGroup)
            sLFGMgr->TeleportPlayer(bot, false /* out */, false /* fromOpcode */);
        }

        // Cleanup old teleport attempt records (older than 1 minute)
        if (!_botDungeonTeleportTimes.empty())
        {
            uint32 now = GameTime::GetGameTimeMS();
            constexpr uint32 TELEPORT_RECORD_TIMEOUT = 60 * IN_MILLISECONDS;

            std::vector<ObjectGuid> toRemove;
            for (auto const& pair : _botDungeonTeleportTimes)
            {
                if (now - pair.second > TELEPORT_RECORD_TIMEOUT)
                    toRemove.push_back(pair.first);
            }

            for (ObjectGuid const& guid : toRemove)
                _botDungeonTeleportTimes.erase(guid);
        }
    }

    /**
     * @brief CRITICAL FIX: Detect when LFG groups are formed and trigger leadership transfer
     *
     * ROOT CAUSE: LFGBotManager::OnGroupFormed() is NEVER CALLED.
     * The leadership transfer code EXISTS in LFGGroupCoordinator::OnGroupFormed() but the
     * call chain is broken - no one invokes LFGBotManager::OnGroupFormed() when LFG finishes.
     *
     * This method detects when human players transition to LFG_STATE_DUNGEON, which indicates
     * the LFG system has formed a group and begun the dungeon instance. At this point:
     * 1. The group exists and has members assigned
     * 2. The first player who joined (often a bot) is the default leader
     * 3. We need to transfer leadership to the human player
     *
     * SOLUTION: Detect LFG_STATE_DUNGEON transition and call sLFGBotManager->OnGroupFormed()
     * which triggers the leadership transfer via LFGGroupCoordinator::OnGroupFormed().
     */
    void PollGroupFormation()
    {
        SessionMap const& sessions = sWorld->GetAllSessions();
        uint32 now = GameTime::GetGameTimeMS();

        for (auto const& pair : sessions)
        {
            WorldSession* session = pair.second;
            if (!session)
                continue;

            Player* player = session->GetPlayer();
            if (!player)
                continue;

            // FIX: Allow players who are being teleported (not in world but connected)
            // During LFG dungeon teleport, IsInWorld() returns false but we still need to
            // detect group formation to transfer leadership to the human player.
            // The player is valid, just being teleported between maps.
            bool isBeingTeleported = player->IsBeingTeleportedFar() || player->IsBeingTeleportedNear();
            if (!player->IsInWorld() && !isBeingTeleported)
                continue;

            // Only process human players
            if (IsPlayerBotCheck(player))
                continue;

            ObjectGuid playerGuid = player->GetGUID();

            // Get player's current LFG state
            lfg::LfgState state = sLFGMgr->GetState(playerGuid);

            // ENHANCED DIAGNOSTICS: Log state for all players in DUNGEON or teleporting state
            if (state == lfg::LFG_STATE_DUNGEON || isBeingTeleported)
            {
                TC_LOG_DEBUG("module.playerbot.lfg",
                    "PollGroupFormation: Checking human {} - State: {} - IsInWorld: {} - Teleporting: {} (Far:{} Near:{})",
                    player->GetName(), static_cast<uint32>(state),
                    player->IsInWorld(), isBeingTeleported,
                    player->IsBeingTeleportedFar(), player->IsBeingTeleportedNear());
            }

            // We're looking for players who just entered LFG_STATE_DUNGEON
            if (state != lfg::LFG_STATE_DUNGEON)
                continue;

            // Check previous state - only process transition TO dungeon state
            auto lastStateIt = _lastQueueState.find(playerGuid);
            if (lastStateIt == _lastQueueState.end())
            {
                // No previous state tracked, but player is in dungeon - still process
                // This handles edge cases where state tracking was missed
                TC_LOG_INFO("module.playerbot.lfg",
                    "PollGroupFormation: Human {} in DUNGEON state, no previous state tracked - processing as new transition",
                    player->GetName());
            }
            else if (lastStateIt->second == lfg::LFG_STATE_DUNGEON)
            {
                // Already in dungeon state, not a new transition
                continue;
            }
            else
            {
                TC_LOG_INFO("module.playerbot.lfg",
                    "PollGroupFormation: Human {} transitioned from state {} to DUNGEON",
                    player->GetName(), static_cast<uint32>(lastStateIt->second));
            }

            // Update tracked state BEFORE potential continues
            _lastQueueState[playerGuid] = state;

            // Get the group GUID from LFG system
            ObjectGuid groupGuid = sLFGMgr->GetGroup(playerGuid);
            if (groupGuid.IsEmpty())
            {
                TC_LOG_WARN("module.playerbot.lfg",
                    "PollGroupFormation: Human {} in LFG_STATE_DUNGEON but no group GUID - LFGMgr::GetGroup returned empty",
                    player->GetName());
                continue;
            }

            // Check if we've already processed this group
            if (_processedGroups.count(groupGuid))
            {
                TC_LOG_DEBUG("module.playerbot.lfg",
                    "PollGroupFormation: Group {} already processed for human {}",
                    groupGuid.ToString(), player->GetName());
                continue;
            }

            // Get the actual Group object to verify it's valid
            Group* group = player->GetGroup();
            if (!group)
            {
                TC_LOG_WARN("module.playerbot.lfg",
                    "PollGroupFormation: Human {} has LFG group GUID {} but player->GetGroup() returned null",
                    player->GetName(), groupGuid.ToString());
                continue;
            }

            // Verify this is an LFG group
            if (!group->isLFGGroup())
            {
                TC_LOG_WARN("module.playerbot.lfg",
                    "PollGroupFormation: Group {} for human {} is not an LFG group (isLFGGroup() = false)",
                    groupGuid.ToString(), player->GetName());
                continue;
            }

            // Mark group as processed BEFORE calling OnGroupFormed to prevent re-entry
            _processedGroups.insert(groupGuid);
            _processedGroupTimes[groupGuid] = now;

            // Get current leader for diagnostic logging
            ObjectGuid currentLeader = group->GetLeaderGUID();
            TC_LOG_INFO("module.playerbot.lfg",
                "PollGroupFormation: ✅ DETECTED LFG group formed! Human: {} ({}), Group: {}, Current Leader: {} - triggering leadership transfer",
                player->GetName(), playerGuid.ToString(), groupGuid.ToString(), currentLeader.ToString());

            // CRITICAL: Call LFGBotManager::OnGroupFormed() to trigger leadership transfer
            // This was the MISSING CALL that caused bots to remain as leaders
            sLFGBotManager->OnGroupFormed(groupGuid);
        }

        // Cleanup old processed groups (older than 30 minutes is stale)
        if (!_processedGroupTimes.empty())
        {
            constexpr uint32 GROUP_RECORD_TIMEOUT = 30 * MINUTE * IN_MILLISECONDS;

            std::vector<ObjectGuid> toRemove;
            for (auto const& groupPair : _processedGroupTimes)
            {
                if (now - groupPair.second > GROUP_RECORD_TIMEOUT)
                    toRemove.push_back(groupPair.first);
            }

            for (ObjectGuid const& guid : toRemove)
            {
                _processedGroups.erase(guid);
                _processedGroupTimes.erase(guid);
            }

            if (!toRemove.empty())
            {
                TC_LOG_DEBUG("module.playerbot.lfg",
                    "PollGroupFormation: Cleaned up {} stale group records", toRemove.size());
            }
        }
    }

    /**
     * @brief Handle a player joining the LFG queue
     *
     * Uses the Hybrid Instance Bot System:
     * 1. Triggers InstanceBotHooks to create/reserve bots from pool
     * 2. Falls back to LFGBotManager to use existing online bots
     */
    void HandlePlayerJoinedQueue(Player* player)
    {
        ObjectGuid playerGuid = player->GetGUID();

        // Check if we've already processed this player recently
        if (_processedPlayers.count(playerGuid))
        {
            TC_LOG_DEBUG("module.playerbot.lfg",
                "PlayerbotLFGScript: Player {} already processed, skipping", player->GetName());
            return;
        }

        TC_LOG_INFO("module.playerbot.lfg",
            "PlayerbotLFGScript: Detected player {} joined LFG queue", player->GetName());

        // Get the player's role and dungeons
        uint8 roles = sLFGMgr->GetRoles(playerGuid);
        lfg::LfgDungeonSet const& dungeons = sLFGMgr->GetSelectedDungeons(playerGuid);

        if (dungeons.empty())
        {
            TC_LOG_WARN("module.playerbot.lfg",
                "PlayerbotLFGScript: Player {} has no selected dungeons", player->GetName());
            return;
        }

        // HYBRID APPROACH: Use both new and old systems
        //
        // Step 1: Trigger the new Instance Bot System to create/reserve bots
        // This ensures bots exist (created via BotCloneEngine) and are being warmed up
        if (InstanceBotHooks::IsEnabled())
        {
            TC_LOG_INFO("module.playerbot.lfg",
                "PlayerbotLFGScript: Triggering Instance Bot System for player {}", player->GetName());

            // Convert lfg::LfgDungeonSet to std::set<uint32>
            std::set<uint32> dungeonSet(dungeons.begin(), dungeons.end());

            InstanceBotHooks::OnPlayerJoinLfg(player, dungeonSet, roles);
        }

        // Step 2: Use LFGBotManager to add CURRENTLY ONLINE bots to queue
        // This handles the case where pool bots are already warmed up
        sLFGBotManager->OnPlayerJoinQueue(player, roles, dungeons);

        // Mark player as processed
        _processedPlayers.insert(playerGuid);
        _processedPlayerTimes[playerGuid] = GameTime::GetGameTimeMS();

        TC_LOG_INFO("module.playerbot.lfg",
            "PlayerbotLFGScript: Triggered bot recruitment for player {} (roles: {}, dungeons: {})",
            player->GetName(), static_cast<uint32>(roles), dungeons.size());
    }

    /**
     * @brief Handle a player leaving the LFG queue
     */
    void HandlePlayerLeftQueue(Player* player)
    {
        ObjectGuid playerGuid = player->GetGUID();

        TC_LOG_INFO("module.playerbot.lfg",
            "PlayerbotLFGScript: Player {} left LFG queue", player->GetName());

        // Notify LFGBotManager to cleanup bots assigned to this player
        sLFGBotManager->OnPlayerLeaveQueue(playerGuid);

        // Remove from processed set so they can queue again
        _processedPlayers.erase(playerGuid);
        _processedPlayerTimes.erase(playerGuid);
    }

    /**
     * @brief Handle role check for a player
     */
    void HandleRoleCheck(Player* player)
    {
        ObjectGuid playerGuid = player->GetGUID();

        TC_LOG_DEBUG("module.playerbot.lfg",
            "PlayerbotLFGScript: Role check started for player {}", player->GetName());

        // Get the group GUID
        ObjectGuid groupGuid = sLFGMgr->GetGroup(playerGuid);
        if (!groupGuid.IsEmpty())
        {
            // Notify LFGBotManager to confirm roles for all bots in this group
            sLFGBotManager->OnRoleCheckReceived(groupGuid);
        }
    }

    /**
     * @brief Check if a player is a bot
     */
    bool IsPlayerBotCheck(Player* player) const
    {
        if (!player)
            return false;

        // Use PlayerBotHooks::IsPlayerBot for consistent bot detection
        return PlayerBotHooks::IsPlayerBot(player);
    }

    /**
     * @brief Cleanup stale tracking data
     */
    void CleanupStaleData()
    {
        uint32 now = GameTime::GetGameTimeMS();

        // Only cleanup every 5 minutes
        if (now - _lastCleanupTime < CLEANUP_INTERVAL)
            return;
        _lastCleanupTime = now;

        // Remove processed players that haven't re-queued in 10 minutes
        constexpr uint32 STALE_THRESHOLD = 10 * MINUTE * IN_MILLISECONDS;

        std::vector<ObjectGuid> toRemove;
        for (auto const& pair : _processedPlayerTimes)
        {
            if (now - pair.second > STALE_THRESHOLD)
            {
                toRemove.push_back(pair.first);
            }
        }

        for (ObjectGuid const& guid : toRemove)
        {
            _processedPlayers.erase(guid);
            _processedPlayerTimes.erase(guid);
            _lastQueueState.erase(guid);
        }

        // Also cleanup bot proposal accept times (older than 1 minute is stale)
        constexpr uint32 BOT_PROPOSAL_STALE_THRESHOLD = 60 * IN_MILLISECONDS;
        std::vector<ObjectGuid> staleBotTimes;
        for (auto const& pair : _botProposalAcceptTimes)
        {
            if (now - pair.second > BOT_PROPOSAL_STALE_THRESHOLD)
            {
                staleBotTimes.push_back(pair.first);
            }
        }
        for (ObjectGuid const& guid : staleBotTimes)
        {
            _botProposalAcceptTimes.erase(guid);
        }

        if (!toRemove.empty() || !staleBotTimes.empty())
        {
            TC_LOG_DEBUG("module.playerbot.lfg",
                "PlayerbotLFGScript: Cleaned up {} stale player entries, {} stale bot proposal times",
                toRemove.size(), staleBotTimes.size());
        }
    }

    // Configuration
    // RELIABILITY FIX: Reduced from 1000ms to 500ms for faster queue detection
    // While LFG is less time-sensitive than BG, faster detection still improves
    // the user experience by reducing wait time for bots to join
    static constexpr uint32 LFG_POLL_INTERVAL = 500; // 500ms for responsive detection
    static constexpr uint32 CLEANUP_INTERVAL = 5 * MINUTE * IN_MILLISECONDS;

    // State tracking
    uint32 _updateAccumulator = 0;
    uint32 _lastCleanupTime = 0;

    // Processed players (to avoid duplicate bot additions)
    std::unordered_set<ObjectGuid> _processedPlayers;
    std::unordered_map<ObjectGuid, uint32> _processedPlayerTimes;

    // Last known LFG state for each player (for detecting transitions)
    std::unordered_map<ObjectGuid, lfg::LfgState> _lastQueueState;

    // Processed proposals
    std::unordered_set<uint32> _processedProposals;

    // Bot proposal acceptance tracking (debounce)
    std::unordered_map<ObjectGuid, uint32> _botProposalAcceptTimes;

    // Bot dungeon teleport tracking (debounce for PollDungeonTeleports)
    std::unordered_map<ObjectGuid, uint32> _botDungeonTeleportTimes;

    // Track highest proposal ID seen for efficient scanning
    uint32 _highestProposalIdSeen = 0;

    // Processed LFG groups (to avoid duplicate OnGroupFormed calls)
    std::unordered_set<ObjectGuid> _processedGroups;
    std::unordered_map<ObjectGuid, uint32> _processedGroupTimes;
};

void AddSC_PlayerbotLFGScript()
{
    new PlayerbotLFGScript();
}

} // namespace Playerbot
