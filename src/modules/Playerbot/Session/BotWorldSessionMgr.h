/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 */

#ifndef BOT_WORLD_SESSION_MGR_H
#define BOT_WORLD_SESSION_MGR_H

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include "ObjectGuid.h"
#include "QueryHolder.h"
#include "DatabaseEnvFwd.h"
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <mutex>
#include <atomic>
#include <boost/lockfree/queue.hpp>

class WorldSession;
class Player;
class SQLQueryHolderBase;

namespace Playerbot {
    class BotSession;
}

namespace Playerbot {

/**
 * Clean implementation using TrinityCore''s native login pattern
 * Based on mod-playerbots'' proven approach with modern enhancements
 */
class TC_GAME_API BotWorldSessionMgr final
{
public:
    // Thread-safe singleton
    static BotWorldSessionMgr* instance()
    {
        static BotWorldSessionMgr instance;
        return &instance;
    }

    // Basic lifecycle
    bool Initialize();
    void Shutdown();

    // Bot management using TrinityCore''s native login
    bool AddPlayerBot(ObjectGuid playerGuid, uint32 masterAccountId = 0, bool bypassLimit = false);
    void RemovePlayerBot(ObjectGuid playerGuid);
    Player* GetPlayerBot(ObjectGuid playerGuid) const;

    // Session updates
    void UpdateSessions(uint32 diff);

    // Deferred packet processing (main thread only!)
    // Processes packets queued by worker threads that require serialization with Map::Update()
    uint32 ProcessAllDeferredPackets();

    // Administrative
    uint32 GetBotCount() const;
    bool IsEnabled() const { return _enabled.load(); }
    void SetEnabled(bool enabled) { _enabled.store(enabled); }

    // Character login trigger (compatibility with existing system)
    void TriggerCharacterLoginForAllSessions();

    // Chat command support - NEW APIs for command system
    ::std::vector<Player*> GetPlayerBotsByAccount(uint32 accountId) const;
    void RemoveAllPlayerBots(uint32 accountId);
    uint32 GetBotCountByAccount(uint32 accountId) const;

    // All-bots operations (for LFG, BG, etc.)
    ::std::vector<Player*> GetAllBotPlayers() const;

    // Check if a bot is currently in the loading queue
    // Used by ProcessPendingBGQueues to avoid double-queuing race conditions
    bool IsBotLoading(ObjectGuid botGuid) const;

    // ========================================================================
    // INSTANCE BOT MANAGEMENT
    // ========================================================================
    // Instance bots (JIT/warm pool) are for instanced content only.
    // They auto-logout after 60 seconds of being idle (not in queue/group).
    // ========================================================================

    /**
     * @brief Mark a bot as an instance bot (JIT or warm pool)
     * @param botGuid Bot to mark
     *
     * Called by: InstanceBotOrchestrator when bot is assigned to instance content
     * Effect: Enables automatic logout when not in queue or group
     */
    void MarkAsInstanceBot(ObjectGuid botGuid);

    /**
     * @brief Get count of bots marked as instance bots
     * @return Number of currently active instance bots
     *
     * Used by JITBotFactory hard cap check to prevent bot explosion.
     * Instance bots are those marked via MarkAsInstanceBot() - they have
     * the 60-second idle timeout and restricted behavior.
     */
    uint32 GetInstanceBotCount() const;

private:
    BotWorldSessionMgr() = default;
    ~BotWorldSessionMgr() = default;
    BotWorldSessionMgr(const BotWorldSessionMgr&) = delete;
    BotWorldSessionMgr& operator=(const BotWorldSessionMgr&) = delete;

    // Native login callback (TrinityCore pattern)
    void HandlePlayerBotLoginCallback(SQLQueryHolderBase const& holder, uint32 masterAccountId, ObjectGuid playerGuid);

    // Character cache synchronization
    bool SynchronizeCharacterCache(ObjectGuid playerGuid);

    // Session management
    ::std::unordered_map<ObjectGuid, ::std::shared_ptr<BotSession>> _botSessions;
    ::std::unordered_set<ObjectGuid> _botsLoading;

    // CRITICAL FIX: Spawn throttling to prevent database overload
    // Queue pending bot spawns and process them at controlled rate
    ::std::vector<::std::pair<ObjectGuid, uint32>> _pendingSpawns;  // <playerGuid, masterAccountId>
    uint32 _spawnsProcessedThisTick{0};
    uint32 _maxSpawnsPerTick{10};  // From Playerbot.LevelManager.MaxBotsPerUpdate config

    // Thread safety
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::SESSION_MANAGER> _sessionsMutex;
    ::std::atomic<bool> _initialized{false};
    ::std::atomic<bool> _enabled{false};

    // ENTERPRISE-GRADE PERFORMANCE SYSTEM (Phase 2)
    // Tracks ticks for priority scheduling
    uint32 _tickCounter{0};

    // DEPRECATED: Simple rotation (Phase 1) - kept for fallback
    // Use enterprise priority system for optimal 5000 bot performance
    uint32 _updateRotationIndex{0};
    static constexpr uint32 MAX_BOTS_PER_UPDATE = 100;

    // ENTERPRISE SYSTEM: Priority-based update management
    // Managed by BotPriorityManager, BotPerformanceMonitor, and BotHealthCheck
    bool _enterpriseMode{true};  // Toggle between simple and enterprise mode

    // OPTION 5: Lock-free async cleanup queue for disconnected sessions
    // Worker threads push disconnected bot GUIDs here (thread-safe, no mutex needed)
    boost::lockfree::queue<ObjectGuid> _asyncDisconnections{1000};

    // Atomic counter for bot updates completed asynchronously
    ::std::atomic<uint32> _asyncBotsUpdated{0};

    // CRITICAL FIX: Two-phase deferred logout queue to prevent Cell::Visit crash (GridNotifiers.cpp:237)
    // Problem: When LogoutPlayer() is called during UpdateSessions(), it removes the Player from the Grid.
    //          But Map worker threads (ProcessRelocationNotifies) may already have iterators pointing to
    //          that Player, causing ACCESS_VIOLATION when they access player->m_seer.
    // Solution: Two-phase deferred cleanup:
    //   Phase 1 (current tick): Collect disconnected GUIDs into _pendingLogouts (don''t logout yet)
    //   Phase 2 (next tick): Move _pendingLogouts to _readyForLogout, then call LogoutPlayer()
    //   This ensures LogoutPlayer() is only called AFTER the map update cycle for the tick where
    //   the disconnection was detected has fully completed.
    ::std::vector<ObjectGuid> _pendingLogouts;    // Collected this tick, will be processed next tick
    ::std::vector<ObjectGuid> _readyForLogout;    // Moved from _pendingLogouts, safe to logout now
};

// Global instance accessor
#define sBotWorldSessionMgr BotWorldSessionMgr::instance()

} // namespace Playerbot

#endif // BOT_WORLD_SESSION_MGR_H
