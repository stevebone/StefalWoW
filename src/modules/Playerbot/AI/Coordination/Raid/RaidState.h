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
#include <map>

namespace Playerbot {

// ============================================================================
// RAID STATE ENUMS
// ============================================================================

/**
 * @enum RaidState
 * @brief Overall raid progression state
 */
enum class RaidState : uint8
{
    IDLE = 0,               // Raid not active
    FORMING = 1,            // Building the raid group
    BUFFING = 2,            // Pre-pull buffing phase
    PULLING = 3,            // Tank initiating pull
    COMBAT = 4,             // Active combat encounter
    PHASE_TRANSITION = 5,   // Boss phase change
    WIPED = 6,              // Group wipe occurred
    RECOVERING = 7          // Post-wipe recovery
};

/**
 * @enum TankRole
 * @brief Specific role of a tank in the raid
 */
enum class TankRole : uint8
{
    ACTIVE = 0,         // Main tank on boss
    SWAP_READY = 1,     // Ready to taunt
    ADD_DUTY = 2,       // Handling adds
    KITING = 3,         // Kiting adds/boss
    RECOVERING = 4,     // Recovering from debuffs
    OFF_TANK = 5        // Secondary tank backup
};

/**
 * @enum HealerAssignment
 * @brief Healer duty assignments
 */
enum class HealerAssignment : uint8
{
    RAID_HEALING = 0,       // General raid healing
    TANK_1 = 1,             // Main tank healer
    TANK_2 = 2,             // Off-tank healer
    GROUP_1 = 11,           // Group 1 healer
    GROUP_2 = 12,
    GROUP_3 = 13,
    GROUP_4 = 14,
    GROUP_5 = 15,
    GROUP_6 = 16,
    GROUP_7 = 17,
    GROUP_8 = 18,
    DISPEL_DUTY = 20,       // Priority dispelling
    MOBILE = 21,            // Flexible assignment
    EXTERNAL_CD = 22        // External cooldown focus
};

/**
 * @enum RaidDifficulty
 * @brief Raid instance difficulty
 * @note WoW 12.0: Changed from uint8 to int16 to match core Difficulty type
 */
enum class RaidDifficulty : int16
{
    NORMAL = 0,
    HEROIC = 1,
    MYTHIC = 2,
    LFR = 3
};

/**
 * @enum EncounterPhase
 * @brief Boss encounter phase
 */
enum class EncounterPhase : uint8
{
    PHASE_1 = 1,
    PHASE_2 = 2,
    PHASE_3 = 3,
    PHASE_4 = 4,
    INTERMISSION = 10,
    SOFT_ENRAGE = 20,
    HARD_ENRAGE = 21
};

/**
 * @enum MechanicType
 * @brief Types of boss mechanics
 */
enum class MechanicType : uint8
{
    NONE = 0,
    TANK_SWAP = 1,          // Tank swap required
    SPREAD = 2,             // Spread out
    STACK = 3,              // Stack together
    SOAK = 4,               // Soak mechanics
    DODGE = 5,              // Dodge area
    INTERRUPT = 6,          // Interrupt required
    DISPEL = 7,             // Dispel required
    ADD_SPAWN = 8,          // Adds spawning
    MOVEMENT = 9,           // Specific movement
    FRONTAL = 10,           // Frontal cone
    TARGETED = 11           // Target-based mechanic
};

/**
 * @enum AddPriority
 * @brief Priority level for add management
 */
enum class AddPriority : uint8
{
    LOW = 0,
    NORMAL = 1,
    HIGH = 2,
    CRITICAL = 3,       // Must die immediately
    IGNORE = 10         // Don't target
};

/**
 * @enum CooldownType
 * @brief Types of raid cooldowns
 */
enum class CooldownType : uint8
{
    BLOODLUST = 0,          // Bloodlust/Heroism/Time Warp
    RAID_DEFENSIVE = 1,     // Spirit Link, Rallying Cry, etc.
    EXTERNAL = 2,           // Pain Suppression, Ironbark, etc.
    BATTLE_REZ = 3,         // Combat resurrection
    DAMAGE = 4,             // Damage cooldowns
    PERSONAL = 5            // Personal defensives
};

// ============================================================================
// RAID STATE STRUCTS
// ============================================================================

/**
 * @struct RaidTankInfo
 * @brief Information about a tank in the raid
 */
struct RaidTankInfo
{
    ObjectGuid guid;
    TankRole role;
    uint8 debuffStacks;             // Swap-trigger debuff stacks
    ObjectGuid currentTarget;        // Current tank target
    bool isMainTank;
    uint32 tauntCooldown;           // Time until taunt ready
    uint32 lastSwapTime;            // When last swap occurred
    float threatPercentage;          // Current threat on target

    RaidTankInfo()
        : role(TankRole::OFF_TANK), debuffStacks(0), isMainTank(false),
          tauntCooldown(0), lastSwapTime(0), threatPercentage(0.0f) {}
};

/**
 * @struct RaidHealerInfo
 * @brief Information about a healer in the raid
 */
struct RaidHealerInfo
{
    ObjectGuid guid;
    HealerAssignment assignment;
    float manaPercent;
    bool hasExternalAvailable;       // Has external CD ready
    uint32 externalCooldown;         // Time until external ready
    ObjectGuid assignedTarget;       // Tank/player assigned to
    uint8 groupAssignment;           // Sub-group number if group healer

    RaidHealerInfo()
        : assignment(HealerAssignment::RAID_HEALING), manaPercent(100.0f),
          hasExternalAvailable(true), externalCooldown(0), groupAssignment(0) {}
};

/**
 * @struct RaidSubGroup
 * @brief A sub-group (1-8) within the raid
 */
struct RaidSubGroup
{
    uint8 groupNumber;
    ::std::vector<ObjectGuid> members;
    bool hasTank;
    bool hasHealer;
    uint8 meleeCount;
    uint8 rangedCount;

    RaidSubGroup()
        : groupNumber(0), hasTank(false), hasHealer(false),
          meleeCount(0), rangedCount(0) {}
};

/**
 * @struct KiteWaypoint
 * @brief A point in a kiting path
 */
struct KiteWaypoint
{
    float x;
    float y;
    float z;
    uint32 waitTime;        // Time to wait at this point (ms)
    bool shouldStop;        // Should stop or continue moving

    KiteWaypoint()
        : x(0), y(0), z(0), waitTime(0), shouldStop(false) {}

    KiteWaypoint(float px, float py, float pz)
        : x(px), y(py), z(pz), waitTime(0), shouldStop(false) {}
};

/**
 * @struct KitePath
 * @brief A complete kiting path with waypoints
 */
struct KitePath
{
    uint32 pathId;
    ::std::string pathName;
    ::std::vector<KiteWaypoint> waypoints;
    ObjectGuid assignedKiter;
    bool isLoop;                    // Does path loop back to start
    float safeDistance;             // Minimum distance to maintain

    KitePath()
        : pathId(0), isLoop(false), safeDistance(15.0f) {}
};

/**
 * @struct RaidAdd
 * @brief Information about an add in a raid encounter
 */
struct RaidAdd
{
    ObjectGuid guid;
    uint32 creatureId;
    AddPriority priority;
    bool requiresTank;
    ObjectGuid assignedTank;
    ::std::vector<ObjectGuid> assignedDPS;
    float healthPercent;
    bool isActiveTarget;            // Currently being focused
    uint32 spawnTime;

    RaidAdd()
        : creatureId(0), priority(AddPriority::NORMAL), requiresTank(false),
          healthPercent(100.0f), isActiveTarget(false), spawnTime(0) {}
};

/**
 * @struct TankSwapTrigger
 * @brief Configuration for automatic tank swaps
 */
struct TankSwapTrigger
{
    uint32 debuffSpellId;           // Spell that triggers swap
    uint8 stackThreshold;           // Stacks at which to swap
    uint32 debuffDuration;          // How long debuff lasts
    bool swapOnCast;                // Swap on cast, not on stacks
    ::std::string description;

    TankSwapTrigger()
        : debuffSpellId(0), stackThreshold(0), debuffDuration(0), swapOnCast(false) {}
};

/**
 * @struct EncounterMechanic
 * @brief A single boss mechanic
 */
struct EncounterMechanic
{
    uint32 spellId;
    MechanicType type;
    EncounterPhase phase;           // Which phase this occurs in
    float x, y, z;                  // Position (for movement mechanics)
    float radius;                   // Radius of effect
    uint32 castTime;                // Time to react
    ::std::string description;

    EncounterMechanic()
        : spellId(0), type(MechanicType::NONE), phase(EncounterPhase::PHASE_1),
          x(0), y(0), z(0), radius(0), castTime(0) {}
};

/**
 * @struct RaidCooldownEntry
 * @brief A raid cooldown in the rotation
 */
struct RaidCooldownEntry
{
    ObjectGuid playerGuid;
    uint32 spellId;
    CooldownType type;
    uint32 cooldownDuration;        // Full cooldown time
    uint32 remainingCooldown;       // Time until ready
    bool isAvailable;
    uint8 priority;                 // Order in rotation

    RaidCooldownEntry()
        : spellId(0), type(CooldownType::PERSONAL), cooldownDuration(0),
          remainingCooldown(0), isAvailable(true), priority(0) {}
};

/**
 * @struct PositionAssignment
 * @brief A player's assigned position
 */
struct PositionAssignment
{
    ObjectGuid playerGuid;
    ::std::string positionName;     // e.g., "Ranged Stack Point"
    float x, y, z;
    float allowedDeviation;         // How far from position is acceptable

    PositionAssignment()
        : x(0), y(0), z(0), allowedDeviation(5.0f) {}
};

/**
 * @struct RaidEncounterInfo
 * @brief Complete encounter information
 */
struct RaidEncounterInfo
{
    uint32 encounterId;
    ::std::string bossName;
    uint8 totalPhases;
    EncounterPhase currentPhase;
    uint32 enrageTimer;             // Time until enrage
    uint32 combatStartTime;
    ::std::vector<TankSwapTrigger> swapTriggers;
    ::std::vector<EncounterMechanic> mechanics;
    ::std::map<EncounterPhase, float> phaseHealthThresholds;

    RaidEncounterInfo()
        : encounterId(0), totalPhases(1), currentPhase(EncounterPhase::PHASE_1),
          enrageTimer(0), combatStartTime(0) {}
};

/**
 * @struct RaidMatchStats
 * @brief Overall raid performance statistics
 */
struct RaidMatchStats
{
    uint32 wipeCount;
    uint32 totalDeaths;
    uint32 battleRezUsed;
    uint32 bloodlustUsed;
    uint32 bestAttemptHealthPercent;    // Lowest boss health achieved
    uint32 combatTime;
    uint32 totalDamageDealt;
    uint32 totalHealingDone;

    RaidMatchStats()
        : wipeCount(0), totalDeaths(0), battleRezUsed(0), bloodlustUsed(0),
          bestAttemptHealthPercent(100), combatTime(0), totalDamageDealt(0),
          totalHealingDone(0) {}
};

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

inline const char* RaidStateToString(RaidState state)
{
    switch (state)
    {
        case RaidState::IDLE:               return "IDLE";
        case RaidState::FORMING:            return "FORMING";
        case RaidState::BUFFING:            return "BUFFING";
        case RaidState::PULLING:            return "PULLING";
        case RaidState::COMBAT:             return "COMBAT";
        case RaidState::PHASE_TRANSITION:   return "PHASE_TRANSITION";
        case RaidState::WIPED:              return "WIPED";
        case RaidState::RECOVERING:         return "RECOVERING";
        default:                            return "UNKNOWN";
    }
}

inline const char* TankRoleToString(TankRole role)
{
    switch (role)
    {
        case TankRole::ACTIVE:      return "ACTIVE";
        case TankRole::SWAP_READY:  return "SWAP_READY";
        case TankRole::ADD_DUTY:    return "ADD_DUTY";
        case TankRole::KITING:      return "KITING";
        case TankRole::RECOVERING:  return "RECOVERING";
        case TankRole::OFF_TANK:    return "OFF_TANK";
        default:                    return "UNKNOWN";
    }
}

inline const char* HealerAssignmentToString(HealerAssignment assignment)
{
    switch (assignment)
    {
        case HealerAssignment::RAID_HEALING:    return "RAID_HEALING";
        case HealerAssignment::TANK_1:          return "TANK_1";
        case HealerAssignment::TANK_2:          return "TANK_2";
        case HealerAssignment::DISPEL_DUTY:     return "DISPEL_DUTY";
        case HealerAssignment::MOBILE:          return "MOBILE";
        case HealerAssignment::EXTERNAL_CD:     return "EXTERNAL_CD";
        default:
            if (static_cast<uint8>(assignment) >= 11 &&
                static_cast<uint8>(assignment) <= 18)
                return "GROUP_HEALER";
            return "UNKNOWN";
    }
}

inline const char* MechanicTypeToString(MechanicType type)
{
    switch (type)
    {
        case MechanicType::NONE:        return "NONE";
        case MechanicType::TANK_SWAP:   return "TANK_SWAP";
        case MechanicType::SPREAD:      return "SPREAD";
        case MechanicType::STACK:       return "STACK";
        case MechanicType::SOAK:        return "SOAK";
        case MechanicType::DODGE:       return "DODGE";
        case MechanicType::INTERRUPT:   return "INTERRUPT";
        case MechanicType::DISPEL:      return "DISPEL";
        case MechanicType::ADD_SPAWN:   return "ADD_SPAWN";
        case MechanicType::MOVEMENT:    return "MOVEMENT";
        case MechanicType::FRONTAL:     return "FRONTAL";
        case MechanicType::TARGETED:    return "TARGETED";
        default:                        return "UNKNOWN";
    }
}

} // namespace Playerbot
