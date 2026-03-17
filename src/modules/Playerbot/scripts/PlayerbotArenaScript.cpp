/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * Arena Bot Integration Script - Module-Only Approach
 *
 * This script integrates the ArenaBotManager with TrinityCore's Arena system
 * using a polling approach that requires NO core file modifications.
 *
 * Design:
 * - Uses WorldScript::OnUpdate to periodically check arena queue state
 * - Detects human players who have joined arena queues
 * - Triggers bot recruitment via ArenaBotManager::OnPlayerJoinQueue
 * - Monitors invitations for automatic bot acceptance
 * - Supports both rated arenas (2v2/3v3) and skirmishes
 *
 * Note: Solo Shuffle is NOT available in TrinityCore 12.0
 */

#include "ScriptMgr.h"
#include "Player.h"
#include "World.h"
#include "WorldSession.h"
#include "GameTime.h"
#include "BattlegroundMgr.h"
#include "Battleground.h"
#include "../Session/BotWorldSessionMgr.h"
#include "../Core/PlayerBotHooks.h"
#include "../PvP/ArenaBotManager.h"
#include "Log.h"
#include <unordered_set>
#include <unordered_map>

namespace Playerbot
{

/**
 * @brief Arena Bot Integration using polling approach
 *
 * This WorldScript polls the arena system periodically to detect human players
 * who have joined the queue, then triggers bot recruitment for teammates and opponents.
 */
class PlayerbotArenaScript : public WorldScript
{
public:
    PlayerbotArenaScript() : WorldScript("PlayerbotArenaScript") {}

    void OnUpdate(uint32 diff) override
    {
        // Throttle polling to every 1 second
        _updateAccumulator += diff;
        if (_updateAccumulator < ARENA_POLL_INTERVAL)
            return;
        _updateAccumulator = 0;

        // Skip if Arena bot manager is not enabled
        if (!sArenaBotManager || !sArenaBotManager->IsEnabled())
            return;

        // Update ArenaBotManager
        sArenaBotManager->Update(diff);

        // Poll for new queued players
        PollQueuedPlayers();

        // Cleanup stale tracking data
        CleanupStaleData();
    }

    void OnStartup() override
    {
        TC_LOG_INFO("module.playerbot.arena", "PlayerbotArenaScript: Initializing arena bot integration...");

        if (sArenaBotManager)
        {
            sArenaBotManager->Initialize();
            TC_LOG_INFO("module.playerbot.arena", "PlayerbotArenaScript: ArenaBotManager initialized");
        }
        else
        {
            TC_LOG_ERROR("module.playerbot.arena", "PlayerbotArenaScript: ArenaBotManager not available!");
        }
    }

    void OnShutdownInitiate(ShutdownExitCode /*code*/, ShutdownMask /*mask*/) override
    {
        TC_LOG_INFO("module.playerbot.arena", "PlayerbotArenaScript: Shutting down arena bot integration...");

        if (sArenaBotManager)
        {
            sArenaBotManager->Shutdown();
        }

        _processedPlayers.clear();
        _lastQueueState.clear();
    }

private:
    /**
     * @brief Queue state info for tracking
     */
    struct ArenaQueueState
    {
        bool inQueue = false;
        ArenaBracketType bracketType = ArenaBracketType::ARENA_2v2;
        ArenaQueueMode mode = ArenaQueueMode::SKIRMISH;
    };

    /**
     * @brief Poll all online players to detect new arena queue joins
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

            // Check if player is in arena queue
            ArenaQueueState currentState;
            DetectArenaQueueState(player, currentState);

            // Track state changes
            auto lastStateIt = _lastQueueState.find(playerGuid);
            ArenaQueueState lastState;
            if (lastStateIt != _lastQueueState.end())
                lastState = lastStateIt->second;

            _lastQueueState[playerGuid] = currentState;

            // Handle state transitions
            if (currentState.inQueue && !lastState.inQueue)
            {
                // Player just joined the arena queue
                HandlePlayerJoinedQueue(player, currentState.bracketType, currentState.mode);
            }
            else if (!currentState.inQueue && lastState.inQueue)
            {
                // Player left the arena queue
                HandlePlayerLeftQueue(player);
            }
        }
    }

    /**
     * @brief Detect arena queue state for a player
     */
    void DetectArenaQueueState(Player* player, ArenaQueueState& state)
    {
        state.inQueue = false;

        for (uint8 i = 0; i < PLAYER_MAX_BATTLEGROUND_QUEUES; ++i)
        {
            BattlegroundQueueTypeId queueTypeId = player->GetBattlegroundQueueTypeId(i);
            if (queueTypeId == BATTLEGROUND_QUEUE_NONE)
                continue;

            // Check if this is an arena queue
            // In TrinityCore 12.0, BattlemasterListId maps to BattlegroundTypeId
            BattlegroundTypeId bgTypeId = BattlegroundTypeId(queueTypeId.BattlemasterListId);
            if (bgTypeId != BATTLEGROUND_AA) // BATTLEGROUND_AA is arena
                continue;

            state.inQueue = true;

            // Determine bracket type and mode from queue type
            // In 12.0, access queueTypeId members directly
            BattlegroundQueueIdType queueIdType = BattlegroundQueueIdType(queueTypeId.Type);

            if (queueIdType == BattlegroundQueueIdType::ArenaSkirmish)
            {
                state.mode = ArenaQueueMode::SKIRMISH;
                // TeamSize directly contains arena type (2, 3, or 5)
                uint8 arenaType = queueTypeId.TeamSize;
                if (arenaType == 2)
                    state.bracketType = ArenaBracketType::SKIRMISH_2v2;
                else if (arenaType == 3)
                    state.bracketType = ArenaBracketType::SKIRMISH_3v3;
                else
                    state.bracketType = ArenaBracketType::SKIRMISH_2v2; // Default
            }
            else if (queueIdType == BattlegroundQueueIdType::Arena)
            {
                state.mode = ArenaQueueMode::RATED;
                uint8 arenaType = queueTypeId.TeamSize;
                if (arenaType == 2)
                    state.bracketType = ArenaBracketType::ARENA_2v2;
                else if (arenaType == 3)
                    state.bracketType = ArenaBracketType::ARENA_3v3;
                else if (arenaType == 5)
                    state.bracketType = ArenaBracketType::ARENA_5v5;
                else
                    state.bracketType = ArenaBracketType::ARENA_2v2; // Default
            }

            break; // Found arena queue
        }
    }

    /**
     * @brief Handle a player joining the arena queue
     */
    void HandlePlayerJoinedQueue(Player* player, ArenaBracketType bracketType, ArenaQueueMode mode)
    {
        ObjectGuid playerGuid = player->GetGUID();

        // Check if already processed
        if (_processedPlayers.count(playerGuid))
        {
            TC_LOG_DEBUG("module.playerbot.arena",
                "PlayerbotArenaScript: Player {} already processed, skipping", player->GetName());
            return;
        }

        uint8 teamSize = sArenaBotManager->GetTeamSize(bracketType);
        TC_LOG_INFO("module.playerbot.arena",
            "PlayerbotArenaScript: Detected player {} joined arena queue ({}v{}, Mode: {})",
            player->GetName(), teamSize, teamSize,
            mode == ArenaQueueMode::RATED ? "Rated" : "Skirmish");

        // Call ArenaBotManager to populate queue with bots
        sArenaBotManager->OnPlayerJoinQueue(player, bracketType, mode, player->GetGroup() != nullptr);

        // Mark as processed
        _processedPlayers.insert(playerGuid);
        _processedPlayerTimes[playerGuid] = GameTime::GetGameTimeMS();
    }

    /**
     * @brief Handle a player leaving the arena queue
     */
    void HandlePlayerLeftQueue(Player* player)
    {
        ObjectGuid playerGuid = player->GetGUID();

        TC_LOG_INFO("module.playerbot.arena",
            "PlayerbotArenaScript: Player {} left arena queue", player->GetName());

        // Notify ArenaBotManager
        sArenaBotManager->OnPlayerLeaveQueue(playerGuid);

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
     * @brief Cleanup stale tracking data
     */
    void CleanupStaleData()
    {
        uint32 now = GameTime::GetGameTimeMS();

        if (now - _lastCleanupTime < CLEANUP_INTERVAL)
            return;
        _lastCleanupTime = now;

        constexpr uint32 STALE_THRESHOLD = 15 * MINUTE * IN_MILLISECONDS;

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
            TC_LOG_DEBUG("module.playerbot.arena",
                "PlayerbotArenaScript: Cleaned up {} stale player entries", toRemove.size());
        }
    }

    // Configuration
    static constexpr uint32 ARENA_POLL_INTERVAL = 1000; // 1 second
    static constexpr uint32 CLEANUP_INTERVAL = 5 * MINUTE * IN_MILLISECONDS;

    // State tracking
    uint32 _updateAccumulator = 0;
    uint32 _lastCleanupTime = 0;

    // Processed players
    std::unordered_set<ObjectGuid> _processedPlayers;
    std::unordered_map<ObjectGuid, uint32> _processedPlayerTimes;

    // Last known queue state
    std::unordered_map<ObjectGuid, ArenaQueueState> _lastQueueState;
};

void AddSC_PlayerbotArenaScript()
{
    new PlayerbotArenaScript();
}

} // namespace Playerbot
