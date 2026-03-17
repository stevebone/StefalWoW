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
#include "Arbiter/MovementArbiter.h"
#include "Pathfinding/PathfindingAdapter.h"
#include "AI/Combat/FormationManager.h"
#include "AI/Combat/PositionManager.h"
#include "Player.h"
#include "Group.h"
#include "Position.h"
#include <memory>
#include <atomic>

namespace Playerbot
{

/**
 * @brief Unified movement coordination system
 *
 * Consolidates four separate managers into one cohesive system:
 * - MovementArbiter: Movement request arbitration
 * - PathfindingAdapter: Path calculation and caching
 * - FormationManager: Group formation management
 * - PositionManager: Combat positioning and tactical movement
 *
 * **Architecture:**
 * ```
 * UnifiedMovementCoordinator
 *   > ArbiterModule       (movement request arbitration)
 *   > PathfindingModule   (path calculation, caching)
 *   > FormationModule     (group formations)
 *   > PositionModule      (combat positioning)
 * ```
 *
 * **Thread Safety:**
 * - Uses OrderedMutex<MOVEMENT_ARBITER> for all operations
 * - Modules share data through thread-safe interfaces
 * - Lock ordering prevents deadlocks
 *
 * **Migration Path:**
 * - Old managers (MovementArbiter, PathfindingAdapter, etc.) still work
 * - New code should use UnifiedMovementCoordinator
 * - Gradually migrate callsites over time
 * - Eventually deprecate old managers
 */
class TC_GAME_API UnifiedMovementCoordinator final 
{
public:
    explicit UnifiedMovementCoordinator(Player* bot);
    ~UnifiedMovementCoordinator();

    // Non-copyable, non-movable
    UnifiedMovementCoordinator(UnifiedMovementCoordinator const&) = delete;
    UnifiedMovementCoordinator& operator=(UnifiedMovementCoordinator const&) = delete;
    UnifiedMovementCoordinator(UnifiedMovementCoordinator&&) = delete;
    UnifiedMovementCoordinator& operator=(UnifiedMovementCoordinator&&) = delete;

    // ========================================================================
    // ARBITER MODULE INTERFACE
    // ========================================================================

    bool RequestMovement(MovementRequest const& request);
    void ClearPendingRequests();
    void StopMovement();
    void Update(uint32 diff);
    MovementArbiterStatistics const& GetArbiterStatistics() const;
    void ResetArbiterStatistics();
    ::std::string GetArbiterDiagnosticString() const;
    void LogArbiterStatistics() const;
    MovementArbiterConfig GetArbiterConfig() const;
    void SetArbiterConfig(MovementArbiterConfig const& config);
    void SetDiagnosticLogging(bool enable);
    uint32 GetPendingRequestCount() const;
    bool HasPendingRequests() const;

    // ========================================================================
    // PATHFINDING MODULE INTERFACE
    // ========================================================================

    bool InitializePathfinding(uint32 cacheSize = 100, uint32 cacheDuration = 5000);
    void ShutdownPathfinding();
    bool CalculatePath(Player* bot, Position const& destination, MovementPath& path, bool forceDirect = false);
    bool CalculatePathToUnit(Player* bot, Unit* target, MovementPath& path, float range = 0.0f);
    bool CalculateFormationPath(Player* bot, Unit* leader, Position const& offset, MovementPath& path);
    bool CalculateFleePath(Player* bot, Unit* threat, float distance, MovementPath& path);
    bool HasCachedPath(Player* bot, Position const& destination) const;
    bool GetCachedPath(Player* bot, Position const& destination, MovementPath& path) const;
    void ClearPathCache(Player* bot);
    void ClearAllPathCache();
    void SetPathParameters(uint32 maxNodes = 3000, float straightDistance = 10.0f, float maxSearchDistance = 100.0f);
    void EnablePathSmoothing(bool enable);
    void EnablePathCaching(bool enable);
    void SetCacheParameters(uint32 maxSize, uint32 duration);
    void GetCacheStatistics(uint32& hits, uint32& misses, uint32& evictions) const;
    void GetPathStatistics(uint32& totalPaths, uint32& avgTime, uint32& maxTime) const;
    void ResetPathStatistics();
    bool IsWalkablePosition(Map* map, Position const& position) const;
    bool GetNearestWalkablePosition(Map* map, Position const& position, Position& walkable, float searchRange = 20.0f) const;

    // ========================================================================
    // FORMATION MODULE INTERFACE
    // ========================================================================

    bool JoinFormation(::std::vector<Player*> const& groupMembers, MovementFormationType formation = MovementFormationType::DUNGEON);
    bool LeaveFormation();
    bool ChangeFormation(MovementFormationType newFormation);
    bool SetFormationLeader(Player* leader);
    Player* GetFormationLeader() const;
    void UpdateFormation(uint32 diff);
    bool ExecuteFormationCommand(FormationCommand const& command);
    bool MoveFormationToPosition(Position const& targetPos, float orientation = 0.0f);
    bool AdjustFormationForCombat(::std::vector<Unit*> const& threats);
    bool AddFormationMember(Player* player, FormationRole role = FormationRole::SUPPORT);
    bool RemoveFormationMember(Player* player);
    bool ChangeFormationMemberRole(Player* player, FormationRole newRole);
    FormationMember* GetFormationMember(Player* player);
    ::std::vector<FormationMember> GetAllFormationMembers() const;
    Position CalculateFormationPosition(FormationRole role, uint32 memberIndex);
    ::std::vector<Position> CalculateAllFormationPositions();
    Position GetAssignedFormationPosition() const;
    bool IsInFormationPosition(float tolerance = 2.0f) const;
    FormationIntegrity AssessFormationIntegrity();
    float CalculateCohesionLevel();
    ::std::vector<Player*> GetOutOfPositionMembers(float tolerance = 3.0f);
    bool RequiresReformation();
    void CoordinateFormationMovement(Position const& destination);
    void MaintainFormationDuringMovement();
    bool CanMoveWithoutBreakingFormation(Position const& newPos);
    Position AdjustMovementForFormation(Position const& intendedPos);
    void TransitionToCombatFormation(::std::vector<Unit*> const& enemies);
    void TransitionToTravelFormation();
    void AdjustForThreatSpread(::std::vector<Unit*> const& threats);
    void HandleFormationBreakage();
    MovementFormationType DetermineOptimalFormation(::std::vector<Player*> const& members);
    FormationConfig GetFormationConfig(MovementFormationType formation);
    void SetFormationConfig(MovementFormationType formation, FormationConfig const& config);
    void AdjustFormationForTerrain();
    void AdjustFormationForObstacles(::std::vector<Position> const& obstacles);
    void AdjustFormationForGroupSize();
    void HandleMemberDisconnection(Player* disconnectedMember);
    MovementFormationType GetCurrentFormation() const;
    FormationMovementState GetFormationMovementState() const;
    bool IsFormationLeader() const;
    bool IsInFormation() const;
    uint32 GetFormationMemberCount() const;
    void SetFormationUpdateInterval(uint32 intervalMs);
    uint32 GetFormationUpdateInterval() const;
    void SetCohesionRadius(float radius);
    float GetCohesionRadius() const;
    void SetFormationSpacing(float spacing);
    float GetFormationSpacing() const;
    FormationMetrics const& GetFormationMetrics() const;
    void ResetFormationMetrics();
    void EnableAdaptiveFormations(bool enable);
    bool IsAdaptiveFormationsEnabled() const;
    void SetFormationPriority(uint32 priority);
    uint32 GetFormationPriority() const;
    void ActivateEmergencyScatter();
    void DeactivateEmergencyScatter();
    bool IsEmergencyScatterActive() const;
    void HandleEmergencyRegroup(Position const& rallyPoint);

    // ========================================================================
    // POSITION MODULE INTERFACE
    // ========================================================================

    MovementResult UpdatePosition(MovementContext const& context);
    MovementResult FindOptimalPosition(MovementContext const& context);
    MovementResult ExecuteMovement(Position const& targetPos, MovementPriority priority);
    PositionInfo EvaluatePosition(Position const& pos, MovementContext const& context);
    ::std::vector<PositionInfo> EvaluatePositions(::std::vector<Position> const& positions, MovementContext const& context);
    ::std::vector<Position> GenerateCandidatePositions(MovementContext const& context);
    Position FindRangePosition(Unit* target, float minRange, float maxRange, float preferredAngle = 0.0f);
    Position FindMeleePosition(Unit* target, bool preferBehind = true);
    Position FindRangedPosition(Unit* target, float preferredRange = 25.0f);
    Position FindHealingPosition(::std::vector<Player*> const& allies);
    Position FindKitingPosition(Unit* threat, float minDistance = 15.0f);
    Position FindTankPosition(Unit* target);
    Position FindDpsPosition(Unit* target, PositionType type = PositionType::MELEE_COMBAT);
    Position FindHealerPosition(::std::vector<Player*> const& groupMembers);
    Position FindSupportPosition(::std::vector<Player*> const& groupMembers);
    bool IsPositionSafe(Position const& pos, MovementContext const& context);
    bool IsInDangerZone(Position const& pos);
    Position FindSafePosition(Position const& fromPos, float minDistance = 10.0f);
    Position FindEscapePosition(::std::vector<Unit*> const& threats);
    void RegisterAoEZone(AoEZone const& zone);
    void UpdateAoEZones(uint32 currentTime);
    void ClearExpiredZones(uint32 currentTime);
    ::std::vector<AoEZone> GetActiveZones() const;
    bool ValidatePosition(Position const& pos, PositionValidation flags);
    bool HasLineOfSight(Position const& from, Position const& to);
    bool IsWalkable(Position const& pos);
    float CalculateMovementCost(Position const& from, Position const& to);
    Position FindFormationPositionForRole(::std::vector<Player*> const& groupMembers, PositionType formationType);
    bool ShouldMaintainGroupProximity();
    float GetOptimalGroupDistance(uint8 role);
    bool ShouldStrafe(Unit* target);
    bool ShouldCircleStrafe(Unit* target);
    Position CalculateStrafePosition(Unit* target, bool strafeLeft = true);
    Position PredictTargetPosition(Unit* target, float timeAhead);
    void SetPositionWeights(PositionWeights const& weights);
    PositionWeights const& GetPositionWeights() const;
    void SetPositionUpdateInterval(uint32 intervalMs);
    uint32 GetPositionUpdateInterval() const;
    PositionMetrics const& GetPositionMetrics() const;
    void ResetPositionMetrics();
    MovementResult HandleEmergencyMovement(MovementContext const& context);
    bool IsInEmergencyPosition();
    Position FindEmergencyEscapePosition();
    void RecordPositionSuccess(Position const& pos, PositionType type);
    void RecordPositionFailure(Position const& pos, ::std::string const& reason);
    float GetPositionSuccessRate(Position const& pos, float radius = 5.0f);

    // ========================================================================
    // UNIFIED OPERATIONS
    // ========================================================================

    void CoordinateCompleteMovement(Player* bot, MovementContext const& context);
    ::std::string GetMovementRecommendation(Player* bot, MovementContext const& context);
    void OptimizeBotMovement(Player* bot);
    ::std::string GetMovementStatistics() const;

private:
    // ========================================================================
    // INTERNAL MODULES
    // ========================================================================

    /**
     * @brief Arbiter module - movement request arbitration
     */
    class ArbiterModule
    {
    public:
        explicit ArbiterModule(Player* bot);

        // Delegates to MovementArbiter instance
        bool RequestMovement(MovementRequest const& request);
        void ClearPendingRequests();
        void StopMovement();
        void Update(uint32 diff);
        MovementArbiterStatistics const& GetStatistics() const;
        void ResetStatistics();
        ::std::string GetDiagnosticString() const;
        void LogStatistics() const;
        MovementArbiterConfig GetConfig() const;
        void SetConfig(MovementArbiterConfig const& config);
        void SetDiagnosticLogging(bool enable);
        uint32 GetPendingRequestCount() const;
        bool HasPendingRequests() const;

        // Metrics accessors
        uint64 GetRequestsProcessed() const { return _requestsProcessed.load(); }

    private:
        ::std::unique_ptr<MovementArbiter> _arbiter;
        ::std::atomic<uint64> _requestsProcessed{0};
    };

    /**
     * @brief Pathfinding module - path calculation and caching
     */
    class PathfindingModule
    {
    public:
        PathfindingModule();

        // Delegates to PathfindingAdapter singleton
        bool Initialize(uint32 cacheSize, uint32 cacheDuration);
        void Shutdown();
        bool CalculatePath(Player* bot, Position const& destination, MovementPath& path, bool forceDirect);
        bool CalculatePathToUnit(Player* bot, Unit* target, MovementPath& path, float range);
        bool CalculateFormationPath(Player* bot, Unit* leader, Position const& offset, MovementPath& path);
        bool CalculateFleePath(Player* bot, Unit* threat, float distance, MovementPath& path);
        bool HasCachedPath(Player* bot, Position const& destination) const;
        bool GetCachedPath(Player* bot, Position const& destination, MovementPath& path) const;
        void ClearCache(Player* bot);
        void ClearAllCache();
        void SetPathParameters(uint32 maxNodes, float straightDistance, float maxSearchDistance);
        void EnableSmoothing(bool enable);
        void EnableCaching(bool enable);
        void SetCacheParameters(uint32 maxSize, uint32 duration);
        void GetCacheStatistics(uint32& hits, uint32& misses, uint32& evictions) const;
        void GetPathStatistics(uint32& totalPaths, uint32& avgTime, uint32& maxTime) const;
        void ResetStatistics();
        bool IsWalkablePosition(Map* map, Position const& position) const;
        bool GetNearestWalkablePosition(Map* map, Position const& position, Position& walkable, float searchRange) const;

        // Metrics accessors
        uint64 GetPathsCalculated() const { return _pathsCalculated.load(); }

    private:
        ::std::unique_ptr<PathfindingAdapter> _adapter;
        ::std::atomic<uint64> _pathsCalculated{0};
    };

    /**
     * @brief Formation module - group formation management
     */
    class FormationModule
    {
    public:
        explicit FormationModule(Player* bot);

        // Delegates to FormationManager instance
        bool JoinFormation(::std::vector<Player*> const& groupMembers, MovementFormationType formation);
        bool LeaveFormation();
        bool ChangeFormation(MovementFormationType newFormation);
        bool SetFormationLeader(Player* leader);
        Player* GetFormationLeader() const;
        void UpdateFormation(uint32 diff);
        bool ExecuteFormationCommand(FormationCommand const& command);
        bool MoveFormationToPosition(Position const& targetPos, float orientation);
        bool AdjustFormationForCombat(::std::vector<Unit*> const& threats);
        bool AddMember(Player* player, FormationRole role);
        bool RemoveMember(Player* player);
        bool ChangeMemberRole(Player* player, FormationRole newRole);
        FormationMember* GetMember(Player* player);
        ::std::vector<FormationMember> GetAllMembers() const;
        Position CalculateFormationPosition(FormationRole role, uint32 memberIndex);
        ::std::vector<Position> CalculateAllFormationPositions();
        Position GetAssignedPosition() const;
        bool IsInFormationPosition(float tolerance) const;
        FormationIntegrity AssessIntegrity();
        float CalculateCohesion();
        ::std::vector<Player*> GetOutOfPositionMembers(float tolerance);
        bool RequiresReformation();
        void CoordinateMovement(Position const& destination);
        void MaintainFormationDuringMovement();
        bool CanMoveWithoutBreaking(Position const& newPos);
        Position AdjustMovementForFormation(Position const& intendedPos);
        void TransitionToCombatFormation(::std::vector<Unit*> const& enemies);
        void TransitionToTravelFormation();
        void AdjustForThreatSpread(::std::vector<Unit*> const& threats);
        void HandleBreakage();
        MovementFormationType DetermineOptimalFormation(::std::vector<Player*> const& members);
        FormationConfig GetConfig(MovementFormationType formation);
        void SetConfig(MovementFormationType formation, FormationConfig const& config);
        void AdjustForTerrain();
        void AdjustForObstacles(::std::vector<Position> const& obstacles);
        void AdjustForGroupSize();
        void HandleMemberDisconnection(Player* disconnectedMember);
        MovementFormationType GetCurrentFormation() const;
        FormationMovementState GetMovementState() const;
        bool IsLeader() const;
        bool IsInFormation() const;
        uint32 GetMemberCount() const;
        void SetUpdateInterval(uint32 intervalMs);
        uint32 GetUpdateInterval() const;
        void SetCohesionRadius(float radius);
        float GetCohesionRadius() const;
        void SetFormationSpacing(float spacing);
        float GetFormationSpacing() const;
        FormationMetrics const& GetMetrics() const;
        void ResetMetrics();
        void EnableAdaptive(bool enable);
        bool IsAdaptiveEnabled() const;
        void SetPriority(uint32 priority);
        uint32 GetPriority() const;
        void ActivateEmergencyScatter();
        void DeactivateEmergencyScatter();
        bool IsEmergencyScatterActive() const;
        void HandleEmergencyRegroup(Position const& rallyPoint);

        // Metrics accessors
        uint64 GetFormationsExecuted() const { return _formationsExecuted.load(); }

    private:
        ::std::unique_ptr<FormationManager> _manager;
        ::std::atomic<uint64> _formationsExecuted{0};
    };

    /**
     * @brief Position module - combat positioning and tactical movement
     */
    class PositionModule
    {
    public:
        PositionModule(Player* bot, BotThreatManager* threatManager);

        // Delegates to PositionManager instance
        MovementResult UpdatePosition(MovementContext const& context);
        PositionMovementResult FindOptimalPosition(MovementContext const& context);
        MovementResult ExecuteMovement(Position const& targetPos, MovementPriority priority);
        PositionInfo EvaluatePosition(Position const& pos, MovementContext const& context);
        ::std::vector<PositionInfo> EvaluatePositions(::std::vector<Position> const& positions, MovementContext const& context);
        ::std::vector<Position> GenerateCandidatePositions(MovementContext const& context);
        Position FindRangePosition(Unit* target, float minRange, float maxRange, float preferredAngle);
        Position FindMeleePosition(Unit* target, bool preferBehind);
        Position FindRangedPosition(Unit* target, float preferredRange);
        Position FindHealingPosition(::std::vector<Player*> const& allies);
        Position FindKitingPosition(Unit* threat, float minDistance);
        Position FindTankPosition(Unit* target);
        Position FindDpsPosition(Unit* target, PositionType type);
        Position FindHealerPosition(::std::vector<Player*> const& groupMembers);
        Position FindSupportPosition(::std::vector<Player*> const& groupMembers);
        bool IsPositionSafe(Position const& pos, MovementContext const& context);
        bool IsInDangerZone(Position const& pos);
        Position FindSafePosition(Position const& fromPos, float minDistance);
        Position FindEscapePosition(::std::vector<Unit*> const& threats);
        void RegisterAoEZone(AoEZone const& zone);
        void UpdateAoEZones(uint32 currentTime);
        void ClearExpiredZones(uint32 currentTime);
        ::std::vector<AoEZone> GetActiveZones() const;
        bool ValidatePosition(Position const& pos, PositionValidation flags);
        bool HasLineOfSight(Position const& from, Position const& to);
        bool IsWalkable(Position const& pos);
        float CalculateMovementCost(Position const& from, Position const& to);
        Position FindFormationPosition(::std::vector<Player*> const& groupMembers, PositionType formationType);
        bool ShouldMaintainGroupProximity();
        float GetOptimalGroupDistance(uint8 role);
        bool ShouldStrafe(Unit* target);
        bool ShouldCircleStrafe(Unit* target);
        Position CalculateStrafePosition(Unit* target, bool strafeLeft);
        Position PredictTargetPosition(Unit* target, float timeAhead);
        void SetWeights(PositionWeights const& weights);
        PositionWeights const& GetWeights() const;
        void SetUpdateInterval(uint32 intervalMs);
        uint32 GetUpdateInterval() const;
        PositionMetrics const& GetMetrics() const;
        void ResetMetrics();
        MovementResult HandleEmergencyMovement(MovementContext const& context);
        bool IsInEmergencyPosition();
        Position FindEmergencyEscapePosition();
        void RecordPositionSuccess(Position const& pos, PositionType type);
        void RecordPositionFailure(Position const& pos, ::std::string const& reason);
        float GetPositionSuccessRate(Position const& pos, float radius);

        // Metrics accessors
        uint64 GetPositionsEvaluated() const { return _positionsEvaluated.load(); }

    private:
        ::std::unique_ptr<PositionManager> _manager;
        ::std::atomic<uint64> _positionsEvaluated{0};
    };

    // Module instances
    ::std::unique_ptr<ArbiterModule> _arbiter;
    ::std::unique_ptr<PathfindingModule> _pathfinding;
    ::std::unique_ptr<FormationModule> _formation;
    ::std::unique_ptr<PositionModule> _position;

    // Bot reference
    Player* _bot;

    // Global mutex for unified operations
    mutable Playerbot::OrderedMutex<Playerbot::LockOrder::MOVEMENT_ARBITER> _mutex;

    // Statistics
    ::std::atomic<uint64> _totalOperations{0};
    ::std::atomic<uint64> _totalProcessingTimeMs{0};
};

} // namespace Playerbot
