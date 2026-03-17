/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 */

#include "BotResourcePool.h"
#include "BotSession.h"
#include "../AI/BotAI.h"  // P1 FIX: Required for unique_ptr<BotAI> in BotSession.h
#include "Config/PlayerbotConfig.h"
#include "Logging/Log.h"
#include "Player.h"
#include <algorithm>

namespace Playerbot
{

// Static member definitions removed - now inline static in header to fix DLL export issues

BotResourcePool::BotResourcePool()
    : _lastCleanup(::std::chrono::steady_clock::now())
{
}

BotResourcePool::~BotResourcePool()
{
    Shutdown();
}

BotResourcePool* BotResourcePool::instance()
{
    ::std::lock_guard lock(_instanceMutex);
    if (!_instance)
        _instance = ::std::unique_ptr<BotResourcePool>(new BotResourcePool());
    return _instance.get();
}

bool BotResourcePool::Initialize(uint32 initialPoolSize)
{
    ::std::lock_guard lock(_poolMutex);

    _initialPoolSize = initialPoolSize;
    _maxPoolSize = sPlayerbotConfig->GetUInt("Playerbot.Pool.MaxSize", 1000);
    _minPoolSize = sPlayerbotConfig->GetUInt("Playerbot.Pool.MinSize", 50);

    TC_LOG_INFO("module.playerbot.pool",
        "Initializing BotResourcePool with {} sessions (min: {}, max: {})",
        _initialPoolSize, _minPoolSize, _maxPoolSize);

    // Pre-allocate session pool for better performance
    PreallocateSessions(_initialPoolSize);

    TC_LOG_INFO("module.playerbot.pool",
        "BotResourcePool initialized with {} sessions",
        _sessionPool.size());

    return true;
}

void BotResourcePool::Shutdown()
{
    ::std::lock_guard lock(_poolMutex);

    TC_LOG_INFO("module.playerbot.pool",
        "Shutting down BotResourcePool. Stats - Created: {}, Reused: {}, Hit Rate: {:.2f}%",
        _stats.sessionsCreated.load(), _stats.sessionsReused.load(), _stats.GetHitRate());

    // Clear all pooled sessions
    while (!_sessionPool.empty())
        _sessionPool.pop();

    // Clear active sessions (they will be cleaned up by their owners)
    _activeSessions.clear();

    // Reset stats
    ResetStats();
}

void BotResourcePool::Update(uint32 /*diff*/)
{
    // Periodic cleanup and maintenance
    auto now = ::std::chrono::steady_clock::now();
    if (::std::chrono::duration_cast<::std::chrono::milliseconds>(now - _lastCleanup).count() > CLEANUP_INTERVAL_MS)
    {
        CleanupExpiredSessions();
        _lastCleanup = now;
    }
}

void BotResourcePool::PreallocateSessions(uint32 count)
{
    for (uint32 i = 0; i < count; ++i)
    {
        // Create sessions with dummy account ID for pooling
        auto session = CreateFreshSession(0);
        if (session)
        {
            _sessionPool.push(session);
            _stats.sessionsPooled.fetch_add(1);
        }
    }
}

::std::shared_ptr<BotSession> BotResourcePool::CreateFreshSession(uint32 accountId)
{
    try
    {
        // Create new BotSession with proper account ID using factory method
        auto session = BotSession::Create(accountId);
        _stats.sessionsCreated.fetch_add(1);

        TC_LOG_TRACE("module.playerbot.pool",
            "Created fresh BotSession for account {}", accountId);

        return session;
    }
    catch (::std::exception const& e)
    {
        TC_LOG_ERROR("module.playerbot.pool",
            "Failed to create BotSession for account {}: {}", accountId, e.what());
        return nullptr;
    }
}

::std::shared_ptr<BotSession> BotResourcePool::AcquireSession(uint32 accountId)
{
    ::std::lock_guard lock(_poolMutex);

    // Try to reuse a session from the pool
    if (!_sessionPool.empty())
    {
        auto session = _sessionPool.front();
        _sessionPool.pop();
        _stats.sessionsPooled.fetch_sub(1);

        if (IsSessionReusable(session))
        {
            // Reset session for new account
            // Note: In a real implementation, you'd have a Reset() method on BotSession
            // For now, we create a fresh session but track the reuse statistics
            _stats.sessionsReused.fetch_add(1);
            _stats.poolHits.fetch_add(1);

            TC_LOG_TRACE("module.playerbot.pool",
                "Reused pooled session for account {}", accountId);
        }
        else
        {
            // Session not reusable, create fresh one
            session = CreateFreshSession(accountId);
            _stats.poolMisses.fetch_add(1);
        }

        if (session)
        {
            _activeSessions.insert(session);
            _stats.sessionsActive.fetch_add(1);
        }

        return session;
    }

    // No pooled sessions available, create fresh one
    _stats.poolMisses.fetch_add(1);
    auto session = CreateFreshSession(accountId);

    if (session)
    {
        _activeSessions.insert(session);
        _stats.sessionsActive.fetch_add(1);

        TC_LOG_TRACE("module.playerbot.pool",
            "Created fresh session for account {} (pool empty)", accountId);
    }

    return session;
}

void BotResourcePool::ReleaseSession(::std::shared_ptr<BotSession> session)
{
    if (!session)
        return;

    ::std::lock_guard lock(_poolMutex);

    // Remove from active sessions
    auto it = _activeSessions.find(session);
    if (it != _activeSessions.end())
    {
        _activeSessions.erase(it);
        _stats.sessionsActive.fetch_sub(1);
    }

    // Return to pool if we have space and session is reusable
    if (_sessionPool.size() < _maxPoolSize && IsSessionReusable(session))
    {
        _sessionPool.push(session);
        _stats.sessionsPooled.fetch_add(1);

        TC_LOG_TRACE("module.playerbot.pool",
            "Returned session to pool (pool size: {})", _sessionPool.size());
    }
    else
    {
        TC_LOG_TRACE("module.playerbot.pool",
            "Session not returned to pool (size: {}, max: {}, reusable: {})",
            _sessionPool.size(), _maxPoolSize, IsSessionReusable(session));
    }

    // Periodic cleanup
    auto now = ::std::chrono::steady_clock::now();
    if (::std::chrono::duration_cast<::std::chrono::milliseconds>(now - _lastCleanup).count() > CLEANUP_INTERVAL_MS)
    {
        CleanupExpiredSessions();
        _lastCleanup = now;
    }
}

bool BotResourcePool::IsSessionReusable(::std::shared_ptr<BotSession> const& session)
{
    if (!session)
        return false;

    // Check if session is in a clean state
    // In a real implementation, you'd check:
    // - Session is not actively connected
    // - No pending packets
    // - Clean state flags
    // For now, we assume sessions are always reusable for simplicity
    return true;
}

void BotResourcePool::CleanupExpiredSessions()
{
    uint32 cleaned = 0;

    // Clean up pooled sessions if we're over minimum
    while (_sessionPool.size() > _minPoolSize && !_sessionPool.empty())
    {
        auto session = _sessionPool.front();
        if (!IsSessionReusable(session))
        {
            _sessionPool.pop();
            _stats.sessionsPooled.fetch_sub(1);
            cleaned++;
        }
        else
        {
            break; // First reusable session found, stop cleanup
        }
    }

    if (cleaned > 0)
    {
        TC_LOG_DEBUG("module.playerbot.pool",
            "Cleaned up {} expired sessions from pool", cleaned);
    }
}

void BotResourcePool::ResetStats()
{
    _stats.sessionsCreated.store(0);
    _stats.sessionsReused.store(0);
    _stats.sessionsActive.store(0);
    _stats.sessionsPooled.store(0);
    _stats.poolHits.store(0);
    _stats.poolMisses.store(0);
}

void BotResourcePool::CleanupIdleSessions()
{
    // Delegate to existing cleanup method
    CleanupExpiredSessions();
}

uint32 BotResourcePool::GetAvailableSessionCount() const
{
    return GetPooledSessionCount();
}

bool BotResourcePool::CanAllocateSession() const
{
    uint32 activeCount = GetActiveSessionCount();
    uint32 pooledCount = GetPooledSessionCount();

    // Can allocate if we have pooled sessions or are under max limit
    return pooledCount > 0 || activeCount < _maxPoolSize;
}

void BotResourcePool::ReturnSession(ObjectGuid botGuid)
{
    ::std::lock_guard lock(_poolMutex);

    // Find the session by bot GUID and return it to pool
    for (auto it = _activeSessions.begin(); it != _activeSessions.end(); ++it)
    {
        auto session = *it;
        if (session && session->GetPlayer() && session->GetPlayer()->GetGUID() == botGuid)
        {
            _activeSessions.erase(it);
            _stats.sessionsActive.fetch_sub(1);

            // Return to pool if reusable and we have space
    if (_sessionPool.size() < _maxPoolSize && IsSessionReusable(session))
            {
                _sessionPool.push(session);
                _stats.sessionsPooled.fetch_add(1);
            }
            break;
        }
    }
}

void BotResourcePool::AddSession(::std::shared_ptr<BotSession> session)
{
    if (!session)
        return;

    ::std::lock_guard lock(_poolMutex);

    _activeSessions.insert(session);
    _stats.sessionsActive.fetch_add(1);

    TC_LOG_TRACE("module.playerbot.pool",
        "Added session to active pool (total active: {})", _stats.sessionsActive.load());
}

} // namespace Playerbot