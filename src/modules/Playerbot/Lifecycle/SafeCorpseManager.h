/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * SafeCorpseManager: Thread-safe corpse lifecycle tracking for bots
 * Prevents premature corpse deletion during Map::SendObjectUpdates
 */

#pragma once

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include "ObjectGuid.h"
#include <memory>
#include <unordered_map>
#include <shared_mutex>
#include <atomic>
#include <chrono>

class Player;
class Corpse;

namespace Playerbot
{

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

class TC_GAME_API SafeCorpseManager
{
public:
    static SafeCorpseManager& Instance()
    {
        static SafeCorpseManager instance;
        return instance;
    }

    // Track corpse creation
    void RegisterCorpse(Player* bot, Corpse* corpse);

    // Mark corpse safe for deletion (after Map update cycle)
    void MarkCorpseSafeForDeletion(ObjectGuid corpseGuid);

    // Check if corpse can be safely deleted
    bool IsCorpseSafeToDelete(ObjectGuid corpseGuid) const;

    // Increment/decrement reference count
    void AddCorpseReference(ObjectGuid corpseGuid);
    void RemoveCorpseReference(ObjectGuid corpseGuid);

    // Get corpse location without accessing Corpse object
    bool GetCorpseLocation(ObjectGuid ownerGuid, float& x, float& y, float& z, uint32& mapId) const;

    // Cleanup old entries
    void CleanupExpiredCorpses();

    // Statistics
    uint32 GetTrackedCorpseCount() const { return _trackedCorpses.size(); }
    uint32 GetSafetyDelayedCount() const { return _safetyDelayedDeletions.load(); }

private:
    SafeCorpseManager() = default;
    ~SafeCorpseManager() = default;

    SafeCorpseManager(SafeCorpseManager const&) = delete;
    SafeCorpseManager& operator=(SafeCorpseManager const&) = delete;

    mutable Playerbot::OrderedSharedMutex<Playerbot::LockOrder::BOT_SPAWNER> _mutex;
    ::std::unordered_map<ObjectGuid, ::std::unique_ptr<CorpseTracker>> _trackedCorpses;
    ::std::unordered_map<ObjectGuid, ObjectGuid> _ownerToCorpse; // owner -> corpse mapping

    ::std::atomic<uint32> _safetyDelayedDeletions{0};
    static constexpr auto CORPSE_EXPIRY_TIME = ::std::chrono::minutes(30);
};

// RAII guard for corpse references during Map updates
class CorpseReferenceGuard
{
public:
    explicit CorpseReferenceGuard(ObjectGuid corpseGuid)
        : _corpseGuid(corpseGuid)
    {
        if (!_corpseGuid.IsEmpty())
            SafeCorpseManager::Instance().AddCorpseReference(_corpseGuid);
    }

    ~CorpseReferenceGuard()
    {
        if (!_corpseGuid.IsEmpty())
            SafeCorpseManager::Instance().RemoveCorpseReference(_corpseGuid);
    }

    CorpseReferenceGuard(CorpseReferenceGuard const&) = delete;
    CorpseReferenceGuard& operator=(CorpseReferenceGuard const&) = delete;

private:
    ObjectGuid _corpseGuid;
};

} // namespace Playerbot