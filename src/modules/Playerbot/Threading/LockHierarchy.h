/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include "Log.h"

namespace Playerbot
{

/**
 * @brief Global lock ordering hierarchy
 *
 * CRITICAL RULE: Locks MUST be acquired in ascending order
 * Violating this order causes runtime errors in debug builds
 *
 * Design Pattern: Hierarchical Lock Ordering
 * - Prevents circular wait conditions
 * - Enforces consistent lock acquisition order across all threads
 * - Runtime validation in debug, production safety through testing
 */
enum class LockOrder : uint32
{
    // Layer 1: Infrastructure (acquired first)
    // These are foundational systems that never depend on higher layers
    LOG_SYSTEM = 100,
    CONFIG_MANAGER = 200,
    METRICS_COLLECTOR = 300,
    EVENT_BUS = 400,

    // Layer 2: Core data structures
    // Read-mostly data structures with minimal dependencies
    SPATIAL_GRID = 1000,
    OBJECT_CACHE = 1100,
    PLAYER_SNAPSHOT_BUFFER = 1200,

    // Layer 3: Session management
    // Network layer and session state
    SESSION_MANAGER = 2000,
    PACKET_QUEUE = 2100,
    PACKET_RELAY = 2200,

    // Layer 4: Bot lifecycle
    // Bot spawning, despawning, and state transitions
    BOT_SPAWNER = 3000,
    BOT_SCHEDULER = 3100,
    DEATH_RECOVERY = 3200,

    // Layer 5: Bot AI
    // AI decision-making and behavior execution
    BOT_AI = 4000,
    BOT_AI_STATE = 4050,
    BEHAVIOR_MANAGER = 4100,
    ACTION_PRIORITY = 4200,

    // Layer 6: Combat systems
    // Combat coordination and targeting
    THREAT_COORDINATOR = 5000,
    INTERRUPT_COORDINATOR = 5100,
    DISPEL_COORDINATOR = 5200,
    TARGET_SELECTOR = 5300,

    // Layer 7: Group/Raid coordination
    // Group management and role assignment
    GROUP_MANAGER = 6000,
    QUEUE_MONITOR = 6050,        // Queue state polling for JIT bot creation
    RAID_COORDINATOR = 6100,
    ROLE_ASSIGNMENT = 6200,

    // Layer 8: Movement and pathfinding
    // Movement arbitration and path calculation
    MOVEMENT_ARBITER = 7000,
    PATHFINDING_ADAPTER = 7100,
    FORMATION_MANAGER = 7200,

    // Layer 9: Game systems
    // Quest, loot, trade, professions
    QUEST_MANAGER = 8000,
    LOOT_MANAGER = 8100,
    TRADE_MANAGER = 8200,
    PROFESSION_MANAGER = 8300,

    // Layer 10: Database operations
    // Database connections and query execution
    DATABASE_POOL = 9000,
    DATABASE_TRANSACTION = 9100,

    // Layer 11: External dependencies (acquired last)
    // TrinityCore systems - we never control their internal locking
    TRINITYCORE_MAP = 10000,
    TRINITYCORE_WORLD = 10100,
    TRINITYCORE_OBJECTMGR = 10200,

    MAX_LOCK_ORDER = 20000
};

/**
 * @brief Thread-local lock tracking
 *
 * Maintains a stack of currently held locks per thread
 * Used by OrderedMutex to detect lock ordering violations
 */
class ThreadLocalLockTracker
{
public:
    /**
     * @brief Record lock acquisition
     * @param order Lock order value
     */
    static void PushLock(uint32 order)
    {
        GetLockStack().push_back(order);
    }

    /**
     * @brief Record lock release
     * @param order Lock order value (for validation)
     */
    static void PopLock(uint32 order)
    {
        auto& stack = GetLockStack();

        // Validate LIFO order (last lock acquired should be first released)
        if (!stack.empty() && stack.back() == order)
        {
            stack.pop_back();
        }
        else
        {
            TC_LOG_ERROR("playerbot.deadlock",
                "Lock release order violation! Attempting to release lock {} "
                "but last acquired lock is {}",
                order, stack.empty() ? 0 : stack.back());
        }
    }

    /**
     * @brief Get highest lock order currently held by this thread
     * @return Maximum lock order, or 0 if no locks held
     */
    static uint32 GetMaxLockOrder()
    {
        auto const& stack = GetLockStack();
        if (stack.empty())
            return 0;

        return *::std::max_element(stack.begin(), stack.end());
    }

    /**
     * @brief Check if thread currently holds any locks
     * @return True if at least one lock is held
     */
    static bool HasActiveLocks()
    {
        return !GetLockStack().empty();
    }

    /**
     * @brief Get number of locks currently held
     * @return Lock count
     */
    static size_t GetLockCount()
    {
        return GetLockStack().size();
    }

private:
    /**
     * @brief Get thread-local lock stack
     * @return Reference to thread's lock stack
     */
    static ::std::vector<uint32>& GetLockStack()
    {
        thread_local ::std::vector<uint32> lockStack;
        return lockStack;
    }
};

/**
 * @brief Lock wrapper that enforces ordering at runtime
 *
 * Usage:
 *   OrderedMutex<LockOrder::SPATIAL_GRID> _gridMutex;
 *
 *   void UpdateGrid()
 *   {
 *       std::lock_guard<decltype(_gridMutex)> lock(_gridMutex);
 *       // ... critical section
 *   }
 *
 * Benefits:
 *   - Compile-time lock order declaration
 *   - Runtime validation in debug builds
 *   - Zero overhead in release builds (can be optimized out)
 *   - Compatible with std::lock_guard, std::unique_lock
 */
template<LockOrder Order>
class OrderedMutex
{
public:
    OrderedMutex() = default;

    // Non-copyable, non-movable (like std::mutex)
    OrderedMutex(OrderedMutex const&) = delete;
    OrderedMutex& operator=(OrderedMutex const&) = delete;
    OrderedMutex(OrderedMutex&&) = delete;
    OrderedMutex& operator=(OrderedMutex&&) = delete;

    /**
     * @brief Acquire lock with ordering validation
     *
     * In debug builds, checks if lock ordering is violated
     * Throws exception if violation detected (prevents deadlock)
     */
    void lock()
    {
#ifdef _DEBUG
        // Check if current thread holds locks with higher or equal order
        uint32 currentMaxOrder = ThreadLocalLockTracker::GetMaxLockOrder();

        if (currentMaxOrder >= static_cast<uint32>(Order))
        {
            // DEADLOCK DETECTED: Attempting to acquire lock out of order
            TC_LOG_FATAL("playerbot.deadlock",
                "Lock ordering violation! Current thread holds lock with order {}, "
                "attempting to acquire lock with order {}. This WILL cause deadlock! "
                "Thread has {} active locks.",
                currentMaxOrder, static_cast<uint32>(Order),
                ThreadLocalLockTracker::GetLockCount());

#ifdef _MSC_VER
            // Trigger breakpoint in MSVC debugger
            __debugbreak();
#elif defined(__GNUC__) || defined(__clang__)
            // Trigger breakpoint in GCC/Clang debugger
            __builtin_trap();
#endif

            // Throw exception to prevent deadlock
            throw ::std::runtime_error("Lock ordering violation detected");
        }
#endif

        // Acquire the actual mutex
        _mutex.lock();

        // Track this lock acquisition
        ThreadLocalLockTracker::PushLock(static_cast<uint32>(Order));
    }

    /**
     * @brief Release lock
     */
    void unlock()
    {
        // Untrack this lock
        ThreadLocalLockTracker::PopLock(static_cast<uint32>(Order));

        // Release the actual mutex
        _mutex.unlock();
    }

    /**
     * @brief Try to acquire lock without blocking
     * @return True if lock was acquired, false otherwise
     */
    bool try_lock()
    {
        if (_mutex.try_lock())
        {
            ThreadLocalLockTracker::PushLock(static_cast<uint32>(Order));
            return true;
        }
        return false;
    }

    /**
     * @brief Get lock order for this mutex
     * @return Lock order value
     */
    static constexpr uint32 GetOrder()
    {
        return static_cast<uint32>(Order);
    }

private:
    ::std::mutex _mutex;
};

/**
 * @brief Helper for acquiring multiple locks in correct order
 *
 * Usage:
 *   OrderedMutex<LockOrder::SPATIAL_GRID> _gridLock;
 *   OrderedMutex<LockOrder::SESSION_MANAGER> _sessionLock;
 *
 *   MultiLockGuard guard(_gridLock, _sessionLock);
 *   // Both locks acquired in correct order automatically
 *
 * Note: This is a simple implementation. For complex scenarios,
 *       consider std::scoped_lock (C++17) with OrderedMutex.
 */
template<typename... Mutexes>
class MultiLockGuard
{
public:
    explicit MultiLockGuard(Mutexes&... mutexes)
        : _mutexes(::std::tie(mutexes...))
    {
        LockAll(::std::index_sequence_for<Mutexes...>{});
    }

    ~MultiLockGuard()
    {
        UnlockAll(::std::index_sequence_for<Mutexes...>{});
    }

    MultiLockGuard(MultiLockGuard const&) = delete;
    MultiLockGuard& operator=(MultiLockGuard const&) = delete;

private:
    template<size_t... Is>
    void LockAll(::std::index_sequence<Is...>)
    {
        // Lock in parameter order (should already be in correct order)
        (..., ::std::get<Is>(_mutexes).lock());
    }

    template<size_t... Is>
    void UnlockAll(::std::index_sequence<Is...>)
    {
        // Unlock in reverse order
        (..., ::std::get<sizeof...(Is) - 1 - Is>(_mutexes).unlock());
    }

    ::std::tuple<Mutexes&...> _mutexes;
};

/**
 * @brief Shared mutex wrapper with lock ordering enforcement
 *
 * Similar to OrderedMutex but supports shared (read) locks
 * Use for read-mostly data structures
 */
template<LockOrder Order>
class OrderedSharedMutex
{
public:
    OrderedSharedMutex() = default;

    // Non-copyable, non-movable
    OrderedSharedMutex(OrderedSharedMutex const&) = delete;
    OrderedSharedMutex& operator=(OrderedSharedMutex const&) = delete;
    OrderedSharedMutex(OrderedSharedMutex&&) = delete;
    OrderedSharedMutex& operator=(OrderedSharedMutex&&) = delete;

    /**
     * @brief Acquire exclusive (write) lock
     */
    void lock()
    {
#ifdef _DEBUG
        uint32 currentMaxOrder = ThreadLocalLockTracker::GetMaxLockOrder();

        if (currentMaxOrder >= static_cast<uint32>(Order))
        {
            TC_LOG_FATAL("playerbot.deadlock",
                "Shared lock ordering violation! Current thread holds lock with order {}, "
                "attempting to acquire exclusive lock with order {}. This WILL cause deadlock!",
                currentMaxOrder, static_cast<uint32>(Order));

#ifdef _MSC_VER
            __debugbreak();
#elif defined(__GNUC__) || defined(__clang__)
            __builtin_trap();
#endif

            throw ::std::runtime_error("Shared lock ordering violation detected");
        }
#endif

        _mutex.lock();
        ThreadLocalLockTracker::PushLock(static_cast<uint32>(Order));
    }

    void unlock()
    {
        ThreadLocalLockTracker::PopLock(static_cast<uint32>(Order));
        _mutex.unlock();
    }

    bool try_lock()
    {
        if (_mutex.try_lock())
        {
            ThreadLocalLockTracker::PushLock(static_cast<uint32>(Order));
            return true;
        }
        return false;
    }

    /**
     * @brief Acquire shared (read) lock
     */
    void lock_shared()
    {
#ifdef _DEBUG
        uint32 currentMaxOrder = ThreadLocalLockTracker::GetMaxLockOrder();

        if (currentMaxOrder >= static_cast<uint32>(Order))
        {
            TC_LOG_FATAL("playerbot.deadlock",
                "Shared lock ordering violation! Current thread holds lock with order {}, "
                "attempting to acquire shared lock with order {}. This WILL cause deadlock!",
                currentMaxOrder, static_cast<uint32>(Order));

#ifdef _MSC_VER
            __debugbreak();
#elif defined(__GNUC__) || defined(__clang__)
            __builtin_trap();
#endif

            throw ::std::runtime_error("Shared lock ordering violation detected");
        }
#endif

        _mutex.lock_shared();
        ThreadLocalLockTracker::PushLock(static_cast<uint32>(Order));
    }

    void unlock_shared()
    {
        ThreadLocalLockTracker::PopLock(static_cast<uint32>(Order));
        _mutex.unlock_shared();
    }

    bool try_lock_shared()
    {
        if (_mutex.try_lock_shared())
        {
            ThreadLocalLockTracker::PushLock(static_cast<uint32>(Order));
            return true;
        }
        return false;
    }

    static constexpr uint32 GetOrder()
    {
        return static_cast<uint32>(Order);
    }

private:
    ::std::shared_mutex _mutex;
};

/**
 * @brief Recursive mutex wrapper with lock ordering enforcement
 *
 * Allows same thread to acquire lock multiple times
 * Use sparingly - usually indicates design issue
 */
template<LockOrder Order>
class OrderedRecursiveMutex
{
public:
    OrderedRecursiveMutex() = default;

    // Non-copyable, non-movable
    OrderedRecursiveMutex(OrderedRecursiveMutex const&) = delete;
    OrderedRecursiveMutex& operator=(OrderedRecursiveMutex const&) = delete;
    OrderedRecursiveMutex(OrderedRecursiveMutex&&) = delete;
    OrderedRecursiveMutex& operator=(OrderedRecursiveMutex&&) = delete;

    void lock()
    {
#ifdef _DEBUG
        // For recursive mutex, only check on first acquisition
        if (_lockCount.load() == 0)
        {
            uint32 currentMaxOrder = ThreadLocalLockTracker::GetMaxLockOrder();

            if (currentMaxOrder >= static_cast<uint32>(Order))
            {
                TC_LOG_FATAL("playerbot.deadlock",
                    "Recursive lock ordering violation! Current thread holds lock with order {}, "
                    "attempting to acquire recursive lock with order {}. This WILL cause deadlock!",
                    currentMaxOrder, static_cast<uint32>(Order));

#ifdef _MSC_VER
                __debugbreak();
#elif defined(__GNUC__) || defined(__clang__)
                __builtin_trap();
#endif

                throw ::std::runtime_error("Recursive lock ordering violation detected");
            }
        }
#endif

        _mutex.lock();

        // Only track first acquisition
        if (_lockCount.fetch_add(1, ::std::memory_order_relaxed) == 0)
        {
            ThreadLocalLockTracker::PushLock(static_cast<uint32>(Order));
        }
    }

    void unlock()
    {
        // Only untrack final release
        if (_lockCount.fetch_sub(1, ::std::memory_order_relaxed) == 1)
        {
            ThreadLocalLockTracker::PopLock(static_cast<uint32>(Order));
        }

        _mutex.unlock();
    }

    bool try_lock()
    {
        if (_mutex.try_lock())
        {
            if (_lockCount.fetch_add(1, ::std::memory_order_relaxed) == 0)
            {
                ThreadLocalLockTracker::PushLock(static_cast<uint32>(Order));
            }
            return true;
        }
        return false;
    }

    static constexpr uint32 GetOrder()
    {
        return static_cast<uint32>(Order);
    }

private:
    ::std::recursive_mutex _mutex;
    ::std::atomic<uint32> _lockCount{0};
};

} // namespace Playerbot
