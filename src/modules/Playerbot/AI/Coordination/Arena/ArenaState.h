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
#include <map>
#include <string>

namespace Playerbot {

// ============================================================================
// ARENA ENUMS
// ============================================================================

enum class ArenaState : uint8
{
    IDLE = 0,
    QUEUED = 1,
    PREPARATION = 2,    // In arena, gates closed
    GATES_OPENING = 3,  // 5 second countdown
    COMBAT = 4,         // Active combat
    VICTORY = 5,
    DEFEAT = 6
};

enum class ArenaType : uint8
{
    ARENA_2V2 = 2,
    ARENA_3V3 = 3,
    ARENA_5V5 = 5,
    SKIRMISH_2v2 = 12,
    SKIRMISH_3v3 = 13
};

enum class ArenaBracket : uint8
{
    SKIRMISH = 0,
    RATED = 1,
    SOLO_SHUFFLE = 2
};

enum class ArenaRole : uint8
{
    UNKNOWN = 0,
    HEALER = 1,
    MELEE_DPS = 2,
    RANGED_DPS = 3,
    HYBRID = 4      // Can swap roles
};

enum class TargetPriority : uint8
{
    IGNORE = 0,     // In CC, don't touch
    LOW = 1,
    NORMAL = 2,
    HIGH = 3,
    KILL_TARGET = 4
};

enum class DefensiveState : uint8
{
    HEALTHY = 0,        // >80% HP, no pressure
    PRESSURED = 1,      // 50-80% HP, taking damage
    IN_DANGER = 2,      // 30-50% HP, need help
    CRITICAL = 3,       // <30% HP, emergency
    USING_DEFENSIVES = 4 // Personal defensives active
};

enum class BurstPhase : uint8
{
    NONE = 0,
    PREPARING = 1,      // Setting up (CC, positioning)
    EXECUTING = 2,      // Cooldowns popped, going in
    SUSTAINING = 3,     // Maintaining pressure
    RETREATING = 4      // Burst failed, back off
};

// ============================================================================
// ARENA STRUCTURES
// ============================================================================

struct ArenaEnemy
{
    ObjectGuid guid;
    uint32 classId;
    uint32 specId;
    ArenaRole role;

    // Tracked cooldowns
    bool trinketAvailable;
    uint32 trinketCooldown;
    ::std::map<uint32, uint32> majorCooldowns;  // spellId -> readyTime

    // Status
    float healthPercent;
    float manaPercent;
    bool isInCC;
    uint32 ccEndTime;
    bool isInDefensiveCooldown;
    uint32 defensiveEndTime;
    TargetPriority currentPriority;

    // Position tracking
    float lastKnownX;
    float lastKnownY;
    float lastKnownZ;
    uint32 lastSeenTime;
    bool isLOSBlocked;

    ArenaEnemy()
        : guid(), classId(0), specId(0), role(ArenaRole::UNKNOWN),
          trinketAvailable(true), trinketCooldown(0),
          healthPercent(100.0f), manaPercent(100.0f),
          isInCC(false), ccEndTime(0),
          isInDefensiveCooldown(false), defensiveEndTime(0),
          currentPriority(TargetPriority::NORMAL),
          lastKnownX(0), lastKnownY(0), lastKnownZ(0),
          lastSeenTime(0), isLOSBlocked(false) {}
};

struct ArenaTeammate
{
    ObjectGuid guid;
    uint32 classId;
    uint32 specId;
    ArenaRole role;

    // Resources
    float healthPercent;
    float manaPercent;
    DefensiveState defensiveState;

    // Cooldowns available
    bool hasDefensivesAvailable;
    bool hasCCAvailable;
    bool hasBurstAvailable;
    bool hasInterruptAvailable;

    // Status
    bool needsPeel;
    bool isCC;
    uint32 ccEndTime;
    uint8 ccBreakPriority;  // 0 = don't break, 3 = break immediately

    // Position
    float x, y, z;
    float distanceToNearestEnemy;
    float distanceToHealer;

    ArenaTeammate()
        : guid(), classId(0), specId(0), role(ArenaRole::UNKNOWN),
          healthPercent(100.0f), manaPercent(100.0f),
          defensiveState(DefensiveState::HEALTHY),
          hasDefensivesAvailable(true), hasCCAvailable(true),
          hasBurstAvailable(true), hasInterruptAvailable(true),
          needsPeel(false), isCC(false), ccEndTime(0), ccBreakPriority(0),
          x(0), y(0), z(0),
          distanceToNearestEnemy(0), distanceToHealer(0) {}
};

struct BurstWindow
{
    uint32 startTime;
    uint32 duration;
    ObjectGuid target;
    ::std::vector<ObjectGuid> participants;
    BurstPhase phase;
    bool isActive;

    // Success tracking
    float targetHealthAtStart;
    float lowestHealthReached;
    bool targetKilled;
    bool targetUsedTrinket;
    bool targetUsedDefensive;

    BurstWindow()
        : startTime(0), duration(0), target(),
          phase(BurstPhase::NONE), isActive(false),
          targetHealthAtStart(100.0f), lowestHealthReached(100.0f),
          targetKilled(false), targetUsedTrinket(false),
          targetUsedDefensive(false) {}

    void Reset()
    {
        startTime = 0;
        duration = 0;
        target = ObjectGuid::Empty;
        participants.clear();
        phase = BurstPhase::NONE;
        isActive = false;
        targetHealthAtStart = 100.0f;
        lowestHealthReached = 100.0f;
        targetKilled = false;
        targetUsedTrinket = false;
        targetUsedDefensive = false;
    }
};

struct CCRequest
{
    ObjectGuid target;
    ObjectGuid requester;
    uint32 requestTime;
    uint32 desiredDurationMs;
    uint8 priority;     // Higher = more urgent
    bool isFilled;
    ObjectGuid assignedCCer;
    uint32 assignedSpellId;

    CCRequest()
        : target(), requester(), requestTime(0),
          desiredDurationMs(0), priority(0), isFilled(false),
          assignedCCer(), assignedSpellId(0) {}
};

struct PeelRequest
{
    ObjectGuid teammate;
    ObjectGuid threat;
    uint32 requestTime;
    uint8 urgency;      // 1-3, higher = more urgent
    bool isFilled;
    ObjectGuid assignedPeeler;

    PeelRequest()
        : teammate(), threat(), requestTime(0),
          urgency(0), isFilled(false), assignedPeeler() {}
};

struct ArenaMatchStats
{
    uint32 matchStartTime;
    uint32 matchDuration;
    uint8 teamSize;

    // Kill tracking
    uint32 killsScored;
    uint32 deathsSuffered;
    ObjectGuid firstBlood;

    // Damage
    uint32 totalDamageDealt;
    uint32 totalDamageTaken;
    uint32 totalHealingDone;

    // CC tracking
    uint32 totalCCApplied;
    uint32 totalCCReceived;
    uint32 trinketsForcedOnEnemies;
    uint32 trinketsUsedByTeam;

    // Burst tracking
    uint32 burstWindowsInitiated;
    uint32 burstWindowsSuccessful;

    ArenaMatchStats()
        : matchStartTime(0), matchDuration(0), teamSize(0),
          killsScored(0), deathsSuffered(0), firstBlood(),
          totalDamageDealt(0), totalDamageTaken(0), totalHealingDone(0),
          totalCCApplied(0), totalCCReceived(0),
          trinketsForcedOnEnemies(0), trinketsUsedByTeam(0),
          burstWindowsInitiated(0), burstWindowsSuccessful(0) {}
};

// ============================================================================
// STRING CONVERSION UTILITIES
// ============================================================================

inline const char* ArenaStateToString(ArenaState state)
{
    switch (state)
    {
        case ArenaState::IDLE: return "IDLE";
        case ArenaState::QUEUED: return "QUEUED";
        case ArenaState::PREPARATION: return "PREPARATION";
        case ArenaState::GATES_OPENING: return "GATES_OPENING";
        case ArenaState::COMBAT: return "COMBAT";
        case ArenaState::VICTORY: return "VICTORY";
        case ArenaState::DEFEAT: return "DEFEAT";
        default: return "UNKNOWN";
    }
}

inline const char* ArenaRoleToString(ArenaRole role)
{
    switch (role)
    {
        case ArenaRole::UNKNOWN: return "UNKNOWN";
        case ArenaRole::HEALER: return "HEALER";
        case ArenaRole::MELEE_DPS: return "MELEE_DPS";
        case ArenaRole::RANGED_DPS: return "RANGED_DPS";
        case ArenaRole::HYBRID: return "HYBRID";
        default: return "UNKNOWN";
    }
}

inline const char* TargetPriorityToString(TargetPriority priority)
{
    switch (priority)
    {
        case TargetPriority::IGNORE: return "IGNORE";
        case TargetPriority::LOW: return "LOW";
        case TargetPriority::NORMAL: return "NORMAL";
        case TargetPriority::HIGH: return "HIGH";
        case TargetPriority::KILL_TARGET: return "KILL_TARGET";
        default: return "UNKNOWN";
    }
}

inline const char* DefensiveStateToString(DefensiveState state)
{
    switch (state)
    {
        case DefensiveState::HEALTHY: return "HEALTHY";
        case DefensiveState::PRESSURED: return "PRESSURED";
        case DefensiveState::IN_DANGER: return "IN_DANGER";
        case DefensiveState::CRITICAL: return "CRITICAL";
        case DefensiveState::USING_DEFENSIVES: return "USING_DEFENSIVES";
        default: return "UNKNOWN";
    }
}

inline const char* BurstPhaseToString(BurstPhase phase)
{
    switch (phase)
    {
        case BurstPhase::NONE: return "NONE";
        case BurstPhase::PREPARING: return "PREPARING";
        case BurstPhase::EXECUTING: return "EXECUTING";
        case BurstPhase::SUSTAINING: return "SUSTAINING";
        case BurstPhase::RETREATING: return "RETREATING";
        default: return "UNKNOWN";
    }
}

} // namespace Playerbot
