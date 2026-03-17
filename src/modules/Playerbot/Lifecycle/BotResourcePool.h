/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include "ObjectGuid.h"
#include <memory>
#include <queue>
#include <mutex>
#include <atomic>
#include <unordered_set>
#include <chrono>

namespace Playerbot
{

// Forward declarations
class BotSession;

/**
 * @class BotResourcePool
 * @brief High-performance object pooling for 5000+ concurrent bots
 *
 * PERFORMANCE OPTIMIZATION: Object pooling eliminates memory allocation
 * overhead during bot spawning/despawning operations, critical for
 * 5000 bot scalability target.
 *
 * Features:
 * - Pre-allocated session pools
 * - Lock-free operations where possible
 * - Memory reuse patterns
 * - Automatic pool scaling
 */
class TC_GAME_API BotResourcePool final 
{
public:
    BotResourcePool();
    ~BotResourcePool();

    // Singleton access
    static BotResourcePool* instance();

    // Pool lifecycle
    bool Initialize(uint32 initialPoolSize = 100);
    void Shutdown();
    void Update(uint32 diff);

    // Session pool management
    ::std::shared_ptr<BotSession> AcquireSession(uint32 accountId);
    void ReleaseSession(::std::shared_ptr<BotSession> session);
    void ReturnSession(ObjectGuid botGuid);
    void AddSession(::std::shared_ptr<BotSession> session);

    // Pool statistics for monitoring
    struct PoolStats
    {
        ::std::atomic<uint32> sessionsCreated{0};
        ::std::atomic<uint32> sessionsReused{0};
        ::std::atomic<uint32> sessionsActive{0};
        ::std::atomic<uint32> sessionsPooled{0};
        ::std::atomic<uint32> poolHits{0};
        ::std::atomic<uint32> poolMisses{0};

        float GetHitRate() const
        {
            uint32 hits = poolHits.load();
            uint32 misses = poolMisses.load();
            uint32 total = hits + misses;
            return total > 0 ? static_cast<float>(hits) / total * 100.0f : 0.0f;
        }

        float GetReuseRate() const
        {
            uint32 reused = sessionsReused.load();
            uint32 created = sessionsCreated.load();
            uint32 total = reused + created;
            return total > 0 ? static_cast<float>(reused) / total * 100.0f : 0.0f;
        }
    };

    PoolStats const& GetStats() const { return _stats; }
    void ResetStats();

    // Pool configuration
    void SetMaxPoolSize(uint32 maxSize) { _maxPoolSize = maxSize; }
    void SetMinPoolSize(uint32 minSize) { _minPoolSize = minSize; }

    uint32 GetActiveSessionCount() const { return _stats.sessionsActive.load(); }
    uint32 GetPooledSessionCount() const { return _stats.sessionsPooled.load(); }

    // Additional methods needed by BotSpawnOrchestrator
    void CleanupIdleSessions();
    uint32 GetAvailableSessionCount() const;
    bool CanAllocateSession() const;

private:
    // Session pool management
    void PreallocateSessions(uint32 count);
    ::std::shared_ptr<BotSession> CreateFreshSession(uint32 accountId);
    void CleanupExpiredSessions();
    bool IsSessionReusable(::std::shared_ptr<BotSession> const& session);

    // Pool data
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_SPAWNER> _poolMutex;
    ::std::queue<::std::shared_ptr<BotSession>> _sessionPool;
    ::std::unordered_set<::std::shared_ptr<BotSession>> _activeSessions;

    // Pool configuration
    uint32 _maxPoolSize = 1000;
    uint32 _minPoolSize = 50;
    uint32 _initialPoolSize = 100;

    // Session cleanup tracking
    ::std::chrono::steady_clock::time_point _lastCleanup;
    static constexpr uint32 CLEANUP_INTERVAL_MS = 30000; // 30 seconds

    // Statistics
    mutable PoolStats _stats;

    // Singleton
    inline static ::std::unique_ptr<BotResourcePool> _instance;
    inline static Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_SPAWNER> _instanceMutex;

    // Non-copyable
    BotResourcePool(BotResourcePool const&) = delete;
    BotResourcePool& operator=(BotResourcePool const&) = delete;
};

#define sBotResourcePool BotResourcePool::instance()

} // namespace Playerbot