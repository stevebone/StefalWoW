/*
 * Copyright (C) 2024 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "InnkeeperInteractionManager.h"
#include "Creature.h"
#include "DB2Stores.h"
#include "GameTime.h"
#include "Log.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "RestMgr.h"
#include "Spell.h"
#include "SpellAuras.h"
#include "SpellHistory.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "World.h"
#include "WorldSession.h"
#include "Spatial/DoubleBufferedSpatialGrid.h"
#include "Spatial/SpatialGridQueryHelpers.h"
#include "Spatial/SpatialGridManager.h"
#include <chrono>

namespace Playerbot
{

// ============================================================================
// PRIVATE HELPER METHOD DECLARATIONS (defined at end of file)
// ============================================================================

// These are implemented at the bottom of the file

// ============================================================================
// CONSTRUCTOR
// ============================================================================

InnkeeperInteractionManager::InnkeeperInteractionManager(Player* bot)
    : m_bot(bot)
    , m_stats()
    , m_restPreference(RestPreference::AUTO)
    , m_cpuUsage(0.0f)
    , m_totalInteractionTime(0)
    , m_interactionCount(0)
    , m_cachedHomebind()
    , m_lastHomebindCheck(0)
{
    if (m_bot)
    {
        // Initialize cached homebind info
        m_cachedHomebind = GetCurrentHomebind();
    }
}

// ============================================================================
// Core Innkeeper Methods
// ============================================================================

bool InnkeeperInteractionManager::BindHearthstone(Creature* innkeeper)
{
    if (!m_bot || !innkeeper)
        return false;

    auto startTime = std::chrono::high_resolution_clock::now();

    // Verify this is an innkeeper
    if (!IsInnkeeper(innkeeper))
    {
        TC_LOG_DEBUG("playerbot", "InnkeeperInteractionManager[{}]: {} is not an innkeeper",
            m_bot->GetName(), innkeeper->GetName());
        return false;
    }

    // Check distance
    static constexpr float INNKEEPER_INTERACTION_DISTANCE = 10.0f;
    if (m_bot->GetDistance(innkeeper) > INNKEEPER_INTERACTION_DISTANCE)
    {
        TC_LOG_DEBUG("playerbot", "InnkeeperInteractionManager[{}]: Too far from innkeeper {}",
            m_bot->GetName(), innkeeper->GetName());
        return false;
    }

    // Execute the bind
    bool success = ExecuteBind(innkeeper);

    if (success)
    {
        // Check if this is a new location
        HomebindInfo oldBind = m_cachedHomebind;
        m_cachedHomebind = HomebindInfo(); // Clear cache
        m_lastHomebindCheck = 0;
        HomebindInfo newBind = GetCurrentHomebind();

        bool isNewLocation = (oldBind.mapId != newBind.mapId ||
                              std::abs(oldBind.x - newBind.x) > 1.0f ||
                              std::abs(oldBind.y - newBind.y) > 1.0f);

        RecordBind(isNewLocation);

        TC_LOG_DEBUG("playerbot", "InnkeeperInteractionManager[{}]: Bound hearthstone at {} (new location: {})",
            m_bot->GetName(), innkeeper->GetName(), isNewLocation ? "yes" : "no");
    }

    // Track performance
    auto endTime = std::chrono::high_resolution_clock::now();
    uint32 duration = static_cast<uint32>(
        std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count());
    m_totalInteractionTime += duration;
    m_interactionCount++;
    m_cpuUsage = static_cast<float>(m_totalInteractionTime) / (m_interactionCount * 1000.0f);

    return success;
}

bool InnkeeperInteractionManager::ShouldBindHere(Creature* innkeeper) const
{
    if (!m_bot || !innkeeper)
        return false;

    if (!IsInnkeeper(innkeeper))
        return false;

    // Get current homebind
    HomebindInfo currentBind = GetCurrentHomebind();

    // If no valid homebind, always bind
    if (!currentBind.isValid)
        return true;

    // Evaluate this innkeeper
    InnEvaluation eval = EvaluateInnkeeper(innkeeper);

    return eval.isRecommended;
}

bool InnkeeperInteractionManager::SmartBind(Creature* innkeeper)
{
    if (!m_bot || !innkeeper)
        return false;

    // Check if this innkeeper is recommended
    InnEvaluation eval = EvaluateInnkeeper(innkeeper);

    if (eval.isCurrentBind)
    {
        TC_LOG_DEBUG("playerbot", "InnkeeperInteractionManager[{}]: Already bound at this innkeeper",
            m_bot->GetName());
        return true; // Already bound here, consider success
    }

    if (!eval.isRecommended)
    {
        TC_LOG_DEBUG("playerbot", "InnkeeperInteractionManager[{}]: Not binding at {} - {}",
            m_bot->GetName(), innkeeper->GetName(), eval.reason);
        return false;
    }

    return BindHearthstone(innkeeper);
}

bool InnkeeperInteractionManager::UseHearthstone()
{
    if (!m_bot)
        return false;

    // Check if hearthstone is ready
    if (!IsHearthstoneReady())
    {
        TC_LOG_DEBUG("playerbot", "InnkeeperInteractionManager[{}]: Hearthstone on cooldown ({} seconds remaining)",
            m_bot->GetName(), GetHearthstoneCooldown());
        return false;
    }

    // Check if bot has homebind set
    HomebindInfo homebind = GetCurrentHomebind();
    if (!homebind.isValid)
    {
        TC_LOG_DEBUG("playerbot", "InnkeeperInteractionManager[{}]: No valid homebind location",
            m_bot->GetName());
        return false;
    }

    // Check if already at homebind
    float distance = GetDistanceToHomebind();
    if (distance < 10.0f)
    {
        TC_LOG_DEBUG("playerbot", "InnkeeperInteractionManager[{}]: Already at homebind location",
            m_bot->GetName());
        return false;
    }

    // Cast hearthstone
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(HEARTHSTONE_SPELL_ID, DIFFICULTY_NONE);
    if (!spellInfo)
    {
        TC_LOG_ERROR("playerbot", "InnkeeperInteractionManager[{}]: Hearthstone spell {} not found",
            m_bot->GetName(), HEARTHSTONE_SPELL_ID);
        return false;
    }

    // Create and cast the spell
    Spell* spell = new Spell(m_bot, spellInfo, TRIGGERED_NONE);
    SpellCastTargets targets;
    targets.SetUnitTarget(m_bot);

    SpellCastResult result = spell->prepare(targets);
    bool success = (result == SPELL_CAST_OK);

    if (success)
    {
        RecordHearthstoneUse(true);
        TC_LOG_DEBUG("playerbot", "InnkeeperInteractionManager[{}]: Using hearthstone to return to {}",
            m_bot->GetName(), homebind.zoneName);
    }
    else
    {
        RecordHearthstoneUse(false);
        TC_LOG_DEBUG("playerbot", "InnkeeperInteractionManager[{}]: Failed to cast hearthstone (result: {})",
            m_bot->GetName(), static_cast<uint32>(result));
    }

    return success;
}

bool InnkeeperInteractionManager::IsHearthstoneReady() const
{
    if (!m_bot)
        return false;

    return !m_bot->GetSpellHistory()->HasCooldown(HEARTHSTONE_SPELL_ID);
}

uint32 InnkeeperInteractionManager::GetHearthstoneCooldown() const
{
    if (!m_bot)
        return 0;

    SpellHistory* history = m_bot->GetSpellHistory();
    if (!history)
        return 0;

    // Get remaining cooldown in milliseconds and convert to seconds
    auto cooldownEnd = history->GetRemainingCooldown(sSpellMgr->GetSpellInfo(HEARTHSTONE_SPELL_ID, DIFFICULTY_NONE));
    if (cooldownEnd <= Milliseconds::zero())
        return 0;

    return static_cast<uint32>(cooldownEnd.count() / 1000);
}

// ============================================================================
// Rested State Methods
// ============================================================================

bool InnkeeperInteractionManager::StartResting(Creature* innkeeper)
{
    if (!m_bot || !innkeeper)
        return false;

    if (!IsInnkeeper(innkeeper))
        return false;

    // Check if in range
    static constexpr float INNKEEPER_INTERACTION_DISTANCE = 10.0f;
    if (m_bot->GetDistance(innkeeper) > INNKEEPER_INTERACTION_DISTANCE)
        return false;

    // Set rested state - TrinityCore handles this automatically when near an innkeeper
    // but we can also trigger it manually via RestMgr
    m_bot->GetRestMgr().SetRestFlag(REST_FLAG_IN_TAVERN);

    m_stats.restingSessions++;

    TC_LOG_DEBUG("playerbot", "InnkeeperInteractionManager[{}]: Started resting at {}",
        m_bot->GetName(), innkeeper->GetName());

    return true;
}

bool InnkeeperInteractionManager::IsResting() const
{
    if (!m_bot)
        return false;

    return m_bot->GetRestMgr().HasRestFlag(REST_FLAG_IN_TAVERN);
}

uint32 InnkeeperInteractionManager::GetRestedBonus() const
{
    if (!m_bot)
        return 0;

    // Get rested XP bonus
    return static_cast<uint32>(m_bot->GetRestMgr().GetRestBonus(REST_TYPE_XP));
}

// ============================================================================
// Inn Analysis Methods
// ============================================================================

InnkeeperInteractionManager::HomebindInfo InnkeeperInteractionManager::GetCurrentHomebind() const
{
    if (!m_bot)
        return HomebindInfo();

    // Check cache
    uint32 currentTime = GameTime::GetGameTimeMS();
    if (m_lastHomebindCheck > 0 &&
        (currentTime - m_lastHomebindCheck) < HOMEBIND_CACHE_DURATION &&
        m_cachedHomebind.isValid)
    {
        return m_cachedHomebind;
    }

    HomebindInfo info;

    // Get homebind location from player's m_homebind member
    WorldLocation const& homebindLoc = m_bot->m_homebind;

    info.x = homebindLoc.GetPositionX();
    info.y = homebindLoc.GetPositionY();
    info.z = homebindLoc.GetPositionZ();
    info.mapId = homebindLoc.GetMapId();

    // Get zone ID from area ID
    info.zoneId = m_bot->m_homebindAreaId;

    // Get zone name
    if (AreaTableEntry const* area = sAreaTableStore.LookupEntry(info.zoneId))
    {
        info.zoneName = area->AreaName[sWorld->GetDefaultDbcLocale()];
    }
    else
    {
        info.zoneName = "Unknown";
    }

    // Validate - check if we have a valid position
    info.isValid = (info.mapId != 0 || (info.x != 0.0f && info.y != 0.0f));

    // Update cache
    m_cachedHomebind = info;
    m_lastHomebindCheck = currentTime;

    return info;
}

InnkeeperInteractionManager::InnEvaluation InnkeeperInteractionManager::EvaluateInnkeeper(Creature* innkeeper) const
{
    InnEvaluation eval;

    if (!m_bot || !innkeeper)
        return eval;

    eval.innkeeperGuid = innkeeper->GetGUID();
    eval.position = innkeeper->GetPosition();
    eval.mapId = innkeeper->GetMapId();
    eval.zoneId = innkeeper->GetZoneId();

    // Get current homebind
    HomebindInfo currentBind = GetCurrentHomebind();

    // Calculate distance from current bind
    if (currentBind.isValid)
    {
        float dx = eval.position.GetPositionX() - currentBind.x;
        float dy = eval.position.GetPositionY() - currentBind.y;
        eval.distanceFromCurrent = std::sqrt(dx * dx + dy * dy);

        // Check if this is the current bind location
        eval.isCurrentBind = (eval.mapId == currentBind.mapId && eval.distanceFromCurrent < 50.0f);
    }
    else
    {
        eval.distanceFromCurrent = 0.0f;
        eval.isCurrentBind = false;
    }

    // Calculate distance to quest zones
    eval.distanceToQuestZone = CalculateDistanceToQuestZones(eval.position, eval.mapId);

    // Determine if recommended
    DetermineRecommendation(eval, currentBind);

    return eval;
}

bool InnkeeperInteractionManager::IsAtInn() const
{
    if (!m_bot)
        return false;

    return m_bot->GetRestMgr().HasRestFlag(REST_FLAG_IN_TAVERN);
}

bool InnkeeperInteractionManager::IsInnkeeper(Creature* creature) const
{
    if (!creature)
        return false;

    // Check if creature has innkeeper NPC flag
    return creature->IsInnkeeper();
}

float InnkeeperInteractionManager::GetDistanceToHomebind() const
{
    if (!m_bot)
        return 0.0f;

    HomebindInfo homebind = GetCurrentHomebind();
    if (!homebind.isValid)
        return 0.0f;

    // If on different map, return large distance
    if (m_bot->GetMapId() != homebind.mapId)
        return 99999.0f;

    float dx = m_bot->GetPositionX() - homebind.x;
    float dy = m_bot->GetPositionY() - homebind.y;
    float dz = m_bot->GetPositionZ() - homebind.z;

    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

Creature* InnkeeperInteractionManager::FindNearestInnkeeper(float maxRange) const
{
    if (!m_bot || !m_bot->IsInWorld())
        return nullptr;

    Map* map = m_bot->GetMap();
    if (!map)
        return nullptr;

    // DEADLOCK FIX: Use lock-free spatial grid instead of Cell::VisitGridObjects
    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
    {
        sSpatialGridManager.CreateGrid(map);
        spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)
            return nullptr;
    }

    // Query nearby creature GUIDs (lock-free!)
    std::vector<ObjectGuid> nearbyGuids = spatialGrid->QueryNearbyCreatureGuids(
        m_bot->GetPosition(), maxRange);

    Creature* nearest = nullptr;
    float nearestDist = maxRange;

    // Check each creature for innkeeper flag
    for (ObjectGuid guid : nearbyGuids)
    {
        // Thread-safe spatial grid validation
        auto snapshot = SpatialGridQueryHelpers::FindCreatureByGuid(m_bot, guid);
        if (!snapshot)
            continue;

        // Get Creature* for NPC type detection
        Creature* creature = ObjectAccessor::GetCreature(*m_bot, guid);
        if (!creature || !creature->IsAlive() || creature->IsHostileTo(m_bot))
            continue;

        // Check for innkeeper flag
        if (!IsInnkeeper(creature))
            continue;

        float dist = m_bot->GetDistance(creature);
        if (dist < nearestDist)
        {
            nearest = creature;
            nearestDist = dist;
        }
    }

    return nearest;
}

// ============================================================================
// Strategic Methods
// ============================================================================

uint32 InnkeeperInteractionManager::GetRecommendedBindZone() const
{
    if (!m_bot)
        return 0;

    // Get active quest zones
    std::vector<uint32> questZones = GetActiveQuestZones();

    if (questZones.empty())
    {
        // No active quests - recommend current zone
        return m_bot->GetZoneId();
    }

    // Find the zone with most quests
    std::unordered_map<uint32, uint32> zoneCounts;
    for (uint32 zoneId : questZones)
    {
        zoneCounts[zoneId]++;
    }

    uint32 bestZone = 0;
    uint32 bestCount = 0;
    for (auto const& [zoneId, count] : zoneCounts)
    {
        if (count > bestCount)
        {
            bestZone = zoneId;
            bestCount = count;
        }
    }

    return bestZone;
}

bool InnkeeperInteractionManager::ShouldChangeBind(uint32 newZoneId) const
{
    if (!m_bot)
        return false;

    HomebindInfo currentBind = GetCurrentHomebind();

    // If no valid bind, always change
    if (!currentBind.isValid)
        return true;

    // If same zone, don't change
    if (currentBind.zoneId == newZoneId)
        return false;

    // Check if new zone is strategic
    uint32 recommendedZone = GetRecommendedBindZone();
    if (newZoneId == recommendedZone && currentBind.zoneId != recommendedZone)
        return true;

    return false;
}

// ============================================================================
// Statistics and Performance
// ============================================================================

size_t InnkeeperInteractionManager::GetMemoryUsage() const
{
    size_t usage = sizeof(*this);
    usage += m_cachedHomebind.zoneName.capacity();
    return usage;
}

// ============================================================================
// Private Helper Methods
// ============================================================================

bool InnkeeperInteractionManager::ExecuteBind(Creature* innkeeper)
{
    if (!m_bot || !innkeeper)
        return false;

    // Get innkeeper's position for the bind location
    WorldLocation bindLoc(innkeeper->GetMapId(),
                          innkeeper->GetPositionX(),
                          innkeeper->GetPositionY(),
                          innkeeper->GetPositionZ(),
                          innkeeper->GetOrientation());

    // Use TrinityCore's SetHomebind
    m_bot->SetHomebind(bindLoc, innkeeper->GetAreaId());

    // Save to database (with crash protection)
    // CRITICAL FIX (Item.cpp:1304 crash): Check for pending spell events before SaveToDB
    bool hasPendingEvents = !m_bot->m_Events.GetEvents().empty();
    bool isCurrentlyCasting = m_bot->GetCurrentSpell(CURRENT_GENERIC_SPELL) != nullptr ||
                              m_bot->GetCurrentSpell(CURRENT_CHANNELED_SPELL) != nullptr ||
                              m_bot->GetCurrentSpell(CURRENT_AUTOREPEAT_SPELL) != nullptr;

    if (!hasPendingEvents && !isCurrentlyCasting)
    {
        m_bot->SaveToDB();
    }
    // If bot is busy, homebind is still set - will be saved on next safe opportunity

    return true;
}

float InnkeeperInteractionManager::CalculateBindValue(Position const& position, uint32 mapId) const
{
    if (!m_bot)
        return 0.0f;

    float value = 50.0f; // Base value

    // Bonus for being close to quest objectives
    std::vector<uint32> questZones = GetActiveQuestZones();
    for (uint32 zoneId : questZones)
    {
        // Simplified: just check if same map
        if (mapId == m_bot->GetMapId())
            value += 10.0f;
    }

    // Bonus for being in a major city
    if (AreaTableEntry const* area = sAreaTableStore.LookupEntry(m_bot->GetZoneId()))
    {
        if (area->GetFlags().HasFlag(AreaFlags::AllowTradeChannel))
            value += 20.0f;
    }

    return value;
}

bool InnkeeperInteractionManager::IsStrategicLocation(Position const& position, uint32 mapId) const
{
    float bindValue = CalculateBindValue(position, mapId);
    return bindValue >= 60.0f; // Threshold for strategic location
}

std::vector<uint32> InnkeeperInteractionManager::GetActiveQuestZones() const
{
    std::vector<uint32> zones;

    if (!m_bot)
        return zones;

    // Get quest log and extract zone IDs from quest objectives
    for (uint8 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 questId = m_bot->GetQuestSlotQuestId(slot);
        if (!questId)
            continue;

        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        if (!quest)
            continue;

        // Get zone from quest area group
        int32 zoneOrSort = quest->GetZoneOrSort();
        if (zoneOrSort > 0)
        {
            zones.push_back(static_cast<uint32>(zoneOrSort));
        }
    }

    return zones;
}

void InnkeeperInteractionManager::RecordBind(bool isNewLocation)
{
    m_stats.hearthstoneBounds++;
    if (isNewLocation)
        m_stats.bindChanges++;
}

void InnkeeperInteractionManager::RecordHearthstoneUse(bool success)
{
    if (success)
        m_stats.hearthstoneUses++;
}

float InnkeeperInteractionManager::CalculateDistanceToQuestZones(Position const& position, uint32 mapId) const
{
    std::vector<uint32> questZones = GetActiveQuestZones();
    if (questZones.empty())
        return 0.0f;

    // Simplified: if same map as bot, consider close to quests
    if (m_bot && mapId == m_bot->GetMapId())
        return 100.0f; // Nominal distance

    return 99999.0f; // Far away if different map
}

void InnkeeperInteractionManager::DetermineRecommendation(InnEvaluation& eval, HomebindInfo const& currentBind) const
{
    // If no valid current bind, recommend binding
    if (!currentBind.isValid)
    {
        eval.isRecommended = true;
        eval.reason = "No current homebind - binding recommended";
        return;
    }

    // If already bound here, not recommended (no change needed)
    if (eval.isCurrentBind)
    {
        eval.isRecommended = false;
        eval.reason = "Already bound at this location";
        return;
    }

    // Check if this location is more strategic
    float currentValue = CalculateBindValue(
        Position(currentBind.x, currentBind.y, currentBind.z), currentBind.mapId);
    float newValue = CalculateBindValue(eval.position, eval.mapId);

    if (newValue > currentValue + 10.0f)
    {
        eval.isRecommended = true;
        eval.reason = "More strategic location for current objectives";
        return;
    }

    // Check distance - if much closer to quest zones
    if (eval.distanceToQuestZone < eval.distanceFromCurrent * 0.5f)
    {
        eval.isRecommended = true;
        eval.reason = "Closer to active quest zones";
        return;
    }

    eval.isRecommended = false;
    eval.reason = "Current homebind is adequate";
}

} // namespace Playerbot
