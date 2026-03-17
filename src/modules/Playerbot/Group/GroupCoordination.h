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
#include "Threading/LockHierarchy.h"
#include "Position.h"
#include "Unit.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <atomic>
#include <mutex>
#include <chrono>

class Player;
class Unit;

namespace Playerbot
{

enum class CoordinationCommand : uint8
{
    ATTACK_TARGET       = 0,
    FOCUS_FIRE         = 1,
    SPREAD_OUT         = 2,
    STACK_UP           = 3,
    MOVE_TO_POSITION   = 4,
    FOLLOW_LEADER      = 5,
    DEFENSIVE_MODE     = 6,
    AGGRESSIVE_MODE    = 7,
    RETREAT            = 8,
    HOLD_POSITION      = 9,
    USE_COOLDOWNS      = 10,
    SAVE_COOLDOWNS     = 11,
    INTERRUPT_CAST     = 12,
    DISPEL_DEBUFFS     = 13,
    CROWD_CONTROL      = 14,
    BURN_PHASE         = 15
};

enum class ThreatLevel : uint8
{
    NONE     = 0,
    LOW      = 1,
    MEDIUM   = 2,
    HIGH     = 3,
    CRITICAL = 4
};

enum class EncounterPhase : uint8
{
    PREPARATION = 0,
    ENGAGE      = 1,
    NORMAL      = 2,
    TRANSITION  = 3,
    BURN        = 4,
    RECOVERY    = 5
};

struct CoordinationTarget
{
    ObjectGuid targetGuid;
    uint32 priority;
    ThreatLevel threatLevel;
    float estimatedTimeToKill;
    ::std::unordered_set<uint32> assignedMembers;
    Position lastKnownPosition;
    uint32 lastSeen;

    CoordinationTarget(ObjectGuid guid, uint32 prio = 100, ThreatLevel threat = ThreatLevel::MEDIUM)
        : targetGuid(guid), priority(prio), threatLevel(threat), estimatedTimeToKill(0.0f)
        , lastSeen(GameTime::GetGameTimeMS()) {}
};

struct MovementWaypoint
{
    Position position;
    float waitTime;
    bool isRequired;
    ::std::string description;

    MovementWaypoint(const Position& pos, float wait = 0.0f, bool required = false, const ::std::string& desc = "")
        : position(pos), waitTime(wait), isRequired(required), description(desc) {}
};

struct FormationSlot
{
    uint32 memberGuid;
    Position relativePosition;
    float maxDistance;
    bool isFlexible;
    ::std::string roleDescription;

    FormationSlot(uint32 guid, const Position& pos, float maxDist = 5.0f, bool flexible = true, const ::std::string& role = "")
        : memberGuid(guid), relativePosition(pos), maxDistance(maxDist), isFlexible(flexible), roleDescription(role) {}
};

class TC_GAME_API GroupCoordination
{
public:
    GroupCoordination(uint32 groupId);
    ~GroupCoordination() = default;

    // Command execution
    void ExecuteCommand(CoordinationCommand command, const ::std::vector<uint32>& targets = {});
    void IssueCommand(uint32 memberGuid, CoordinationCommand command, const ::std::vector<uint32>& targets = {});
    void BroadcastCommand(CoordinationCommand command, const ::std::vector<uint32>& targets = {});

    // Target coordination
    void SetPrimaryTarget(ObjectGuid targetGuid, uint32 priority = 100);
    void AddSecondaryTarget(ObjectGuid targetGuid, uint32 priority = 50);
    void RemoveTarget(ObjectGuid targetGuid);
    void UpdateTargetPriorities();
    ObjectGuid GetPrimaryTarget() const;
    ::std::vector<ObjectGuid> GetTargetPriorityList() const;

    // Formation management
    void SetFormation(const ::std::vector<FormationSlot>& formation);
    void UpdateFormation(const Position& leaderPosition);
    Position GetFormationPosition(uint32 memberGuid) const;
    bool IsInFormation(uint32 memberGuid, float tolerance = 2.0f) const;
    void AdjustFormation(const ::std::vector<uint32>& members);

    // Movement coordination
    void SetMovementPath(const ::std::vector<MovementWaypoint>& waypoints);
    void MoveToPosition(const Position& destination, bool maintainFormation = true);
    void FollowLeader(uint32 leaderGuid, float distance = 5.0f);
    Position GetNextWaypoint() const;
    bool HasReachedDestination() const;

    // Combat coordination
    void InitiateCombat(Unit* target);
    void UpdateCombatCoordination();
    void EndCombat();
    void SetEncounterPhase(EncounterPhase phase);
    void HandleEmergencySituation(ThreatLevel level);

    // Spell and ability coordination
    void CoordinateCooldowns(const ::std::vector<uint32>& spellIds);
    void RequestInterrupt(ObjectGuid targetGuid, uint32 spellId);
    void CoordinateDispelling(const ::std::vector<uint32>& debuffTypes);
    void HandleCrowdControl(const ::std::vector<ObjectGuid>& targets);

    // Threat management
    void UpdateThreatAssessment();
    ThreatLevel GetOverallThreatLevel() const;
    void HandleThreatRedirection(uint32 fromMember, uint32 toMember);
    void BalanceThreat();

    // Communication
    void SendCoordinationMessage(const ::std::string& message, uint32 targetMember = 0);
    void LogCoordinationEvent(const ::std::string& event);
    void NotifyMembersOfChange(const ::std::string& change);

    // Performance monitoring
    struct CoordinationMetrics
    {
        ::std::atomic<uint32> commandsIssued{0};
        ::std::atomic<uint32> commandsExecuted{0};
        ::std::atomic<float> responseTime{0.0f};
        ::std::atomic<float> formationCompliance{1.0f};
        ::std::atomic<float> targetSwitchEfficiency{1.0f};
        ::std::atomic<float> combatCoordination{1.0f};
        ::std::atomic<uint32> successfulEncounters{0};
        ::std::atomic<uint32> failedEncounters{0};
        ::std::chrono::steady_clock::time_point lastUpdate;

        void Reset()
        {
            commandsIssued = 0; commandsExecuted = 0; responseTime = 0.0f;
            formationCompliance = 1.0f; targetSwitchEfficiency = 1.0f;
            combatCoordination = 1.0f; successfulEncounters = 0; failedEncounters = 0;
            lastUpdate = ::std::chrono::steady_clock::now();
        }
    };

    CoordinationMetrics GetMetrics() const { return _metrics; }
    void UpdateMetrics();

    // State management
    bool IsActive() const { return _isActive; }
    void SetActive(bool active) { _isActive = active; }
    uint32 GetGroupId() const { return _groupId; }

    // Update cycle
    void Update(uint32 diff);

private:
    uint32 _groupId;
    ::std::atomic<bool> _isActive{true};

    // Target management
    ::std::unordered_map<ObjectGuid, CoordinationTarget> _targets;
    ObjectGuid _primaryTarget;
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::GROUP_MANAGER> _targetMutex;

    // Formation data
    ::std::vector<FormationSlot> _formation;
    Position _formationCenter;
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::GROUP_MANAGER> _formationMutex;

    // Movement data
    ::std::queue<MovementWaypoint> _movementPath;
    Position _currentDestination;
    bool _maintainFormationDuringMove;
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::GROUP_MANAGER> _movementMutex;

    // Combat state
    ::std::atomic<bool> _inCombat{false};
    EncounterPhase _currentPhase;
    ThreatLevel _overallThreat;
    ::std::chrono::steady_clock::time_point _combatStartTime;

    // Command queue
    struct CoordinationCommandData
    {
        CoordinationCommand command;
        ::std::vector<uint32> targets;
        uint32 issuerGuid;
        uint32 timestamp;
        uint32 priority;

        CoordinationCommandData(CoordinationCommand cmd, const ::std::vector<uint32>& tgts, uint32 issuer, uint32 prio = 100)
            : command(cmd), targets(tgts), issuerGuid(issuer), timestamp(GameTime::GetGameTimeMS()), priority(prio) {}
    };

    ::std::priority_queue<CoordinationCommandData> _commandQueue;
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::GROUP_MANAGER> _commandMutex;

    // Cooldown coordination
    struct CooldownCoordination
    {
        ::std::unordered_map<uint32, uint32> spellCooldowns; // spellId -> expiry time
        ::std::unordered_set<uint32> reservedCooldowns;
        ::std::queue<uint32> cooldownQueue;
        mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::GROUP_MANAGER> cooldownMutex;

        bool IsSpellOnCooldown(uint32 spellId) const
        {
            ::std::lock_guard lock(cooldownMutex);
            auto it = spellCooldowns.find(spellId);
            return it != spellCooldowns.end() && it->second > GameTime::GetGameTimeMS();
        }

        void SetSpellCooldown(uint32 spellId, uint32 duration)
        {
            ::std::lock_guard lock(cooldownMutex);
            spellCooldowns[spellId] = GameTime::GetGameTimeMS() + duration;
        }
    } _cooldownCoordination;

    // Performance tracking
    CoordinationMetrics _metrics;

    // Helper functions
    void ProcessCommandQueue();
    void UpdateTargetAssessment();
    void UpdateFormationPositions();
    void UpdateMovementProgress();
    void UpdateCombatTactics();
    float CalculateResponseTime(uint32 commandTime);
    float AssessFormationCompliance();
    void OptimizeTargetAssignments();
    bool ValidateCommand(const CoordinationCommandData& command);
    void ExecuteCommandInternal(const CoordinationCommandData& command);

    // Combat tactics
    void HandleTankThreatManagement();
    void HandleHealerPriorities();
    void HandleDPSTargeting();
    void HandleSupportActions();
    void AdaptToEncounterMechanics();

    // Emergency responses
    void HandleMemberDown(uint32 memberGuid);
    void HandleGroupWipe();
    void HandleLeaderDisconnect();
    void HandleCriticalHealth(uint32 memberGuid);

    // Constants
    static constexpr uint32 COMMAND_TIMEOUT = 5000; // 5 seconds
    static constexpr uint32 FORMATION_UPDATE_INTERVAL = 500; // 0.5 seconds
    static constexpr uint32 TARGET_UPDATE_INTERVAL = 1000; // 1 second
    static constexpr float FORMATION_TOLERANCE = 3.0f;
    static constexpr float WAYPOINT_REACH_DISTANCE = 2.0f;
    static constexpr uint32 MAX_COMMAND_QUEUE_SIZE = 50;
    static constexpr float MIN_COORDINATION_EFFICIENCY = 0.5f;
};

} // namespace Playerbot