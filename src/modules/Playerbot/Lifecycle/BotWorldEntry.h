/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 */

#ifndef BOT_WORLD_ENTRY_H
#define BOT_WORLD_ENTRY_H

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include "ObjectGuid.h"
#include <memory>
#include <functional>
#include <chrono>
#include <atomic>
#include <queue>
#include <mutex>

class Player;
class Map;
class WorldSession;

namespace Playerbot {

class BotSession;
class BotAI;

/**
 * Bot World Entry State Machine
 *
 * Manages the complete bot entry sequence from character load to fully active in world.
 * Designed for high performance with support for 100+ concurrent bot logins.
 */
enum class BotWorldEntryState : uint8
{
    NONE,                       // Initial state
    CHARACTER_LOADED,           // Character data loaded from database
    PLAYER_CREATED,            // Player object created and initialized
    MAP_LOADING,               // Loading target map
    MAP_LOADED,                // Map loaded and ready
    ADDING_TO_MAP,             // Adding player to map
    IN_WORLD,                  // Successfully added to world
    AI_INITIALIZING,           // Initializing AI system
    AI_ACTIVE,                 // AI fully active
    FULLY_ACTIVE,              // Bot is fully operational
    FAILED,                    // Entry failed at some point
    CLEANUP                    // Cleaning up after failure or logout
};

/**
 * Performance metrics for bot world entry
 */
struct BotWorldEntryMetrics
{
    ::std::chrono::steady_clock::time_point startTime;
    ::std::chrono::steady_clock::time_point endTime;

    // Phase durations in microseconds
    uint32 databaseLoadTime = 0;
    uint32 playerCreationTime = 0;
    uint32 mapLoadTime = 0;
    uint32 worldEntryTime = 0;
    uint32 aiInitTime = 0;
    uint32 totalTime = 0;

    // Memory usage
    size_t memoryBeforeEntry = 0;
    size_t memoryAfterEntry = 0;

    // Error tracking
    ::std::string lastError;
    BotWorldEntryState failedState = BotWorldEntryState::NONE;
};

/**
 * Bot World Entry Manager
 *
 * Central coordinator for bot world entry operations.
 * Ensures proper sequencing and error handling.
 */
class TC_GAME_API BotWorldEntry
{
public:
    using EntryCallback = ::std::function<void(bool success, BotWorldEntryMetrics const& metrics)>;

    BotWorldEntry(::std::shared_ptr<BotSession> session, ObjectGuid characterGuid);
    ~BotWorldEntry();

    // === Main Entry Process ===

    /**
     * Begin the world entry process for this bot
     * @param callback Optional callback when entry completes
     * @return true if entry process started successfully
     */
    bool BeginWorldEntry(EntryCallback callback = nullptr);

    /**
     * Process the next step in world entry sequence
     * Called from session Update() or dedicated worker thread
     * @param diff Time since last update in milliseconds
     * @return true if still processing, false if complete or failed
     */
    bool ProcessWorldEntry(uint32 diff);

    /**
     * Synchronous world entry (blocks until complete)
     * WARNING: Should only be used during server startup or testing
     * @param timeoutMs Maximum time to wait in milliseconds
     * @return true if successfully entered world
     */
    bool EnterWorldSync(uint32 timeoutMs = 30000);

    // === State Management ===

    BotWorldEntryState GetState() const { return _state.load(); }
    bool IsComplete() const { return _state == BotWorldEntryState::FULLY_ACTIVE; }
    bool IsFailed() const { return _state == BotWorldEntryState::FAILED; }
    bool IsProcessing() const;

    // === Performance Monitoring ===

    BotWorldEntryMetrics const& GetMetrics() const { return _metrics; }
    uint32 GetElapsedTime() const;

    // === Error Handling ===

    ::std::string GetLastError() const { return _metrics.lastError; }
    void SetError(::std::string const& error);

private:
    // === State Transition Functions ===

    bool TransitionToState(BotWorldEntryState newState);
    void RecordStateTransition(BotWorldEntryState oldState, BotWorldEntryState newState);

    // === Phase Implementation Functions ===

    /**
     * Phase 1: Load character data from database
     */
    bool LoadCharacterData();

    /**
     * Phase 2: Create and initialize Player object
     */
    bool CreatePlayerObject();

    /**
     * Phase 3: Load target map
     */
    bool LoadTargetMap();

    /**
     * Phase 4: Add player to map and world
     */
    bool AddPlayerToWorld();

    /**
     * Phase 5: Initialize AI system
     */
    bool InitializeAI();

    /**
     * Phase 6: Finalize and activate bot
     */
    bool FinalizeBotActivation();

    // === Helper Functions ===

    /**
     * Send required packets before adding to map
     */
    void SendInitialPacketsBeforeMap();

    /**
     * Send required packets after adding to map
     */
    void SendInitialPacketsAfterMap();

    /**
     * Initialize bot movement and position
     */
    void InitializeBotPosition();

    /**
     * Set up initial bot equipment and appearance
     */
    void InitializeBotAppearance();

    /**
     * Handle world entry failure and cleanup
     */
    void HandleWorldEntryFailure(::std::string const& reason);

    /**
     * Clean up resources on failure or logout
     */
    void Cleanup();

    /**
     * Get current process memory usage in bytes
     * Platform-specific implementation (Windows/Linux/macOS)
     * @return Memory usage in bytes, or 0 if unsupported
     */
    size_t GetCurrentMemoryUsage() const;

private:
    // Core components
    ::std::shared_ptr<BotSession> _session;
    ObjectGuid _characterGuid;
    Player* _player;

    // State management
    ::std::atomic<BotWorldEntryState> _state;
    ::std::atomic<bool> _processing;

    // Performance tracking
    BotWorldEntryMetrics _metrics;

    // Callback management
    EntryCallback _callback;
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_SPAWNER> _callbackMutex;

    // Error handling
    uint32 _retryCount;
    static constexpr uint32 MAX_RETRY_COUNT = 3;

    // Timeout management
    ::std::chrono::steady_clock::time_point _phaseStartTime;
    static constexpr auto PHASE_TIMEOUT = ::std::chrono::seconds(10);

    // Thread safety
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_SPAWNER> _stateMutex;
};

/**
 * Bot World Entry Queue Manager
 *
 * Manages concurrent bot world entries to prevent server overload
 */
class TC_GAME_API BotWorldEntryQueue final 
{
public:
    static BotWorldEntryQueue* instance();

    /**
     * Queue a bot for world entry
     * @param entry The bot world entry to queue
     * @return Position in queue (0 = immediate processing)
     */
    uint32 QueueEntry(::std::shared_ptr<BotWorldEntry> entry);

    /**
     * Process queued entries
     * @param maxConcurrent Maximum concurrent entries to process
     */
    void ProcessQueue(uint32 maxConcurrent = 10);

    /**
     * Get current queue statistics
     */
    struct QueueStats
    {
        uint32 queuedEntries;
        uint32 activeEntries;
        uint32 completedEntries;
        uint32 failedEntries;
        float averageEntryTime; // in seconds
    };

    QueueStats GetStats() const;

    /**
     * Clear all queued entries (emergency use only)
     */
    void ClearQueue();

private:
    BotWorldEntryQueue() = default;
    ~BotWorldEntryQueue() = default;

    // Queue management
    ::std::queue<::std::shared_ptr<BotWorldEntry>> _pendingQueue;
    ::std::vector<::std::shared_ptr<BotWorldEntry>> _activeEntries;
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_SPAWNER> _queueMutex;

    // Statistics
    ::std::atomic<uint32> _totalCompleted{0};
    ::std::atomic<uint32> _totalFailed{0};
    ::std::atomic<uint64> _totalEntryTime{0}; // in microseconds

    // Singleton
    BotWorldEntryQueue(BotWorldEntryQueue const&) = delete;
    BotWorldEntryQueue& operator=(BotWorldEntryQueue const&) = delete;
};

} // namespace Playerbot

#endif // BOT_WORLD_ENTRY_H