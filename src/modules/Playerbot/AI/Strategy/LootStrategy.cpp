/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "LootStrategy.h"
#include "../BotAI.h"
#include "Player.h"
#include "Creature.h"
#include "GameObject.h"
#include "Item.h"
#include "Bag.h"
#include "ItemTemplate.h"
#include "ObjectAccessor.h"
#include "Map.h"
#include "Log.h"
#include "GridNotifiers.h"
#include "CellImpl.h"
#include "Loot.h"
#include "LootMgr.h"
#include "MotionMaster.h"
#include "../../Spatial/SpatialGridManager.h"  // Lock-free spatial grid for deadlock fix
#include "../../Spatial/SpatialGridQueryHelpers.h"  // Thread-safe spatial queries
#include "../../Core/Threading/SafeGridOperations.h"  // SEH-protected grid operations
#include "Movement/UnifiedMovementCoordinator.h"
#include "../../Movement/Arbiter/MovementPriorityMapper.h"
#include "UnitAI.h"
#include "../../Session/BotSession.h"
#include "../../Session/BotSessionManager.h"
#include <unordered_map>  // For distance map in PrioritizeLootTargets
#include "GameTime.h"

namespace Playerbot
{

LootStrategy::LootStrategy()
    : Strategy("loot")
{
    TC_LOG_DEBUG("module.playerbot.strategy", "LootStrategy: Initialized");
}

void LootStrategy::InitializeActions()
{
    // No actions needed - loot strategy drives looting directly
    TC_LOG_DEBUG("module.playerbot.strategy", "LootStrategy: No actions (direct loot control)");
}

void LootStrategy::InitializeTriggers()
{
    // No triggers needed - relevance system handles activation
    TC_LOG_DEBUG("module.playerbot.strategy", "LootStrategy: No triggers (using relevance system)");
}

void LootStrategy::InitializeValues()
{
    // No values needed for this simple strategy
    TC_LOG_DEBUG("module.playerbot.strategy", "LootStrategy: No values");
}

void LootStrategy::OnActivate(BotAI* ai)
{
    if (!ai || !ai->GetBot())
        return;

    TC_LOG_INFO("module.playerbot.strategy", "Loot strategy activated for bot {}", ai->GetBot()->GetName());
    SetActive(true);
}

void LootStrategy::OnDeactivate(BotAI* ai)
{
    if (!ai || !ai->GetBot())
        return;

    TC_LOG_INFO("module.playerbot.strategy", "Loot strategy deactivated for bot {}", ai->GetBot()->GetName());
    SetActive(false);

    // Clear current loot target
    _currentLootTarget.Clear();
    _lootAttempts = 0;
}

bool LootStrategy::IsActive(BotAI* ai) const
{
    if (!ai || !ai->GetBot())
        return false;

    Player* bot = ai->GetBot();
    // NOT active during combat
    if (bot->IsInCombat())
        return false;

    // Active if explicitly activated and not in a group
    return _active && !bot->GetGroup();
}

float LootStrategy::GetRelevance(BotAI* ai) const
{
    if (!ai || !ai->GetBot())
        return 0.0f;

    Player* bot = ai->GetBot();
    // Don't loot during combat
    if (bot->IsInCombat())
        return 0.0f;

    // Don't loot if inventory is full
    if (!HasInventorySpace(ai))
        return 0.0f;

    // CRITICAL FIX (2026-01-11): Always scan for actual loot availability
    // The previous caching logic relied on _currentLootTarget which could become
    // stale (loot collected but not cleared), causing LootStrategy to return 60.0f
    // forever and block QuestStrategy from running for bots without quests.
    //
    // Now we always check fresh scan results. The scan itself is still throttled
    // in UpdateBehavior, and the actual loot operations are infrequent, so this
    // is acceptable overhead for correct behavior selection.
    ::std::vector<ObjectGuid> corpses = FindLootableCorpses(ai);
    ::std::vector<ObjectGuid> objects = FindLootableObjects(ai);

    // Medium-high relevance if loot available (lower than quest=70, higher than solo=10)
    // Returns 0.0f if no loot → allows QuestStrategy (50.0f) to run for quest seeking
    bool hasLoot = !corpses.empty() || !objects.empty();

    // DEBUG: Log what's being found when loot is detected
    if (hasLoot)
    {
        TC_LOG_DEBUG("module.playerbot.strategy",
            "LootStrategy::GetRelevance: Bot {} found {} corpses, {} objects - returning 60.0f",
            bot->GetName(), corpses.size(), objects.size());
    }

    return hasLoot ? 60.0f : 0.0f;
}
void LootStrategy::UpdateBehavior(BotAI* ai, uint32 diff)
{
    if (!ai || !ai->GetBot())
        return;

    Player* bot = ai->GetBot();
    // Don't loot during combat
    if (bot->IsInCombat())
        return;

    // Don't loot if inventory is full
    if (!HasInventorySpace(ai))
    {
        TC_LOG_DEBUG("module.playerbot.strategy", "LootStrategy: Bot {} inventory full, skipping loot",
                     bot->GetName());
        return;
    }

    TC_LOG_DEBUG("module.playerbot.strategy", "LootStrategy::UpdateBehavior: Bot {} searching for loot",
                 bot->GetName());

    uint32 currentTime = GameTime::GetGameTimeMS();

    // Throttle loot scanning
    if (currentTime - _lastLootScan < _lootScanInterval)
        return;

    _lastLootScan = currentTime;

    // Find lootable targets
    ::std::vector<ObjectGuid> corpses = FindLootableCorpses(ai);
    ::std::vector<ObjectGuid> objects = FindLootableObjects(ai);

    TC_LOG_DEBUG("module.playerbot.strategy", "LootStrategy: Bot {} found {} corpses and {} objects",
                 bot->GetName(), corpses.size(), objects.size());

    // Combine and prioritize targets
    ::std::vector<ObjectGuid> allTargets;
    allTargets.insert(allTargets.end(), corpses.begin(), corpses.end());
    allTargets.insert(allTargets.end(), objects.begin(), objects.end());

    if (allTargets.empty())
    {
        _currentLootTarget.Clear();
        return;
    }

    // Prioritize targets
    allTargets = PrioritizeLootTargets(ai, allTargets);

    // Try to loot the highest priority target
    for (ObjectGuid const& targetGuid : allTargets)
    {
        if (targetGuid.IsCreature())
        {
            if (LootCorpse(ai, targetGuid))
            {
                _currentLootTarget = targetGuid;
                _corpseLooted++;
                break;
            }
        }
        else if (targetGuid.IsGameObject())
        {
            if (LootObject(ai, targetGuid))
            {
                _currentLootTarget = targetGuid;
                break;
            }
        }
    }
}

::std::vector<ObjectGuid> LootStrategy::FindLootableCorpses(BotAI* ai, float maxDistance) const
{
    ::std::vector<ObjectGuid> lootableCorpses;
    if (!ai || !ai->GetBot())
        return lootableCorpses;

    Player* bot = ai->GetBot();

    // THREAD-SAFE: Use SafeGridOperations with SEH protection to catch access violations
    ::std::list<Creature*> nearbyCreatures;
    if (!SafeGridOperations::GetCreatureListSafe(bot, nearbyCreatures, 0, maxDistance))
        return lootableCorpses;

    uint32 deadCount = 0;
    uint32 canHaveLootCount = 0;
    uint32 hasRecipientCount = 0;

    // Filter for dead creatures with loot
    for (Creature* creature : nearbyCreatures)
    {
        if (!creature || !creature->isDead())
            continue;

        // DISTANCE FILTER: GetCreatureListWithEntryInGrid uses grid cells,
        // which can return creatures beyond maxDistance. Filter properly.
        float distance = bot->GetExactDist(creature);
        if (distance > maxDistance)
            continue;

        deadCount++;

        bool canHaveLoot = creature->CanHaveLoot();
        bool hasRecipient = creature->hasLootRecipient();

        if (canHaveLoot) canHaveLootCount++;
        if (hasRecipient) hasRecipientCount++;

        // Check if creature has loot - RELAXED: only require CanHaveLoot
        // hasLootRecipient check removed as bots may not be properly tagged as recipients
        if (!canHaveLoot)
            continue;

        // Check if loot has already been taken (prevents re-queueing looted corpses)
        Loot* loot = creature->GetLootForPlayer(bot);
        if (loot && loot->isLooted())
            continue;  // Already looted, skip

        // Check if bot is allowed to loot (is in tap list or in group with someone who tapped)
        if (hasRecipient)
        {
            GuidUnorderedSet const& tapList = creature->GetTapList();
            bool canLoot = tapList.count(bot->GetGUID()) > 0;

            // If bot didn't tap, check if group member tapped
            if (!canLoot)
            {
                if (Group* group = bot->GetGroup())
                {
                    for (ObjectGuid const& tapperGuid : tapList)
                    {
                        if (group->IsMember(tapperGuid))
                        {
                            canLoot = true;
                            break;
                        }
                    }
                }
            }

            // Debug: Log tap list contents when bot can't loot
            if (!canLoot)
            {
                TC_LOG_DEBUG("module.playerbot.strategy",
                    "FindLootableCorpses: Bot {} ({}) NOT in tap list for creature {} (entry {}). Tap list size: {}",
                    bot->GetName(), bot->GetGUID().ToString(),
                    creature->GetGUID().ToString(), creature->GetEntry(),
                    tapList.size());
            }

            if (!canLoot)
                continue; // Not our loot
        }

        // Skip blacklisted corpses (unreachable or failed too many times)
        if (IsBlacklisted(creature->GetGUID()))
        {
            TC_LOG_DEBUG("module.playerbot.strategy",
                "FindLootableCorpses: Skipping blacklisted corpse {}",
                creature->GetGUID().ToString());
            continue;
        }

        // Add to lootable list
        lootableCorpses.push_back(creature->GetGUID());
    }

    if (deadCount > 0)
    {
        TC_LOG_DEBUG("module.playerbot.strategy", "FindLootableCorpses: Bot {} found {} dead creatures, {} canHaveLoot, {} hasRecipient, {} lootable",
            bot->GetName(), deadCount, canHaveLootCount, hasRecipientCount, lootableCorpses.size());
    }

    return lootableCorpses;
}

::std::vector<ObjectGuid> LootStrategy::FindLootableObjects(BotAI* ai, float maxDistance) const
{
    ::std::vector<ObjectGuid> lootableObjects;

    if (!ai || !ai->GetBot())
        return lootableObjects;

    Player* bot = ai->GetBot();
    Map* map = bot->GetMap();
    if (!map)
        return lootableObjects;

    // DEADLOCK FIX: Use lock-free spatial grid instead of Cell::VisitGridObjects
    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
    {
        // Grid not yet created for this map - create it on demand
        sSpatialGridManager.CreateGrid(map);
        spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)
            return lootableObjects;
    }

    // Query nearby GameObjects (lock-free!)
    ::std::vector<DoubleBufferedSpatialGrid::GameObjectSnapshot> nearbyObjects =
        spatialGrid->QueryNearbyGameObjects(bot->GetPosition(), maxDistance);

    // Filter lootable objects using snapshot data
    for (auto const& snapshot : nearbyObjects)
    {
        if (!snapshot.isSpawned)
            continue;

        // Check if object is lootable container (chest)
        if (snapshot.goType != GAMEOBJECT_TYPE_CHEST)
            continue;

        // VALIDATION: Verify GameObject still exists in map before adding
        // The spatial grid snapshot might be stale if the object was looted/despawned
        GameObject* go = map->GetGameObject(snapshot.guid);
        if (!go)
        {
            TC_LOG_DEBUG("module.playerbot.strategy",
                "FindLootableObjects: Skipping stale snapshot {} - GameObject not found in map",
                snapshot.guid.ToString());
            continue;
        }

        // Check if the chest still has loot available
        if (go->GetGoState() != GO_STATE_READY)
        {
            TC_LOG_DEBUG("module.playerbot.strategy",
                "FindLootableObjects: Skipping {} - GoState is not READY (already looted?)",
                snapshot.guid.ToString());
            continue;
        }

        // Skip blacklisted objects (unreachable or failed too many times)
        if (IsBlacklisted(snapshot.guid))
        {
            TC_LOG_DEBUG("module.playerbot.strategy",
                "FindLootableObjects: Skipping blacklisted object {}",
                snapshot.guid.ToString());
            continue;
        }

        // Add to lootable list
        lootableObjects.push_back(snapshot.guid);
    }

    return lootableObjects;
}

bool LootStrategy::LootCorpse(BotAI* ai, ObjectGuid corpseGuid)
{
    if (!ai || !ai->GetBot())
        return false;

    Player* bot = ai->GetBot();
    Map* map = bot->GetMap();
    if (!map)
        return false;

    TC_LOG_DEBUG("module.playerbot.strategy", "LootCorpse: Bot {} attempting to loot corpse {}",
                 bot->GetName(), corpseGuid.ToString());

    // Find the creature using TrinityCore's live API (same as FindLootableCorpses)
    // This is more reliable than spatial grid which may not have updated dead state yet
    // THREAD-SAFE: Use SafeGridOperations with SEH protection to catch access violations
    Creature* creature = nullptr;
    ::std::list<Creature*> nearbyCreatures;
    if (!SafeGridOperations::GetCreatureListSafe(bot, nearbyCreatures, 0, 50.0f))
        return false;

    TC_LOG_DEBUG("module.playerbot.strategy", "LootCorpse: Bot {} found {} nearby creatures",
                 bot->GetName(), nearbyCreatures.size());

    for (Creature* c : nearbyCreatures)
    {
        if (c && c->GetGUID() == corpseGuid)
        {
            creature = c;
            break;
        }
    }

    // Validate creature exists and is dead
    if (!creature || !creature->isDead())
    {
        TC_LOG_DEBUG("module.playerbot.strategy", "LootCorpse: Bot {} - creature {} not found or not dead (found={}, dead={})",
                     bot->GetName(), corpseGuid.ToString(), creature != nullptr, creature ? creature->isDead() : false);
        return false;
    }

    // Check distance
    float distance = bot->GetExactDist(creature);
    TC_LOG_DEBUG("module.playerbot.strategy", "LootCorpse: Bot {} distance to corpse {:.1f} (need <= {:.1f})",
                 bot->GetName(), distance, INTERACTION_DISTANCE);

    if (distance > INTERACTION_DISTANCE)
    {
        // Track attempts - blacklist if too many failures
        uint32 attempts = IncrementAttempts(corpseGuid);
        if (attempts >= _maxObjectAttempts)
        {
            TC_LOG_DEBUG("module.playerbot.strategy",
                "LootCorpse: Bot {} failed to reach corpse {} after {} attempts - blacklisting",
                bot->GetName(), corpseGuid.ToString(), attempts);
            BlacklistObject(corpseGuid);
            ClearAttempts(corpseGuid);
            return false;
        }

        // Move closer to creature - MUST use arbiter (thread-safe)
        // Direct MotionMaster calls are NOT thread-safe from worker threads!
        Position pos = creature->GetPosition();

        // Use the BotAI passed to this function (already validated)
        if (ai->GetUnifiedMovementCoordinator())
        {
            bool accepted = ai->RequestPointMovement(
                PlayerBotMovementPriority::LOOT,  // Priority 40 - MINIMAL tier
                pos,
                "Moving to corpse for looting",
                "LootStrategy");

            TC_LOG_DEBUG("module.playerbot.strategy", "LootCorpse: Bot {} movement request {} (attempt {}/{}, distance {:.1f})",
                         bot->GetName(), accepted ? "ACCEPTED" : "REJECTED", attempts, _maxObjectAttempts, distance);
        }
        else
        {
            TC_LOG_DEBUG("module.playerbot.strategy", "LootCorpse: Bot {} NO movement coordinator - cannot reach corpse (attempt {}/{})",
                         bot->GetName(), attempts, _maxObjectAttempts);
        }
        return false;
    }

    // THREAD SAFETY: Bot AI updates can happen on worker threads.
    // SendLoot() modifies _updateObjects which must happen on main thread.
    // Queue the loot target for processing on main thread via BotSession.

    // Get BotSession to queue loot
    BotSession* botSession = BotSessionManager::GetBotSession(bot->GetSession());
    if (!botSession)
    {
        TC_LOG_DEBUG("module.playerbot.strategy", "LootStrategy: Bot {} - no BotSession available",
                     bot->GetName());
        return false;
    }

    // Queue the loot target for main thread processing
    botSession->QueueLootTarget(corpseGuid);

    // Clear attempt counter on successful queue
    ClearAttempts(corpseGuid);

    TC_LOG_DEBUG("module.playerbot.strategy", "LootStrategy: Bot {} queued corpse {} for looting on main thread",
                 bot->GetName(), corpseGuid.ToString());

    return true;
}

bool LootStrategy::LootObject(BotAI* ai, ObjectGuid objectGuid)
{
    if (!ai || !ai->GetBot())
        return false;

    Player* bot = ai->GetBot();
    Map* map = bot->GetMap();
    if (!map)
        return false;

    // DEADLOCK FIX: Use spatial grid to validate object state without pointer access
    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
        return false;

    // Query nearby game objects to find our target
    ::std::vector<DoubleBufferedSpatialGrid::GameObjectSnapshot> nearbyObjects =
        spatialGrid->QueryNearbyGameObjects(bot->GetPosition(), 50.0f);

    // Find the object in snapshots
    DoubleBufferedSpatialGrid::GameObjectSnapshot const* objectSnapshot = nullptr;
    for (auto const& snapshot : nearbyObjects)
    {
        if (snapshot.guid == objectGuid)
        {
            objectSnapshot = &snapshot;
            break;
        }
    }

    // Validate object exists and is spawned
    if (!objectSnapshot || !objectSnapshot->isSpawned)
        return false;

    // Check distance using snapshot position
    float distance = bot->GetExactDist(objectSnapshot->position);
    if (distance > INTERACTION_DISTANCE)
    {
        // Track attempts - blacklist if too many failures
        uint32 attempts = IncrementAttempts(objectGuid);
        if (attempts >= _maxObjectAttempts)
        {
            TC_LOG_DEBUG("module.playerbot.strategy",
                "LootStrategy: Bot {} failed to reach object {} after {} attempts - blacklisting",
                bot->GetName(), objectGuid.ToString(), attempts);
            BlacklistObject(objectGuid);
            ClearAttempts(objectGuid);
            return false;
        }

        // Move closer - MUST use arbiter (thread-safe)
        // Direct MotionMaster calls are NOT thread-safe from worker threads!
        Position pos;
        pos.Relocate(objectSnapshot->position);

        // Use the BotAI passed to this function (already validated)
        if (ai->GetUnifiedMovementCoordinator())
        {
            bool accepted = ai->RequestPointMovement(
                PlayerBotMovementPriority::LOOT,  // Priority 40 - MINIMAL tier
                pos,
                "Moving to object for looting",
                "LootStrategy");

            if (accepted)
            {
                TC_LOG_DEBUG("module.playerbot.strategy", "LootStrategy: Bot {} moving to object at distance {:.1f} (attempt {}/{})",
                             bot->GetName(), distance, attempts, _maxObjectAttempts);
            }
            else
            {
                TC_LOG_DEBUG("module.playerbot.strategy", "LootStrategy: Bot {} movement REJECTED for object at distance {:.1f} (attempt {}/{})",
                             bot->GetName(), distance, attempts, _maxObjectAttempts);
            }
        }
        else
        {
            TC_LOG_DEBUG("module.playerbot.strategy", "LootStrategy: Bot {} has no movement coordinator - cannot reach object (attempt {}/{})",
                         bot->GetName(), attempts, _maxObjectAttempts);
        }
        // No fallback - direct MotionMaster calls crash from worker threads
        return false;
    }

    // THREAD-SAFE: Queue object use for main thread processing
    // GameObject::Use() is NOT thread-safe - it modifies game object state and
    // triggers Map updates that cause ACCESS_VIOLATION if called from worker threads.
    // Solution: Defer to main thread via BotSession::QueueObjectUse()
    BotSession* botSession = BotSessionManager::GetBotSession(bot->GetSession());
    if (!botSession)
    {
        TC_LOG_DEBUG("module.playerbot.strategy", "LootObject: Bot {} has no BotSession, cannot queue object use",
                     bot->GetName());
        return false;
    }

    // Queue the object for main thread Use()
    botSession->QueueObjectUse(objectGuid);

    // Clear attempt counter on successful queue
    ClearAttempts(objectGuid);

    TC_LOG_DEBUG("module.playerbot.strategy", "LootStrategy: Bot {} queued object {} (entry {}) for Use() on main thread",
                 bot->GetName(), objectGuid.ToString(), objectSnapshot->entry);

    return true;
}

bool LootStrategy::HasInventorySpace(BotAI* ai) const
{
    if (!ai || !ai->GetBot())
        return false;

    Player* bot = ai->GetBot();

    // Check if bot has at least 5 free bag slots
    uint32 freeSlots = 0;
    for (uint8 i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
    {
        if (Bag* bag = bot->GetBagByPos(i))
        {
            freeSlots += bag->GetFreeSlots();
        }
    }

    // Add main backpack slots
    for (uint8 i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
    {
        if (!bot->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
            freeSlots++;
    }

    return freeSlots >= 5;
}

::std::vector<ObjectGuid> LootStrategy::PrioritizeLootTargets(BotAI* ai, ::std::vector<ObjectGuid> const& targets) const
{
    if (!ai || !ai->GetBot())
        return targets;

    Player* bot = ai->GetBot();
    Map* map = bot->GetMap();
    if (!map)
        return targets;

    // DEADLOCK FIX: Build distance map from snapshots instead of resolving GUIDs in lambda
    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
        return targets;

    // Query all nearby entities once (lock-free!)
    ::std::vector<DoubleBufferedSpatialGrid::CreatureSnapshot> nearbyCreatures =
        spatialGrid->QueryNearbyCreatures(bot->GetPosition(), 50.0f);
    ::std::vector<DoubleBufferedSpatialGrid::GameObjectSnapshot> nearbyObjects =
        spatialGrid->QueryNearbyGameObjects(bot->GetPosition(), 50.0f);

    // Build distance map using snapshot positions
    ::std::unordered_map<ObjectGuid, float> distanceMap;

    for (auto const& snapshot : nearbyCreatures)
    {
        float distance = bot->GetExactDist(snapshot.position);
        distanceMap[snapshot.guid] = distance;
    }

    for (auto const& snapshot : nearbyObjects)
    {
        float distance = bot->GetExactDist(snapshot.position);
        distanceMap[snapshot.guid] = distance;
    }

    // Sort targets by distance (closest first) using pre-computed distances
    ::std::vector<ObjectGuid> prioritized = targets;
    ::std::sort(prioritized.begin(), prioritized.end(),
        [&distanceMap](ObjectGuid const& a, ObjectGuid const& b) -> bool
        {
            auto itA = distanceMap.find(a);
            auto itB = distanceMap.find(b);

            // If either GUID not found in distance map, deprioritize it
    if (itA == distanceMap.end())
                return false;
            if (itB == distanceMap.end())
                return true;

            return itA->second < itB->second;
        });

    return prioritized;
}

bool LootStrategy::IsBlacklisted(ObjectGuid guid) const
{
    auto it = _blacklistedObjects.find(guid);
    if (it == _blacklistedObjects.end())
        return false;

    uint32 currentTime = GameTime::GetGameTimeMS();
    if (currentTime >= it->second)
    {
        // Expired - remove from blacklist
        _blacklistedObjects.erase(it);
        return false;
    }

    return true;
}

void LootStrategy::BlacklistObject(ObjectGuid guid)
{
    uint32 expiryTime = GameTime::GetGameTimeMS() + _blacklistDuration;
    _blacklistedObjects[guid] = expiryTime;

    TC_LOG_DEBUG("module.playerbot.strategy",
        "LootStrategy: Blacklisted object {} for {}ms (too many failed attempts)",
        guid.ToString(), _blacklistDuration);
}

uint32 LootStrategy::IncrementAttempts(ObjectGuid guid)
{
    return ++_objectAttempts[guid];
}

void LootStrategy::ClearAttempts(ObjectGuid guid)
{
    _objectAttempts.erase(guid);
}

} // namespace Playerbot
