/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * BOT FACTORY - Two-Phase Initialization System
 *
 * Enterprise-grade factory pattern for bot creation implementing strict
 * lifecycle state management with the Two-Phase AddToWorld pattern.
 *
 * DESIGN PRINCIPLES:
 * 1. Factory is the ONLY way to create bots (enforced via private constructors)
 * 2. All initialization happens BEFORE AddToWorld()
 * 3. State machine ensures correct initialization order
 * 4. Events are deferred until bot is fully ACTIVE
 * 5. Comprehensive metrics for debugging and monitoring
 *
 * CREATION FLOW:
 *   BotFactory::Create()
 *       |
 *       v
 *   [CREATED] - BotSession allocated, Player object created (but not loaded)
 *       |
 *       v
 *   BotFactory::LoadFromDatabase() - Async or sync DB queries
 *       |
 *       v
 *   [LOADING_DB] - Player data loading from database
 *       |
 *       v
 *   BotFactory::InitializeManagers() - BotAI and all managers created
 *       |
 *       v
 *   [INITIALIZING_MANAGERS] - Managers being initialized with player data
 *       |
 *       v
 *   BotFactory::FinalizeCreation() - Final validation, mark READY
 *       |
 *       v
 *   [READY] - AddToWorld() is now SAFE
 *       |
 *       v
 *   Map::AddPlayerToMap() / Player::AddToWorld()
 *       |
 *       v
 *   First UpdateAI() - Process deferred events, mark ACTIVE
 *       |
 *       v
 *   [ACTIVE] - Bot is fully operational
 */

#pragma once

#include "BotLifecycleState.h"
#include "ObjectGuid.h"
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

class Player;
class WorldSession;

namespace Playerbot
{

// Forward declarations
class BotAI;
class BotSession;
class GameSystemsManager;

/**
 * @brief Result of a bot creation operation
 */
struct TC_GAME_API BotCreationResult
{
    bool success{false};
    std::string errorMessage;
    ObjectGuid botGuid;
    BotInitState finalState{BotInitState::FAILED};

    // Timing metrics
    std::chrono::milliseconds creationTime{0};
    std::chrono::milliseconds dbLoadTime{0};
    std::chrono::milliseconds managerInitTime{0};

    explicit operator bool() const noexcept { return success; }

    static BotCreationResult Success(ObjectGuid guid, BotInitState state)
    {
        BotCreationResult result;
        result.success = true;
        result.botGuid = guid;
        result.finalState = state;
        return result;
    }

    static BotCreationResult Failure(std::string_view error)
    {
        BotCreationResult result;
        result.success = false;
        result.errorMessage = std::string(error);
        result.finalState = BotInitState::FAILED;
        return result;
    }
};

/**
 * @brief Configuration for bot creation
 */
struct TC_GAME_API BotCreationConfig
{
    // Required parameters
    ObjectGuid ownerGuid;          // GUID of the player who owns this bot
    ObjectGuid botGuid;            // GUID of the character to load as bot
    uint32 accountId{0};           // Account ID for the bot

    // Optional parameters
    bool async{false};             // Use async database loading (recommended for many bots)
    bool deferEventProcessing{true}; // Queue events until ACTIVE (recommended)
    uint32 timeoutMs{30000};       // Timeout for entire creation process

    // Callbacks (optional)
    std::function<void(const BotCreationResult&)> onComplete;
    std::function<void(BotInitState, BotInitState)> onStateChange;
    std::function<void(std::string_view)> onError;

    // Validation
    bool IsValid() const
    {
        return ownerGuid.IsPlayer() && botGuid.IsPlayer() && accountId > 0;
    }
};

/**
 * @brief Factory for creating and managing bot instances
 *
 * This factory implements the Two-Phase AddToWorld pattern:
 * Phase 1: Complete all initialization (DB load, managers, etc.)
 * Phase 2: AddToWorld() only after everything is ready
 *
 * Thread Safety:
 * - Factory methods are thread-safe
 * - Individual bot operations should be called from the appropriate thread
 *
 * Usage:
 *   BotCreationConfig config;
 *   config.ownerGuid = ownerPlayer->GetGUID();
 *   config.botGuid = characterGuid;
 *   config.accountId = botAccountId;
 *
 *   auto result = BotFactory::CreateBot(config);
 *   if (result)
 *   {
 *       // Bot is in READY state - safe to add to world
 *       BotSession* session = BotFactory::GetBotSession(result.botGuid);
 *       if (session && session->GetPlayer())
 *       {
 *           map->AddPlayerToMap(session->GetPlayer());
 *           // Bot will transition to ACTIVE on first UpdateAI
 *       }
 *   }
 */
class TC_GAME_API BotFactory
{
public:
    // Singleton access
    static BotFactory& Instance();

    // Non-copyable, non-movable
    BotFactory(const BotFactory&) = delete;
    BotFactory& operator=(const BotFactory&) = delete;
    BotFactory(BotFactory&&) = delete;
    BotFactory& operator=(BotFactory&&) = delete;

    // ========================================================================
    // BOT CREATION - Primary Factory Method
    // ========================================================================

    /**
     * @brief Create a new bot with full lifecycle management
     *
     * This is the primary factory method. It creates a bot and initializes
     * it through all stages until it reaches READY state.
     *
     * @param config Configuration for bot creation
     * @return Result containing success status, GUID, and timing metrics
     *
     * After successful creation:
     * - Bot is in READY state
     * - All managers are initialized
     * - Safe to call AddToWorld()
     * - Events are queued until ACTIVE
     */
    BotCreationResult CreateBot(const BotCreationConfig& config);

    /**
     * @brief Create multiple bots in batch (optimized for many bots)
     *
     * @param configs Vector of bot configurations
     * @return Map of bot GUIDs to their creation results
     */
    std::unordered_map<ObjectGuid, BotCreationResult> CreateBots(
        const std::vector<BotCreationConfig>& configs);

    // ========================================================================
    // LIFECYCLE MANAGEMENT
    // ========================================================================

    /**
     * @brief Get the lifecycle manager for a bot
     * @return nullptr if bot not found or doesn't have lifecycle management
     */
    BotInitStateManager* GetLifecycleManager(ObjectGuid botGuid);
    const BotInitStateManager* GetLifecycleManager(ObjectGuid botGuid) const;

    /**
     * @brief Get the current lifecycle state of a bot
     * @return FAILED if bot not found
     */
    BotInitState GetBotState(ObjectGuid botGuid) const;

    /**
     * @brief Check if a bot is in a state safe for player data access
     */
    bool IsPlayerDataSafe(ObjectGuid botGuid) const;

    /**
     * @brief Check if a bot is fully operational
     */
    bool IsBotOperational(ObjectGuid botGuid) const;

    /**
     * @brief Mark a bot as ACTIVE (called from first UpdateAI)
     * @return true if transition succeeded
     */
    bool MarkBotActive(ObjectGuid botGuid);

    /**
     * @brief Start bot removal (called when RemoveFromWorld begins)
     * @return true if transition succeeded
     */
    bool StartBotRemoval(ObjectGuid botGuid);

    /**
     * @brief Complete bot destruction (called during cleanup)
     * @return true if transition succeeded
     */
    bool DestroyBot(ObjectGuid botGuid);

    // ========================================================================
    // SESSION ACCESS
    // ========================================================================

    /**
     * @brief Get the BotSession for a bot
     * @return nullptr if not found
     */
    BotSession* GetBotSession(ObjectGuid botGuid);
    const BotSession* GetBotSession(ObjectGuid botGuid) const;

    /**
     * @brief Get the Player object for a bot (only if state >= READY)
     * @return nullptr if not found or not in safe state
     */
    Player* GetBotPlayer(ObjectGuid botGuid);
    const Player* GetBotPlayer(ObjectGuid botGuid) const;

    /**
     * @brief Get the BotAI for a bot (only if state >= READY)
     * @return nullptr if not found or not in safe state
     */
    BotAI* GetBotAI(ObjectGuid botGuid);
    const BotAI* GetBotAI(ObjectGuid botGuid) const;

    // ========================================================================
    // DEFERRED EVENTS
    // ========================================================================

    /**
     * @brief Queue an event to be processed when bot becomes ACTIVE
     * @return true if queued, false if bot is already ACTIVE (process immediately)
     */
    bool QueueDeferredEvent(ObjectGuid botGuid, BotInitStateManager::DeferredEvent event);

    /**
     * @brief Queue a callback to be executed when bot becomes ACTIVE
     */
    bool QueueDeferredCallback(ObjectGuid botGuid, std::function<void()> callback);

    /**
     * @brief Process all queued events for a bot
     * @param handler Function to process each event
     * @return Number of events processed
     */
    uint32 ProcessDeferredEvents(ObjectGuid botGuid,
        std::function<void(const BotInitStateManager::DeferredEvent&)> handler);

    // ========================================================================
    // DIAGNOSTICS
    // ========================================================================

    /**
     * @brief Get metrics for a specific bot
     */
    std::optional<BotInitStateManager::InitializationMetrics> GetBotMetrics(ObjectGuid botGuid) const;

    /**
     * @brief Get state history for a bot (for debugging)
     */
    std::string GetBotStateHistory(ObjectGuid botGuid) const;

    /**
     * @brief Get summary statistics for all managed bots
     */
    struct FactoryStatistics
    {
        uint32 totalBots{0};
        uint32 botsCreated{0};
        uint32 botsLoading{0};
        uint32 botsInitializing{0};
        uint32 botsReady{0};
        uint32 botsActive{0};
        uint32 botsRemoving{0};
        uint32 botsFailed{0};

        std::chrono::milliseconds avgCreationTime{0};
        std::chrono::milliseconds avgDbLoadTime{0};
        std::chrono::milliseconds avgManagerInitTime{0};

        uint32 totalDeferredEvents{0};
    };

    FactoryStatistics GetStatistics() const;

    /**
     * @brief Log detailed diagnostics for all bots
     */
    void LogDiagnostics() const;

    // ========================================================================
    // CLEANUP
    // ========================================================================

    /**
     * @brief Cleanup all bots (called during server shutdown)
     */
    void Shutdown();

    /**
     * @brief Remove a bot from factory management (does NOT destroy the bot)
     */
    void UnregisterBot(ObjectGuid botGuid);

private:
    BotFactory();
    ~BotFactory();

    // Internal creation stages
    struct BotCreationContext
    {
        BotCreationConfig config;
        std::unique_ptr<BotInitStateManager> lifecycleManager;
        BotSession* session{nullptr};
        Player* player{nullptr};
        BotAI* ai{nullptr};

        std::chrono::steady_clock::time_point startTime;
        std::chrono::steady_clock::time_point dbLoadEndTime;
        std::chrono::steady_clock::time_point managerInitEndTime;

        BotCreationResult ToResult() const;
    };

    // Stage implementations
    bool StageAllocatePlayer(BotCreationContext& ctx);
    bool StageDatabaseLoading(BotCreationContext& ctx);
    bool StageInitializeManagers(BotCreationContext& ctx);
    bool StageFinalizeCreation(BotCreationContext& ctx);

    // Helper to handle stage failure
    void HandleStageFailure(BotCreationContext& ctx, std::string_view stage, std::string_view reason);

    // Registered bots and their lifecycle managers
    mutable std::mutex _botsMutex;
    std::unordered_map<ObjectGuid, std::unique_ptr<BotInitStateManager>> _lifecycleManagers;

    // Statistics tracking
    mutable std::mutex _statsMutex;
    uint32 _totalCreated{0};
    uint32 _totalFailed{0};
    std::chrono::milliseconds _totalCreationTime{0};
    std::chrono::milliseconds _totalDbLoadTime{0};
    std::chrono::milliseconds _totalManagerInitTime{0};
};

/**
 * @brief Convenience macro for accessing the factory singleton
 */
#define sBotFactory BotFactory::Instance()

/**
 * @brief RAII guard that ensures a bot is in READY state before proceeding
 *
 * Usage:
 *   if (auto guard = BotReadyGuard::TryCreate(botGuid))
 *   {
 *       // Bot is guaranteed to be in READY or ACTIVE state
 *       Player* player = guard->GetPlayer();
 *       // Safe to access player data
 *   }
 */
class TC_GAME_API BotReadyGuard
{
public:
    static std::unique_ptr<BotReadyGuard> TryCreate(ObjectGuid botGuid);

    Player* GetPlayer() const { return _player; }
    BotSession* GetSession() const { return _session; }
    BotAI* GetAI() const { return _ai; }
    BotInitState GetState() const { return _state; }

    explicit operator bool() const noexcept { return _valid; }

private:
    explicit BotReadyGuard(ObjectGuid botGuid);

    ObjectGuid _botGuid;
    Player* _player{nullptr};
    BotSession* _session{nullptr};
    BotAI* _ai{nullptr};
    BotInitState _state{BotInitState::FAILED};
    bool _valid{false};
};

} // namespace Playerbot
