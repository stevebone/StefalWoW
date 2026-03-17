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

#include "ArenaState.h"
#include <vector>
#include <map>

namespace Playerbot {

class ArenaCoordinator;

/**
 * @enum PositioningGoal
 * @brief Represents the current positioning objective
 */
enum class PositioningGoal : uint8
{
    NONE = 0,
    ATTACK = 1,         // Move toward kill target
    DEFEND = 2,         // Stay near teammate
    LOS_PILLAR = 3,     // Get behind pillar
    SPREAD = 4,         // Spread from teammates
    STACK = 5,          // Stack with teammates
    KITE = 6,           // Kite enemy melee
    CHASE = 7,          // Chase fleeing enemy
    RESET = 8           // Run to reset position
};

/**
 * @struct PillarInfo
 * @brief Information about a pillar/LOS obstacle in the arena
 */
struct PillarInfo
{
    uint32 id;
    float centerX;
    float centerY;
    float centerZ;
    float radius;
    float height;
    ::std::string name;

    PillarInfo()
        : id(0), centerX(0), centerY(0), centerZ(0),
          radius(0), height(0) {}
};

/**
 * @struct PositionRecommendation
 * @brief A recommended position for a player
 */
struct PositionRecommendation
{
    ObjectGuid player;
    float x, y, z;
    PositioningGoal goal;
    float urgency;      // 0-1, how urgent this repositioning is
    ::std::string reason;

    PositionRecommendation()
        : player(), x(0), y(0), z(0),
          goal(PositioningGoal::NONE), urgency(0) {}
};

/**
 * @struct LOSAnalysis
 * @brief Analysis of line of sight from a position
 */
struct LOSAnalysis
{
    float x, y, z;
    bool hasLOSToKillTarget;
    bool hasLOSToHealer;
    bool hasLOSToAllTeammates;
    bool isInLOSOfAllEnemies;
    uint8 enemiesWithLOS;
    float nearestPillarDistance;
    const PillarInfo* nearestPillar;

    LOSAnalysis()
        : x(0), y(0), z(0),
          hasLOSToKillTarget(true), hasLOSToHealer(true),
          hasLOSToAllTeammates(true), isInLOSOfAllEnemies(true),
          enemiesWithLOS(0), nearestPillarDistance(0), nearestPillar(nullptr) {}
};

/**
 * @class ArenaPositioning
 * @brief Manages arena positioning including pillar play and LOS
 *
 * Handles positioning strategy in arena including:
 * - Pillar awareness and LOS management
 * - Spread/stack positioning
 * - Kiting paths
 * - Chase interception
 * - Position optimization
 */
class ArenaPositioning
{
public:
    ArenaPositioning(ArenaCoordinator* coordinator);

    void Initialize(uint32 arenaMapId);
    void Update(uint32 diff);
    void Reset();

    // ========================================================================
    // POSITION RECOMMENDATIONS
    // ========================================================================

    PositionRecommendation GetRecommendedPosition(ObjectGuid player) const;
    ::std::vector<PositionRecommendation> GetAllRecommendations() const;
    void RequestReposition(ObjectGuid player, float x, float y, float z);
    PositioningGoal GetCurrentGoal(ObjectGuid player) const;

    // ========================================================================
    // LINE OF SIGHT
    // ========================================================================

    bool HasLOS(ObjectGuid from, ObjectGuid to) const;
    bool HasLOSToPosition(ObjectGuid from, float x, float y, float z) const;
    LOSAnalysis AnalyzeLOS(ObjectGuid player) const;
    LOSAnalysis AnalyzePositionLOS(float x, float y, float z) const;
    bool IsInLOSOfHealer(ObjectGuid player) const;
    bool IsInLOSOfKillTarget(ObjectGuid player) const;

    // ========================================================================
    // PILLAR PLAY
    // ========================================================================

    bool ShouldLOS() const;
    bool ShouldLOS(ObjectGuid player) const;
    const PillarInfo* GetNearestPillar(ObjectGuid player) const;
    const PillarInfo* GetNearestPillar(float x, float y, float z) const;
    float GetPillarDistance(ObjectGuid player) const;
    void GetLOSPosition(ObjectGuid player, float& x, float& y, float& z) const;
    void GetPillarSafeSpot(const PillarInfo* pillar, ObjectGuid player, float& x, float& y, float& z) const;

    // ========================================================================
    // SPREAD/STACK
    // ========================================================================

    float GetSpreadDistance() const { return _spreadDistance; }
    void SetSpreadDistance(float distance) { _spreadDistance = distance; }
    bool IsSpreadCorrectly(ObjectGuid player) const;
    void GetSpreadPosition(ObjectGuid player, float& x, float& y, float& z) const;
    void GetStackPosition(ObjectGuid player, ObjectGuid stackTarget, float& x, float& y, float& z) const;

    // ========================================================================
    // KITING
    // ========================================================================

    bool ShouldKite(ObjectGuid player) const;
    void GetKiteDirection(ObjectGuid player, float& dirX, float& dirY) const;
    void GetKitePosition(ObjectGuid player, float& x, float& y, float& z) const;
    float GetKiteSpeed(ObjectGuid player) const;
    ::std::vector<ObjectGuid> GetChasingEnemies(ObjectGuid player) const;

    // ========================================================================
    // CHASE/INTERCEPT
    // ========================================================================

    bool ShouldChase(ObjectGuid player, ObjectGuid target) const;
    void GetInterceptPosition(ObjectGuid chaser, ObjectGuid target, float& x, float& y, float& z) const;
    float GetTimeToIntercept(ObjectGuid chaser, ObjectGuid target) const;

    // ========================================================================
    // DISTANCE QUERIES
    // ========================================================================

    float GetDistanceToKillTarget(ObjectGuid player) const;
    float GetDistanceToHealer(ObjectGuid player) const;
    float GetDistanceToNearestEnemy(ObjectGuid player) const;
    float GetDistanceToNearestTeammate(ObjectGuid player) const;
    float GetDistanceBetween(ObjectGuid a, ObjectGuid b) const;

    // ========================================================================
    // ARENA MAP DATA
    // ========================================================================

    uint32 GetArenaMapId() const { return _arenaMapId; }
    const ::std::vector<PillarInfo>& GetPillars() const { return _pillars; }
    void GetArenaCenter(float& x, float& y, float& z) const;
    float GetArenaRadius() const { return _arenaRadius; }

private:
    ArenaCoordinator* _coordinator;

    // ========================================================================
    // ARENA DATA
    // ========================================================================

    uint32 _arenaMapId = 0;
    ::std::vector<PillarInfo> _pillars;
    float _arenaCenterX = 0;
    float _arenaCenterY = 0;
    float _arenaCenterZ = 0;
    float _arenaRadius = 0;

    // ========================================================================
    // PLAYER GOALS
    // ========================================================================

    ::std::map<ObjectGuid, PositioningGoal> _playerGoals;
    ::std::map<ObjectGuid, PositionRecommendation> _currentRecommendations;

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    float _spreadDistance = 8.0f;
    float _meleeRange = 5.0f;
    float _rangedRange = 30.0f;
    float _healerMaxRange = 40.0f;
    float _losThreshold = 0.5f;  // How often to recommend LOS

    // ========================================================================
    // ARENA MAP LOADING
    // ========================================================================

    void LoadArenaData(uint32 mapId);
    void LoadBladesEdgePillars();
    void LoadNagrandPillars();
    void LoadDalaranSewers();
    void LoadRuinsOfLordaeron();
    void LoadRingOfValor();
    void LoadTolViron();
    void LoadTigersPeak();
    void LoadAshamanesFall();
    void LoadBlackRookArena();

    // ========================================================================
    // POSITION CALCULATION
    // ========================================================================

    void CalculateRecommendations();
    PositionRecommendation CalculateAttackPosition(ObjectGuid player) const;
    PositionRecommendation CalculateDefendPosition(ObjectGuid player) const;
    PositionRecommendation CalculateLOSPosition(ObjectGuid player) const;
    PositionRecommendation CalculateKitePosition(ObjectGuid player) const;
    PositionRecommendation CalculateChasePosition(ObjectGuid player) const;

    // ========================================================================
    // GOAL DETERMINATION
    // ========================================================================

    PositioningGoal DetermineGoal(ObjectGuid player) const;
    bool ShouldUpdateGoal(ObjectGuid player) const;

    // ========================================================================
    // LOS CALCULATIONS
    // ========================================================================

    bool CheckLOSThroughPillars(float x1, float y1, float z1, float x2, float y2, float z2) const;
    bool IsBlockedByPillar(const PillarInfo& pillar, float x1, float y1, float x2, float y2) const;

    // ========================================================================
    // UTILITY
    // ========================================================================

    void GetPlayerPosition(ObjectGuid player, float& x, float& y, float& z) const;
    float CalculateDistance2D(float x1, float y1, float x2, float y2) const;
    float CalculateDistance3D(float x1, float y1, float z1, float x2, float y2, float z2) const;
    void NormalizeDirection(float& x, float& y) const;
};

} // namespace Playerbot
