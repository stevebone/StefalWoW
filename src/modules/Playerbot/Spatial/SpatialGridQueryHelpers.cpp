/**
 * @file SpatialGridQueryHelpers.cpp
 * @brief Implementation of thread-safe spatial grid query helpers
 *
 * @created 2025-10-21
 * @part-of Phase 5A: ObjectAccessor → SpatialGrid Migration
 */

#include "SpatialGridQueryHelpers.h"
#include "Player.h"
#include "Group.h"
#include "ObjectAccessor.h"

namespace Playerbot
{

// ===== PRIVATE HELPER =====

DoubleBufferedSpatialGrid* SpatialGridQueryHelpers::GetSpatialGrid(Player* bot)
{
    if (!bot)
        return nullptr;

    return sSpatialGridManager.GetGrid(bot->GetMapId());
}

// ===== CREATURE QUERIES =====

DoubleBufferedSpatialGrid::CreatureSnapshot const*
SpatialGridQueryHelpers::FindCreatureByGuid(Player* bot, ObjectGuid guid, float searchRadius)
{
    auto spatialGrid = GetSpatialGrid(bot);
    if (!spatialGrid || guid.IsEmpty())
        return nullptr;

    // Query nearby creatures from spatial grid (lock-free)
    auto creatureSnapshots = spatialGrid->QueryNearbyCreatures(bot->GetPosition(), searchRadius);
    // Find snapshot matching GUID
    for (auto const& snapshot : creatureSnapshots)
    {
        if (snapshot.guid == guid)
            return &snapshot;
    }

    return nullptr;
}

::std::vector<DoubleBufferedSpatialGrid::CreatureSnapshot>
SpatialGridQueryHelpers::FindHostileCreaturesInRange(Player* bot, float range, bool requireAlive)
{
    ::std::vector<DoubleBufferedSpatialGrid::CreatureSnapshot> result;

    auto spatialGrid = GetSpatialGrid(bot);
    if (!spatialGrid)
        return result;

    // Query nearby creatures (lock-free)
    auto creatureSnapshots = spatialGrid->QueryNearbyCreatures(bot->GetPosition(), range);
    // Filter for hostile creatures
    for (auto const& snapshot : creatureSnapshots)
    {
        // Check hostility
        if (!snapshot.isHostile)
            continue;

        // Check alive requirement
        if (requireAlive && !snapshot.IsAlive())
            continue;

        // Check distance (spatial grid may return entities slightly outside range)
        float distance = bot->GetDistance(snapshot.position);
        if (distance > range)
            continue;

        result.push_back(snapshot);  // Copy instead of pointer
    }

    return result;
}

bool SpatialGridQueryHelpers::ValidateCreature(Player* bot, ObjectGuid guid,
    bool requireAlive, bool requireHostile)
{
    auto snapshot = FindCreatureByGuid(bot, guid);
    if (!snapshot)
        return false;

    if (requireAlive && !snapshot->IsAlive())
        return false;

    if (requireHostile && !snapshot->isHostile)
        return false;

    return true;
}

// ===== PLAYER QUERIES =====

DoubleBufferedSpatialGrid::PlayerSnapshot const*
SpatialGridQueryHelpers::FindPlayerByGuid(Player* bot, ObjectGuid guid, float searchRadius)
{
    auto spatialGrid = GetSpatialGrid(bot);
    if (!spatialGrid || guid.IsEmpty())
        return nullptr;

    // Query nearby players from spatial grid (lock-free)
    auto playerSnapshots = spatialGrid->QueryNearbyPlayers(bot->GetPosition(), searchRadius);
    // Find snapshot matching GUID
    for (auto const& snapshot : playerSnapshots)
    {
        if (snapshot.guid == guid)
            return &snapshot;
    }

    return nullptr;
}

::std::vector<DoubleBufferedSpatialGrid::PlayerSnapshot>
SpatialGridQueryHelpers::FindGroupMembersInRange(Player* bot, float range)
{
    ::std::vector<DoubleBufferedSpatialGrid::PlayerSnapshot> result;

    auto spatialGrid = GetSpatialGrid(bot);
    if (!spatialGrid)
        return result;

    // Get bot's group
    Group* group = bot->GetGroup();
    if (!group)
        return result;

    // Query nearby players (lock-free)
    auto playerSnapshots = spatialGrid->QueryNearbyPlayers(bot->GetPosition(), range);
    // Filter for group members
    for (auto const& snapshot : playerSnapshots)
    {
        // Check if player is in our group
        if (snapshot.groupGuid != group->GetGUID())
            continue;

        // Check distance
        float distance = bot->GetDistance(snapshot.position);
        if (distance > range)
            continue;

        result.push_back(snapshot);  // Copy instead of pointer
    }

    return result;
}

// ===== GAMEOBJECT QUERIES =====

DoubleBufferedSpatialGrid::GameObjectSnapshot const*
SpatialGridQueryHelpers::FindGameObjectByGuid(Player* bot, ObjectGuid guid, float searchRadius)
{
    auto spatialGrid = GetSpatialGrid(bot);
    if (!spatialGrid || guid.IsEmpty())
        return nullptr;

    // Query nearby GameObjects from spatial grid (lock-free)
    auto gameObjectSnapshots = spatialGrid->QueryNearbyGameObjects(bot->GetPosition(), searchRadius);
    // Find snapshot matching GUID
    for (auto const& snapshot : gameObjectSnapshots)
    {
        if (snapshot.guid == guid)
            return &snapshot;
    }

    return nullptr;
}

::std::vector<DoubleBufferedSpatialGrid::GameObjectSnapshot>
SpatialGridQueryHelpers::FindQuestGameObjectsInRange(Player* bot, float range)
{
    ::std::vector<DoubleBufferedSpatialGrid::GameObjectSnapshot> result;

    auto spatialGrid = GetSpatialGrid(bot);
    if (!spatialGrid)
        return result;

    // Query nearby GameObjects (lock-free)
    auto gameObjectSnapshots = spatialGrid->QueryNearbyGameObjects(bot->GetPosition(), range);
    // Filter for quest objects
    for (auto const& snapshot : gameObjectSnapshots)
    {
        // Check if quest object
        if (!snapshot.isQuestObject)
            continue;

        // Check distance
        float distance = bot->GetDistance(snapshot.position);
        if (distance > range)
            continue;

        result.push_back(snapshot);  // Copy instead of pointer
    }

    return result;
}

// ===== DYNAMICOBJECT QUERIES =====

DoubleBufferedSpatialGrid::DynamicObjectSnapshot const*
SpatialGridQueryHelpers::FindDynamicObjectByGuid(Player* bot, ObjectGuid guid, float searchRadius)
{
    auto spatialGrid = GetSpatialGrid(bot);
    if (!spatialGrid || guid.IsEmpty())
        return nullptr;

    // Query nearby DynamicObjects from spatial grid (lock-free)
    auto dynamicObjectSnapshots = spatialGrid->QueryNearbyDynamicObjects(bot->GetPosition(), searchRadius);
    // Find snapshot matching GUID
    for (auto const& snapshot : dynamicObjectSnapshots)
    {
        if (snapshot.guid == guid)
            return &snapshot;
    }

    return nullptr;
}

::std::vector<DoubleBufferedSpatialGrid::DynamicObjectSnapshot>
SpatialGridQueryHelpers::FindDangerousDynamicObjectsInRange(Player* bot, float range)
{
    ::std::vector<DoubleBufferedSpatialGrid::DynamicObjectSnapshot> result;

    auto spatialGrid = GetSpatialGrid(bot);
    if (!spatialGrid)
        return result;

    // Query nearby DynamicObjects (lock-free)
    auto dynamicObjectSnapshots = spatialGrid->QueryNearbyDynamicObjects(bot->GetPosition(), range);
    // Filter for dangerous objects (hostile faction)
    for (auto const& snapshot : dynamicObjectSnapshots)
    {
        // Check if active
        if (!snapshot.IsActive())
            continue;

        // Check distance
        float distance = bot->GetDistance(snapshot.position);
        if (distance > range)
            continue;

        // Return all active objects - caller determines what's dangerous based on caster/faction
        result.push_back(snapshot);  // Copy instead of pointer
    }

    return result;
}

// ===== AREATRIGGER QUERIES =====

DoubleBufferedSpatialGrid::AreaTriggerSnapshot const*
SpatialGridQueryHelpers::FindAreaTriggerByGuid(Player* bot, ObjectGuid guid, float searchRadius)
{
    auto spatialGrid = GetSpatialGrid(bot);
    if (!spatialGrid || guid.IsEmpty())
        return nullptr;

    // Query nearby AreaTriggers from spatial grid (lock-free)
    auto areaTriggerSnapshots = spatialGrid->QueryNearbyAreaTriggers(bot->GetPosition(), searchRadius);
    // Find snapshot matching GUID
    for (auto const& snapshot : areaTriggerSnapshots)
    {
        if (snapshot.guid == guid)
            return &snapshot;
    }

    return nullptr;
}

::std::vector<DoubleBufferedSpatialGrid::AreaTriggerSnapshot>
SpatialGridQueryHelpers::FindDangerousAreaTriggersInRange(Player* bot, float range)
{
    ::std::vector<DoubleBufferedSpatialGrid::AreaTriggerSnapshot> result;

    auto spatialGrid = GetSpatialGrid(bot);
    if (!spatialGrid)
        return result;

    // Query nearby AreaTriggers (lock-free)
    auto areaTriggerSnapshots = spatialGrid->QueryNearbyAreaTriggers(bot->GetPosition(), range);
    // Filter for dangerous triggers (hostile spells/effects)
    for (auto const& snapshot : areaTriggerSnapshots)
    {
        // Check if active
        if (!snapshot.IsActive())
            continue;

        // Get caster to check faction hostility
        Unit* caster = nullptr;
        if (!snapshot.casterGuid.IsEmpty())
            caster = ObjectAccessor::GetUnit(*bot, snapshot.casterGuid);

        // Dangerous if caster is hostile or missing (assume hostile)
        if (caster && !bot->IsHostileTo(caster))
            continue;

        // Check distance
        float distance = bot->GetDistance(snapshot.position);
        if (distance > range)
            continue;

        result.push_back(snapshot);  // Copy instead of pointer
    }

    return result;
}

// ===== DISTANCE & POSITION UTILITIES =====

float SpatialGridQueryHelpers::GetDistanceToEntity(Player* bot, ObjectGuid guid)
{
    if (!bot || guid.IsEmpty())
        return -1.0f;

    auto spatialGrid = GetSpatialGrid(bot);
    if (!spatialGrid)
        return -1.0f;

    Position botPos = bot->GetPosition();

    // Try creatures first (most common)
    {
        auto creatureSnapshots = spatialGrid->QueryNearbyCreatures(botPos, 100.0f);
        for (auto const& snapshot : creatureSnapshots)
        {
            if (snapshot.guid == guid)
                return bot->GetDistance(snapshot.position);
        }
    }

    // Try players
    {
        auto playerSnapshots = spatialGrid->QueryNearbyPlayers(botPos, 100.0f);
        for (auto const& snapshot : playerSnapshots)
        {
            if (snapshot.guid == guid)
                return bot->GetDistance(snapshot.position);
        }
    }

    // Try GameObjects
    {
        auto gameObjectSnapshots = spatialGrid->QueryNearbyGameObjects(botPos, 100.0f);
        for (auto const& snapshot : gameObjectSnapshots)
        {
            if (snapshot.guid == guid)
                return bot->GetDistance(snapshot.position);
        }
    }

    // Try DynamicObjects
    {
        auto dynamicObjectSnapshots = spatialGrid->QueryNearbyDynamicObjects(botPos, 100.0f);
        for (auto const& snapshot : dynamicObjectSnapshots)
        {
            if (snapshot.guid == guid)
                return bot->GetDistance(snapshot.position);
        }
    }

    // Try AreaTriggers
    {
        auto areaTriggerSnapshots = spatialGrid->QueryNearbyAreaTriggers(botPos, 100.0f);
        for (auto const& snapshot : areaTriggerSnapshots)
        {
            if (snapshot.guid == guid)
                return bot->GetDistance(snapshot.position);
        }
    }

    return -1.0f; // Not found
}

bool SpatialGridQueryHelpers::GetEntityPosition(Player* bot, ObjectGuid guid, Position& position)
{
    if (!bot || guid.IsEmpty())
        return false;

    auto spatialGrid = GetSpatialGrid(bot);
    if (!spatialGrid)
        return false;

    Position botPos = bot->GetPosition();

    // Try creatures first (most common)
    {
        auto creatureSnapshots = spatialGrid->QueryNearbyCreatures(botPos, 100.0f);
        for (auto const& snapshot : creatureSnapshots)
        {
            if (snapshot.guid == guid)
            {
                position = snapshot.position;
                return true;
            }
        }
    }

    // Try players
    {
        auto playerSnapshots = spatialGrid->QueryNearbyPlayers(botPos, 100.0f);
        for (auto const& snapshot : playerSnapshots)
        {
            if (snapshot.guid == guid)
            {
                position = snapshot.position;
                return true;
            }
        }
    }

    // Try GameObjects
    {
        auto gameObjectSnapshots = spatialGrid->QueryNearbyGameObjects(botPos, 100.0f);
        for (auto const& snapshot : gameObjectSnapshots)
        {
            if (snapshot.guid == guid)
            {
                position = snapshot.position;
                return true;
            }
        }
    }

    // Try DynamicObjects
    {
        auto dynamicObjectSnapshots = spatialGrid->QueryNearbyDynamicObjects(botPos, 100.0f);
        for (auto const& snapshot : dynamicObjectSnapshots)
        {
            if (snapshot.guid == guid)
            {
                position = snapshot.position;
                return true;
            }
        }
    }

    // Try AreaTriggers
    {
        auto areaTriggerSnapshots = spatialGrid->QueryNearbyAreaTriggers(botPos, 100.0f);
        for (auto const& snapshot : areaTriggerSnapshots)
        {
            if (snapshot.guid == guid)
            {
                position = snapshot.position;
                return true;
            }
        }
    }

    return false; // Not found
}

// ===== VALIDATION UTILITIES =====

bool SpatialGridQueryHelpers::EntityExists(Player* bot, ObjectGuid guid)
{
    if (!bot || guid.IsEmpty())
        return false;

    // Quick check using GetDistanceToEntity
    // If distance >= 0, entity exists
    return GetDistanceToEntity(bot, guid) >= 0.0f;
}

TypeID SpatialGridQueryHelpers::GetEntityType(Player* bot, ObjectGuid guid)
{
    if (!bot || guid.IsEmpty())
        return TYPEID_UNIT; // Default

    auto spatialGrid = GetSpatialGrid(bot);
    if (!spatialGrid)
        return TYPEID_UNIT;

    Position botPos = bot->GetPosition();

    // Check each entity type
    {
        auto creatureSnapshots = spatialGrid->QueryNearbyCreatures(botPos, 100.0f);
        for (auto const& snapshot : creatureSnapshots)
        {
            if (snapshot.guid == guid)
                return TYPEID_UNIT;
        }
    }

    {
        auto playerSnapshots = spatialGrid->QueryNearbyPlayers(botPos, 100.0f);
        for (auto const& snapshot : playerSnapshots)
        {
            if (snapshot.guid == guid)
                return TYPEID_PLAYER;
        }
    }

    {
        auto gameObjectSnapshots = spatialGrid->QueryNearbyGameObjects(botPos, 100.0f);
        for (auto const& snapshot : gameObjectSnapshots)
        {
            if (snapshot.guid == guid)
                return TYPEID_GAMEOBJECT;
        }
    }

    {
        auto dynamicObjectSnapshots = spatialGrid->QueryNearbyDynamicObjects(botPos, 100.0f);
        for (auto const& snapshot : dynamicObjectSnapshots)
        {
            if (snapshot.guid == guid)
                return TYPEID_DYNAMICOBJECT;
        }
    }

    {
        auto areaTriggerSnapshots = spatialGrid->QueryNearbyAreaTriggers(botPos, 100.0f);
        for (auto const& snapshot : areaTriggerSnapshots)
        {
            if (snapshot.guid == guid)
                return TYPEID_AREATRIGGER;
        }
    }

    return TYPEID_UNIT; // Not found, return default
}

// ===== ENTRY-BASED QUERIES (THREAD-SAFE REPLACEMENTS FOR GRID OPERATIONS) =====

::std::vector<DoubleBufferedSpatialGrid::CreatureSnapshot>
SpatialGridQueryHelpers::FindCreaturesByEntryInRange(Player* bot, uint32 entry, float range, bool requireAlive)
{
    ::std::vector<DoubleBufferedSpatialGrid::CreatureSnapshot> results;

    auto spatialGrid = GetSpatialGrid(bot);
    if (!spatialGrid)
        return results;

    // Query all nearby creatures from spatial grid (lock-free, thread-safe)
    auto creatureSnapshots = spatialGrid->QueryNearbyCreatures(bot->GetPosition(), range);

    // Filter by entry and alive status
    for (auto const& snapshot : creatureSnapshots)
    {
        // Filter by entry (0 = all entries)
        if (entry != 0 && snapshot.entry != entry)
            continue;

        // Filter by alive status if required
        if (requireAlive && !snapshot.IsAlive())
            continue;

        results.push_back(snapshot);  // Copy instead of pointer - safe for caller to use later
    }

    return results;
}

::std::vector<DoubleBufferedSpatialGrid::GameObjectSnapshot>
SpatialGridQueryHelpers::FindGameObjectsByEntryInRange(Player* bot, uint32 entry, float range)
{
    ::std::vector<DoubleBufferedSpatialGrid::GameObjectSnapshot> results;

    auto spatialGrid = GetSpatialGrid(bot);
    if (!spatialGrid)
        return results;

    // Query all nearby GameObjects from spatial grid (lock-free, thread-safe)
    auto gameObjectSnapshots = spatialGrid->QueryNearbyGameObjects(bot->GetPosition(), range);

    // Filter by entry
    for (auto const& snapshot : gameObjectSnapshots)
    {
        // Filter by entry (0 = all entries)
        if (entry != 0 && snapshot.entry != entry)
            continue;

        results.push_back(snapshot);  // Copy instead of pointer - safe for caller to use later
    }

    return results;
}

} // namespace Playerbot
