/*
 * Copyright (C) 2024 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include "Position.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <string>

class Player;
class Creature;
struct TaxiNodesEntry;

namespace Playerbot
{

/**
 * @enum FlightResult
 * @brief Result codes for flight operations
 */
enum class FlightResult : uint8
{
    SUCCESS = 0,          // Flight initiated successfully
    ALREADY_FLYING,       // Bot is already in flight
    NO_FLIGHT_MASTER,     // No flight master nearby
    NODE_UNKNOWN,         // Destination taxi node not discovered
    PATH_NOT_FOUND,       // No valid path between nodes
    INSUFFICIENT_GOLD,    // Cannot afford flight
    INVALID_NODE,         // Invalid taxi node ID
    CROSS_CONTINENT,      // Nodes on different continents (no direct path)
    NOT_AT_NODE,          // Bot not at a taxi node
    INTERNAL_ERROR        // Internal error
};

/**
 * @enum FlightPathStrategy
 * @brief Strategy options for flight path calculation
 */
enum class FlightPathStrategy : uint8
{
    SHORTEST_DISTANCE = 0,  // Minimize total distance
    CHEAPEST_COST,          // Minimize gold cost
    FEWEST_STOPS,           // Minimize number of stops
    FASTEST_TIME            // Minimize travel time
};

/**
 * @struct FlightMasterLocation
 * @brief Information about a nearby flight master
 */
struct FlightMasterLocation
{
    ::std::string name;            // Flight master name
    uint32 taxiNode;               // Associated taxi node ID
    float distanceFromPlayer;      // Distance from player
    Position position;             // World position
    ObjectGuid guid;               // Creature GUID

    FlightMasterLocation()
        : name(), taxiNode(0), distanceFromPlayer(0.0f), position()
    { }
};

/**
 * @struct FlightPathInfo
 * @brief Information about a calculated flight path
 */
struct FlightPathInfo
{
    uint32 stopCount;              // Number of stops in path
    uint32 goldCost;               // Total cost in copper
    uint32 flightTime;             // Estimated time in seconds
    ::std::vector<uint32> nodes;     // Node IDs in path
    bool crossesContinent;         // Whether path crosses continents

    FlightPathInfo()
        : stopCount(0), goldCost(0), flightTime(0), crossesContinent(false)
    { }
};

/**
 * @class FlightMasterManager
 * @brief Manages all flight master interactions for player bots
 *
 * This class provides complete flight master functionality using TrinityCore's
 * taxi system APIs. It handles:
 * - Automatic flight path discovery
 * - Smart destination selection based on bot goals
 * - Flight cost calculation
 * - Flight execution via TrinityCore taxi system
 * - Route optimization
 *
 * Performance Target: <1ms per flight decision
 * Memory Target: <20KB overhead
 *
 * TrinityCore API Integration:
 * - sObjectMgr->GetNearestTaxiNode() - Find taxi node at location
 * - Player::m_taxi.IsTaximaskNodeKnown() - Check known nodes
 * - Player::m_taxi.SetTaximaskNode() - Learn new node
 * - TaxiPathGraph::GetCompleteNodeRoute() - Calculate route
 * - Player::ActivateTaxiPathTo() - Execute flight
 * - sTaxiNodesStore.LookupEntry() - Get taxi node data
 */
class FlightMasterManager
{
public:
    /**
     * @brief Destination priority levels for flight selection
     *
     * Determines which destinations are prioritized:
     * - QUEST_OBJECTIVE: Fly to quest objective locations
     * - TRAINER_VENDOR: Fly to training/vendor hubs
     * - LEVELING_ZONE: Fly to appropriate leveling zones
     * - EXPLORATION: Discover new areas
     */
    enum class DestinationPriority : uint8
    {
        QUEST_OBJECTIVE = 0,  // Highest priority - quest locations
        TRAINER_VENDOR  = 1,  // Training and vendor hubs
        LEVELING_ZONE   = 2,  // Appropriate leveling zones
        EXPLORATION     = 3   // Exploration and discovery
    };

    /**
     * @brief Flight path evaluation result
     */
    struct FlightPathEvaluation
    {
        uint32 nodeId;                    // Destination taxi node ID
        DestinationPriority priority;     // Priority level
        uint32 estimatedCost;             // Estimated flight cost in copper
        float distance;                   // Flight distance
        uint32 estimatedTime;             // Estimated flight time in seconds
        bool isKnown;                     // Whether bot knows this node
        ::std::vector<uint32> route;        // Full route (node IDs)
        ::std::string reason;               // Human-readable reason for priority

        FlightPathEvaluation()
            : nodeId(0), priority(DestinationPriority::EXPLORATION)
            , estimatedCost(0), distance(0.0f), estimatedTime(0)
            , isKnown(false)
        { }
    };

    /**
     * @brief Flight destination info with coordinates
     */
    struct FlightDestination
    {
        uint32 nodeId;
        ::std::string name;
        float x, y, z;
        uint32 mapId;
        uint32 continentId;
        bool isKnown;
        bool isReachable;

        FlightDestination()
            : nodeId(0), x(0.0f), y(0.0f), z(0.0f)
            , mapId(0), continentId(0)
            , isKnown(false), isReachable(false)
        { }
    };

    FlightMasterManager(Player* bot);
    ~FlightMasterManager() = default;

    // Core Flight Methods

    /**
     * @brief Learn flight path at current flight master
     * @param flightMaster Flight master creature
     * @return True if learned new path
     *
     * Automatically discovers the flight path at the flight master's location.
     * Uses sObjectMgr->GetNearestTaxiNode() to find the node and
     * Player::m_taxi.SetTaximaskNode() to learn it.
     */
    bool LearnFlightPath(Creature* flightMaster);

    /**
     * @brief Fly to a specific destination
     * @param flightMaster Flight master creature
     * @param destinationNodeId Target taxi node ID
     * @return True if flight successfully initiated
     *
     * Calculates route using TaxiPathGraph::GetCompleteNodeRoute() and
     * executes flight via Player::ActivateTaxiPathTo().
     */
    bool FlyToDestination(Creature* flightMaster, uint32 destinationNodeId);

    /**
     * @brief Smart flight - automatically select best destination
     * @param flightMaster Flight master creature
     * @return True if flight successfully initiated
     *
     * Evaluates all available destinations, prioritizes based on bot goals,
     * and automatically flies to the best option.
     */
    bool SmartFlight(Creature* flightMaster);

    /**
     * @brief Get current taxi node at flight master
     * @param flightMaster Flight master creature
     * @return Taxi node ID, or 0 if none
     */
    uint32 GetCurrentTaxiNode(Creature* flightMaster) const;

    /**
     * @brief Check if bot knows a specific taxi node
     * @param nodeId Taxi node ID
     * @return True if known
     */
    bool IsFlightPathKnown(uint32 nodeId) const;

    /**
     * @brief Get all known flight paths
     * @return Vector of known taxi node IDs
     */
    ::std::vector<uint32> GetKnownFlightPaths() const;

    /**
     * @brief Get all reachable destinations from current location
     * @param flightMaster Flight master creature
     * @return Vector of reachable flight destinations
     */
    ::std::vector<FlightDestination> GetReachableDestinations(Creature* flightMaster) const;

    // Flight Evaluation Methods

    /**
     * @brief Evaluate a potential flight destination
     * @param from Source taxi node entry
     * @param to Destination taxi node entry
     * @return Evaluation result with priority and reasoning
     */
    FlightPathEvaluation EvaluateDestination(TaxiNodesEntry const* from, TaxiNodesEntry const* to) const;

    /**
     * @brief Calculate destination priority based on bot needs
     * @param nodeId Destination taxi node ID
     * @param nodeEntry Taxi node entry
     * @return Priority level
     */
    DestinationPriority CalculateDestinationPriority(uint32 nodeId, TaxiNodesEntry const* nodeEntry) const;

    /**
     * @brief Calculate estimated flight cost
     * @param from Source taxi node entry
     * @param to Destination taxi node entry
     * @return Estimated cost in copper
     */
    uint32 CalculateFlightCost(TaxiNodesEntry const* from, TaxiNodesEntry const* to) const;

    /**
     * @brief Calculate estimated flight time
     * @param distance Flight distance
     * @return Estimated time in seconds
     */
    uint32 CalculateFlightTime(float distance) const;

    /**
     * @brief Check if bot can afford flight
     * @param cost Flight cost in copper
     * @return True if bot has sufficient gold
     */
    bool CanAffordFlight(uint32 cost) const;

    // Goal-Based Flight Selection

    /**
     * @brief Find nearest flight master to a position
     * @param targetX Target X coordinate
     * @param targetY Target Y coordinate
     * @param targetZ Target Z coordinate
     * @param mapId Map ID
     * @return Taxi node ID nearest to target, or 0 if none
     */
    uint32 FindNearestFlightMasterToPosition(float targetX, float targetY, float targetZ, uint32 mapId) const;

    /**
     * @brief Get flight destination for quest objective
     * @param questId Quest ID
     * @return Taxi node ID for quest area, or 0 if none
     */
    uint32 GetFlightDestinationForQuest(uint32 questId) const;

    /**
     * @brief Get flight destination for training
     * @return Taxi node ID for training hub, or 0 if none
     *
     * Returns node ID for capital city or training area appropriate
     * for bot's class and level.
     */
    uint32 GetFlightDestinationForTraining() const;

    /**
     * @brief Get flight destination for leveling
     * @return Taxi node ID for appropriate leveling zone, or 0 if none
     *
     * Returns node ID for a zone appropriate for bot's level.
     */
    uint32 GetFlightDestinationForLeveling() const;

    // Statistics and Performance

    struct Statistics
    {
        uint32 flightPathsLearned;    // Total paths learned
        uint32 flightsTaken;          // Total flights taken
        uint64 totalGoldSpent;        // Total gold spent on flights
        uint32 flightAttempts;        // Total flight attempts
        uint32 flightFailures;        // Failed flight attempts
        uint32 insufficientGold;      // Failed due to insufficient gold
        uint32 pathNotKnown;          // Failed due to unknown path

        Statistics()
            : flightPathsLearned(0), flightsTaken(0), totalGoldSpent(0)
            , flightAttempts(0), flightFailures(0), insufficientGold(0)
            , pathNotKnown(0)
        { }
    };

    Statistics const& GetStatistics() const { return m_stats; }
    void ResetStatistics() { m_stats = Statistics(); }

    float GetCPUUsage() const { return m_cpuUsage; }
    size_t GetMemoryUsage() const;

    // ========================================================================
    // STATIC UTILITY METHODS (for use without instance)
    // ========================================================================

    /**
     * @brief Find nearest taxi node to a position (static utility)
     * @param pos Position to search near
     * @param mapId Map ID
     * @param player Player for faction filtering
     * @return Taxi node ID, or 0 if none found
     *
     * Uses sObjectMgr->GetNearestTaxiNode() with faction awareness.
     */
    static uint32 FindNearestTaxiNode(Position const& pos, uint32 mapId, Player* player);

    /**
     * @brief Find nearest flight master to a player (static utility)
     * @param player The player to search near
     * @return Optional FlightMasterLocation with info, empty if none found
     */
    static ::std::optional<FlightMasterLocation> FindNearestFlightMaster(Player* player);

    /**
     * @brief Check if player has discovered a taxi node (static utility)
     * @param player The player to check
     * @param nodeId Taxi node ID
     * @return True if the player knows this taxi node
     */
    static bool HasTaxiNode(Player* player, uint32 nodeId);

    /**
     * @brief Check if a valid flight path exists between two nodes (static utility)
     * @param startNode Starting taxi node ID
     * @param endNode Ending taxi node ID
     * @param player Player for faction/discovery checking
     * @return True if a valid path exists
     */
    static bool HasValidFlightPath(uint32 startNode, uint32 endNode, Player* player);

    /**
     * @brief Calculate flight path between nodes (static utility)
     * @param player The player
     * @param startNode Starting taxi node ID
     * @param endNode Ending taxi node ID
     * @param strategy Path calculation strategy
     * @return Optional FlightPathInfo, empty if no path found
     */
    static ::std::optional<FlightPathInfo> CalculateFlightPath(
        Player* player, uint32 startNode, uint32 endNode, FlightPathStrategy strategy);

    /**
     * @brief Initiate flight to a taxi node
     * @param player The player to fly
     * @param destinationNode Destination taxi node ID
     * @param strategy Path calculation strategy
     * @return FlightResult indicating success or failure reason
     */
    FlightResult FlyToTaxiNode(Player* player, uint32 destinationNode, FlightPathStrategy strategy);

    /**
     * @brief Get human-readable string for a FlightResult
     * @param result The result code
     * @return String description of the result
     */
    static ::std::string GetResultString(FlightResult result);

private:
    // Internal Helper Methods

    /**
     * @brief Get taxi node data by ID
     * @param nodeId Taxi node ID
     * @return Taxi node entry, or nullptr if not found
     */
    TaxiNodesEntry const* GetTaxiNode(uint32 nodeId) const;

    /**
     * @brief Calculate complete route between nodes
     * @param from Source taxi node entry
     * @param to Destination taxi node entry
     * @param route Output vector of node IDs
     * @return True if route found
     */
    bool CalculateRoute(TaxiNodesEntry const* from, TaxiNodesEntry const* to, ::std::vector<uint32>& route) const;

    /**
     * @brief Calculate distance between two nodes
     * @param from Source taxi node entry
     * @param to Destination taxi node entry
     * @return Distance in yards
     */
    float CalculateDistance(TaxiNodesEntry const* from, TaxiNodesEntry const* to) const;

    /**
     * @brief Check if destination is in appropriate leveling zone
     * @param nodeEntry Taxi node entry
     * @return True if appropriate for bot's level
     */
    bool IsAppropriateForLevel(TaxiNodesEntry const* nodeEntry) const;

    /**
     * @brief Check if destination is near quest objectives
     * @param nodeEntry Taxi node entry
     * @return True if near active quest objectives
     */
    bool IsNearQuestObjectives(TaxiNodesEntry const* nodeEntry) const;

    /**
     * @brief Check if destination is a major city/hub
     * @param nodeEntry Taxi node entry
     * @return True if major city
     */
    bool IsMajorCity(TaxiNodesEntry const* nodeEntry) const;

    /**
     * @brief Get recommended leveling zone for bot's level
     * @return Taxi node ID for leveling zone, or 0 if none
     */
    uint32 GetRecommendedLevelingZone() const;

    /**
     * @brief Execute flight via TrinityCore API
     * @param route Vector of taxi node IDs representing the route
     * @param flightMaster Flight master creature
     * @return True if flight successfully initiated
     */
    bool ExecuteFlight(::std::vector<uint32> const& route, Creature* flightMaster);

    /**
     * @brief Record flight in statistics
     */
    void RecordFlight(uint32 cost, bool success);

    /**
     * @brief Record path learning in statistics
     */
    void RecordPathLearned(uint32 nodeId);

    /**
     * @brief Update the cache of known flight paths
     */
    void UpdateKnownPathsCache();

    // Member Variables
    Player* m_bot;
    Statistics m_stats;

    // Performance Tracking
    float m_cpuUsage;
    uint32 m_totalFlightTime;    // microseconds
    uint32 m_flightDecisionCount;

    // Cache for frequently accessed data
    ::std::unordered_map<uint32, DestinationPriority> m_priorityCache;
    ::std::unordered_set<uint32> m_knownPathsCache;
    uint32 m_lastCacheUpdate;
    static constexpr uint32 CACHE_UPDATE_INTERVAL = 60000; // 1 minute
};

} // namespace Playerbot
