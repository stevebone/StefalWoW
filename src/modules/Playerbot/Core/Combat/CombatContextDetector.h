/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Define.h"

class Player;
class Group;
class Map;

namespace Playerbot
{

/**
 * @brief Combat context types for bot behavior optimization
 *
 * Different contexts require different update frequencies, threat tracking,
 * and coordination levels. This enum enables context-aware performance tuning.
 */
enum class CombatContext : uint8
{
    SOLO = 0,           // No group, no coordination needed
    GROUP = 1,          // 2-5 players, basic coordination
    DUNGEON = 2,        // Instanced 5-man, tighter coordination
    RAID = 3,           // 10-40 players, complex coordination
    ARENA = 4,          // PvP 2v2/3v3/5v5, fast reactions needed
    BATTLEGROUND = 5    // Large scale PvP, objective focus
};

/**
 * @brief Detects and provides information about combat context
 *
 * This utility class provides static methods for detecting the current
 * combat context of a bot and determining appropriate behavior parameters.
 *
 * Performance impact: Minimal - simple checks, no allocations
 */
class TC_GAME_API CombatContextDetector
{
public:
    /**
     * @brief Detect the current combat context for a player
     *
     * Priority: Arena > BG > Raid > Dungeon > Group > Solo
     *
     * @param player The player to check
     * @return CombatContext The detected context
     */
    static CombatContext Detect(Player* player);

    /**
     * @brief Convert context to string for logging/debug
     *
     * @param ctx Combat context
     * @return const char* Human-readable context name
     */
    static const char* ToString(CombatContext ctx)
    {
        switch (ctx)
        {
            case CombatContext::SOLO:        return "Solo";
            case CombatContext::GROUP:       return "Group";
            case CombatContext::DUNGEON:     return "Dungeon";
            case CombatContext::RAID:        return "Raid";
            case CombatContext::ARENA:       return "Arena";
            case CombatContext::BATTLEGROUND: return "Battleground";
            default:                         return "Unknown";
        }
    }

    /**
     * @brief Check if context requires group coordination
     *
     * Solo bots can skip coordination overhead entirely.
     *
     * @param ctx Combat context
     * @return true if coordination is needed
     */
    static bool RequiresCoordination(CombatContext ctx)
    {
        return ctx != CombatContext::SOLO;
    }

    /**
     * @brief Check if context is PvP
     *
     * PvP contexts need faster reactions and different strategies.
     *
     * @param ctx Combat context
     * @return true if this is a PvP context
     */
    static bool IsPvP(CombatContext ctx)
    {
        return ctx == CombatContext::ARENA || ctx == CombatContext::BATTLEGROUND;
    }

    /**
     * @brief Check if context is instanced content
     *
     * Instanced content has predictable mechanics and spawns.
     *
     * @param ctx Combat context
     * @return true if instanced
     */
    static bool IsInstanced(CombatContext ctx)
    {
        return ctx == CombatContext::DUNGEON ||
               ctx == CombatContext::RAID ||
               ctx == CombatContext::ARENA ||
               ctx == CombatContext::BATTLEGROUND;
    }

    /**
     * @brief Get recommended update interval for strategy/behavior updates
     *
     * Balances responsiveness vs CPU usage based on context.
     * Faster updates for PvP, slower for solo grinding.
     *
     * @param ctx Combat context
     * @return uint32 Recommended update interval in milliseconds
     */
    static uint32 GetRecommendedUpdateInterval(CombatContext ctx)
    {
        switch (ctx)
        {
            case CombatContext::ARENA:       return 25;   // 40 TPS - fast PvP reactions
            case CombatContext::BATTLEGROUND: return 50;  // 20 TPS - larger scale PvP
            case CombatContext::DUNGEON:     return 75;   // ~13 TPS - mechanics timing
            case CombatContext::RAID:        return 100;  // 10 TPS - balance CPU
            case CombatContext::GROUP:       return 100;  // 10 TPS - standard
            case CombatContext::SOLO:        return 150;  // ~7 TPS - relaxed
            default:                         return 100;
        }
    }

    /**
     * @brief Get maximum threat entries to track based on context
     *
     * Limits memory usage while maintaining needed precision.
     * More entries needed in raids with many adds.
     *
     * @param ctx Combat context
     * @return uint32 Maximum threat entries to maintain
     */
    static uint32 GetMaxThreatEntries(CombatContext ctx)
    {
        switch (ctx)
        {
            case CombatContext::SOLO:        return 10;   // Few enemies
            case CombatContext::GROUP:       return 20;   // Small pulls
            case CombatContext::DUNGEON:     return 25;   // Dungeon packs
            case CombatContext::RAID:        return 50;   // Many adds possible
            case CombatContext::ARENA:       return 15;   // Limited targets
            case CombatContext::BATTLEGROUND: return 30;  // Medium scale
            default:                         return 20;
        }
    }

    /**
     * @brief Get strategy relevance multiplier for context
     *
     * Used to adjust strategy activation based on context.
     * Combat strategies more relevant in dungeons/raids.
     *
     * @param ctx Combat context
     * @return float Relevance multiplier (1.0 = normal)
     */
    static float GetCombatRelevanceMultiplier(CombatContext ctx)
    {
        switch (ctx)
        {
            case CombatContext::ARENA:       return 2.0f;  // Combat is everything
            case CombatContext::BATTLEGROUND: return 1.5f; // Combat + objectives
            case CombatContext::RAID:        return 1.5f;  // Combat focused
            case CombatContext::DUNGEON:     return 1.3f;  // Combat + movement
            case CombatContext::GROUP:       return 1.0f;  // Balanced
            case CombatContext::SOLO:        return 0.8f;  // Quest/grind focus
            default:                         return 1.0f;
        }
    }

    /**
     * @brief Check if formations should be used
     *
     * Solo bots don't need formation calculations.
     *
     * @param ctx Combat context
     * @return true if formations apply
     */
    static bool ShouldUseFormations(CombatContext ctx)
    {
        return ctx != CombatContext::SOLO;
    }
};

} // namespace Playerbot
