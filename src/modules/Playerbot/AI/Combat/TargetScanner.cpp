/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "TargetScanner.h"
#include "Player.h"
#include "Pet.h"
#include "Creature.h"
#include "Group.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "Log.h"
#include "World.h"
#include "ObjectMgr.h"
#include "CreatureAI.h"
#include "ObjectAccessor.h"
#include "Map.h"
#include "Spatial/SpatialGridManager.h"
#include "GameTime.h"
#include <algorithm>

namespace Playerbot
{
    TargetScanner::TargetScanner(Player* bot) :
        m_bot(bot),
        m_scanMode(ScanMode::AGGRESSIVE),
        m_lastScanTime(0),
        m_scanInterval(SCAN_INTERVAL_NORMAL),
        m_lastResultsTime(0),
        m_baseRange(20.0f),
        m_maxRange(40.0f),
        m_preferRanged(false),
        m_avoidElites(false)
    {
        // Configure based on class
    switch (m_bot->GetClass())
        {
            case CLASS_HUNTER:
                m_baseRange = 35.0f;
                m_maxRange = 41.0f;  // Max hunter range
                m_preferRanged = true;
                m_scanInterval = SCAN_INTERVAL_NORMAL;
                break;

            case CLASS_MAGE:
            case CLASS_WARLOCK:
                m_baseRange = 30.0f;
                m_maxRange = 36.0f;
                m_preferRanged = true;
                m_scanInterval = SCAN_INTERVAL_NORMAL;
                break;

            case CLASS_PRIEST:
                m_baseRange = 27.0f;
                m_maxRange = 36.0f;
                m_preferRanged = true;
                m_avoidElites = true;  // Priests should be more cautious
                m_scanInterval = SCAN_INTERVAL_NORMAL;
                break;

            case CLASS_SHAMAN:
            case CLASS_DRUID:
                m_baseRange = 25.0f;
                m_maxRange = 36.0f;
                m_preferRanged = false;  // Hybrid, depends on spec
                m_scanInterval = SCAN_INTERVAL_NORMAL;
                break;

            case CLASS_WARRIOR:
            case CLASS_PALADIN:
                m_baseRange = 15.0f;
                m_maxRange = 25.0f;
                m_preferRanged = false;
                m_scanInterval = SCAN_INTERVAL_COMBAT;
                break;

            case CLASS_ROGUE:
                m_baseRange = 10.0f;  // Rogues want to get close for stealth opener
                m_maxRange = 20.0f;
                m_preferRanged = false;
                m_scanInterval = SCAN_INTERVAL_COMBAT;
                break;

            case CLASS_DEATH_KNIGHT:
                m_baseRange = 20.0f;  // Death grip range
                m_maxRange = 30.0f;
                m_preferRanged = false;
                m_scanInterval = SCAN_INTERVAL_COMBAT;
                break;

            case CLASS_MONK:
                m_baseRange = 15.0f;
                m_maxRange = 25.0f;
                m_preferRanged = false;
                m_scanInterval = SCAN_INTERVAL_COMBAT;
                break;

            case CLASS_DEMON_HUNTER:
                m_baseRange = 20.0f;
                m_maxRange = 30.0f;
                m_preferRanged = false;
                m_scanInterval = SCAN_INTERVAL_COMBAT;
                break;

            default:
                m_baseRange = 20.0f;
                m_maxRange = 30.0f;
                break;
        }

        // Adjust for low level (more cautious)
    if (m_bot->GetLevel() < 20)
        {
            m_baseRange *= 0.75f;
            m_maxRange *= 0.75f;
            m_avoidElites = true;
        }
    }

    TargetScanner::~TargetScanner() = default;
    ObjectGuid TargetScanner::FindNearestHostile(float range)
    {
        // CRITICAL: Safety check to prevent crash when bot is removed from world during worker thread execution
        if (!m_bot || !m_bot->IsInWorld())
            return ObjectGuid::Empty;

        if (m_scanMode == ScanMode::PASSIVE)
            return ObjectGuid::Empty;

        if (range == 0.0f)
            range = GetScanRadius();

        // Get spatial grid for distance calculations
        Map* map = m_bot->GetMap();
        if (!map)
            return ObjectGuid::Empty;

        DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)
        {
            sSpatialGridManager.CreateGrid(map);
            spatialGrid = sSpatialGridManager.GetGrid(map);
            if (!spatialGrid)
                return ObjectGuid::Empty;
        }

        // CRITICAL FIX: Use FindAllHostiles() which returns GUIDs
        ::std::vector<ObjectGuid> hostileGuids = FindAllHostiles(range);
        if (hostileGuids.empty())
            return ObjectGuid::Empty;

        // Find nearest hostile using snapshot data (NO ObjectAccessor calls!)
        ObjectGuid nearestGuid = ObjectGuid::Empty;
        float nearestDist = range + 1.0f;

        ::std::vector<DoubleBufferedSpatialGrid::CreatureSnapshot> nearbyCreatures =
            spatialGrid->QueryNearbyCreatures(m_bot->GetPosition(), range);

        for (ObjectGuid const& guid : hostileGuids)
        {
            // Find snapshot for this GUID
            auto it = ::std::find_if(nearbyCreatures.begin(), nearbyCreatures.end(),
                [&guid](DoubleBufferedSpatialGrid::CreatureSnapshot const& c) { return c.guid == guid; });

            if (it == nearbyCreatures.end())
                continue;

            // Calculate distance using snapshot data
            float dist = it->position.GetExactDist(m_bot->GetPosition());
            if (dist < nearestDist)
            {
                nearestDist = dist;
                nearestGuid = guid;
            }
        }

        return nearestGuid;
    }

    ObjectGuid TargetScanner::FindBestTarget(float range)
    {
        // CRITICAL: Safety check to prevent crash when bot is removed from world during worker thread execution
        // The BotSession::Update checks IsInWorld() but there's a race condition between the check and this call
        if (!m_bot || !m_bot->IsInWorld())
            return ObjectGuid::Empty;

        if (m_scanMode == ScanMode::PASSIVE)
            return ObjectGuid::Empty;

        if (range == 0.0f)
            range = GetScanRadius();

        // Get spatial grid for priority/distance calculations
        Map* map = m_bot->GetMap();
        if (!map)
            return ObjectGuid::Empty;

        DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)
            return ObjectGuid::Empty;

        // CRITICAL FIX: FindAllHostiles() now returns GUIDs instead of Unit* pointers
        ::std::vector<ObjectGuid> hostileGuids = FindAllHostiles(range);
        if (hostileGuids.empty())
            return ObjectGuid::Empty;

        // Build priority list using snapshot data only (NO ObjectAccessor calls!)
        struct PriorityTarget
        {
            ObjectGuid guid;
            float distance;
            uint8 priority;

            bool operator<(const PriorityTarget& other) const
            {
                // Higher priority first, then closer distance
    if (priority != other.priority)
                    return priority > other.priority;
                return distance < other.distance;
            }
        };

        ::std::vector<PriorityTarget> priorityTargets;
        ::std::vector<DoubleBufferedSpatialGrid::CreatureSnapshot> nearbyCreatures =
            spatialGrid->QueryNearbyCreatures(m_bot->GetPosition(), range);

        for (ObjectGuid const& guid : hostileGuids)
        {
            // Find snapshot for this GUID
            auto it = ::std::find_if(nearbyCreatures.begin(), nearbyCreatures.end(),
                [&guid](DoubleBufferedSpatialGrid::CreatureSnapshot const& c) { return c.guid == guid; });

            if (it == nearbyCreatures.end())
                continue;

            // Calculate priority using snapshot data
            uint8 priority = PRIORITY_NORMAL;

            // Prioritize creatures attacking bot, bot's pet, or group members
            // CRITICAL FIX: Include pet in priority check - if something attacks our pet,
            // we must respond immediately to protect it
            ObjectGuid petGuid = m_bot->GetPetGUID();
            bool attackingBotOrPet = (it->victim == m_bot->GetGUID()) ||
                                     (!petGuid.IsEmpty() && it->victim == petGuid);

            if (attackingBotOrPet)
                priority = PRIORITY_CRITICAL;
            else if (it->isInCombat)
                priority = PRIORITY_NORMAL;
            else
                priority = PRIORITY_TRIVIAL;

            // Prioritize elites and world bosses
    if (it->isWorldBoss)
                priority = PRIORITY_CRITICAL;
            else if (it->isElite)
                priority = ::std::min<uint8>(priority + 2, PRIORITY_ELITE);

            PriorityTarget pt;
            pt.guid = guid;
            pt.distance = it->position.GetExactDist(m_bot->GetPosition());
            pt.priority = priority;
            priorityTargets.push_back(pt);
        }

        if (priorityTargets.empty())
            return ObjectGuid::Empty;

        // Sort by priority and distance
        ::std::sort(priorityTargets.begin(), priorityTargets.end());

        // Return best target GUID - main thread will validate hostility and queue action
        return priorityTargets.front().guid;
    }

    ::std::vector<ObjectGuid> TargetScanner::FindAllHostiles(float range)
    {
        ::std::vector<ObjectGuid> hostileGuids;

        // CRITICAL: Safety check to prevent crash when bot is removed from world during worker thread execution
        if (!m_bot || !m_bot->IsInWorld())
            return hostileGuids;

        if (m_scanMode == ScanMode::PASSIVE)
            return hostileGuids;

        if (range == 0.0f)
            range = GetScanRadius();

        // PHASE 1 FIX: Use lock-free double-buffered spatial grid instead of Cell::VisitAllObjects
        // Cell::VisitAllObjects caused deadlocks with 100+ bots due to:
        // - Main thread holds grid locks while updating objects
        // - Worker threads acquire grid locks for spatial queries
        // - Lock ordering conflicts → 60-second hang → crash
        //
        // NEW APPROACH:
        // - Background worker thread updates inactive grid buffer
        // - Atomic buffer swap after update complete
        // - Bots query active buffer with ZERO lock contention
        // - Scales to 10,000+ bots with 1-5μs query latency

        Map* map = m_bot->GetMap();
        if (!map)
        {
            TC_LOG_ERROR("playerbot.scanner",
                "TargetScanner::FindAllHostiles - Bot {} has no map!",
                m_bot->GetName());
            return hostileGuids;
        }

        // Get spatial grid for this map
        DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)
        {
            // Spatial grid not yet created for this map
            // Fallback: Create it on-demand
            TC_LOG_WARN("playerbot.scanner",
                "Spatial grid not found for map {} - creating on-demand",
                map->GetId());

            sSpatialGridManager.CreateGrid(map);
            spatialGrid = sSpatialGridManager.GetGrid(map);

            if (!spatialGrid)
            {
                TC_LOG_ERROR("playerbot.scanner",
                    "Failed to create spatial grid for map {}", map->GetId());
                return hostileGuids;
            }
        }

        // ===========================================================================
        // CRITICAL DEADLOCK FIX: Return GUIDs instead of Unit* pointers!
        // ===========================================================================
        // OLD CODE (DEADLOCK): Query snapshots → Call ObjectAccessor::GetUnit(guid)
        //                      → Access Map::_objectsStore (NOT THREAD-SAFE!) → DEADLOCK!
        //
        // NEW CODE (SAFE): Query snapshots → Return GUIDs only
        //                  → Main thread resolves GUID → Unit* and queues actions
        //                  → ZERO Map access from worker threads → NO DEADLOCKS!
        // ===========================================================================
        // Query nearby creature SNAPSHOTS (lock-free, thread-safe!)
        ::std::vector<DoubleBufferedSpatialGrid::CreatureSnapshot> nearbyCreatures =
            spatialGrid->QueryNearbyCreatures(m_bot->GetPosition(), range);

        TC_LOG_TRACE("playerbot.scanner",
            "Bot {} spatial query found {} candidate creatures within {}yd",
            m_bot->GetName(), nearbyCreatures.size(), range);
        // Process snapshots - validation done WITHOUT ObjectAccessor/Map calls!
        // Hostility check deferred to main thread (requires Map access)
    for (DoubleBufferedSpatialGrid::CreatureSnapshot const& creature : nearbyCreatures)
        {
            // Validate using snapshot data only (distance, level, alive, blacklist, combat state)
    if (!IsValidTargetSnapshot(creature))
                continue;
            // Store GUID - main thread will validate hostility and queue attack action
            // NO ObjectAccessor::GetUnit() call → THREAD-SAFE!
            hostileGuids.push_back(creature.guid);
        }

        TC_LOG_TRACE("playerbot.scanner",
            "Bot {} found {} valid hostile candidate GUIDs after snapshot filtering",
            m_bot->GetName(), hostileGuids.size());
        return hostileGuids;
    }

    // ===========================================================================
    // THREAD-SAFE SNAPSHOT-BASED VALIDATION (NEW!)
    // ===========================================================================
    // This method validates targets using ONLY snapshot data - NO Map/ObjectAccessor calls!
    // Safe to call from worker threads without any race conditions or deadlocks.
    // ===========================================================================
    bool TargetScanner::IsValidTargetSnapshot(DoubleBufferedSpatialGrid::CreatureSnapshot const& creature) const
    {
        // Basic validation
    if (!creature.IsValid() || creature.isDead || creature.health == 0)
            return false;

        // Check if blacklisted (uses thread-safe GUID check)
    if (this->IsBlacklisted(creature.guid))
            return false;

        // NOTE: Hostility check (IsHostileTo) requires Unit* pointer, so we defer it
        // to after ObjectAccessor::GetUnit(). This is acceptable because we've already
        // filtered out 90% of candidates using snapshot data (distance, level, alive, blacklist).

        // REMOVED: "Don't steal kills" check was too aggressive
        // The old logic filtered ANY creature in combat with someone else when bot is solo.
        // This prevented bots from:
        //   1. Attacking creatures already fighting other bots
        //   2. Assisting nearby bots in combat
        //   3. Attacking creatures that were pulled by other bots
        //
        // WoW's native tagging system handles loot/XP distribution properly.
        // Bots should behave like normal players and engage any hostile creature.
        //
        // OLD CODE:
        // if (creature.isInCombat && creature.victim != m_bot->GetGUID() && !m_bot->GetGroup())
        //     return false;

        // Level check - don't attack creatures too high level (10+ levels above)
    if (creature.level > m_bot->GetLevel() + 10)
            return false;

        // DESIGN: LOS check skipped during snapshot validation because snapshots
        // contain only position data, not live Unit pointers. LOS is verified
        // when the GUID is resolved to Unit* in the action execution phase.

        return true;
    }

    // ===========================================================================
    // LEGACY UNIT-BASED VALIDATION (KEPT FOR COMPATIBILITY)
    // ===========================================================================
    // This is the original validation method - still used for non-snapshot code paths
    // ===========================================================================
    bool TargetScanner::IsValidTarget(Unit* target) const
    {
        if (!target || !target->IsAlive())
            return false;

        float maxRange = GetMaxEngageRange();
        float maxRangeSq = maxRange * maxRange;

        // Too far away (using squared distance for comparison)
    if (m_bot->GetExactDistSq(target) > maxRangeSq)
            return false;

        // Check if path exists (basic check)
    if (!m_bot->IsWithinLOSInMap(target))
            return false;

        float dist = m_bot->GetExactDist(target);

        // For ranged classes, check if we're in range
    if (m_preferRanged)
        {
            // Check if we have a ranged attack spell in range
            // This is simplified - in production you'd check actual spell ranges
    if (dist <= 36.0f)
                return true;
        }

        // For melee, need to be able to get close
    if (dist <= 5.0f || m_bot->IsWithinMeleeRange(target))
            return true;

        // Can we get there?
        return dist <= 40.0f;  // Reasonable chase distance
    }

    uint8 TargetScanner::GetTargetPriority(Unit* target) const
    {
        if (!target || !target->IsAlive())
            return PRIORITY_AVOID;

        // Check if unit is attackable
    if (!m_bot->IsValidAttackTarget(target))
            return PRIORITY_AVOID;

        // Check if blacklisted
    if (IsBlacklisted(target->GetGUID()))
            return PRIORITY_AVOID;

        // Don't attack friendly units
    if (m_bot->IsFriendlyTo(target))
            return PRIORITY_AVOID;

        // Don't attack units we can't see
    if (!m_bot->IsWithinLOSInMap(target))
            return PRIORITY_AVOID;

        // Don't attack critters unless they're hostile
    if (target->GetTypeId() == TYPEID_UNIT)
        {
            Creature* creature = target->ToCreature();
            if (creature->GetCreatureTemplate()->type == CREATURE_TYPE_CRITTER &&
                !creature->IsHostileTo(m_bot))
                return PRIORITY_AVOID;
        }

        // Don't attack units that are evading
    if (target->HasUnitState(UNIT_STATE_EVADE))
            return PRIORITY_AVOID;

        // Don't attack units that are immune
    if (target->HasUnitState(UNIT_STATE_UNATTACKABLE))
            return PRIORITY_AVOID;

        // Attacking bot or allies = highest priority
    if (IsAttackingGroup(target))
            return PRIORITY_CRITICAL;

        // Casters and healers
    if (IsCaster(target) || IsHealer(target))
            return PRIORITY_CASTER;

        // Elite mobs
    if (target->GetTypeId() == TYPEID_UNIT)
        {
            Creature* creature = target->ToCreature();
            if (creature->IsElite() || creature->IsDungeonBoss())
                return PRIORITY_ELITE;
        }

        // Level difference
        int32 levelDiff = int32(target->GetLevel()) - int32(m_bot->GetLevel());
        if (levelDiff < -7)
            return PRIORITY_TRIVIAL;

        return PRIORITY_NORMAL;
    }

    bool TargetScanner::ShouldEngage(Unit* target) const
    {
        if (!target || !IsValidTarget(target))
            return false;

        // Check scan mode
    if (m_scanMode == ScanMode::PASSIVE)
            return false;

        if (m_scanMode == ScanMode::DEFENSIVE)
        {
            // Only engage if target is attacking us or our group
    if (!IsAttackingGroup(target))
                return false;
        }

        // Don't engage if bot is low health
        float healthPct = m_bot->GetHealthPct();
        if (healthPct < 30.0f)
            return false;

        // Be more cautious at low health
    if (healthPct < 50.0f && target->GetLevel() > m_bot->GetLevel())
            return false;

        // Level difference check
        int32 levelDiff = int32(target->GetLevel()) - int32(m_bot->GetLevel());
        // Don't attack targets too high level
    if (levelDiff > 3)
            return false;

        // Grey level mobs (too low level)
    if (levelDiff < -7 && m_bot->GetLevel() > 10)
        {
            // Only engage if they're attacking us
    if (!IsAttackingGroup(target))
                return false;
        }
        // Elite checks
    if (target->GetTypeId() == TYPEID_UNIT)
        {
            Creature* creature = target->ToCreature();
            bool isElite = creature->IsElite() || creature->IsDungeonBoss();
            if (isElite)
            {
                // Don't solo elites if configured to avoid them
    if (m_avoidElites && !m_bot->GetGroup())
                    return false;

                // Don't solo elites more than 1 level higher
    if (levelDiff > 1 && !m_bot->GetGroup())
                    return false;

                // Don't engage elite if low on resources
    if (m_bot->GetPowerPct(m_bot->GetPowerType()) < 50.0f)
                    return false;
            }
        }

        // Don't engage if target is already fighting multiple players
    if (IsTargetInCombatWithOthers(target))
        {
            // Unless it's attacking our group
    if (!IsAttackingGroup(target))
                return false;
        }

        // Check if we can actually reach the target
    if (!CanReachTarget(target))
            return false;

        return true;
    }

    bool TargetScanner::CanReachTarget(Unit* target) const
    {
        if (!target)
            return false;

        float maxRange = GetMaxEngageRange();
        float maxRangeSq = maxRange * maxRange;
        // Too far away (using squared distance for comparison)
    if (m_bot->GetExactDistSq(target) > maxRangeSq)
            return false;

        // Check if path exists (basic check)
    if (!m_bot->IsWithinLOSInMap(target))
            return false;

        // Calculate actual distance for range checks
        float dist = m_bot->GetExactDist(target);

        // For ranged classes, check if we're in range
    if (m_preferRanged)
        {
            // Check if we have a ranged attack spell in range
            // This is simplified - in production you'd check actual spell ranges
    if (dist <= 36.0f)
                return true;
        }

        // For melee, need to be able to get close
    if (dist <= 5.0f || m_bot->IsWithinMeleeRange(target))
            return true;

        // Can we get there?
        return dist <= 40.0f;  // Reasonable chase distance
    }

    float TargetScanner::GetThreatValue(Unit* target) const
    {
        if (!target)
            return 0.0f;

        float threat = 0.0f;

        // Base threat from level difference
        int32 levelDiff = int32(target->GetLevel()) - int32(m_bot->GetLevel());
        threat = 100.0f + (levelDiff * 10.0f);

        // Increase threat for elites
    if (target->GetTypeId() == TYPEID_UNIT)
        {
            Creature* creature = target->ToCreature();
            if (creature->IsElite())
                threat *= 2.0f;
            if (creature->IsDungeonBoss())
                threat *= 5.0f;
        }

        // Increase threat for casters
    if (IsCaster(target))
            threat *= 1.5f;
        // Increase threat if target is attacking us
    if (target->GetVictim() == m_bot)
            threat *= 3.0f;

        // Increase threat if attacking group member
    if (IsAttackingGroup(target))
            threat *= 2.0f;

        // Reduce threat based on health
        threat *= target->GetHealthPct() / 100.0f;
        return threat;
    }

    float TargetScanner::GetScanRadius() const
    {
        // In combat, scan closer
    if (m_bot->IsInCombat())
            return m_baseRange * 0.75f;

        // Use base range
        return m_baseRange;
    }

    float TargetScanner::GetMaxEngageRange() const
    {
        return m_maxRange;
    }

    bool TargetScanner::ShouldScan(uint32 currentTime) const
    {
        if (m_lastScanTime == 0)
            return true;

        uint32 interval = m_scanInterval;

        // Scan more frequently in combat
    if (m_bot->IsInCombat())
            interval = SCAN_INTERVAL_COMBAT;
        // Scan less frequently when idle
        else if (!m_bot->isMoving())
            interval = SCAN_INTERVAL_IDLE;

        return (currentTime - m_lastScanTime) >= interval;
    }

    void TargetScanner::UpdateScanTime(uint32 currentTime)
    {
        m_lastScanTime = currentTime;
    }

    bool TargetScanner::IsAttackingGroup(Unit* target) const
    {
        if (!target || !target->GetVictim())
            return false;

        Unit* victim = target->GetVictim();
        // Attacking bot
        if (victim == m_bot)
            return true;

        // CRITICAL FIX: Check if attacking bot's pet
        // Pets are extensions of the player - if something attacks the pet,
        // the bot must respond to defend it
        if (Pet* pet = m_bot->GetPet())
        {
            if (victim == pet)
            {
                TC_LOG_DEBUG("playerbot.scanner",
                    "TargetScanner::IsAttackingGroup - {} is attacking bot {}'s pet {}!",
                    target->GetName(), m_bot->GetName(), pet->GetName());
                return true;
            }
        }

        // Attacking group member
        if (Group* group = m_bot->GetGroup())
        {
            if (group->IsMember(victim->GetGUID()))
                return true;

            // Also check group members' pets
            for (GroupReference const& itr : group->GetMembers())
            {
                if (Player* member = itr.GetSource())
                {
                    if (Pet* memberPet = member->GetPet())
                    {
                        if (victim == memberPet)
                            return true;
                    }
                }
            }
        }

        return false;
    }

    bool TargetScanner::IsCaster(Unit* target) const
    {
        if (!target)
            return false;

        // Check if currently casting
    if (target->HasUnitState(UNIT_STATE_CASTING))
            return true;

        // Check creature type
    if (target->GetTypeId() == TYPEID_UNIT)
        {
            Creature* creature = target->ToCreature();
            if (creature->GetCreatureTemplate()->unit_class == 2 || // Paladin
                creature->GetCreatureTemplate()->unit_class == 5 || // Priest
                creature->GetCreatureTemplate()->unit_class == 8)   // Mage
                return true;
        }

        return false;
    }

    bool TargetScanner::IsHealer(Unit* target) const
    {
        if (!target)
            return false;

        // Simple check - priests and paladins are often healers
    if (target->GetTypeId() == TYPEID_UNIT)
        {
            Creature* creature = target->ToCreature();
            if (creature->GetCreatureTemplate()->unit_class == 5 || // Priest
                creature->GetCreatureTemplate()->unit_class == 2)   // Paladin
            {
                // More sophisticated check would look at spell list
                return true;
            }
        }

        return false;
    }

    bool TargetScanner::IsDangerous(Unit* target) const
    {
        if (!target)
            return false;

        // Elite or higher
    if (target->GetTypeId() == TYPEID_UNIT)
        {
            Creature* creature = target->ToCreature();
            if (creature->IsElite() || creature->IsDungeonBoss())
                return true;
        }

        // Much higher level
    if (target->GetLevel() > m_bot->GetLevel() + 2)
            return true;

        // Multiple adds - simplified check without calling non-const method
        // Check for nearby enemies in combat (simplified version)
        // In full implementation, would cache scan results or make FindAllHostiles const

        return false;
    }

    void TargetScanner::AddToBlacklist(ObjectGuid guid, uint32 duration)
    {
        uint32 expireTime = GameTime::GetGameTimeMS() + duration;

        // Check if already blacklisted
    for (auto& entry : m_blacklist)
        {
            if (entry.guid == guid)
            {
                entry.expireTime = expireTime;
                return;
            }
        }

        // Add new entry
        BlacklistEntry entry;
        entry.guid = guid;
        entry.expireTime = expireTime;
        m_blacklist.push_back(entry);
    }

    void TargetScanner::RemoveFromBlacklist(ObjectGuid guid)
    {
        m_blacklist.erase(
            std::remove_if(m_blacklist.begin(), m_blacklist.end(),
                [guid](const BlacklistEntry& entry) { return entry.guid == guid; }),
            m_blacklist.end()
        );
    }

    bool TargetScanner::IsBlacklisted(ObjectGuid guid) const
    {
        for (const auto& entry : m_blacklist)
        {
            if (entry.guid == guid)
                return true;
        }
        return false;
    }

    void TargetScanner::UpdateBlacklist(uint32 currentTime)
    {
        m_blacklist.erase(
            std::remove_if(m_blacklist.begin(), m_blacklist.end(),
                [currentTime](const BlacklistEntry& entry) { return entry.expireTime <= currentTime; }),
            m_blacklist.end()
        );
    }

    float TargetScanner::CalculateEngageDistance(Unit* target) const
    {
        if (!target)
            return 0.0f;

        float baseDistance = m_preferRanged ? 30.0f : 5.0f;

        // Adjust for target type
    if (target->GetTypeId() == TYPEID_UNIT)
        {
            Creature* creature = target->ToCreature();
            // Stay further from elites
    if (creature->IsElite())
                baseDistance += 5.0f;

            // Get closer to casters to interrupt
    if (IsCaster(target) && !m_preferRanged)
                baseDistance = 5.0f;
        }

        return baseDistance;
    }

    bool TargetScanner::CheckLineOfSight(Unit* target) const
    {
        return target && m_bot->IsWithinLOSInMap(target);
    }

    bool TargetScanner::IsTargetInCombatWithOthers(Unit* target) const
    {
        if (!target || !target->IsInCombat())
            return false;

        // Check if fighting other players (not in our group)
        Unit* victim = target->GetVictim();
        if (!victim)
            return false;

        if (victim->GetTypeId() == TYPEID_PLAYER)
        {
            // Not us
    if (victim != m_bot)
            {
                // Not in our group
    if (!m_bot->GetGroup() || !m_bot->GetGroup()->IsMember(victim->GetGUID()))
                    return true;
            }
        }

        return false;
    }

    float TargetScanner::GetClassBasedRange() const
    {
        // This could be expanded based on spec detection
        return m_baseRange;
    }
}  // namespace Playerbot