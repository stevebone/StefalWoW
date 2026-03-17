/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "ContentContextManager.h"
#include "Log.h"

namespace Playerbot
{

ContentContext ContentContextManager::GetContext(ObjectGuid playerGuid) const
{
    std::lock_guard lock(_mutex);

    auto it = _contexts.find(playerGuid);
    if (it != _contexts.end())
        return it->second;

    // Return default solo context
    ContentContext ctx;
    ctx.type = ContentType::SOLO;
    ctx.coordinationLevel = 0.0f;
    return ctx;
}

void ContentContextManager::UpdateContext(ObjectGuid playerGuid, uint32 mapId, uint32 difficultyId,
                                          bool inInstance, bool inBattleground, bool inArena)
{
    std::lock_guard lock(_mutex);

    ContentContext& ctx = _contexts[playerGuid];
    ctx.mapId = mapId;
    ctx.difficultyId = difficultyId;
    ctx.isPvP = inBattleground || inArena;

    ctx.type = DetermineContentType(mapId, difficultyId, inInstance, inBattleground, inArena,
                                    ctx.mythicPlusLevel);
    ctx.coordinationLevel = GetCoordinationLevel(ctx.type);

    TC_LOG_DEBUG("playerbot.coordination", "ContentContext updated for {}: type={}, level={:.2f}",
        playerGuid.ToString(), GetContentTypeName(ctx.type), ctx.coordinationLevel);
}

void ContentContextManager::UpdateMythicPlusContext(ObjectGuid playerGuid, uint32 keystoneLevel,
                                                     std::vector<uint32> const& affixes)
{
    std::lock_guard lock(_mutex);

    ContentContext& ctx = _contexts[playerGuid];
    ctx.mythicPlusLevel = keystoneLevel;
    ctx.activeAffixes = affixes;

    if (keystoneLevel > 0)
    {
        ctx.type = ContentType::MYTHIC_PLUS;
        ctx.coordinationLevel = GetCoordinationLevel(ContentType::MYTHIC_PLUS);

        // Increase coordination level for higher keys
        if (keystoneLevel >= 15)
            ctx.coordinationLevel = std::min(1.0f, ctx.coordinationLevel + 0.1f);
        if (keystoneLevel >= 20)
            ctx.coordinationLevel = std::min(1.0f, ctx.coordinationLevel + 0.1f);
    }

    TC_LOG_DEBUG("playerbot.coordination", "M+ context updated for {}: level={}, affixes={}",
        playerGuid.ToString(), keystoneLevel, affixes.size());
}

void ContentContextManager::UpdateEncounterContext(ObjectGuid playerGuid, uint32 encounterEntry)
{
    std::lock_guard lock(_mutex);

    auto it = _contexts.find(playerGuid);
    if (it != _contexts.end())
    {
        it->second.encounterEntry = encounterEntry;

        TC_LOG_DEBUG("playerbot.coordination", "Encounter context updated for {}: entry={}",
            playerGuid.ToString(), encounterEntry);
    }
}

void ContentContextManager::UpdateCombatState(ObjectGuid playerGuid, bool inCombat)
{
    std::lock_guard lock(_mutex);

    auto it = _contexts.find(playerGuid);
    if (it != _contexts.end())
    {
        it->second.inCombat = inCombat;
    }
}

void ContentContextManager::ClearContext(ObjectGuid playerGuid)
{
    std::lock_guard lock(_mutex);
    _contexts.erase(playerGuid);

    TC_LOG_DEBUG("playerbot.coordination", "Context cleared for {}", playerGuid.ToString());
}

ContentType ContentContextManager::DetermineContentType(uint32 mapId, uint32 difficultyId,
                                                         bool inInstance, bool inBattleground, bool inArena,
                                                         uint32 mythicPlusLevel) const
{
    // Arena detection
    if (inArena)
    {
        // Would need to check arena bracket from ArenaTeam
        return ContentType::ARENA_3V3;  // Default to 3v3
    }

    // Battleground detection
    if (inBattleground)
    {
        // Would need to check rated status from Battleground
        return ContentType::BATTLEGROUND;
    }

    // Instance detection
    if (inInstance)
    {
        // M+ detection
        if (mythicPlusLevel > 0)
            return ContentType::MYTHIC_PLUS;

        // Difficulty-based detection
        // See DBCEnums.h for Difficulty enum values:
        // DIFFICULTY_NORMAL = 1, DIFFICULTY_HEROIC = 2
        // DIFFICULTY_10_N = 3, DIFFICULTY_25_N = 4
        // DIFFICULTY_10_HC = 5, DIFFICULTY_25_HC = 6
        // DIFFICULTY_LFR = 7
        // DIFFICULTY_MYTHIC_KEYSTONE = 8
        // DIFFICULTY_MYTHIC = 16
        // etc.

        switch (difficultyId)
        {
            case 1:  // DIFFICULTY_NORMAL
                return ContentType::DUNGEON_NORMAL;
            case 2:  // DIFFICULTY_HEROIC
                return ContentType::DUNGEON_HEROIC;
            case 8:  // DIFFICULTY_MYTHIC_KEYSTONE
                return ContentType::MYTHIC_PLUS;
            case 23: // DIFFICULTY_MYTHIC_DUNGEON
                return ContentType::MYTHIC_ZERO;
            case 7:  // DIFFICULTY_LFR
            case 17: // DIFFICULTY_LFR_NEW
                return ContentType::RAID_LFR;
            case 14: // DIFFICULTY_NORMAL_RAID
                return ContentType::RAID_NORMAL;
            case 15: // DIFFICULTY_HEROIC_RAID
                return ContentType::RAID_HEROIC;
            case 16: // DIFFICULTY_MYTHIC_RAID
                return ContentType::RAID_MYTHIC;
            default:
                // Check map type for dungeon/raid
                // This is simplified - would need MapEntry lookup
                return ContentType::DUNGEON_NORMAL;
        }
    }

    // Open world
    return ContentType::SOLO;
}

float ContentContextManager::GetCoordinationLevel(ContentType type)
{
    switch (type)
    {
        case ContentType::SOLO:
            return 0.0f;
        case ContentType::OPEN_WORLD_GROUP:
            return 0.2f;
        case ContentType::DUNGEON_NORMAL:
            return 0.3f;
        case ContentType::DUNGEON_HEROIC:
            return 0.4f;
        case ContentType::MYTHIC_ZERO:
            return 0.5f;
        case ContentType::MYTHIC_PLUS:
            return 0.7f;
        case ContentType::RAID_LFR:
            return 0.4f;
        case ContentType::RAID_NORMAL:
            return 0.6f;
        case ContentType::RAID_HEROIC:
            return 0.8f;
        case ContentType::RAID_MYTHIC:
            return 1.0f;
        case ContentType::ARENA_2V2:
            return 0.8f;
        case ContentType::ARENA_3V3:
            return 0.9f;
        case ContentType::ARENA_5V5:
            return 0.9f;
        case ContentType::BATTLEGROUND:
            return 0.5f;
        case ContentType::RATED_BATTLEGROUND:
            return 0.7f;
        default:
            return 0.0f;
    }
}

char const* ContentContextManager::GetContentTypeName(ContentType type)
{
    switch (type)
    {
        case ContentType::SOLO: return "SOLO";
        case ContentType::OPEN_WORLD_GROUP: return "OPEN_WORLD_GROUP";
        case ContentType::DUNGEON_NORMAL: return "DUNGEON_NORMAL";
        case ContentType::DUNGEON_HEROIC: return "DUNGEON_HEROIC";
        case ContentType::MYTHIC_ZERO: return "MYTHIC_ZERO";
        case ContentType::MYTHIC_PLUS: return "MYTHIC_PLUS";
        case ContentType::RAID_LFR: return "RAID_LFR";
        case ContentType::RAID_NORMAL: return "RAID_NORMAL";
        case ContentType::RAID_HEROIC: return "RAID_HEROIC";
        case ContentType::RAID_MYTHIC: return "RAID_MYTHIC";
        case ContentType::ARENA_2V2: return "ARENA_2V2";
        case ContentType::ARENA_3V3: return "ARENA_3V3";
        case ContentType::ARENA_5V5: return "ARENA_5V5";
        case ContentType::BATTLEGROUND: return "BATTLEGROUND";
        case ContentType::RATED_BATTLEGROUND: return "RATED_BATTLEGROUND";
        case ContentType::UNKNOWN: return "UNKNOWN";
        default: return "INVALID";
    }
}

} // namespace Playerbot
