/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * Battleground Bot Integration Script - Module-Only Approach
 *
 * This script integrates the BGBotManager with TrinityCore's Battleground system
 * using a polling approach that requires NO core file modifications.
 *
 * Design:
 * - Uses WorldScript::OnUpdate to periodically check BG queue state
 * - Detects human players who have joined BG queues
 * - Triggers bot recruitment via BGBotManager::OnPlayerJoinQueue
 * - Monitors invitations for automatic bot acceptance
 * - Tracks processed players to avoid duplicate bot additions
 */

#include "ScriptMgr.h"
#include "Player.h"
#include "World.h"
#include "WorldSession.h"
#include "GameTime.h"
#include "BattlegroundMgr.h"
#include "Battleground.h"
#include "DB2Stores.h"
#include "../Session/BotWorldSessionMgr.h"
#include "../Core/PlayerBotHooks.h"
#include "../PvP/BGBotManager.h"
#include "../AI/Coordination/Battleground/BattlegroundCoordinatorManager.h"
#include "../Lifecycle/Instance/InstanceBotHooks.h"
#include "../Lifecycle/Instance/QueueStatePoller.h"
#include "ObjectAccessor.h"
#include "Log.h"
#include <unordered_set>
#include <unordered_map>

namespace Playerbot
{

/**
 * @brief Battleground Bot Integration using polling approach
 *
 * This WorldScript polls the BG system periodically to detect human players
 * who have joined the queue, then triggers bot recruitment to fill both teams.
 */
class PlayerbotBGScript : public WorldScript
{
public:
    PlayerbotBGScript() : WorldScript("PlayerbotBGScript") {}

    void OnUpdate(uint32 diff) override
    {
        // Throttle polling to every 1 second
        _updateAccumulator += diff;
        if (_updateAccumulator < BG_POLL_INTERVAL)
            return;
        _updateAccumulator = 0;

        // Skip if BG bot manager is not enabled
        if (!sBGBotManager || !sBGBotManager->IsEnabled())
            return;

        // Update BGBotManager (handles bot queue management)
        sBGBotManager->Update(diff);

        // Update BattlegroundCoordinatorManager (handles active BG coordination)
        if (sBGCoordinatorMgr)
        {
            sBGCoordinatorMgr->Update(diff);
        }

        // CRITICAL: Monitor active BGs for status transitions
        // This ensures bots are added when BG transitions to IN_PROGRESS
        MonitorActiveBattlegrounds();

        // Poll for new queued players
        PollQueuedPlayers();

        // Cleanup stale tracking data
        CleanupStaleData();
    }

    void OnStartup() override
    {
        TC_LOG_INFO("module.playerbot.bg", "PlayerbotBGScript: Initializing BG bot integration...");

        // Initialize BGBotManager (handles queue population)
        if (sBGBotManager)
        {
            sBGBotManager->Initialize();
            TC_LOG_INFO("module.playerbot.bg", "PlayerbotBGScript: BGBotManager initialized");
        }
        else
        {
            TC_LOG_ERROR("module.playerbot.bg", "PlayerbotBGScript: BGBotManager not available!");
        }

        // Initialize BattlegroundCoordinatorManager (handles strategic coordination)
        if (sBGCoordinatorMgr)
        {
            sBGCoordinatorMgr->Initialize();
            TC_LOG_INFO("module.playerbot.bg", "PlayerbotBGScript: BattlegroundCoordinatorManager initialized");
        }
        else
        {
            TC_LOG_ERROR("module.playerbot.bg", "PlayerbotBGScript: BattlegroundCoordinatorManager not available!");
        }
    }

    void OnShutdownInitiate(ShutdownExitCode /*code*/, ShutdownMask /*mask*/) override
    {
        TC_LOG_INFO("module.playerbot.bg", "PlayerbotBGScript: Shutting down BG bot integration...");

        // Shutdown coordinator manager first
        if (sBGCoordinatorMgr)
        {
            sBGCoordinatorMgr->Shutdown();
        }

        // Then shutdown bot manager
        if (sBGBotManager)
        {
            sBGBotManager->Shutdown();
        }

        _processedPlayers.clear();
        _lastQueueState.clear();
        _bgNoHumanSince.clear();
    }

private:
    /**
     * @brief Poll all online players to detect new BG queue joins
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

            // Check if player is in BG queue
            bool inQueue = false;
            BattlegroundQueueTypeId currentQueueType = BATTLEGROUND_QUEUE_NONE;
            BattlegroundTypeId bgTypeId = BATTLEGROUND_TYPE_NONE;

            for (uint8 i = 0; i < PLAYER_MAX_BATTLEGROUND_QUEUES; ++i)
            {
                BattlegroundQueueTypeId queueTypeId = player->GetBattlegroundQueueTypeId(i);
                if (queueTypeId != BATTLEGROUND_QUEUE_NONE)
                {
                    // Check if this is a regular BG (not arena)
                    // In TrinityCore 12.0, BattlemasterListId maps to BattlegroundTypeId
                    BattlegroundTypeId type = BattlegroundTypeId(queueTypeId.BattlemasterListId);
                    if (type != BATTLEGROUND_AA) // Not arena
                    {
                        inQueue = true;
                        currentQueueType = queueTypeId;
                        bgTypeId = type;
                        break;
                    }
                }
            }

            // Track state changes
            auto lastStateIt = _lastQueueState.find(playerGuid);
            bool wasInQueue = (lastStateIt != _lastQueueState.end()) ? lastStateIt->second : false;

            _lastQueueState[playerGuid] = inQueue;

            // Handle state transitions
            if (inQueue && !wasInQueue)
            {
                // Player just joined the queue
                HandlePlayerJoinedQueue(player, bgTypeId, currentQueueType);
            }
            else if (!inQueue && wasInQueue)
            {
                // Player left the queue
                HandlePlayerLeftQueue(player);
            }
        }
    }

    /**
     * @brief Handle a player joining the BG queue
     */
    void HandlePlayerJoinedQueue(Player* player, BattlegroundTypeId bgTypeId, BattlegroundQueueTypeId queueTypeId)
    {
        ObjectGuid playerGuid = player->GetGUID();

        // Check if already processed
        if (_processedPlayers.count(playerGuid))
        {
            TC_LOG_DEBUG("module.playerbot.bg",
                "PlayerbotBGScript: Player {} already processed, skipping", player->GetName());
            return;
        }

        TC_LOG_INFO("module.playerbot.bg",
            "PlayerbotBGScript: Detected player {} (level {}) joined BG queue (Type: {})",
            player->GetName(), player->GetLevel(), static_cast<uint32>(bgTypeId));

        // Get bracket from player level using BG template map
        BattlegroundBracketId bracket = BG_BRACKET_ID_LAST;  // Default fallback

        // Get the BG template to find the map ID for bracket lookup
        BattlegroundTemplate const* bgTemplate = sBattlegroundMgr->GetBattlegroundTemplateByTypeId(bgTypeId);
        if (bgTemplate && !bgTemplate->MapIDs.empty())
        {
            // Use DB2Manager to get the correct bracket for player's level
            PVPDifficultyEntry const* bracketEntry = DB2Manager::GetBattlegroundBracketByLevel(
                bgTemplate->MapIDs.front(), player->GetLevel());

            if (bracketEntry)
            {
                bracket = BattlegroundBracketId(bracketEntry->RangeIndex);
                TC_LOG_INFO("module.playerbot.bg",
                    "PlayerbotBGScript: Player {} level {} -> Bracket {} (range {}-{})",
                    player->GetName(), player->GetLevel(), static_cast<uint32>(bracket),
                    bracketEntry->MinLevel, bracketEntry->MaxLevel);
            }
            else
            {
                TC_LOG_WARN("module.playerbot.bg",
                    "PlayerbotBGScript: No bracket found for level {}, using max bracket",
                    player->GetLevel());
            }
        }
        else
        {
            TC_LOG_WARN("module.playerbot.bg",
                "PlayerbotBGScript: No BG template for type {}, using max bracket",
                static_cast<uint32>(bgTypeId));
        }

        // ========================================================================
        // FIX: Use SINGLE system to avoid over-spawning
        // ========================================================================
        // Previously used "HYBRID APPROACH" that triggered THREE systems:
        // 1. InstanceBotHooks (warm pool)
        // 2. BGBotManager (online bots)
        // 3. QueueStatePoller (shortage detection)
        //
        // This caused MASSIVE over-spawning because each system independently
        // calculated "need full BG - 1 human" and spawned that many bots.
        //
        // NEW APPROACH: Use InstanceBotHooks as the PRIMARY system when enabled.
        // It handles warm pool assignment, bot spawning, and proper queue tracking.
        // Only fall back to BGBotManager if InstanceBotHooks is disabled.
        // ========================================================================

        if (InstanceBotHooks::IsEnabled())
        {
            // Use the Instance Bot System (warm pool + JIT) - handles everything
            TC_LOG_INFO("module.playerbot.bg",
                "PlayerbotBGScript: Using Instance Bot System for player {} (BG Type: {}, Bracket: {})",
                player->GetName(), static_cast<uint32>(bgTypeId), static_cast<uint32>(bracket));

            InstanceBotHooks::OnPlayerJoinBattleground(
                player,
                static_cast<uint32>(bgTypeId),
                static_cast<uint32>(bracket),
                player->GetGroup() != nullptr
            );

            // NOTE: Do NOT trigger BGBotManager or QueueStatePoller here!
            // InstanceBotHooks already handles warm pool assignment and will
            // use QueueStatePoller internally if needed for JIT fallback.
        }
        else
        {
            // Fallback: Use BGBotManager for online-only bot queueing
            TC_LOG_INFO("module.playerbot.bg",
                "PlayerbotBGScript: Instance Bot System disabled, using BGBotManager for player {} (BG Type: {}, Bracket: {})",
                player->GetName(), static_cast<uint32>(bgTypeId), static_cast<uint32>(bracket));

            sBGBotManager->OnPlayerJoinQueue(player, bgTypeId, bracket, player->GetGroup() != nullptr);

            // Only register with QueueStatePoller when using fallback path
            sQueueStatePoller->RegisterActiveBGQueue(bgTypeId, bracket);
        }

        // Mark as processed
        _processedPlayers.insert(playerGuid);
        _processedPlayerTimes[playerGuid] = GameTime::GetGameTimeMS();

        TC_LOG_INFO("module.playerbot.bg",
            "PlayerbotBGScript: Triggered bot recruitment for player {} (BG Type: {}, Bracket: {})",
            player->GetName(), static_cast<uint32>(bgTypeId), static_cast<uint32>(bracket));
    }

    /**
     * @brief Handle a player leaving the BG queue
     */
    void HandlePlayerLeftQueue(Player* player, BattlegroundTypeId bgTypeId = BATTLEGROUND_TYPE_NONE)
    {
        ObjectGuid playerGuid = player->GetGUID();

        TC_LOG_INFO("module.playerbot.bg",
            "PlayerbotBGScript: Player {} left BG queue", player->GetName());

        // Notify Instance Bot Hooks (new system)
        if (InstanceBotHooks::IsEnabled())
        {
            InstanceBotHooks::OnPlayerLeaveBattlegroundQueue(player, static_cast<uint32>(bgTypeId));
        }

        // Notify BGBotManager (old system)
        sBGBotManager->OnPlayerLeaveQueue(playerGuid);

        // Remove from processed set
        _processedPlayers.erase(playerGuid);
        _processedPlayerTimes.erase(playerGuid);
    }

    /**
     * @brief Check if a player is a bot
     */
    bool IsPlayerBotCheck(Player* player) const
    {
        if (!player)
            return false;

        return PlayerBotHooks::IsPlayerBot(player);
    }

    /**
     * @brief Monitor active battlegrounds for status transitions
     *
     * This detects when a BG transitions to STATUS_IN_PROGRESS and triggers
     * bot addition to prevent premature finish due to "not enough players"
     */
    void MonitorActiveBattlegrounds()
    {
        // Find active BGs through players who are in battlegrounds
        SessionMap const& sessions = sWorld->GetAllSessions();
        std::unordered_set<uint32> processedBGs;

        for (auto const& pair : sessions)
        {
            WorldSession* session = pair.second;
            if (!session)
                continue;

            Player* player = session->GetPlayer();
            if (!player || !player->IsInWorld())
                continue;

            // Get player's BG if any
            Battleground* bg = player->GetBattleground();
            if (!bg)
                continue;

            uint32 instanceId = bg->GetInstanceID();

            // Skip if already processed this tick
            if (processedBGs.count(instanceId))
                continue;
            processedBGs.insert(instanceId);

            BattlegroundStatus status = bg->GetStatus();
            BattlegroundTypeId bgTypeId = bg->GetTypeID();

            // Skip arenas
            if (bg->isArena())
                continue;

            // Check for status change
            auto lastIt = _bgStatusTracker.find(instanceId);
            BattlegroundStatus lastStatus = (lastIt != _bgStatusTracker.end())
                ? lastIt->second
                : STATUS_NONE;

            // Update tracking
            _bgStatusTracker[instanceId] = status;

            // Detect transition to IN_PROGRESS
            if (status == STATUS_IN_PROGRESS && lastStatus != STATUS_IN_PROGRESS)
            {
                TC_LOG_INFO("module.playerbot.bg",
                    "PlayerbotBGScript: Detected BG {} (instance {}) transition to IN_PROGRESS - triggering bot population",
                    bg->GetName(), instanceId);

                // Trigger bot addition to fill empty slots
                sBGBotManager->OnBattlegroundStart(bg);
            }

            // Handle WAIT_JOIN (preparation phase) -> populate BG with bots
            // so they are present when the human player enters during prep
            if (status == STATUS_WAIT_JOIN && lastStatus == STATUS_NONE)
            {
                TC_LOG_INFO("module.playerbot.bg",
                    "PlayerbotBGScript: BG {} (instance {}) entering prep phase - populating with bots",
                    bg->GetName(), instanceId);

                sBGBotManager->PopulateBattleground(bg);
            }

            // Detect transition to WAIT_LEAVE (BG just ended)
            // CRITICAL FIX: Trigger bot cleanup when BG ends to prevent:
            // 1. Resource leaks (bots lingering on BG map without cleanup)
            // 2. Dangling pointers in Map::_updateObjects (use-after-free crash)
            // 3. Pool bots never being released back to the warm pool
            if (status == STATUS_WAIT_LEAVE && lastStatus == STATUS_IN_PROGRESS)
            {
                Team winner = TEAM_OTHER;
                switch (bg->GetWinner())
                {
                    case PVP_TEAM_ALLIANCE: winner = ALLIANCE; break;
                    case PVP_TEAM_HORDE:    winner = HORDE; break;
                    default:                winner = TEAM_OTHER; break;
                }

                TC_LOG_INFO("module.playerbot.bg",
                    "PlayerbotBGScript: BG {} (instance {}) ended (winner: {}) - triggering bot cleanup",
                    bg->GetName(), instanceId,
                    winner == ALLIANCE ? "Alliance" : (winner == HORDE ? "Horde" : "Draw"));

                sBGBotManager->OnBattlegroundEnd(bg, winner);
            }

            // Cleanup finished BGs from tracker
            if (status == STATUS_WAIT_LEAVE)
            {
                // BG is ending
                _bgStatusTracker.erase(instanceId);
                _bgNoHumanSince.erase(instanceId);
            }

            // =========================================================================
            // CHECK: End BG when no human players remain (with grace period)
            // =========================================================================
            if (status == STATUS_IN_PROGRESS)
            {
                bool humanFound = false;
                for (auto const& playerPair : bg->GetPlayers())
                {
                    Player* bgPlayer = ObjectAccessor::FindPlayer(playerPair.first);
                    if (bgPlayer && !PlayerBotHooks::IsPlayerBot(bgPlayer))
                    {
                        humanFound = true;
                        break;
                    }
                }

                uint32 now = GameTime::GetGameTimeMS();

                if (!humanFound)
                {
                    auto graceIt = _bgNoHumanSince.find(instanceId);
                    if (graceIt == _bgNoHumanSince.end())
                    {
                        // First detection of no humans - start grace timer
                        _bgNoHumanSince[instanceId] = now;
                        TC_LOG_INFO("module.playerbot.bg",
                            "PlayerbotBGScript: No human players in BG {} (instance {}) - starting {}s grace period",
                            bg->GetName(), instanceId, NO_HUMAN_GRACE_PERIOD / IN_MILLISECONDS);
                    }
                    else if (now - graceIt->second >= NO_HUMAN_GRACE_PERIOD)
                    {
                        // Grace period expired - end the BG as a draw
                        TC_LOG_INFO("module.playerbot.bg",
                            "PlayerbotBGScript: No human players in BG instance {} - ending after {}s grace period",
                            instanceId, NO_HUMAN_GRACE_PERIOD / IN_MILLISECONDS);

                        // Cleanup bots BEFORE ending the BG to ensure proper removal
                        sBGBotManager->OnBattlegroundEnd(bg, TEAM_OTHER);

                        bg->EndBattleground(TEAM_OTHER);

                        _bgStatusTracker.erase(instanceId);
                        _bgNoHumanSince.erase(instanceId);
                    }
                }
                else
                {
                    // Human found - clear grace timer if it was running
                    _bgNoHumanSince.erase(instanceId);
                }
            }
        }

        // Periodic cleanup of stale entries (every 5 minutes)
        static uint32 lastBgCleanup = 0;
        uint32 now = GameTime::GetGameTimeMS();
        if (now - lastBgCleanup > 5 * MINUTE * IN_MILLISECONDS)
        {
            lastBgCleanup = now;
            // Remove entries for BGs that no longer exist
            std::vector<uint32> keysToRemove;
            for (auto const& [instanceId, status] : _bgStatusTracker)
            {
                // Try to find BG - if not found, it's gone
                Battleground* bg = sBattlegroundMgr->GetBattleground(instanceId, BATTLEGROUND_TYPE_NONE);
                if (!bg)
                {
                    keysToRemove.push_back(instanceId);
                }
            }
            for (uint32 key : keysToRemove)
            {
                _bgStatusTracker.erase(key);
                _bgNoHumanSince.erase(key);
            }
        }
    }

    /**
     * @brief Cleanup stale tracking data
     */
    void CleanupStaleData()
    {
        uint32 now = GameTime::GetGameTimeMS();

        if (now - _lastCleanupTime < CLEANUP_INTERVAL)
            return;
        _lastCleanupTime = now;

        constexpr uint32 STALE_THRESHOLD = 30 * MINUTE * IN_MILLISECONDS;

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

        if (!toRemove.empty())
        {
            TC_LOG_DEBUG("module.playerbot.bg",
                "PlayerbotBGScript: Cleaned up {} stale player entries", toRemove.size());
        }
    }

    // Configuration
    static constexpr uint32 BG_POLL_INTERVAL = 1000; // 1 second polling interval
    static constexpr uint32 CLEANUP_INTERVAL = 5 * MINUTE * IN_MILLISECONDS;
    static constexpr uint32 NO_HUMAN_GRACE_PERIOD = 30 * IN_MILLISECONDS; // 30 seconds

    // State tracking
    uint32 _updateAccumulator = 0;
    uint32 _lastCleanupTime = 0;

    // Processed players
    std::unordered_set<ObjectGuid> _processedPlayers;
    std::unordered_map<ObjectGuid, uint32> _processedPlayerTimes;

    // Last known queue state
    std::unordered_map<ObjectGuid, bool> _lastQueueState;

    // BG status tracker: instanceId -> last known status
    std::unordered_map<uint32, BattlegroundStatus> _bgStatusTracker;

    // Human-absence grace timer: instanceId -> timestamp when last human was detected absent
    std::unordered_map<uint32, uint32> _bgNoHumanSince;
};

void AddSC_PlayerbotBGScript()
{
    new PlayerbotBGScript();
}

} // namespace Playerbot
