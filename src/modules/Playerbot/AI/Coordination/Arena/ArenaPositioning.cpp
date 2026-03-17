/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "ArenaPositioning.h"
#include "ArenaCoordinator.h"
#include "Player.h"
#include "ObjectAccessor.h"
#include "GameTime.h"
#include "Log.h"
#include <cmath>
#include <algorithm>

namespace Playerbot {

// ============================================================================
// ARENA MAP IDS
// ============================================================================

static constexpr uint32 ARENA_NAGRAND = 559;
static constexpr uint32 ARENA_BLADES_EDGE = 562;
static constexpr uint32 ARENA_DALARAN_SEWERS = 617;
static constexpr uint32 ARENA_RUINS_OF_LORDAERON = 572;
static constexpr uint32 ARENA_RING_OF_VALOR = 618;
static constexpr uint32 ARENA_TOL_VIRON = 980;
static constexpr uint32 ARENA_TIGERS_PEAK = 1134;
static constexpr uint32 ARENA_ASHAMANES_FALL = 1552;
static constexpr uint32 ARENA_BLACK_ROOK_ARENA = 1504;

// ============================================================================
// CONSTRUCTOR
// ============================================================================

ArenaPositioning::ArenaPositioning(ArenaCoordinator* coordinator)
    : _coordinator(coordinator)
{
}

void ArenaPositioning::Initialize(uint32 arenaMapId)
{
    Reset();
    _arenaMapId = arenaMapId;
    LoadArenaData(arenaMapId);

    TC_LOG_DEBUG("playerbot", "ArenaPositioning::Initialize - Initialized for arena map %u with %zu pillars",
                 arenaMapId, _pillars.size());
}

void ArenaPositioning::Update(uint32 /*diff*/)
{
    // Update positioning goals for all players
    for (const auto& teammate : _coordinator->GetAliveTeammates())
    {
        if (ShouldUpdateGoal(teammate.guid))
        {
            _playerGoals[teammate.guid] = DetermineGoal(teammate.guid);
        }
    }

    // Recalculate recommendations
    CalculateRecommendations();
}

void ArenaPositioning::Reset()
{
    _arenaMapId = 0;
    _pillars.clear();
    _arenaCenterX = 0;
    _arenaCenterY = 0;
    _arenaCenterZ = 0;
    _arenaRadius = 0;
    _playerGoals.clear();
    _currentRecommendations.clear();
}

// ============================================================================
// POSITION RECOMMENDATIONS
// ============================================================================

PositionRecommendation ArenaPositioning::GetRecommendedPosition(ObjectGuid player) const
{
    auto it = _currentRecommendations.find(player);
    if (it != _currentRecommendations.end())
        return it->second;

    return PositionRecommendation();
}

::std::vector<PositionRecommendation> ArenaPositioning::GetAllRecommendations() const
{
    ::std::vector<PositionRecommendation> recommendations;
    for (const auto& [player, rec] : _currentRecommendations)
    {
        recommendations.push_back(rec);
    }
    return recommendations;
}

void ArenaPositioning::RequestReposition(ObjectGuid player, float x, float y, float z)
{
    PositionRecommendation rec;
    rec.player = player;
    rec.x = x;
    rec.y = y;
    rec.z = z;
    rec.goal = PositioningGoal::NONE;
    rec.urgency = 0.5f;
    rec.reason = "Manual reposition request";

    _currentRecommendations[player] = rec;
}

PositioningGoal ArenaPositioning::GetCurrentGoal(ObjectGuid player) const
{
    auto it = _playerGoals.find(player);
    return it != _playerGoals.end() ? it->second : PositioningGoal::NONE;
}

// ============================================================================
// LINE OF SIGHT
// ============================================================================

bool ArenaPositioning::HasLOS(ObjectGuid from, ObjectGuid to) const
{
    float x1, y1, z1, x2, y2, z2;
    GetPlayerPosition(from, x1, y1, z1);
    GetPlayerPosition(to, x2, y2, z2);

    return CheckLOSThroughPillars(x1, y1, z1, x2, y2, z2);
}

bool ArenaPositioning::HasLOSToPosition(ObjectGuid from, float x, float y, float z) const
{
    float x1, y1, z1;
    GetPlayerPosition(from, x1, y1, z1);

    return CheckLOSThroughPillars(x1, y1, z1, x, y, z);
}

LOSAnalysis ArenaPositioning::AnalyzeLOS(ObjectGuid player) const
{
    float x, y, z;
    GetPlayerPosition(player, x, y, z);
    return AnalyzePositionLOS(x, y, z);
}

LOSAnalysis ArenaPositioning::AnalyzePositionLOS(float x, float y, float z) const
{
    LOSAnalysis analysis;
    analysis.x = x;
    analysis.y = y;
    analysis.z = z;

    // Check LOS to kill target
    ObjectGuid killTarget = _coordinator->GetKillTarget();
    if (!killTarget.IsEmpty())
    {
        float tx, ty, tz;
        const ArenaEnemy* enemy = _coordinator->GetEnemy(killTarget);
        if (enemy)
        {
            tx = enemy->lastKnownX;
            ty = enemy->lastKnownY;
            tz = enemy->lastKnownZ;
            analysis.hasLOSToKillTarget = CheckLOSThroughPillars(x, y, z, tx, ty, tz);
        }
    }

    // Check LOS to healer
    const ArenaTeammate* healer = _coordinator->GetTeamHealer();
    if (healer)
    {
        analysis.hasLOSToHealer = CheckLOSThroughPillars(x, y, z, healer->x, healer->y, healer->z);
    }

    // Check LOS to all enemies
    analysis.enemiesWithLOS = 0;
    for (const auto& enemy : _coordinator->GetAliveEnemies())
    {
        if (CheckLOSThroughPillars(x, y, z, enemy.lastKnownX, enemy.lastKnownY, enemy.lastKnownZ))
        {
            analysis.enemiesWithLOS++;
        }
    }
    analysis.isInLOSOfAllEnemies = (analysis.enemiesWithLOS == _coordinator->GetAliveEnemyCount());

    // Find nearest pillar
    analysis.nearestPillar = GetNearestPillar(x, y, z);
    if (analysis.nearestPillar)
    {
        analysis.nearestPillarDistance = CalculateDistance2D(
            x, y,
            analysis.nearestPillar->centerX, analysis.nearestPillar->centerY);
    }

    return analysis;
}

bool ArenaPositioning::IsInLOSOfHealer(ObjectGuid player) const
{
    const ArenaTeammate* healer = _coordinator->GetTeamHealer();
    if (!healer)
        return true;  // No healer, assume fine

    return HasLOS(player, healer->guid);
}

bool ArenaPositioning::IsInLOSOfKillTarget(ObjectGuid player) const
{
    ObjectGuid killTarget = _coordinator->GetKillTarget();
    if (killTarget.IsEmpty())
        return true;

    return HasLOS(player, killTarget);
}

// ============================================================================
// PILLAR PLAY
// ============================================================================

bool ArenaPositioning::ShouldLOS() const
{
    // LOS when team is losing / needs to reset
    float teamHealth = _coordinator->GetTeamHealthPercent();

    // LOS if team health is low
    if (teamHealth < 40.0f)
        return true;

    // LOS if healer is in trouble
    const ArenaTeammate* healer = _coordinator->GetTeamHealer();
    if (healer && healer->defensiveState >= DefensiveState::IN_DANGER)
        return true;

    return false;
}

bool ArenaPositioning::ShouldLOS(ObjectGuid player) const
{
    const ArenaTeammate* teammate = _coordinator->GetTeammate(player);
    if (!teammate)
        return false;

    // DPS should LOS when low health and waiting for heal
    if (teammate->role != ArenaRole::HEALER)
    {
        if (teammate->defensiveState >= DefensiveState::IN_DANGER)
            return true;
    }

    // Healer should LOS when being trained
    if (teammate->role == ArenaRole::HEALER && teammate->needsPeel)
        return true;

    return ShouldLOS();
}

const PillarInfo* ArenaPositioning::GetNearestPillar(ObjectGuid player) const
{
    float x, y, z;
    GetPlayerPosition(player, x, y, z);
    return GetNearestPillar(x, y, z);
}

const PillarInfo* ArenaPositioning::GetNearestPillar(float x, float y, float /*z*/) const
{
    const PillarInfo* nearest = nullptr;
    float minDistance = std::numeric_limits<float>::max();

    for (const auto& pillar : _pillars)
    {
        float dist = CalculateDistance2D(x, y, pillar.centerX, pillar.centerY);
        if (dist < minDistance)
        {
            minDistance = dist;
            nearest = &pillar;
        }
    }

    return nearest;
}

float ArenaPositioning::GetPillarDistance(ObjectGuid player) const
{
    const PillarInfo* pillar = GetNearestPillar(player);
    if (!pillar)
        return 100.0f;  // No pillars

    float x, y, z;
    GetPlayerPosition(player, x, y, z);

    return CalculateDistance2D(x, y, pillar->centerX, pillar->centerY) - pillar->radius;
}

void ArenaPositioning::GetLOSPosition(ObjectGuid player, float& x, float& y, float& z) const
{
    const PillarInfo* pillar = GetNearestPillar(player);
    if (!pillar)
    {
        GetPlayerPosition(player, x, y, z);
        return;
    }

    GetPillarSafeSpot(pillar, player, x, y, z);
}

void ArenaPositioning::GetPillarSafeSpot(const PillarInfo* pillar, ObjectGuid player, float& x, float& y, float& z) const
{
    if (!pillar)
        return;

    // Find the side of pillar opposite from most enemies
    float avgEnemyX = 0, avgEnemyY = 0;
    uint32 enemyCount = 0;

    for (const auto& enemy : _coordinator->GetAliveEnemies())
    {
        avgEnemyX += enemy.lastKnownX;
        avgEnemyY += enemy.lastKnownY;
        enemyCount++;
    }

    if (enemyCount > 0)
    {
        avgEnemyX /= enemyCount;
        avgEnemyY /= enemyCount;
    }

    // Get direction from enemies to pillar
    float dirX = pillar->centerX - avgEnemyX;
    float dirY = pillar->centerY - avgEnemyY;
    NormalizeDirection(dirX, dirY);

    // Position on far side of pillar
    x = pillar->centerX + dirX * (pillar->radius + 2.0f);
    y = pillar->centerY + dirY * (pillar->radius + 2.0f);
    z = pillar->centerZ;

    (void)player;
}

// ============================================================================
// SPREAD/STACK
// ============================================================================

bool ArenaPositioning::IsSpreadCorrectly(ObjectGuid player) const
{
    float minDistance = GetDistanceToNearestTeammate(player);
    return minDistance >= _spreadDistance;
}

void ArenaPositioning::GetSpreadPosition(ObjectGuid player, float& x, float& y, float& z) const
{
    GetPlayerPosition(player, x, y, z);

    // Find direction away from nearest teammate
    float nearestDist = std::numeric_limits<float>::max();
    float awayX = 0, awayY = 0;

    for (const auto& teammate : _coordinator->GetAliveTeammates())
    {
        if (teammate.guid == player)
            continue;

        float dist = CalculateDistance2D(x, y, teammate.x, teammate.y);
        if (dist < nearestDist && dist > 0)
        {
            nearestDist = dist;
            awayX = x - teammate.x;
            awayY = y - teammate.y;
        }
    }

    if (nearestDist < _spreadDistance && nearestDist > 0)
    {
        NormalizeDirection(awayX, awayY);
        float moveDistance = _spreadDistance - nearestDist;
        x += awayX * moveDistance;
        y += awayY * moveDistance;
    }
}

void ArenaPositioning::GetStackPosition(ObjectGuid player, ObjectGuid stackTarget, float& x, float& y, float& z) const
{
    (void)player;

    // Stack on target position
    const ArenaTeammate* target = _coordinator->GetTeammate(stackTarget);
    if (target)
    {
        x = target->x;
        y = target->y;
        z = target->z;
    }
}

// ============================================================================
// KITING
// ============================================================================

bool ArenaPositioning::ShouldKite(ObjectGuid player) const
{
    const ArenaTeammate* teammate = _coordinator->GetTeammate(player);
    if (!teammate)
        return false;

    // Ranged DPS/healers should kite melee
    if (teammate->role == ArenaRole::MELEE_DPS)
        return false;

    // Check if melee is on us
    float nearestMeleeDistance = GetDistanceToNearestEnemy(player);
    return nearestMeleeDistance < _meleeRange * 2;
}

void ArenaPositioning::GetKiteDirection(ObjectGuid player, float& dirX, float& dirY) const
{
    float x, y, z;
    GetPlayerPosition(player, x, y, z);

    // Kite away from nearest enemy
    float nearestDist = std::numeric_limits<float>::max();

    for (const auto& enemy : _coordinator->GetAliveEnemies())
    {
        float dist = CalculateDistance2D(x, y, enemy.lastKnownX, enemy.lastKnownY);
        if (dist < nearestDist)
        {
            nearestDist = dist;
            dirX = x - enemy.lastKnownX;
            dirY = y - enemy.lastKnownY;
        }
    }

    NormalizeDirection(dirX, dirY);

    // Try to kite toward pillar if available
    const PillarInfo* pillar = GetNearestPillar(player);
    if (pillar)
    {
        float pillarDirX = pillar->centerX - x;
        float pillarDirY = pillar->centerY - y;
        NormalizeDirection(pillarDirX, pillarDirY);

        // Blend kite direction with pillar direction
        dirX = dirX * 0.7f + pillarDirX * 0.3f;
        dirY = dirY * 0.7f + pillarDirY * 0.3f;
        NormalizeDirection(dirX, dirY);
    }
}

void ArenaPositioning::GetKitePosition(ObjectGuid player, float& x, float& y, float& z) const
{
    GetPlayerPosition(player, x, y, z);

    float dirX, dirY;
    GetKiteDirection(player, dirX, dirY);

    // Move in kite direction
    float kiteDistance = 10.0f;  // Kite 10 yards
    x += dirX * kiteDistance;
    y += dirY * kiteDistance;
}

float ArenaPositioning::GetKiteSpeed(ObjectGuid player) const
{
    // Would return movement speed multiplier
    (void)player;
    return 1.0f;
}

::std::vector<ObjectGuid> ArenaPositioning::GetChasingEnemies(ObjectGuid player) const
{
    ::std::vector<ObjectGuid> chasers;
    float x, y, z;
    GetPlayerPosition(player, x, y, z);

    for (const auto& enemy : _coordinator->GetAliveEnemies())
    {
        // Enemy is chasing if they're melee and close
        if (enemy.role == ArenaRole::MELEE_DPS)
        {
            float dist = CalculateDistance2D(x, y, enemy.lastKnownX, enemy.lastKnownY);
            if (dist < _meleeRange * 3)  // Within chase range
            {
                chasers.push_back(enemy.guid);
            }
        }
    }

    return chasers;
}

// ============================================================================
// CHASE/INTERCEPT
// ============================================================================

bool ArenaPositioning::ShouldChase(ObjectGuid player, ObjectGuid target) const
{
    const ArenaTeammate* teammate = _coordinator->GetTeammate(player);
    if (!teammate)
        return false;

    // Melee should chase fleeing targets
    if (teammate->role != ArenaRole::MELEE_DPS)
        return false;

    // Check if target is the kill target
    if (target == _coordinator->GetKillTarget())
        return true;

    (void)target;
    return false;
}

void ArenaPositioning::GetInterceptPosition(ObjectGuid chaser, ObjectGuid target, float& x, float& y, float& z) const
{
    (void)chaser;

    // Simplified - just move toward target
    const ArenaEnemy* enemy = _coordinator->GetEnemy(target);
    if (enemy)
    {
        x = enemy->lastKnownX;
        y = enemy->lastKnownY;
        z = enemy->lastKnownZ;
    }
}

float ArenaPositioning::GetTimeToIntercept(ObjectGuid chaser, ObjectGuid target) const
{
    float distance = GetDistanceBetween(chaser, target);
    // Assume 7 yards per second base speed
    return distance / 7.0f;
}

// ============================================================================
// DISTANCE QUERIES
// ============================================================================

float ArenaPositioning::GetDistanceToKillTarget(ObjectGuid player) const
{
    ObjectGuid killTarget = _coordinator->GetKillTarget();
    if (killTarget.IsEmpty())
        return 100.0f;

    return GetDistanceBetween(player, killTarget);
}

float ArenaPositioning::GetDistanceToHealer(ObjectGuid player) const
{
    const ArenaTeammate* healer = _coordinator->GetTeamHealer();
    if (!healer)
        return 0.0f;

    return GetDistanceBetween(player, healer->guid);
}

float ArenaPositioning::GetDistanceToNearestEnemy(ObjectGuid player) const
{
    float x, y, z;
    GetPlayerPosition(player, x, y, z);

    float minDistance = std::numeric_limits<float>::max();
    for (const auto& enemy : _coordinator->GetAliveEnemies())
    {
        float dist = CalculateDistance2D(x, y, enemy.lastKnownX, enemy.lastKnownY);
        if (dist < minDistance)
            minDistance = dist;
    }

    return minDistance;
}

float ArenaPositioning::GetDistanceToNearestTeammate(ObjectGuid player) const
{
    float x, y, z;
    GetPlayerPosition(player, x, y, z);

    float minDistance = std::numeric_limits<float>::max();
    for (const auto& teammate : _coordinator->GetAliveTeammates())
    {
        if (teammate.guid == player)
            continue;

        float dist = CalculateDistance2D(x, y, teammate.x, teammate.y);
        if (dist < minDistance)
            minDistance = dist;
    }

    return minDistance;
}

float ArenaPositioning::GetDistanceBetween(ObjectGuid a, ObjectGuid b) const
{
    float x1, y1, z1, x2, y2, z2;
    GetPlayerPosition(a, x1, y1, z1);

    // Check if b is teammate or enemy
    const ArenaTeammate* teammate = _coordinator->GetTeammate(b);
    if (teammate)
    {
        x2 = teammate->x;
        y2 = teammate->y;
        z2 = 0;  // z not tracked for teammates in simple struct
    }
    else
    {
        const ArenaEnemy* enemy = _coordinator->GetEnemy(b);
        if (enemy)
        {
            x2 = enemy->lastKnownX;
            y2 = enemy->lastKnownY;
            z2 = enemy->lastKnownZ;
        }
        else
        {
            return 100.0f;
        }
    }

    return CalculateDistance3D(x1, y1, z1, x2, y2, z2);
}

// ============================================================================
// ARENA MAP DATA
// ============================================================================

void ArenaPositioning::GetArenaCenter(float& x, float& y, float& z) const
{
    x = _arenaCenterX;
    y = _arenaCenterY;
    z = _arenaCenterZ;
}

// ============================================================================
// ARENA MAP LOADING (PRIVATE)
// ============================================================================

void ArenaPositioning::LoadArenaData(uint32 mapId)
{
    switch (mapId)
    {
        case ARENA_BLADES_EDGE:
            LoadBladesEdgePillars();
            break;
        case ARENA_NAGRAND:
            LoadNagrandPillars();
            break;
        case ARENA_DALARAN_SEWERS:
            LoadDalaranSewers();
            break;
        case ARENA_RUINS_OF_LORDAERON:
            LoadRuinsOfLordaeron();
            break;
        case ARENA_RING_OF_VALOR:
            LoadRingOfValor();
            break;
        case ARENA_TOL_VIRON:
            LoadTolViron();
            break;
        case ARENA_TIGERS_PEAK:
            LoadTigersPeak();
            break;
        case ARENA_ASHAMANES_FALL:
            LoadAshamanesFall();
            break;
        case ARENA_BLACK_ROOK_ARENA:
            LoadBlackRookArena();
            break;
        default:
            // Unknown arena - set defaults
            _arenaCenterX = 0;
            _arenaCenterY = 0;
            _arenaCenterZ = 0;
            _arenaRadius = 50.0f;
            break;
    }
}

void ArenaPositioning::LoadBladesEdgePillars()
{
    _arenaCenterX = 6238.0f;
    _arenaCenterY = 262.0f;
    _arenaCenterZ = 0.0f;
    _arenaRadius = 47.0f;

    // Bridge pillar (center)
    PillarInfo bridge;
    bridge.id = 1;
    bridge.centerX = 6238.0f;
    bridge.centerY = 262.0f;
    bridge.centerZ = 0.0f;
    bridge.radius = 4.0f;
    bridge.height = 10.0f;
    bridge.name = "Bridge Pillar";
    _pillars.push_back(bridge);
}

void ArenaPositioning::LoadNagrandPillars()
{
    _arenaCenterX = 4030.0f;
    _arenaCenterY = 2959.0f;
    _arenaCenterZ = 12.0f;
    _arenaRadius = 45.0f;

    // Four corner pillars
    PillarInfo pillar1;
    pillar1.id = 1;
    pillar1.centerX = 4011.0f;
    pillar1.centerY = 2977.0f;
    pillar1.centerZ = 12.0f;
    pillar1.radius = 3.0f;
    pillar1.height = 8.0f;
    pillar1.name = "NW Pillar";
    _pillars.push_back(pillar1);

    PillarInfo pillar2;
    pillar2.id = 2;
    pillar2.centerX = 4049.0f;
    pillar2.centerY = 2977.0f;
    pillar2.centerZ = 12.0f;
    pillar2.radius = 3.0f;
    pillar2.height = 8.0f;
    pillar2.name = "NE Pillar";
    _pillars.push_back(pillar2);

    PillarInfo pillar3;
    pillar3.id = 3;
    pillar3.centerX = 4011.0f;
    pillar3.centerY = 2941.0f;
    pillar3.centerZ = 12.0f;
    pillar3.radius = 3.0f;
    pillar3.height = 8.0f;
    pillar3.name = "SW Pillar";
    _pillars.push_back(pillar3);

    PillarInfo pillar4;
    pillar4.id = 4;
    pillar4.centerX = 4049.0f;
    pillar4.centerY = 2941.0f;
    pillar4.centerZ = 12.0f;
    pillar4.radius = 3.0f;
    pillar4.height = 8.0f;
    pillar4.name = "SE Pillar";
    _pillars.push_back(pillar4);
}

void ArenaPositioning::LoadDalaranSewers()
{
    _arenaCenterX = 1291.0f;
    _arenaCenterY = 790.0f;
    _arenaCenterZ = 9.0f;
    _arenaRadius = 45.0f;

    // Center pillar / boxes
    PillarInfo center;
    center.id = 1;
    center.centerX = 1291.0f;
    center.centerY = 790.0f;
    center.centerZ = 9.0f;
    center.radius = 5.0f;
    center.height = 5.0f;
    center.name = "Center Boxes";
    _pillars.push_back(center);
}

void ArenaPositioning::LoadRuinsOfLordaeron()
{
    _arenaCenterX = 1278.0f;
    _arenaCenterY = 1730.0f;
    _arenaCenterZ = 31.0f;
    _arenaRadius = 40.0f;

    // Center tomb
    PillarInfo tomb;
    tomb.id = 1;
    tomb.centerX = 1278.0f;
    tomb.centerY = 1730.0f;
    tomb.centerZ = 31.0f;
    tomb.radius = 6.0f;
    tomb.height = 4.0f;
    tomb.name = "Center Tomb";
    _pillars.push_back(tomb);
}

void ArenaPositioning::LoadRingOfValor()
{
    _arenaCenterX = 763.0f;
    _arenaCenterY = -294.0f;
    _arenaCenterZ = 28.0f;
    _arenaRadius = 45.0f;

    // Dynamic pillars (simplified as static)
    PillarInfo pillar1;
    pillar1.id = 1;
    pillar1.centerX = 763.0f;
    pillar1.centerY = -275.0f;
    pillar1.centerZ = 28.0f;
    pillar1.radius = 3.0f;
    pillar1.height = 10.0f;
    pillar1.name = "North Pillar";
    _pillars.push_back(pillar1);

    PillarInfo pillar2;
    pillar2.id = 2;
    pillar2.centerX = 763.0f;
    pillar2.centerY = -313.0f;
    pillar2.centerZ = 28.0f;
    pillar2.radius = 3.0f;
    pillar2.height = 10.0f;
    pillar2.name = "South Pillar";
    _pillars.push_back(pillar2);
}

void ArenaPositioning::LoadTolViron()
{
    _arenaCenterX = -10842.0f;
    _arenaCenterY = -3854.0f;
    _arenaCenterZ = 48.0f;
    _arenaRadius = 40.0f;
}

void ArenaPositioning::LoadTigersPeak()
{
    _arenaCenterX = 555.0f;
    _arenaCenterY = 734.0f;
    _arenaCenterZ = 358.0f;
    _arenaRadius = 40.0f;

    PillarInfo pillar;
    pillar.id = 1;
    pillar.centerX = 555.0f;
    pillar.centerY = 734.0f;
    pillar.centerZ = 358.0f;
    pillar.radius = 4.0f;
    pillar.height = 6.0f;
    pillar.name = "Center Rock";
    _pillars.push_back(pillar);
}

void ArenaPositioning::LoadAshamanesFall()
{
    _arenaCenterX = 3734.0f;
    _arenaCenterY = 5765.0f;
    _arenaCenterZ = 125.0f;
    _arenaRadius = 45.0f;
}

void ArenaPositioning::LoadBlackRookArena()
{
    _arenaCenterX = 3259.0f;
    _arenaCenterY = 7318.0f;
    _arenaCenterZ = 219.0f;
    _arenaRadius = 40.0f;
}

// ============================================================================
// POSITION CALCULATION (PRIVATE)
// ============================================================================

void ArenaPositioning::CalculateRecommendations()
{
    for (const auto& teammate : _coordinator->GetAliveTeammates())
    {
        PositioningGoal goal = GetCurrentGoal(teammate.guid);
        PositionRecommendation rec;

        switch (goal)
        {
            case PositioningGoal::ATTACK:
                rec = CalculateAttackPosition(teammate.guid);
                break;
            case PositioningGoal::DEFEND:
                rec = CalculateDefendPosition(teammate.guid);
                break;
            case PositioningGoal::LOS_PILLAR:
                rec = CalculateLOSPosition(teammate.guid);
                break;
            case PositioningGoal::KITE:
                rec = CalculateKitePosition(teammate.guid);
                break;
            case PositioningGoal::CHASE:
                rec = CalculateChasePosition(teammate.guid);
                break;
            default:
                GetPlayerPosition(teammate.guid, rec.x, rec.y, rec.z);
                rec.goal = PositioningGoal::NONE;
                rec.urgency = 0.0f;
                break;
        }

        rec.player = teammate.guid;
        _currentRecommendations[teammate.guid] = rec;
    }
}

PositionRecommendation ArenaPositioning::CalculateAttackPosition(ObjectGuid player) const
{
    PositionRecommendation rec;
    rec.player = player;
    rec.goal = PositioningGoal::ATTACK;

    ObjectGuid killTarget = _coordinator->GetKillTarget();
    const ArenaEnemy* target = _coordinator->GetEnemy(killTarget);

    if (target)
    {
        const ArenaTeammate* teammate = _coordinator->GetTeammate(player);
        float range = (teammate && teammate->role == ArenaRole::MELEE_DPS) ?
            _meleeRange : _rangedRange;

        // Position at appropriate range from target
        float x, y, z;
        GetPlayerPosition(player, x, y, z);

        float dirX = target->lastKnownX - x;
        float dirY = target->lastKnownY - y;
        float dist = CalculateDistance2D(x, y, target->lastKnownX, target->lastKnownY);

        if (dist > 0)
        {
            NormalizeDirection(dirX, dirY);

            if (dist > range)
            {
                // Move closer
                rec.x = target->lastKnownX - dirX * range;
                rec.y = target->lastKnownY - dirY * range;
                rec.urgency = (dist - range) / 30.0f;
            }
            else
            {
                // Already in range
                rec.x = x;
                rec.y = y;
                rec.urgency = 0.0f;
            }
        }

        rec.z = target->lastKnownZ;
        rec.reason = "Attack kill target";
    }

    return rec;
}

PositionRecommendation ArenaPositioning::CalculateDefendPosition(ObjectGuid player) const
{
    PositionRecommendation rec;
    rec.player = player;
    rec.goal = PositioningGoal::DEFEND;

    // Defend = stay near healer
    const ArenaTeammate* healer = _coordinator->GetTeamHealer();
    if (healer)
    {
        rec.x = healer->x;
        rec.y = healer->y;
        rec.z = healer->z;
        rec.urgency = 0.5f;
        rec.reason = "Stay near healer";
    }

    return rec;
}

PositionRecommendation ArenaPositioning::CalculateLOSPosition(ObjectGuid player) const
{
    PositionRecommendation rec;
    rec.player = player;
    rec.goal = PositioningGoal::LOS_PILLAR;

    GetLOSPosition(player, rec.x, rec.y, rec.z);
    rec.urgency = 0.8f;
    rec.reason = "LOS behind pillar";

    return rec;
}

PositionRecommendation ArenaPositioning::CalculateKitePosition(ObjectGuid player) const
{
    PositionRecommendation rec;
    rec.player = player;
    rec.goal = PositioningGoal::KITE;

    GetKitePosition(player, rec.x, rec.y, rec.z);
    rec.urgency = 0.7f;
    rec.reason = "Kite melee";

    return rec;
}

PositionRecommendation ArenaPositioning::CalculateChasePosition(ObjectGuid player) const
{
    PositionRecommendation rec;
    rec.player = player;
    rec.goal = PositioningGoal::CHASE;

    ObjectGuid killTarget = _coordinator->GetKillTarget();
    if (!killTarget.IsEmpty())
    {
        GetInterceptPosition(player, killTarget, rec.x, rec.y, rec.z);
        rec.urgency = 0.6f;
        rec.reason = "Chase kill target";
    }

    return rec;
}

// ============================================================================
// GOAL DETERMINATION (PRIVATE)
// ============================================================================

PositioningGoal ArenaPositioning::DetermineGoal(ObjectGuid player) const
{
    const ArenaTeammate* teammate = _coordinator->GetTeammate(player);
    if (!teammate)
        return PositioningGoal::NONE;

    // LOS if low health and no defensives
    if (ShouldLOS(player))
        return PositioningGoal::LOS_PILLAR;

    // Kite if being chased
    if (ShouldKite(player))
        return PositioningGoal::KITE;

    // Attack if DPS
    ObjectGuid killTarget = _coordinator->GetKillTarget();
    if (!killTarget.IsEmpty() && teammate->role != ArenaRole::HEALER)
    {
        if (teammate->role == ArenaRole::MELEE_DPS)
            return PositioningGoal::CHASE;
        return PositioningGoal::ATTACK;
    }

    // Defend healer if healer needs peel
    const ArenaTeammate* healer = _coordinator->GetTeamHealer();
    if (healer && healer->needsPeel && teammate->role != ArenaRole::HEALER)
        return PositioningGoal::DEFEND;

    return PositioningGoal::NONE;
}

bool ArenaPositioning::ShouldUpdateGoal(ObjectGuid player) const
{
    // Could add cooldown/hysteresis logic here
    (void)player;
    return true;
}

// ============================================================================
// LOS CALCULATIONS (PRIVATE)
// ============================================================================

bool ArenaPositioning::CheckLOSThroughPillars(float x1, float y1, float /*z1*/,
                                               float x2, float y2, float /*z2*/) const
{
    for (const auto& pillar : _pillars)
    {
        if (IsBlockedByPillar(pillar, x1, y1, x2, y2))
            return false;
    }

    return true;
}

bool ArenaPositioning::IsBlockedByPillar(const PillarInfo& pillar,
                                          float x1, float y1, float x2, float y2) const
{
    // Simple circle-line intersection test
    // Calculate closest point on line segment to pillar center

    float dx = x2 - x1;
    float dy = y2 - y1;
    float fx = x1 - pillar.centerX;
    float fy = y1 - pillar.centerY;

    float a = dx * dx + dy * dy;
    float b = 2.0f * (fx * dx + fy * dy);
    float c = fx * fx + fy * fy - pillar.radius * pillar.radius;

    float discriminant = b * b - 4.0f * a * c;

    if (discriminant < 0)
        return false;  // No intersection

    discriminant = std::sqrt(discriminant);

    float t1 = (-b - discriminant) / (2.0f * a);
    float t2 = (-b + discriminant) / (2.0f * a);

    // Check if intersection is within line segment [0, 1]
    if ((t1 >= 0 && t1 <= 1) || (t2 >= 0 && t2 <= 1))
        return true;

    return false;
}

// ============================================================================
// UTILITY (PRIVATE)
// ============================================================================

void ArenaPositioning::GetPlayerPosition(ObjectGuid player, float& x, float& y, float& z) const
{
    // Check teammates
    const ArenaTeammate* teammate = _coordinator->GetTeammate(player);
    if (teammate)
    {
        x = teammate->x;
        y = teammate->y;
        z = 0;  // Would need z tracking in ArenaTeammate
        return;
    }

    // Check enemies
    const ArenaEnemy* enemy = _coordinator->GetEnemy(player);
    if (enemy)
    {
        x = enemy->lastKnownX;
        y = enemy->lastKnownY;
        z = enemy->lastKnownZ;
        return;
    }

    // Fallback to actual player position
    Player* p = ObjectAccessor::FindPlayer(player);
    if (p)
    {
        x = p->GetPositionX();
        y = p->GetPositionY();
        z = p->GetPositionZ();
        return;
    }

    // Default
    x = 0;
    y = 0;
    z = 0;
}

float ArenaPositioning::CalculateDistance2D(float x1, float y1, float x2, float y2) const
{
    float dx = x2 - x1;
    float dy = y2 - y1;
    return std::sqrt(dx * dx + dy * dy);
}

float ArenaPositioning::CalculateDistance3D(float x1, float y1, float z1,
                                             float x2, float y2, float z2) const
{
    float dx = x2 - x1;
    float dy = y2 - y1;
    float dz = z2 - z1;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

void ArenaPositioning::NormalizeDirection(float& x, float& y) const
{
    float length = std::sqrt(x * x + y * y);
    if (length > 0)
    {
        x /= length;
        y /= length;
    }
}

} // namespace Playerbot
