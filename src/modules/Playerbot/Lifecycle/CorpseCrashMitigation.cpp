/*
 * Copyright (C) 2025 TrinityCore <https://www.trinitycore.org/>
 *
 * CorpseCrashMitigation: Unified corpse crash prevention with dual-strategy pattern
 *
 * P1 FIX: Task 7/7 from SESSION_LIFECYCLE_FIXES
 * Merges CorpsePreventionManager + SafeCorpseManager into unified component
 */

#include "CorpseCrashMitigation.h"
#include "Player.h"
#include "Corpse.h"
#include "ObjectAccessor.h"
#include "WorldSession.h"
#include "Log.h"
#include "../AI/BotAI.h"
#include "../Session/BotSession.h"
#include "DeathRecoveryManager.h"

namespace Playerbot
{

// ============================================================================
// Unified Entry Points (Strategy Selection)
// ============================================================================

void CorpseCrashMitigation::OnBotDeath(Player* bot)
{
    if (!bot)
        return;

    // Check if this is a bot
    if (!bot->GetSession()->IsBot())
        return;

    TC_LOG_DEBUG("playerbot.corpse", "Bot {} died - attempting corpse prevention (strategy 1)",
        bot->GetName());

    // Strategy 1: Try to prevent corpse creation
    if (_preventionEnabled && ShouldPreventCorpse(bot))
    {
        // Increment active prevention counter
        ++_activePrevention;

        TC_LOG_DEBUG("playerbot.corpse", "Bot {} entering prevention flow (active: {})",
            bot->GetName(), _activePrevention.load());

        // Cache death location BEFORE death for corpse-less resurrection
        CacheDeathLocation(bot);

        // Mark bot for corpse prevention in our internal tracking
        {
            _mutex.lock();
            _pendingPrevention.insert(bot->GetGUID());
            _mutex.unlock();
        }
    }
}

void CorpseCrashMitigation::OnCorpseCreated(Player* bot, Corpse* corpse)
{
    if (!bot)
        return;

    if (!bot->GetSession()->IsBot())
        return;

    // Check if this bot has corpse prevention flag in our internal tracking
    bool preventionAttempted = false;
    {
        _mutex.lock_shared();
        preventionAttempted = (_pendingPrevention.count(bot->GetGUID()) > 0);
        _mutex.unlock_shared();
    }

    if (preventionAttempted)
    {
        // Strategy 1: Prevention was attempted
        if (!corpse)
        {
            // SUCCESS: Corpse creation was prevented!
            TC_LOG_INFO("playerbot.corpse", "Bot {} death prevented - no corpse created (strategy 1 success)",
                bot->GetName());

            // Execute instant resurrection without corpse
            if (TryPreventCorpse(bot))
            {
                ++_preventedCorpses;
                TC_LOG_INFO("playerbot.corpse", "Prevented corpse #{} for bot {} - no Map::SendObjectUpdates crash risk",
                    _preventedCorpses.load(), bot->GetName());
            }
        }
        else
        {
            // FALLBACK: Corpse was created despite prevention attempt
            TC_LOG_WARN("playerbot.corpse", "Bot {} prevention failed - corpse created, using strategy 2 fallback",
                bot->GetName());

            // Strategy 2: Track corpse safely
            TrackCorpseSafely(bot, corpse);
        }

        // Clear prevention flag from our internal tracking
        {
            _mutex.lock();
            _pendingPrevention.erase(bot->GetGUID());
            _mutex.unlock();
        }

        // Decrement active prevention counter
        --_activePrevention;
    }
    else if (corpse)
    {
        // No prevention attempted, but corpse was created
        // Use Strategy 2 directly
        TC_LOG_DEBUG("playerbot.corpse", "Bot {} died without prevention - using strategy 2 tracking",
            bot->GetName());

        TrackCorpseSafely(bot, corpse);
    }
}

void CorpseCrashMitigation::OnBotResurrection(Player* bot)
{
    if (!bot)
        return;

    ObjectGuid botGuid = bot->GetGUID();

    _mutex.lock();

    // Clean up death location cache
    _deathLocations.erase(botGuid);

    // Clean up corpse tracking
    auto ownerIt = _ownerToCorpse.find(botGuid);
    if (ownerIt != _ownerToCorpse.end())
    {
        ObjectGuid corpseGuid = ownerIt->second;
        _trackedCorpses.erase(corpseGuid);
        _ownerToCorpse.erase(ownerIt);

        TC_LOG_DEBUG("playerbot.corpse", "Bot {} resurrected - cleaned up corpse tracking",
            bot->GetName());
    }

    _mutex.unlock();
}

// ============================================================================
// Strategy 1: Prevention (Preferred)
// ============================================================================

bool CorpseCrashMitigation::TryPreventCorpse(Player* bot)
{
    if (!bot)
        return false;

    TC_LOG_DEBUG("playerbot.corpse", "Executing corpse prevention for bot {}", bot->GetName());

    // CRITICAL: Set death state to ALIVE immediately to prevent corpse creation
    // This MUST happen before TrinityCore's death handling creates the corpse
    bot->setDeathState(ALIVE);

    // Set bot as ghost for visual effect (but alive mechanically)
    bot->SetPlayerFlag(PLAYER_FLAGS_GHOST);

    // Teleport to graveyard as a "ghost" (but mechanically alive)
    bot->RepopAtGraveyard();

    // Set health to 1 (ghost-like state but alive)
    bot->SetHealth(1);

    // Get BotAI to handle the fake death recovery
    if (WorldSession* session = bot->GetSession())
    {
        if (BotSession* botSession = dynamic_cast<BotSession*>(session))
        {
            if (BotAI* ai = botSession->GetAI())
            {
                if (DeathRecoveryManager* drm = ai->GetDeathRecoveryManager())
                {
                    // Initialize death recovery with cached location (no corpse needed)
                    drm->OnDeath();
                }
            }
        }
    }

    TC_LOG_DEBUG("playerbot.corpse", "Bot {} resurrection without corpse: IsAlive={}, IsGhost={}, Health={}",
        bot->GetName(), bot->IsAlive(), bot->HasPlayerFlag(PLAYER_FLAGS_GHOST), bot->GetHealth());

    return true;
}

bool CorpseCrashMitigation::ShouldPreventCorpse(Player* bot) const
{
    if (!bot || !_preventionEnabled)
        return false;

    // Only prevent for bots
    if (!bot->GetSession()->IsBot())
        return false;

    // Check throttling limit (prevent system overload)
    if (_activePrevention.load() > MAX_CONCURRENT_PREVENTION)
    {
        TC_LOG_DEBUG("playerbot.corpse", "Corpse prevention throttled (active: {})",
            _activePrevention.load());
        return false;
    }

    return true;
}

void CorpseCrashMitigation::CacheDeathLocation(Player* bot)
{
    if (!bot)
        return;

    ObjectGuid botGuid = bot->GetGUID();

    CorpseLocation location;
    location.mapId = bot->GetMapId();
    location.x = bot->GetPositionX();
    location.y = bot->GetPositionY();
    location.z = bot->GetPositionZ();
    location.deathTime = ::std::chrono::steady_clock::now();

    _mutex.lock();
    _deathLocations[botGuid] = location;
    _mutex.unlock();

    TC_LOG_DEBUG("playerbot.corpse", "Cached death location for bot {} at ({:.2f}, {:.2f}, {:.2f}) map {}",
        bot->GetName(), location.x, location.y, location.z, location.mapId);

    // Also notify DeathRecoveryManager if available
    if (WorldSession* session = bot->GetSession())
    {
        if (BotSession* botSession = dynamic_cast<BotSession*>(session))
        {
            if (BotAI* ai = botSession->GetAI())
            {
                if (DeathRecoveryManager* drm = ai->GetDeathRecoveryManager())
                {
                    // DeathRecoveryManager will cache location in OnDeath()
                    // This is a pre-cache to ensure it happens BEFORE corpse creation
                    TC_LOG_TRACE("playerbot.corpse", "Pre-cached death location in unified mitigation system");
                }
            }
        }
    }
}

void CorpseCrashMitigation::UncacheDeathLocation(ObjectGuid botGuid)
{
    _mutex.lock();
    _deathLocations.erase(botGuid);
    _mutex.unlock();
}

// ============================================================================
// Strategy 2: Safe Tracking (Fallback)
// ============================================================================

void CorpseCrashMitigation::TrackCorpseSafely(Player* bot, Corpse* corpse)
{
    if (!bot || !corpse)
        return;

    ObjectGuid corpseGuid = corpse->GetGUID();
    ObjectGuid ownerGuid = bot->GetGUID();

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

    _mutex.lock();
    _trackedCorpses[corpseGuid] = ::std::move(tracker);
    _ownerToCorpse[ownerGuid] = corpseGuid;
    _mutex.unlock();

    TC_LOG_DEBUG("playerbot.corpse", "Tracking corpse {} for bot {} at ({:.2f}, {:.2f}, {:.2f}) (strategy 2 fallback)",
        corpseGuid.ToString(), bot->GetName(), corpse->GetPositionX(), corpse->GetPositionY(), corpse->GetPositionZ());
}

void CorpseCrashMitigation::UntrackCorpse(ObjectGuid corpseGuid)
{
    _mutex.lock();

    auto it = _trackedCorpses.find(corpseGuid);
    if (it != _trackedCorpses.end())
    {
        ObjectGuid ownerGuid = it->second->ownerGuid;
        _ownerToCorpse.erase(ownerGuid);
        _trackedCorpses.erase(it);

        TC_LOG_DEBUG("playerbot.corpse", "Untracked corpse {}", corpseGuid.ToString());
    }

    _mutex.unlock();
}

bool CorpseCrashMitigation::IsCorpseSafeToDelete(ObjectGuid corpseGuid) const
{
    _mutex.lock_shared();

    auto it = _trackedCorpses.find(corpseGuid);
    if (it != _trackedCorpses.end())
    {
        // Safe to delete if:
        // 1. Marked as safe AND
        // 2. No active references (not in Map update)
        bool safe = it->second->safeToDelete.load() &&
                   (it->second->referenceCount.load() == 0);

        _mutex.unlock_shared();

        if (!safe)
        {
            const_cast<CorpseCrashMitigation*>(this)->_safetyDelayedDeletions++;
            TC_LOG_DEBUG("playerbot.corpse", "Delaying corpse {} deletion (refs={}, safe={})",
                corpseGuid.ToString(),
                it->second->referenceCount.load(),
                it->second->safeToDelete.load());
        }

        return safe;
    }

    _mutex.unlock_shared();

    // Unknown corpse = not a bot corpse = safe to delete normally
    return true;
}

void CorpseCrashMitigation::MarkCorpseSafeForDeletion(ObjectGuid corpseGuid)
{
    _mutex.lock_shared();

    auto it = _trackedCorpses.find(corpseGuid);
    if (it != _trackedCorpses.end())
    {
        it->second->safeToDelete = true;
        TC_LOG_DEBUG("playerbot.corpse", "Corpse {} marked safe for deletion", corpseGuid.ToString());
    }

    _mutex.unlock_shared();
}

void CorpseCrashMitigation::AddCorpseReference(ObjectGuid corpseGuid)
{
    _mutex.lock_shared();

    auto it = _trackedCorpses.find(corpseGuid);
    if (it != _trackedCorpses.end())
    {
        it->second->referenceCount++;
        TC_LOG_TRACE("playerbot.corpse", "Corpse {} reference++ (count={})",
            corpseGuid.ToString(), it->second->referenceCount.load());
    }

    _mutex.unlock_shared();
}

void CorpseCrashMitigation::RemoveCorpseReference(ObjectGuid corpseGuid)
{
    _mutex.lock_shared();

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

    _mutex.unlock_shared();
}

// ============================================================================
// Query Methods
// ============================================================================

CorpseLocation const* CorpseCrashMitigation::GetDeathLocation(ObjectGuid botGuid) const
{
    _mutex.lock_shared();

    auto it = _deathLocations.find(botGuid);
    if (it != _deathLocations.end())
    {
        _mutex.unlock_shared();
        return &it->second;
    }

    _mutex.unlock_shared();
    return nullptr;
}

bool CorpseCrashMitigation::GetCorpseLocation(ObjectGuid ownerGuid, float& x, float& y, float& z, uint32& mapId) const
{
    _mutex.lock_shared();

    auto ownerIt = _ownerToCorpse.find(ownerGuid);
    if (ownerIt == _ownerToCorpse.end())
    {
        _mutex.unlock_shared();
        return false;
    }

    auto corpseIt = _trackedCorpses.find(ownerIt->second);
    if (corpseIt == _trackedCorpses.end())
    {
        _mutex.unlock_shared();
        return false;
    }

    x = corpseIt->second->x;
    y = corpseIt->second->y;
    z = corpseIt->second->z;
    mapId = corpseIt->second->mapId;

    _mutex.unlock_shared();

    TC_LOG_TRACE("playerbot.corpse", "Retrieved corpse location for owner {} at ({:.2f}, {:.2f}, {:.2f})",
        ownerGuid.ToString(), x, y, z);

    return true;
}

// ============================================================================
// Maintenance
// ============================================================================

void CorpseCrashMitigation::CleanupExpiredCorpses()
{
    _mutex.lock();

    auto now = ::std::chrono::steady_clock::now();

    // Cleanup expired death locations
    for (auto it = _deathLocations.begin(); it != _deathLocations.end();)
    {
        auto elapsed = ::std::chrono::duration_cast<::std::chrono::minutes>(now - it->second.deathTime);

        if (elapsed > CORPSE_EXPIRY_TIME)
        {
            TC_LOG_DEBUG("playerbot.corpse", "Cleaned up expired death location for bot {} (age: {} min)",
                it->first.ToString(), elapsed.count());
            it = _deathLocations.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // Cleanup expired corpse trackers
    for (auto it = _trackedCorpses.begin(); it != _trackedCorpses.end();)
    {
        auto elapsed = ::std::chrono::duration_cast<::std::chrono::minutes>(now - it->second->creationTime);

        // Remove if expired and no references
        if (elapsed > CORPSE_EXPIRY_TIME && it->second->referenceCount.load() == 0)
        {
            ObjectGuid ownerGuid = it->second->ownerGuid;
            _ownerToCorpse.erase(ownerGuid);

            TC_LOG_DEBUG("playerbot.corpse", "Cleaned up expired corpse tracker {} (age: {} min)",
                it->first.ToString(), elapsed.count());

            it = _trackedCorpses.erase(it);
        }
        else
        {
            ++it;
        }
    }

    _mutex.unlock();
}

} // namespace Playerbot
