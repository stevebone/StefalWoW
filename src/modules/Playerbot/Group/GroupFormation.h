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
#include <vector>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <chrono>

class Player;

namespace Playerbot
{

enum class FormationType : uint8
{
    LINE_FORMATION      = 0,
    WEDGE_FORMATION     = 1,
    CIRCLE_FORMATION    = 2,
    DIAMOND_FORMATION   = 3,
    DEFENSIVE_SQUARE    = 4,
    ARROW_FORMATION     = 5,
    LOOSE_FORMATION     = 6,
    CUSTOM_FORMATION    = 7
};

enum class FormationBehavior : uint8
{
    RIGID           = 0,  // Strict positioning
    FLEXIBLE        = 1,  // Adaptive positioning
    COMBAT_READY    = 2,  // Combat optimized
    TRAVEL_MODE     = 3,  // Travel optimized
    STEALTH_MODE    = 4,  // Stealth optimized
    DEFENSIVE_MODE  = 5   // Defense optimized
};

struct GroupFormationMember
{
    uint32 memberGuid;
    Position assignedPosition;
    Position currentPosition;
    float maxDeviationDistance;
    float priority;
    bool isFlexible;
    bool isLeader;
    uint32 lastPositionUpdate;

    GroupFormationMember(uint32 guid, const Position& assigned, float maxDev = 3.0f, float prio = 1.0f)
        : memberGuid(guid), assignedPosition(assigned), maxDeviationDistance(maxDev)
        , priority(prio), isFlexible(true), isLeader(false), lastPositionUpdate(GameTime::GetGameTimeMS()) {}
};

struct FormationTemplate
{
    FormationType type;
    ::std::string name;
    ::std::string description;
    ::std::vector<Position> relativePositions;
    float optimalSpacing;
    float maxFormationSize;
    FormationBehavior defaultBehavior;
    bool supportsDynamicSize;

    FormationTemplate(FormationType t, const ::std::string& n, float spacing = 5.0f)
        : type(t), name(n), optimalSpacing(spacing), maxFormationSize(25.0f)
        , defaultBehavior(FormationBehavior::FLEXIBLE), supportsDynamicSize(true) {}
};

class TC_GAME_API GroupFormation
{
public:
    GroupFormation(uint32 groupId, FormationType type = FormationType::LOOSE_FORMATION);
    ~GroupFormation() = default;

    // Formation setup
    void SetFormationType(FormationType type);
    void SetFormationBehavior(FormationBehavior behavior);
    void SetCustomFormation(const ::std::vector<Position>& positions);
    void AddMember(uint32 memberGuid, const Position& preferredPosition = Position());
    void RemoveMember(uint32 memberGuid);

    // Formation management
    void UpdateFormation(const Position& centerPosition, float direction = 0.0f);
    void AdjustForTerrain(const ::std::vector<Position>& obstacles);
    void OptimizeSpacing(float minDistance, float maxDistance);
    void RotateFormation(float angle);
    void ScaleFormation(float scaleFactor);

    // Position queries
    Position GetAssignedPosition(uint32 memberGuid) const;
    Position GetFormationCenter() const;
    float GetFormationRadius() const;
    bool IsInFormation(uint32 memberGuid, float tolerance = 2.0f) const;
    ::std::vector<uint32> GetMembersOutOfPosition(float tolerance = 3.0f) const;

    // Dynamic adjustments
    void HandleMemberMovement(uint32 memberGuid, const Position& newPosition);
    void AdaptToMovement(const Position& leaderPosition, float leaderDirection);
    void HandleCombatFormation(const ::std::vector<Position>& enemyPositions);
    void HandleObstacleAvoidance(const ::std::vector<Position>& obstacles);

    // Formation validation
    bool IsFormationValid() const;
    float CalculateFormationCoherence() const;
    float CalculateFormationEfficiency() const;
    bool CanMaintainFormation(const Position& destination) const;

    // Formation templates
    static FormationTemplate GetFormationTemplate(FormationType type);
    static ::std::vector<FormationType> GetAvailableFormations();
    void ApplyFormationTemplate(const FormationTemplate& formationTemplate);

    // Member management
    void SetMemberPriority(uint32 memberGuid, float priority);
    void SetMemberFlexibility(uint32 memberGuid, bool isFlexible);
    void AssignLeader(uint32 leaderGuid);
    uint32 GetFormationLeader() const { return _leaderGuid; }

    // Performance monitoring (snapshot for external use)
    struct FormationMetricsSnapshot
    {
        float averageDeviation = 0.0f;
        float formationStability = 1.0f;
        float movementEfficiency = 1.0f;
        uint32 positionAdjustments = 0;
        uint32 formationBreaks = 0;
        uint32 terrainCollisions = 0;
        ::std::chrono::steady_clock::time_point lastUpdate;
    };

    FormationMetricsSnapshot GetMetrics() const
    {
        FormationMetricsSnapshot snapshot;
        snapshot.averageDeviation = _metrics.averageDeviation.load();
        snapshot.formationStability = _metrics.formationStability.load();
        snapshot.movementEfficiency = _metrics.movementEfficiency.load();
        snapshot.positionAdjustments = _metrics.positionAdjustments.load();
        snapshot.formationBreaks = _metrics.formationBreaks.load();
        snapshot.terrainCollisions = _metrics.terrainCollisions.load();
        snapshot.lastUpdate = _metrics.lastUpdate;
        return snapshot;
    }
    void UpdateMetrics();

    // State management
    bool IsActive() const { return _isActive; }
    void SetActive(bool active) { _isActive = active; }
    FormationType GetFormationType() const { return _formationType; }
    FormationBehavior GetFormationBehavior() const { return _behavior; }

    // Update cycle
    void Update(uint32 diff);

private:
    uint32 _groupId;
    FormationType _formationType;
    FormationBehavior _behavior;
    uint32 _leaderGuid;
    ::std::atomic<bool> _isActive{true};

    // Formation data
    ::std::vector<GroupFormationMember> _members;
    Position _formationCenter;
    float _formationDirection;
    float _formationSpacing;
    float _formationRadius;
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::GROUP_MANAGER> _formationMutex;

    // Movement tracking
    Position _lastCenterPosition;
    float _lastDirection;
    uint32 _lastUpdateTime;
    ::std::vector<Position> _recentPositions;

    // Terrain and obstacle data
    ::std::vector<Position> _knownObstacles;
    ::std::unordered_map<uint32, Position> _terrainAdjustments;
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::GROUP_MANAGER> _terrainMutex;

    // Internal atomic metrics for thread-safe storage
    struct AtomicFormationMetrics
    {
        ::std::atomic<float> averageDeviation{0.0f};
        ::std::atomic<float> formationStability{1.0f};
        ::std::atomic<float> movementEfficiency{1.0f};
        ::std::atomic<uint32> positionAdjustments{0};
        ::std::atomic<uint32> formationBreaks{0};
        ::std::atomic<uint32> terrainCollisions{0};
        ::std::chrono::steady_clock::time_point lastUpdate;

        void Reset()
        {
            averageDeviation = 0.0f; formationStability = 1.0f; movementEfficiency = 1.0f;
            positionAdjustments = 0; formationBreaks = 0; terrainCollisions = 0;
            lastUpdate = ::std::chrono::steady_clock::now();
        }
    };

    // Performance tracking
    AtomicFormationMetrics _metrics;

    // Formation templates
    static ::std::unordered_map<FormationType, FormationTemplate> _formationTemplates;
    static void InitializeFormationTemplates();

    // Helper functions
    void RecalculateFormationPositions();
    Position CalculateMemberPosition(const GroupFormationMember& member) const;
    float CalculateOptimalSpacing() const;
    void HandleFormationCollisions();
    void UpdateMemberPositions();
    bool ValidateTerrainAtPosition(const Position& pos) const;
    Position AdjustPositionForTerrain(const Position& originalPos) const;
    void OptimizeFormationForCombat(const ::std::vector<Position>& threats);
    void SmoothFormationTransition(const Position& newCenter, float newDirection);

    // Formation algorithms
    ::std::vector<Position> GenerateLineFormation(uint32 memberCount, float spacing) const;
    ::std::vector<Position> GenerateWedgeFormation(uint32 memberCount, float spacing) const;
    ::std::vector<Position> GenerateCircleFormation(uint32 memberCount, float spacing) const;
    ::std::vector<Position> GenerateDiamondFormation(uint32 memberCount, float spacing) const;
    ::std::vector<Position> GenerateDefensiveSquare(uint32 memberCount, float spacing) const;
    ::std::vector<Position> GenerateArrowFormation(uint32 memberCount, float spacing) const;
    ::std::vector<Position> GenerateLooseFormation(uint32 memberCount, float spacing) const;

    // Dynamic adjustment algorithms
    void PerformFormationSmoothing();
    void HandleCollisionResolution();
    void ApplyFlexibilityAdjustments();
    void BalanceFormationLoad();

    // Pathfinding integration
    bool IsPathClearBetweenPositions(const Position& start, const Position& end) const;
    Position FindNearestValidPosition(const Position& desired, float maxSearchRadius = 5.0f) const;
    ::std::vector<Position> GenerateFormationPath(const Position& destination) const;

    // Constants
    static constexpr float MIN_FORMATION_SPACING = 1.5f;
    static constexpr float MAX_FORMATION_SPACING = 15.0f;
    static constexpr float DEFAULT_FORMATION_SPACING = 5.0f;
    static constexpr float FORMATION_UPDATE_THRESHOLD = 1.0f;
    static constexpr uint32 POSITION_HISTORY_SIZE = 10;
    static constexpr float COLLISION_DETECTION_RADIUS = 1.0f;
    static constexpr float TERRAIN_ADJUSTMENT_RADIUS = 2.0f;
    static constexpr uint32 FORMATION_SMOOTHING_INTERVAL = 500; // 0.5 seconds
    static constexpr float MAX_FORMATION_DEVIATION = 10.0f;
    static constexpr float FORMATION_BREAK_THRESHOLD = 15.0f;
    static constexpr float MIN_COORDINATION_EFFICIENCY = 0.5f;
    static constexpr float FORMATION_TOLERANCE = 2.0f;
};

} // namespace Playerbot