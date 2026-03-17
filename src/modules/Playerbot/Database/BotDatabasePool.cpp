/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 */

#include "BotDatabasePool.h"
#include "Log.h"
#include "DatabaseEnv.h"
#include "MySQLConnection.h"
#include <algorithm>
#include <sstream>
#include <unordered_set>

namespace Playerbot {

bool BotDatabasePool::Initialize(::std::string const& connectionString, uint8 asyncThreads, uint8 syncThreads)
{
    if (_initialized.load())
    {
        TC_LOG_WARN("module.playerbot.database", "BotDatabasePool already initialized");
        return true;
    }

    TC_LOG_INFO("module.playerbot.database", "Initializing BotDatabasePool...");

    _connectionString = connectionString;
    _asyncThreads = asyncThreads;
    _syncThreads = syncThreads;

    // Initialize Boost.Asio context
    _ioContext = ::std::make_unique<boost::asio::io_context>();
    _workGuard = ::std::make_unique<boost::asio::executor_work_guard<boost::asio::io_context::executor_type>>(
        _ioContext->get_executor());

    // Initialize connection pool
    if (!InitializeConnections())
    {
        TC_LOG_ERROR("module.playerbot.database", "Failed to initialize database connections");
        return false;
    }

    // Start worker threads
    StartWorkerThreads();

    // Initialize timing
    _startTime = ::std::chrono::steady_clock::now();
    _lastMetricsUpdate = _startTime;
    _lastConnectionRecycle = _startTime;

    _initialized.store(true);

    TC_LOG_INFO("module.playerbot.database",
        " BotDatabasePool initialized: {} async + {} sync threads, {} connections",
        _asyncThreads, _syncThreads, _connections.size());

    return true;
}

void BotDatabasePool::Shutdown()
{
    if (!_initialized.load() || _shutdown.load())
    {
        return;
    }

    TC_LOG_INFO("module.playerbot.database", "Shutting down BotDatabasePool...");

    _shutdown.store(true);

    // Stop accepting new queries
    _workGuard.reset();

    // Stop worker threads
    StopWorkerThreads();

    // Shutdown connections
    ShutdownConnections();

    // Log final metrics
    TC_LOG_INFO("module.playerbot.database",
        "Final metrics: {} queries executed, {:.1f}% cache hit rate, {}ms avg response time",
        _metrics.queriesExecuted.load(),
        GetCacheHitRate(),
        _metrics.avgResponseTimeMs.load());

    _initialized.store(false);

    TC_LOG_INFO("module.playerbot.database", " BotDatabasePool shutdown complete");
}

void BotDatabasePool::ExecuteAsync(CharacterDatabasePreparedStatement* stmt,
                                  ::std::function<void(PreparedQueryResult)> callback,
                                  uint32 timeoutMs)
{
    if (!stmt)
    {
        TC_LOG_ERROR("module.playerbot.database", "Cannot execute null statement");
        if (callback) callback(nullptr);
        return;
    }

    if (!_initialized.load() || _shutdown.load())
    {
        TC_LOG_ERROR("module.playerbot.database", "BotDatabasePool not initialized or shutting down");
        if (callback) callback(nullptr);
        return;
    }

    // Check cache first
    ::std::string cacheKey = GenerateCacheKey(stmt);
    PreparedQueryResult cachedResult = GetCachedResult(cacheKey);
    if (cachedResult)
    {
        _metrics.cacheHits.fetch_add(1, ::std::memory_order_relaxed);
        if (callback) callback(cachedResult);
        return;
    }

    _metrics.cacheMisses.fetch_add(1, ::std::memory_order_relaxed);

    // Create query request
    auto request = ::std::make_unique<QueryRequest>();
    request->statement = stmt;
    request->callback = ::std::move(callback);
    request->submitTime = ::std::chrono::steady_clock::now();
    request->timeoutMs = timeoutMs;
    request->requestId = _nextRequestId.fetch_add(1, ::std::memory_order_relaxed);

    // Submit to queue
    if (!_queryQueue.push(request.release()))
    {
        TC_LOG_WARN("module.playerbot.database", "Query queue full, dropping request");
        _metrics.errors.fetch_add(1, ::std::memory_order_relaxed);
        if (request->callback) request->callback(nullptr);
    }
}

void BotDatabasePool::ExecuteAsyncNoResult(CharacterDatabasePreparedStatement* stmt, uint32 timeoutMs)
{
    ExecuteAsync(stmt, nullptr, timeoutMs);
}

void BotDatabasePool::ExecuteBatchAsync(::std::vector<CharacterDatabasePreparedStatement*> const& statements,
                                       ::std::function<void(::std::vector<PreparedQueryResult>)> callback,
                                       uint32 timeoutMs)
{
    if (statements.empty())
    {
        if (callback) callback({});
        return;
    }

    // For batch operations, we'll execute them sequentially in async context
    // A more advanced implementation could use parallel execution
    auto results = ::std::make_shared<::std::vector<PreparedQueryResult>>();
    results->reserve(statements.size());

    auto counter = ::std::make_shared<::std::atomic<size_t>>(0);

    for (auto* stmt : statements)
    {
        ExecuteAsync(stmt, [results, counter, callback, total = statements.size()](PreparedQueryResult result) {
            results->push_back(result);

            if (counter->fetch_add(1, ::std::memory_order_relaxed) + 1 == total) {
                if (callback) callback(*results);
            }
        }, timeoutMs);
    }
}

PreparedQueryResult BotDatabasePool::ExecuteSync(CharacterDatabasePreparedStatement* stmt, uint32 timeoutMs)
{
    if (!stmt)
    {
        TC_LOG_ERROR("module.playerbot.database", "Cannot execute null statement");
        return nullptr;
    }

    if (!_initialized.load() || _shutdown.load())
    {
        TC_LOG_ERROR("module.playerbot.database", "BotDatabasePool not initialized or shutting down");
        return nullptr;
    }

    // Check cache first
    ::std::string cacheKey = GenerateCacheKey(stmt);
    PreparedQueryResult cachedResult = GetCachedResult(cacheKey);
    if (cachedResult)
    {
        _metrics.cacheHits.fetch_add(1, ::std::memory_order_relaxed);
        return cachedResult;
    }

    _metrics.cacheMisses.fetch_add(1, ::std::memory_order_relaxed);

    auto startTime = ::std::chrono::steady_clock::now();

    // Acquire connection
    size_t connectionIndex = AcquireConnection();
    if (connectionIndex == SIZE_MAX) {
        TC_LOG_ERROR("module.playerbot.database", "No available connections for sync query");
        _metrics.errors.fetch_add(1, ::std::memory_order_relaxed);
        return nullptr;
    }

    PreparedQueryResult result = nullptr;

    try {
        auto& connectionInfo = _connections[connectionIndex];
        if (connectionInfo && connectionInfo->connection)
        {
            // Execute query synchronously
            result = connectionInfo->connection->Query(stmt);
            connectionInfo->queryCount++;

            // Cache the result
    if (result)
    {
                CacheResult(cacheKey, result);
            }
        }
    }
    catch (::std::exception const& e)
    {
        TC_LOG_ERROR("module.playerbot.database",
            "Exception during sync query execution: {}", e.what());
        _metrics.errors.fetch_add(1, ::std::memory_order_relaxed);
    }

    // Release connection
    ReleaseConnection(connectionIndex);

    // Record metrics
    RecordQueryExecution(startTime);

    return result;
}

CharacterDatabasePreparedStatement* BotDatabasePool::GetPreparedStatement(uint32 stmtId)
{
    // CRITICAL FIX: All statements should be properly prepared by Trinity's DoPrepareStatements()
    TC_LOG_DEBUG("module.playerbot.database",
        "BotDatabasePool accessing statement {} - ensuring Trinity connection preparation worked", stmtId);

    // CRITICAL FIX: Comprehensive protection against accessing sync-only statements from async connections
    // This prevents assertion failures: "Could not fetch prepared statement X on database playerbot_characters, connection type: asynchronous"

    // List of all CONNECTION_SYNCH statements that must never be accessed from async connections
    // These correspond to Trinity's sync-only prepared statements from CharacterDatabase.cpp
    static const ::std::unordered_set<uint32> SYNC_ONLY_STATEMENTS = {
        39,   // CHAR_SEL_CHECK_GUID
        45,   // CHAR_SEL_BANINFO
        46,   // CHAR_SEL_GUID_BY_NAME_FILTER
        47,   // CHAR_SEL_BANINFO_LIST
        48,   // CHAR_SEL_BANNED_NAME
        49,   // CHAR_SEL_MAIL_LIST_COUNT
        51,   // CHAR_SEL_MAIL_LIST_INFO
        52,   // CHAR_SEL_MAIL_LIST_ITEMS
        87,   // CHAR_SEL_CHAR_ZONE
        88,   // CHAR_SEL_CHAR_POSITION_XYZ
        89,   // CHAR_SEL_CHAR_POSITION
        179,  // CHAR_SEL_AUCTION_ITEMS
        180,  // CHAR_SEL_AUCTIONS
        183,  // CHAR_SEL_AUCTION_BIDDERS
        189,  // CHAR_UPD_AUCTION_EXPIRATION
        196,  // CHAR_SEL_EXPIRED_MAIL
        197,  // CHAR_SEL_EXPIRED_MAIL_ITEMS
        202,  // CHAR_SEL_ITEM_REFUNDS
        203,  // CHAR_SEL_ITEM_BOP_TRADE
        259,  // CHAR_SEL_ACCOUNT_BY_NAME
        260,  // CHAR_UPD_ACCOUNT_BY_GUID
        263,  // CHAR_SEL_MATCH_MAKER_RATING
        287,  // CHAR_SEL_GUILD_BANK_ITEMS
        327,  // CHAR_SEL_CHAR_DATA_FOR_GUILD
        334,  // CHAR_SEL_GUILD_ACHIEVEMENT
        335,  // CHAR_SEL_GUILD_ACHIEVEMENT_CRITERIA
        358,  // CHAR_SEL_GM_SUGGESTIONS (the original problem statement)
        400,  // CHAR_SEL_PETITION
        401,  // CHAR_SEL_PETITION_SIGNATURE
        403,  // CHAR_SEL_PETITION_BY_OWNER
        404,  // CHAR_SEL_PETITION_SIGNATURES
        405,  // CHAR_SEL_PETITION_SIG_BY_ACCOUNT
        406,  // CHAR_SEL_PETITION_OWNER_BY_GUID
        407,  // CHAR_SEL_PETITION_SIG_BY_GUID
        433,  // CHAR_SEL_CORPSES
        437,  // CHAR_SEL_CORPSE_PHASES
        440,  // CHAR_SEL_CORPSE_CUSTOMIZATIONS
        446,  // CHAR_SEL_RESPAWNS
        452,  // CHAR_SEL_GM_BUGS
        458,  // CHAR_SEL_GM_COMPLAINTS
        461,  // CHAR_SEL_GM_COMPLAINT_CHATLINES
        468,  // CHAR_SEL_GM_SUGGESTIONS (duplicate entry for clarity)
        536,  // CHAR_SEL_CHARACTER_AURA_FROZEN
        537,  // CHAR_SEL_CHARACTER_ONLINE
        539,  // CHAR_SEL_CHAR_DEL_INFO_BY_NAME
        540,  // CHAR_SEL_CHAR_DEL_INFO
        541,  // CHAR_SEL_CHARS_BY_ACCOUNT_ID
        542,  // CHAR_SEL_CHAR_PINFO
        543,  // CHAR_SEL_PINFO_BANS
        545,  // CHAR_SEL_PINFO_MAILS
        547,  // CHAR_SEL_PINFO_XP
        548,  // CHAR_SEL_CHAR_HOMEBIND
        549,  // CHAR_SEL_CHAR_GUID_NAME_BY_ACC
        552,  // CHAR_SEL_CHAR_COD_ITEM_MAIL
        553,  // CHAR_SEL_CHAR_SOCIAL
        554,  // CHAR_SEL_CHAR_OLD_CHARS
        557,  // CHAR_SEL_CHAR_INVENTORY_COUNT_ITEM
        558,  // CHAR_SEL_MAIL_COUNT_ITEM
        559,  // CHAR_SEL_AUCTIONHOUSE_COUNT_ITEM
        560,  // CHAR_SEL_GUILD_BANK_COUNT_ITEM
        564,  // CHAR_SEL_CHAR_INVENTORY_ITEM_BY_ENTRY
        567,  // CHAR_SEL_MAIL_ITEM_BY_ENTRY
        568,  // CHAR_SEL_AUCTIONHOUSE_ITEM_BY_ENTRY
        569,  // CHAR_SEL_GUILD_BANK_ITEM_BY_ENTRY
        606,  // CHAR_SEL_CHAR_REP_BY_FACTION
        692,  // CHAR_SEL_ITEMCONTAINER_ITEMS
        696,  // CHAR_SEL_ITEMCONTAINER_MONEY
        707,  // CHAR_SEL_CHAR_PET_IDS
        741,  // CHAR_SEL_PVPSTATS_MAXID
        744,  // CHAR_SEL_PVPSTATS_FACTIONS_OVERALL
        770,  // CHAR_SEL_BLACKMARKET_AUCTIONS
        783   // CHAR_SEL_WAR_MODE_TUNING
    };

    if (SYNC_ONLY_STATEMENTS.find(stmtId) != SYNC_ONLY_STATEMENTS.end()) {
        TC_LOG_ERROR("module.playerbot.database",
            "CRITICAL: Attempted to access sync-only statement {} from BotDatabasePool async context. "
            "This statement must only be accessed from Trinity's main sync connections. "
            "Preventing async/sync mismatch that causes assertion failures.", stmtId);
        return nullptr;
    }

    auto it = _preparedStatements.find(stmtId);
    if (it != _preparedStatements.end()) {
        // TEMPORARY: Still using CharacterDatabase until full isolation is implemented
        // This is the root cause of the sync/async mismatch but now protected for sync-only statements
        return CharacterDatabase.GetPreparedStatement(static_cast<CharacterDatabaseStatements>(stmtId));
    }

    TC_LOG_WARN("module.playerbot.database",
        "Prepared statement {} not found in cache", stmtId);
    return nullptr;
}

void BotDatabasePool::CachePreparedStatement(uint32 stmtId, ::std::string const& sql)
{
    _preparedStatements[stmtId] = sql;
    TC_LOG_DEBUG("module.playerbot.database",
        "Cached prepared statement {}: {}", stmtId, sql);
}

void BotDatabasePool::CacheResult(::std::string const& key, PreparedQueryResult const& result,
                                 ::std::chrono::seconds ttl)
{
    if (!result || key.empty()) return;

    // Check cache size limit
    if (_resultCache.size() >= _maxCacheSize.load()) {
        EvictLeastRecentlyUsed();
    }

    CacheEntry entry;
    entry.result = result;
    entry.expiry = ::std::chrono::steady_clock::now() + ttl;
    entry.lastAccess = ::std::chrono::steady_clock::now();
    entry.accessCount = 1;

    _resultCache[key] = entry;
}

PreparedQueryResult BotDatabasePool::GetCachedResult(::std::string const& key)
{
    auto it = _resultCache.find(key);
    if (it == _resultCache.end()) {
        return nullptr;
    }

    auto& entry = it->second;

    // Check expiry
    if (::std::chrono::steady_clock::now() > entry.expiry)
    {
        _resultCache.erase(it);
        return nullptr;
    }

    // Update access info
    entry.lastAccess = ::std::chrono::steady_clock::now();
    entry.accessCount++;

    return entry.result;
}

double BotDatabasePool::GetCacheHitRate() const
{
    uint64 hits = _metrics.cacheHits.load();
    uint64 misses = _metrics.cacheMisses.load();
    uint64 total = hits + misses;

    return total > 0 ? (static_cast<double>(hits) / total) * 100.0 : 0.0;
}

bool BotDatabasePool::IsHealthy() const
{
    // Consider healthy if:
    // - Average response time < 10ms
    // - Error rate < 1%
    // - At least 50% of connections are available

    uint32 avgResponseTime = _metrics.avgResponseTimeMs.load();
    uint64 totalQueries = _metrics.queriesExecuted.load();
    uint32 errors = _metrics.errors.load();
    uint32 activeConnections = _metrics.activeConnections.load();

    bool responseTimeOk = avgResponseTime < 10;
    bool errorRateOk = totalQueries == 0 || (static_cast<double>(errors) / totalQueries) < 0.01;
    bool connectionsOk = activeConnections < (_connections.size() / 2);

    return responseTimeOk && errorRateOk && connectionsOk;
}

// === PRIVATE IMPLEMENTATION ===

bool BotDatabasePool::InitializeConnections()
{
    uint32 totalConnections = _asyncThreads + _syncThreads;
    _connections.reserve(totalConnections);

    for (uint32 i = 0; i < totalConnections; ++i) {
        auto connectionInfo = ::std::make_unique<ConnectionInfo>();

        try {
            // Create MySQL connection
            // Note: This is a simplified example - actual implementation would
            // need proper MySQL connection setup with the connection string
            connectionInfo->connection = ::std::make_unique<MySQLConnection>(/* connection params */);
            connectionInfo->lastUsed = ::std::chrono::steady_clock::now();

            _connections.push_back(::std::move(connectionInfo));
            _availableConnections.push(i);

            TC_LOG_DEBUG("module.playerbot.database",
                "Initialized database connection {}", i);
        }
        catch (::std::exception const& e)
        {
            TC_LOG_ERROR("module.playerbot.database",
                "Failed to initialize connection {}: {}", i, e.what());
            return false;
        }
    }

    TC_LOG_INFO("module.playerbot.database",
        "Initialized {} database connections", _connections.size());

    return true;
}

void BotDatabasePool::ShutdownConnections()
{
    ::std::unique_lock lock(_connectionMutex);

    for (auto& connectionInfo : _connections)
    {
        if (connectionInfo && connectionInfo->connection)
        {
            // Close connection gracefully
            connectionInfo->connection.reset();
        }
    }

    _connections.clear();

    // Clear available connections queue
    size_t connectionIndex;
    while (_availableConnections.pop(connectionIndex))
    {
        // Just drain the queue
    }
}

size_t BotDatabasePool::AcquireConnection()
{
    size_t connectionIndex;
    if (_availableConnections.pop(connectionIndex))
    {
        if (connectionIndex < _connections.size())
        {
            auto& connectionInfo = _connections[connectionIndex];
            if (connectionInfo)
            {
                connectionInfo->inUse.store(true);
                connectionInfo->lastUsed = ::std::chrono::steady_clock::now();
                _metrics.activeConnections.fetch_add(1, ::std::memory_order_relaxed);
                return connectionIndex;
            }
        }
    }

    return SIZE_MAX; // No available connections
}

void BotDatabasePool::ReleaseConnection(size_t connectionIndex)
{
    if (connectionIndex >= _connections.size()) return;

    auto& connectionInfo = _connections[connectionIndex];
    if (connectionInfo)
    {
        connectionInfo->inUse.store(false);
        _availableConnections.push(connectionIndex);
        _metrics.activeConnections.fetch_sub(1, ::std::memory_order_relaxed);
    }
}

void BotDatabasePool::StartWorkerThreads()
{
    _workers.reserve(_asyncThreads);

    for (uint8 i = 0; i < _asyncThreads; ++i) {
        _workers.emplace_back([this]() {
            WorkerThreadFunction();
        });

        TC_LOG_DEBUG("module.playerbot.database",
            "Started database worker thread {}", i);
    }
}

void BotDatabasePool::StopWorkerThreads()
{
    // Stop the I/O context
    if (_ioContext)
    {
        _ioContext->stop();
    }

    // Wait for all worker threads to finish
    for (auto& worker : _workers)
    {
        if (worker.joinable())
        {
            worker.join();
        }
    }

    _workers.clear();
}

void BotDatabasePool::WorkerThreadFunction()
{
    while (!_shutdown.load())
    {
        try {
            // Process query queue
            ProcessQueryQueue();

            // Run I/O context
    if (_ioContext)
    {
                _ioContext->run_one();
            }

            // Small sleep to prevent busy waiting
            ::std::this_thread::sleep_for(::std::chrono::milliseconds(1));
        }
        catch (::std::exception const& e)
        {
            TC_LOG_ERROR("module.playerbot.database",
                "Exception in worker thread: {}", e.what());
        }
    }
}

void BotDatabasePool::ProcessQueryQueue()
{
    QueryRequest* request = nullptr;
    while (_queryQueue.pop(request))
    {
        if (request)
        {
            ExecuteQueryRequest(*request);
            delete request;
        }
    }
}

void BotDatabasePool::ExecuteQueryRequest(QueryRequest const& request)
{
    auto startTime = ::std::chrono::steady_clock::now();

    // Check timeout
    auto elapsed = ::std::chrono::duration_cast<::std::chrono::milliseconds>(
        startTime - request.submitTime);
    if (elapsed.count() >= request.timeoutMs) {
        HandleQueryTimeout(request);
        return;
    }

    // Acquire connection
    size_t connectionIndex = AcquireConnection();
    if (connectionIndex == SIZE_MAX) {
        TC_LOG_WARN("module.playerbot.database",
            "No available connections for query {}", request.requestId);
        if (request.callback) request.callback(nullptr);
        return;
    }

    PreparedQueryResult result = nullptr;

    try {
        auto& connectionInfo = _connections[connectionIndex];
        if (connectionInfo && connectionInfo->connection)
        {
            // Execute query
            PreparedResultSet* rawResult = connectionInfo->connection->Query(request.statement);
            if (rawResult)
            {
                result = ::std::shared_ptr<PreparedResultSet>(rawResult);
            }
            connectionInfo->queryCount++;

            // Cache the result if successful
    if (result)
    {
                ::std::string cacheKey = GenerateCacheKey(request.statement);
                CacheResult(cacheKey, result);
            }
        }
    }
    catch (::std::exception const& e)
    {
        TC_LOG_ERROR("module.playerbot.database",
            "Exception executing query {}: {}", request.requestId, e.what());
        _metrics.errors.fetch_add(1, ::std::memory_order_relaxed);
    }

    // Release connection
    ReleaseConnection(connectionIndex);

    // Handle result
    HandleQueryResult(request, result);

    // Record metrics
    RecordQueryExecution(startTime);
}

void BotDatabasePool::HandleQueryResult(QueryRequest const& request, PreparedQueryResult result)
{
    if (request.callback)
    {
        try {
            request.callback(result);
        }
        catch (::std::exception const& e)
        {
            TC_LOG_ERROR("module.playerbot.database",
                "Exception in query callback: {}", e.what());
        }
    }
}

void BotDatabasePool::EvictLeastRecentlyUsed()
{
    if (_resultCache.empty()) return;

    // Find least recently used entry
    auto oldestIt = _resultCache.begin();
    auto oldestTime = oldestIt->second.lastAccess;

    for (auto it = _resultCache.begin(); it != _resultCache.end(); ++it) {
        if (it->second.lastAccess < oldestTime)
        {
            oldestTime = it->second.lastAccess;
            oldestIt = it;
        }
    }

    _resultCache.erase(oldestIt);
}

::std::string BotDatabasePool::GenerateCacheKey(CharacterDatabasePreparedStatement const* stmt) const
{
    if (!stmt) return "";

    ::std::ostringstream key;
    key << stmt->GetIndex();

    // Add parameter values to make key unique
    // This is a simplified implementation
    // A full implementation would hash the actual parameter values

    return key.str();
}

void BotDatabasePool::RecordQueryExecution(::std::chrono::steady_clock::time_point startTime)
{
    auto endTime = ::std::chrono::steady_clock::now();
    uint32 responseTimeMs = static_cast<uint32>(
        ::std::chrono::duration_cast<::std::chrono::milliseconds>(endTime - startTime).count());

    _metrics.queriesExecuted.fetch_add(1, ::std::memory_order_relaxed);

    // Update average response time (simplified moving average)
    uint32 currentAvg = _metrics.avgResponseTimeMs.load();
    uint32 newAvg = (currentAvg + responseTimeMs) / 2;
    _metrics.avgResponseTimeMs.store(newAvg, ::std::memory_order_relaxed);

    // Update max response time
    uint32 currentMax = _metrics.maxResponseTimeMs.load();
    if (responseTimeMs > currentMax)
    {
        _metrics.maxResponseTimeMs.store(responseTimeMs, ::std::memory_order_relaxed);
    }

    // Warn if response time exceeds target
    if (responseTimeMs > 10)
    {
        TC_LOG_DEBUG("module.playerbot.database",
            "Query response time {}ms exceeds target 10ms", responseTimeMs);
    }
}

void BotDatabasePool::HandleQueryTimeout(QueryRequest const& request)
{
    TC_LOG_WARN("module.playerbot.database",
        "Query {} timed out after {}ms", request.requestId, request.timeoutMs);

    _metrics.timeouts.fetch_add(1, ::std::memory_order_relaxed);

    if (request.callback)
    {
        request.callback(nullptr);
    }
}

} // namespace Playerbot