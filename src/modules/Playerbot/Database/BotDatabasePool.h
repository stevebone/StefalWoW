/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 */

#ifndef BOT_DATABASE_POOL_H
#define BOT_DATABASE_POOL_H

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include "DatabaseEnvFwd.h"
#include "PreparedStatement.h"
#include "QueryResult.h"
#include <boost/asio.hpp>
#include <boost/lockfree/queue.hpp>
#include <parallel_hashmap/phmap.h>
#include <memory>
#include <array>
#include <atomic>
#include <thread>
#include <chrono>
#include <functional>
#include <shared_mutex>
#include <string>
#include <vector>

// Forward declarations
class MySQLConnection;

namespace Playerbot {

/**
 * CRITICAL IMPLEMENTATION REQUIREMENTS:
 *
 * Implements IBotDatabasePool for dependency injection compatibility.
 *
 * 1. ISOLATION REQUIREMENTS:
 *    - MUST be completely isolated from TrinityCore's DatabaseWorkerPool
 *    - MUST use separate MySQL connections (not shared)
 *    - MUST never interfere with Trinity's database operations
 *    - MUST handle connection failures gracefully
 *
 * 2. PERFORMANCE REQUIREMENTS:
 *    - MUST use boost::asio for async operations
 *    - MUST implement connection recycling every 60 seconds
 *    - MUST cache prepared statements for reuse
 *    - MUST achieve < 10ms query response time (P95)
 *    - MUST achieve > 80% cache hit rate
 *
 * 3. ASYNC OPERATION REQUIREMENTS:
 *    - MUST never block the main thread with database I/O
 *    - MUST use callback-based async query execution
 *    - MUST support batch query operations
 *    - MUST handle query timeouts (30 second default)
 *
 * 4. MEMORY MANAGEMENT REQUIREMENTS:
 *    - MUST implement LRU cache with size limits
 *    - MUST use object pools for query structures
 *    - MUST recycle connections to prevent memory leaks
 *    - MUST monitor and limit total memory usage
 */
class TC_GAME_API BotDatabasePool final 
{
public:
    // Singleton with thread-safe initialization
    static BotDatabasePool* instance()
    {
        static BotDatabasePool instance;
        return &instance;
    }

    // === IBotDatabasePool interface implementation ===

    // Initialize with connection parameters and thread counts
    bool Initialize(::std::string const& connectionString,
                   uint8 asyncThreads = 4,
                   uint8 syncThreads = 2);
    void Shutdown();

    // === ASYNC QUERY OPERATIONS ===

    // Async query execution with callback
    void ExecuteAsync(CharacterDatabasePreparedStatement* stmt,
                     ::std::function<void(PreparedQueryResult)> callback,
                     uint32 timeoutMs = 30000);

    // Fire-and-forget async execution (no result needed)
    void ExecuteAsyncNoResult(CharacterDatabasePreparedStatement* stmt,
                             uint32 timeoutMs = 30000);

    // Async batch operations
    void ExecuteBatchAsync(::std::vector<CharacterDatabasePreparedStatement*> const& statements,
                          ::std::function<void(::std::vector<PreparedQueryResult>)> callback,
                          uint32 timeoutMs = 30000);

    // === SYNCHRONOUS QUERY OPERATIONS ===

    // Synchronous query for immediate results (use sparingly)
    PreparedQueryResult ExecuteSync(CharacterDatabasePreparedStatement* stmt,
                                   uint32 timeoutMs = 10000);

    // === PREPARED STATEMENT MANAGEMENT ===

    // Get prepared statement by ID
    CharacterDatabasePreparedStatement* GetPreparedStatement(uint32 stmtId);

    // Cache prepared statement for reuse
    void CachePreparedStatement(uint32 stmtId, ::std::string const& sql);

    // === CACHING SYSTEM ===

    // Cache query result with TTL
    void CacheResult(::std::string const& key, PreparedQueryResult const& result,
                    ::std::chrono::seconds ttl = ::std::chrono::seconds(60));

    // Get cached result
    PreparedQueryResult GetCachedResult(::std::string const& key);

    // === PERFORMANCE MONITORING ===

    using DatabaseMetrics = IBotDatabasePool::DatabaseMetrics;

    DatabaseMetrics const& GetMetrics() const override { return _metrics; }

    // Performance queries
    double GetCacheHitRate() const;
    uint32 GetAverageResponseTime() const override { return _metrics.avgResponseTimeMs.load(); }
    bool IsHealthy() const;

    // Configuration
    void SetQueryTimeout(uint32 timeoutMs) override { _defaultTimeoutMs = timeoutMs; }
    void SetCacheSize(size_t maxSize) override { _maxCacheSize = maxSize; }
    void SetConnectionRecycleInterval(::std::chrono::seconds interval) override { _recycleInterval = interval; }

private:
    BotDatabasePool() = default;
    ~BotDatabasePool() = default;
    BotDatabasePool(BotDatabasePool const&) = delete;
    BotDatabasePool& operator=(BotDatabasePool const&) = delete;

    // === INTERNAL STRUCTURES ===

    struct ConnectionInfo {
        ::std::unique_ptr<MySQLConnection> connection;
        ::std::chrono::steady_clock::time_point lastUsed;
        ::std::atomic<bool> inUse{false};
        uint32 queryCount{0};
    };

    struct QueryRequest {
        CharacterDatabasePreparedStatement* statement;
        ::std::function<void(PreparedQueryResult)> callback;
        ::std::chrono::steady_clock::time_point submitTime;
        uint32 timeoutMs;
        uint32 requestId;
    };

    struct CacheEntry {
        PreparedQueryResult result;
        ::std::chrono::steady_clock::time_point expiry;
        ::std::chrono::steady_clock::time_point lastAccess;
        uint32 accessCount{0};
    };

    // === CONNECTION MANAGEMENT ===

    // Connection pool
    ::std::vector<::std::unique_ptr<ConnectionInfo>> _connections;
    boost::lockfree::queue<size_t> _availableConnections{16};
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::DATABASE_POOL> _connectionMutex;

    // Connection configuration
    ::std::string _connectionString;
    uint8 _asyncThreads;
    uint8 _syncThreads;

    // === ASYNC OPERATION SYSTEM ===

    // Boost.Asio for async operations
    ::std::unique_ptr<boost::asio::io_context> _ioContext;
    ::std::unique_ptr<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>> _workGuard;
    ::std::vector<::std::thread> _workers;

    // Query queue
    boost::lockfree::queue<QueryRequest*> _queryQueue{1024};
    ::std::atomic<uint32> _nextRequestId{1};

    // === CACHING SYSTEM ===

    // Parallel hashmap for high-performance caching
    // DEADLOCK FIX #18: Changed mutex type from std::shared_mutex to std::recursive_mutex
    // ROOT CAUSE: phmap::parallel_flat_hash_map uses this mutex type internally
    // When bots query database during update, the hashmap's internal std::shared_mutex
    // throws "resource deadlock would occur" on recursive lock attempts
    using CacheMap = phmap::parallel_flat_hash_map<
        ::std::string,
        CacheEntry,
        ::std::hash<::std::string>,
        ::std::equal_to<>,
        ::std::allocator<::std::pair<::std::string, CacheEntry>>,
        4, // 4 submaps for good concurrency
        ::std::recursive_mutex        // CHANGED: ::std::shared_mutex -> ::std::recursive_mutex
    >;

    CacheMap _resultCache;
    ::std::atomic<size_t> _maxCacheSize{10000};

    // === PREPARED STATEMENT CACHE ===

    // DEADLOCK FIX #18: Changed mutex type for prepared statements cache
    using PreparedStatementMap = phmap::parallel_flat_hash_map<
        uint32,
        ::std::string,
        ::std::hash<uint32>,
        ::std::equal_to<>,
        ::std::allocator<::std::pair<uint32, ::std::string>>,
        4,
        ::std::recursive_mutex        // CHANGED: ::std::shared_mutex -> ::std::recursive_mutex
    >;

    PreparedStatementMap _preparedStatements;

    // === METRICS AND MONITORING ===

    mutable DatabaseMetrics _metrics;
    ::std::chrono::steady_clock::time_point _startTime;
    ::std::chrono::steady_clock::time_point _lastMetricsUpdate;

    // === CONFIGURATION ===

    ::std::atomic<bool> _initialized{false};
    ::std::atomic<bool> _shutdown{false};
    ::std::atomic<uint32> _defaultTimeoutMs{30000};
    ::std::chrono::seconds _recycleInterval{60};
    ::std::chrono::steady_clock::time_point _lastConnectionRecycle;

    // === PRIVATE IMPLEMENTATION ===

    // Connection management
    bool InitializeConnections();
    void ShutdownConnections();
    size_t AcquireConnection();
    void ReleaseConnection(size_t connectionIndex);
    void RecycleConnections();

    // Query processing
    void ProcessQueryQueue();
    void ExecuteQueryRequest(QueryRequest const& request);
    void HandleQueryResult(QueryRequest const& request, PreparedQueryResult result);

    // Caching implementation
    void CleanupExpiredCache();
    void EvictLeastRecentlyUsed();
    ::std::string GenerateCacheKey(CharacterDatabasePreparedStatement const* stmt) const;

    // Metrics and monitoring
    void UpdateMetrics();
    void RecordQueryExecution(::std::chrono::steady_clock::time_point startTime);

    // Worker thread management
    void StartWorkerThreads();
    void StopWorkerThreads();
    void WorkerThreadFunction();

    // Error handling
    void HandleConnectionError(size_t connectionIndex);
    void HandleQueryTimeout(QueryRequest const& request);
};

} // namespace Playerbot

// Global access macro for convenience
#define sBotDBPool Playerbot::BotDatabasePool::instance()

#endif // BOT_DATABASE_POOL_H