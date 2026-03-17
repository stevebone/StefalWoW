/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Strategy.h"
#include "Define.h"
#include "ObjectGuid.h"
#include <vector>
#include <unordered_map>

class Creature;
class GameObject;

namespace Playerbot
{

// Forward declarations
class BotAI;

/**
 * @class LootStrategy
 * @brief Handles corpse looting and item pickup for solo bots
 *
 * This strategy drives bots to:
 * - Loot nearby corpses after combat
 * - Pick up quest items and valuables
 * - Manage inventory space during looting
 * - Prioritize loot based on value and need
 *
 * Priority: Medium (runs after combat ends, before other activities)
 * Performance: <0.1ms per update (only scans when needed)
 */
class TC_GAME_API LootStrategy : public Strategy
{
public:
    LootStrategy();
    ~LootStrategy() override = default;

    // Strategy interface
    void InitializeActions();
    void InitializeTriggers();
    void InitializeValues();

    void OnActivate(BotAI* ai);
    void OnDeactivate(BotAI* ai);

    bool IsActive(BotAI* ai) const;
    float GetRelevance(BotAI* ai) const;
    void UpdateBehavior(BotAI* ai, uint32 diff);

private:
    /**
     * @brief Find nearby lootable corpses
     * @param ai Bot AI instance
     * @param maxDistance Maximum search distance (default: 30 yards)
     * @return List of lootable creature GUIDs
     */
    ::std::vector<ObjectGuid> FindLootableCorpses(BotAI* ai, float maxDistance = 30.0f) const;

    /**
     * @brief Find nearby lootable game objects (chests, herb nodes, etc.)
     * @param ai Bot AI instance
     * @param maxDistance Maximum search distance (default: 20 yards)
     * @return List of lootable GameObject GUIDs
     */
    ::std::vector<ObjectGuid> FindLootableObjects(BotAI* ai, float maxDistance = 20.0f) const;

    /**
     * @brief Move to and loot a corpse
     * @param ai Bot AI instance
     * @param corpseGuid GUID of corpse to loot
     * @return true if looting initiated
     */
    bool LootCorpse(BotAI* ai, ObjectGuid corpseGuid);

    /**
     * @brief Move to and loot a game object
     * @param ai Bot AI instance
     * @param objectGuid GUID of object to loot
     * @return true if looting initiated
     */
    bool LootObject(BotAI* ai, ObjectGuid objectGuid);

    /**
     * @brief Check if bot has inventory space
     * @param ai Bot AI instance
     * @return true if bot can pick up more items
     */
    bool HasInventorySpace(BotAI* ai) const;

    /**
     * @brief Prioritize loot targets by distance and value
     * @param ai Bot AI instance
     * @param targets List of loot target GUIDs
     * @return Sorted list (highest priority first)
     */
    ::std::vector<ObjectGuid> PrioritizeLootTargets(BotAI* ai, ::std::vector<ObjectGuid> const& targets) const;

private:
    // Loot scan throttling
    uint32 _lastLootScan = 0;
    uint32 _lootScanInterval = 1000; // Scan every 1 second

    // Current loot target
    ObjectGuid _currentLootTarget;
    uint32 _lootAttempts = 0;
    uint32 _maxLootAttempts = 3;

    // Blacklist for unreachable objects (GUID -> expiry time in ms)
    mutable ::std::unordered_map<ObjectGuid, uint32> _blacklistedObjects;
    uint32 _blacklistDuration = 60000; // Blacklist for 60 seconds

    // Per-object attempt tracking (GUID -> attempt count)
    mutable ::std::unordered_map<ObjectGuid, uint32> _objectAttempts;
    uint32 _maxObjectAttempts = 5; // Max attempts before blacklisting

    // Performance tracking
    uint32 _itemsLooted = 0;
    uint32 _corpseLooted = 0;
    uint32 _goldLooted = 0;

    /**
     * @brief Check if an object is blacklisted
     * @param guid Object GUID to check
     * @return true if blacklisted and not expired
     */
    bool IsBlacklisted(ObjectGuid guid) const;

    /**
     * @brief Add object to blacklist
     * @param guid Object GUID to blacklist
     */
    void BlacklistObject(ObjectGuid guid);

    /**
     * @brief Increment attempt counter for object
     * @param guid Object GUID
     * @return Current attempt count after increment
     */
    uint32 IncrementAttempts(ObjectGuid guid);

    /**
     * @brief Clear attempt counter for object (on success)
     * @param guid Object GUID
     */
    void ClearAttempts(ObjectGuid guid);
};

} // namespace Playerbot
