/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_CONTENT_CONTEXT_MANAGER_H
#define PLAYERBOT_CONTENT_CONTEXT_MANAGER_H

#include "Define.h"
#include "ObjectGuid.h"
#include <unordered_map>
#include <vector>
#include <mutex>

namespace Playerbot
{

/**
 * @brief Content type classification
 */
enum class ContentType : uint8
{
    SOLO,                   // Solo play, no coordination needed
    OPEN_WORLD_GROUP,       // Open world group, light coordination
    DUNGEON_NORMAL,         // Normal dungeon
    DUNGEON_HEROIC,         // Heroic dungeon
    MYTHIC_ZERO,            // Mythic dungeon (no timer)
    MYTHIC_PLUS,            // Mythic+ with timer/affixes
    RAID_LFR,               // Looking For Raid
    RAID_NORMAL,            // Normal raid
    RAID_HEROIC,            // Heroic raid
    RAID_MYTHIC,            // Mythic raid
    ARENA_2V2,              // 2v2 Arena
    ARENA_3V3,              // 3v3 Arena
    ARENA_5V5,              // 5v5 Arena (if applicable)
    BATTLEGROUND,           // Random/Casual battleground
    RATED_BATTLEGROUND,     // Rated battleground
    UNKNOWN
};

/**
 * @brief Content context information for a group/player
 */
struct ContentContext
{
    ContentType type{ContentType::UNKNOWN};
    float coordinationLevel{0.0f};      // 0.0 = none, 1.0 = maximum
    uint32 groupSize{0};
    uint32 mapId{0};
    uint32 difficultyId{0};
    uint32 mythicPlusLevel{0};          // 0 if not M+
    std::vector<uint32> activeAffixes;  // M+ affixes
    uint32 encounterEntry{0};           // Current boss entry (0 if trash)
    bool inCombat{false};
    bool isPvP{false};

    /**
     * @brief Get recommended features based on content type
     */
    bool RequiresInterruptCoordination() const
    {
        return coordinationLevel >= 0.3f;
    }

    bool RequiresDispelCoordination() const
    {
        return coordinationLevel >= 0.3f;
    }

    bool RequiresDefensiveCoordination() const
    {
        return coordinationLevel >= 0.5f;
    }

    bool RequiresTankSwapCoordination() const
    {
        return type >= ContentType::RAID_NORMAL && type <= ContentType::RAID_MYTHIC;
    }

    bool RequiresCCChaining() const
    {
        return isPvP && type >= ContentType::ARENA_2V2;
    }

    bool RequiresBurstCoordination() const
    {
        return isPvP || coordinationLevel >= 0.7f;
    }

    bool HasAffixes() const
    {
        return !activeAffixes.empty();
    }

    bool HasAffix(uint32 affixId) const
    {
        for (uint32 id : activeAffixes)
        {
            if (id == affixId)
                return true;
        }
        return false;
    }
};

/**
 * @brief Content Context Manager
 *
 * Detects the current content type based on map, instance, difficulty,
 * and provides appropriate coordination context to all bot systems.
 *
 * Thread Safety: All public methods are thread-safe.
 *
 * Usage:
 *   // Get context for a player
 *   ContentContext ctx = ContentContextManager::instance()->GetContext(playerGuid);
 *
 *   // Check if features are needed
 *   if (ctx.RequiresInterruptCoordination()) { ... }
 *
 *   // Check for specific M+ affix
 *   if (ctx.HasAffix(AFFIX_SANGUINE)) { ... }
 */
class ContentContextManager
{
public:
    static ContentContextManager* instance()
    {
        static ContentContextManager instance;
        return &instance;
    }

    /**
     * @brief Get the content context for a player
     *
     * @param playerGuid The player's GUID
     * @return ContentContext for the player's current content
     */
    ContentContext GetContext(ObjectGuid playerGuid) const;

    /**
     * @brief Update the context for a player
     *
     * Called when a player enters new content or their situation changes.
     *
     * @param playerGuid The player's GUID
     * @param mapId Current map ID
     * @param difficultyId Current difficulty
     * @param inInstance True if in an instance
     * @param inBattleground True if in a battleground
     * @param inArena True if in an arena
     */
    void UpdateContext(ObjectGuid playerGuid, uint32 mapId, uint32 difficultyId,
                       bool inInstance, bool inBattleground, bool inArena);

    /**
     * @brief Update M+ context
     *
     * @param playerGuid The player's GUID
     * @param keystoneLevel The M+ keystone level
     * @param affixes Active affix IDs
     */
    void UpdateMythicPlusContext(ObjectGuid playerGuid, uint32 keystoneLevel,
                                  std::vector<uint32> const& affixes);

    /**
     * @brief Update encounter context
     *
     * @param playerGuid The player's GUID
     * @param encounterEntry Current boss entry (0 if trash)
     */
    void UpdateEncounterContext(ObjectGuid playerGuid, uint32 encounterEntry);

    /**
     * @brief Update combat state
     *
     * @param playerGuid The player's GUID
     * @param inCombat True if in combat
     */
    void UpdateCombatState(ObjectGuid playerGuid, bool inCombat);

    /**
     * @brief Clear context for a player (on logout/disconnect)
     *
     * @param playerGuid The player's GUID
     */
    void ClearContext(ObjectGuid playerGuid);

    /**
     * @brief Get coordination level for a content type
     *
     * @param type The content type
     * @return Coordination level (0.0 - 1.0)
     */
    static float GetCoordinationLevel(ContentType type);

    /**
     * @brief Get content type name for logging
     */
    static char const* GetContentTypeName(ContentType type);

private:
    ContentContextManager() = default;
    ~ContentContextManager() = default;

    // Disable copy/move
    ContentContextManager(ContentContextManager const&) = delete;
    ContentContextManager& operator=(ContentContextManager const&) = delete;

    ContentType DetermineContentType(uint32 mapId, uint32 difficultyId,
                                     bool inInstance, bool inBattleground, bool inArena,
                                     uint32 mythicPlusLevel) const;

    std::unordered_map<ObjectGuid, ContentContext> _contexts;
    mutable std::mutex _mutex;
};

// Common M+ Affix IDs for reference
namespace MythicPlusAffixId
{
    constexpr uint32 FORTIFIED = 10;
    constexpr uint32 TYRANNICAL = 9;
    constexpr uint32 BOLSTERING = 7;
    constexpr uint32 BURSTING = 11;
    constexpr uint32 INSPIRING = 122;
    constexpr uint32 NECROTIC = 4;
    constexpr uint32 QUAKING = 14;
    constexpr uint32 RAGING = 6;
    constexpr uint32 SANGUINE = 8;
    constexpr uint32 SPITEFUL = 123;
    constexpr uint32 STORMING = 124;
    constexpr uint32 VOLCANIC = 3;
    constexpr uint32 EXPLOSIVE = 13;
    constexpr uint32 GRIEVOUS = 12;
    constexpr uint32 INCORPOREAL = 136;
    constexpr uint32 AFFLICTED = 135;
    constexpr uint32 ENTANGLING = 134;
}

} // namespace Playerbot

#endif // PLAYERBOT_CONTENT_CONTEXT_MANAGER_H
