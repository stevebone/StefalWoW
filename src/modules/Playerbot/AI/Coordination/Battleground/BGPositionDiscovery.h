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
#include "Position.h"
#include "ObjectGuid.h"
#include <vector>
#include <map>
#include <optional>

class Battleground;
class BattlegroundMap;
class GameObject;
class Player;

namespace Playerbot::Coordination::Battleground
{

/**
 * @brief Position validation result
 */
struct PositionValidation
{
    bool isValid = false;           // Position has valid ground
    bool isReachable = false;       // Position is reachable via navmesh
    Position correctedPosition;     // Corrected position (ground Z, or nearest navmesh point)
    float distanceToOriginal = 0.0f;// Distance from original to corrected position
    std::string failureReason;      // Why position is invalid (for debugging)
};

/**
 * @brief Discovered POI information
 */
struct DiscoveredPOI
{
    uint32 id = 0;                  // POI identifier (objective ID, orb ID, etc.)
    std::string name;               // Human-readable name
    Position position;              // Actual position (validated)
    uint32 gameObjectEntry = 0;     // Associated game object entry (if any)
    ObjectGuid gameObjectGuid;      // GUID of discovered game object
    bool isValidated = false;       // Has been validated against navmesh
    bool isDynamic = false;         // Was discovered dynamically (vs hardcoded)
};

/**
 * @class BGPositionDiscovery
 * @brief Dynamic POI discovery and position validation for battlegrounds
 *
 * This system solves the problem of hardcoded positions that may be invalid or
 * unreachable on certain maps. It provides:
 *
 * 1. **Dynamic Game Object Discovery**: Find orbs, flags, and other objectives
 *    by querying the actual game objects on the map instead of using hardcoded coords.
 *
 * 2. **Position Validation**: Validate any position against:
 *    - Ground height (prevent floating/falling through)
 *    - Navmesh reachability (ensure bots can actually reach the position)
 *
 * 3. **Automatic Correction**: When a position is invalid, find the nearest
 *    valid/reachable position automatically.
 *
 * Usage:
 * @code
 *   BGPositionDiscovery discovery(battleground);
 *   discovery.Initialize();
 *
 *   // Discover orbs dynamically (ToK orb entries: Blue=212091, Purple=212092, Green=212093, Orange=212094)
 *   auto orbPOIs = discovery.DiscoverGameObjects({212091, 212092, 212093, 212094}, "Orb");
 *
 *   // Validate and correct a position
 *   Position targetPos(1850.17f, 1250.12f, 13.21f, 0.0f);  // Orange orb position
 *   auto validation = discovery.ValidatePosition(someBot, targetPos);
 *   if (validation.isReachable)
 *       MoveToPosition(bot, validation.correctedPosition);
 * @endcode
 */
class TC_GAME_API BGPositionDiscovery
{
public:
    explicit BGPositionDiscovery(::Battleground* bg);
    ~BGPositionDiscovery() = default;

    /**
     * @brief Initialize discovery system (call after BG map is available)
     * @return true if initialization succeeded
     */
    bool Initialize();

    // ========================================================================
    // POSITION VALIDATION
    // ========================================================================

    /**
     * @brief Validate a position for bot movement
     *
     * Checks:
     * 1. Ground height validity (not in void, underwater)
     * 2. Navmesh reachability from a reference point
     * 3. Corrects Z to actual ground level
     *
     * @param referenceBot Bot to use as reference for pathfinding
     * @param position Position to validate
     * @param maxCorrectionDistance Max distance to search for valid position (default 20y)
     * @return Validation result with corrected position if possible
     */
    PositionValidation ValidatePosition(::Player* referenceBot, Position const& position,
                                        float maxCorrectionDistance = 20.0f) const;

    /**
     * @brief Validate multiple positions and filter to reachable ones
     * @param referenceBot Bot for pathfinding reference
     * @param positions Input positions
     * @return Vector of validated/corrected positions (invalid ones removed)
     */
    std::vector<Position> ValidatePositions(::Player* referenceBot,
                                            std::vector<Position> const& positions) const;

    /**
     * @brief Check if a position is on the navmesh
     * @param position Position to check
     * @return true if position is on navmesh
     */
    bool IsOnNavmesh(Position const& position) const;

    /**
     * @brief Get the nearest navmesh point to a position
     * @param position Input position
     * @param searchRadius How far to search (default 10y)
     * @return Nearest navmesh point, or empty if none found
     */
    std::optional<Position> GetNearestNavmeshPoint(Position const& position,
                                                   float searchRadius = 10.0f) const;

    // ========================================================================
    // DYNAMIC GAME OBJECT DISCOVERY
    // ========================================================================

    /**
     * @brief Discover game objects by entry on the BG map
     *
     * Searches the entire BG map for game objects with the given entries.
     * Much more reliable than hardcoded positions.
     *
     * @param entries Vector of game object entries to find
     * @param namePrefix Name prefix for discovered POIs
     * @return Vector of discovered POIs with validated positions
     */
    std::vector<DiscoveredPOI> DiscoverGameObjects(std::vector<uint32> const& entries,
                                                   std::string const& namePrefix) const;

    /**
     * @brief Discover a single game object by entry
     * @param entry Game object entry
     * @param name POI name
     * @return Discovered POI if found, empty optional otherwise
     */
    std::optional<DiscoveredPOI> DiscoverGameObject(uint32 entry, std::string const& name) const;

    /**
     * @brief Find game object nearest to a position
     * @param entry Game object entry to find
     * @param nearPosition Search center
     * @param searchRadius Search radius (default 100y)
     * @return Game object if found, nullptr otherwise
     */
    ::GameObject* FindNearestGameObject(uint32 entry, Position const& nearPosition,
                                        float searchRadius = 100.0f) const;

    // ========================================================================
    // SPAWN POINT DISCOVERY
    // ========================================================================

    /**
     * @brief Get spawn positions from WorldSafeLocsEntry
     *
     * Uses TrinityCore's world_safe_locs table instead of hardcoded positions.
     *
     * @param faction ALLIANCE or HORDE
     * @return Vector of validated spawn positions
     */
    std::vector<Position> GetSpawnPositions(uint32 faction) const;

    /**
     * @brief Get graveyard positions for faction
     * @param faction ALLIANCE or HORDE
     * @return Vector of graveyard positions
     */
    std::vector<Position> GetGraveyardPositions(uint32 faction) const;

    // ========================================================================
    // CACHED DISCOVERIES
    // ========================================================================

    /**
     * @brief Get a cached discovered POI by ID
     * @param poiId POI identifier
     * @return Pointer to POI, or nullptr if not found
     */
    DiscoveredPOI const* GetDiscoveredPOI(uint32 poiId) const;

    /**
     * @brief Get all cached discovered POIs
     * @return Map of POI ID to DiscoveredPOI
     */
    std::map<uint32, DiscoveredPOI> const& GetAllDiscoveredPOIs() const { return _discoveredPOIs; }

    /**
     * @brief Cache a discovered POI
     * @param poi POI to cache
     */
    void CachePOI(DiscoveredPOI const& poi);

    /**
     * @brief Clear all cached discoveries (call when BG resets)
     */
    void ClearCache();

    // ========================================================================
    // DIAGNOSTICS
    // ========================================================================

    /**
     * @brief Log discovery diagnostics
     */
    void LogDiscoveryStatus() const;

    /**
     * @brief Get the BG map
     */
    ::BattlegroundMap* GetMap() const { return _map; }

private:
    ::Battleground* _battleground;
    ::BattlegroundMap* _map = nullptr;
    std::map<uint32, DiscoveredPOI> _discoveredPOIs;

    /**
     * @brief Correct Z coordinate to ground level
     */
    bool CorrectZToGround(Position& pos) const;

    /**
     * @brief Check path reachability between two points
     */
    bool IsPathReachable(Position const& from, Position const& to) const;
};

} // namespace Playerbot::Coordination::Battleground
