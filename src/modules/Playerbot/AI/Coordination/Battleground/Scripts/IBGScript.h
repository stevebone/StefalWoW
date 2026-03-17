/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2021+ WarheadCore <https://github.com/AzerothCore/WarheadCore>
 * Copyright (C) 2025+ TrinityCore Playerbot Integration
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_AI_COORDINATION_BG_IBGSCRIPT_H
#define PLAYERBOT_AI_COORDINATION_BG_IBGSCRIPT_H

#include "BGState.h"
#include "BGStrategyEngine.h"
#include "Position.h"
#include "ObjectGuid.h"
#include "SharedDefines.h"
#include <vector>
#include <map>
#include <string>
#include <memory>
#include <functional>
#include <cstdint>

// Forward declarations
class Player;

// Forward declare BattlegroundCoordinator in its actual namespace
namespace Playerbot {
    class BattlegroundCoordinator;
}

namespace Playerbot::Coordination::Battleground
{

// Bring BattlegroundCoordinator into this namespace for convenience
using ::Playerbot::BattlegroundCoordinator;

// Forward declarations - these are in this namespace
class ObjectiveManager;
class FlagCarrierManager;
class NodeController;
class BGRoleManager;
class BGStrategyEngine;

// ============================================================================
// BG Script Event System
// ============================================================================

/**
 * @brief Types of events that can occur in a battleground
 */
enum class BGScriptEvent : uint8
{
    // Match lifecycle
    MATCH_START = 0,
    MATCH_END = 1,
    ROUND_STARTED = 2,
    ROUND_ENDED = 3,

    // Objective events
    OBJECTIVE_CAPTURED = 10,
    OBJECTIVE_LOST = 11,
    OBJECTIVE_CONTESTED = 12,
    OBJECTIVE_NEUTRALIZED = 13,

    // Flag events (CTF)
    FLAG_PICKED_UP = 20,
    FLAG_DROPPED = 21,
    FLAG_CAPTURED = 22,
    FLAG_RETURNED = 23,
    FLAG_RESET = 24,

    // Orb events (Kotmogu)
    ORB_PICKED_UP = 30,
    ORB_DROPPED = 31,

    // Cart events (Silvershard, Deepwind)
    CART_CAPTURED = 40,
    CART_CONTESTED = 41,

    // Siege events
    GATE_DESTROYED = 50,
    VEHICLE_SPAWNED = 51,
    VEHICLE_DESTROYED = 52,
    BOSS_ENGAGED = 53,
    BOSS_KILLED = 54,
    TOWER_DESTROYED = 55,

    // Player events
    PLAYER_KILLED = 60,
    PLAYER_DIED = 61,
    PLAYER_RESURRECTED = 62,

    // Resource events
    AZERITE_SPAWNED = 70,
    RESOURCE_NODE_CLAIMED = 71,

    // World state
    WORLD_STATE_CHANGED = 80,
    SCORE_THRESHOLD_REACHED = 81,
    TIME_WARNING = 82,

    // Special
    CUSTOM_EVENT = 255
};

/**
 * @brief Data structure for BG script events
 */
struct BGScriptEventData
{
    BGScriptEvent eventType;
    uint32 timestamp;

    // Primary identifiers
    ObjectGuid primaryGuid;       // Player/unit causing the event
    ObjectGuid secondaryGuid;     // Victim/target if applicable
    uint32 objectiveId;           // Objective involved

    // Position data
    float x, y, z;
    uint32 mapId;

    // Faction data
    uint32 faction;               // ALLIANCE or HORDE

    // State data
    int32 stateId;                // World state ID if applicable
    int32 stateValue;             // World state value
    BGObjectiveState oldState;
    BGObjectiveState newState;

    // Score data
    uint32 allianceScore;
    uint32 hordeScore;

    // Metadata
    std::string customData;       // JSON-like additional data

    BGScriptEventData() :
        eventType(BGScriptEvent::CUSTOM_EVENT),
        timestamp(0),
        objectiveId(0),
        x(0.0f), y(0.0f), z(0.0f),
        mapId(0),
        faction(0),
        stateId(0),
        stateValue(0),
        oldState(BGObjectiveState::NEUTRAL),
        newState(BGObjectiveState::NEUTRAL),
        allianceScore(0),
        hordeScore(0)
    {}
};

// ============================================================================
// BG Static Data Structures
// ============================================================================

/**
 * @brief Static data for an objective in a battleground
 */
struct BGObjectiveData
{
    uint32 id;
    ObjectiveType type;
    std::string name;
    float x, y, z, orientation;
    uint8 strategicValue;                  // 1-10
    uint32 captureTime;                    // Default capture time in ms
    uint32 gameObjectEntry;                // Associated game object entry
    uint32 spellId;                        // Capture/interaction spell

    // World state mappings
    int32 allianceWorldState;
    int32 hordeWorldState;
    int32 neutralWorldState;
    int32 contestedWorldState;

    // Connectivity data (for path planning)
    std::vector<uint32> connectedObjectives;
    float distanceFromAllianceSpawn;
    float distanceFromHordeSpawn;

    BGObjectiveData() :
        id(0),
        type(ObjectiveType::NODE),
        x(0.0f), y(0.0f), z(0.0f), orientation(0.0f),
        strategicValue(5),
        captureTime(60000),
        gameObjectEntry(0),
        spellId(0),
        allianceWorldState(0),
        hordeWorldState(0),
        neutralWorldState(0),
        contestedWorldState(0),
        distanceFromAllianceSpawn(0.0f),
        distanceFromHordeSpawn(0.0f)
    {}
};

/**
 * @brief Static position data for strategic locations
 */
struct BGPositionData
{
    std::string name;
    float x, y, z, orientation;
    uint32 faction;                        // 0 = neutral, 1 = alliance, 2 = horde

    enum class PositionType : uint8
    {
        SPAWN_POINT = 0,
        GRAVEYARD = 1,
        STRATEGIC_POINT = 2,
        CHOKEPOINT = 3,
        SNIPER_POSITION = 4,
        DEFENSIVE_POSITION = 5,
        FLAG_ROOM = 6,
        TUNNEL_ENTRANCE = 7,
        VEHICLE_SPAWN = 8,
        HEALING_SPRING = 9,
        BUFF_LOCATION = 10,
        CUSTOM = 255
    };

    PositionType posType;
    std::string description;
    uint8 importance;                      // 1-10

    BGPositionData() :
        x(0.0f), y(0.0f), z(0.0f), orientation(0.0f),
        faction(0),
        posType(PositionType::STRATEGIC_POINT),
        importance(5)
    {}

    BGPositionData(const std::string& n, float px, float py, float pz, float o,
                   PositionType pt, uint32 fac = 0, uint8 imp = 5) :
        name(n), x(px), y(py), z(pz), orientation(o),
        faction(fac), posType(pt), importance(imp)
    {}
};

/**
 * @brief Vehicle data for siege battlegrounds
 */
struct BGVehicleData
{
    uint32 entry;
    std::string name;
    float spawnX, spawnY, spawnZ, spawnO;
    uint32 faction;
    uint32 respawnTime;                    // In milliseconds

    enum class VehicleType : uint8
    {
        DEMOLISHER = 0,
        SIEGE_ENGINE = 1,
        CATAPULT = 2,
        GLAIVE_THROWER = 3,
        GUNSHIP = 4,
        KEEP_CANNON = 5,
        RAM = 6,
        CUSTOM = 255
    };

    VehicleType vehicleType;
    uint32 maxHealth;
    uint32 attackPower;
    float movementSpeed;
    bool canAttackGates;
    bool canAttackPlayers;
    uint8 priority;                        // Usage priority 1-10

    BGVehicleData() :
        entry(0),
        spawnX(0.0f), spawnY(0.0f), spawnZ(0.0f), spawnO(0.0f),
        faction(0),
        respawnTime(180000),
        vehicleType(VehicleType::DEMOLISHER),
        maxHealth(0),
        attackPower(0),
        movementSpeed(1.0f),
        canAttackGates(true),
        canAttackPlayers(false),
        priority(5)
    {}

    BGVehicleData(uint32 e, const std::string& n, uint32 health, uint8 pri, bool attacksGates) :
        entry(e),
        name(n),
        spawnX(0.0f), spawnY(0.0f), spawnZ(0.0f), spawnO(0.0f),
        faction(0),
        respawnTime(180000),
        vehicleType(VehicleType::DEMOLISHER),
        maxHealth(health),
        attackPower(0),
        movementSpeed(1.0f),
        canAttackGates(attacksGates),
        canAttackPlayers(false),
        priority(pri)
    {}
};

/**
 * @brief World state mapping for BG scoring
 */
struct BGWorldState
{
    int32 stateId;
    std::string description;

    enum class StateType : uint8
    {
        SCORE_ALLIANCE = 0,
        SCORE_HORDE = 1,
        FLAG_STATE = 2,
        OBJECTIVE_STATE = 3,
        TIMER = 4,
        REINFORCEMENTS = 5,
        ROUND = 6,
        CUSTOM = 255
    };

    StateType stateType;
    uint32 associatedObjectiveId;          // If this state relates to an objective
    int32 defaultValue;

    BGWorldState() :
        stateId(0),
        stateType(StateType::CUSTOM),
        associatedObjectiveId(0),
        defaultValue(0)
    {}

    BGWorldState(int32 id, const std::string& desc, StateType type, int32 def = 0) :
        stateId(id), description(desc), stateType(type), associatedObjectiveId(0), defaultValue(def)
    {}
};

/**
 * @brief Phase tracking for match progression
 */
struct BGPhaseInfo
{
    enum class Phase : uint8
    {
        OPENING = 0,          // First 2-3 minutes
        EARLY_GAME = 1,       // First third
        MID_GAME = 2,         // Middle third
        LATE_GAME = 3,        // Final third
        OVERTIME = 4,         // Tied at end
        CLOSING = 5           // Final push (< 1 min)
    };

    Phase currentPhase;
    uint32 phaseStartTime;
    float matchProgress;                   // 0.0 to 1.0

    BGPhaseInfo() :
        currentPhase(Phase::OPENING),
        phaseStartTime(0),
        matchProgress(0.0f)
    {}
};

// ============================================================================
// Role Distribution Recommendation
// ============================================================================

/**
 * @brief Recommended role distribution for a given situation
 */
struct RoleDistribution
{
    std::map<BGRole, uint8> roleCounts;    // Role -> min count
    std::map<BGRole, uint8> roleMax;       // Role -> max count
    std::string reasoning;

    uint8 GetCount(BGRole role) const
    {
        auto it = roleCounts.find(role);
        return it != roleCounts.end() ? it->second : 0;
    }

    uint8 GetMax(BGRole role) const
    {
        auto it = roleMax.find(role);
        return it != roleMax.end() ? it->second : 0;
    }

    void SetRole(BGRole role, uint8 min, uint8 max)
    {
        roleCounts[role] = min;
        roleMax[role] = max;
    }
};

// ============================================================================
// IBGScript Interface
// ============================================================================

/**
 * @brief Interface for battleground-specific scripts
 *
 * Each battleground implements this interface to provide:
 * - Static map data (objectives, positions, vehicles)
 * - World state interpretation
 * - Strategy adjustments
 * - Event handling
 * - Special mechanics
 *
 * Scripts are loaded by the BattlegroundCoordinator based on map ID.
 */
class IBGScript
{
public:
    virtual ~IBGScript() = default;

    // ========================================================================
    // IDENTIFICATION
    // ========================================================================

    /**
     * @brief Get the map ID this script handles
     */
    virtual uint32 GetMapId() const = 0;

    /**
     * @brief Get a human-readable name for this battleground
     */
    virtual std::string GetName() const = 0;

    /**
     * @brief Get the battleground type enum value
     */
    virtual BGType GetBGType() const = 0;

    /**
     * @brief Get the maximum score to win
     */
    virtual uint32 GetMaxScore() const = 0;

    /**
     * @brief Get the maximum duration in milliseconds
     */
    virtual uint32 GetMaxDuration() const = 0;

    /**
     * @brief Get the team size
     */
    virtual uint8 GetTeamSize() const = 0;

    // ========================================================================
    // LIFECYCLE
    // ========================================================================

    /**
     * @brief Called when the script is loaded for a battleground instance
     * @param coordinator Pointer to the owning coordinator
     */
    virtual void OnLoad(BattlegroundCoordinator* coordinator) = 0;

    /**
     * @brief Called when the script is being unloaded
     */
    virtual void OnUnload() = 0;

    /**
     * @brief Called every coordinator update tick
     * @param diff Time since last update in milliseconds
     */
    virtual void OnUpdate(uint32 diff) = 0;

    // ========================================================================
    // STATIC DATA PROVIDERS
    // ========================================================================

    /**
     * @brief Get all objectives for this battleground
     */
    virtual std::vector<BGObjectiveData> GetObjectiveData() const = 0;

    /**
     * @brief Get spawn positions for a faction
     * @param faction ALLIANCE or HORDE
     */
    virtual std::vector<BGPositionData> GetSpawnPositions(uint32 faction) const = 0;

    /**
     * @brief Get strategic positions for tactical planning
     */
    virtual std::vector<BGPositionData> GetStrategicPositions() const = 0;

    /**
     * @brief Get graveyard positions
     * @param faction ALLIANCE or HORDE (0 for all)
     */
    virtual std::vector<BGPositionData> GetGraveyardPositions(uint32 faction) const = 0;

    /**
     * @brief Get vehicle data (for siege BGs)
     */
    virtual std::vector<BGVehicleData> GetVehicleData() const { return {}; }

    /**
     * @brief Get initial world states for this BG
     */
    virtual std::vector<BGWorldState> GetInitialWorldStates() const = 0;

    // ========================================================================
    // WORLD STATE INTERPRETATION
    // ========================================================================

    /**
     * @brief Interpret a world state change
     * @param stateId The world state ID
     * @param value The new value
     * @param outObjectiveId Output: associated objective if any
     * @param outState Output: objective state if applicable
     * @return true if this state was meaningful
     */
    virtual bool InterpretWorldState(int32 stateId, int32 value,
        uint32& outObjectiveId, BGObjectiveState& outState) const = 0;

    /**
     * @brief Extract scores from world state map
     * @param states Current world states
     * @param allianceScore Output alliance score
     * @param hordeScore Output horde score
     */
    virtual void GetScoreFromWorldStates(const std::map<int32, int32>& states,
        uint32& allianceScore, uint32& hordeScore) const = 0;

    /**
     * @brief Get the current match phase based on time/score
     * @param timeRemaining Milliseconds remaining
     * @param allianceScore Current alliance score
     * @param hordeScore Current horde score
     */
    virtual BGPhaseInfo::Phase GetMatchPhase(uint32 timeRemaining,
        uint32 allianceScore, uint32 hordeScore) const;

    // ========================================================================
    // STRATEGY
    // ========================================================================

    /**
     * @brief Get recommended role distribution for the current situation
     * @param decision Current strategic decision
     * @param scoreAdvantage Our score advantage (-1.0 to 1.0)
     * @param timeRemaining Milliseconds remaining
     */
    virtual RoleDistribution GetRecommendedRoles(
        const StrategicDecision& decision,
        float scoreAdvantage,
        uint32 timeRemaining) const = 0;

    /**
     * @brief Adjust a strategic decision based on BG-specific factors
     * @param decision The decision to potentially modify
     * @param scoreAdvantage Our score advantage (-1.0 to 1.0)
     * @param controlledCount Number of objectives we control
     * @param totalObjectives Total number of capturable objectives
     * @param timeRemaining Milliseconds remaining
     */
    virtual void AdjustStrategy(StrategicDecision& decision,
        float scoreAdvantage, uint32 controlledCount,
        uint32 totalObjectives, uint32 timeRemaining) const = 0;

    /**
     * @brief Get attack priority for an objective
     * @param objectiveId The objective
     * @param state Current state of the objective
     * @param faction Our faction
     * @return Priority 0-10 (0 = don't attack)
     */
    virtual uint8 GetObjectiveAttackPriority(uint32 objectiveId,
        BGObjectiveState state, uint32 faction) const = 0;

    /**
     * @brief Get defense priority for an objective
     * @param objectiveId The objective
     * @param state Current state of the objective
     * @param faction Our faction
     * @return Priority 0-10 (0 = don't defend)
     */
    virtual uint8 GetObjectiveDefensePriority(uint32 objectiveId,
        BGObjectiveState state, uint32 faction) const = 0;

    /**
     * @brief Calculate win probability based on current state
     * @param allianceScore Current alliance score
     * @param hordeScore Current horde score
     * @param timeRemaining Milliseconds remaining
     * @param objectivesControlled Number of objectives we control
     * @param faction Our faction
     * @return Probability 0.0 to 1.0
     */
    virtual float CalculateWinProbability(uint32 allianceScore, uint32 hordeScore,
        uint32 timeRemaining, uint32 objectivesControlled, uint32 faction) const;

    // ========================================================================
    // EVENTS
    // ========================================================================

    /**
     * @brief Handle a battleground event
     * @param event The event data
     */
    virtual void OnEvent(const BGScriptEventData& event) = 0;

    /**
     * @brief Called when the match starts
     */
    virtual void OnMatchStart() = 0;

    /**
     * @brief Called when the match ends
     * @param victory true if we won
     */
    virtual void OnMatchEnd(bool victory) = 0;

    // ========================================================================
    // MECHANICS QUERY
    // ========================================================================

    /**
     * @brief Is this a Capture-The-Flag battleground?
     */
    virtual bool IsCTF() const { return false; }

    /**
     * @brief Is this a domination (node control) battleground?
     */
    virtual bool IsDomination() const { return false; }

    /**
     * @brief Does this battleground have vehicles?
     */
    virtual bool HasVehicles() const { return false; }

    /**
     * @brief Does this battleground have multiple rounds?
     */
    virtual bool HasRounds() const { return false; }

    /**
     * @brief Is this an epic battleground (40v40)?
     */
    virtual bool IsEpic() const { return false; }

    /**
     * @brief Does this BG have a central objective (e.g., flag in EOTS)?
     */
    virtual bool HasCentralObjective() const { return false; }

    /**
     * @brief Does this BG have special resource mechanics (carts, orbs)?
     */
    virtual bool HasSpecialResources() const { return false; }

    // ========================================================================
    // CTF-SPECIFIC (override in CTF scripts)
    // ========================================================================

    /**
     * @brief Get escort formation positions around flag carrier
     * @param fcPos Flag carrier position
     * @param escortCount Number of escorts
     * @return Formation positions
     */
    virtual std::vector<Position> GetEscortFormation(
        const Position& fcPos, uint8 escortCount) const { return {}; }

    /**
     * @brief Get flag room positions for a faction
     * @param faction ALLIANCE or HORDE
     */
    virtual std::vector<Position> GetFlagRoomPositions(uint32 faction) const { return {}; }

    /**
     * @brief Get flag debuff spell ID
     * @param stackCount Number of stacks
     */
    virtual uint32 GetFlagDebuffSpellId(uint8 stackCount) const { return 0; }

    // ========================================================================
    // DOMINATION-SPECIFIC (override in domination scripts)
    // ========================================================================

    /**
     * @brief Get tick rate per node count
     * @param nodeCount Number of controlled nodes
     * @return Points per tick
     */
    virtual uint32 GetTickPoints(uint32 nodeCount) const { return 0; }

    /**
     * @brief Get optimal node control count for win
     */
    virtual uint32 GetOptimalNodeCount() const { return 0; }

    // ========================================================================
    // SIEGE-SPECIFIC (override in siege scripts)
    // ========================================================================

    /**
     * @brief Get gate destruction priority
     * @param gateId The gate objective ID
     */
    virtual uint8 GetGatePriority(uint32 gateId) const { return 0; }

    /**
     * @brief Get vehicle usage recommendation
     * @param botGuid The bot
     * @param vehicleEntry The vehicle type
     */
    virtual bool ShouldUseVehicle(ObjectGuid botGuid, uint32 vehicleEntry) const { return true; }

    // ========================================================================
    // RUNTIME BEHAVIOR
    // ========================================================================

    /**
     * @brief Execute BG-specific strategy for a bot player
     *
     * BG scripts that implement runtime behavior (movement, combat, orb pickup,
     * etc.) override this method. The BattlegroundAI delegates to this method
     * instead of embedding BG-specific logic.
     *
     * @param player The bot player to execute strategy for
     * @return true if the script handled the player's behavior
     */
    virtual bool ExecuteStrategy(::Player* player) { return false; }

    // ========================================================================
    // UTILITY
    // ========================================================================

    /**
     * @brief Check if a bot should interact with an objective
     * @param botGuid The bot
     * @param objectiveId The objective
     * @param role The bot's current role
     */
    virtual bool ShouldInteractWithObjective(ObjectGuid botGuid,
        uint32 objectiveId, BGRole role) const { return true; }

    /**
     * @brief Get a position for a specific tactical purpose
     * @param positionType Type of position needed
     * @param faction Our faction
     */
    virtual Position GetTacticalPosition(
        BGPositionData::PositionType positionType, uint32 faction) const;

    /**
     * @brief Calculate optimal path between objectives
     * @param fromObjective Starting objective
     * @param toObjective Destination objective
     * @return Intermediate positions for pathfinding
     */
    virtual std::vector<Position> GetObjectivePath(
        uint32 fromObjective, uint32 toObjective) const { return {}; }

protected:
    BattlegroundCoordinator* m_coordinator = nullptr;
};

} // namespace Playerbot::Coordination::Battleground

#endif // PLAYERBOT_AI_COORDINATION_BG_IBGSCRIPT_H
