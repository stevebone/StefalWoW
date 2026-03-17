/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * SafeCorpseManager: Thread-safe corpse lifecycle tracking for bots
 */

#include "SafeCorpseManager.h"
#include "Player.h"
#include "Corpse.h"
#include "ObjectAccessor.h"
#include "Log.h"

namespace Playerbot
{

void SafeCorpseManager::RegisterCorpse(Player* bot, Corpse* corpse)
{
    if (!bot || !corpse)
        return;

    ObjectGuid corpseGuid = corpse->GetGUID();
    ObjectGuid ownerGuid = bot->GetGUID();
    ::std::unique_lock<::std::shared_mutex> lock(_mutex);

    // Create tracker
    auto tracker = ::std::make_unique<CorpseTracker>();
    tracker->corpseGuid = corpseGuid;
    tracker->ownerGuid = ownerGuid;
    tracker->mapId = corpse->GetMapId();
    tracker->x = corpse->GetPositionX();
    tracker->y = corpse->GetPositionY();
    tracker->z = corpse->GetPositionZ();
    tracker->creationTime = ::std::chrono::steady_clock::now();
    tracker->safeToDelete = false; // NOT safe until Map update completes
    tracker->referenceCount = 1;
    _trackedCorpses[corpseGuid] = ::std::move(tracker);
    _ownerToCorpse[ownerGuid] = corpseGuid;

    TC_LOG_DEBUG("playerbot.corpse", "Registered corpse {} for bot {} at ({:.2f}, {:.2f}, {:.2f})",
        corpseGuid.ToString(), bot->GetName(), tracker->x, tracker->y, tracker->z);
}

void SafeCorpseManager::MarkCorpseSafeForDeletion(ObjectGuid corpseGuid)
{
    ::std::shared_lock<::std::shared_mutex> lock(_mutex);

    auto it = _trackedCorpses.find(corpseGuid);
    if (it != _trackedCorpses.end())
    {
        it->second->safeToDelete = true;
        TC_LOG_DEBUG("playerbot.corpse", "Corpse {} marked safe for deletion", corpseGuid.ToString());
    }
}

bool SafeCorpseManager::IsCorpseSafeToDelete(ObjectGuid corpseGuid) const
{
    ::std::shared_lock<::std::shared_mutex> lock(_mutex);

    auto it = _trackedCorpses.find(corpseGuid);
    if (it != _trackedCorpses.end())
    {
        // Safe to delete if:
        // 1. Marked as safe AND
        // 2. No active references (not in Map update)
        bool safe = it->second->safeToDelete.load() &&
                   (it->second->referenceCount.load() == 0);

        if (!safe)
        {
            const_cast<SafeCorpseManager*>(this)->_safetyDelayedDeletions++;
            TC_LOG_DEBUG("playerbot.corpse", "Delaying corpse {} deletion (refs={}, safe={})",
                corpseGuid.ToString(),
                it->second->referenceCount.load(),
                it->second->safeToDelete.load());
        }

        return safe;
    }

    // Unknown corpse = not a bot corpse = safe to delete normally
    return true;
}

void SafeCorpseManager::AddCorpseReference(ObjectGuid corpseGuid)
{
    ::std::shared_lock<::std::shared_mutex> lock(_mutex);

    auto it = _trackedCorpses.find(corpseGuid);
    if (it != _trackedCorpses.end())
    {
        it->second->referenceCount++;
        TC_LOG_TRACE("playerbot.corpse", "Corpse {} reference++ (count={})",
            corpseGuid.ToString(), it->second->referenceCount.load());
    }
}

void SafeCorpseManager::RemoveCorpseReference(ObjectGuid corpseGuid)
{
    ::std::shared_lock<::std::shared_mutex> lock(_mutex);

    auto it = _trackedCorpses.find(corpseGuid);
    if (it != _trackedCorpses.end())
    {
        uint32 refs = --it->second->referenceCount;
        TC_LOG_TRACE("playerbot.corpse", "Corpse {} reference-- (count={})",
            corpseGuid.ToString(), refs);

        // If no more references and marked safe, it can now be deleted
    if (refs == 0 && it->second->safeToDelete.load())
        {
            TC_LOG_DEBUG("playerbot.corpse", "Corpse {} now safe for deletion (no references)",
                corpseGuid.ToString());
        }
    }
}

bool SafeCorpseManager::GetCorpseLocation(ObjectGuid ownerGuid, float& x, float& y, float& z, uint32& mapId) const
{
    ::std::shared_lock<::std::shared_mutex> lock(_mutex);

    auto ownerIt = _ownerToCorpse.find(ownerGuid);
    if (ownerIt == _ownerToCorpse.end())
        return false;

    auto corpseIt = _trackedCorpses.find(ownerIt->second);
    if (corpseIt == _trackedCorpses.end())
        return false;

    x = corpseIt->second->x;
    y = corpseIt->second->y;
    z = corpseIt->second->z;
    mapId = corpseIt->second->mapId;

    TC_LOG_TRACE("playerbot.corpse", "Retrieved corpse location for owner {} at ({:.2f}, {:.2f}, {:.2f})",
        ownerGuid.ToString(), x, y, z);

    return true;
}

void SafeCorpseManager::CleanupExpiredCorpses()
{
    ::std::unique_lock<::std::shared_mutex> lock(_mutex);

    auto now = ::std::chrono::steady_clock::now();

    for (auto it = _trackedCorpses.begin(); it != _trackedCorpses.end();)
    {
        auto elapsed = ::std::chrono::duration_cast<::std::chrono::minutes>(now - it->second->creationTime);

        // Remove if expired and no references
    if (elapsed > CORPSE_EXPIRY_TIME && it->second->referenceCount.load() == 0)
        {
            ObjectGuid ownerGuid = it->second->ownerGuid;
            _ownerToCorpse.erase(ownerGuid);

            TC_LOG_DEBUG("playerbot.corpse", "Cleaned up expired corpse {} (age: {} min)",
                it->first.ToString(), elapsed.count());

            it = _trackedCorpses.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

} // namespace Playerbot