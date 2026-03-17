/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 */

#include "LazyManagerFactory.h"
#include "Social/TradeManager.h"
#include "Professions/GatheringManager.h"
#include "Economy/AuctionManager.h"
#include "Advanced/GroupCoordinator.h"
#include "Lifecycle/DeathRecoveryManager.h"
#include "AI/BotAI.h"
#include "Player.h"
#include "Log.h"
#include "Timer.h"
#include <chrono>

namespace Playerbot
{

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

LazyManagerFactory::LazyManagerFactory(Player* bot, BotAI* ai)
    : _bot(bot), _ai(ai)
{
    // CRITICAL: Do NOT access _bot->GetName() or _bot->GetGUID() in constructor!
    // Bot may not be fully in world yet during BotAI construction,
    // and Player::m_name/m_guid are not initialized, causing ACCESS_VIOLATION.
    // Logging deferred to lazy getter methods when bot is IsInWorld().
}
LazyManagerFactory::~LazyManagerFactory()
{
    ShutdownAll();
    // CRITICAL: Do NOT call _bot->GetName() in destructor!
    // During destruction, _bot may be in invalid state where GetName() returns
    // garbage data, causing ACCESS_VIOLATION or std::bad_alloc.
    // Only log manager statistics without bot identity.
    TC_LOG_DEBUG("module.playerbot.lazy", "LazyManagerFactory destroyed - {} managers initialized, total init time: {}ms",
                 _initCount.load(),
                 _totalInitTime.count());
}

// ============================================================================
// LAZY MANAGER GETTERS - Double-checked locking pattern
// ============================================================================

TradeManager* LazyManagerFactory::GetTradeManager()
{
    return GetOrCreate<TradeManager>(
        _tradeManager,
        _tradeManagerInit,
        [this]() -> ::std::unique_ptr<TradeManager> {
            auto start = ::std::chrono::steady_clock::now();
            TC_LOG_DEBUG("module.playerbot.lazy", "Creating TradeManager for bot {}", _bot->GetName());
            auto manager = ::std::make_unique<TradeManager>(_bot, _ai);
            if (!manager->Initialize())
            {
                TC_LOG_ERROR("module.playerbot.lazy", "Failed to initialize TradeManager for bot {}", _bot->GetName());
                return nullptr;
            }

            auto duration = ::std::chrono::duration_cast<::std::chrono::milliseconds>(
                ::std::chrono::steady_clock::now() - start);

            RecordInitTime("TradeManager", duration);
            TC_LOG_INFO("module.playerbot.lazy", " TradeManager created for bot {} in {}ms", _bot->GetName(), duration.count());

            return manager;
        }
    );
}

GatheringManager* LazyManagerFactory::GetGatheringManager()
{
    return GetOrCreate<GatheringManager>(
        _gatheringManager,
        _gatheringManagerInit,
        [this]() -> ::std::unique_ptr<GatheringManager> {
            auto start = ::std::chrono::steady_clock::now();
            TC_LOG_DEBUG("module.playerbot.lazy", "Creating GatheringManager for bot {}", _bot->GetName());
            auto manager = ::std::make_unique<GatheringManager>(_bot, _ai);
            if (!manager->Initialize())
            {
                TC_LOG_ERROR("module.playerbot.lazy", "Failed to initialize GatheringManager for bot {}", _bot->GetName());
                return nullptr;
            }

            auto duration = ::std::chrono::duration_cast<::std::chrono::milliseconds>(
                ::std::chrono::steady_clock::now() - start);
            RecordInitTime("GatheringManager", duration);
            TC_LOG_INFO("module.playerbot.lazy", " GatheringManager created for bot {} in {}ms", _bot->GetName(), duration.count());

            return manager;
        }
    );
}

AuctionManager* LazyManagerFactory::GetAuctionManager()
{
    return GetOrCreate<AuctionManager>(
        _auctionManager,
        _auctionManagerInit,
        [this]() -> ::std::unique_ptr<AuctionManager> {
            auto start = ::std::chrono::steady_clock::now();
            TC_LOG_DEBUG("module.playerbot.lazy", "Creating AuctionManager for bot {}", _bot->GetName());
            auto manager = ::std::make_unique<AuctionManager>(_bot, _ai);
            if (!manager->Initialize())
            {
                TC_LOG_ERROR("module.playerbot.lazy", "Failed to initialize AuctionManager for bot {}", _bot->GetName());
                return nullptr;
            }

            auto duration = ::std::chrono::duration_cast<::std::chrono::milliseconds>(
                ::std::chrono::steady_clock::now() - start);

            RecordInitTime("AuctionManager", duration);
            TC_LOG_INFO("module.playerbot.lazy", " AuctionManager created for bot {} in {}ms", _bot->GetName(), duration.count());

            return manager;
        }
    );
}

GroupCoordinator* LazyManagerFactory::GetGroupCoordinator()
{
    return GetOrCreate<GroupCoordinator>(
        _groupCoordinator,
        _groupCoordinatorInit,
        [this]() -> ::std::unique_ptr<GroupCoordinator> {
            auto start = ::std::chrono::steady_clock::now();
            TC_LOG_DEBUG("module.playerbot.lazy", "Creating GroupCoordinator for bot {}", _bot->GetName());
            auto manager = ::std::make_unique<GroupCoordinator>(_bot, _ai);
            // Initialize() returns void, just call it
            manager->Initialize();

            auto duration = ::std::chrono::duration_cast<::std::chrono::milliseconds>(
                ::std::chrono::steady_clock::now() - start);

            RecordInitTime("GroupCoordinator", duration);
            TC_LOG_INFO("module.playerbot.lazy", " GroupCoordinator created for bot {} in {}ms", _bot->GetName(), duration.count());

            return manager;
        }
    );
}

DeathRecoveryManager* LazyManagerFactory::GetDeathRecoveryManager()
{
    return GetOrCreate<DeathRecoveryManager>(
        _deathRecoveryManager,
        _deathRecoveryManagerInit,
        [this]() -> ::std::unique_ptr<DeathRecoveryManager> {
            auto start = ::std::chrono::steady_clock::now();
            TC_LOG_DEBUG("module.playerbot.lazy", "Creating DeathRecoveryManager for bot {}", _bot->GetName());
            auto manager = ::std::make_unique<DeathRecoveryManager>(_bot, _ai);
            // DeathRecoveryManager doesn't have Initialize() method - ready after construction

            auto duration = ::std::chrono::duration_cast<::std::chrono::milliseconds>(
                ::std::chrono::steady_clock::now() - start);

            RecordInitTime("DeathRecoveryManager", duration);
            TC_LOG_INFO("module.playerbot.lazy", " DeathRecoveryManager created for bot {} in {}ms", _bot->GetName(), duration.count());

            return manager;
        }
    );
}

// ============================================================================
// GENERIC LAZY INITIALIZATION TEMPLATE
// ============================================================================

template<typename T>
T* LazyManagerFactory::GetOrCreate(
    ::std::unique_ptr<T>& manager,
    ::std::atomic<bool>& flag,
    ::std::function<::std::unique_ptr<T>()> factory)
{
    // Fast path: Check if already initialized (lock-free)
    if (flag.load(::std::memory_order_acquire))
    {
        return manager.get();
    }

    // Slow path: Need to create manager
    {
        ::std::unique_lock lock(_mutex);

        // Double-check after acquiring lock (another thread may have created it)
    if (flag.load(::std::memory_order_relaxed))
        {
            return manager.get();
        }

        // Create manager via factory function
        try
        {
            manager = factory();
            if (!manager)
            {
                TC_LOG_ERROR("module.playerbot.lazy", "Factory function returned null manager for bot {}",
                             _bot ? _bot->GetName() : "Unknown");
                return nullptr;
            }

            // Mark as initialized (memory_order_release ensures all writes are visible)
            flag.store(true, ::std::memory_order_release);
            _initCount.fetch_add(1, ::std::memory_order_relaxed);

            return manager.get();
        }
        catch (::std::exception const& e)
        {
            TC_LOG_ERROR("module.playerbot.lazy", "Exception creating manager for bot {}: {}",
                         _bot ? _bot->GetName() : "Unknown", e.what());
            return nullptr;
        }
        catch (...)
        {
            TC_LOG_ERROR("module.playerbot.lazy", "Unknown exception creating manager for bot {}",
                         _bot ? _bot->GetName() : "Unknown");
            return nullptr;
        }
    }
}

// ============================================================================
// STATE QUERIES
// ============================================================================

template<>
bool LazyManagerFactory::IsInitialized<TradeManager>() const
{
    return _tradeManagerInit.load(::std::memory_order_acquire);
}

template<>
bool LazyManagerFactory::IsInitialized<GatheringManager>() const
{
    return _gatheringManagerInit.load(::std::memory_order_acquire);
}

template<>
bool LazyManagerFactory::IsInitialized<AuctionManager>() const
{
    return _auctionManagerInit.load(::std::memory_order_acquire);
}

template<>
bool LazyManagerFactory::IsInitialized<GroupCoordinator>() const
{
    return _groupCoordinatorInit.load(::std::memory_order_acquire);
}

template<>
bool LazyManagerFactory::IsInitialized<DeathRecoveryManager>() const
{
    return _deathRecoveryManagerInit.load(::std::memory_order_acquire);
}

size_t LazyManagerFactory::GetInitializedCount() const
{
    return _initCount.load(::std::memory_order_relaxed);
}

::std::chrono::milliseconds LazyManagerFactory::GetTotalInitTime() const
{
    ::std::lock_guard lock(_metricsMutex);
    return _totalInitTime;
}
// ============================================================================
// LIFECYCLE MANAGEMENT
// ============================================================================

void LazyManagerFactory::Update(uint32 diff)
{
    // Only update managers that have been initialized (zero overhead for uninit)
    ::std::shared_lock lock(_mutex);

    if (_tradeManager)
        _tradeManager->Update(diff);

    if (_gatheringManager)
        _gatheringManager->Update(diff);

    if (_auctionManager)
        _auctionManager->Update(diff);

    if (_groupCoordinator)
        _groupCoordinator->Update(diff);

    if (_deathRecoveryManager)
        _deathRecoveryManager->Update(diff);
}

void LazyManagerFactory::ShutdownAll()
{
    ::std::unique_lock lock(_mutex);

    TC_LOG_DEBUG("module.playerbot.lazy", "Shutting down {} managers for bot {}",
                 _initCount.load(), _bot ? _bot->GetName() : "Unknown");

    if (_tradeManager)
    {
        _tradeManager->Shutdown();
        _tradeManager.reset();
        TC_LOG_DEBUG("module.playerbot.lazy", "TradeManager shutdown complete");
    }

    if (_gatheringManager)
    {
        _gatheringManager->Shutdown();
        _gatheringManager.reset();
        TC_LOG_DEBUG("module.playerbot.lazy", "GatheringManager shutdown complete");
    }

    if (_auctionManager)
    {
        _auctionManager->Shutdown();
        _auctionManager.reset();
        TC_LOG_DEBUG("module.playerbot.lazy", "AuctionManager shutdown complete");
    }

    if (_groupCoordinator)
    {
        _groupCoordinator->Shutdown();
        _groupCoordinator.reset();
        TC_LOG_DEBUG("module.playerbot.lazy", "GroupCoordinator shutdown complete");
    }

    if (_deathRecoveryManager)
    {
        // DeathRecoveryManager doesn't have Shutdown() method - cleanup via reset()
        _deathRecoveryManager.reset();
        TC_LOG_DEBUG("module.playerbot.lazy", "DeathRecoveryManager shutdown complete");
    }

    // Reset flags
    _tradeManagerInit.store(false, ::std::memory_order_release);
    _gatheringManagerInit.store(false, ::std::memory_order_release);
    _auctionManagerInit.store(false, ::std::memory_order_release);
    _groupCoordinatorInit.store(false, ::std::memory_order_release);
    _deathRecoveryManagerInit.store(false, ::std::memory_order_release);
    _initCount.store(0, ::std::memory_order_release);
}

void LazyManagerFactory::InitializeAll()
{
    TC_LOG_WARN("module.playerbot.lazy", "Force-initializing ALL managers for bot {} - this defeats lazy initialization!",
                _bot ? _bot->GetName() : "Unknown");

    auto start = ::std::chrono::steady_clock::now();

    GetTradeManager();
    GetGatheringManager();
    GetAuctionManager();
    GetGroupCoordinator();
    GetDeathRecoveryManager();

    auto duration = ::std::chrono::duration_cast<::std::chrono::milliseconds>(
        ::std::chrono::steady_clock::now() - start);

    TC_LOG_INFO("module.playerbot.lazy", "All managers initialized for bot {} in {}ms (lazy init would be instant)",
                _bot ? _bot->GetName() : "Unknown", duration.count());
}

// ============================================================================
// INTERNAL IMPLEMENTATION
// ============================================================================

void LazyManagerFactory::RecordInitTime(::std::string const& managerName, ::std::chrono::milliseconds duration)
{
    ::std::lock_guard lock(_metricsMutex);
    _totalInitTime += duration;

    TC_LOG_DEBUG("module.playerbot.lazy", "{} initialization time: {}ms (total: {}ms)",
                 managerName, duration.count(), _totalInitTime.count());
}

// Explicit template instantiations for all manager types
template TradeManager* LazyManagerFactory::GetOrCreate(
    ::std::unique_ptr<TradeManager>&,
    ::std::atomic<bool>&,
    ::std::function<::std::unique_ptr<TradeManager>()>);

template GatheringManager* LazyManagerFactory::GetOrCreate(
    ::std::unique_ptr<GatheringManager>&,
    ::std::atomic<bool>&,
    ::std::function<::std::unique_ptr<GatheringManager>()>);

template AuctionManager* LazyManagerFactory::GetOrCreate(
    ::std::unique_ptr<AuctionManager>&,
    ::std::atomic<bool>&,
    ::std::function<::std::unique_ptr<AuctionManager>()>);

template GroupCoordinator* LazyManagerFactory::GetOrCreate(
    ::std::unique_ptr<GroupCoordinator>&,
    ::std::atomic<bool>&,
    ::std::function<::std::unique_ptr<GroupCoordinator>()>);

template DeathRecoveryManager* LazyManagerFactory::GetOrCreate(
    ::std::unique_ptr<DeathRecoveryManager>&,
    ::std::atomic<bool>&,
    ::std::function<::std::unique_ptr<DeathRecoveryManager>()>);

} // namespace Playerbot
