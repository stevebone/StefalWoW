/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * MULTI-STATION TRAVEL PLANNING SYSTEM
 * =====================================
 * Enterprise-grade travel route planning for cross-map bot navigation.
 *
 * Supports complex multi-hop journeys:
 * - FlightMaster -> Ship -> FlightMaster
 * - FlightMaster -> Portal -> FlightMaster
 * - Hearthstone -> FlightMaster -> Zeppelin -> FlightMaster
 * - Portal -> Ship -> FlightMaster
 *
 * Transport Types Supported:
 * - Taxi/Flight Paths (same continent)
 * - Ships (Booty Bay <-> Ratchet, Menethil <-> Theramore, etc.)
 * - Zeppelins (Undercity <-> Orgrimmar, etc.)
 * - Portals (Mage portals, Dalaran portals, etc.)
 * - Boats/Ferries (Auberdine <-> Darnassus, etc.)
 * - Hearthstone (if bound to useful location)
 *
 * Performance Targets:
 * - Route calculation: <50ms for complex routes
 * - Memory per route: <1KB
 * - State updates: <1ms
 */

#pragma once

#include "Define.h"
#include "Position.h"
#include "ObjectGuid.h"
#include <vector>
#include <unordered_map>
#include <memory>
#include <string>
#include <functional>

class Player;
class Creature;
class Transport;  // Forward declaration in global namespace (TrinityCore's Transport class)

namespace Playerbot
{

/**
 * @enum TransportType
 * @brief Types of transport available for travel
 */
enum class TransportType : uint8
{
    NONE            = 0,
    TAXI_FLIGHT     = 1,  // Standard flight path
    SHIP            = 2,  // Ship transport (neutral)
    ZEPPELIN        = 3,  // Zeppelin (Horde)
    PORTAL          = 4,  // Magical portal
    BOAT            = 5,  // Boat/ferry (Alliance)
    HEARTHSTONE     = 6,  // Hearthstone teleport
    WALK            = 7,  // Walking to transport
    TELEPORT        = 8,  // Mage/warlock teleport
    DUNGEON_FINDER  = 9,  // Teleport via LFG
};

/**
 * @enum TravelState
 * @brief Current state in multi-hop journey
 */
enum class TravelState : uint8
{
    IDLE                    = 0,   // No active travel
    WALKING_TO_TRANSPORT    = 1,   // Walking to transport departure point
    WAITING_FOR_TRANSPORT   = 2,   // At transport, waiting for arrival
    ON_TRANSPORT            = 3,   // Currently on transport (ship/zeppelin)
    ARRIVING                = 4,   // Transport arriving at destination
    WALKING_FROM_TRANSPORT  = 5,   // Walking from transport to next point
    TAXI_FLIGHT             = 6,   // Currently on taxi flight
    CASTING_HEARTHSTONE     = 7,   // Casting hearthstone
    USING_PORTAL            = 8,   // Using portal
    COMPLETED               = 9,   // Journey completed
    FAILED                  = 10,  // Journey failed
};

/**
 * @struct TransportConnection
 * @brief Defines a transport connection between two points
 */
struct TransportConnection
{
    uint32 connectionId;              // Unique ID for this connection
    TransportType type;               // Type of transport
    std::string name;                 // Human-readable name

    // Departure point
    uint32 departureMapId;
    Position departurePosition;
    uint32 departureTaxiNode;         // Nearest taxi node to departure (0 if none)

    // Arrival point
    uint32 arrivalMapId;
    Position arrivalPosition;
    uint32 arrivalTaxiNode;           // Nearest taxi node to arrival (0 if none)

    // Transport details
    uint32 transportEntry;            // GameObject/Creature entry for transport
    uint32 waitTimeSeconds;           // Average wait time in seconds
    uint32 travelTimeSeconds;         // Travel time in seconds

    // Faction restrictions
    bool allianceOnly;
    bool hordeOnly;
    bool requiresLevel;
    uint8 minLevel;

    // Cost
    uint32 costCopper;                // Cost in copper (0 if free)

    TransportConnection()
        : connectionId(0), type(TransportType::NONE)
        , departureMapId(0), departureTaxiNode(0)
        , arrivalMapId(0), arrivalTaxiNode(0)
        , transportEntry(0), waitTimeSeconds(0), travelTimeSeconds(0)
        , allianceOnly(false), hordeOnly(false), requiresLevel(false), minLevel(1)
        , costCopper(0)
    { }
};

/**
 * @struct TravelLeg
 * @brief Single leg of a multi-hop journey
 */
struct TravelLeg
{
    uint32 legIndex;                  // Order in journey (0, 1, 2, ...)
    TransportType type;               // Transport type for this leg
    std::string description;          // Human-readable description

    // Start and end points
    uint32 startMapId;
    Position startPosition;
    uint32 endMapId;
    Position endPosition;

    // For taxi legs
    uint32 taxiStartNode;
    uint32 taxiEndNode;
    std::vector<uint32> taxiRoute;

    // For transport legs (ship/zeppelin/portal)
    TransportConnection const* connection;

    // Timing
    uint32 estimatedTimeSeconds;
    uint32 estimatedCostCopper;

    // State tracking
    TravelState currentState;
    uint32 stateStartTime;            // GameTime when state started

    TravelLeg()
        : legIndex(0), type(TransportType::NONE)
        , startMapId(0), endMapId(0)
        , taxiStartNode(0), taxiEndNode(0)
        , connection(nullptr)
        , estimatedTimeSeconds(0), estimatedCostCopper(0)
        , currentState(TravelState::IDLE), stateStartTime(0)
    { }
};

/**
 * @struct TravelRoute
 * @brief Complete multi-hop travel route
 */
struct TravelRoute
{
    uint32 routeId;                   // Unique route ID
    std::string description;          // Human-readable route description

    // Journey endpoints
    uint32 originMapId;
    Position originPosition;
    uint32 destinationMapId;
    Position destinationPosition;

    // Route legs
    std::vector<TravelLeg> legs;

    // Overall metrics
    uint32 totalEstimatedTimeSeconds;
    uint32 totalEstimatedCostCopper;
    uint32 totalLegs;

    // State
    uint32 currentLegIndex;
    TravelState overallState;
    uint32 routeStartTime;            // GameTime when route started

    // Callbacks
    std::function<void(TravelRoute const&)> onCompleted;
    std::function<void(TravelRoute const&, std::string const&)> onFailed;

    TravelRoute()
        : routeId(0)
        , originMapId(0), destinationMapId(0)
        , totalEstimatedTimeSeconds(0), totalEstimatedCostCopper(0), totalLegs(0)
        , currentLegIndex(0), overallState(TravelState::IDLE), routeStartTime(0)
    { }

    bool IsComplete() const { return overallState == TravelState::COMPLETED; }
    bool IsFailed() const { return overallState == TravelState::FAILED; }
    bool IsActive() const { return overallState != TravelState::IDLE && overallState != TravelState::COMPLETED && overallState != TravelState::FAILED; }
    TravelLeg* GetCurrentLeg() { return currentLegIndex < legs.size() ? &legs[currentLegIndex] : nullptr; }
    TravelLeg const* GetCurrentLeg() const { return currentLegIndex < legs.size() ? &legs[currentLegIndex] : nullptr; }
};

/**
 * @class TravelRouteManager
 * @brief Plans and manages multi-station travel routes
 *
 * This class is responsible for:
 * 1. Planning optimal routes across continents using transport connections
 * 2. Managing travel state machine for multi-hop journeys
 * 3. Coordinating with FlightMasterManager for taxi portions
 * 4. Handling transport waiting and boarding
 */
class TravelRouteManager
{
public:
    explicit TravelRouteManager(Player* bot);
    ~TravelRouteManager();

    // Route Planning

    /**
     * @brief Plan a route from current position to destination
     * @param destinationMapId Target map ID
     * @param destination Target position
     * @return Planned route, or empty route if no path found
     */
    TravelRoute PlanRoute(uint32 destinationMapId, Position const& destination);

    /**
     * @brief Check if a route exists between two maps
     * @param fromMapId Source map ID
     * @param toMapId Destination map ID
     * @return True if route is possible
     */
    bool CanReachMap(uint32 fromMapId, uint32 toMapId) const;

    /**
     * @brief Get estimated travel time between maps
     * @param fromMapId Source map ID
     * @param fromPos Source position
     * @param toMapId Destination map ID
     * @param toPos Destination position
     * @return Estimated time in seconds, or UINT32_MAX if unreachable
     */
    uint32 GetEstimatedTravelTime(uint32 fromMapId, Position const& fromPos, uint32 toMapId, Position const& toPos) const;

    // Route Execution

    /**
     * @brief Start executing a planned route
     * @param route Route to execute
     * @return True if route execution started successfully
     */
    bool StartRoute(TravelRoute&& route);

    /**
     * @brief Update current route progress (call every tick)
     * @param diff Time since last update in ms
     * @return True if route is still active
     */
    bool Update(uint32 diff);

    /**
     * @brief Cancel current route
     */
    void CancelRoute();

    /**
     * @brief Check if currently traveling
     * @return True if route is active
     */
    bool IsTraveling() const { return m_activeRoute && m_activeRoute->IsActive(); }

    /**
     * @brief Get current route
     * @return Current route, or nullptr if none
     */
    TravelRoute const* GetCurrentRoute() const { return m_activeRoute.get(); }

    /**
     * @brief Get current travel state
     * @return Current state
     */
    TravelState GetCurrentState() const;

    /**
     * @brief Get human-readable status
     * @return Status string
     */
    std::string GetStatusString() const;

    // Transport Database Access

    /**
     * @brief Get all transport connections from a map
     * @param mapId Source map ID
     * @return Vector of available connections
     */
    std::vector<TransportConnection const*> GetConnectionsFromMap(uint32 mapId) const;

    /**
     * @brief Get all transport connections to a map
     * @param mapId Destination map ID
     * @return Vector of available connections
     */
    std::vector<TransportConnection const*> GetConnectionsToMap(uint32 mapId) const;

    /**
     * @brief Find direct connection between two maps
     * @param fromMapId Source map ID
     * @param toMapId Destination map ID
     * @return Connection if exists, nullptr otherwise
     */
    TransportConnection const* FindDirectConnection(uint32 fromMapId, uint32 toMapId) const;

    // Statistics

    struct Statistics
    {
        uint32 routesPlanned;
        uint32 routesCompleted;
        uint32 routesFailed;
        uint32 totalLegsCompleted;
        uint64 totalTravelTimeMs;
        uint64 totalCostCopper;

        Statistics() : routesPlanned(0), routesCompleted(0), routesFailed(0),
                       totalLegsCompleted(0), totalTravelTimeMs(0), totalCostCopper(0) {}
    };

    Statistics const& GetStatistics() const { return m_stats; }

private:
    // Internal Route Planning

    /**
     * @brief Build route using A* pathfinding across transport graph
     */
    bool BuildRoute(TravelRoute& route, uint32 fromMapId, Position const& fromPos,
                    uint32 toMapId, Position const& toPos);

    /**
     * @brief Add taxi leg to route
     */
    bool AddTaxiLeg(TravelRoute& route, uint32 mapId, Position const& from, Position const& to);

    /**
     * @brief Add transport leg to route
     */
    bool AddTransportLeg(TravelRoute& route, TransportConnection const* connection);

    /**
     * @brief Add walking leg to route
     */
    void AddWalkingLeg(TravelRoute& route, uint32 mapId, Position const& from, Position const& to);

    /**
     * @brief Add hearthstone leg to route
     */
    bool AddHearthstoneLeg(TravelRoute& route);

    // State Machine

    /**
     * @brief Update state machine for current leg
     */
    void UpdateLegState(TravelLeg& leg, uint32 diff);

    /**
     * @brief Advance to next leg
     */
    void AdvanceToNextLeg();

    /**
     * @brief Handle walking to transport
     */
    void HandleWalkingToTransport(TravelLeg& leg);

    /**
     * @brief Handle waiting for transport
     */
    void HandleWaitingForTransport(TravelLeg& leg);

    /**
     * @brief Handle being on transport
     */
    void HandleOnTransport(TravelLeg& leg);

    /**
     * @brief Handle taxi flight
     */
    void HandleTaxiFlight(TravelLeg& leg);

    /**
     * @brief Handle hearthstone cast
     */
    void HandleHearthstone(TravelLeg& leg);

    /**
     * @brief Handle portal usage
     */
    void HandlePortal(TravelLeg& leg);

    // Helper Methods

    /**
     * @brief Check if bot is near position
     */
    bool IsNearPosition(Position const& pos, float range = 10.0f) const;

    /**
     * @brief Check if bot is on transport
     */
    bool IsOnTransport() const;

    /**
     * @brief Get nearest transport of type
     */
    ObjectGuid FindNearbyTransport(uint32 entry, float range = 100.0f) const;

    /**
     * @brief Check if transport is at departure point
     */
    bool IsTransportAtDeparture(ObjectGuid transportGuid, TransportConnection const* connection) const;

    /**
     * @brief Check if bot can use connection (faction, level)
     */
    bool CanUseConnection(TransportConnection const* connection) const;

    /**
     * @brief Initialize transport connections database
     */
    void InitializeTransportConnections();

    /**
     * @brief Find transport at a specific position
     * @param pos Position to search near
     * @param transportEntry Optional specific transport entry to find
     * @param range Search range in yards
     * @return Transport if found, nullptr otherwise
     */
    ::Transport* FindTransportAtPosition(Position const& pos, uint32 transportEntry, float range) const;

    // Member Variables
    Player* m_bot;
    std::unique_ptr<TravelRoute> m_activeRoute;
    Statistics m_stats;

    // Current transport tracking (for ship/zeppelin boarding)
    ObjectGuid m_currentTransportGuid;

    // Transport movement tracking (for detecting when transport has stopped)
    Position m_lastTransportPosition;
    uint32 m_transportStationaryStartTime;  // GameTime when transport stopped moving
    static constexpr float TRANSPORT_MOVEMENT_THRESHOLD = 1.0f;  // Consider stopped if moved <1yd
    static constexpr uint32 TRANSPORT_STOPPED_DURATION_MS = 2000; // 2 seconds stationary = stopped

    // Transport connections database (static, shared across all managers)
    static std::vector<TransportConnection> s_transportConnections;
    static std::unordered_map<uint32, std::vector<size_t>> s_connectionsByDepartureMap;
    static std::unordered_map<uint32, std::vector<size_t>> s_connectionsByArrivalMap;
    static bool s_connectionsInitialized;

    // Map connectivity graph for pathfinding
    static std::unordered_map<uint32, std::vector<std::pair<uint32, TransportConnection const*>>> s_mapConnectivityGraph;

    // Route planning cache
    uint32 m_lastRoutePlanTime;
    static constexpr uint32 ROUTE_CACHE_DURATION_MS = 60000; // 1 minute

    // State update timing
    uint32 m_lastStateUpdateTime;
    static constexpr uint32 STATE_UPDATE_INTERVAL_MS = 500; // 500ms
};

// ============================================================================
// TRANSPORT CONNECTION DATABASE
// ============================================================================
// Pre-defined transport connections for World of Warcraft
// These cover the major ship, zeppelin, and portal routes

/**
 * @brief Initialize all known transport connections
 *
 * Map IDs Reference:
 * 0 = Eastern Kingdoms (Azeroth)
 * 1 = Kalimdor
 * 530 = Outland
 * 571 = Northrend
 * 870 = Pandaria
 * 1116 = Draenor
 * 1220 = Broken Isles
 * 1642 = Zandalar
 * 1643 = Kul Tiras
 * 2222 = Dragon Isles
 * 2552 = Khaz Algar
 */

} // namespace Playerbot
