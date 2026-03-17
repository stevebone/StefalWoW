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

#include "ObjectGuid.h"
#include <cstdint>
#include <vector>
#include <string>

namespace Playerbot {

// ============================================================================
// BATTLEGROUND ENUMS
// ============================================================================

enum class BGState : uint8
{
    IDLE = 0,
    QUEUED = 1,
    PREPARATION = 2,
    ACTIVE = 3,
    OVERTIME = 4,
    VICTORY = 5,
    DEFEAT = 6
};

enum class BGType : uint32
{
    WARSONG_GULCH = 489,
    ARATHI_BASIN = 529,
    ALTERAC_VALLEY = 30,
    EYE_OF_THE_STORM = 566,
    STRAND_OF_THE_ANCIENTS = 607,
    ISLE_OF_CONQUEST = 628,
    TWIN_PEAKS = 726,
    BATTLE_FOR_GILNEAS = 761,
    SILVERSHARD_MINES = 727,
    TEMPLE_OF_KOTMOGU = 998,
    DEEPWIND_GORGE = 1105,
    SEETHING_SHORE = 1803,
    ASHRAN = 1191
};

enum class BGRole : uint8
{
    UNASSIGNED = 0,
    FLAG_CARRIER = 1,
    FLAG_ESCORT = 2,
    FLAG_HUNTER = 3,         // Hunt enemy FC
    NODE_ATTACKER = 4,
    NODE_DEFENDER = 5,
    ROAMER = 6,
    HEALER_OFFENSE = 7,
    HEALER_DEFENSE = 8,
    CART_PUSHER = 9,         // Silvershard Mines
    ORB_CARRIER = 10,        // Temple of Kotmogu
    GRAVEYARD_ASSAULT = 11,
    RESOURCE_GATHERER = 12,  // Deepwind Gorge
    VEHICLE_DRIVER = 13,     // Isle of Conquest, Strand of the Ancients
    VEHICLE_GUNNER = 14,     // Vehicle passenger/gunner
    BOSS_ASSAULT = 15,       // Isle of Conquest boss push
    TURRET_OPERATOR = 16,    // Strand of the Ancients turrets

    // Generic roles used by BattlegroundAI
    FLAG_DEFENDER = 17,      // Defend flag room in CTF
    HEALER_SUPPORT = 18,     // Generic healer support role
    ATTACKER = 19,           // Generic attacker role
    DEFENDER = 20,           // Generic defender role
    BASE_CAPTURER = 21,      // Capture bases/nodes
    BASE_DEFENDER = 22,      // Defend bases/nodes
    SIEGE_OPERATOR = 23      // Operate siege vehicles
};

enum class ObjectiveType : uint8
{
    FLAG = 1,
    NODE = 2,
    TOWER = 3,
    GRAVEYARD = 4,
    GATE = 5,
    CART = 6,
    ORB = 7,
    BOSS = 8,
    MINE = 9,
    WORKSHOP = 10,
    RELIC = 11,
    STRATEGIC = 12,      // Generic strategic position
    CONTROL_POINT = 13,  // Capturable control point (AB, BFG)
    CAPTURABLE = 14      // Generic capturable objective
};

// Objective action types (what action is being performed)
enum class BGObjectiveType : uint8
{
    CAPTURE_FLAG = 1,    // Capturing enemy flag
    DEFEND_FLAG = 2,     // Defending/returning friendly flag
    CAPTURE_BASE = 3,    // Capturing a base/node
    DEFEND_BASE = 4,     // Defending a base/node
    CAPTURE_TOWER = 5,   // Capturing a tower
    DESTROY_GATE = 6,    // Destroying a gate
    PUSH_CART = 7,       // Pushing a mine cart
    CARRY_ORB = 8,       // Carrying an orb
    KILL_BOSS = 9,       // Killing a boss
    GENERAL = 10         // Generic objective action
};

enum class BGObjectiveState : uint8
{
    NEUTRAL = 0,
    ALLIANCE_CONTROLLED = 1,
    HORDE_CONTROLLED = 2,
    ALLIANCE_CONTESTED = 3,
    HORDE_CONTESTED = 4,
    ALLIANCE_CAPTURING = 5,
    HORDE_CAPTURING = 6,
    DESTROYED = 7,
    CONTESTED = 8,             // Generic contested (not faction-specific)
    CONTROLLED_FRIENDLY = 9,   // Controlled by our team (context-dependent)
    CONTROLLED_ENEMY = 10      // Controlled by enemy team (context-dependent)
};

enum class BGPriority : uint8
{
    IGNORE = 0,
    LOW = 1,
    NORMAL = 2,
    HIGH = 3,
    CRITICAL = 4
};

// ============================================================================
// BATTLEGROUND STRUCTURES
// ============================================================================

/// @brief Position structure for coordinates
struct BGPosition
{
    float x, y, z;
    BGPosition() : x(0), y(0), z(0) {}
    BGPosition(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
};

struct BGObjective
{
    uint32 id;
    uint32& objectiveId = id;    // Alias reference to id
    ObjectiveType type;
    BGObjectiveState state;
    ::std::string name;
    float x, y, z;

    // Position as a struct for compatibility
    BGPosition position;

    // Capture progress
    float captureProgress;  // 0.0 - 1.0
    uint32 captureTime;     // Time when capture will complete
    uint32 contestedSince;  // When contesting started

    // Assignment
    ::std::vector<ObjectGuid> assignedDefenders;
    ::std::vector<ObjectGuid> assignedAttackers;

    // Strategic value
    uint8 strategicValue;   // 1-10
    bool isContested;
    BGPriority currentPriority;
    BGPriority& priority = currentPriority;    // Alias reference to currentPriority

    // Additional properties
    uint32 controllingFaction;   // ALLIANCE or HORDE or 0 for neutral
    uint32 resourceValue;        // Resource points this objective provides

    // Location info
    float nearbyEnemyCount;
    float nearbyAllyCount;

    BGObjective()
        : id(0), type(ObjectiveType::NODE), state(BGObjectiveState::NEUTRAL),
          x(0), y(0), z(0), position(0, 0, 0),
          captureProgress(0), captureTime(0), contestedSince(0),
          strategicValue(5), isContested(false),
          currentPriority(BGPriority::NORMAL),
          controllingFaction(0), resourceValue(0),
          nearbyEnemyCount(0), nearbyAllyCount(0) {}

    // Copy constructor (needed due to reference members)
    BGObjective(const BGObjective& other)
        : id(other.id), type(other.type), state(other.state), name(other.name),
          x(other.x), y(other.y), z(other.z), position(other.position),
          captureProgress(other.captureProgress), captureTime(other.captureTime),
          contestedSince(other.contestedSince),
          assignedDefenders(other.assignedDefenders), assignedAttackers(other.assignedAttackers),
          strategicValue(other.strategicValue), isContested(other.isContested),
          currentPriority(other.currentPriority),
          controllingFaction(other.controllingFaction), resourceValue(other.resourceValue),
          nearbyEnemyCount(other.nearbyEnemyCount), nearbyAllyCount(other.nearbyAllyCount) {}

    // Copy assignment operator (needed due to reference members)
    BGObjective& operator=(const BGObjective& other)
    {
        if (this != &other)
        {
            id = other.id;
            type = other.type;
            state = other.state;
            name = other.name;
            x = other.x; y = other.y; z = other.z;
            position = other.position;
            captureProgress = other.captureProgress;
            captureTime = other.captureTime;
            contestedSince = other.contestedSince;
            assignedDefenders = other.assignedDefenders;
            assignedAttackers = other.assignedAttackers;
            strategicValue = other.strategicValue;
            isContested = other.isContested;
            currentPriority = other.currentPriority;
            controllingFaction = other.controllingFaction;
            resourceValue = other.resourceValue;
            nearbyEnemyCount = other.nearbyEnemyCount;
            nearbyAllyCount = other.nearbyAllyCount;
        }
        return *this;
    }

    // Sync position with x,y,z
    void SetPosition(float _x, float _y, float _z)
    {
        x = _x; y = _y; z = _z;
        position = BGPosition(_x, _y, _z);
    }
};

struct BGScoreInfo
{
    uint32 allianceScore;
    uint32 hordeScore;
    uint32 maxScore;
    uint32 allianceResources;
    uint32 hordeResources;
    uint32 timeRemaining;

    // Context-dependent score (set by coordinator based on faction)
    uint32 friendlyScore;
    uint32 enemyScore;

    // Flag-specific
    uint32 allianceFlagCaptures;
    uint32 hordeFlagCaptures;

    // Resource income rate
    float allianceResourceRate;
    float hordeResourceRate;

    BGScoreInfo()
        : allianceScore(0), hordeScore(0), maxScore(0),
          allianceResources(0), hordeResources(0), timeRemaining(0),
          friendlyScore(0), enemyScore(0),
          allianceFlagCaptures(0), hordeFlagCaptures(0),
          allianceResourceRate(0), hordeResourceRate(0) {}
};

struct FlagInfo
{
    ObjectGuid carrierGuid;
    bool isPickedUp;
    bool isAtBase;
    bool isDropped;
    float x, y, z;
    BGPosition droppedPosition;   // Position where flag was dropped
    uint8 stackCount;             // For debuffs (focused assault, etc.)
    uint32 pickupTime;
    uint32 dropTime;

    FlagInfo()
        : carrierGuid(), isPickedUp(false), isAtBase(true), isDropped(false),
          x(0), y(0), z(0), droppedPosition(), stackCount(0), pickupTime(0), dropTime(0) {}
};

struct BGPlayer
{
    ObjectGuid guid;
    uint32 classId;
    BGRole role;
    BGPriority threat;

    // Status
    float healthPercent;
    float manaPercent;
    bool isAlive;
    bool isInCombat;
    bool hasFlag;

    // Position
    float x, y, z;
    uint32 nearestObjectiveId;
    float distanceToObjective;

    // Performance
    uint32 kills;
    uint32 deaths;
    uint32 honorableKills;
    uint32 objectivesAssisted;

    BGPlayer()
        : guid(), classId(0), role(BGRole::UNASSIGNED), threat(BGPriority::NORMAL),
          healthPercent(100.0f), manaPercent(100.0f), isAlive(true),
          isInCombat(false), hasFlag(false),
          x(0), y(0), z(0), nearestObjectiveId(0), distanceToObjective(0),
          kills(0), deaths(0), honorableKills(0), objectivesAssisted(0) {}
};

struct BGRoleAssignment
{
    ObjectGuid player;
    BGRole role;
    uint32 objectiveId;     // If assigned to specific objective
    uint32 assignTime;
    float efficiency;       // 0-1, how well performing role

    BGRoleAssignment()
        : player(), role(BGRole::UNASSIGNED), objectiveId(0),
          assignTime(0), efficiency(0.5f) {}
};

struct BGMatchStats
{
    uint32 matchStartTime;
    uint32 matchDuration;
    uint32 remainingTime;     // Time remaining in match
    BGType bgType;

    // Score tracking
    uint32 peakScoreAdvantage;
    uint32 peakScoreDisadvantage;

    // Objective tracking
    uint32 objectivesCaptured;
    uint32 objectivesLost;
    uint32 objectivesDefended;

    // Combat tracking
    uint32 totalKills;
    uint32 totalDeaths;
    uint32 flagCaptures;
    uint32 flagReturns;

    BGMatchStats()
        : matchStartTime(0), matchDuration(0), remainingTime(0), bgType(BGType::WARSONG_GULCH),
          peakScoreAdvantage(0), peakScoreDisadvantage(0),
          objectivesCaptured(0), objectivesLost(0), objectivesDefended(0),
          totalKills(0), totalDeaths(0), flagCaptures(0), flagReturns(0) {}
};

// ============================================================================
// STRING CONVERSION UTILITIES
// ============================================================================

inline const char* BGStateToString(BGState state)
{
    switch (state)
    {
        case BGState::IDLE: return "IDLE";
        case BGState::QUEUED: return "QUEUED";
        case BGState::PREPARATION: return "PREPARATION";
        case BGState::ACTIVE: return "ACTIVE";
        case BGState::OVERTIME: return "OVERTIME";
        case BGState::VICTORY: return "VICTORY";
        case BGState::DEFEAT: return "DEFEAT";
        default: return "UNKNOWN";
    }
}

inline const char* BGRoleToString(BGRole role)
{
    switch (role)
    {
        case BGRole::UNASSIGNED: return "UNASSIGNED";
        case BGRole::FLAG_CARRIER: return "FLAG_CARRIER";
        case BGRole::FLAG_ESCORT: return "FLAG_ESCORT";
        case BGRole::FLAG_HUNTER: return "FLAG_HUNTER";
        case BGRole::NODE_ATTACKER: return "NODE_ATTACKER";
        case BGRole::NODE_DEFENDER: return "NODE_DEFENDER";
        case BGRole::ROAMER: return "ROAMER";
        case BGRole::HEALER_OFFENSE: return "HEALER_OFFENSE";
        case BGRole::HEALER_DEFENSE: return "HEALER_DEFENSE";
        case BGRole::CART_PUSHER: return "CART_PUSHER";
        case BGRole::ORB_CARRIER: return "ORB_CARRIER";
        case BGRole::GRAVEYARD_ASSAULT: return "GRAVEYARD_ASSAULT";
        case BGRole::RESOURCE_GATHERER: return "RESOURCE_GATHERER";
        case BGRole::VEHICLE_DRIVER: return "VEHICLE_DRIVER";
        case BGRole::VEHICLE_GUNNER: return "VEHICLE_GUNNER";
        case BGRole::BOSS_ASSAULT: return "BOSS_ASSAULT";
        case BGRole::TURRET_OPERATOR: return "TURRET_OPERATOR";
        default: return "UNKNOWN";
    }
}

inline const char* BGTypeToString(BGType type)
{
    switch (type)
    {
        case BGType::WARSONG_GULCH: return "WARSONG_GULCH";
        case BGType::ARATHI_BASIN: return "ARATHI_BASIN";
        case BGType::ALTERAC_VALLEY: return "ALTERAC_VALLEY";
        case BGType::EYE_OF_THE_STORM: return "EYE_OF_THE_STORM";
        case BGType::STRAND_OF_THE_ANCIENTS: return "STRAND_OF_ANCIENTS";
        case BGType::ISLE_OF_CONQUEST: return "ISLE_OF_CONQUEST";
        case BGType::TWIN_PEAKS: return "TWIN_PEAKS";
        case BGType::BATTLE_FOR_GILNEAS: return "BATTLE_FOR_GILNEAS";
        case BGType::SILVERSHARD_MINES: return "SILVERSHARD_MINES";
        case BGType::TEMPLE_OF_KOTMOGU: return "TEMPLE_OF_KOTMOGU";
        case BGType::DEEPWIND_GORGE: return "DEEPWIND_GORGE";
        case BGType::SEETHING_SHORE: return "SEETHING_SHORE";
        case BGType::ASHRAN: return "ASHRAN";
        default: return "UNKNOWN";
    }
}

inline const char* ObjectiveTypeToString(ObjectiveType type)
{
    switch (type)
    {
        case ObjectiveType::FLAG: return "FLAG";
        case ObjectiveType::NODE: return "NODE";
        case ObjectiveType::TOWER: return "TOWER";
        case ObjectiveType::GRAVEYARD: return "GRAVEYARD";
        case ObjectiveType::GATE: return "GATE";
        case ObjectiveType::CART: return "CART";
        case ObjectiveType::ORB: return "ORB";
        case ObjectiveType::BOSS: return "BOSS";
        case ObjectiveType::MINE: return "MINE";
        case ObjectiveType::WORKSHOP: return "WORKSHOP";
        case ObjectiveType::RELIC: return "RELIC";
        case ObjectiveType::STRATEGIC: return "STRATEGIC";
        case ObjectiveType::CONTROL_POINT: return "CONTROL_POINT";
        case ObjectiveType::CAPTURABLE: return "CAPTURABLE";
        default: return "UNKNOWN";
    }
}

inline const char* BGObjectiveStateToString(BGObjectiveState state)
{
    switch (state)
    {
        case BGObjectiveState::NEUTRAL: return "NEUTRAL";
        case BGObjectiveState::ALLIANCE_CONTROLLED: return "ALLIANCE_CONTROLLED";
        case BGObjectiveState::HORDE_CONTROLLED: return "HORDE_CONTROLLED";
        case BGObjectiveState::ALLIANCE_CONTESTED: return "ALLIANCE_CONTESTED";
        case BGObjectiveState::HORDE_CONTESTED: return "HORDE_CONTESTED";
        case BGObjectiveState::ALLIANCE_CAPTURING: return "ALLIANCE_CAPTURING";
        case BGObjectiveState::HORDE_CAPTURING: return "HORDE_CAPTURING";
        case BGObjectiveState::DESTROYED: return "DESTROYED";
        case BGObjectiveState::CONTESTED: return "CONTESTED";
        case BGObjectiveState::CONTROLLED_FRIENDLY: return "CONTROLLED_FRIENDLY";
        case BGObjectiveState::CONTROLLED_ENEMY: return "CONTROLLED_ENEMY";
        default: return "UNKNOWN";
    }
}

} // namespace Playerbot
