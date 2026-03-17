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
#include "ObjectGuid.h"
#include "Position.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <atomic>
#include <shared_mutex>
#include <chrono>

// Forward declarations
class Player;
class Unit;
class Group;

namespace Playerbot
{

/**
 * @brief Movement formation types for group positioning
 */
enum class MovementFormationType : uint8
{
    NONE = 0,           // No specific formation
    LINE = 1,           // Single file line
    COLUMN = 2,         // Side by side column
    WEDGE = 3,          // V-shaped wedge
    DIAMOND = 4,        // Diamond formation
    CIRCLE = 5,         // Circular formation
    BOX = 6,            // Box/square formation
    SPREAD = 7,         // Spread out
    CLUSTER = 8,        // Tight cluster
    DEFENSIVE = 9,      // Defensive formation
    DUNGEON = 10,       // Dungeon/instance formation
    RAID = 11,          // Raid formation
    PVP = 12            // PvP formation
};

/**
 * @brief Formation roles for members
 */
enum class FormationRole : uint8
{
    LEADER = 0,         // Formation leader
    TANK = 1,           // Tank position
    HEALER = 2,         // Healer position
    MELEE_DPS = 3,      // Melee DPS position
    RANGED_DPS = 4,     // Ranged DPS position
    SUPPORT = 5,        // Support position
    SCOUT = 6,          // Scout position
    REAR_GUARD = 7      // Rear guard position
};

// Formation movement states
enum class FormationMovementState : uint8
{
    STATIONARY = 0,         // Formation is stationary
    MOVING = 1,             // Formation is moving
    REFORMING = 2,          // Formation is adjusting positions
    COMBAT = 3,             // Formation is in combat
    SCATTERED = 4,          // Formation is broken/scattered
    EMERGENCY = 5,          // Emergency movement
    TRANSITIONING = 6       // Changing to new formation
};

// Formation integrity levels
enum class FormationIntegrity : uint8
{
    PERFECT = 0,            // All members in perfect position
    GOOD = 1,               // Minor deviations acceptable
    ACCEPTABLE = 2,         // Some members out of position
    POOR = 3,               // Formation partially broken
    BROKEN = 4              // Formation completely broken
};

// Formation member information
struct FormationMember
{
    ObjectGuid guid;
    Player* player;
    FormationRole role;
    Position assignedPosition;
    Position currentPosition;
    Position targetPosition;
    float distanceFromAssigned;
    float distanceFromLeader;
    bool isInPosition;
    bool isMoving;
    uint32 lastPositionUpdate;
    float movementSpeed;
    ::std::string name;

    // Formation-specific data
    uint32 formationSlot;
    float preferredDistance;
    float maxAllowedDeviation;
    bool maintainRelativePosition;
    bool canBreakFormation;

    FormationMember() : player(nullptr), role(FormationRole::SUPPORT),
                       distanceFromAssigned(0.0f), distanceFromLeader(0.0f),
                       isInPosition(false), isMoving(false), lastPositionUpdate(0),
                       movementSpeed(0.0f), formationSlot(0), preferredDistance(5.0f),
                       maxAllowedDeviation(3.0f), maintainRelativePosition(true),
                       canBreakFormation(false) {}
};

// Formation movement command
struct FormationCommand
{
    MovementFormationType newFormation;
    Position targetPosition;
    float targetOrientation;
    FormationMovementState movementState;
    float movementSpeed;
    uint32 priority;
    uint32 timeoutMs;
    bool maintainCohesion;
    bool allowBreaking;
    ::std::string reason;

    FormationCommand() : newFormation(MovementFormationType::NONE), targetOrientation(0.0f),
                        movementState(FormationMovementState::MOVING), movementSpeed(0.0f),
                        priority(0), timeoutMs(10000), maintainCohesion(true),
                        allowBreaking(false) {}
};

// Formation configuration
struct FormationConfig
{
    MovementFormationType type;
    float baseSpacing;              // Base distance between members
    float cohesionRadius;           // Maximum allowed spread
    float reformationThreshold;     // Distance that triggers reformation
    bool maintainOrientation;       // Keep relative orientation
    bool allowDynamicAdjustment;    // Allow real-time adjustments
    bool combatFormation;           // Is this a combat formation
    ::std::vector<FormationRole> roleOrder;  // Preferred role positioning
    ::std::unordered_map<FormationRole, Position> roleOffsets;  // Role-specific offsets

    FormationConfig() : type(MovementFormationType::NONE), baseSpacing(5.0f),
                       cohesionRadius(15.0f), reformationThreshold(10.0f),
                       maintainOrientation(true), allowDynamicAdjustment(true),
                       combatFormation(false) {}
};

// Formation performance metrics
struct FormationMetrics
{
    ::std::atomic<uint32> formationChanges{0};
    ::std::atomic<uint32> membersRepositioned{0};
    ::std::atomic<uint32> cohesionBreaks{0};
    ::std::atomic<uint32> reformationEvents{0};
    ::std::chrono::microseconds averageFormationTime{0};
    ::std::chrono::microseconds maxFormationTime{0};
    float averageIntegrity{100.0f};
    float minIntegrity{100.0f};
    ::std::chrono::steady_clock::time_point lastUpdate;

    void Reset()
    {
        formationChanges = 0;
        membersRepositioned = 0;
        cohesionBreaks = 0;
        reformationEvents = 0;
        averageFormationTime = ::std::chrono::microseconds{0};
        maxFormationTime = ::std::chrono::microseconds{0};
        averageIntegrity = 100.0f;
        minIntegrity = 100.0f;
        lastUpdate = ::std::chrono::steady_clock::now();
    }
};

class TC_GAME_API FormationManager
{
public:
    explicit FormationManager(Player* bot);
    ~FormationManager() = default;

    // Formation management interface
    bool JoinFormation(const ::std::vector<Player*>& groupMembers, MovementFormationType formation = MovementFormationType::DUNGEON);
    bool LeaveFormation();
    bool ChangeFormation(MovementFormationType newFormation);
    bool SetFormationLeader(Player* leader);
    Player* GetFormationLeader() const { return _leader; }

    // Formation execution
    void UpdateFormation(uint32 diff);
    bool ExecuteFormationCommand(const FormationCommand& command);
    bool MoveFormationToPosition(const Position& targetPos, float orientation = 0.0f);
    bool AdjustFormationForCombat(const ::std::vector<Unit*>& threats);

    // Member management
    bool AddMember(Player* player, FormationRole role = FormationRole::SUPPORT);
    bool RemoveMember(Player* player);
    bool ChangeMemberRole(Player* player, FormationRole newRole);
    FormationMember* GetMember(Player* player);
    ::std::vector<FormationMember> GetAllMembers() const;

    // Position calculation
    Position CalculateFormationPosition(FormationRole role, uint32 memberIndex);
    ::std::vector<Position> CalculateAllFormationPositions();
    Position GetAssignedPosition() const;
    bool IsInFormationPosition(float tolerance = 2.0f) const;

    // Formation analysis
    FormationIntegrity AssessFormationIntegrity();
    float CalculateCohesionLevel();
    ::std::vector<Player*> GetOutOfPositionMembers(float tolerance = 3.0f);
    bool RequiresReformation();

    // Movement coordination
    void CoordinateMovement(const Position& destination);
    void MaintainFormationDuringMovement();
    bool CanMoveWithoutBreakingFormation(const Position& newPos);
    Position AdjustMovementForFormation(const Position& intendedPos);

    // Combat formations
    void TransitionToCombatFormation(const ::std::vector<Unit*>& enemies);
    void TransitionToTravelFormation();
    void AdjustForThreatSpread(const ::std::vector<Unit*>& threats);
    void HandleFormationBreakage();

    // Role-specific formations
    MovementFormationType DetermineOptimalFormation(const ::std::vector<Player*>& members);
    FormationConfig GetFormationConfig(MovementFormationType formation);
    void SetFormationConfig(MovementFormationType formation, const FormationConfig& config);

    // Dynamic adjustments
    void AdjustForTerrain();
    void AdjustForObstacles(const ::std::vector<Position>& obstacles);
    void AdjustForGroupSize();
    void HandleMemberDisconnection(Player* disconnectedMember);

    // Query methods
    MovementFormationType GetCurrentFormation() const { return _currentFormation; }
    FormationMovementState GetMovementState() const { return _movementState; }
    bool IsFormationLeader() const { return _isLeader; }
    bool IsInFormation() const { return _inFormation; }
    uint32 GetMemberCount() const { return static_cast<uint32>(_members.size()); }

    // Configuration
    void SetUpdateInterval(uint32 intervalMs) { _updateInterval = intervalMs; }
    uint32 GetUpdateInterval() const { return _updateInterval; }
    void SetCohesionRadius(float radius) { _cohesionRadius = radius; }
    float GetCohesionRadius() const { return _cohesionRadius; }
    void SetFormationSpacing(float spacing) { _formationSpacing = spacing; }
    float GetFormationSpacing() const { return _formationSpacing; }

    // Performance monitoring
    FormationMetrics const& GetMetrics() const { return _metrics; }
    void ResetMetrics() { _metrics.Reset(); }

    // Advanced features
    void EnableAdaptiveFormations(bool enable) { _adaptiveFormations = enable; }
    bool IsAdaptiveFormationsEnabled() const { return _adaptiveFormations; }
    void SetFormationPriority(uint32 priority) { _formationPriority = priority; }
    uint32 GetFormationPriority() const { return _formationPriority; }

    // Emergency handling
    void ActivateEmergencyScatter();
    void DeactivateEmergencyScatter();
    bool IsEmergencyScatterActive() const { return _emergencyScatter; }
    void HandleEmergencyRegroup(const Position& rallyPoint);

private:
    // Formation calculation methods
    ::std::vector<Position> CalculateLineFormation(const Position& leaderPos, float orientation);
    ::std::vector<Position> CalculateColumnFormation(const Position& leaderPos, float orientation);
    ::std::vector<Position> CalculateWedgeFormation(const Position& leaderPos, float orientation);
    ::std::vector<Position> CalculateDiamondFormation(const Position& leaderPos, float orientation);
    ::std::vector<Position> CalculateCircleFormation(const Position& leaderPos);
    ::std::vector<Position> CalculateBoxFormation(const Position& leaderPos, float orientation);
    ::std::vector<Position> CalculateDungeonFormation(const Position& leaderPos, float orientation);
    ::std::vector<Position> CalculateRaidFormation(const Position& leaderPos, float orientation);

    // Position assignment methods
    void AssignFormationPositions();
    Position CalculateRoleBasedPosition(FormationRole role, const Position& leaderPos, float orientation);
    uint32 GetRoleSlotIndex(FormationRole role);
    float GetRoleDistance(FormationRole role);

    // Movement coordination methods
    void UpdateMemberPositions();
    void CalculateMovementTargets();
    bool ShouldWaitForFormation();
    void IssueMovementCommands();

    // Formation maintenance methods
    void MonitorFormationIntegrity();
    void HandleOutOfPositionMembers();
    void TriggerReformationIfNeeded();
    void OptimizeFormationLayout();

    // Utility methods
    FormationRole DeterminePlayerRole(Player* player);
    float CalculateFormationRadius();
    bool IsPositionValidForFormation(const Position& pos);
    Position ClampPositionToFormation(const Position& pos);

    // Initialization methods
    void InitializeFormationConfigs();

    // Performance tracking
    void TrackPerformance(::std::chrono::microseconds duration, const ::std::string& operation);

private:
    Player* _bot;
    Player* _leader;
    bool _isLeader;
    bool _inFormation;

    // Formation state
    MovementFormationType _currentFormation;
    FormationMovementState _movementState;
    FormationIntegrity _currentIntegrity;
    ::std::vector<FormationMember> _members;
    ::std::unordered_map<MovementFormationType, FormationConfig> _formationConfigs;

    // Position tracking
    Position _formationCenter;
    float _formationOrientation;
    Position _targetDestination;
    bool _isMovingToDestination;

    // Configuration
    uint32 _updateInterval;
    float _cohesionRadius;
    float _formationSpacing;
    float _reformationThreshold;
    uint32 _formationPriority;
    bool _adaptiveFormations;
    bool _emergencyScatter;

    // Timing
    uint32 _lastUpdate;
    uint32 _lastIntegrityCheck;
    uint32 _lastReformation;

    // Performance metrics
    mutable FormationMetrics _metrics;

    // Thread safety
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE> _mutex;

    // Constants
    static constexpr uint32 DEFAULT_UPDATE_INTERVAL = 250;      // 250ms
    static constexpr float DEFAULT_COHESION_RADIUS = 15.0f;     // 15 yards
    static constexpr float DEFAULT_FORMATION_SPACING = 5.0f;    // 5 yards
    static constexpr float DEFAULT_REFORMATION_THRESHOLD = 8.0f; // 8 yards
    static constexpr uint32 INTEGRITY_CHECK_INTERVAL = 1000;    // 1 second
    static constexpr uint32 MIN_REFORMATION_INTERVAL = 3000;    // 3 seconds
};

// Formation utilities
class TC_GAME_API FormationUtils
{
public:
    // Formation type analysis
    static MovementFormationType GetOptimalFormationForGroup(const ::std::vector<Player*>& members);
    static MovementFormationType GetOptimalFormationForCombat(const ::std::vector<Player*>& members, const ::std::vector<Unit*>& enemies);
    static MovementFormationType GetOptimalFormationForTerrain(const Position& location);
    static bool IsFormationSuitableForCombat(MovementFormationType formation);

    // Position calculations
    static Position CalculateFormationCenterFromMembers(const ::std::vector<Player*>& members);
    static float CalculateOptimalSpacing(const ::std::vector<Player*>& members, MovementFormationType formation);
    static ::std::vector<Position> GenerateFormationGrid(const Position& center, uint32 rows, uint32 columns, float spacing);

    // Role assignment utilities
    static FormationRole DetermineOptimalRole(Player* player);
    static ::std::vector<FormationRole> GetRoleOrderForFormation(MovementFormationType formation);
    static bool IsRoleCompatibleWithFormation(FormationRole role, MovementFormationType formation);

    // Formation validation
    static bool IsFormationValid(const ::std::vector<Position>& positions, MovementFormationType formation);
    static float CalculateFormationEfficiency(const ::std::vector<Player*>& members, MovementFormationType formation);
    static bool CanFormationFitInArea(MovementFormationType formation, uint32 memberCount, float availableRadius);

    // Movement coordination utilities
    static ::std::vector<Position> CalculateStaggeredMovement(const ::std::vector<Player*>& members, const Position& destination);
    static float EstimateFormationMovementTime(const ::std::vector<Player*>& members, const Position& destination);
    static bool WillMovementBreakFormation(const ::std::vector<Player*>& members, const Position& destination, float threshold);

    // Combat formation utilities
    static MovementFormationType GetCounterFormation(MovementFormationType enemyFormation);
    static ::std::vector<Position> CalculateFlankingPositions(const Position& enemyCenter, uint32 flankerCount);
    static bool IsFormationVulnerableToAoE(MovementFormationType formation, float aoERadius);

    // Terrain adaptation utilities
    static MovementFormationType AdaptFormationToTerrain(MovementFormationType currentFormation, const Position& location);
    static ::std::vector<Position> AdjustFormationForObstacles(const ::std::vector<Position>& originalPositions, const ::std::vector<Position>& obstacles);
    static bool IsTerrainSuitableForFormation(MovementFormationType formation, const Position& location, float radius);
};

} // namespace Playerbot