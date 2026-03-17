/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include <cstdint>
#include <string>
#include <vector>

namespace Playerbot {

/**
 * @enum DungeonState
 * @brief State machine for dungeon coordination
 */
enum class DungeonState : uint8
{
    IDLE = 0,           // Not in dungeon
    ENTERING = 1,       // Zoning into dungeon
    READY_CHECK = 2,    // Waiting for group ready
    CLEARING_TRASH = 3, // Fighting trash mobs
    PRE_BOSS = 4,       // Preparing for boss (rebuff, mana)
    BOSS_COMBAT = 5,    // Fighting boss
    POST_BOSS = 6,      // Looting, recovering after boss
    WIPED = 7,          // Group wiped, recovery needed
    COMPLETED = 8       // Dungeon complete
};

/**
 * @enum DungeonDifficulty
 * @brief Dungeon difficulty modes
 * @note WoW 12.0: Changed from uint8 to int16 to match core Difficulty type
 */
enum class DungeonDifficulty : int16
{
    NORMAL = 0,
    HEROIC = 1,
    MYTHIC = 2,
    MYTHIC_PLUS = 3
};

/**
 * @enum TrashPackPriority
 * @brief Priority levels for trash pack handling
 */
enum class TrashPackPriority : uint8
{
    SKIP = 0,       // Can be skipped
    OPTIONAL = 1,   // Kill if convenient
    REQUIRED = 2,   // Must kill for progress
    PATROL = 3,     // Patrol - timing dependent
    DANGEROUS = 4   // High priority dangerous pack
};

/**
 * @enum RaidMarker
 * @brief Raid marker assignments for targets
 */
enum class RaidMarker : uint8
{
    NONE = 0,
    SKULL = 1,      // Kill first
    CROSS = 2,      // Kill second
    MOON = 3,       // Polymorph/CC
    SQUARE = 4,     // Trap/CC
    TRIANGLE = 5,   // Sap/CC
    DIAMOND = 6,    // CC
    CIRCLE = 7,     // CC
    STAR = 8        // Misc
};

/**
 * @struct TrashPack
 * @brief Information about a trash pack in the dungeon
 */
struct TrashPack
{
    uint32 packId = 0;
    ::std::vector<ObjectGuid> members;
    TrashPackPriority priority = TrashPackPriority::REQUIRED;
    bool requiresCC = false;
    uint8 recommendedCCCount = 0;
    bool isPatrol = false;
    float patrolPathLength = 0.0f;
    bool canBePulledWith = false;  // Can combine with another pack
    uint32 linkedPackId = 0;       // Pack that comes if pulled
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    [[nodiscard]] bool IsEmpty() const { return members.empty(); }
    [[nodiscard]] uint32 GetMemberCount() const { return static_cast<uint32>(members.size()); }
};

/**
 * @struct BossInfo
 * @brief Information about a boss encounter
 */
struct BossInfo
{
    uint32 bossId = 0;
    uint32 encounterId = 0;
    ::std::string name;
    uint8 currentPhase = 0;
    uint8 maxPhases = 1;
    bool hasEnrageTimer = false;
    uint32 enrageTimeMs = 0;
    uint32 combatStartTime = 0;
    ::std::vector<uint32> importantSpellIds;  // Spells to interrupt/avoid
    ::std::vector<uint32> tankSwapSpellIds;   // Spells requiring tank swap
    float healthPercent = 100.0f;

    [[nodiscard]] bool IsInCombat() const { return combatStartTime > 0; }
    [[nodiscard]] uint32 GetCombatDuration(uint32 currentTime) const
    {
        return combatStartTime > 0 ? (currentTime - combatStartTime) : 0;
    }
};

/**
 * @struct DungeonProgress
 * @brief Tracks progress through the dungeon
 */
struct DungeonProgress
{
    uint32 dungeonId = 0;
    uint32 instanceId = 0;
    DungeonDifficulty difficulty = DungeonDifficulty::NORMAL;
    uint8 bossesKilled = 0;
    uint8 totalBosses = 0;
    uint32 trashKilled = 0;
    uint32 totalTrash = 0;
    float completionPercent = 0.0f;

    // Mythic+ specific
    bool isMythicPlus = false;
    uint8 keystoneLevel = 0;
    uint32 timeLimit = 0;
    uint32 elapsedTime = 0;
    uint32 deathCount = 0;
    float enemyForcesPercent = 0.0f;
    float requiredEnemyForces = 100.0f;

    [[nodiscard]] float GetBossProgress() const
    {
        return totalBosses > 0 ? (static_cast<float>(bossesKilled) / totalBosses * 100.0f) : 0.0f;
    }

    [[nodiscard]] bool IsComplete() const
    {
        return bossesKilled >= totalBosses;
    }

    [[nodiscard]] bool IsMythicPlusComplete() const
    {
        return isMythicPlus && IsComplete() && enemyForcesPercent >= requiredEnemyForces;
    }
};

/**
 * @brief Convert DungeonState to string
 */
inline const char* DungeonStateToString(DungeonState state)
{
    switch (state)
    {
        case DungeonState::IDLE:           return "IDLE";
        case DungeonState::ENTERING:       return "ENTERING";
        case DungeonState::READY_CHECK:    return "READY_CHECK";
        case DungeonState::CLEARING_TRASH: return "CLEARING_TRASH";
        case DungeonState::PRE_BOSS:       return "PRE_BOSS";
        case DungeonState::BOSS_COMBAT:    return "BOSS_COMBAT";
        case DungeonState::POST_BOSS:      return "POST_BOSS";
        case DungeonState::WIPED:          return "WIPED";
        case DungeonState::COMPLETED:      return "COMPLETED";
        default:                           return "UNKNOWN";
    }
}

/**
 * @brief Convert DungeonDifficulty to string
 */
inline const char* DifficultyToString(DungeonDifficulty diff)
{
    switch (diff)
    {
        case DungeonDifficulty::NORMAL:      return "Normal";
        case DungeonDifficulty::HEROIC:      return "Heroic";
        case DungeonDifficulty::MYTHIC:      return "Mythic";
        case DungeonDifficulty::MYTHIC_PLUS: return "Mythic+";
        default:                              return "Unknown";
    }
}

/**
 * @brief Convert TrashPackPriority to string
 */
inline const char* TrashPackPriorityToString(TrashPackPriority priority)
{
    switch (priority)
    {
        case TrashPackPriority::SKIP:      return "Skip";
        case TrashPackPriority::OPTIONAL:  return "Optional";
        case TrashPackPriority::REQUIRED:  return "Required";
        case TrashPackPriority::PATROL:    return "Patrol";
        case TrashPackPriority::DANGEROUS: return "Dangerous";
        default:                            return "Unknown";
    }
}

/**
 * @brief Convert RaidMarker to string
 */
inline const char* RaidMarkerToString(RaidMarker marker)
{
    switch (marker)
    {
        case RaidMarker::NONE:     return "None";
        case RaidMarker::SKULL:    return "Skull";
        case RaidMarker::CROSS:    return "Cross";
        case RaidMarker::MOON:     return "Moon";
        case RaidMarker::SQUARE:   return "Square";
        case RaidMarker::TRIANGLE: return "Triangle";
        case RaidMarker::DIAMOND:  return "Diamond";
        case RaidMarker::CIRCLE:   return "Circle";
        case RaidMarker::STAR:     return "Star";
        default:                    return "Unknown";
    }
}

} // namespace Playerbot
