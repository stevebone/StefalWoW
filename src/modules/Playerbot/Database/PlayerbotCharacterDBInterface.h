/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_CHARACTER_DB_INTERFACE_H
#define PLAYERBOT_CHARACTER_DB_INTERFACE_H

#include "Define.h"
#include "DatabaseEnvFwd.h"
#include "DatabaseEnv.h"
#include "CharacterDatabase.h"
#include "QueryHolder.h"
#include <atomic>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <chrono>

namespace Playerbot
{
    // Forward declarations
    class StatementClassifier;
    class ExecutionContext;
    class SafeExecutionEngine;

    /**
     * Enterprise-grade database interface wrapper that solves the sync/async mismatch
     * between TrinityCore's sync-only statements and Playerbot's async operations
     */
    class PlayerbotCharacterDBInterface final
    {
    public:
        // Singleton instance
        static PlayerbotCharacterDBInterface* instance();

        // Initialization and shutdown
        bool Initialize();
        void Shutdown();
        void Update(uint32 diff);

        // === Primary Interface Methods ===

        /**
         * Get a prepared statement with automatic sync/async routing
         * @param statementId The statement index from CharacterDatabaseStatements enum
         * @return Prepared statement or nullptr if invalid/unavailable
         */
        CharacterDatabasePreparedStatement* GetPreparedStatement(CharacterDatabaseStatements statementId);

        /**
         * Execute async query with automatic routing based on statement type
         * @param stmt The prepared statement to execute
         * @param callback Optional callback for result handling
         * @param timeoutMs Maximum execution time in milliseconds
         */
        void ExecuteAsync(CharacterDatabasePreparedStatement* stmt,
                         ::std::function<void(PreparedQueryResult)> callback = nullptr,
                         uint32 timeoutMs = 10000);

        /**
         * Execute QueryHolder asynchronously (for bot login systems)
         * Routes to appropriate database connection based on context
         * @param holder The query holder to execute
         * @return Callback handle for async processing
         */
        template<typename T>
        SQLQueryHolderCallback DelayQueryHolder(::std::shared_ptr<T> holder);

        /**
         * Execute synchronous query with safety checks
         * @param stmt The prepared statement to execute
         * @return Query result or nullptr on failure
         */
        PreparedQueryResult ExecuteSync(CharacterDatabasePreparedStatement* stmt);

        /**
         * Begin a database transaction with proper isolation
         * @return Transaction handle
         */
        CharacterDatabaseTransaction BeginTransaction();

        /**
         * Commit a transaction with safety checks
         * @param trans The transaction to commit
         * @param async Whether to commit asynchronously
         */
        void CommitTransaction(CharacterDatabaseTransaction trans, bool async = true);

        /**
         * Execute a direct SQL query (for migrations/setup only)
         * @param sql The SQL query to execute
         * @return True if successful
         */
        bool ExecuteDirectSQL(::std::string const& sql);

        // === Context Detection ===

        /**
         * Check if current thread is in async context
         * @return True if in async worker thread
         */
        bool IsAsyncContext() const;

        /**
         * Check if statement requires synchronous execution
         * @param statementId The statement to check
         * @return True if statement must be executed synchronously
         */
        bool IsSyncOnlyStatement(uint32 statementId) const;

        /**
         * Get the main thread ID for context detection
         * @return Main thread ID
         */
        ::std::thread::id GetMainThreadId() const { return _mainThreadId; }

        // === Performance Metrics ===

        struct Metrics
        {
            ::std::atomic<uint64> totalQueries{0};
            ::std::atomic<uint64> syncQueries{0};
            ::std::atomic<uint64> asyncQueries{0};
            ::std::atomic<uint64> routedQueries{0};
            ::std::atomic<uint64> errors{0};
            ::std::atomic<uint64> timeouts{0};
            ::std::atomic<uint32> avgResponseTimeMs{0};
            ::std::atomic<uint32> maxResponseTimeMs{0};

            void Reset()
            {
                totalQueries.store(0);
                syncQueries.store(0);
                asyncQueries.store(0);
                routedQueries.store(0);
                errors.store(0);
                timeouts.store(0);
                avgResponseTimeMs.store(0);
                maxResponseTimeMs.store(0);
            }
        };

        Metrics const& GetMetrics() const { return _metrics; }
        void ResetMetrics() { _metrics.Reset(); }

        // === Configuration ===

        struct Config
        {
            bool enableSmartRouting = true;
            bool enableMetrics = true;
            bool enableDetailedLogging = false;
            uint32 defaultTimeoutMs = 10000;
            uint32 syncQueueMaxSize = 1000;
            uint32 asyncQueueMaxSize = 10000;
            bool fallbackToDirectDatabase = true;
        };

        Config& GetConfig() { return _config; }
        void UpdateConfig(Config const& config) { _config = config; }

    private:
        PlayerbotCharacterDBInterface();
        ~PlayerbotCharacterDBInterface();

        // Disable copy/move
        PlayerbotCharacterDBInterface(PlayerbotCharacterDBInterface const&) = delete;
        PlayerbotCharacterDBInterface& operator=(PlayerbotCharacterDBInterface const&) = delete;
        PlayerbotCharacterDBInterface(PlayerbotCharacterDBInterface&&) = delete;
        PlayerbotCharacterDBInterface& operator=(PlayerbotCharacterDBInterface&&) = delete;

        // === Internal Implementation ===

        /**
         * Route query to appropriate execution path
         * @param stmt The statement to route
         * @param callback Optional result callback
         * @param forceSync Force synchronous execution
         * @return True if routed successfully
         */
        bool RouteQuery(CharacterDatabasePreparedStatement* stmt,
                       ::std::function<void(PreparedQueryResult)> callback,
                       bool forceSync = false);

        /**
         * Execute sync-only statement from async context
         * @param stmt The sync-only statement
         * @param callback Result callback
         */
        void ExecuteSyncFromAsync(CharacterDatabasePreparedStatement* stmt,
                                 ::std::function<void(PreparedQueryResult)> callback);

        /**
         * Process sync queue on main thread
         */
        void ProcessSyncQueue();

        /**
         * Initialize statement classification
         */
        void InitializeStatementClassification();

        /**
         * Detect current execution context
         * @return Current context information
         */
        ExecutionContext DetectContext() const;

        /**
         * Update performance metrics
         */
        void UpdateMetrics(uint32 responseTimeMs, bool isSync, bool hadError = false);

        // === Sync Queue for Async-to-Sync Bridge ===

        struct SyncRequest
        {
            CharacterDatabasePreparedStatement* statement;
            ::std::function<void(PreparedQueryResult)> callback;
            ::std::chrono::steady_clock::time_point submitTime;
            uint32 timeoutMs;
            ::std::condition_variable* completionSignal;
            PreparedQueryResult* result;
            bool completed;

            SyncRequest() : statement(nullptr), timeoutMs(10000),
                          completionSignal(nullptr), result(nullptr), completed(false) {}
        };

        // === Member Variables ===

        ::std::atomic<bool> _initialized{false};
        ::std::atomic<bool> _shutdown{false};

        // Statement classification
        ::std::unique_ptr<StatementClassifier> _classifier;
        ::std::unordered_set<uint32> _syncOnlyStatements;
        ::std::unordered_map<uint32, ::std::string> _statementNames;

        // Sync queue for async-to-sync bridge
        ::std::queue<::std::shared_ptr<SyncRequest>> _syncQueue;
        mutable ::std::recursive_mutex _syncQueueMutex;
        ::std::condition_variable _syncQueueCV;

        // Thread tracking
        ::std::thread::id _mainThreadId;
        ::std::unordered_set<::std::thread::id> _asyncThreadIds;
        mutable ::std::recursive_mutex _threadMutex;

        // Metrics
        mutable Metrics _metrics;
        ::std::chrono::steady_clock::time_point _startTime;

        // Configuration
        Config _config;

        // Safe execution engine
        ::std::unique_ptr<SafeExecutionEngine> _executionEngine;
    };

    /**
     * Statement Classifier - Determines statement execution requirements
     */
    class StatementClassifier
    {
    public:
        enum StatementType
        {
            SYNC_ONLY,      // Must be executed synchronously
            ASYNC_SAFE,     // Can be executed asynchronously
            DUAL_MODE,      // Can be executed either way
            UNKNOWN         // Not classified
        };

        StatementClassifier();
        ~StatementClassifier();

        void Initialize();
        StatementType ClassifyStatement(uint32 statementId) const;
        ::std::string GetStatementName(uint32 statementId) const;

    private:
        void LoadSyncOnlyStatements();
        void LoadAsyncSafeStatements();

        ::std::unordered_map<uint32, StatementType> _statementTypes;
        ::std::unordered_map<uint32, ::std::string> _statementNames;
    };

    /**
     * Execution Context - Tracks current execution environment
     */
    class ExecutionContext
    {
    public:
        enum ContextType
        {
            MAIN_THREAD,        // Main worldserver thread
            ASYNC_WORKER,       // Async database worker
            BOT_THREAD,         // Bot-specific thread
            UNKNOWN_CONTEXT     // Unknown context
        };

        ExecutionContext();

        ContextType GetType() const { return _type; }
        ::std::thread::id GetThreadId() const { return _threadId; }
        bool IsAsync() const { return _type == ASYNC_WORKER || _type == BOT_THREAD; }
        bool IsMainThread() const { return _type == MAIN_THREAD; }

        static ExecutionContext Detect();

    private:
        ContextType _type;
        ::std::thread::id _threadId;
        ::std::string _threadName;
    };

    /**
     * Safe Execution Engine - Handles statement execution with proper error handling
     */
    class SafeExecutionEngine
    {
    public:
        SafeExecutionEngine();
        ~SafeExecutionEngine();

        void Initialize();
        void Shutdown();

        /**
         * Execute statement with comprehensive error handling
         * @param stmt Statement to execute
         * @param async Whether to execute asynchronously
         * @param callback Optional result callback
         * @return Result for sync execution, nullptr for async
         */
        PreparedQueryResult ExecuteWithSafety(CharacterDatabasePreparedStatement* stmt,
                                             bool async,
                                             ::std::function<void(PreparedQueryResult)> callback = nullptr);

        /**
         * Execute with retry logic for transient failures
         */
        PreparedQueryResult ExecuteWithRetry(CharacterDatabasePreparedStatement* stmt,
                                            uint32 maxRetries = 3,
                                            uint32 retryDelayMs = 100);

    private:
        bool HandleError(uint32 errorCode, ::std::string const& context);
        bool IsTransientError(uint32 errorCode) const;
        void LogExecution(CharacterDatabasePreparedStatement* stmt, bool success, uint32 durationMs);

        ::std::atomic<bool> _initialized{false};
        ::std::atomic<uint64> _executionCounter{0};
    };

} // namespace Playerbot

// Convenience macro for accessing the interface
#define sPlayerbotCharDB Playerbot::PlayerbotCharacterDBInterface::instance()

#endif // PLAYERBOT_CHARACTER_DB_INTERFACE_H