/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _PLAYERBOT_PORTAL_DATABASE_H
#define _PLAYERBOT_PORTAL_DATABASE_H

#include "Common.h"
#include "ObjectGuid.h"
#include "Position.h"
#include "SharedDefines.h"
#include "Threading/LockHierarchy.h"
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

class Player;
class GameObject;

namespace Playerbot
{

/**
 * @enum PortalType
 * @brief Classification of portal objects in the game world
 */
enum class PortalType : uint8
{
    NONE                = 0,
    MAGE_PORTAL         = 1,    ///< Player-created mage portal (temporary)
    STATIC_PORTAL       = 2,    ///< Permanent world portal (capital cities, etc.)
    DUNGEON_PORTAL      = 3,    ///< Instance/dungeon entrance portal
    BATTLEGROUND_PORTAL = 4,    ///< Battleground queue portal
    EXPANSION_PORTAL    = 5,    ///< Expansion-specific portals (Dark Portal, etc.)
    EVENT_PORTAL        = 6,    ///< Event-related temporary portals
    ENGINEERING_PORTAL  = 7,    ///< Engineering-crafted portals
    PROFESSION_PORTAL   = 8,    ///< Profession-specific portals (archaeology, etc.)
};

/**
 * @enum PortalFaction
 * @brief Faction restriction for portal usage
 */
enum class PortalFaction : uint8
{
    NEUTRAL   = 0,    ///< Both factions can use
    ALLIANCE  = 1,    ///< Alliance only
    HORDE     = 2,    ///< Horde only
};

/**
 * @struct PortalInfo
 * @brief Complete information about a portal in the game world
 *
 * Contains all data needed for bot navigation to and through portals:
 * - Location information (where to find the portal)
 * - Destination information (where it leads)
 * - Metadata (faction, level requirements, etc.)
 *
 * Performance: ~128 bytes per portal instance
 * Thread-safety: Read-only after construction, safe for concurrent access
 */
struct PortalInfo
{
    /// Unique identifier for this portal entry
    uint32 portalId = 0;

    /// GameObject entry from gameobject_template
    uint32 gameObjectEntry = 0;

    /// Portal classification
    PortalType type = PortalType::NONE;

    /// Human-readable name for logging and debugging
    ::std::string name;

    // ========== Source Location ==========

    /// Map ID where this portal is located
    uint32 sourceMapId = 0;

    /// Position of the portal in the source map
    Position sourcePosition;

    /// Zone ID where this portal is located (for spatial indexing)
    uint32 sourceZoneId = 0;

    /// Area ID for more specific location tracking
    uint32 sourceAreaId = 0;

    // ========== Destination Location ==========

    /// Map ID where this portal leads to
    uint32 destinationMapId = 0;

    /// Position at the destination (from spell_target_position)
    Position destinationPosition;

    /// Zone ID at destination (for route planning)
    uint32 destinationZoneId = 0;

    /// Name of destination location for display
    ::std::string destinationName;

    // ========== Spell Information ==========

    /// Spell ID that this portal casts (for teleportation)
    uint32 teleportSpellId = 0;

    /// Effect index in the spell (usually 0)
    uint8 spellEffectIndex = 0;

    // ========== Access Restrictions ==========

    /// Faction restriction
    PortalFaction faction = PortalFaction::NEUTRAL;

    /// Minimum level required to use this portal
    uint8 minLevel = 0;

    /// Maximum level (0 = no cap)
    uint8 maxLevel = 0;

    /// Required quest completion (0 = no requirement)
    uint32 requiredQuestId = 0;

    /// Required achievement (0 = no requirement)
    uint32 requiredAchievementId = 0;

    /// Required skill (e.g., engineering portals) - SkillType enum value
    uint16 requiredSkillId = 0;

    /// Required skill level
    uint16 requiredSkillValue = 0;

    // ========== Status Flags ==========

    /// Whether this portal is currently active (from spawn data)
    bool isActive = true;

    /// Whether this is a temporary/player-created portal
    bool isTemporary = false;

    /// Whether this portal requires interaction (click) vs walk-through
    bool requiresInteraction = true;

    // ========== Methods ==========

    /**
     * @brief Check if a player can use this portal
     *
     * @param player The player to check
     * @return true if player meets all requirements
     *
     * Checks:
     * - Faction compatibility
     * - Level requirements
     * - Quest completion
     * - Achievement requirements
     * - Skill requirements
     *
     * Performance: O(1) - simple comparisons
     * Thread-safety: Thread-safe (read-only)
     */
    [[nodiscard]] bool CanPlayerUse(Player const* player) const;

    /**
     * @brief Calculate distance from player to this portal
     *
     * @param player The player to measure from
     * @return Distance in yards (3D distance)
     *
     * Performance: O(1) - ~50ns
     * Thread-safety: Thread-safe
     */
    [[nodiscard]] float GetDistanceFrom(Player const* player) const;

    /**
     * @brief Check if player is on the same map as the portal source
     *
     * @param player The player to check
     * @return true if on same map
     */
    [[nodiscard]] bool IsOnSameMap(Player const* player) const;

    /**
     * @brief Check if this portal leads to a specific destination map
     *
     * @param mapId Target map ID
     * @return true if this portal leads to the specified map
     */
    [[nodiscard]] bool LeadsToMap(uint32 mapId) const { return destinationMapId == mapId; }
};

/**
 * @class PortalDatabase
 * @brief Enterprise-grade portal discovery and management system
 *
 * Provides comprehensive portal location services for bot navigation:
 *
 * Data Sources (loaded at startup):
 * - gameobject + gameobject_template: Portal spawn locations and metadata
 * - spell_target_position: Teleport destinations from portal spells
 *
 * Features:
 * - Automatic portal detection from database at server startup
 * - Spatial indexing by map and zone for fast queries
 * - Destination indexing for route planning
 * - Faction-aware filtering
 * - Level/quest/achievement requirement checking
 * - Dynamic fallback search for runtime-spawned portals
 *
 * Performance Targets:
 * - Initialization: < 200ms at server startup
 * - Portal lookup: < 0.5ms per query
 * - Memory usage: < 1MB for ~1000 portals
 * - Thread-safe concurrent access
 *
 * Integration:
 * - Used by TravelRouteManager for cross-continent navigation
 * - Integrates with SpellMgr for teleport destination data
 * - Coordinates with FlightMasterManager for multi-leg routes
 *
 * @code
 * // Example usage:
 * auto& portalDb = PortalDatabase::Instance();
 *
 * // Find all portals from current map to destination
 * auto portals = portalDb.GetPortalsToMap(bot, destinationMapId);
 * for (auto const* portal : portals)
 * {
 *     TC_LOG_DEBUG("playerbot", "Portal '{}' at distance {:.1f}y leads to map {}",
 *                  portal->name, portal->GetDistanceFrom(bot), portal->destinationMapId);
 * }
 *
 * // Find nearest portal to specific destination
 * if (auto* portal = portalDb.GetNearestPortalToDestination(bot, mapId, position))
 * {
 *     // Navigate to portal
 * }
 * @endcode
 */
class PortalDatabase
{
public:
    /// Meyer's singleton instance
    static PortalDatabase& Instance();

    /// Delete copy/move constructors (singleton)
    PortalDatabase(PortalDatabase const&) = delete;
    PortalDatabase(PortalDatabase&&) = delete;
    PortalDatabase& operator=(PortalDatabase const&) = delete;
    PortalDatabase& operator=(PortalDatabase&&) = delete;

    // ========== Initialization ==========

    /**
     * @brief Initialize the portal database from game data
     *
     * Loads portal data from multiple sources:
     * 1. gameobject + gameobject_template: Portal spawn locations
     * 2. spell_target_position: Teleport destinations
     * 3. Derives zone/area information from positions
     *
     * This method must be called during server startup before bots use travel.
     *
     * @return true if initialization succeeded
     *
     * Performance: < 200ms typical
     * Thread-safety: NOT thread-safe, call only during single-threaded init
     */
    bool Initialize();

    /**
     * @brief Reload portal data from database
     *
     * Used for runtime updates without server restart.
     * Briefly locks the database during reload.
     *
     * @return true if reload succeeded
     *
     * Performance: < 300ms (includes database queries)
     * Thread-safety: Thread-safe, uses exclusive lock during reload
     */
    bool Reload();

    /**
     * @brief Check if the database has been initialized
     *
     * @return true if Initialize() has been called successfully
     */
    [[nodiscard]] bool IsInitialized() const { return _initialized; }

    // ========== Portal Queries ==========

    /**
     * @brief Get all portals accessible to a player that lead to a specific map
     *
     * Returns portals sorted by distance from player (nearest first).
     * Only returns portals the player can actually use (faction, level, etc.).
     *
     * @param player The player (for location and access checks)
     * @param destinationMapId Target map ID
     * @param maxCount Maximum number of results (default: 10)
     * @return Vector of portal pointers sorted by distance
     *
     * Performance: O(n) where n = portals to destination map, ~0.3ms typical
     * Thread-safety: Thread-safe (uses shared lock)
     */
    [[nodiscard]] ::std::vector<PortalInfo const*> GetPortalsToMap(
        Player const* player,
        uint32 destinationMapId,
        uint32 maxCount = 10) const;

    /**
     * @brief Get all portals on a specific map
     *
     * Returns all portals located on the specified map.
     * Does not filter by player access.
     *
     * @param mapId Map ID to search on
     * @return Vector of portal pointers
     *
     * Performance: O(1) hash lookup + O(n) copy, ~0.2ms typical
     * Thread-safety: Thread-safe (uses shared lock)
     */
    [[nodiscard]] ::std::vector<PortalInfo const*> GetPortalsOnMap(uint32 mapId) const;

    /**
     * @brief Get all portals in a specific zone
     *
     * Returns portals within a zone (e.g., Stormwind City zone).
     *
     * @param zoneId Zone ID from AreaTable.dbc
     * @return Vector of portal pointers
     *
     * Performance: O(1) hash lookup + O(n) copy, ~0.1ms typical
     * Thread-safety: Thread-safe (uses shared lock)
     */
    [[nodiscard]] ::std::vector<PortalInfo const*> GetPortalsInZone(uint32 zoneId) const;

    /**
     * @brief Get the nearest portal to a player that leads to a destination
     *
     * Finds the closest portal that:
     * 1. Is on the same map as the player
     * 2. Leads to the destination map
     * 3. Player can access (faction, level, etc.)
     *
     * @param player The player (for location and access)
     * @param destinationMapId Target map ID
     * @return Nearest accessible portal, or nullptr if none found
     *
     * Performance: O(n) where n = relevant portals, ~0.5ms typical
     * Thread-safety: Thread-safe (uses shared lock)
     */
    [[nodiscard]] PortalInfo const* GetNearestPortalToDestination(
        Player const* player,
        uint32 destinationMapId) const;

    /**
     * @brief Get the nearest portal to player that goes to nearest point to destination
     *
     * More advanced search that considers:
     * 1. Distance from player to portal
     * 2. Distance from portal destination to final target
     * 3. Total travel efficiency
     *
     * @param player The player
     * @param destinationMapId Target map ID
     * @param destinationPos Target position on destination map
     * @return Best portal for reaching destination, or nullptr if none
     *
     * Performance: O(n) with scoring calculations, ~1ms typical
     * Thread-safety: Thread-safe (uses shared lock)
     */
    [[nodiscard]] PortalInfo const* GetBestPortalForDestination(
        Player const* player,
        uint32 destinationMapId,
        Position const& destinationPos) const;

    /**
     * @brief Find a specific portal by GameObject entry
     *
     * @param gameObjectEntry GameObject entry ID
     * @return Portal info if found, nullptr otherwise
     *
     * Performance: O(1) hash lookup, ~50ns
     * Thread-safety: Thread-safe (uses shared lock)
     */
    [[nodiscard]] PortalInfo const* GetPortalByEntry(uint32 gameObjectEntry) const;

    /**
     * @brief Find a portal by its unique ID
     *
     * @param portalId Internal portal ID
     * @return Portal info if found, nullptr otherwise
     *
     * Performance: O(1) array access, ~10ns
     * Thread-safety: Thread-safe (uses shared lock)
     */
    [[nodiscard]] PortalInfo const* GetPortalById(uint32 portalId) const;

    // ========== Dynamic Portal Detection ==========

    /**
     * @brief Search for portals near a player at runtime
     *
     * Fallback method for finding portals that weren't in the database.
     * Searches nearby GameObjects for portal-type objects.
     *
     * @param player The player to search around
     * @param searchRadius Search radius in yards (default: 100)
     * @return Vector of found portal GameObjects
     *
     * Performance: O(n) where n = nearby GameObjects, ~2ms typical
     * Thread-safety: Thread-safe (read-only world state)
     *
     * @note This is slower than database lookup and should be used as fallback
     */
    [[nodiscard]] ::std::vector<GameObject*> FindNearbyPortalObjects(
        Player const* player,
        float searchRadius = 100.0f) const;

    /**
     * @brief Get teleport destination for a portal spell
     *
     * Queries SpellMgr for the teleport destination of a portal spell.
     * Used for both database portals and dynamically found ones.
     *
     * @param spellId Portal spell ID
     * @param effIndex Spell effect index (default: 0)
     * @return Destination position, or nullopt if not found
     *
     * Performance: O(1) SpellMgr lookup, ~100ns
     * Thread-safety: Thread-safe
     */
    [[nodiscard]] ::std::optional<WorldLocation> GetPortalDestination(
        uint32 spellId,
        uint8 effIndex = 0) const;

    // ========== Statistics ==========

    /**
     * @brief Get total number of portals in the database
     *
     * @return Number of registered portals
     */
    [[nodiscard]] size_t GetPortalCount() const;

    /**
     * @brief Get memory usage statistics
     *
     * @return Approximate memory usage in bytes
     */
    [[nodiscard]] size_t GetMemoryUsage() const;

    /**
     * @brief Get statistics about portal database
     */
    struct Statistics
    {
        uint32 totalPortals = 0;          ///< Total portals loaded
        uint32 staticPortals = 0;         ///< Permanent world portals
        uint32 dungeonPortals = 0;        ///< Instance entrance portals
        uint32 expansionPortals = 0;      ///< Expansion-specific portals
        uint32 alliancePortals = 0;       ///< Alliance-only portals
        uint32 hordePortals = 0;          ///< Horde-only portals
        uint32 neutralPortals = 0;        ///< Neutral portals
        uint32 uniqueDestinations = 0;    ///< Unique destination maps
        uint32 mapsWithPortals = 0;       ///< Maps containing portals
        uint64 loadTimeMs = 0;            ///< Database load time in ms
    };

    /**
     * @brief Get database statistics
     *
     * @return Statistics structure
     */
    [[nodiscard]] Statistics GetStatistics() const;

private:
    /// Private constructor for singleton
    PortalDatabase() = default;

    /// Destructor
    ~PortalDatabase() = default;

    // ========== Loading Methods ==========

    /**
     * @brief Load portal GameObjects from database
     *
     * Queries gameobject + gameobject_template for portal-type objects:
     * - GAMEOBJECT_TYPE_SPELLCASTER (22) with teleport spells
     * - GAMEOBJECT_TYPE_GOOBER (10) with teleport spells
     *
     * @return Number of portals loaded
     */
    uint32 LoadPortalsFromDB();

    /**
     * @brief Load teleport destinations for all portals
     *
     * Queries spell_target_position for each portal's teleport spell.
     *
     * @return Number of destinations resolved
     */
    uint32 LoadDestinations();

    /**
     * @brief Determine portal faction from spawn data
     *
     * @param portal Portal to analyze
     * @param gameObjectEntry GameObject entry for additional data
     */
    void DeterminePortalFaction(PortalInfo& portal, uint32 gameObjectEntry);

    /**
     * @brief Classify portal type based on destination and location
     *
     * @param portal Portal to classify
     */
    void ClassifyPortalType(PortalInfo& portal);

    /**
     * @brief Build spatial indexes for fast lookup
     *
     * Creates indexes:
     * - By source map ID
     * - By source zone ID
     * - By destination map ID
     * - By GameObject entry
     */
    void BuildIndexes();

    /**
     * @brief Validate portal data integrity
     *
     * Checks for:
     * - Valid positions
     * - Valid map IDs
     * - Valid spell references
     * - Duplicate entries
     */
    void ValidateData();

    /**
     * @brief Check if a spell is a teleport spell
     *
     * @param spellId Spell to check
     * @return true if spell has teleport effect
     */
    bool IsTeleportSpell(uint32 spellId) const;

private:
    /// Primary storage for all portal data
    ::std::vector<PortalInfo> _portals;

    /// Index: source map ID -> portal indices
    ::std::unordered_map<uint32, ::std::vector<size_t>> _portalsBySourceMap;

    /// Index: source zone ID -> portal indices
    ::std::unordered_map<uint32, ::std::vector<size_t>> _portalsBySourceZone;

    /// Index: destination map ID -> portal indices
    ::std::unordered_map<uint32, ::std::vector<size_t>> _portalsByDestination;

    /// Index: GameObject entry -> portal index
    ::std::unordered_map<uint32, size_t> _portalByEntry;

    /// Reader-writer lock for thread-safe access
    mutable Playerbot::OrderedSharedMutex<Playerbot::LockOrder::QUEST_MANAGER> _mutex;

    /// Initialization flag
    bool _initialized = false;

    /// Statistics
    Statistics _stats;

    /// Memory usage tracking
    size_t _memoryUsage = 0;
};

} // namespace Playerbot

#endif // _PLAYERBOT_PORTAL_DATABASE_H
