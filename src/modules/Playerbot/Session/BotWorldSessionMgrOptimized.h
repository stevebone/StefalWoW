/*
 * Copyright (C) 2024 TrinityCore Playerbot Module
 *
 * Optimized Bot World Session Manager for 5000+ Concurrent Bots
 */

#ifndef MODULE_PLAYERBOT_BOT_WORLD_SESSION_MGR_OPTIMIZED_H
#define MODULE_PLAYERBOT_BOT_WORLD_SESSION_MGR_OPTIMIZED_H

#include "Common.h"
#include "ObjectGuid.h"
#include <memory>
#include <atomic>
#include <tbb/concurrent_hash_map.h>
#include <tbb/concurrent_vector.h>
#include <tbb/concurrent_queue.h>
#include "../Threading/ThreadingPolicy.h"

class BotSession;
class Player;
class WorldSession;

namespace Playerbot
{

/**
 * Thread-Safe Bot Session Manager with Zero Contention
 *
 * CRITICAL IMPROVEMENTS:
 * 1. Lock-free concurrent hash maps for session storage
 * 2. Atomic operations for all counters
 * 3. Parallel session updates using work-stealing
 * 4. Memory-efficient session pooling
 * 5. Zero-copy session iteration
 */
class BotWorldSessionMgrOptimized
{
public:
    // Session statistics (all atomic for wait-free access)
    struct SessionStatistics
    {
        ::std::atomic<uint32> totalSessions{0};
        ::std::atomic<uint32> activeSessions{0};
        ::std::atomic<uint32> loadingSessions{0};
        ::std::atomic<uint32> failedLogins{0};
        ::std::atomic<uint32> successfulLogins{0};
        ::std::atomic<uint64> totalUpdateTime{0}; // microseconds
        ::std::atomic<uint32> updateCycles{0};
        ::std::atomic<uint64> averageUpdateTime{0}; // nanoseconds per session

        void RecordUpdate(uint64 timeMicros, uint32 sessionCount)
        {
            totalUpdateTime.fetch_add(timeMicros, ::std::memory_order_relaxed);
            updateCycles.fetch_add(1, ::std::memory_order_relaxed);

            if (sessionCount > 0)
            {
                uint64 avgNanos = (timeMicros * 1000) / sessionCount;
                averageUpdateTime.store(avgNanos, ::std::memory_order_relaxed);
            }
        }
    };

    // Session state for tracking
    enum class SessionState : uint8
    {
        INITIALIZING,
        LOADING,
        ACTIVE,
        DISCONNECTING,
        DISCONNECTED
    };

    // Bot session info
    struct BotSessionInfo
    {
        ::std::shared_ptr<BotSession> session;
        ::std::atomic<SessionState> state{SessionState::INITIALIZING};
        ::std::atomic<uint64> lastUpdate{0};
        ::std::atomic<uint32> updateCount{0};
        ::std::atomic<bool> needsUpdate{true};
    };

    static BotWorldSessionMgrOptimized& Instance();

    // Initialization and shutdown
    void Initialize();
    void Shutdown();
    bool IsInitialized() const { return _initialized.load(::std::memory_order_acquire); }
    bool IsEnabled() const { return _enabled.load(::std::memory_order_acquire); }
    void SetEnabled(bool enabled) { _enabled.store(enabled, ::std::memory_order_release); }

    // Session management (all thread-safe)
    ::std::shared_ptr<BotSession> CreateBotSession(uint32 accountId);
    bool AddBotSession(ObjectGuid playerGuid, ::std::shared_ptr<BotSession> session);
    bool RemoveBotSession(ObjectGuid playerGuid);
    ::std::shared_ptr<BotSession> GetBotSession(ObjectGuid playerGuid) const;
    bool HasBotSession(ObjectGuid playerGuid) const;

    // Batch operations
    void UpdateAllSessions(uint32 diff);
    void DisconnectAllBots();
    uint32 GetBotCount() const;
    ::std::vector<ObjectGuid> GetAllBotGuids() const;

    // Loading management
    bool StartBotLoading(ObjectGuid playerGuid);
    bool FinishBotLoading(ObjectGuid playerGuid);
    bool IsBotLoading(ObjectGuid playerGuid) const;
    uint32 GetLoadingCount() const { return _stats.loadingSessions.load(::std::memory_order_relaxed); }

    // Statistics
    SessionStatistics GetStatistics() const;
    void ResetStatistics();

    // Performance tuning
    void SetMaxSessionsPerUpdate(uint32 max) { _maxSessionsPerUpdate.store(max); }
    uint32 GetMaxSessionsPerUpdate() const { return _maxSessionsPerUpdate.load(); }
    void SetUpdateBatchSize(uint32 size) { _updateBatchSize.store(size); }
    uint32 GetUpdateBatchSize() const { return _updateBatchSize.load(); }

private:
    BotWorldSessionMgrOptimized();
    ~BotWorldSessionMgrOptimized();

    // Internal update methods
    void UpdateSessionBatch(::std::vector<BotSessionInfo*> const& batch, uint32 diff);
    void ProcessDisconnectedSessions();
    void CleanupExpiredSessions();

    // Lock-free concurrent data structures
    using SessionMap = tbb::concurrent_hash_map<ObjectGuid, BotSessionInfo>;
    using LoadingSet = tbb::concurrent_hash_map<ObjectGuid, ::std::chrono::steady_clock::time_point>;
    using DisconnectQueue = tbb::concurrent_vector<ObjectGuid>;

    // Primary session storage (optimized for 5000+ sessions)
    ::std::unique_ptr<SessionMap> _botSessions;

    // Loading tracking (temporary state)
    LoadingSet _botsLoading;

    // Disconnect queue for deferred cleanup
    DisconnectQueue _pendingDisconnects;

    // State flags (all atomic)
    ::std::atomic<bool> _initialized{false};
    ::std::atomic<bool> _enabled{true};
    ::std::atomic<bool> _updating{false};

    // Statistics
    mutable SessionStatistics _stats;

    // Performance tuning
    ::std::atomic<uint32> _maxSessionsPerUpdate{100};
    ::std::atomic<uint32> _updateBatchSize{10};
    ::std::atomic<uint32> _parallelUpdateThreads{4};

    // Timing
    ::std::atomic<uint64> _lastUpdateTime{0};
    ::std::atomic<uint64> _lastCleanupTime{0};

    // Constants
    static constexpr uint32 CLEANUP_INTERVAL_MS = 10000;
    static constexpr uint32 MAX_LOADING_TIME_MS = 30000;
    static constexpr uint32 SESSION_TIMEOUT_MS = 60000;

    // Session pool for memory efficiency
    class SessionPool
    {
    public:
        ::std::shared_ptr<BotSession> Acquire(uint32 accountId);
        void Release(::std::shared_ptr<BotSession> session);

    private:
        tbb::concurrent_queue<::std::shared_ptr<BotSession>> _pool;
        ::std::atomic<uint32> _poolSize{0};
        static constexpr uint32 MAX_POOL_SIZE = 100;
    };

    SessionPool _sessionPool;

    // Deleted operations
    BotWorldSessionMgrOptimized(BotWorldSessionMgrOptimized const&) = delete;
    BotWorldSessionMgrOptimized& operator=(BotWorldSessionMgrOptimized const&) = delete;
};

// Global accessor
#define sBotWorldSessionMgrOptimized BotWorldSessionMgrOptimized::Instance()

} // namespace Playerbot

#endif // MODULE_PLAYERBOT_BOT_WORLD_SESSION_MGR_OPTIMIZED_H