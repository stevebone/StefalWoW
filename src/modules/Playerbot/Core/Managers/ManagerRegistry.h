/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PLAYERBOT_MANAGERREGISTRY_H
#define PLAYERBOT_MANAGERREGISTRY_H

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include "IManagerBase.h"
#include <unordered_map>
#include <vector>
#include <mutex>
#include <memory>
#include <string>

namespace Playerbot
{

/**
 * @brief Manager lifecycle and coordination system for Phase 7
 *
 * The ManagerRegistry manages the lifecycle of all manager instances for a bot,
 * coordinating initialization, updates, and shutdown. It provides centralized
 * access to managers and ensures proper ordering of operations.
 *
 * Architecture:
 * - One ManagerRegistry per BotAI instance
 * - Owns manager lifecycles via smart pointers
 * - Coordinates manager updates with throttling
 * - Provides manager lookup by ID
 *
 * Lifecycle Management:
 * 1. Register managers during BotAI construction
 * 2. Initialize() all managers when bot spawns
 * 3. Update() managers with throttling during world updates
 * 4. Shutdown() all managers when bot despawns
 *
 * Thread Safety:
 * - Register/Unregister use mutex for thread safety
 * - Update() should be called from single thread (world update thread)
 * - GetManager() uses shared lock for concurrent read access
 *
 * Usage Example:
 * @code
 * // In BotAI constructor:
 * _managerRegistry = std::make_unique<ManagerRegistry>();
 * _managerRegistry->RegisterManager(std::move(_questManager));
 * _managerRegistry->RegisterManager(std::move(_tradeManager));
 * _managerRegistry->RegisterManager(std::move(_socialManager));
 *
 * // In BotAI::Initialize():
 * _managerRegistry->InitializeAll();
 *
 * // In BotAI::Update():
 * _managerRegistry->UpdateAll(diff);
 *
 * // In BotAI destructor:
 * _managerRegistry->ShutdownAll();
 * @endcode
 */
class TC_GAME_API ManagerRegistry
{
public:
    /**
     * @brief Construct manager registry
     */
    ManagerRegistry();

    /**
     * @brief Destructor - ensures all managers are shut down
     */
    ~ManagerRegistry();

    /**
     * @brief Register a manager with the registry
     *
     * Transfers ownership of the manager to the registry.
     * The manager must have a unique ID (GetManagerId()).
     *
     * @param manager Unique pointer to the manager (ownership transferred)
     * @return true if registration succeeded, false if ID already exists
     *
     * Thread Safety: Uses mutex, safe to call from any thread
     * Performance: O(1) average case
     *
     * Note: Managers should be registered during BotAI construction,
     * before Initialize() is called.
     */
    bool RegisterManager(::std::unique_ptr<IManagerBase> manager);

    /**
     * @brief Unregister a manager by ID
     *
     * Shuts down the manager and removes it from the registry.
     * The manager is destroyed after shutdown completes.
     *
     * @param managerId The manager's unique identifier
     * @return true if manager was found and unregistered, false otherwise
     *
     * Thread Safety: Uses mutex, safe to call from any thread
     * Performance: O(1) average case
     */
    bool UnregisterManager(::std::string const& managerId);

    /**
     * @brief Get a manager by ID
     *
     * Returns a raw pointer to the manager for access.
     * The registry retains ownership of the manager.
     *
     * @param managerId The manager's unique identifier
     * @return Pointer to manager, or nullptr if not found
     *
     * Thread Safety: Uses shared lock, safe for concurrent read access
     * Performance: O(1) average case
     *
     * Note: The returned pointer is valid until the manager is unregistered
     * or the registry is destroyed. Do not store this pointer long-term.
     */
    IManagerBase* GetManager(::std::string const& managerId) const;

    /**
     * @brief Check if a manager is registered
     *
     * @param managerId The manager's unique identifier
     * @return true if manager is registered, false otherwise
     *
     * Thread Safety: Uses shared lock, safe for concurrent read access
     * Performance: O(1) average case
     */
    bool HasManager(::std::string const& managerId) const;

    /**
     * @brief Initialize all registered managers
     *
     * Calls Initialize() on all managers in registration order.
     * If any manager fails to initialize, it is logged but initialization continues.
     *
     * @return Number of managers successfully initialized
     *
     * Thread Safety: Should be called from single thread during bot initialization
     * Performance: Sum of all manager Initialize() times (<100ms total)
     */
    uint32 InitializeAll();

    /**
     * @brief Shutdown all registered managers
     *
     * Calls Shutdown() on all managers in reverse registration order
     * (to respect dependency ordering).
     *
     * Thread Safety: Should be called from single thread during bot cleanup
     * Performance: Sum of all manager Shutdown() times (<500ms total)
     */
    void ShutdownAll();

    /**
     * @brief Update all registered managers
     *
     * Calls Update() on managers that are due for update based on their
     * GetUpdateInterval(). Uses throttling to avoid updating managers
     * too frequently.
     *
     * @param diff Time elapsed since last update in milliseconds
     * @return Number of managers actually updated this cycle
     *
     * Thread Safety: Should be called from single thread (world update thread)
     * Performance: <0.1ms total for all managers per update
     *
     * Throttling Algorithm:
     * - Track last update time per manager
     * - Only update if (currentTime - lastUpdateTime) >= GetUpdateInterval()
     * - Prevents managers from consuming too much CPU
     */
    uint32 UpdateAll(uint32 diff);

    /**
     * @brief Get the number of registered managers
     *
     * @return Number of managers in the registry
     *
     * Thread Safety: Uses shared lock, safe for concurrent read access
     * Performance: O(1)
     */
    size_t GetManagerCount() const;

    /**
     * @brief Get list of all manager IDs
     *
     * Useful for debugging and monitoring.
     *
     * @return Vector of manager IDs
     *
     * Thread Safety: Uses shared lock, safe for concurrent read access
     * Performance: O(n) where n is number of managers
     */
    ::std::vector<::std::string> GetManagerIds() const;

    /**
     * @brief Set a manager's active state
     *
     * Inactive managers skip Update() and event processing.
     * Useful for temporarily disabling managers.
     *
     * @param managerId The manager's unique identifier
     * @param active true to activate, false to deactivate
     * @return true if manager was found and state changed, false otherwise
     *
     * Thread Safety: Uses mutex, safe to call from any thread
     * Performance: O(1) average case
     */
    bool SetManagerActive(::std::string const& managerId, bool active);

    /**
     * @brief Get performance metrics for all managers
     *
     * Returns statistics about manager update times for monitoring.
     *
     * @return struct containing metrics per manager
     *
     * Thread Safety: Uses shared lock, safe for concurrent read access
     */
    struct ManagerMetrics
    {
        ::std::string managerId;
        uint32 updateInterval;
        uint64 lastUpdateTime;
        uint64 totalUpdates;
        uint64 totalUpdateTimeMs;
        float averageUpdateTimeMs;
        bool isActive;
    };
    ::std::vector<ManagerMetrics> GetMetrics() const;

    /**
     * @brief Reset all manager metrics
     *
     * Clears accumulated statistics for fresh measurements.
     *
     * Thread Safety: Uses mutex, safe to call from any thread
     */
    void ResetMetrics();

private:
    /**
     * @brief Manager entry with metadata
     */
    struct ManagerEntry
    {
        ::std::unique_ptr<IManagerBase> manager;
        uint64 lastUpdateTime;
        uint64 totalUpdates;
        uint64 totalUpdateTimeMs;
        bool initialized;

        ManagerEntry()
            : lastUpdateTime(0)
            , totalUpdates(0)
            , totalUpdateTimeMs(0)
            , initialized(false)
        {}
    };

    /**
     * @brief Manager map: manager ID -> entry
     */
    ::std::unordered_map<::std::string, ManagerEntry> _managers;

    /**
     * @brief Manager initialization order (manager IDs)
     *
     * Preserved to ensure managers are initialized in registration order
     * and shut down in reverse order.
     */
    ::std::vector<::std::string> _initializationOrder;

    /**
     * @brief Mutex protecting the manager map
     */
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BEHAVIOR_MANAGER> _managerMutex;

    /**
     * @brief Flag indicating whether InitializeAll() has been called
     */
    bool _initialized;

    /**
     * @brief Disable copy construction
     */
    ManagerRegistry(ManagerRegistry const&) = delete;

    /**
     * @brief Disable copy assignment
     */
    ManagerRegistry& operator=(ManagerRegistry const&) = delete;
};

} // namespace Playerbot

#endif // PLAYERBOT_MANAGERREGISTRY_H
