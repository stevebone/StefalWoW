/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "PlayerbotCharacterDBInterface.h"
#include "DatabaseEnv.h"
#include "CharacterDatabase.h"
#include "QueryHolder.h"
#include "Log.h"
#include "World.h"
#include <algorithm>
#include <sstream>

namespace Playerbot
{

// === PlayerbotCharacterDBInterface Implementation ===

PlayerbotCharacterDBInterface::PlayerbotCharacterDBInterface()
    : _classifier(::std::make_unique<StatementClassifier>()),
      _executionEngine(::std::make_unique<SafeExecutionEngine>())
{
    _startTime = ::std::chrono::steady_clock::now();
    _mainThreadId = ::std::this_thread::get_id();
}

PlayerbotCharacterDBInterface::~PlayerbotCharacterDBInterface()
{
    if (_initialized.load())
    {
        Shutdown();
    }
}

PlayerbotCharacterDBInterface* PlayerbotCharacterDBInterface::instance()
{
    static PlayerbotCharacterDBInterface instance;
    return &instance;
}

bool PlayerbotCharacterDBInterface::Initialize()
{
    if (_initialized.load())
    {
        TC_LOG_WARN("module.playerbot.database", "PlayerbotCharacterDBInterface already initialized");
        return true;
    }

    TC_LOG_INFO("module.playerbot.database", "Initializing PlayerbotCharacterDBInterface...");

    // Initialize statement classification
    InitializeStatementClassification();

    // Initialize classifier
    _classifier->Initialize();

    // Initialize execution engine
    _executionEngine->Initialize();

    // Store main thread ID
    _mainThreadId = ::std::this_thread::get_id();

    _initialized.store(true);

    TC_LOG_INFO("module.playerbot.database",
        "PlayerbotCharacterDBInterface initialized with {} sync-only statements classified",
        _syncOnlyStatements.size());

    return true;
}

void PlayerbotCharacterDBInterface::Shutdown()
{
    if (!_initialized.load() || _shutdown.load())
    {
        return;
    }

    TC_LOG_INFO("module.playerbot.database", "Shutting down PlayerbotCharacterDBInterface...");

    _shutdown.store(true);

    // Process remaining sync queue
    ProcessSyncQueue();

    // Shutdown execution engine
    if (_executionEngine)
    {
        _executionEngine->Shutdown();
    }

    // Log final metrics
    TC_LOG_INFO("module.playerbot.database",
        "Final metrics - Total: {}, Sync: {}, Async: {}, Routed: {}, Errors: {}",
        _metrics.totalQueries.load(),
        _metrics.syncQueries.load(),
        _metrics.asyncQueries.load(),
        _metrics.routedQueries.load(),
        _metrics.errors.load());

    _initialized.store(false);
}

CharacterDatabasePreparedStatement* PlayerbotCharacterDBInterface::GetPreparedStatement(CharacterDatabaseStatements statementId)
{
    // Check if interface is initialized
    if (!_initialized.load())
    {
        TC_LOG_ERROR("module.playerbot.database", "Interface not initialized or shutting down");
        _metrics.errors.fetch_add(1);
        return nullptr;
    }

    // Validate statement ID
    if (statementId >= MAX_CHARACTERDATABASE_STATEMENTS)
    {
        TC_LOG_ERROR("module.playerbot.database",
            "Invalid statement ID {} >= MAX({})",
            static_cast<uint32>(statementId), MAX_CHARACTERDATABASE_STATEMENTS);
        _metrics.errors.fetch_add(1);
        return nullptr;
    }

    // Check if this is a sync-only statement being accessed from async context
    if (IsSyncOnlyStatement(static_cast<uint32>(statementId)) && IsAsyncContext())
    {
        TC_LOG_WARN("module.playerbot.database",
            "CRITICAL: Sync-only statement {} accessed from async context - will route through sync queue",
            static_cast<uint32>(statementId));
        _metrics.routedQueries.fetch_add(1);
    }

    // For now, still get from CharacterDatabase but with proper validation
    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(statementId);

    if (!stmt)
    {
        TC_LOG_ERROR("module.playerbot.database",
            "Failed to get prepared statement {}", static_cast<uint32>(statementId));
        _metrics.errors.fetch_add(1);
        return nullptr;
    }

    return stmt;
}

void PlayerbotCharacterDBInterface::ExecuteAsync(CharacterDatabasePreparedStatement* stmt,
                                                ::std::function<void(PreparedQueryResult)> callback,
                                                uint32 timeoutMs)
{
    (void)timeoutMs; // Parameter reserved for future timeout implementation

    // Check if interface is initialized
    if (!_initialized.load())
    {
        TC_LOG_ERROR("module.playerbot.database", "Interface not initialized or shutting down");
        if (callback) callback(nullptr);
        _metrics.errors.fetch_add(1);
        return;
    }

    if (!stmt)
    {
        TC_LOG_ERROR("module.playerbot.database", "Cannot execute null statement");
        if (callback) callback(nullptr);
        _metrics.errors.fetch_add(1);
        return;
    }

    if (!_initialized.load() || _shutdown.load())
    {
        TC_LOG_ERROR("module.playerbot.database", "Interface not initialized or shutting down");
        if (callback) callback(nullptr);
        _metrics.errors.fetch_add(1);
        return;
    }

    _metrics.totalQueries.fetch_add(1);

    // Check if this statement requires special routing
    uint32 statementId = stmt->GetIndex();

    if (IsSyncOnlyStatement(statementId))
    {
        // This is a sync-only statement - must route through sync queue
        TC_LOG_DEBUG("module.playerbot.database",
            "Routing sync-only statement {} through sync queue", statementId);

        ExecuteSyncFromAsync(stmt, callback);
        _metrics.routedQueries.fetch_add(1);
    }
    else
    {
        // Safe to execute asynchronously
        auto startTime = ::std::chrono::steady_clock::now();

        CharacterDatabase.AsyncQuery(stmt).WithPreparedCallback(
            [this, callback, startTime](PreparedQueryResult result)
            {
                auto duration = ::std::chrono::duration_cast<::std::chrono::milliseconds>(
                    ::std::chrono::steady_clock::now() - startTime).count();

                UpdateMetrics(static_cast<uint32>(duration), false, !result);

                if (callback)
                {
                    callback(result);
                }
            });

        _metrics.asyncQueries.fetch_add(1);
    }
}

PreparedQueryResult PlayerbotCharacterDBInterface::ExecuteSync(CharacterDatabasePreparedStatement* stmt)
{
    if (!stmt)
    {
        TC_LOG_ERROR("module.playerbot.database", "Cannot execute null statement");
        _metrics.errors.fetch_add(1);
        return nullptr;
    }

    if (!_initialized.load() || _shutdown.load())
    {
        TC_LOG_ERROR("module.playerbot.database", "Interface not initialized or shutting down");
        _metrics.errors.fetch_add(1);
        return nullptr;
    }

    _metrics.totalQueries.fetch_add(1);
    _metrics.syncQueries.fetch_add(1);

    auto startTime = ::std::chrono::steady_clock::now();

    PreparedQueryResult result = CharacterDatabase.Query(stmt);

    auto duration = ::std::chrono::duration_cast<::std::chrono::milliseconds>(
        ::std::chrono::steady_clock::now() - startTime).count();

    UpdateMetrics(static_cast<uint32>(duration), true, !result);

    return result;
}

CharacterDatabaseTransaction PlayerbotCharacterDBInterface::BeginTransaction()
{
    return CharacterDatabase.BeginTransaction();
}

void PlayerbotCharacterDBInterface::CommitTransaction(CharacterDatabaseTransaction trans, bool async)
{
    if (!trans)
    {
        TC_LOG_ERROR("module.playerbot.database", "Cannot commit null transaction");
        _metrics.errors.fetch_add(1);
        return;
    }

    _metrics.totalQueries.fetch_add(1);

    // CRITICAL FIX: When in async context (worker thread), we MUST use async commit
    // because DirectCommitTransaction uses synchronous connections which don't have
    // async-only prepared statements loaded (like those in Player::SaveToDB).
    // Only use DirectCommitTransaction when we're on the main thread AND async is explicitly false.
    bool useAsyncCommit = async || IsAsyncContext();

    if (useAsyncCommit)
    {
        CharacterDatabase.CommitTransaction(trans);
        _metrics.asyncQueries.fetch_add(1);
    }
    else
    {
        CharacterDatabase.DirectCommitTransaction(trans);
        _metrics.syncQueries.fetch_add(1);
    }
}

bool PlayerbotCharacterDBInterface::ExecuteDirectSQL(::std::string const& sql)
{
    if (sql.empty())
    {
        TC_LOG_ERROR("module.playerbot.database", "Cannot execute empty SQL");
        return false;
    }

    _metrics.totalQueries.fetch_add(1);

    try
    {
        CharacterDatabase.DirectExecute(sql.c_str());
    }
    catch (...)
    {
        _metrics.errors.fetch_add(1);
        TC_LOG_ERROR("module.playerbot.database", "DirectExecute failed for SQL: {}", sql);
        return false;
    }

    return true;
}

bool PlayerbotCharacterDBInterface::IsAsyncContext() const
{
    ::std::thread::id currentThread = ::std::this_thread::get_id();

    // Check if we're in main thread (no lock needed - _mainThreadId is set once and never changes)
    if (currentThread == _mainThreadId)
    {
        return false;
    }

    // For simplicity and deadlock prevention, assume any non-main thread is async
    // This is safer than maintaining a lock-protected set of async thread IDs
    return true;
}

bool PlayerbotCharacterDBInterface::IsSyncOnlyStatement(uint32 statementId) const
{
    return _syncOnlyStatements.find(statementId) != _syncOnlyStatements.end();
}

bool PlayerbotCharacterDBInterface::RouteQuery(CharacterDatabasePreparedStatement* stmt,
                                              ::std::function<void(PreparedQueryResult)> callback,
                                              bool forceSync)
{
    if (!stmt)
    {
        return false;
    }

    uint32 statementId = stmt->GetIndex();

    // Determine execution path
    bool needsSync = forceSync || IsSyncOnlyStatement(statementId);
    bool inAsyncContext = IsAsyncContext();

    if (needsSync && inAsyncContext)
    {
        // Need to route through sync queue
        ExecuteSyncFromAsync(stmt, callback);
        return true;
    }
    else if (needsSync)
    {
        // Execute synchronously
        PreparedQueryResult result = ExecuteSync(stmt);
        if (callback)
        {
            callback(result);
        }
        return true;
    }
    else
    {
        // Execute asynchronously
        ExecuteAsync(stmt, callback);
        return true;
    }
}

void PlayerbotCharacterDBInterface::ExecuteSyncFromAsync(CharacterDatabasePreparedStatement* stmt,
                                                        ::std::function<void(PreparedQueryResult)> callback)
{
    // Create sync request
    auto request = ::std::make_shared<SyncRequest>();
    request->statement = stmt;
    request->callback = callback;
    request->submitTime = ::std::chrono::steady_clock::now();
    request->timeoutMs = _config.defaultTimeoutMs;

    // Add to sync queue
    {
        ::std::lock_guard lock(_syncQueueMutex);

        if (_syncQueue.size() >= _config.syncQueueMaxSize)
        {
            TC_LOG_ERROR("module.playerbot.database",
                "Sync queue full ({} items), dropping request", _syncQueue.size());
            _metrics.errors.fetch_add(1);

            if (callback)
            {
                callback(nullptr);
            }
            return;
        }

        _syncQueue.push(request);
    }

    _syncQueueCV.notify_one();

    TC_LOG_DEBUG("module.playerbot.database",
        "Queued sync-only statement {} for main thread execution",
        stmt->GetIndex());
}

void PlayerbotCharacterDBInterface::ProcessSyncQueue()
{
    // Process queue in batches to prevent recursive deadlocks
    constexpr uint32 MAX_BATCH_SIZE = 10;

    for (uint32 batch = 0; batch < MAX_BATCH_SIZE; ++batch)
    {
        ::std::shared_ptr<SyncRequest> request = nullptr;

        // Extract one request at a time
        {
            ::std::lock_guard lock(_syncQueueMutex);
            if (_syncQueue.empty())
                break;

            request = _syncQueue.front();
            _syncQueue.pop();
        }

        // Execute outside of any lock to prevent deadlocks
        try
        {
            PreparedQueryResult result = CharacterDatabase.Query(request->statement);

            // Call callback if provided (also outside of locks)
    if (request->callback)
            {
                request->callback(result);
            }

            // Mark as completed
            request->completed = true;

            // Signal completion if waiting
    if (request->completionSignal)
            {
                request->completionSignal->notify_one();
            }
        }
        catch (::std::exception const& e)
        {
            TC_LOG_ERROR("module.playerbot.database", "ProcessSyncQueue failed: {}", e.what());

            // Mark as completed even on error to prevent hanging
            request->completed = true;
            if (request->completionSignal)
            {
                request->completionSignal->notify_one();
            }
        }
    }
}

void PlayerbotCharacterDBInterface::InitializeStatementClassification()
{
    // All CONNECTION_SYNCH statements from CharacterDatabase.cpp
    // These MUST be executed synchronously to avoid assertion failures
    _syncOnlyStatements = {
        // Character data
        31,   // CHAR_SEL_CHARACTER
        43,   // CHAR_SEL_DATA_BY_NAME
        44,   // CHAR_SEL_DATA_BY_GUID
        45,   // CHAR_SEL_CHECK_NAME
        46,   // CHAR_SEL_CHECK_GUID_NAME
        47,   // CHAR_SEL_SUM_CHARS
        48,   // CHAR_SEL_CHAR_CREATE_INFO
        69,   // CHAR_SEL_ENUM
        70,   // CHAR_SEL_ENUM_DECLINED_NAME

        // Auction house
        177,  // CHAR_SEL_AUCTION_ITEMS
        189,  // CHAR_UPD_AUCTION_EXPIRATION

        // Mail system
        196,  // CHAR_SEL_EXPIRED_MAIL
        197,  // CHAR_SEL_EXPIRED_MAIL_ITEMS

        // Items and trading
        202,  // CHAR_SEL_ITEM_REFUNDS
        203,  // CHAR_SEL_ITEM_BOP_TRADE

        // Account management
        259,  // CHAR_SEL_ACCOUNT_BY_NAME
        260,  // CHAR_UPD_ACCOUNT_BY_GUID

        // PvP
        263,  // CHAR_SEL_MATCH_MAKER_RATING

        // Guild
        287,  // CHAR_SEL_GUILD_BANK_ITEMS
        327,  // CHAR_SEL_CHAR_DATA_FOR_GUILD
        334,  // CHAR_SEL_GUILD_ACHIEVEMENT
        335,  // CHAR_SEL_GUILD_ACHIEVEMENT_CRITERIA

        // GM/Support system (CRITICAL - these cause the assertion failures)
        358,  // CHAR_SEL_GM_SUGGESTIONS - The primary culprit
        452,  // CHAR_SEL_GM_BUGS
        458,  // CHAR_SEL_GM_COMPLAINTS
        461,  // CHAR_SEL_GM_COMPLAINT_CHATLINES

        // Petitions
        400,  // CHAR_SEL_PETITION
        401,  // CHAR_SEL_PETITION_SIGNATURE
        403,  // CHAR_SEL_PETITION_BY_OWNER
        404,  // CHAR_SEL_PETITION_SIGNATURES
        405,  // CHAR_SEL_PETITION_SIG_BY_ACCOUNT
        406,  // CHAR_SEL_PETITION_OWNER_BY_GUID
        407,  // CHAR_SEL_PETITION_SIG_BY_GUID

        // World state
        433,  // CHAR_SEL_CORPSES
        437,  // CHAR_SEL_CORPSE_PHASES
        440,  // CHAR_SEL_CORPSE_CUSTOMIZATIONS
        446,  // CHAR_SEL_RESPAWNS

        // Character info queries
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

        // Item searches
        557,  // CHAR_SEL_CHAR_INVENTORY_COUNT_ITEM
        558,  // CHAR_SEL_MAIL_COUNT_ITEM
        559,  // CHAR_SEL_AUCTIONHOUSE_COUNT_ITEM
        560,  // CHAR_SEL_GUILD_BANK_COUNT_ITEM
        564,  // CHAR_SEL_CHAR_INVENTORY_ITEM_BY_ENTRY
        567,  // CHAR_SEL_MAIL_ITEM_BY_ENTRY
        568,  // CHAR_SEL_AUCTIONHOUSE_ITEM_BY_ENTRY
        569,  // CHAR_SEL_GUILD_BANK_ITEM_BY_ENTRY

        // Character reputation
        606,  // CHAR_SEL_CHAR_REP_BY_FACTION

        // Container queries
        692,  // CHAR_SEL_ITEMCONTAINER_ITEMS
        696,  // CHAR_SEL_ITEMCONTAINER_MONEY

        // Pet system
        707,  // CHAR_SEL_CHAR_PET_IDS

        // PvP stats
        741,  // CHAR_SEL_PVPSTATS_MAXID
        744,  // CHAR_SEL_PVPSTATS_FACTIONS_OVERALL

        // Black market
        770,  // CHAR_SEL_BLACKMARKET_AUCTIONS

        // War mode
        783   // CHAR_SEL_WAR_MODE_TUNING
    };

    TC_LOG_INFO("module.playerbot.database",
        "Classified {} sync-only statements for proper routing",
        _syncOnlyStatements.size());
}

ExecutionContext PlayerbotCharacterDBInterface::DetectContext() const
{
    return ExecutionContext::Detect();
}

void PlayerbotCharacterDBInterface::UpdateMetrics(uint32 responseTimeMs, bool isSync, bool hadError)
{
    if (!_config.enableMetrics)
    {
        return;
    }

    // Update average response time (simple moving average)
    uint32 currentAvg = _metrics.avgResponseTimeMs.load();
    uint32 newAvg = (currentAvg * 9 + responseTimeMs) / 10;
    _metrics.avgResponseTimeMs.store(newAvg);

    // Update max response time
    uint32 currentMax = _metrics.maxResponseTimeMs.load();
    if (responseTimeMs > currentMax)
    {
        _metrics.maxResponseTimeMs.store(responseTimeMs);
    }

    // Update error counter
    if (hadError)
    {
        _metrics.errors.fetch_add(1);
    }

    // Log slow queries
    if (responseTimeMs > 100)
    {
        TC_LOG_WARN("module.playerbot.database",
            "Slow query detected: {}ms ({})",
            responseTimeMs, isSync ? "sync" : "async");
    }
}

// === StatementClassifier Implementation ===

StatementClassifier::StatementClassifier()
{
}

StatementClassifier::~StatementClassifier()
{
}

void StatementClassifier::Initialize()
{
    LoadSyncOnlyStatements();
    LoadAsyncSafeStatements();

    TC_LOG_INFO("module.playerbot.database",
        "Statement classifier initialized with {} statements classified",
        _statementTypes.size());
}

StatementClassifier::StatementType StatementClassifier::ClassifyStatement(uint32 statementId) const
{
    auto it = _statementTypes.find(statementId);
    if (it != _statementTypes.end())
    {
        return it->second;
    }
    return UNKNOWN;
}

::std::string StatementClassifier::GetStatementName(uint32 statementId) const
{
    auto it = _statementNames.find(statementId);
    if (it != _statementNames.end())
    {
        return it->second;
    }
    return "UNKNOWN_STATEMENT_" + ::std::to_string(statementId);
}

void StatementClassifier::LoadSyncOnlyStatements()
{
    // Load all CONNECTION_SYNCH statements
    // These are loaded from the main interface's classification

    // GM/Support statements
    _statementTypes[358] = SYNC_ONLY;  // CHAR_SEL_GM_SUGGESTIONS
    _statementNames[358] = "CHAR_SEL_GM_SUGGESTIONS";

    _statementTypes[452] = SYNC_ONLY;  // CHAR_SEL_GM_BUGS
    _statementNames[452] = "CHAR_SEL_GM_BUGS";

    _statementTypes[458] = SYNC_ONLY;  // CHAR_SEL_GM_COMPLAINTS
    _statementNames[458] = "CHAR_SEL_GM_COMPLAINTS";

    // Add more as needed...
}

void StatementClassifier::LoadAsyncSafeStatements()
{
    // Load statements that are safe for async execution
    // These typically have CONNECTION_BOTH or CONNECTION_ASYNC flags

    // Character updates
    _statementTypes[3] = ASYNC_SAFE;   // CHAR_UPD_CHARACTER
    _statementNames[3] = "CHAR_UPD_CHARACTER";

    // Add more as needed...
}

// === ExecutionContext Implementation ===

ExecutionContext::ExecutionContext()
    : _type(UNKNOWN_CONTEXT)
    , _threadId(::std::this_thread::get_id())
{
}

ExecutionContext ExecutionContext::Detect()
{
    ExecutionContext context;
    context._threadId = ::std::this_thread::get_id();

    // Try to determine context type
    // This is a simplified detection - real implementation would check thread pools

    // Check if we're in the main thread
    if (context._threadId == PlayerbotCharacterDBInterface::instance()->GetMainThreadId())
    {
        context._type = MAIN_THREAD;
    }
    else
    {
        // Assume async worker for now
        context._type = ASYNC_WORKER;
    }

    return context;
}

// === SafeExecutionEngine Implementation ===

SafeExecutionEngine::SafeExecutionEngine()
{
}

SafeExecutionEngine::~SafeExecutionEngine()
{
    if (_initialized.load())
    {
        Shutdown();
    }
}

void SafeExecutionEngine::Initialize()
{
    _initialized.store(true);
    TC_LOG_INFO("module.playerbot.database", "Safe execution engine initialized");
}

void SafeExecutionEngine::Shutdown()
{
    _initialized.store(false);
    TC_LOG_INFO("module.playerbot.database",
        "Safe execution engine shutdown - {} total executions",
        _executionCounter.load());
}

PreparedQueryResult SafeExecutionEngine::ExecuteWithSafety(CharacterDatabasePreparedStatement* stmt,
                                                          bool async,
                                                          ::std::function<void(PreparedQueryResult)> callback)
{
    if (!stmt)
    {
        TC_LOG_ERROR("module.playerbot.database", "Cannot execute null statement");
        if (callback) callback(nullptr);
        return nullptr;
    }

    _executionCounter.fetch_add(1);

    auto startTime = ::std::chrono::steady_clock::now();

    try
    {
        if (async)
        {
            CharacterDatabase.AsyncQuery(stmt).WithPreparedCallback(
                [this, callback, startTime](PreparedQueryResult result)
                {
                    auto duration = ::std::chrono::duration_cast<::std::chrono::milliseconds>(
                        ::std::chrono::steady_clock::now() - startTime).count();

                    LogExecution(nullptr, result != nullptr, static_cast<uint32>(duration));

                    if (callback)
                    {
                        callback(result);
                    }
                });

            return nullptr;
        }
        else
        {
            PreparedQueryResult result = CharacterDatabase.Query(stmt);

            auto duration = ::std::chrono::duration_cast<::std::chrono::milliseconds>(
                ::std::chrono::steady_clock::now() - startTime).count();

            LogExecution(stmt, result != nullptr, static_cast<uint32>(duration));

            return result;
        }
    }
    catch (::std::exception const& e)
    {
        TC_LOG_ERROR("module.playerbot.database",
            "Exception during statement execution: {}", e.what());

        if (callback) callback(nullptr);
        return nullptr;
    }
}

PreparedQueryResult SafeExecutionEngine::ExecuteWithRetry(CharacterDatabasePreparedStatement* stmt,
                                                         uint32 maxRetries,
                                                         uint32 retryDelayMs)
{
    if (!stmt)
    {
        return nullptr;
    }

    for (uint32 attempt = 0; attempt <= maxRetries; ++attempt)
    {
        PreparedQueryResult result = ExecuteWithSafety(stmt, false);

        if (result)
        {
            return result;
        }

        if (attempt < maxRetries)
        {
            TC_LOG_WARN("module.playerbot.database",
                "Query failed, retrying ({}/{})", attempt + 1, maxRetries);

            ::std::this_thread::sleep_for(::std::chrono::milliseconds(retryDelayMs));
        }
    }

    TC_LOG_ERROR("module.playerbot.database",
        "Query failed after {} retries", maxRetries);

    return nullptr;
}

bool SafeExecutionEngine::HandleError(uint32 errorCode, ::std::string const& context)
{
    TC_LOG_ERROR("module.playerbot.database",
        "Database error {} in context: {}", errorCode, context);

    // Determine if error is recoverable
    if (IsTransientError(errorCode))
    {
        return true;  // Can retry
    }

    return false;  // Cannot retry
}

bool SafeExecutionEngine::IsTransientError(uint32 errorCode) const
{
    // MySQL transient error codes
    switch (errorCode)
    {
        case 1205:  // Lock wait timeout
        case 1213:  // Deadlock
        case 2006:  // MySQL server has gone away
        case 2013:  // Lost connection
            return true;
        default:
            return false;
    }
}

void SafeExecutionEngine::LogExecution(CharacterDatabasePreparedStatement* stmt,
                                      bool success,
                                      uint32 durationMs)
{
    if (durationMs > 100)
    {
        TC_LOG_WARN("module.playerbot.database",
            "Slow query execution: {}ms, Success: {}",
            durationMs, success);
    }
    else if (!success)
    {
        TC_LOG_DEBUG("module.playerbot.database",
            "Query execution failed after {}ms",
            durationMs);
    }
}

void PlayerbotCharacterDBInterface::Update(uint32 diff)
{
    if (!_initialized.load() || _shutdown.load())
        return;

    // Process sync queue regularly to handle sync-only statements
    ProcessSyncQueue();
}

template<typename T>
SQLQueryHolderCallback PlayerbotCharacterDBInterface::DelayQueryHolder(::std::shared_ptr<T> holder)
{
    if (!_initialized.load())
    {
        TC_LOG_ERROR("module.playerbot.database",
            "PlayerbotCharacterDBInterface::DelayQueryHolder called before initialization");
        // Return empty callback that will never trigger
        auto emptyFuture = ::std::async(::std::launch::deferred, [](){});
        return SQLQueryHolderCallback(nullptr, ::std::move(emptyFuture));
    }

    // Route QueryHolder to the standard CharacterDatabase for async processing
    // This ensures proper async callback handling while maintaining safety
    TC_LOG_INFO("module.playerbot.database",
        " DelayQueryHolder: Routing QueryHolder to CharacterDatabase for async processing");

    SQLQueryHolderCallback result = CharacterDatabase.DelayQueryHolder(holder);
    TC_LOG_INFO("module.playerbot.database",
        " DelayQueryHolder: CharacterDatabase.DelayQueryHolder completed successfully");

    return result;
}

// Explicit template instantiation for BotLoginQueryHolder
template SQLQueryHolderCallback PlayerbotCharacterDBInterface::DelayQueryHolder<CharacterDatabaseQueryHolder>(::std::shared_ptr<CharacterDatabaseQueryHolder> holder);

} // namespace Playerbot