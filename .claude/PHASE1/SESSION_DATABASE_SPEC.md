# BotDatabasePool Complete Implementation Specification

## Overview
Isolated, high-performance database layer with async operations, connection pooling, result caching, and complete separation from TrinityCore's database systems.

## Architecture Requirements

### Database Usage Rules (CORRECTED)
- ✅ **USE auth database** - For bot account creation via `AccountMgr::CreateAccount()`
- ✅ **USE characters database** - For bot character creation via standard `Player` class
- ✅ **Bot accounts** stored in `auth.account` (standard Trinity accounts)
- ✅ **Bot characters** stored in `characters.characters` (standard Trinity characters)
- ✅ **Playerbot database** - ONLY for bot-specific metadata (AI state, settings)
- ❌ **NEVER use WorldDatabase directly** - Use Trinity's existing world data systems

## Class Declaration

### Header File: `src/modules/Playerbot/Database/BotDatabasePool.h`

```cpp
/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 */

#ifndef BOT_DATABASE_POOL_H
#define BOT_DATABASE_POOL_H

#include "DatabaseEnvFwd.h"
#include "MySQLConnection.h"
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

/**
 * CRITICAL IMPLEMENTATION REQUIREMENTS:
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

    // === INITIALIZATION ===

    // Initialize with connection parameters and thread counts
    bool Initialize(std::string const& connectionString,
                   uint8 asyncThreads = 4,
                   uint8 syncThreads = 2);
    void Shutdown();

    // === ASYNC QUERY OPERATIONS ===

    // Async query execution with callback
    void ExecuteAsync(PreparedStatement* stmt,
                     std::function<void(PreparedQueryResult)> callback,
                     uint32 timeoutMs = 30000);

    // Async batch operations
    void ExecuteBatchAsync(std::vector<PreparedStatement*> const& statements,
                          std::function<void(std::vector<PreparedQueryResult>)> callback,
                          uint32 timeoutMs = 30000);

    // Fire-and-forget async execution (no result needed)
    void ExecuteAsyncNoResult(PreparedStatement* stmt);

    // === SYNC QUERY OPERATIONS ===

    // Synchronous query execution (ONLY for bot threads, never main thread)
    PreparedQueryResult Query(PreparedStatement* stmt, uint32 timeoutMs = 30000);

    // Synchronous batch execution
    std::vector<PreparedQueryResult> QueryBatch(std::vector<PreparedStatement*> const& statements,
                                                uint32 timeoutMs = 30000);

    // === CONNECTION MANAGEMENT ===

    // Connection recycling and health monitoring
    void RecycleConnections();
    void ValidateConnections();

    // Connection pool statistics
    struct ConnectionStats {
        std::atomic<uint32> totalConnections{0};
        std::atomic<uint32> activeConnections{0};
        std::atomic<uint32> idleConnections{0};
        std::atomic<uint32> failedConnections{0};
        std::atomic<uint64> totalQueries{0};
        std::atomic<uint64> successfulQueries{0};
        std::atomic<uint64> failedQueries{0};
        std::atomic<uint32> avgQueryTimeMs{0};
    };

    ConnectionStats const& GetConnectionStats() const { return _connectionStats; }

    // === RESULT CACHING SYSTEM ===

    // Cache query result with TTL
    void CacheResult(uint32 queryHash, PreparedQueryResult const& result, uint32 ttlSeconds = 60);

    // Retrieve cached result
    PreparedQueryResult GetCachedResult(uint32 queryHash);

    // Cache management
    void ClearCache();
    void ClearExpiredCache();

    // Cache statistics
    struct CacheStats {
        std::atomic<uint64> totalRequests{0};
        std::atomic<uint64> cacheHits{0};
        std::atomic<uint64> cacheMisses{0};
        std::atomic<uint32> cacheSize{0};
        std::atomic<uint64> totalMemoryBytes{0};

        double GetHitRate() const {
            uint64 total = totalRequests.load();
            return total > 0 ? (static_cast<double>(cacheHits.load()) / total) * 100.0 : 0.0;
        }
    };

    CacheStats const& GetCacheStats() const { return _cacheStats; }

    // === PREPARED STATEMENT MANAGEMENT ===

    // Get prepared statement by ID (with caching)
    PreparedStatement* GetPreparedStatement(uint32 statementId);

    // Create and cache prepared statement
    PreparedStatement* CreatePreparedStatement(uint32 statementId, std::string const& sql);

    // === PERFORMANCE MONITORING ===

    // Performance metrics
    struct PerformanceMetrics {
        std::atomic<uint32> queriesPerSecond{0};
        std::atomic<uint32> avgResponseTimeMs{0};
        std::atomic<uint32> maxResponseTimeMs{0};
        std::atomic<float> cpuUsagePercent{0.0f};
        std::atomic<uint64> totalBytesTransferred{0};
        std::atomic<uint32> connectionPoolUtilization{0};
    };

    PerformanceMetrics const& GetPerformanceMetrics() const { return _performanceMetrics; }

    // Health status
    enum class HealthStatus {
        HEALTHY,
        WARNING,
        CRITICAL,
        OFFLINE
    };

    HealthStatus GetHealthStatus() const;
    std::string GetHealthReport() const;

private:
    BotDatabasePool() = default;
    ~BotDatabasePool() = default;

    // Prevent copying
    BotDatabasePool(BotDatabasePool const&) = delete;
    BotDatabasePool& operator=(BotDatabasePool const&) = delete;

    // === CONNECTION POOL ===

    struct ConnectionInfo {
        std::unique_ptr<MySQLConnection> connection;
        std::atomic<bool> inUse{false};
        std::atomic<uint32> queryCount{0};
        std::chrono::steady_clock::time_point lastUsed;
        std::chrono::steady_clock::time_point created;
        std::atomic<bool> isHealthy{true};

        bool IsExpired(std::chrono::minutes maxAge = std::chrono::minutes(60)) const {
            auto now = std::chrono::steady_clock::now();
            return (now - created) > maxAge;
        }
    };

    static constexpr uint8 MAX_CONNECTIONS = 20;
    std::array<ConnectionInfo, MAX_CONNECTIONS> _connections;
    std::atomic<uint8> _connectionCount{0};
    mutable std::shared_mutex _connectionMutex;

    // === ASYNC OPERATION SYSTEM ===

    struct AsyncQuery {
        std::unique_ptr<PreparedStatement> statement;
        std::function<void(PreparedQueryResult)> callback;
        std::chrono::steady_clock::time_point submitted;
        uint32 timeoutMs;
        uint32 queryId;

        bool IsExpired() const {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - submitted);
            return elapsed.count() >= timeoutMs;
        }
    };

    // Boost.Asio async processing
    std::unique_ptr<boost::asio::io_context> _ioContext;
    std::unique_ptr<boost::asio::io_context::work> _work;
    std::vector<std::thread> _asyncWorkers;

    // Lock-free queue for async operations
    boost::lockfree::queue<AsyncQuery*> _asyncQueue{1024};
    std::atomic<uint32> _nextQueryId{1};

    // === RESULT CACHING ===

    struct CachedResult {
        PreparedQueryResult result;
        std::chrono::steady_clock::time_point expiry;
        uint32 accessCount{0};
        std::chrono::steady_clock::time_point lastAccessed;
        size_t memorySize;

        bool IsExpired() const {
            return std::chrono::steady_clock::now() > expiry;
        }
    };

    // Parallel hashmap for thread-safe cache access
    phmap::parallel_flat_hash_map<uint32, CachedResult> _resultCache;
    mutable std::shared_mutex _cacheMutex;

    static constexpr size_t MAX_CACHE_SIZE = 10000;
    static constexpr size_t MAX_CACHE_MEMORY_MB = 100;

    // === PREPARED STATEMENT CACHE ===

    phmap::parallel_flat_hash_map<uint32, std::unique_ptr<PreparedStatement>> _preparedStatements;
    mutable std::shared_mutex _statementMutex;

    // === CONFIGURATION ===

    struct DatabaseConfig {
        std::string host;
        uint16 port;
        std::string username;
        std::string password;
        std::string database;
        uint32 connectionTimeoutMs{30000};
        uint32 queryTimeoutMs{30000};
        bool enableSSL{false};
        uint32 maxRetries{3};
    } _config;

    // === STATISTICS AND MONITORING ===

    mutable ConnectionStats _connectionStats;
    mutable CacheStats _cacheStats;
    mutable PerformanceMetrics _performanceMetrics;

    // Performance tracking
    std::chrono::steady_clock::time_point _lastStatsUpdate;
    std::atomic<uint64> _totalQueryTime{0};
    std::atomic<uint32> _queryCounter{0};

    // === INTERNAL METHODS ===

    // Connection management
    MySQLConnection* AcquireConnection();
    void ReleaseConnection(MySQLConnection* connection);
    bool CreateConnection(ConnectionInfo& connInfo);
    void CloseConnection(ConnectionInfo& connInfo);

    // Async operation processing
    void AsyncWorker();
    void ProcessAsyncQuery(AsyncQuery* query);
    void HandleQueryTimeout(uint32 queryId);

    // Cache management
    void EvictLRUCacheEntries();
    uint32 CalculateQueryHash(PreparedStatement const* stmt) const;
    size_t CalculateResultMemorySize(PreparedQueryResult const& result) const;

    // Health monitoring
    void UpdateConnectionHealth();
    void UpdatePerformanceMetrics();
    void LogPerformanceWarnings();

    // Configuration parsing
    bool ParseConnectionString(std::string const& connectionString);
    void ValidateConfiguration() const;

    // Utility methods
    inline bool ShouldUpdateStats() const;
    inline void RecordQueryTime(uint32 timeMs);
    inline void RecordCacheHit();
    inline void RecordCacheMiss();
};

// Global accessor
#define sBotDBPool BotDatabasePool::instance()

#endif // BOT_DATABASE_POOL_H
```

## Implementation Requirements

### Dependency Integration
```cmake
# Required in CMakeLists.txt
find_package(Boost REQUIRED COMPONENTS system)
find_package(phmap REQUIRED)
find_package(MySQL REQUIRED)

target_link_libraries(playerbot-database
    PRIVATE
        Boost::system
        phmap::phmap
        ${MYSQL_LIBRARIES})

target_include_directories(playerbot-database
    PRIVATE
        ${MYSQL_INCLUDE_DIRS})
```

### Initialization Implementation
```cpp
bool BotDatabasePool::Initialize(std::string const& connectionString,
                                uint8 asyncThreads,
                                uint8 syncThreads)
{
    TC_LOG_INFO("module.playerbot.database",
        "Initializing isolated BotDatabasePool with {} async + {} sync threads",
        asyncThreads, syncThreads);

    // Parse connection string
    if (!ParseConnectionString(connectionString)) {
        TC_LOG_ERROR("module.playerbot.database",
            "Failed to parse connection string");
        return false;
    }

    // Validate configuration
    ValidateConfiguration();

    // Initialize Boost.Asio context
    _ioContext = std::make_unique<boost::asio::io_context>();
    _work = std::make_unique<boost::asio::io_context::work>(*_ioContext);

    // Create initial connection pool
    uint8 totalConnections = asyncThreads + syncThreads;
    for (uint8 i = 0; i < totalConnections && i < MAX_CONNECTIONS; ++i) {
        if (!CreateConnection(_connections[i])) {
            TC_LOG_ERROR("module.playerbot.database",
                "Failed to create initial connection {}", i);
            return false;
        }
        _connectionCount.fetch_add(1);
    }

    // Start async worker threads
    _asyncWorkers.reserve(asyncThreads);
    for (uint8 i = 0; i < asyncThreads; ++i) {
        _asyncWorkers.emplace_back([this]() {
            AsyncWorker();
        });
    }

    // Initialize performance tracking
    _lastStatsUpdate = std::chrono::steady_clock::now();

    TC_LOG_INFO("module.playerbot.database",
        "BotDatabasePool initialized successfully with {} connections",
        _connectionCount.load());

    return true;
}
```

### Async Query Implementation
```cpp
void BotDatabasePool::ExecuteAsync(PreparedStatement* stmt,
                                  std::function<void(PreparedQueryResult)> callback,
                                  uint32 timeoutMs)
{
    if (!stmt || !callback) {
        TC_LOG_ERROR("module.playerbot.database",
            "Invalid parameters for async query execution");
        return;
    }

    // Create async query structure
    auto asyncQuery = std::make_unique<AsyncQuery>();
    asyncQuery->statement = std::unique_ptr<PreparedStatement>(stmt);
    asyncQuery->callback = std::move(callback);
    asyncQuery->submitted = std::chrono::steady_clock::now();
    asyncQuery->timeoutMs = timeoutMs;
    asyncQuery->queryId = _nextQueryId.fetch_add(1);

    // Check cache first
    uint32 queryHash = CalculateQueryHash(stmt);
    PreparedQueryResult cachedResult = GetCachedResult(queryHash);

    if (cachedResult) {
        // Cache hit - execute callback immediately
        RecordCacheHit();

        // Post callback to IO context to maintain async behavior
        _ioContext->post([callback = asyncQuery->callback, result = cachedResult]() {
            callback(result);
        });

        return;
    }

    RecordCacheMiss();

    // Add to async queue
    if (!_asyncQueue.push(asyncQuery.release())) {
        TC_LOG_WARN("module.playerbot.database",
            "Async query queue full, query dropped");
        return;
    }

    _connectionStats.totalQueries.fetch_add(1);
}
```

### Connection Management Implementation
```cpp
MySQLConnection* BotDatabasePool::AcquireConnection()
{
    std::shared_lock<std::shared_mutex> lock(_connectionMutex);

    // Find available connection
    for (auto& connInfo : _connections) {
        if (connInfo.connection && connInfo.isHealthy.load()) {
            bool expected = false;
            if (connInfo.inUse.compare_exchange_strong(expected, true)) {
                connInfo.lastUsed = std::chrono::steady_clock::now();
                connInfo.queryCount.fetch_add(1);
                _connectionStats.activeConnections.fetch_add(1);
                return connInfo.connection.get();
            }
        }
    }

    // No available connections
    TC_LOG_WARN("module.playerbot.database",
        "No available database connections in pool");
    return nullptr;
}

void BotDatabasePool::ReleaseConnection(MySQLConnection* connection)
{
    if (!connection) return;

    std::shared_lock<std::shared_mutex> lock(_connectionMutex);

    // Find and release the connection
    for (auto& connInfo : _connections) {
        if (connInfo.connection.get() == connection) {
            connInfo.inUse.store(false);
            _connectionStats.activeConnections.fetch_sub(1);
            break;
        }
    }
}

void BotDatabasePool::RecycleConnections()
{
    std::unique_lock<std::shared_mutex> lock(_connectionMutex);

    TC_LOG_DEBUG("module.playerbot.database",
        "Starting connection recycling process");

    uint32 recycledCount = 0;
    auto now = std::chrono::steady_clock::now();

    for (auto& connInfo : _connections) {
        if (!connInfo.connection) continue;

        // Check if connection should be recycled
        bool shouldRecycle = false;

        // Recycle expired connections
        if (connInfo.IsExpired()) {
            shouldRecycle = true;
        }

        // Recycle heavily used connections
        if (connInfo.queryCount.load() > 10000) {
            shouldRecycle = true;
        }

        // Recycle unhealthy connections
        if (!connInfo.isHealthy.load()) {
            shouldRecycle = true;
        }

        if (shouldRecycle && !connInfo.inUse.load()) {
            CloseConnection(connInfo);

            if (CreateConnection(connInfo)) {
                recycledCount++;
            }
        }
    }

    TC_LOG_DEBUG("module.playerbot.database",
        "Connection recycling completed, recycled {} connections",
        recycledCount);
}
```

### Result Caching Implementation
```cpp
void BotDatabasePool::CacheResult(uint32 queryHash,
                                 PreparedQueryResult const& result,
                                 uint32 ttlSeconds)
{
    if (!result) return;

    std::unique_lock<std::shared_mutex> lock(_cacheMutex);

    // Check cache size limits
    if (_resultCache.size() >= MAX_CACHE_SIZE) {
        EvictLRUCacheEntries();
    }

    // Calculate memory usage
    size_t resultMemorySize = CalculateResultMemorySize(result);

    // Check memory limits
    if (_cacheStats.totalMemoryBytes.load() + resultMemorySize > MAX_CACHE_MEMORY_MB * 1024 * 1024) {
        EvictLRUCacheEntries();
    }

    // Create cache entry
    CachedResult cachedResult;
    cachedResult.result = result;
    cachedResult.expiry = std::chrono::steady_clock::now() + std::chrono::seconds(ttlSeconds);
    cachedResult.lastAccessed = std::chrono::steady_clock::now();
    cachedResult.memorySize = resultMemorySize;

    // Insert into cache
    _resultCache[queryHash] = std::move(cachedResult);

    // Update statistics
    _cacheStats.cacheSize.store(_resultCache.size());
    _cacheStats.totalMemoryBytes.fetch_add(resultMemorySize);
}

PreparedQueryResult BotDatabasePool::GetCachedResult(uint32 queryHash)
{
    std::shared_lock<std::shared_mutex> lock(_cacheMutex);

    auto it = _resultCache.find(queryHash);
    if (it != _resultCache.end()) {
        auto& cachedResult = it->second;

        // Check expiration
        if (!cachedResult.IsExpired()) {
            // Update access statistics
            cachedResult.accessCount++;
            cachedResult.lastAccessed = std::chrono::steady_clock::now();

            return cachedResult.result;
        }
        else {
            // Remove expired entry
            lock.unlock();
            std::unique_lock<std::shared_mutex> writeLock(_cacheMutex);
            _resultCache.erase(it);
        }
    }

    return PreparedQueryResult();
}

uint32 BotDatabasePool::CalculateQueryHash(PreparedStatement const* stmt) const
{
    if (!stmt) return 0;

    // Simple hash based on statement ID and parameter values
    uint32 hash = stmt->GetIndex();

    // Add parameter data to hash
    for (uint8 i = 0; i < stmt->GetParameterCount(); ++i) {
        // Hash each parameter value
        // This is a simplified hash - in production would use proper hashing
        hash = hash * 31 + static_cast<uint32>(stmt->GetParameter(i).GetData().size());
    }

    return hash;
}
```

## Performance Requirements Validation

### Database Performance Tests
```cpp
// MANDATORY: Query response time must be < 10ms (P95)
TEST(BotDatabasePoolPerformanceTest, QueryResponseTime) {
    auto* pool = sBotDBPool;

    constexpr size_t QUERY_COUNT = 1000;
    std::vector<uint32> responseTimes;
    responseTimes.reserve(QUERY_COUNT);

    for (size_t i = 0; i < QUERY_COUNT; ++i) {
        auto startTime = std::chrono::high_resolution_clock::now();

        PreparedStatement* stmt = pool->GetPreparedStatement(BOT_SEL_ACCOUNT_INFO);
        stmt->SetData(0, 10000 + i);

        auto result = pool->Query(stmt);

        auto endTime = std::chrono::high_resolution_clock::now();
        uint32 responseTimeMs = static_cast<uint32>(
            std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count());

        responseTimes.push_back(responseTimeMs);
    }

    // Calculate P95
    std::sort(responseTimes.begin(), responseTimes.end());
    uint32 p95ResponseTime = responseTimes[static_cast<size_t>(QUERY_COUNT * 0.95)];

    // REQUIREMENT: P95 < 10ms
    EXPECT_LT(p95ResponseTime, 10);
}

// MANDATORY: Cache hit rate must be > 80%
TEST(BotDatabasePoolPerformanceTest, CacheHitRate) {
    auto* pool = sBotDBPool;

    // Execute same queries multiple times
    constexpr uint32 ACCOUNT_ID = 10000;
    constexpr size_t REPEAT_COUNT = 100;

    for (size_t i = 0; i < REPEAT_COUNT; ++i) {
        PreparedStatement* stmt = pool->GetPreparedStatement(BOT_SEL_ACCOUNT_INFO);
        stmt->SetData(0, ACCOUNT_ID);

        auto result = pool->Query(stmt);
    }

    auto cacheStats = pool->GetCacheStats();
    double hitRate = cacheStats.GetHitRate();

    // REQUIREMENT: > 80% hit rate
    EXPECT_GT(hitRate, 80.0);
}
```

### Async Operation Tests
```cpp
// MANDATORY: Async operations must not block main thread
TEST(BotDatabasePoolAsyncTest, NonBlockingAsync) {
    auto* pool = sBotDBPool;

    std::atomic<bool> callbackExecuted{false};
    std::atomic<uint32> queryCount{0};

    auto startTime = std::chrono::high_resolution_clock::now();

    // Submit 100 async queries
    for (uint32 i = 0; i < 100; ++i) {
        PreparedStatement* stmt = pool->GetPreparedStatement(BOT_SEL_ACCOUNT_INFO);
        stmt->SetData(0, 10000 + i);

        pool->ExecuteAsync(stmt, [&callbackExecuted, &queryCount](PreparedQueryResult result) {
            queryCount.fetch_add(1);
            if (queryCount.load() == 100) {
                callbackExecuted.store(true);
            }
        });
    }

    auto submitTime = std::chrono::high_resolution_clock::now();
    auto submitDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
        submitTime - startTime);

    // REQUIREMENT: Submitting 100 async queries should take < 10ms
    EXPECT_LT(submitDuration.count(), 10);

    // Wait for all callbacks to complete
    while (!callbackExecuted.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    EXPECT_EQ(queryCount.load(), 100);
}
```

## Integration with BotSession

### Query Execution in BotSession
```cpp
void BotSession::ExecuteBotQuery(PreparedStatement* stmt)
{
    // Use isolated database pool - NEVER Trinity's databases
    sBotDBPool->ExecuteAsync(stmt, [this](PreparedQueryResult result) {
        // Process result in bot context
        ProcessQueryResult(result);
    });
}

void BotSession::LoadBotData()
{
    // Load bot-specific data from isolated database
    PreparedStatement* stmt = sBotDBPool->GetPreparedStatement(BOT_SEL_SESSION_DATA);
    stmt->SetData(0, GetAccountId());

    auto result = sBotDBPool->Query(stmt);
    if (result) {
        // Process bot session data
        ProcessSessionData(result);
    }
}
```

### Configuration Integration
```cpp
// In playerbots.conf
Playerbot.Database.Host = "127.0.0.1"
Playerbot.Database.Port = 3306
Playerbot.Database.Username = "playerbot"
Playerbot.Database.Password = "playerbot_password"
Playerbot.Database.Name = "playerbot"
Playerbot.Database.AsyncThreads = 4
Playerbot.Database.SyncThreads = 2
Playerbot.Database.ConnectionTimeout = 30
Playerbot.Database.QueryTimeout = 30
Playerbot.Database.CacheTTL = 60
Playerbot.Database.MaxCacheSize = 10000
Playerbot.Database.MaxCacheMemoryMB = 100
```

**COMPLETE DATABASE ISOLATION. ENTERPRISE-GRADE ASYNC OPERATIONS. ALL DEPENDENCIES REQUIRED.**