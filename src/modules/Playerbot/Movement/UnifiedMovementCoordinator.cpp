/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "UnifiedMovementCoordinator.h"
#include "GameTime.h"
#include "../AI/Combat/BotThreatManager.h"
#include "Log.h"
#include "Timer.h"
#include <sstream>

namespace Playerbot
{

// ============================================================================
// MODULE CONSTRUCTORS
// ============================================================================

UnifiedMovementCoordinator::ArbiterModule::ArbiterModule(Player* bot)
    : _arbiter(::std::make_unique<MovementArbiter>(bot))
{
}

UnifiedMovementCoordinator::PathfindingModule::PathfindingModule()
    : _adapter(::std::make_unique<PathfindingAdapter>())
{
}

UnifiedMovementCoordinator::FormationModule::FormationModule(Player* bot)
    : _manager(::std::make_unique<FormationManager>(bot))
{
}

UnifiedMovementCoordinator::PositionModule::PositionModule(Player* bot, BotThreatManager* threatManager)
    : _manager(::std::make_unique<PositionManager>(bot, threatManager))
{
}

// ============================================================================
// UNIFIED COORDINATOR CONSTRUCTION
// ============================================================================

UnifiedMovementCoordinator::UnifiedMovementCoordinator(Player* bot)
    : _bot(bot)
{
    // Initialize all modules
    _arbiter = ::std::make_unique<ArbiterModule>(bot);
    _pathfinding = ::std::make_unique<PathfindingModule>();
    
    // FormationManager and PositionManager need bot reference
    _formation = ::std::make_unique<FormationModule>(bot);
    
    // PositionManager needs threat manager - get from bot
    BotThreatManager* threatMgr = nullptr; // Threat manager wired later when BotAI is fully initialized
    _position = ::std::make_unique<PositionModule>(bot, threatMgr);
    
    // CRITICAL: No logging with bot->GetName() in constructor
    // Player's m_name can be corrupted during concurrent access, causing ACCESS_VIOLATION
}

UnifiedMovementCoordinator::~UnifiedMovementCoordinator()
{
    // CRITICAL: No logging in destructor - allocations can throw
}

// ============================================================================
// ARBITER MODULE DELEGATION
// ============================================================================

// Module implementation - delegates to MovementArbiter
bool UnifiedMovementCoordinator::ArbiterModule::RequestMovement(MovementRequest const& request)
{
    _requestsProcessed++;
    return _arbiter->RequestMovement(request);
}

void UnifiedMovementCoordinator::ArbiterModule::ClearPendingRequests()
{
    _arbiter->ClearPendingRequests();
}

void UnifiedMovementCoordinator::ArbiterModule::StopMovement()
{
    _arbiter->StopMovement();
}

void UnifiedMovementCoordinator::ArbiterModule::Update(uint32 diff)
{
    _arbiter->Update(diff);
}

MovementArbiterStatistics const& UnifiedMovementCoordinator::ArbiterModule::GetStatistics() const
{
    return _arbiter->GetStatistics();
}

void UnifiedMovementCoordinator::ArbiterModule::ResetStatistics()
{
    _arbiter->ResetStatistics();
}

::std::string UnifiedMovementCoordinator::ArbiterModule::GetDiagnosticString() const
{
    return _arbiter->GetDiagnosticString();
}

void UnifiedMovementCoordinator::ArbiterModule::LogStatistics() const
{
    _arbiter->LogStatistics();
}

MovementArbiterConfig UnifiedMovementCoordinator::ArbiterModule::GetConfig() const
{
    return _arbiter->GetConfig();
}

void UnifiedMovementCoordinator::ArbiterModule::SetConfig(MovementArbiterConfig const& config)
{
    _arbiter->SetConfig(config);
}

void UnifiedMovementCoordinator::ArbiterModule::SetDiagnosticLogging(bool enable)
{
    _arbiter->SetDiagnosticLogging(enable);
}

uint32 UnifiedMovementCoordinator::ArbiterModule::GetPendingRequestCount() const
{
    return _arbiter->GetPendingRequestCount();
}

bool UnifiedMovementCoordinator::ArbiterModule::HasPendingRequests() const
{
    return _arbiter->HasPendingRequests();
}

// Coordinator interface - delegates to module
bool UnifiedMovementCoordinator::RequestMovement(MovementRequest const& request)
{
    return _arbiter->RequestMovement(request);
}

void UnifiedMovementCoordinator::ClearPendingRequests()
{
    _arbiter->ClearPendingRequests();
}

void UnifiedMovementCoordinator::StopMovement()
{
    _arbiter->StopMovement();
}

void UnifiedMovementCoordinator::Update(uint32 diff)
{
    _arbiter->Update(diff);
}

MovementArbiterStatistics const& UnifiedMovementCoordinator::GetArbiterStatistics() const
{
    return _arbiter->GetStatistics();
}

void UnifiedMovementCoordinator::ResetArbiterStatistics()
{
    _arbiter->ResetStatistics();
}

::std::string UnifiedMovementCoordinator::GetArbiterDiagnosticString() const
{
    return _arbiter->GetDiagnosticString();
}

void UnifiedMovementCoordinator::LogArbiterStatistics() const
{
    _arbiter->LogStatistics();
}

MovementArbiterConfig UnifiedMovementCoordinator::GetArbiterConfig() const
{
    return _arbiter->GetConfig();
}

void UnifiedMovementCoordinator::SetArbiterConfig(MovementArbiterConfig const& config)
{
    _arbiter->SetConfig(config);
}

void UnifiedMovementCoordinator::SetDiagnosticLogging(bool enable)
{
    _arbiter->SetDiagnosticLogging(enable);
}

uint32 UnifiedMovementCoordinator::GetPendingRequestCount() const
{
    return _arbiter->GetPendingRequestCount();
}

bool UnifiedMovementCoordinator::HasPendingRequests() const
{
    return _arbiter->HasPendingRequests();
}

// ============================================================================
// PATHFINDING MODULE DELEGATION (abbreviated - pattern continues)
// ============================================================================

bool UnifiedMovementCoordinator::PathfindingModule::Initialize(uint32 cacheSize, uint32 cacheDuration)
{
    return _adapter->Initialize(cacheSize, cacheDuration);
}

void UnifiedMovementCoordinator::PathfindingModule::Shutdown()
{
    _adapter->Shutdown();
}

bool UnifiedMovementCoordinator::PathfindingModule::CalculatePath(Player* bot, Position const& destination, MovementPath& path, bool forceDirect)
{
    _pathsCalculated++;
    return _adapter->CalculatePath(bot, destination, path, forceDirect);
}

// ... [Additional pathfinding delegation methods follow same pattern]

// Coordinator delegation
bool UnifiedMovementCoordinator::InitializePathfinding(uint32 cacheSize, uint32 cacheDuration)
{
    return _pathfinding->Initialize(cacheSize, cacheDuration);
}

void UnifiedMovementCoordinator::ShutdownPathfinding()
{
    _pathfinding->Shutdown();
}

bool UnifiedMovementCoordinator::CalculatePath(Player* bot, Position const& destination, MovementPath& path, bool forceDirect)
{
    return _pathfinding->CalculatePath(bot, destination, path, forceDirect);
}

// ... [Pattern continues for all pathfinding methods]

// ============================================================================
// FORMATION MODULE DELEGATION (abbreviated - pattern continues)
// ============================================================================

bool UnifiedMovementCoordinator::FormationModule::JoinFormation(::std::vector<Player*> const& groupMembers, MovementFormationType formation)
{
    _formationsExecuted++;
    return _manager->JoinFormation(groupMembers, formation);
}

// ... [Formation module delegation continues]

// Coordinator delegation
bool UnifiedMovementCoordinator::JoinFormation(::std::vector<Player*> const& groupMembers, MovementFormationType formation)
{
    return _formation->JoinFormation(groupMembers, formation);
}

// ... [Pattern continues for all formation methods]

// ============================================================================
// POSITION MODULE DELEGATION (abbreviated - pattern continues)
// ============================================================================

MovementResult UnifiedMovementCoordinator::PositionModule::UpdatePosition(MovementContext const& context)
{
    _positionsEvaluated++;
    PositionMovementResult result = _manager->UpdatePosition(context);
    return result.success ? MovementResult::MOVEMENT_SUCCESS : MovementResult::MOVEMENT_FAILED;
}

PositionMovementResult UnifiedMovementCoordinator::PositionModule::FindOptimalPosition(MovementContext const& context)
{
    _positionsEvaluated++;
    return _manager->FindOptimalPosition(context);
}

// ... [Position module delegation continues]

// Coordinator delegation
MovementResult UnifiedMovementCoordinator::UpdatePosition(MovementContext const& context)
{
    return _position->UpdatePosition(context);
}

// ... [Pattern continues for all position methods]

// ============================================================================
// UNIFIED OPERATIONS IMPLEMENTATION
// ============================================================================

void UnifiedMovementCoordinator::CoordinateCompleteMovement(Player* bot, MovementContext const& context)
{
    ::std::lock_guard<decltype(_mutex)> lock(_mutex);
    auto startTime = GameTime::GetGameTimeMS();
    _totalOperations++;

    // 1. Position evaluation (Position module)
    PositionMovementResult posResult = _position->FindOptimalPosition(context);

    if (!posResult.success)
    {
        TC_LOG_DEBUG("playerbot.movement", "Failed to find optimal position for bot {}", bot->GetName());
        return;
    }

    // 2. Path calculation (Pathfinding module)
    MovementPath path;
    if (!_pathfinding->CalculatePath(bot, posResult.targetPosition, path, false))
    {
        TC_LOG_DEBUG("playerbot.movement", "Failed to calculate path for bot {}", bot->GetName());
        return;
    }

    // 3. Formation adjustment (Formation module)
    if (_formation->IsInFormation())
    {
        Position adjustedPos = _formation->AdjustMovementForFormation(posResult.targetPosition);
        // Recalculate path if position was adjusted
    if (adjustedPos.GetExactDist(&posResult.targetPosition) > 2.0f)
        {
            _pathfinding->CalculatePath(bot, adjustedPos, path, false);
        }
    }

    // 4. Movement request arbitration (Arbiter module)
    MovementRequest req = MovementRequest::MakePointMovement(
        PlayerBotMovementPriority::TACTICAL_POSITIONING,
        posResult.targetPosition,
        true,
        {},
        {},
        {},
        "Coordinated movement",
        "UnifiedMovementCoordinator"
    );

    _arbiter->RequestMovement(req);

    auto endTime = GameTime::GetGameTimeMS();
    _totalProcessingTimeMs += (endTime - startTime);
}

::std::string UnifiedMovementCoordinator::GetMovementRecommendation(Player* bot, MovementContext const& context)
{
    ::std::ostringstream oss;

    // Position evaluation
    PositionMovementResult posResult = _position->FindOptimalPosition(context);

    // Path quality
    MovementPath path;
    bool hasPath = _pathfinding->CalculatePath(bot, posResult.targetPosition, path, false);
    
    // Formation impact
    bool inFormation = _formation->IsInFormation();
    FormationIntegrity integrity = FormationIntegrity::PERFECT;
    if (inFormation)
        integrity = _formation->AssessIntegrity();

    oss << "Movement Recommendation for " << bot->GetName() << ":\n";
    oss << "  Optimal Position: " << (posResult.success ? "Found" : "Not Found") << "\n";
    oss << "  Position Score: " << posResult.targetPosition.ToString() << "\n";
    oss << "  Path Available: " << (hasPath ? "Yes" : "No") << "\n";
    if (hasPath)
        oss << "  Path Distance: " << posResult.pathDistance << " yards\n";
    oss << "  In Formation: " << (inFormation ? "Yes" : "No") << "\n";
    if (inFormation)
        oss << "  Formation Integrity: " << static_cast<uint32>(integrity) << "\n";
    oss << "  Movement Priority: " << static_cast<uint32>(posResult.priority) << "\n";
    
    return oss.str();
}

void UnifiedMovementCoordinator::OptimizeBotMovement(Player* bot)
{
    ::std::lock_guard<decltype(_mutex)> lock(_mutex);

    // Clear stale movement requests
    if (_arbiter->GetPendingRequestCount() > 10)
        _arbiter->ClearPendingRequests();

    // Refresh formation if needed
    if (_formation->IsInFormation() && _formation->RequiresReformation())
        _formation->ChangeFormation(_formation->GetCurrentFormation());

    // Clear expired path cache
    _pathfinding->ClearCache(bot);

    // Update AoE zones
    _position->UpdateAoEZones(GameTime::GetGameTimeMS());
    _position->ClearExpiredZones(GameTime::GetGameTimeMS());
}

::std::string UnifiedMovementCoordinator::GetMovementStatistics() const
{
    ::std::ostringstream oss;
    oss << "=== Unified Movement Coordinator Statistics ===\n";
    oss << "Total Operations: " << _totalOperations.load() << "\n";
    oss << "Total Processing Time (ms): " << _totalProcessingTimeMs.load() << "\n";
    oss << "\n--- Arbiter Module ---\n";
    oss << "Requests Processed: " << _arbiter->GetRequestsProcessed() << "\n";
    oss << "Pending Requests: " << _arbiter->GetPendingRequestCount() << "\n";
    oss << "\n--- Pathfinding Module ---\n";
    oss << "Paths Calculated: " << _pathfinding->GetPathsCalculated() << "\n";
    uint32 hits, misses, evictions;
    _pathfinding->GetCacheStatistics(hits, misses, evictions);
    oss << "Cache Hits: " << hits << ", Misses: " << misses << ", Evictions: " << evictions << "\n";
    oss << "\n--- Formation Module ---\n";
    oss << "Formations Executed: " << _formation->GetFormationsExecuted() << "\n";
    oss << "In Formation: " << (_formation->IsInFormation() ? "Yes" : "No") << "\n";
    oss << "\n--- Position Module ---\n";
    oss << "Positions Evaluated: " << _position->GetPositionsEvaluated() << "\n";
    
    return oss.str();
}

// ============================================================================
// PATHFINDING MODULE - COMPLETE DELEGATION
// ============================================================================

bool UnifiedMovementCoordinator::PathfindingModule::CalculatePathToUnit(Player* bot, Unit* target, MovementPath& path, float range)
{
    _pathsCalculated++;
    return _adapter->CalculatePathToUnit(bot, target, path, range);
}

bool UnifiedMovementCoordinator::PathfindingModule::CalculateFormationPath(Player* bot, Unit* leader, Position const& offset, MovementPath& path)
{
    _pathsCalculated++;
    return _adapter->CalculateFormationPath(bot, leader, offset, path);
}

bool UnifiedMovementCoordinator::PathfindingModule::CalculateFleePath(Player* bot, Unit* threat, float distance, MovementPath& path)
{
    _pathsCalculated++;
    return _adapter->CalculateFleePath(bot, threat, distance, path);
}

bool UnifiedMovementCoordinator::PathfindingModule::HasCachedPath(Player* bot, Position const& destination) const
{
    return _adapter->HasCachedPath(bot, destination);
}

bool UnifiedMovementCoordinator::PathfindingModule::GetCachedPath(Player* bot, Position const& destination, MovementPath& path) const
{
    return _adapter->GetCachedPath(bot, destination, path);
}

void UnifiedMovementCoordinator::PathfindingModule::ClearCache(Player* bot)
{
    _adapter->ClearCache(bot);
}

void UnifiedMovementCoordinator::PathfindingModule::ClearAllCache()
{
    _adapter->ClearAllCache();
}

void UnifiedMovementCoordinator::PathfindingModule::SetPathParameters(uint32 maxNodes, float straightDistance, float maxSearchDistance)
{
    _adapter->SetPathParameters(maxNodes, straightDistance, maxSearchDistance);
}

void UnifiedMovementCoordinator::PathfindingModule::EnableSmoothing(bool enable)
{
    _adapter->EnablePathSmoothing(enable);
}

void UnifiedMovementCoordinator::PathfindingModule::EnableCaching(bool enable)
{
    _adapter->EnableCaching(enable);
}

void UnifiedMovementCoordinator::PathfindingModule::SetCacheParameters(uint32 maxSize, uint32 duration)
{
    _adapter->SetCacheParameters(maxSize, duration);
}

void UnifiedMovementCoordinator::PathfindingModule::GetCacheStatistics(uint32& hits, uint32& misses, uint32& evictions) const
{
    _adapter->GetCacheStatistics(hits, misses, evictions);
}

void UnifiedMovementCoordinator::PathfindingModule::GetPathStatistics(uint32& totalPaths, uint32& avgTime, uint32& maxTime) const
{
    _adapter->GetPathStatistics(totalPaths, avgTime, maxTime);
}

void UnifiedMovementCoordinator::PathfindingModule::ResetStatistics()
{
    _pathsCalculated = 0;
}

bool UnifiedMovementCoordinator::PathfindingModule::IsWalkablePosition(Map* map, Position const& position) const
{
    return _adapter->IsWalkablePosition(map, position);
}

bool UnifiedMovementCoordinator::PathfindingModule::GetNearestWalkablePosition(Map* map, Position const& position, Position& walkable, float searchRange) const
{
    return _adapter->GetNearestWalkablePosition(map, position, walkable, searchRange);
}

// Coordinator pathfinding delegation
bool UnifiedMovementCoordinator::CalculatePathToUnit(Player* bot, Unit* target, MovementPath& path, float range)
{
    return _pathfinding->CalculatePathToUnit(bot, target, path, range);
}

bool UnifiedMovementCoordinator::CalculateFormationPath(Player* bot, Unit* leader, Position const& offset, MovementPath& path)
{
    return _pathfinding->CalculateFormationPath(bot, leader, offset, path);
}

bool UnifiedMovementCoordinator::CalculateFleePath(Player* bot, Unit* threat, float distance, MovementPath& path)
{
    return _pathfinding->CalculateFleePath(bot, threat, distance, path);
}

bool UnifiedMovementCoordinator::HasCachedPath(Player* bot, Position const& destination) const
{
    return _pathfinding->HasCachedPath(bot, destination);
}

bool UnifiedMovementCoordinator::GetCachedPath(Player* bot, Position const& destination, MovementPath& path) const
{
    return _pathfinding->GetCachedPath(bot, destination, path);
}

void UnifiedMovementCoordinator::ClearPathCache(Player* bot)
{
    _pathfinding->ClearCache(bot);
}

void UnifiedMovementCoordinator::ClearAllPathCache()
{
    _pathfinding->ClearAllCache();
}

void UnifiedMovementCoordinator::SetPathParameters(uint32 maxNodes, float straightDistance, float maxSearchDistance)
{
    _pathfinding->SetPathParameters(maxNodes, straightDistance, maxSearchDistance);
}

void UnifiedMovementCoordinator::EnablePathSmoothing(bool enable)
{
    _pathfinding->EnableSmoothing(enable);
}

void UnifiedMovementCoordinator::EnablePathCaching(bool enable)
{
    _pathfinding->EnableCaching(enable);
}

void UnifiedMovementCoordinator::SetCacheParameters(uint32 maxSize, uint32 duration)
{
    _pathfinding->SetCacheParameters(maxSize, duration);
}

void UnifiedMovementCoordinator::GetCacheStatistics(uint32& hits, uint32& misses, uint32& evictions) const
{
    _pathfinding->GetCacheStatistics(hits, misses, evictions);
}

void UnifiedMovementCoordinator::GetPathStatistics(uint32& totalPaths, uint32& avgTime, uint32& maxTime) const
{
    _pathfinding->GetPathStatistics(totalPaths, avgTime, maxTime);
}

void UnifiedMovementCoordinator::ResetPathStatistics()
{
    _pathfinding->ResetStatistics();
}

bool UnifiedMovementCoordinator::IsWalkablePosition(Map* map, Position const& position) const
{
    return _pathfinding->IsWalkablePosition(map, position);
}

bool UnifiedMovementCoordinator::GetNearestWalkablePosition(Map* map, Position const& position, Position& walkable, float searchRange) const
{
    return _pathfinding->GetNearestWalkablePosition(map, position, walkable, searchRange);
}

// ============================================================================
// FORMATION MODULE - COMPLETE DELEGATION
// ============================================================================

bool UnifiedMovementCoordinator::FormationModule::LeaveFormation()
{
    return _manager->LeaveFormation();
}

bool UnifiedMovementCoordinator::FormationModule::ChangeFormation(MovementFormationType newFormation)
{
    return _manager->ChangeFormation(newFormation);
}

bool UnifiedMovementCoordinator::FormationModule::SetFormationLeader(Player* leader)
{
    return _manager->SetFormationLeader(leader);
}

Player* UnifiedMovementCoordinator::FormationModule::GetFormationLeader() const
{
    return _manager->GetFormationLeader();
}

void UnifiedMovementCoordinator::FormationModule::UpdateFormation(uint32 diff)
{
    _manager->UpdateFormation(diff);
}

bool UnifiedMovementCoordinator::FormationModule::ExecuteFormationCommand(FormationCommand const& command)
{
    return _manager->ExecuteFormationCommand(command);
}

bool UnifiedMovementCoordinator::FormationModule::MoveFormationToPosition(Position const& targetPos, float orientation)
{
    return _manager->MoveFormationToPosition(targetPos, orientation);
}

bool UnifiedMovementCoordinator::FormationModule::AdjustFormationForCombat(::std::vector<Unit*> const& threats)
{
    return _manager->AdjustFormationForCombat(threats);
}

bool UnifiedMovementCoordinator::FormationModule::AddMember(Player* player, FormationRole role)
{
    // INTEGRATION REQUIRED: Formation member management
    // Implementation depends on:
    // - FormationManager::AddMember() method
    // - Member tracking data structure
    // - Role validation logic
    // See: FormationManager.h for required interface
    return false;
}

bool UnifiedMovementCoordinator::FormationModule::RemoveMember(Player* player)
{
    // INTEGRATION REQUIRED: Formation member removal
    // Implementation depends on:
    // - FormationManager::RemoveMember() method
    // - Member list update logic
    // - Position reallocation for remaining members
    // See: FormationManager.h for required interface
    return false;
}

bool UnifiedMovementCoordinator::FormationModule::ChangeMemberRole(Player* player, FormationRole newRole)
{
    // INTEGRATION REQUIRED: Dynamic role reassignment
    // Implementation depends on:
    // - FormationManager::ChangeMemberRole() method
    // - Role validation for formation type
    // - Position recalculation based on new role
    // See: FormationManager.h for required interface
    return false;
}

FormationMember* UnifiedMovementCoordinator::FormationModule::GetMember(Player* player)
{
    // INTEGRATION REQUIRED: Member lookup functionality
    // Implementation depends on:
    // - FormationManager::GetMember() method
    // - Member tracking data structure (map or vector)
    // - FormationMember structure definition
    // See: FormationManager.h for required interface
    return nullptr;
}

::std::vector<FormationMember> UnifiedMovementCoordinator::FormationModule::GetAllMembers() const
{
    // INTEGRATION REQUIRED: Bulk member retrieval
    // Implementation depends on:
    // - FormationManager::GetAllMembers() method
    // - Member storage container access
    // - FormationMember copy or reference return strategy
    // See: FormationManager.h for required interface
    return {};
}

Position UnifiedMovementCoordinator::FormationModule::CalculateFormationPosition(FormationRole role, uint32 memberIndex)
{
    return _manager->CalculateFormationPosition(role, memberIndex);
}

::std::vector<Position> UnifiedMovementCoordinator::FormationModule::CalculateAllFormationPositions()
{
    return _manager->CalculateAllFormationPositions();
}

Position UnifiedMovementCoordinator::FormationModule::GetAssignedPosition() const
{
    // INTEGRATION REQUIRED: Individual bot position query
    // Implementation depends on:
    // - FormationManager::GetAssignedPosition() method
    // - Per-bot position cache/storage
    // - Current formation state tracking
    // See: FormationManager.h for required interface
    return Position();
}

bool UnifiedMovementCoordinator::FormationModule::IsInFormationPosition(float tolerance) const
{
    return _manager->IsInFormationPosition(tolerance);
}

FormationIntegrity UnifiedMovementCoordinator::FormationModule::AssessIntegrity()
{
    return _manager->AssessFormationIntegrity();
}

float UnifiedMovementCoordinator::FormationModule::CalculateCohesion()
{
    return _manager->CalculateCohesionLevel();
}

::std::vector<Player*> UnifiedMovementCoordinator::FormationModule::GetOutOfPositionMembers(float tolerance)
{
    return _manager->GetOutOfPositionMembers(tolerance);
}

bool UnifiedMovementCoordinator::FormationModule::RequiresReformation()
{
    return _manager->RequiresReformation();
}

void UnifiedMovementCoordinator::FormationModule::CoordinateMovement(Position const& destination)
{
    // INTEGRATION REQUIRED: Formation-wide movement coordination
    // Implementation depends on:
    // - FormationManager::CoordinateMovement() method
    // - Leader movement detection and follower synchronization
    // - Path calculation for each member maintaining formation
    // - Movement speed synchronization across all members
    // See: FormationManager.h for required interface
    (void)destination;
}

void UnifiedMovementCoordinator::FormationModule::MaintainFormationDuringMovement()
{
    _manager->MaintainFormationDuringMovement();
}

bool UnifiedMovementCoordinator::FormationModule::CanMoveWithoutBreaking(Position const& newPos)
{
    return _manager->CanMoveWithoutBreakingFormation(newPos);
}

Position UnifiedMovementCoordinator::FormationModule::AdjustMovementForFormation(Position const& intendedPos)
{
    return _manager->AdjustMovementForFormation(intendedPos);
}

void UnifiedMovementCoordinator::FormationModule::TransitionToCombatFormation(::std::vector<Unit*> const& enemies)
{
    _manager->TransitionToCombatFormation(enemies);
}

void UnifiedMovementCoordinator::FormationModule::TransitionToTravelFormation()
{
    _manager->TransitionToTravelFormation();
}

void UnifiedMovementCoordinator::FormationModule::AdjustForThreatSpread(::std::vector<Unit*> const& threats)
{
    _manager->AdjustForThreatSpread(threats);
}

void UnifiedMovementCoordinator::FormationModule::HandleBreakage()
{
    _manager->HandleFormationBreakage();
}

MovementFormationType UnifiedMovementCoordinator::FormationModule::DetermineOptimalFormation(::std::vector<Player*> const& members)
{
    return _manager->DetermineOptimalFormation(members);
}

FormationConfig UnifiedMovementCoordinator::FormationModule::GetConfig(MovementFormationType formation)
{
    return _manager->GetFormationConfig(formation);
}

void UnifiedMovementCoordinator::FormationModule::SetConfig(MovementFormationType formation, FormationConfig const& config)
{
    _manager->SetFormationConfig(formation, config);
}

void UnifiedMovementCoordinator::FormationModule::AdjustForTerrain()
{
    // ENHANCEMENT: Terrain-aware formation adaptation
    // Future implementation should:
    // - Detect narrow passages, slopes, and elevation changes
    // - Automatically switch to single-file or compact formations in tight spaces
    // - Maintain cohesion on uneven terrain
    // - Restore original formation when terrain permits
    // See: FormationManager.h for integration point
}

void UnifiedMovementCoordinator::FormationModule::AdjustForObstacles(::std::vector<Position> const& obstacles)
{
    // ENHANCEMENT: Obstacle-aware formation deformation
    // Future implementation should:
    // - Detect static and dynamic obstacles in formation path
    // - Calculate obstacle-free positions for each member
    // - Temporarily deform formation to flow around obstacles
    // - Reform once obstacles are cleared
    // See: FormationManager.h for integration point
    (void)obstacles;
}

void UnifiedMovementCoordinator::FormationModule::AdjustForGroupSize()
{
    // ENHANCEMENT: Dynamic formation scaling
    // Future implementation should:
    // - Calculate optimal spacing based on current member count
    // - Adjust formation width/depth for small vs large groups
    // - Switch to appropriate formation type for group size (5-man vs raid)
    // - Maintain proportional member distribution
    // See: FormationManager.h for integration point
}

void UnifiedMovementCoordinator::FormationModule::HandleMemberDisconnection(Player* disconnectedMember)
{
    _manager->HandleMemberDisconnection(disconnectedMember);
}

MovementFormationType UnifiedMovementCoordinator::FormationModule::GetCurrentFormation() const
{
    return _manager->GetCurrentFormation();
}

FormationMovementState UnifiedMovementCoordinator::FormationModule::GetMovementState() const
{
    return _manager->GetMovementState();
}

bool UnifiedMovementCoordinator::FormationModule::IsLeader() const
{
    return _manager->IsFormationLeader();
}

bool UnifiedMovementCoordinator::FormationModule::IsInFormation() const
{
    return _manager->IsInFormation();
}

uint32 UnifiedMovementCoordinator::FormationModule::GetMemberCount() const
{
    // INTEGRATION REQUIRED: Member count query
    // Implementation depends on:
    // - FormationManager::GetMemberCount() method
    // - Active member tracking (exclude disconnected/dead)
    // - Thread-safe access to member container size
    // See: FormationManager.h for required interface
    return 0;
}

void UnifiedMovementCoordinator::FormationModule::SetUpdateInterval(uint32 intervalMs)
{
    _manager->SetUpdateInterval(intervalMs);
}

uint32 UnifiedMovementCoordinator::FormationModule::GetUpdateInterval() const
{
    return _manager->GetUpdateInterval();
}

void UnifiedMovementCoordinator::FormationModule::SetCohesionRadius(float radius)
{
    _manager->SetCohesionRadius(radius);
}

float UnifiedMovementCoordinator::FormationModule::GetCohesionRadius() const
{
    return _manager->GetCohesionRadius();
}

void UnifiedMovementCoordinator::FormationModule::SetFormationSpacing(float spacing)
{
    _manager->SetFormationSpacing(spacing);
}

float UnifiedMovementCoordinator::FormationModule::GetFormationSpacing() const
{
    return _manager->GetFormationSpacing();
}

FormationMetrics const& UnifiedMovementCoordinator::FormationModule::GetMetrics() const
{
    // INTEGRATION REQUIRED: Formation performance metrics
    // Implementation depends on:
    // - FormationManager::GetMetrics() method
    // - FormationMetrics structure with tracking data
    // - Metrics: cohesion violations, reformation count, average position deviation
    // See: FormationManager.h for required interface
    static FormationMetrics dummyMetrics{};
    return dummyMetrics;
}

void UnifiedMovementCoordinator::FormationModule::ResetMetrics()
{
    // INTEGRATION REQUIRED: Metrics reset functionality
    // Implementation depends on:
    // - FormationManager::ResetMetrics() method
    // - Zero out all tracking counters and statistics
    // - Preserve configuration while clearing runtime data
    // See: FormationManager.h for required interface
}

void UnifiedMovementCoordinator::FormationModule::EnableAdaptive(bool enable)
{
    _manager->EnableAdaptiveFormations(enable);
}

bool UnifiedMovementCoordinator::FormationModule::IsAdaptiveEnabled() const
{
    return _manager->IsAdaptiveFormationsEnabled();
}

void UnifiedMovementCoordinator::FormationModule::SetPriority(uint32 priority)
{
    _manager->SetFormationPriority(priority);
}

uint32 UnifiedMovementCoordinator::FormationModule::GetPriority() const
{
    return _manager->GetFormationPriority();
}

void UnifiedMovementCoordinator::FormationModule::ActivateEmergencyScatter()
{
    _manager->ActivateEmergencyScatter();
}

void UnifiedMovementCoordinator::FormationModule::DeactivateEmergencyScatter()
{
    _manager->DeactivateEmergencyScatter();
}

bool UnifiedMovementCoordinator::FormationModule::IsEmergencyScatterActive() const
{
    return _manager->IsEmergencyScatterActive();
}

void UnifiedMovementCoordinator::FormationModule::HandleEmergencyRegroup(Position const& rallyPoint)
{
    _manager->HandleEmergencyRegroup(rallyPoint);
}

// Coordinator formation delegation
bool UnifiedMovementCoordinator::LeaveFormation()
{
    return _formation->LeaveFormation();
}

bool UnifiedMovementCoordinator::ChangeFormation(MovementFormationType newFormation)
{
    return _formation->ChangeFormation(newFormation);
}

bool UnifiedMovementCoordinator::SetFormationLeader(Player* leader)
{
    return _formation->SetFormationLeader(leader);
}

Player* UnifiedMovementCoordinator::GetFormationLeader() const
{
    return _formation->GetFormationLeader();
}

void UnifiedMovementCoordinator::UpdateFormation(uint32 diff)
{
    _formation->UpdateFormation(diff);
}

bool UnifiedMovementCoordinator::ExecuteFormationCommand(FormationCommand const& command)
{
    return _formation->ExecuteFormationCommand(command);
}

bool UnifiedMovementCoordinator::MoveFormationToPosition(Position const& targetPos, float orientation)
{
    return _formation->MoveFormationToPosition(targetPos, orientation);
}

bool UnifiedMovementCoordinator::AdjustFormationForCombat(::std::vector<Unit*> const& threats)
{
    return _formation->AdjustFormationForCombat(threats);
}

bool UnifiedMovementCoordinator::AddFormationMember(Player* player, FormationRole role)
{
    return _formation->AddMember(player, role);
}

bool UnifiedMovementCoordinator::RemoveFormationMember(Player* player)
{
    return _formation->RemoveMember(player);
}

bool UnifiedMovementCoordinator::ChangeFormationMemberRole(Player* player, FormationRole newRole)
{
    return _formation->ChangeMemberRole(player, newRole);
}

FormationMember* UnifiedMovementCoordinator::GetFormationMember(Player* player)
{
    return _formation->GetMember(player);
}

::std::vector<FormationMember> UnifiedMovementCoordinator::GetAllFormationMembers() const
{
    return _formation->GetAllMembers();
}

Position UnifiedMovementCoordinator::CalculateFormationPosition(FormationRole role, uint32 memberIndex)
{
    return _formation->CalculateFormationPosition(role, memberIndex);
}

::std::vector<Position> UnifiedMovementCoordinator::CalculateAllFormationPositions()
{
    return _formation->CalculateAllFormationPositions();
}

Position UnifiedMovementCoordinator::GetAssignedFormationPosition() const
{
    return _formation->GetAssignedPosition();
}

bool UnifiedMovementCoordinator::IsInFormationPosition(float tolerance) const
{
    return _formation->IsInFormationPosition(tolerance);
}

FormationIntegrity UnifiedMovementCoordinator::AssessFormationIntegrity()
{
    return _formation->AssessIntegrity();
}

float UnifiedMovementCoordinator::CalculateCohesionLevel()
{
    return _formation->CalculateCohesion();
}

::std::vector<Player*> UnifiedMovementCoordinator::GetOutOfPositionMembers(float tolerance)
{
    return _formation->GetOutOfPositionMembers(tolerance);
}

bool UnifiedMovementCoordinator::RequiresReformation()
{
    return _formation->RequiresReformation();
}

void UnifiedMovementCoordinator::CoordinateFormationMovement(Position const& destination)
{
    _formation->CoordinateMovement(destination);
}

void UnifiedMovementCoordinator::MaintainFormationDuringMovement()
{
    _formation->MaintainFormationDuringMovement();
}

bool UnifiedMovementCoordinator::CanMoveWithoutBreakingFormation(Position const& newPos)
{
    return _formation->CanMoveWithoutBreaking(newPos);
}

Position UnifiedMovementCoordinator::AdjustMovementForFormation(Position const& intendedPos)
{
    return _formation->AdjustMovementForFormation(intendedPos);
}

void UnifiedMovementCoordinator::TransitionToCombatFormation(::std::vector<Unit*> const& enemies)
{
    _formation->TransitionToCombatFormation(enemies);
}

void UnifiedMovementCoordinator::TransitionToTravelFormation()
{
    _formation->TransitionToTravelFormation();
}

void UnifiedMovementCoordinator::AdjustForThreatSpread(::std::vector<Unit*> const& threats)
{
    _formation->AdjustForThreatSpread(threats);
}

void UnifiedMovementCoordinator::HandleFormationBreakage()
{
    _formation->HandleBreakage();
}

MovementFormationType UnifiedMovementCoordinator::DetermineOptimalFormation(::std::vector<Player*> const& members)
{
    return _formation->DetermineOptimalFormation(members);
}

FormationConfig UnifiedMovementCoordinator::GetFormationConfig(MovementFormationType formation)
{
    return _formation->GetConfig(formation);
}

void UnifiedMovementCoordinator::SetFormationConfig(MovementFormationType formation, FormationConfig const& config)
{
    _formation->SetConfig(formation, config);
}

void UnifiedMovementCoordinator::AdjustFormationForTerrain()
{
    _formation->AdjustForTerrain();
}

void UnifiedMovementCoordinator::AdjustFormationForObstacles(::std::vector<Position> const& obstacles)
{
    _formation->AdjustForObstacles(obstacles);
}

void UnifiedMovementCoordinator::AdjustFormationForGroupSize()
{
    _formation->AdjustForGroupSize();
}

void UnifiedMovementCoordinator::HandleMemberDisconnection(Player* disconnectedMember)
{
    _formation->HandleMemberDisconnection(disconnectedMember);
}

MovementFormationType UnifiedMovementCoordinator::GetCurrentFormation() const
{
    return _formation->GetCurrentFormation();
}

FormationMovementState UnifiedMovementCoordinator::GetFormationMovementState() const
{
    return _formation->GetMovementState();
}

bool UnifiedMovementCoordinator::IsFormationLeader() const
{
    return _formation->IsLeader();
}

bool UnifiedMovementCoordinator::IsInFormation() const
{
    return _formation->IsInFormation();
}

uint32 UnifiedMovementCoordinator::GetFormationMemberCount() const
{
    return _formation->GetMemberCount();
}

void UnifiedMovementCoordinator::SetFormationUpdateInterval(uint32 intervalMs)
{
    _formation->SetUpdateInterval(intervalMs);
}

uint32 UnifiedMovementCoordinator::GetFormationUpdateInterval() const
{
    return _formation->GetUpdateInterval();
}

void UnifiedMovementCoordinator::SetCohesionRadius(float radius)
{
    _formation->SetCohesionRadius(radius);
}

float UnifiedMovementCoordinator::GetCohesionRadius() const
{
    return _formation->GetCohesionRadius();
}

void UnifiedMovementCoordinator::SetFormationSpacing(float spacing)
{
    _formation->SetFormationSpacing(spacing);
}

float UnifiedMovementCoordinator::GetFormationSpacing() const
{
    return _formation->GetFormationSpacing();
}

FormationMetrics const& UnifiedMovementCoordinator::GetFormationMetrics() const
{
    return _formation->GetMetrics();
}

void UnifiedMovementCoordinator::ResetFormationMetrics()
{
    _formation->ResetMetrics();
}

void UnifiedMovementCoordinator::EnableAdaptiveFormations(bool enable)
{
    _formation->EnableAdaptive(enable);
}

bool UnifiedMovementCoordinator::IsAdaptiveFormationsEnabled() const
{
    return _formation->IsAdaptiveEnabled();
}

void UnifiedMovementCoordinator::SetFormationPriority(uint32 priority)
{
    _formation->SetPriority(priority);
}

uint32 UnifiedMovementCoordinator::GetFormationPriority() const
{
    return _formation->GetPriority();
}

void UnifiedMovementCoordinator::ActivateEmergencyScatter()
{
    _formation->ActivateEmergencyScatter();
}

void UnifiedMovementCoordinator::DeactivateEmergencyScatter()
{
    _formation->DeactivateEmergencyScatter();
}

bool UnifiedMovementCoordinator::IsEmergencyScatterActive() const
{
    return _formation->IsEmergencyScatterActive();
}

void UnifiedMovementCoordinator::HandleEmergencyRegroup(Position const& rallyPoint)
{
    _formation->HandleEmergencyRegroup(rallyPoint);
}

// ============================================================================
// POSITION MODULE - COMPLETE DELEGATION
// ============================================================================

MovementResult UnifiedMovementCoordinator::PositionModule::ExecuteMovement(Position const& targetPos, MovementPriority priority)
{
    PositionMovementResult result = _manager->ExecuteMovement(targetPos, priority);
    return result.success ? MovementResult::MOVEMENT_SUCCESS : MovementResult::MOVEMENT_FAILED;
}

PositionInfo UnifiedMovementCoordinator::PositionModule::EvaluatePosition(Position const& pos, MovementContext const& context)
{
    _positionsEvaluated++;
    return _manager->EvaluatePosition(pos, context);
}

::std::vector<PositionInfo> UnifiedMovementCoordinator::PositionModule::EvaluatePositions(::std::vector<Position> const& positions, MovementContext const& context)
{
    _positionsEvaluated += positions.size();
    return _manager->EvaluatePositions(positions, context);
}

::std::vector<Position> UnifiedMovementCoordinator::PositionModule::GenerateCandidatePositions(MovementContext const& context)
{
    return _manager->GenerateCandidatePositions(context);
}

Position UnifiedMovementCoordinator::PositionModule::FindRangePosition(Unit* target, float minRange, float maxRange, float preferredAngle)
{
    return _manager->FindRangePosition(target, minRange, maxRange, preferredAngle);
}

Position UnifiedMovementCoordinator::PositionModule::FindMeleePosition(Unit* target, bool preferBehind)
{
    return _manager->FindMeleePosition(target, preferBehind);
}

Position UnifiedMovementCoordinator::PositionModule::FindRangedPosition(Unit* target, float preferredRange)
{
    return _manager->FindRangedPosition(target, preferredRange);
}

Position UnifiedMovementCoordinator::PositionModule::FindHealingPosition(::std::vector<Player*> const& allies)
{
    return _manager->FindHealingPosition(allies);
}

Position UnifiedMovementCoordinator::PositionModule::FindKitingPosition(Unit* threat, float minDistance)
{
    return _manager->FindKitingPosition(threat, minDistance);
}

Position UnifiedMovementCoordinator::PositionModule::FindTankPosition(Unit* target)
{
    return _manager->FindTankPosition(target);
}

Position UnifiedMovementCoordinator::PositionModule::FindDpsPosition(Unit* target, PositionType type)
{
    return _manager->FindDpsPosition(target, type);
}

Position UnifiedMovementCoordinator::PositionModule::FindHealerPosition(::std::vector<Player*> const& groupMembers)
{
    return _manager->FindHealerPosition(groupMembers);
}

Position UnifiedMovementCoordinator::PositionModule::FindSupportPosition(::std::vector<Player*> const& groupMembers)
{
    return _manager->FindSupportPosition(groupMembers);
}

bool UnifiedMovementCoordinator::PositionModule::IsPositionSafe(Position const& pos, MovementContext const& context)
{
    return _manager->IsPositionSafe(pos, context);
}

bool UnifiedMovementCoordinator::PositionModule::IsInDangerZone(Position const& pos)
{
    return _manager->IsInDangerZone(pos);
}

Position UnifiedMovementCoordinator::PositionModule::FindSafePosition(Position const& fromPos, float minDistance)
{
    return _manager->FindSafePosition(fromPos, minDistance);
}

Position UnifiedMovementCoordinator::PositionModule::FindEscapePosition(::std::vector<Unit*> const& threats)
{
    return _manager->FindEscapePosition(threats);
}

void UnifiedMovementCoordinator::PositionModule::RegisterAoEZone(AoEZone const& zone)
{
    _manager->RegisterAoEZone(zone);
}

void UnifiedMovementCoordinator::PositionModule::UpdateAoEZones(uint32 currentTime)
{
    _manager->UpdateAoEZones(currentTime);
}

void UnifiedMovementCoordinator::PositionModule::ClearExpiredZones(uint32 currentTime)
{
    _manager->ClearExpiredZones(currentTime);
}

::std::vector<AoEZone> UnifiedMovementCoordinator::PositionModule::GetActiveZones() const
{
    return _manager->GetActiveZones();
}

bool UnifiedMovementCoordinator::PositionModule::ValidatePosition(Position const& pos, PositionValidation flags)
{
    return _manager->ValidatePosition(pos, flags);
}

bool UnifiedMovementCoordinator::PositionModule::HasLineOfSight(Position const& from, Position const& to)
{
    return _manager->HasLineOfSight(from, to);
}

bool UnifiedMovementCoordinator::PositionModule::IsWalkable(Position const& pos)
{
    return _manager->IsWalkablePosition(pos);
}

float UnifiedMovementCoordinator::PositionModule::CalculateMovementCost(Position const& from, Position const& to)
{
    return _manager->CalculateMovementCost(from, to);
}

Position UnifiedMovementCoordinator::PositionModule::FindFormationPosition(::std::vector<Player*> const& groupMembers, PositionType formationType)
{
    return _manager->FindFormationPosition(groupMembers, formationType);
}

bool UnifiedMovementCoordinator::PositionModule::ShouldMaintainGroupProximity()
{
    return _manager->ShouldMaintainGroupProximity();
}

float UnifiedMovementCoordinator::PositionModule::GetOptimalGroupDistance(uint8 role)
{
    // Convert uint8 role to ThreatRole enum for PositionManager
    ThreatRole threatRole = static_cast<ThreatRole>(role);
    return _manager->GetOptimalGroupDistance(threatRole);
}

bool UnifiedMovementCoordinator::PositionModule::ShouldStrafe(Unit* target)
{
    return _manager->ShouldStrafe(target);
}

bool UnifiedMovementCoordinator::PositionModule::ShouldCircleStrafe(Unit* target)
{
    return _manager->ShouldCircleStrafe(target);
}

Position UnifiedMovementCoordinator::PositionModule::CalculateStrafePosition(Unit* target, bool strafeLeft)
{
    return _manager->CalculateStrafePosition(target, strafeLeft);
}

Position UnifiedMovementCoordinator::PositionModule::PredictTargetPosition(Unit* target, float timeAhead)
{
    return _manager->PredictTargetPosition(target, timeAhead);
}

void UnifiedMovementCoordinator::PositionModule::SetWeights(PositionWeights const& weights)
{
    _manager->SetWeights(weights);
}

PositionWeights const& UnifiedMovementCoordinator::PositionModule::GetWeights() const
{
    return _manager->GetWeights();
}

void UnifiedMovementCoordinator::PositionModule::SetUpdateInterval(uint32 intervalMs)
{
    _manager->SetUpdateInterval(intervalMs);
}

uint32 UnifiedMovementCoordinator::PositionModule::GetUpdateInterval() const
{
    return _manager->GetUpdateInterval();
}

PositionMetrics const& UnifiedMovementCoordinator::PositionModule::GetMetrics() const
{
    return _manager->GetMetrics();
}

void UnifiedMovementCoordinator::PositionModule::ResetMetrics()
{
    _manager->ResetMetrics();
}

MovementResult UnifiedMovementCoordinator::PositionModule::HandleEmergencyMovement(MovementContext const& context)
{
    PositionMovementResult result = _manager->HandleEmergencyMovement(context);
    return result.success ? MovementResult::MOVEMENT_SUCCESS : MovementResult::MOVEMENT_FAILED;
}

bool UnifiedMovementCoordinator::PositionModule::IsInEmergencyPosition()
{
    return _manager->IsInEmergencyPosition();
}

Position UnifiedMovementCoordinator::PositionModule::FindEmergencyEscapePosition()
{
    return _manager->FindEmergencyEscapePosition();
}

void UnifiedMovementCoordinator::PositionModule::RecordPositionSuccess(Position const& pos, PositionType type)
{
    _manager->RecordPositionSuccess(pos, type);
}

void UnifiedMovementCoordinator::PositionModule::RecordPositionFailure(Position const& pos, ::std::string const& reason)
{
    _manager->RecordPositionFailure(pos, reason);
}

float UnifiedMovementCoordinator::PositionModule::GetPositionSuccessRate(Position const& pos, float radius)
{
    return _manager->GetPositionSuccessRate(pos, radius);
}

// Coordinator position delegation
MovementResult UnifiedMovementCoordinator::FindOptimalPosition(MovementContext const& context)
{
    PositionMovementResult result = _position->FindOptimalPosition(context);
    return result.success ? MovementResult::MOVEMENT_SUCCESS : MovementResult::MOVEMENT_FAILED;
}

MovementResult UnifiedMovementCoordinator::ExecuteMovement(Position const& targetPos, MovementPriority priority)
{
    return _position->ExecuteMovement(targetPos, priority);
}

PositionInfo UnifiedMovementCoordinator::EvaluatePosition(Position const& pos, MovementContext const& context)
{
    return _position->EvaluatePosition(pos, context);
}

::std::vector<PositionInfo> UnifiedMovementCoordinator::EvaluatePositions(::std::vector<Position> const& positions, MovementContext const& context)
{
    return _position->EvaluatePositions(positions, context);
}

::std::vector<Position> UnifiedMovementCoordinator::GenerateCandidatePositions(MovementContext const& context)
{
    return _position->GenerateCandidatePositions(context);
}

Position UnifiedMovementCoordinator::FindRangePosition(Unit* target, float minRange, float maxRange, float preferredAngle)
{
    return _position->FindRangePosition(target, minRange, maxRange, preferredAngle);
}

Position UnifiedMovementCoordinator::FindMeleePosition(Unit* target, bool preferBehind)
{
    return _position->FindMeleePosition(target, preferBehind);
}

Position UnifiedMovementCoordinator::FindRangedPosition(Unit* target, float preferredRange)
{
    return _position->FindRangedPosition(target, preferredRange);
}

Position UnifiedMovementCoordinator::FindHealingPosition(::std::vector<Player*> const& allies)
{
    return _position->FindHealingPosition(allies);
}

Position UnifiedMovementCoordinator::FindKitingPosition(Unit* threat, float minDistance)
{
    return _position->FindKitingPosition(threat, minDistance);
}

Position UnifiedMovementCoordinator::FindTankPosition(Unit* target)
{
    return _position->FindTankPosition(target);
}

Position UnifiedMovementCoordinator::FindDpsPosition(Unit* target, PositionType type)
{
    return _position->FindDpsPosition(target, type);
}

Position UnifiedMovementCoordinator::FindHealerPosition(::std::vector<Player*> const& groupMembers)
{
    return _position->FindHealerPosition(groupMembers);
}

Position UnifiedMovementCoordinator::FindSupportPosition(::std::vector<Player*> const& groupMembers)
{
    return _position->FindSupportPosition(groupMembers);
}

bool UnifiedMovementCoordinator::IsPositionSafe(Position const& pos, MovementContext const& context)
{
    return _position->IsPositionSafe(pos, context);
}

bool UnifiedMovementCoordinator::IsInDangerZone(Position const& pos)
{
    return _position->IsInDangerZone(pos);
}

Position UnifiedMovementCoordinator::FindSafePosition(Position const& fromPos, float minDistance)
{
    return _position->FindSafePosition(fromPos, minDistance);
}

Position UnifiedMovementCoordinator::FindEscapePosition(::std::vector<Unit*> const& threats)
{
    return _position->FindEscapePosition(threats);
}

void UnifiedMovementCoordinator::RegisterAoEZone(AoEZone const& zone)
{
    _position->RegisterAoEZone(zone);
}

void UnifiedMovementCoordinator::UpdateAoEZones(uint32 currentTime)
{
    _position->UpdateAoEZones(currentTime);
}

void UnifiedMovementCoordinator::ClearExpiredZones(uint32 currentTime)
{
    _position->ClearExpiredZones(currentTime);
}

::std::vector<AoEZone> UnifiedMovementCoordinator::GetActiveZones() const
{
    return _position->GetActiveZones();
}

bool UnifiedMovementCoordinator::ValidatePosition(Position const& pos, PositionValidation flags)
{
    return _position->ValidatePosition(pos, flags);
}

bool UnifiedMovementCoordinator::HasLineOfSight(Position const& from, Position const& to)
{
    return _position->HasLineOfSight(from, to);
}

bool UnifiedMovementCoordinator::IsWalkable(Position const& pos)
{
    return _position->IsWalkable(pos);
}

float UnifiedMovementCoordinator::CalculateMovementCost(Position const& from, Position const& to)
{
    return _position->CalculateMovementCost(from, to);
}

Position UnifiedMovementCoordinator::FindFormationPositionForRole(::std::vector<Player*> const& groupMembers, PositionType formationType)
{
    return _position->FindFormationPosition(groupMembers, formationType);
}

bool UnifiedMovementCoordinator::ShouldMaintainGroupProximity()
{
    return _position->ShouldMaintainGroupProximity();
}

float UnifiedMovementCoordinator::GetOptimalGroupDistance(uint8 role)
{
    return _position->GetOptimalGroupDistance(role);
}

bool UnifiedMovementCoordinator::ShouldStrafe(Unit* target)
{
    return _position->ShouldStrafe(target);
}

bool UnifiedMovementCoordinator::ShouldCircleStrafe(Unit* target)
{
    return _position->ShouldCircleStrafe(target);
}

Position UnifiedMovementCoordinator::CalculateStrafePosition(Unit* target, bool strafeLeft)
{
    return _position->CalculateStrafePosition(target, strafeLeft);
}

Position UnifiedMovementCoordinator::PredictTargetPosition(Unit* target, float timeAhead)
{
    return _position->PredictTargetPosition(target, timeAhead);
}

void UnifiedMovementCoordinator::SetPositionWeights(PositionWeights const& weights)
{
    _position->SetWeights(weights);
}

PositionWeights const& UnifiedMovementCoordinator::GetPositionWeights() const
{
    return _position->GetWeights();
}

void UnifiedMovementCoordinator::SetPositionUpdateInterval(uint32 intervalMs)
{
    _position->SetUpdateInterval(intervalMs);
}

uint32 UnifiedMovementCoordinator::GetPositionUpdateInterval() const
{
    return _position->GetUpdateInterval();
}

PositionMetrics const& UnifiedMovementCoordinator::GetPositionMetrics() const
{
    return _position->GetMetrics();
}

void UnifiedMovementCoordinator::ResetPositionMetrics()
{
    _position->ResetMetrics();
}

MovementResult UnifiedMovementCoordinator::HandleEmergencyMovement(MovementContext const& context)
{
    return _position->HandleEmergencyMovement(context);
}

bool UnifiedMovementCoordinator::IsInEmergencyPosition()
{
    return _position->IsInEmergencyPosition();
}

Position UnifiedMovementCoordinator::FindEmergencyEscapePosition()
{
    return _position->FindEmergencyEscapePosition();
}

void UnifiedMovementCoordinator::RecordPositionSuccess(Position const& pos, PositionType type)
{
    _position->RecordPositionSuccess(pos, type);
}

void UnifiedMovementCoordinator::RecordPositionFailure(Position const& pos, ::std::string const& reason)
{
    _position->RecordPositionFailure(pos, reason);
}

float UnifiedMovementCoordinator::GetPositionSuccessRate(Position const& pos, float radius)
{
    return _position->GetPositionSuccessRate(pos, radius);
}

} // namespace Playerbot
