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

#include "ManagerRegistry.h"
#include "GameTime.h"
#include "Log.h"
#include "Timer.h"
#include <algorithm>

namespace Playerbot
{

ManagerRegistry::ManagerRegistry()
    : _initialized(false)
{
    TC_LOG_DEBUG("module.playerbot.managers",
        "ManagerRegistry created");
}

ManagerRegistry::~ManagerRegistry()
{
    // Ensure all managers are shut down
    if (_initialized)
    {
        ShutdownAll();
    }

    TC_LOG_DEBUG("module.playerbot.managers",
        "ManagerRegistry destroyed with {} managers",
        _managers.size());
}

bool ManagerRegistry::RegisterManager(::std::unique_ptr<IManagerBase> manager)
{

    ::std::string managerId = manager->GetManagerId();

    ::std::lock_guard lock(_managerMutex);

    // Check if manager ID already exists
    if (_managers.find(managerId) != _managers.end())
    {
        TC_LOG_ERROR("module.playerbot.managers",
            "Manager ID '{}' already registered", managerId);
        return false;
    }

    // Create entry and transfer ownership
    ManagerEntry entry;
    entry.manager = ::std::move(manager);
    entry.initialized = false;
    entry.lastUpdateTime = 0;
    entry.totalUpdates = 0;
    entry.totalUpdateTimeMs = 0;

    _managers[managerId] = ::std::move(entry);
    _initializationOrder.push_back(managerId);

    TC_LOG_INFO("module.playerbot.managers",
        "Manager '{}' registered (total managers: {})",
        managerId,
        _managers.size());

    return true;
}

bool ManagerRegistry::UnregisterManager(::std::string const& managerId)
{
    ::std::lock_guard lock(_managerMutex);

    auto it = _managers.find(managerId);
    if (it == _managers.end())
    {
        TC_LOG_WARN("module.playerbot.managers",
            "Attempted to unregister non-existent manager '{}'", managerId);
        return false;
    }

    // Shutdown manager if initialized
    if (it->second.initialized && it->second.manager)
    {
        try
        {
            it->second.manager->Shutdown();
        }
        catch (::std::exception const& ex)
        {
            TC_LOG_ERROR("module.playerbot.managers",
                "Exception during shutdown of manager '{}': {}",
                managerId, ex.what());
        }
    }

    // Remove from initialization order
    auto orderIt = ::std::find(_initializationOrder.begin(), _initializationOrder.end(), managerId);
    if (orderIt != _initializationOrder.end())
    {
        _initializationOrder.erase(orderIt);
    }

    // Remove from map (destroys manager)
    _managers.erase(it);

    TC_LOG_INFO("module.playerbot.managers",
        "Manager '{}' unregistered (remaining managers: {})",
        managerId,
        _managers.size());

    return true;
}

IManagerBase* ManagerRegistry::GetManager(::std::string const& managerId) const
{
    ::std::lock_guard lock(_managerMutex);

    auto it = _managers.find(managerId);
    if (it == _managers.end())
        return nullptr;

    return it->second.manager.get();
}

bool ManagerRegistry::HasManager(::std::string const& managerId) const
{
    ::std::lock_guard lock(_managerMutex);
    return _managers.find(managerId) != _managers.end();
}

uint32 ManagerRegistry::InitializeAll()
{
    ::std::lock_guard lock(_managerMutex);

    uint32 successCount = 0;
    uint64 startTime = GameTime::GetGameTimeMS();

    TC_LOG_INFO("module.playerbot.managers",
        "Initializing {} managers...", _managers.size());

    // Initialize in registration order
    for (auto const& managerId : _initializationOrder)
    {
        auto it = _managers.find(managerId);
        if (it == _managers.end())
            continue;

        auto& entry = it->second;
        if (entry.initialized)
        {
            TC_LOG_WARN("module.playerbot.managers",
                "Manager '{}' already initialized", managerId);
            continue;
        }

        try
        {
            uint64 managerStartTime = GameTime::GetGameTimeMS();

            if (entry.manager->Initialize())
            {
                entry.initialized = true;
                ++successCount;

                uint64 initTime = getMSTimeDiff(managerStartTime, GameTime::GetGameTimeMS());
                TC_LOG_DEBUG("module.playerbot.managers",
                    "Manager '{}' initialized in {}ms", managerId, initTime);

                if (initTime > 10) // Warn if initialization took >10ms
                {
                    TC_LOG_WARN("module.playerbot.managers",
                        "Manager '{}' took {}ms to initialize (expected <10ms)",
                        managerId, initTime);
                }
            }
            else
            {
                TC_LOG_ERROR("module.playerbot.managers",
                    "Manager '{}' failed to initialize", managerId);
            }
        }
        catch (::std::exception const& ex)
        {
            TC_LOG_ERROR("module.playerbot.managers",
                "Exception initializing manager '{}': {}", managerId, ex.what());
        }
    }

    uint64 totalTime = getMSTimeDiff(startTime, GameTime::GetGameTimeMS());
    _initialized = true;

    TC_LOG_INFO("module.playerbot.managers",
        "Initialized {}/{} managers in {}ms",
        successCount, _managers.size(), totalTime);

    return successCount;
}

void ManagerRegistry::ShutdownAll()
{
    ::std::lock_guard lock(_managerMutex);

    uint64 startTime = GameTime::GetGameTimeMS();

    TC_LOG_INFO("module.playerbot.managers",
        "Shutting down {} managers...", _managers.size());

    // Shutdown in reverse order (to respect dependencies)
    for (auto it = _initializationOrder.rbegin(); it != _initializationOrder.rend(); ++it)
    {
        ::std::string const& managerId = *it;
        auto managerIt = _managers.find(managerId);

        if (managerIt == _managers.end())
            continue;

        auto& entry = managerIt->second;
        if (!entry.initialized || !entry.manager)
            continue;

        try
        {
            uint64 managerStartTime = GameTime::GetGameTimeMS();

            entry.manager->Shutdown();
            entry.initialized = false;

            uint64 shutdownTime = getMSTimeDiff(managerStartTime, GameTime::GetGameTimeMS());
            TC_LOG_DEBUG("module.playerbot.managers",
                "Manager '{}' shut down in {}ms", managerId, shutdownTime);

            if (shutdownTime > 50) // Warn if shutdown took >50ms
            {
                TC_LOG_WARN("module.playerbot.managers",
                    "Manager '{}' took {}ms to shut down (expected <50ms)",
                    managerId, shutdownTime);
            }
        }
        catch (::std::exception const& ex)
        {
            TC_LOG_ERROR("module.playerbot.managers",
                "Exception shutting down manager '{}': {}", managerId, ex.what());
        }
    }

    uint64 totalTime = getMSTimeDiff(startTime, GameTime::GetGameTimeMS());
    _initialized = false;

    TC_LOG_INFO("module.playerbot.managers",
        "All managers shut down in {}ms", totalTime);
}

uint32 ManagerRegistry::UpdateAll(uint32 diff)
{
    // PERFORMANCE FIX: Removed global lock that was serializing all bot updates
    // Each bot has its own ManagerRegistry instance, so _managers is per-bot data
    // No cross-bot access means no lock needed for UpdateAll()
    // See: CORRECTED_RUNTIME_BOTTLENECK_ANALYSIS.md for details
    if (!_initialized)
        return 0;

    uint32 updateCount = 0;
    uint64 currentTime = GameTime::GetGameTimeMS();

    // Update managers that are due for update
    for (auto& [managerId, entry] : _managers)
    {
        if (!entry.initialized || !entry.manager)
            continue;

        // Skip inactive managers
    if (!entry.manager->IsActive())
            continue;

        // Check if manager is due for update
        uint32 updateInterval = entry.manager->GetUpdateInterval();
        uint64 timeSinceLastUpdate = currentTime - entry.lastUpdateTime;

        if (timeSinceLastUpdate < updateInterval)
            continue;

        // Update manager
        try
        {
            uint64 updateStartTime = GameTime::GetGameTimeMS();

            entry.manager->Update(diff);

            uint64 updateTime = getMSTimeDiff(updateStartTime, GameTime::GetGameTimeMS());
            entry.lastUpdateTime = currentTime;
            entry.totalUpdates++;
            entry.totalUpdateTimeMs += updateTime;
            ++updateCount;

            // Warn if update took too long
    if (updateTime > 1) // >1ms is concerning
            {
                TC_LOG_WARN("module.playerbot.managers",
                    "Manager '{}' update took {}ms (expected <1ms)",
                    managerId, updateTime);
            }
        }
        catch (::std::exception const& ex)
        {
            TC_LOG_ERROR("module.playerbot.managers",
                "Exception updating manager '{}': {}", managerId, ex.what());
        }
    }

    return updateCount;
}

size_t ManagerRegistry::GetManagerCount() const
{
    ::std::lock_guard lock(_managerMutex);
    return _managers.size();
}

::std::vector<::std::string> ManagerRegistry::GetManagerIds() const
{
    ::std::lock_guard lock(_managerMutex);

    ::std::vector<::std::string> ids;
    ids.reserve(_managers.size());

    for (auto const& [managerId, entry] : _managers)
    {
        ids.push_back(managerId);
    }

    return ids;
}

bool ManagerRegistry::SetManagerActive(::std::string const& managerId, bool active)
{
    ::std::lock_guard lock(_managerMutex);

    auto it = _managers.find(managerId);
    if (it == _managers.end())
        return false;

    // Note: IManagerBase::IsActive() is const, so we can't directly set it
    // Managers need to implement their own active state management
    // This method is a placeholder for future active state control

    TC_LOG_DEBUG("module.playerbot.managers",
        "Manager '{}' active state change requested to {}",
        managerId, active);

    return true;
}

::std::vector<ManagerRegistry::ManagerMetrics> ManagerRegistry::GetMetrics() const
{
    ::std::lock_guard lock(_managerMutex);

    ::std::vector<ManagerMetrics> metrics;
    metrics.reserve(_managers.size());

    for (auto const& [managerId, entry] : _managers)
    {
        if (!entry.manager)
            continue;

        ManagerMetrics m;
        m.managerId = managerId;
        m.updateInterval = entry.manager->GetUpdateInterval();
        m.lastUpdateTime = entry.lastUpdateTime;
        m.totalUpdates = entry.totalUpdates;
        m.totalUpdateTimeMs = entry.totalUpdateTimeMs;
        m.isActive = entry.manager->IsActive();

        if (entry.totalUpdates > 0)
        {
            m.averageUpdateTimeMs =
                static_cast<float>(entry.totalUpdateTimeMs) / entry.totalUpdates;
        }
        else
        {
            m.averageUpdateTimeMs = 0.0f;
        }

        metrics.push_back(m);
    }

    return metrics;
}

void ManagerRegistry::ResetMetrics()
{
    ::std::lock_guard lock(_managerMutex);

    for (auto& [managerId, entry] : _managers)
    {
        entry.totalUpdates = 0;
        entry.totalUpdateTimeMs = 0;
    }

    TC_LOG_DEBUG("module.playerbot.managers",
        "Manager metrics reset for {} managers", _managers.size());
}

} // namespace Playerbot
