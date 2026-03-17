/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * CorpseCrashMitigation: Unified corpse crash prevention with dual-strategy pattern
 *
 * This component merges CorpsePreventionManager and SafeCorpseManager into a single
 * unified system that uses two complementary strategies:
 *
 * Strategy 1 (Prevention): Try to prevent corpse creation entirely by immediately
 *                          resurrecting the bot as a "ghost" without creating a Corpse object.
 *                          This eliminates Map::SendObjectUpdates crashes by avoiding the
 *                          race condition entirely.
 *
 * Strategy 2 (Safe Tracking): If prevention fails and a corpse is created, track it
 *                             safely with reference counting to prevent premature deletion
 *                             during Map update cycles.
 *
 * P1 FIX: Task 7/7 from SESSION_LIFECYCLE_FIXES
 */

#pragma once

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include "ObjectGuid.h"
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <shared_mutex>
#include <atomic>
#include <chrono>

class Player;
class Corpse;

namespace Playerbot
{

/**
 * @struct CorpseLocation
 * @brief Cached death location for corpse-less resurrection
 */
struct CorpseLocation
{
    uint32 mapId;
    float x, y, z;
    ::std::chrono::steady_clock::time_point deathTime;
};

/**
 * @struct CorpseTracker
 * @brief Thread-safe corpse tracking for fallback strategy
 */
struct CorpseTracker
{
    ObjectGuid corpseGuid;
    ObjectGuid ownerGuid;
    uint32 mapId;
    float x, y, z;
    ::std::chrono::steady_clock::time_point creationTime;
    ::std::atomic<bool> safeToDelete{false};
    ::std::atomic<uint32> referenceCount{1};
};

/**
 * @class CorpseCrashMitigation
 * @brief Unified corpse crash prevention with dual-strategy pattern
 *
 * This singleton manages bot death and corpse lifecycle with two complementary strategies:
 *
 * 1. **Prevention Strategy** (Preferred):
 *    - Prevents Corpse object creation by immediately setting bot to ALIVE
 *    - Bot is teleported to graveyard as a "ghost" (visually) but ALIVE (mechanically)
 *    - Eliminates Map::SendObjectUpdates crashes entirely
 *    - Tracked via _preventedCorpses counter
 *
 * 2. **Safe Tracking Strategy** (Fallback):
 *    - If corpse is created despite prevention, track it with reference counting
 *    - Prevents premature deletion during Map update cycles
 *    - Uses RAII guards (CorpseReferenceGuard) for safe map iteration
 *    - Tracked via _trackedCorpses counter
 *
 * Thread Safety:
 * - All public methods are fully thread-safe
 * - Uses shared_mutex for read-heavy operations
 * - Atomic counters for statistics and state
 */
class TC_GAME_API CorpseCrashMitigation
{
public:
    static CorpseCrashMitigation& Instance()
    {
        static CorpseCrashMitigation instance;
        return instance;
    }

    // ========================================================================
    // Unified Entry Points (Strategy Selection)
    // ========================================================================

    /**
     * @brief Called when bot is about to die
     * @param bot The dying bot
     *
     * Attempts prevention strategy first. If enabled, marks bot for corpse prevention
     * and caches death location.
     */
    void OnBotDeath(Player* bot);

    /**
     * @brief Called after bot death (if corpse was created)
     * @param bot The dead bot
     * @param corpse The created corpse (may be nullptr if prevention succeeded)
     *
     * If prevention succeeded, executes instant resurrection.
     * If prevention failed, registers corpse for safe tracking (fallback strategy).
     */
    void OnCorpseCreated(Player* bot, Corpse* corpse);

    /**
     * @brief Called when bot is resurrected
     * @param bot The resurrected bot
     *
     * Cleans up tracked corpse data and death locations.
     */
    void OnBotResurrection(Player* bot);

    // ========================================================================
    // Strategy 1: Prevention (Preferred)
    // ========================================================================

    /**
     * @brief Try to prevent corpse creation by instant resurrection
     * @param bot The bot to resurrect without corpse
     * @return true if prevention succeeded, false if corpse will be created
     *
     * Sets bot to ALIVE state, teleports to graveyard as ghost, sets health to 1.
     * This prevents TrinityCore from creating a Corpse object.
     */
    bool TryPreventCorpse(Player* bot);

    /**
     * @brief Check if bot should skip corpse creation
     * @param bot The bot to check
     * @return true if prevention should be attempted
     *
     * Checks if prevention is enabled and throttling limits are OK.
     */
    bool ShouldPreventCorpse(Player* bot) const;

    // ========================================================================
    // Strategy 2: Safe Tracking (Fallback)
    // ========================================================================

    /**
     * @brief Register corpse for safe tracking (fallback if prevention failed)
     * @param bot The bot that died
     * @param corpse The created corpse
     *
     * Caches corpse location and initializes reference counting for safe deletion.
     */
    void TrackCorpseSafely(Player* bot, Corpse* corpse);

    /**
     * @brief Check if corpse can be safely deleted
     * @param corpseGuid The corpse GUID
     * @return true if safe to delete (no active references and marked safe)
     *
     * Used by Map deletion logic to prevent premature deletion during updates.
     */
    bool IsCorpseSafeToDelete(ObjectGuid corpseGuid) const;

    /**
     * @brief Mark corpse safe for deletion (after Map update cycle)
     * @param corpseGuid The corpse GUID
     *
     * Sets safeToDelete flag. Corpse can only be deleted if no active references.
     */
    void MarkCorpseSafeForDeletion(ObjectGuid corpseGuid);

    /**
     * @brief Increment corpse reference count (Map is accessing it)
     * @param corpseGuid The corpse GUID
     */
    void AddCorpseReference(ObjectGuid corpseGuid);

    /**
     * @brief Decrement corpse reference count (Map finished accessing)
     * @param corpseGuid The corpse GUID
     */
    void RemoveCorpseReference(ObjectGuid corpseGuid);

    // ========================================================================
    // Query Methods
    // ========================================================================

    /**
     * @brief Get cached death location
     * @param botGuid The bot's GUID
     * @return Pointer to cached location, or nullptr if not found
     *
     * Used for corpse-less resurrection (prevention strategy).
     */
    CorpseLocation const* GetDeathLocation(ObjectGuid botGuid) const;

    /**
     * @brief Get corpse location without accessing Corpse object
     * @param ownerGuid The bot's GUID
     * @param x, y, z, mapId Output parameters for location
     * @return true if location was found
     *
     * Safe alternative to Corpse->GetPosition() during Map updates.
     */
    bool GetCorpseLocation(ObjectGuid ownerGuid, float& x, float& y, float& z, uint32& mapId) const;

    // ========================================================================
    // Configuration
    // ========================================================================

    /**
     * @brief Enable/disable prevention strategy
     * @param enabled If false, always uses safe tracking (no prevention attempts)
     *
     * Useful for debugging or disabling prevention if issues occur.
     */
    void SetPreventionEnabled(bool enabled) { _preventionEnabled = enabled; }

    /**
     * @brief Check if prevention strategy is enabled
     * @return true if prevention attempts are enabled
     */
    bool IsPreventionEnabled() const { return _preventionEnabled; }

    // ========================================================================
    // Maintenance
    // ========================================================================

    /**
     * @brief Remove expired corpse trackers (called periodically)
     *
     * Removes trackers older than 30 minutes with no active references.
     */
    void CleanupExpiredCorpses();

    // ========================================================================
    // Statistics
    // ========================================================================

    /**
     * @brief Get count of corpses prevented (strategy 1 successes)
     * @return Number of times corpse creation was prevented
     */
    uint32 GetPreventedCorpses() const { return _preventedCorpses.load(); }

    /**
     * @brief Get count of corpses tracked (strategy 2 fallbacks)
     * @return Number of corpses currently tracked
     */
    uint32 GetTrackedCorpses() const { return _trackedCorpses.size(); }

    /**
     * @brief Get count of safety-delayed deletions
     * @return Number of times deletion was delayed due to active references
     */
    uint32 GetSafetyDelayedCount() const { return _safetyDelayedDeletions.load(); }

    /**
     * @brief Get count of active prevention operations
     * @return Number of bots currently in prevention flow
     */
    uint32 GetActivePreventionCount() const { return _activePrevention.load(); }

private:
    CorpseCrashMitigation() = default;
    ~CorpseCrashMitigation() = default;

    CorpseCrashMitigation(CorpseCrashMitigation const&) = delete;
    CorpseCrashMitigation& operator=(CorpseCrashMitigation const&) = delete;

    // Strategy 1: Prevention helpers
    void CacheDeathLocation(Player* bot);
    void UncacheDeathLocation(ObjectGuid botGuid);

    // Strategy 2: Safe tracking helpers
    void UntrackCorpse(ObjectGuid corpseGuid);

    // Unified data structures
    mutable Playerbot::OrderedSharedMutex<Playerbot::LockOrder::BOT_SPAWNER> _mutex;

    // Strategy 1: Death location cache (for corpse-less resurrection)
    ::std::unordered_map<ObjectGuid, CorpseLocation> _deathLocations;

    // Strategy 1: Pending prevention tracking (internal flag replacement)
    ::std::unordered_set<ObjectGuid> _pendingPrevention;

    // Strategy 2: Corpse tracking (fallback if prevention fails)
    ::std::unordered_map<ObjectGuid, ::std::unique_ptr<CorpseTracker>> _trackedCorpses;
    ::std::unordered_map<ObjectGuid, ObjectGuid> _ownerToCorpse; // owner -> corpse mapping

    // Configuration
    ::std::atomic<bool> _preventionEnabled{true};

    // Statistics
    ::std::atomic<uint32> _preventedCorpses{0};      // Strategy 1 successes
    ::std::atomic<uint32> _activePrevention{0};      // Currently in prevention flow
    ::std::atomic<uint32> _safetyDelayedDeletions{0}; // Strategy 2 safety delays

    // Constants
    static constexpr auto CORPSE_EXPIRY_TIME = ::std::chrono::minutes(30);
    static constexpr uint32 MAX_CONCURRENT_PREVENTION = 10; // Throttling limit
};

/**
 * @class CorpseReferenceGuard
 * @brief RAII guard for corpse references during Map updates
 *
 * Automatically increments reference count on construction and decrements on destruction.
 * Ensures corpse cannot be deleted while Map is iterating over it.
 *
 * Usage:
 * @code
 * {
 *     CorpseReferenceGuard guard(corpseGuid);
 *     // Safe to access corpse - it cannot be deleted
 *     corpse->DoSomething();
 * } // Guard destructor decrements reference, allowing deletion
 * @endcode
 */
class CorpseReferenceGuard
{
public:
    explicit CorpseReferenceGuard(ObjectGuid corpseGuid)
        : _corpseGuid(corpseGuid)
    {
        if (!_corpseGuid.IsEmpty())
            CorpseCrashMitigation::Instance().AddCorpseReference(_corpseGuid);
    }

    ~CorpseReferenceGuard()
    {
        if (!_corpseGuid.IsEmpty())
            CorpseCrashMitigation::Instance().RemoveCorpseReference(_corpseGuid);
    }

    CorpseReferenceGuard(CorpseReferenceGuard const&) = delete;
    CorpseReferenceGuard& operator=(CorpseReferenceGuard const&) = delete;

private:
    ObjectGuid _corpseGuid;
};

// Singleton accessor
#define sCorpseCrashMitigation CorpseCrashMitigation::Instance()

} // namespace Playerbot
