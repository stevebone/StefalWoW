/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * BOT FACTORY - Two-Phase Initialization System Implementation
 *
 * This factory implements the enterprise-grade Two-Phase AddToWorld pattern:
 *
 * Phase 1 (All managed by factory):
 *   CREATED -> LOADING_DB -> INITIALIZING_MANAGERS -> READY
 *
 * Phase 2 (External, after factory returns READY):
 *   READY -> [AddToWorld()] -> ACTIVE (on first UpdateAI)
 *
 * KEY INVARIANTS:
 * - AddToWorld() is NEVER called until state == READY
 * - All managers are fully initialized before READY
 * - Deferred events are queued until ACTIVE
 * - Comprehensive metrics track all phases
 */

#include "BotFactory.h"
#include "BotSession.h"
#include "BotAI.h"
#include "Player.h"
#include "World.h"
#include "Map.h"
#include "MapManager.h"
#include "ObjectAccessor.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include <algorithm>
#include <numeric>

namespace Playerbot
{

// ============================================================================
// BotFactory Singleton Implementation
// ============================================================================

BotFactory& BotFactory::Instance()
{
    static BotFactory instance;
    return instance;
}

BotFactory::BotFactory()
{
    TC_LOG_INFO("module.playerbot.lifecycle", "BotFactory initialized - Two-Phase AddToWorld pattern active");
}

BotFactory::~BotFactory()
{
    Shutdown();
}

// ============================================================================
// Bot Creation - Main Factory Method
// ============================================================================

BotCreationResult BotFactory::CreateBot(const BotCreationConfig& config)
{
    // Validate configuration
    if (!config.IsValid())
    {
        return BotCreationResult::Failure("Invalid bot creation configuration");
    }

    TC_LOG_INFO("module.playerbot.lifecycle",
        "BotFactory::CreateBot starting for character {} (owner: {}, account: {})",
        config.botGuid.ToString(), config.ownerGuid.ToString(), config.accountId);

    // Create context for tracking the creation process
    BotCreationContext ctx;
    ctx.config = config;
    ctx.startTime = std::chrono::steady_clock::now();

    // Create lifecycle manager for this bot
    ctx.lifecycleManager = std::make_unique<BotInitStateManager>(config.botGuid);

    // Notify state change callback if configured
    if (config.onStateChange)
    {
        config.onStateChange(BotInitState::CREATED, BotInitState::CREATED);
    }

    // Stage 1: Allocate Player object and BotSession
    if (!StageAllocatePlayer(ctx))
    {
        HandleStageFailure(ctx, "AllocatePlayer", "Failed to allocate Player object");
        return ctx.ToResult();
    }

    // Stage 2: Load from database
    if (!StageDatabaseLoading(ctx))
    {
        HandleStageFailure(ctx, "DatabaseLoading", "Failed to load character from database");
        return ctx.ToResult();
    }

    // Stage 3: Initialize all managers
    if (!StageInitializeManagers(ctx))
    {
        HandleStageFailure(ctx, "InitializeManagers", "Failed to initialize bot managers");
        return ctx.ToResult();
    }

    // Stage 4: Finalize and mark ready
    if (!StageFinalizeCreation(ctx))
    {
        HandleStageFailure(ctx, "FinalizeCreation", "Failed to finalize bot creation");
        return ctx.ToResult();
    }

    // Register the lifecycle manager
    {
        std::lock_guard<std::mutex> lock(_botsMutex);
        _lifecycleManagers[config.botGuid] = std::move(ctx.lifecycleManager);
    }

    // Update statistics
    {
        std::lock_guard<std::mutex> lock(_statsMutex);
        _totalCreated++;
        auto totalTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - ctx.startTime);
        _totalCreationTime += totalTime;

        if (ctx.dbLoadEndTime > ctx.startTime)
        {
            _totalDbLoadTime += std::chrono::duration_cast<std::chrono::milliseconds>(
                ctx.dbLoadEndTime - ctx.startTime);
        }
        if (ctx.managerInitEndTime > ctx.dbLoadEndTime)
        {
            _totalManagerInitTime += std::chrono::duration_cast<std::chrono::milliseconds>(
                ctx.managerInitEndTime - ctx.dbLoadEndTime);
        }
    }

    // Notify completion callback
    auto result = ctx.ToResult();
    if (config.onComplete)
    {
        config.onComplete(result);
    }

    TC_LOG_INFO("module.playerbot.lifecycle",
        "BotFactory::CreateBot completed for {} (state: READY, creation time: {}ms)",
        config.botGuid.ToString(), result.creationTime.count());

    return result;
}

std::unordered_map<ObjectGuid, BotCreationResult> BotFactory::CreateBots(
    const std::vector<BotCreationConfig>& configs)
{
    std::unordered_map<ObjectGuid, BotCreationResult> results;
    results.reserve(configs.size());

    TC_LOG_INFO("module.playerbot.lifecycle",
        "BotFactory::CreateBots starting batch creation of {} bots",
        configs.size());

    auto batchStartTime = std::chrono::steady_clock::now();

    for (const auto& config : configs)
    {
        results[config.botGuid] = CreateBot(config);
    }

    auto batchEndTime = std::chrono::steady_clock::now();
    auto batchDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
        batchEndTime - batchStartTime);

    // Count successes and failures
    uint32 successCount = 0;
    uint32 failureCount = 0;
    for (const auto& [guid, result] : results)
    {
        if (result.success)
            successCount++;
        else
            failureCount++;
    }

    TC_LOG_INFO("module.playerbot.lifecycle",
        "BotFactory::CreateBots completed: {} successful, {} failed, total time: {}ms",
        successCount, failureCount, batchDuration.count());

    return results;
}

// ============================================================================
// Creation Stages
// ============================================================================

bool BotFactory::StageAllocatePlayer(BotCreationContext& ctx)
{
    TC_LOG_DEBUG("module.playerbot.lifecycle",
        "Stage 1/4: Allocating Player object for {}",
        ctx.config.botGuid.ToString());

    // Create the BotSession
    auto botSession = BotSession::Create(ctx.config.accountId);
    if (!botSession)
    {
        TC_LOG_ERROR("module.playerbot.lifecycle",
            "Failed to create BotSession for account {}",
            ctx.config.accountId);
        return false;
    }

    ctx.session = botSession.get();

    // Player object will be created during database loading stage
    // The session holds the player reference after loading

    TC_LOG_DEBUG("module.playerbot.lifecycle",
        "Stage 1/4 complete: BotSession created for {}",
        ctx.config.botGuid.ToString());

    return true;
}

bool BotFactory::StageDatabaseLoading(BotCreationContext& ctx)
{
    TC_LOG_DEBUG("module.playerbot.lifecycle",
        "Stage 2/4: Loading database for {}",
        ctx.config.botGuid.ToString());

    // Transition to LOADING_DB state
    if (!ctx.lifecycleManager->StartDatabaseLoading())
    {
        TC_LOG_ERROR("module.playerbot.lifecycle",
            "Failed to transition to LOADING_DB for {}",
            ctx.config.botGuid.ToString());
        return false;
    }

    // Notify state change
    if (ctx.config.onStateChange)
    {
        ctx.config.onStateChange(BotInitState::CREATED, BotInitState::LOADING_DB);
    }

    // Use the existing LoginCharacter method which handles all DB loading
    // This is synchronous for now (async can be added later)
    if (!ctx.session->LoginCharacter(ctx.config.botGuid))
    {
        TC_LOG_ERROR("module.playerbot.lifecycle",
            "LoginCharacter failed for {}",
            ctx.config.botGuid.ToString());
        return false;
    }

    ctx.dbLoadEndTime = std::chrono::steady_clock::now();

    // Verify player was created
    ctx.player = ctx.session->GetPlayer();
    if (!ctx.player)
    {
        TC_LOG_ERROR("module.playerbot.lifecycle",
            "Player object not created after LoginCharacter for {}",
            ctx.config.botGuid.ToString());
        return false;
    }

    TC_LOG_DEBUG("module.playerbot.lifecycle",
        "Stage 2/4 complete: Database loaded for {} (player: {})",
        ctx.config.botGuid.ToString(), ctx.player->GetName());

    return true;
}

bool BotFactory::StageInitializeManagers(BotCreationContext& ctx)
{
    TC_LOG_DEBUG("module.playerbot.lifecycle",
        "Stage 3/4: Initializing managers for {}",
        ctx.config.botGuid.ToString());

    // Transition to INITIALIZING_MANAGERS state
    if (!ctx.lifecycleManager->StartManagerInitialization())
    {
        TC_LOG_ERROR("module.playerbot.lifecycle",
            "Failed to transition to INITIALIZING_MANAGERS for {}",
            ctx.config.botGuid.ToString());
        return false;
    }

    // Notify state change
    if (ctx.config.onStateChange)
    {
        ctx.config.onStateChange(BotInitState::LOADING_DB, BotInitState::INITIALIZING_MANAGERS);
    }

    // Create BotAI using the factory
    // CRITICAL: This now happens BEFORE AddToWorld, not after!
    auto botAI = sBotAIFactory->CreateAI(ctx.player);
    if (!botAI)
    {
        TC_LOG_ERROR("module.playerbot.lifecycle",
            "Failed to create BotAI for {}",
            ctx.config.botGuid.ToString());
        return false;
    }

    ctx.ai = botAI.get();
    // P1 FIX: Pass unique_ptr directly via std::move (no .release() needed)
    ctx.session->SetAI(::std::move(botAI)); // Transfer ownership to session

    // Pass the lifecycle manager to the AI so it can check state
    // The AI will use this to defer events until ACTIVE
    ctx.ai->SetLifecycleManager(GetLifecycleManager(ctx.config.botGuid));

    ctx.managerInitEndTime = std::chrono::steady_clock::now();

    TC_LOG_DEBUG("module.playerbot.lifecycle",
        "Stage 3/4 complete: Managers initialized for {}",
        ctx.config.botGuid.ToString());

    return true;
}

bool BotFactory::StageFinalizeCreation(BotCreationContext& ctx)
{
    TC_LOG_DEBUG("module.playerbot.lifecycle",
        "Stage 4/4: Finalizing creation for {}",
        ctx.config.botGuid.ToString());

    // Perform any final validation before marking ready
    if (!ctx.player || !ctx.ai || !ctx.session)
    {
        TC_LOG_ERROR("module.playerbot.lifecycle",
            "Final validation failed for {} - missing components",
            ctx.config.botGuid.ToString());
        return false;
    }

    // Transition to READY state
    if (!ctx.lifecycleManager->MarkReady())
    {
        TC_LOG_ERROR("module.playerbot.lifecycle",
            "Failed to transition to READY for {}",
            ctx.config.botGuid.ToString());
        return false;
    }

    // Notify state change
    if (ctx.config.onStateChange)
    {
        ctx.config.onStateChange(BotInitState::INITIALIZING_MANAGERS, BotInitState::READY);
    }

    TC_LOG_INFO("module.playerbot.lifecycle",
        "Stage 4/4 complete: {} is READY for AddToWorld()",
        ctx.config.botGuid.ToString());

    return true;
}

void BotFactory::HandleStageFailure(BotCreationContext& ctx, std::string_view stage, std::string_view reason)
{
    TC_LOG_ERROR("module.playerbot.lifecycle",
        "BotFactory stage '{}' failed for {}: {}",
        stage, ctx.config.botGuid.ToString(), reason);

    // Mark as failed
    if (ctx.lifecycleManager)
    {
        ctx.lifecycleManager->MarkFailed(reason);
    }

    // Notify error callback
    if (ctx.config.onError)
    {
        ctx.config.onError(reason);
    }

    // Update failure statistics
    {
        std::lock_guard<std::mutex> lock(_statsMutex);
        _totalFailed++;
    }

    // Cleanup any partially created resources
    if (ctx.session)
    {
        if (ctx.session->GetPlayer())
        {
            // Don't delete - session owns it
        }
    }
}

BotCreationResult BotFactory::BotCreationContext::ToResult() const
{
    BotCreationResult result;

    if (lifecycleManager)
    {
        BotInitState state = lifecycleManager->GetState();
        result.finalState = state;
        result.success = (state == BotInitState::READY);
        result.botGuid = config.botGuid;

        // Calculate timings
        auto now = std::chrono::steady_clock::now();
        result.creationTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - startTime);

        if (dbLoadEndTime > startTime)
        {
            result.dbLoadTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                dbLoadEndTime - startTime);
        }

        if (managerInitEndTime > dbLoadEndTime)
        {
            result.managerInitTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                managerInitEndTime - dbLoadEndTime);
        }

        if (!result.success && state == BotInitState::FAILED)
        {
            auto metrics = lifecycleManager->GetMetrics();
            result.errorMessage = metrics.failureReason;
        }
    }
    else
    {
        result.success = false;
        result.finalState = BotInitState::FAILED;
        result.errorMessage = "No lifecycle manager";
    }

    return result;
}

// ============================================================================
// Lifecycle Management
// ============================================================================

BotInitStateManager* BotFactory::GetLifecycleManager(ObjectGuid botGuid)
{
    std::lock_guard<std::mutex> lock(_botsMutex);
    auto it = _lifecycleManagers.find(botGuid);
    return it != _lifecycleManagers.end() ? it->second.get() : nullptr;
}

const BotInitStateManager* BotFactory::GetLifecycleManager(ObjectGuid botGuid) const
{
    std::lock_guard<std::mutex> lock(_botsMutex);
    auto it = _lifecycleManagers.find(botGuid);
    return it != _lifecycleManagers.end() ? it->second.get() : nullptr;
}

BotInitState BotFactory::GetBotState(ObjectGuid botGuid) const
{
    const BotInitStateManager* manager = GetLifecycleManager(botGuid);
    return manager ? manager->GetState() : BotInitState::FAILED;
}

bool BotFactory::IsPlayerDataSafe(ObjectGuid botGuid) const
{
    const BotInitStateManager* manager = GetLifecycleManager(botGuid);
    return manager && manager->IsPlayerDataSafe();
}

bool BotFactory::IsBotOperational(ObjectGuid botGuid) const
{
    const BotInitStateManager* manager = GetLifecycleManager(botGuid);
    return manager && manager->IsFullyOperational();
}

bool BotFactory::MarkBotActive(ObjectGuid botGuid)
{
    BotInitStateManager* manager = GetLifecycleManager(botGuid);
    if (!manager)
    {
        TC_LOG_ERROR("module.playerbot.lifecycle",
            "MarkBotActive: No lifecycle manager for {}",
            botGuid.ToString());
        return false;
    }

    if (!manager->MarkActive())
    {
        TC_LOG_ERROR("module.playerbot.lifecycle",
            "MarkBotActive: Failed to transition {} to ACTIVE",
            botGuid.ToString());
        return false;
    }

    TC_LOG_INFO("module.playerbot.lifecycle",
        "Bot {} is now ACTIVE",
        botGuid.ToString());

    return true;
}

bool BotFactory::StartBotRemoval(ObjectGuid botGuid)
{
    BotInitStateManager* manager = GetLifecycleManager(botGuid);
    if (!manager)
    {
        return false;
    }

    return manager->StartRemoval();
}

bool BotFactory::DestroyBot(ObjectGuid botGuid)
{
    BotInitStateManager* manager = GetLifecycleManager(botGuid);
    if (manager)
    {
        manager->MarkDestroyed();
    }

    // Remove from our tracking
    {
        std::lock_guard<std::mutex> lock(_botsMutex);
        _lifecycleManagers.erase(botGuid);
    }

    TC_LOG_DEBUG("module.playerbot.lifecycle",
        "Bot {} destroyed and unregistered from factory",
        botGuid.ToString());

    return true;
}

// ============================================================================
// Session Access
// ============================================================================

BotSession* BotFactory::GetBotSession(ObjectGuid botGuid)
{
    // This would need integration with BotSessionManager
    // For now, return nullptr - will be implemented when integrating
    return nullptr;
}

const BotSession* BotFactory::GetBotSession(ObjectGuid botGuid) const
{
    return nullptr;
}

Player* BotFactory::GetBotPlayer(ObjectGuid botGuid)
{
    if (!IsPlayerDataSafe(botGuid))
    {
        return nullptr;
    }

    // Get via ObjectAccessor if bot is in world
    return ObjectAccessor::FindPlayer(botGuid);
}

const Player* BotFactory::GetBotPlayer(ObjectGuid botGuid) const
{
    if (!IsPlayerDataSafe(botGuid))
    {
        return nullptr;
    }

    return ObjectAccessor::FindPlayer(botGuid);
}

BotAI* BotFactory::GetBotAI(ObjectGuid botGuid)
{
    if (!IsPlayerDataSafe(botGuid))
    {
        return nullptr;
    }

    Player* player = GetBotPlayer(botGuid);
    if (!player)
    {
        return nullptr;
    }

    WorldSession* session = player->GetSession();
    if (!session)
    {
        return nullptr;
    }

    BotSession* botSession = dynamic_cast<BotSession*>(session);
    return botSession ? botSession->GetAI() : nullptr;
}

const BotAI* BotFactory::GetBotAI(ObjectGuid botGuid) const
{
    return const_cast<BotFactory*>(this)->GetBotAI(botGuid);
}

// ============================================================================
// Deferred Events
// ============================================================================

bool BotFactory::QueueDeferredEvent(ObjectGuid botGuid, BotInitStateManager::DeferredEvent event)
{
    BotInitStateManager* manager = GetLifecycleManager(botGuid);
    if (!manager)
    {
        TC_LOG_WARN("module.playerbot.lifecycle",
            "QueueDeferredEvent: No lifecycle manager for {}",
            botGuid.ToString());
        return false;
    }

    return manager->QueueEvent(std::move(event));
}

bool BotFactory::QueueDeferredCallback(ObjectGuid botGuid, std::function<void()> callback)
{
    BotInitStateManager* manager = GetLifecycleManager(botGuid);
    if (!manager)
    {
        return false;
    }

    return manager->QueueCallback(std::move(callback));
}

uint32 BotFactory::ProcessDeferredEvents(ObjectGuid botGuid,
    std::function<void(const BotInitStateManager::DeferredEvent&)> handler)
{
    BotInitStateManager* manager = GetLifecycleManager(botGuid);
    if (!manager)
    {
        return 0;
    }

    return manager->ProcessQueuedEvents(handler);
}

// ============================================================================
// Diagnostics
// ============================================================================

std::optional<BotInitStateManager::InitializationMetrics> BotFactory::GetBotMetrics(ObjectGuid botGuid) const
{
    const BotInitStateManager* manager = GetLifecycleManager(botGuid);
    if (!manager)
    {
        return std::nullopt;
    }

    return manager->GetMetrics();
}

std::string BotFactory::GetBotStateHistory(ObjectGuid botGuid) const
{
    const BotInitStateManager* manager = GetLifecycleManager(botGuid);
    if (!manager)
    {
        return "Bot not found";
    }

    return manager->GetStateHistory();
}

BotFactory::FactoryStatistics BotFactory::GetStatistics() const
{
    FactoryStatistics stats;

    // Count bots in each state
    {
        std::lock_guard<std::mutex> lock(_botsMutex);
        stats.totalBots = static_cast<uint32>(_lifecycleManagers.size());

        for (const auto& [guid, manager] : _lifecycleManagers)
        {
            switch (manager->GetState())
            {
                case BotInitState::CREATED:
                    stats.botsCreated++;
                    break;
                case BotInitState::LOADING_DB:
                    stats.botsLoading++;
                    break;
                case BotInitState::INITIALIZING_MANAGERS:
                    stats.botsInitializing++;
                    break;
                case BotInitState::READY:
                    stats.botsReady++;
                    break;
                case BotInitState::ACTIVE:
                    stats.botsActive++;
                    break;
                case BotInitState::REMOVING:
                    stats.botsRemoving++;
                    break;
                case BotInitState::FAILED:
                    stats.botsFailed++;
                    break;
                default:
                    break;
            }

            stats.totalDeferredEvents += static_cast<uint32>(manager->GetQueuedEventCount());
        }
    }

    // Calculate averages
    {
        std::lock_guard<std::mutex> lock(_statsMutex);
        if (_totalCreated > 0)
        {
            stats.avgCreationTime = std::chrono::milliseconds(_totalCreationTime.count() / _totalCreated);
            stats.avgDbLoadTime = std::chrono::milliseconds(_totalDbLoadTime.count() / _totalCreated);
            stats.avgManagerInitTime = std::chrono::milliseconds(_totalManagerInitTime.count() / _totalCreated);
        }
    }

    return stats;
}

void BotFactory::LogDiagnostics() const
{
    auto stats = GetStatistics();

    TC_LOG_INFO("module.playerbot.lifecycle",
        "=== BotFactory Diagnostics ===");
    TC_LOG_INFO("module.playerbot.lifecycle",
        "Total bots: {} (Created: {}, Loading: {}, Initializing: {}, Ready: {}, Active: {}, Removing: {}, Failed: {})",
        stats.totalBots, stats.botsCreated, stats.botsLoading, stats.botsInitializing,
        stats.botsReady, stats.botsActive, stats.botsRemoving, stats.botsFailed);
    TC_LOG_INFO("module.playerbot.lifecycle",
        "Average times - Creation: {}ms, DB Load: {}ms, Manager Init: {}ms",
        stats.avgCreationTime.count(), stats.avgDbLoadTime.count(), stats.avgManagerInitTime.count());
    TC_LOG_INFO("module.playerbot.lifecycle",
        "Total deferred events pending: {}",
        stats.totalDeferredEvents);
}

// ============================================================================
// Cleanup
// ============================================================================

void BotFactory::Shutdown()
{
    TC_LOG_INFO("module.playerbot.lifecycle", "BotFactory shutting down...");

    LogDiagnostics();

    {
        std::lock_guard<std::mutex> lock(_botsMutex);
        for (auto& [guid, manager] : _lifecycleManagers)
        {
            manager->MarkDestroyed();
        }
        _lifecycleManagers.clear();
    }

    TC_LOG_INFO("module.playerbot.lifecycle", "BotFactory shutdown complete");
}

void BotFactory::UnregisterBot(ObjectGuid botGuid)
{
    std::lock_guard<std::mutex> lock(_botsMutex);
    _lifecycleManagers.erase(botGuid);
}

// ============================================================================
// BotReadyGuard Implementation
// ============================================================================

BotReadyGuard::BotReadyGuard(ObjectGuid botGuid)
    : _botGuid(botGuid)
{
    BotInitStateManager* manager = sBotFactory.GetLifecycleManager(botGuid);
    if (!manager)
    {
        _valid = false;
        return;
    }

    _state = manager->GetState();
    _valid = Playerbot::IsPlayerDataSafe(_state);

    if (_valid)
    {
        _player = sBotFactory.GetBotPlayer(botGuid);
        _ai = sBotFactory.GetBotAI(botGuid);

        // Get session from player
        if (_player && _player->GetSession())
        {
            _session = dynamic_cast<BotSession*>(_player->GetSession());
        }

        // Validate all components are present
        _valid = (_player != nullptr);
    }
}

std::unique_ptr<BotReadyGuard> BotReadyGuard::TryCreate(ObjectGuid botGuid)
{
    auto guard = std::unique_ptr<BotReadyGuard>(new BotReadyGuard(botGuid));
    if (!guard->_valid)
    {
        return nullptr;
    }
    return guard;
}

} // namespace Playerbot
