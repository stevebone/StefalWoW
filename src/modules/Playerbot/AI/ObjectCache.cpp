/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * ObjectCache Implementation - Fix #19 for ObjectAccessor Deadlock
 */

#include "ObjectCache.h"
#include "Player.h"
#include "Unit.h"
#include "Creature.h"
#include "GameObject.h"
#include "Object.h"
#include "Group.h"
#include "ObjectAccessor.h"
#include "Log.h"

namespace Playerbot
{

// ============================================================================
// FIX #22: PASSIVE CACHE POPULATION - Zero ObjectAccessor calls
// ============================================================================

void ObjectCache::SetTarget(::Unit* target)
{
    _cachedTarget = target;
    _targetGuid = target ? target->GetGUID() : ObjectGuid::Empty;
    _stats.totalRefreshes++;
}

void ObjectCache::SetGroupLeader(Player* leader)
{
    _cachedGroupLeader = leader;
    _groupLeaderGuid = leader ? leader->GetGUID() : ObjectGuid::Empty;
}

void ObjectCache::SetGroupMembers(::std::vector<Player*> const& members)
{
    _cachedGroupMembers = members;
    _groupMemberGuids.clear();
    _groupMemberGuids.reserve(members.size());
    for (Player* member : members)
    {
        if (member)
            _groupMemberGuids.push_back(member->GetGUID());
    }
}

void ObjectCache::SetFollowTarget(::Unit* followTarget)
{
    _cachedFollowTarget = followTarget;
    _followTargetGuid = followTarget ? followTarget->GetGUID() : ObjectGuid::Empty;
}

void ObjectCache::InvalidateCache()
{
    _cachedTarget = nullptr;
    _targetGuid = ObjectGuid::Empty;

    _cachedGroupLeader = nullptr;
    _groupLeaderGuid = ObjectGuid::Empty;

    _cachedGroupMembers.clear();
    _groupMemberGuids.clear();

    _cachedFollowTarget = nullptr;
    _followTargetGuid = ObjectGuid::Empty;

    _cachedInteractionTarget = nullptr;
    _interactionTargetGuid = ObjectGuid::Empty;

    _lastRefreshTime = 0;

    _stats.invalidations++;
}

void ObjectCache::ClearNonEssential()
{
    // P5: Release non-essential caches for MINIMAL tier bots.
    // Keep _cachedTarget for instant combat escalation.
    _cachedGroupLeader = nullptr;
    _groupLeaderGuid = ObjectGuid::Empty;

    _cachedGroupMembers.clear();
    _groupMemberGuids.clear();

    _cachedFollowTarget = nullptr;
    _followTargetGuid = ObjectGuid::Empty;

    _cachedInteractionTarget = nullptr;
    _interactionTargetGuid = ObjectGuid::Empty;

    _stats.invalidations++;
}

// ============================================================================
// CACHED OBJECT ACCESS - Zero ObjectAccessor calls, lock-free
// ============================================================================

::Unit* ObjectCache::GetTarget() const
{
    // Fast path: Return cached pointer without any locks
    if (_cachedTarget)
    {
        _stats.cacheHits++;
        return _cachedTarget;
    }

    _stats.cacheMisses++;
    return nullptr;
}

Player* ObjectCache::GetGroupLeader() const
{
    if (_cachedGroupLeader)
    {
        _stats.cacheHits++;
        return _cachedGroupLeader;
    }

    _stats.cacheMisses++;
    return nullptr;
}

::std::vector<Player*> const& ObjectCache::GetGroupMembers() const
{
    if (!_cachedGroupMembers.empty())
        _stats.cacheHits++;
    else
        _stats.cacheMisses++;

    return _cachedGroupMembers;
}

Player* ObjectCache::GetGroupMember(ObjectGuid guid) const
{
    for (size_t i = 0; i < _groupMemberGuids.size(); ++i)
    {
        if (_groupMemberGuids[i] == guid)
        {
            _stats.cacheHits++;
            return _cachedGroupMembers[i];
        }
    }

    _stats.cacheMisses++;
    return nullptr;
}

::Unit* ObjectCache::GetFollowTarget() const
{
    if (_cachedFollowTarget)
    {
        _stats.cacheHits++;
        return _cachedFollowTarget;
    }

    _stats.cacheMisses++;
    return nullptr;
}

WorldObject* ObjectCache::GetInteractionTarget() const
{
    if (_cachedInteractionTarget)
    {
        _stats.cacheHits++;
        return _cachedInteractionTarget;
    }

    _stats.cacheMisses++;
    return nullptr;
}

// ============================================================================
// CACHE STATUS QUERIES
// ============================================================================

bool ObjectCache::IsValid(uint32 now) const
{
    return !NeedsRefresh(now);
}

bool ObjectCache::NeedsRefresh(uint32 now) const
{
    if (_lastRefreshTime == 0)
        return true;  // Never refreshed

    return (now - _lastRefreshTime) >= _cacheLifetimeMs;
}

uint32 ObjectCache::GetAge(uint32 now) const
{
    if (_lastRefreshTime == 0)
        return 0xFFFFFFFF;  // Invalid/never refreshed

    return now - _lastRefreshTime;
}

// ============================================================================
// CACHE CONFIGURATION
// ============================================================================

void ObjectCache::SetCacheLifetime(uint32 lifetimeMs)
{
    _cacheLifetimeMs = lifetimeMs;

    TC_LOG_DEBUG("module.playerbot.cache",
                 "ObjectCache lifetime set to {}ms", lifetimeMs);
}

// ============================================================================
// PERFORMANCE METRICS
// ============================================================================

void ObjectCache::ResetStats()
{
    _stats = CacheStats();
}

// ============================================================================
// VALIDATION HELPERS - Private methods
// ============================================================================

bool ObjectCache::ValidatePointer(WorldObject* obj, ObjectGuid expectedGuid) const
{
    if (!obj)
        return false;

    // Check object is still in world
    if (!obj->IsInWorld())
        return false;

    // Check GUID matches (object may have been replaced)
    if (obj->GetGUID() != expectedGuid)
        return false;

    return true;
}

bool ObjectCache::ValidateUnit(::Unit* unit, ObjectGuid expectedGuid, Player* bot) const
{
    if (!ValidatePointer(unit, expectedGuid))
        return false;

    // Additional Unit-specific validation
    if (!unit->IsAlive() && !unit->isDead())
        return false;  // Unit in invalid state

    // Check unit is on same map as bot (prevent cross-map targeting)
    if (bot && unit->GetMapId() != bot->GetMapId())
        return false;

    return true;
}

} // namespace Playerbot
