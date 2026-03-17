/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

// Combat/ThreatManager.h removed - not used in this file
#include "MechanicAwareness.h"
#include "Player.h"
#include "Unit.h"
#include "UnitAI.h"
#include "Spell.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "SpellAuras.h"
#include "AreaTrigger.h"
#include "DBCEnums.h"
#include "Map.h"
#include "MotionMaster.h"
#include "Log.h"
#include "World.h"
#include "AI/BotAI.h"
#include "Core/PlayerBotHelpers.h"
#include "Movement/UnifiedMovementCoordinator.h"
#include "Movement/Arbiter/MovementRequest.h"
#include "Movement/Arbiter/MovementPriorityMapper.h"
#include <algorithm>
#include <execution>
#include <cmath>
#include "GameTime.h"

namespace Playerbot
{

// AOEZone implementation
bool AOEZone::IsPointInZone(const Position& point, uint32 currentTime) const
{
    if (!IsActive(currentTime))
        return false;

    float currentRadius = GetCurrentRadius(currentTime);
    float distance = center.GetExactDist(point);

    if (angle >= 360.0f)
    {
        // Circular AOE
        return distance <= currentRadius;
    }
    else
    {
        // Cone AOE
    if (distance > currentRadius)
            return false;

        // Check angle
        float pointAngle = ::std::atan2(point.m_positionY - center.m_positionY,
                                      point.m_positionX - center.m_positionX);
        float angleDiff = ::std::abs(Position::NormalizeOrientation(pointAngle - orientation));

        return angleDiff <= (angle * static_cast<float>(M_PI) / 360.0f);  // Convert to radians and check half angle
    }
}

float AOEZone::GetCurrentRadius(uint32 currentTime) const
{
    if (!isGrowing)
        return radius;

    uint32 elapsed = currentTime > startTime ? currentTime - startTime : 0;
    return radius + (growthRate * elapsed / 1000.0f);
}

bool AOEZone::IsActive(uint32 currentTime) const
{
    if (!isPersistent && currentTime > startTime + duration)
        return false;

    return currentTime >= startTime;
}

float AOEZone::EstimateDamage(uint32 timeInZone) const
{
    if (tickInterval == 0)
        return damagePerTick;  // One-time damage

    uint32 ticks = timeInZone / tickInterval;
    return damagePerTick * ticks;
}

// ProjectileInfo implementation
Position ProjectileInfo::PredictPosition(uint32 atTime) const
{
    if (atTime <= launchTime)
        return origin;

    if (atTime >= impactTime)
        return destination;

    float progress = float(atTime - launchTime) / float(impactTime - launchTime);
    Position predicted;

    // Linear interpolation for simple projectiles
    predicted.m_positionX = origin.m_positionX + (destination.m_positionX - origin.m_positionX) * progress;
    predicted.m_positionY = origin.m_positionY + (destination.m_positionY - origin.m_positionY) * progress;
    predicted.m_positionZ = origin.m_positionZ + (destination.m_positionZ - origin.m_positionZ) * progress;

    return predicted;
}

bool ProjectileInfo::WillHitPosition(const Position& pos, float tolerance) const
{
    // Check if position is near projectile path
    float distToOrigin = pos.GetExactDist(origin);
    float distToDest = pos.GetExactDist(destination);
    float pathLength = origin.GetExactDist(destination);

    // Simple check: if position is near the line between origin and destination
    if (distToOrigin + distToDest <= pathLength + tolerance)
    {
        // Calculate perpendicular distance to path
        float A = destination.m_positionY - origin.m_positionY;
        float B = origin.m_positionX - destination.m_positionX;
        float C = destination.m_positionX * origin.m_positionY - origin.m_positionX * destination.m_positionY;

        float perpDistance = ::std::abs(A * pos.m_positionX + B * pos.m_positionY + C) /
                            ::std::sqrt(A * A + B * B);

        return perpDistance <= radius + tolerance;
    }

    return false;
}

uint32 ProjectileInfo::TimeToImpact(uint32 currentTime) const
{
    if (currentTime >= impactTime)
        return 0;

    return impactTime - currentTime;
}

// CleaveMechanic implementation
bool CleaveMechanic::IsPositionSafe(const Position& pos) const
{
    if (!source || !isActive)
        return true;

    float distance = source->GetExactDist(&pos);
    if (distance > range)
        return true;

    // Check angle
    float targetAngle = source->GetRelativeAngle(&pos);
    float sourceface = source->GetOrientation();
    float angleDiff = ::std::abs(Position::NormalizeOrientation(targetAngle - sourceface));

    return angleDiff > (angle / 2.0f * static_cast<float>(M_PI) / 180.0f);
}

float CleaveMechanic::GetSafeAngle(bool preferLeft) const
{
    if (!source)
        return 0.0f;

    float safeAngle = angle / 2.0f * static_cast<float>(M_PI) / 180.0f + 0.1f;  // Add small buffer
    float baseAngle = source->GetOrientation();
    return Position::NormalizeOrientation(baseAngle + (preferLeft ? -safeAngle : safeAngle));
}

// MechanicAwareness implementation
MechanicAwareness::MechanicAwareness()
{
    _metrics.lastUpdate = ::std::chrono::steady_clock::now();
}

::std::vector<MechanicInfo> MechanicAwareness::DetectMechanics(Player* bot, Unit* target)
{
    ::std::vector<MechanicInfo> detectedMechanics;

    if (!bot)
        return detectedMechanics;

    // Scan for various mechanics
    ::std::vector<MechanicInfo> threats = ScanForThreats(bot);
    detectedMechanics.insert(detectedMechanics.end(), threats.begin(), threats.end());

    // Check casting mechanics
    if (target && target->IsAlive())
    {
        MechanicInfo castMechanic = DetectCastingMechanic(target);
        if (castMechanic.type != MechanicType::NONE)
            detectedMechanics.push_back(castMechanic);
    }
    // Check debuff mechanics
    MechanicInfo debuffMechanic = DetectDebuffMechanic(bot);
    if (debuffMechanic.type != MechanicType::NONE)
        detectedMechanics.push_back(debuffMechanic);

    // Update metrics
    _metrics.mechanicsDetected += detectedMechanics.size();

    return detectedMechanics;
}

MechanicInfo MechanicAwareness::AnalyzeSpellMechanic(uint32 spellId, Unit* caster, Unit* target)
{
    MechanicInfo mechanic;
    mechanic.spellId = spellId;

    if (caster)
    {
        mechanic.sourceGuid = caster->GetGUID();
        mechanic.sourcePosition = caster->GetPosition();
    }

    if (target)
        mechanic.targetGuid = target->GetGUID();
    // Get spell info
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return mechanic;

    // Determine mechanic type based on spell effects
    mechanic.type = GetSpellMechanicType(spellId);

    // Set urgency based on cast time
    uint32 castTime = spellInfo->CalcCastTime();
    if (castTime == 0)
        mechanic.urgency = MechanicUrgency::IMMEDIATE;
    else if (castTime < 1000)
        mechanic.urgency = MechanicUrgency::URGENT;
    else if (castTime < 2000)
        mechanic.urgency = MechanicUrgency::HIGH;
    else
        mechanic.urgency = MechanicUrgency::MODERATE;

    // Determine response
    if (EnumFlag<MechanicType>(mechanic.type).HasFlag(MechanicType::INTERRUPT_REQUIRED))
        mechanic.response = MechanicResponse::INTERRUPT;
    else if (EnumFlag<MechanicType>(mechanic.type).HasFlag(MechanicType::AOE_DAMAGE))
        mechanic.response = MechanicResponse::MOVE_AWAY;
    else if (EnumFlag<MechanicType>(mechanic.type).HasFlag(MechanicType::FRONTAL_CLEAVE))
        mechanic.response = MechanicResponse::AVOID;
    else if (EnumFlag<MechanicType>(mechanic.type).HasFlag(MechanicType::SPREAD_REQUIRED))
        mechanic.response = MechanicResponse::SPREAD_OUT;
    else if (EnumFlag<MechanicType>(mechanic.type).HasFlag(MechanicType::STACK_REQUIRED))
        mechanic.response = MechanicResponse::STACK_UP;
    // Get danger radius
    mechanic.dangerRadius = GetSpellDangerRadius(spellId);
    mechanic.safeDistance = mechanic.dangerRadius + _safeDistanceBuffer;

    // Set timing
    mechanic.triggerTime = GameTime::GetGameTimeMS() + castTime;
    mechanic.duration = spellInfo->GetDuration();

    // Estimate damage
    if (caster && target)
        mechanic.damageEstimate = EstimateSpellDamage(spellId, caster, target);
    mechanic.isActive = true;
    mechanic.description = (*spellInfo->SpellName)[sWorld->GetDefaultDbcLocale()];

    return mechanic;
}

bool MechanicAwareness::DetectAOECast(Unit* caster, float& radius, Position& center)
{
    if (!caster || !caster->GetCurrentSpell(CURRENT_GENERIC_SPELL))
        return false;

    Spell const* spell = caster->GetCurrentSpell(CURRENT_GENERIC_SPELL);
    SpellInfo const* spellInfo = spell->GetSpellInfo();
    if (!spellInfo)
        return false;

    // Check if spell has AOE effects
    for (SpellEffectInfo const& effect : spellInfo->GetEffects())
    {
        if (effect.IsTargetingArea())
        {
            radius = effect.CalcRadius(caster);
            center = caster->GetPosition();

            // Check if target location is specified
    if (spell->m_targets.HasDst())
            {
                WorldLocation const* loc = spell->m_targets.GetDstPos();
                if (loc)
                {
                    center.m_positionX = loc->GetPositionX();
                    center.m_positionY = loc->GetPositionY();
                    center.m_positionZ = loc->GetPositionZ();
                }
            }

            return true;
        }
    }

    return false;
}

bool MechanicAwareness::DetectCleave(Unit* target, float& angle, float& range)
{
    if (!target)
        return false;

    // Check if target has cleave abilities (simplified check)
    // In real implementation, would check creature template abilities
    angle = DEFAULT_CLEAVE_ANGLE;
    range = 10.0f;

    // Check active spells
    if (Spell const* spell = target->GetCurrentSpell(CURRENT_GENERIC_SPELL))
    {
        SpellInfo const* spellInfo = spell->GetSpellInfo();
        if (spellInfo)
        {
            for (SpellEffectInfo const& effect : spellInfo->GetEffects())
            {
                if (effect.IsTargetingArea() && effect.TargetA.GetTarget() == TARGET_UNIT_CONE_ENEMY_24)
                {
                    angle = 90.0f;  // Example cone angle
                    range = effect.CalcRadius(target);
                    return true;
                }
            }
        }
    }

    return false;
}

void MechanicAwareness::HandleCleaveMechanic(Unit* target, float cleaveAngle, float cleaveRange)
{
    if (!target)
        return;

    CleaveMechanic cleave;
    cleave.source = target;
    cleave.angle = cleaveAngle;
    cleave.range = cleaveRange;
    cleave.isActive = true;

    ::std::lock_guard lock(_mutex);
    _cleaveMechanics[target->GetGUID()] = cleave;
}

void MechanicAwareness::HandleAOEMechanic(const AOEZone& zone, Player* bot)
{
    if (!bot)
        return;

    // Check if bot is in danger zone
    if (zone.IsPointInZone(bot->GetPosition(), GameTime::GetGameTimeMS()))
    {
        // Find safe position
        Position safePos = FindSafeSpot(bot, zone);
        // Move to safe position
        ExecuteMovementResponse(bot, safePos, MechanicUrgency::URGENT);

        _metrics.mechanicsAvoided++;
    }
}

void MechanicAwareness::HandleProjectile(const ProjectileInfo& projectile, Player* bot)
{
    if (!bot)
        return;

    // Check if projectile will hit bot
    if (WillProjectileHit(projectile, bot))
    {
        // Calculate dodge position
        Position dodgePos = bot->GetPosition();
        // Simple dodge: move perpendicular to projectile path
        float projectileAngle = ::std::atan2(projectile.destination.m_positionY - projectile.origin.m_positionY,
                                          projectile.destination.m_positionX - projectile.origin.m_positionX);
        float dodgeAngle = Position::NormalizeOrientation(projectileAngle + static_cast<float>(M_PI) / 2.0f);

        dodgePos.m_positionX += 5.0f * cos(dodgeAngle);
        dodgePos.m_positionY += 5.0f * sin(dodgeAngle);

        ExecuteMovementResponse(bot, dodgePos, MechanicUrgency::IMMEDIATE);
    }
}

void MechanicAwareness::HandleGroundEffect(const Position& center, float radius, Player* bot)
{
    if (!bot)
        return;

    float distSq = bot->GetExactDistSq(center);
    if (distSq <= (radius * radius))
    {
        // Move out of ground effect
        float angle = center.GetRelativeAngle(bot);
        Position safePos;
        safePos.m_positionX = center.m_positionX + (radius + _safeDistanceBuffer) * cos(angle);
        safePos.m_positionY = center.m_positionY + (radius + _safeDistanceBuffer) * sin(angle);
        safePos.m_positionZ = bot->GetPositionZ();
        ExecuteMovementResponse(bot, safePos, MechanicUrgency::URGENT);
    }
}

SafePositionResult MechanicAwareness::CalculateSafePosition(Player* bot, const ::std::vector<MechanicInfo>& threats)
{
    SafePositionResult result;

    if (!bot || threats.empty())
    {
        result.position = bot ? bot->GetPosition() : Position();
        result.safetyScore = 100.0f;
        return result;
    }

    Position currentPos = bot->GetPosition();
    ::std::vector<Position> candidates = GenerateSafePositions(currentPos);

    float bestScore = -1.0f;
    Position bestPos = currentPos;

    for (const Position& candidate : candidates)
    {
        float score = EvaluatePositionSafety(candidate, threats);

        if (score > bestScore)
        {
            bestScore = score;
            bestPos = candidate;
        }

        if (score >= 90.0f)  // Good enough position found
            result.alternativePositions.push_back(candidate);
    }

    result.position = bestPos;
    result.safetyScore = bestScore;
    result.distanceToMove = currentPos.GetExactDist(bestPos);
    result.requiresMovement = result.distanceToMove > 1.0f;

    // Determine response type
    if (result.requiresMovement)
    {
        for (const MechanicInfo& threat : threats)
        {
            if (threat.urgency == MechanicUrgency::IMMEDIATE)
            {
                result.requiredResponse = MechanicResponse::MOVE_AWAY;
                break;
            }
        }
    }

    return result;
}

Position MechanicAwareness::FindSafeSpot(Player* bot, const AOEZone& danger, float minSafeDistance)
{
    if (!bot)
        return {};

    Position currentPos = bot->GetPosition();
    float currentDistance = currentPos.GetExactDist(danger.center);

    // If already safe, stay put
    if (currentDistance > danger.radius + minSafeDistance)
        return currentPos;

    // Calculate escape angle
    float escapeAngle = danger.center.GetRelativeAngle(&currentPos);
    float escapeDistance = danger.radius + minSafeDistance;

    Position safePos;
    safePos.m_positionX = danger.center.m_positionX + escapeDistance * cos(escapeAngle);
    safePos.m_positionY = danger.center.m_positionY + escapeDistance * sin(escapeAngle);
    safePos.m_positionZ = currentPos.m_positionZ;

    return safePos;
}

::std::vector<Position> MechanicAwareness::GenerateSafePositions(const Position& currentPos, float searchRadius)
{
    ::std::vector<Position> positions;

    // Generate positions in a circle around current position
    for (int angle = 0; angle < 360; angle += 30)
    {
        float radians = angle * static_cast<float>(M_PI) / 180.0f;

        for (float distance = 5.0f; distance <= searchRadius; distance += 5.0f)
        {
            Position candidate;
            candidate.m_positionX = currentPos.m_positionX + distance * cos(radians);
            candidate.m_positionY = currentPos.m_positionY + distance * sin(radians);
            candidate.m_positionZ = currentPos.m_positionZ;
            positions.push_back(candidate);
        }
    }

    return positions;
}

bool MechanicAwareness::IsPositionSafe(const Position& pos, const ::std::vector<AOEZone>& dangers, uint32 currentTime)
{
    for (const AOEZone& zone : dangers)
    {
        if (zone.IsPointInZone(pos, currentTime))
            return false;
    }

    return true;
}

MechanicResponse MechanicAwareness::DetermineResponse(Player* bot, const MechanicInfo& mechanic)
{
    if (!bot)
        return MechanicResponse::NONE;

    // Priority-based response determination
    if (EnumFlag<MechanicType>(mechanic.type).HasFlag(MechanicType::INTERRUPT_REQUIRED))
        return MechanicResponse::INTERRUPT;

    if (EnumFlag<MechanicType>(mechanic.type).HasFlag(MechanicType::DISPEL_REQUIRED))
        return MechanicResponse::DISPEL;

    if (EnumFlag<MechanicType>(mechanic.type).HasFlag(MechanicType::AOE_DAMAGE))
        return MechanicResponse::MOVE_AWAY;

    if (EnumFlag<MechanicType>(mechanic.type).HasFlag(MechanicType::FRONTAL_CLEAVE))
        return MechanicResponse::AVOID;

    if (EnumFlag<MechanicType>(mechanic.type).HasFlag(MechanicType::SPREAD_REQUIRED))
        return MechanicResponse::SPREAD_OUT;

    if (EnumFlag<MechanicType>(mechanic.type).HasFlag(MechanicType::STACK_REQUIRED))
        return MechanicResponse::STACK_UP;

    if (EnumFlag<MechanicType>(mechanic.type).HasFlag(MechanicType::SOAK_REQUIRED))
        return MechanicResponse::SOAK;

    if (EnumFlag<MechanicType>(mechanic.type).HasFlag(MechanicType::TANK_SWAP))
        return MechanicResponse::TANK_SWAP;

    if (EnumFlag<MechanicType>(mechanic.type).HasFlag(MechanicType::LOS_BREAK))
        return MechanicResponse::BREAK_LOS;

    return MechanicResponse::NONE;
}

void MechanicAwareness::RespondToPositionalRequirement(Spell* spell, Player* caster)
{
    if (!spell || !caster)
        return;

    SpellInfo const* spellInfo = spell->GetSpellInfo();
    if (!spellInfo)
        return;

    // Check for positional requirements
    // This would check spell attributes for behind/flank requirements
}

bool MechanicAwareness::ShouldInterrupt(Unit* target, uint32 spellId)
{
    if (!target)
        return false;

    return IsInterruptibleSpell(spellId);
}

bool MechanicAwareness::ShouldDispel(Unit* target, uint32 spellId)
{
    if (!target)
        return false;

    return IsDispellableDebuff(spellId);
}

void MechanicAwareness::RegisterAOEZone(const AOEZone& zone)
{
    ::std::lock_guard lock(_mutex);
    _activeAOEZones.push_back(zone);

    // Merge overlapping zones
    MergeOverlappingZones();
}

void MechanicAwareness::UpdateAOEZones(uint32 currentTime)
{
    ::std::lock_guard lock(_mutex);

    // Remove expired zones
    _activeAOEZones.erase(
        ::std::remove_if(_activeAOEZones.begin(), _activeAOEZones.end(),
            [currentTime](const AOEZone& zone) {
                return !zone.IsActive(currentTime);
            }),
        _activeAOEZones.end()
    );
}

void MechanicAwareness::RemoveExpiredZones(uint32 currentTime)
{
    UpdateAOEZones(currentTime);
}

::std::vector<AOEZone> MechanicAwareness::GetActiveAOEZones() const
{
    ::std::lock_guard lock(_mutex);
    return _activeAOEZones;
}

::std::vector<AOEZone> MechanicAwareness::GetUpcomingAOEZones(uint32 timeWindow) const
{
    ::std::lock_guard lock(_mutex);
    ::std::vector<AOEZone> upcoming;
    uint32 currentTime = GameTime::GetGameTimeMS();

    for (const AOEZone& zone : _activeAOEZones)
    {
        if (zone.startTime > currentTime && zone.startTime <= currentTime + timeWindow)
            upcoming.push_back(zone);
    }

    return upcoming;
}

void MechanicAwareness::TrackProjectile(const ProjectileInfo& projectile)
{
    ::std::lock_guard lock(_mutex);
    _trackedProjectiles.push_back(projectile);
}

void MechanicAwareness::UpdateProjectiles(uint32 currentTime)
{
    ::std::lock_guard lock(_mutex);

    // Remove projectiles that have impacted
    _trackedProjectiles.erase(
        ::std::remove_if(_trackedProjectiles.begin(), _trackedProjectiles.end(),
            [currentTime](const ProjectileInfo& proj) {
                return currentTime >= proj.impactTime;
            }),
        _trackedProjectiles.end()
    );

    // Update current positions
    for (ProjectileInfo& proj : _trackedProjectiles)
    {
        proj.currentPosition = proj.PredictPosition(currentTime);
    }
}

::std::vector<ProjectileInfo> MechanicAwareness::GetIncomingProjectiles(Player* target) const
{
    if (!target)
        return {};

    ::std::lock_guard lock(_mutex);
    ::std::vector<ProjectileInfo> incoming;
    for (const ProjectileInfo& proj : _trackedProjectiles)
    {
        if (proj.targetGuid == target->GetGUID() ||
            proj.WillHitPosition(target->GetPosition()))
        {
            incoming.push_back(proj);
        }
    }

    return incoming;
}

bool MechanicAwareness::WillProjectileHit(const ProjectileInfo& projectile, Player* target, float tolerance)
{
    if (!target)
        return false;

    return projectile.targetGuid == target->GetGUID() ||
           projectile.WillHitPosition(target->GetPosition(), tolerance);
}

void MechanicAwareness::RegisterCleaveMechanic(Unit* source, const CleaveMechanic& cleave)
{
    if (!source)
        return;

    ::std::lock_guard lock(_mutex);
    _cleaveMechanics[source->GetGUID()] = cleave;
}

void MechanicAwareness::UpdateCleaveMechanics()
{
    ::std::lock_guard lock(_mutex);
    uint32 currentTime = GameTime::GetGameTimeMS();

    // Update cleave states
    for (auto& [guid, cleave] : _cleaveMechanics)
    {
        if (cleave.nextCleaveTime > 0 && currentTime >= cleave.nextCleaveTime)
        {
            cleave.isActive = true;
            if (cleave.cleaveInterval > 0)
                cleave.nextCleaveTime = currentTime + cleave.cleaveInterval;
        }
    }
}

bool MechanicAwareness::IsInCleaveZone(Player* bot, Unit* source)
{
    if (!bot || !source)
        return false;

    ::std::lock_guard lock(_mutex);
    auto it = _cleaveMechanics.find(source->GetGUID());

    if (it == _cleaveMechanics.end())
        return false;

    return !it->second.IsPositionSafe(bot->GetPosition());
}

Position MechanicAwareness::GetCleaveAvoidancePosition(Player* bot, Unit* source)
{
    if (!bot || !source)
        return bot ? bot->GetPosition() : Position();

    ::std::lock_guard lock(_mutex);
    auto it = _cleaveMechanics.find(source->GetGUID());

    if (it == _cleaveMechanics.end())
        return bot->GetPosition();

    // Calculate safe position to side of cleave
    float safeAngle = it->second.GetSafeAngle(true);
    float distance = ::std::sqrt(bot->GetExactDistSq(source)); // Calculate once from squared distance

    Position safePos;
    safePos.m_positionX = source->GetPositionX() + distance * cos(safeAngle);
    safePos.m_positionY = source->GetPositionY() + distance * sin(safeAngle);
    safePos.m_positionZ = bot->GetPositionZ();
    return safePos;
}

::std::vector<MechanicPrediction> MechanicAwareness::PredictMechanics(Unit* target, uint32 timeAhead)
{
    ::std::vector<MechanicPrediction> predictions;

    if (!target)
        return predictions;

    ::std::lock_guard lock(_mutex);
    auto it = _mechanicHistory.find(target->GetGUID());
    if (it == _mechanicHistory.end())
        return predictions;

    // Analyze patterns in mechanic history
    for (const MechanicInfo& historic : it->second)
    {
        MechanicPrediction prediction;
        prediction.type = historic.type;
        prediction.confidence = AnalyzeMechanicPattern(target, historic.type);

        if (prediction.confidence > 0.5f)
        {
            prediction.predictedTime = GameTime::GetGameTimeMS() + timeAhead;
            prediction.predictedLocation = target->GetPosition();
            prediction.predictedRadius = historic.dangerRadius;
            prediction.basis = "Pattern analysis";
            predictions.push_back(prediction);
        }
    }

    return predictions;
}

MechanicPrediction MechanicAwareness::PredictNextMechanic(Unit* target)
{
    auto predictions = PredictMechanics(target, 5000);

    if (!predictions.empty())
    {
        // Return most confident prediction
        return *::std::max_element(predictions.begin(), predictions.end(),
            [](const MechanicPrediction& a, const MechanicPrediction& b) {
                return a.confidence < b.confidence;
            });
    }

    return MechanicPrediction();
}

float MechanicAwareness::CalculateMechanicProbability(Unit* target, MechanicType type)
{
    if (!target)
        return 0.0f;

    return AnalyzeMechanicPattern(target, type);
}

void MechanicAwareness::CoordinateGroupResponse(const MechanicInfo& mechanic, ::std::vector<Player*> group)
{
    if (group.empty())
        return;

    MechanicResponse response = mechanic.response;

    switch (response)
    {
        case MechanicResponse::SPREAD_OUT:
        {
            auto positions = CalculateSpreadPositions(group);
            for (Player* member : group)
            {
                auto it = positions.find(member->GetGUID());
                if (it != positions.end())
                    ExecuteMovementResponse(member, it->second, mechanic.urgency);
            }
            break;
        }

        case MechanicResponse::STACK_UP:
        {
            Position stackPos = CalculateStackPosition(group);
            for (Player* member : group)
                ExecuteMovementResponse(member, stackPos, mechanic.urgency);
            break;
        }

        default:
            ExecuteGroupResponse(group, response);
            break;
    }
}

::std::unordered_map<ObjectGuid, Position> MechanicAwareness::CalculateSpreadPositions(
    const ::std::vector<Player*>& group, float minDistance)
{
    ::std::unordered_map<ObjectGuid, Position> positions;

    if (group.empty())
        return positions;

    // Calculate center position
    float centerX = 0, centerY = 0, centerZ = 0;
    for (const Player* member : group)
    {
        centerX += member->GetPositionX();
        centerY += member->GetPositionY();
        centerZ += member->GetPositionZ();
    }

    centerX /= group.size();
    centerY /= group.size();
    centerZ /= group.size();

    // Distribute players in a circle
    float angleStep = 2.0f * static_cast<float>(M_PI) / group.size();
    float radius = minDistance * group.size() / (2.0f * static_cast<float>(M_PI));

    for (size_t i = 0; i < group.size(); ++i)
    {
        float angle = i * angleStep;
        Position pos;
        pos.m_positionX = centerX + radius * cos(angle);
        pos.m_positionY = centerY + radius * sin(angle);
        pos.m_positionZ = centerZ;

        positions[group[i]->GetGUID()] = pos;
    }

    return positions;
}

Position MechanicAwareness::CalculateStackPosition(const ::std::vector<Player*>& group)
{
    Position stackPos;

    if (group.empty())
        return stackPos;

    // Calculate average position
    float sumX = 0, sumY = 0, sumZ = 0;
    for (const Player* member : group)
    {
        sumX += member->GetPositionX();
        sumY += member->GetPositionY();
        sumZ += member->GetPositionZ();
    }

    stackPos.m_positionX = sumX / group.size();
    stackPos.m_positionY = sumY / group.size();
    stackPos.m_positionZ = sumZ / group.size();

    return stackPos;
}

bool MechanicAwareness::IsInterruptibleSpell(uint32 spellId)
{
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return false;

    return !spellInfo->HasAttribute(SPELL_ATTR0_NO_IMMUNITIES);
           // Note: SPELL_ATTR0_UNAFFECTED_BY_INVULNERABILITY not available in this TrinityCore version
}

bool MechanicAwareness::IsDispellableDebuff(uint32 spellId)
{
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return false;

    return spellInfo->Dispel != DISPEL_NONE;
           // Note: SPELL_ATTR1_CANT_BE_REFLECTED not available in this TrinityCore version
}

bool MechanicAwareness::RequiresSoak(uint32 spellId)
{
    // Would check spell database for soak requirements
    // For now, use spell ID ranges or specific spells
    return false;
}

float MechanicAwareness::GetSpellDangerRadius(uint32 spellId)
{
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return DEFAULT_AOE_RADIUS;

    for (SpellEffectInfo const& effect : spellInfo->GetEffects())
    {
        if (effect.IsTargetingArea())
            return effect.CalcRadius(nullptr);
    }

    return DEFAULT_AOE_RADIUS;
}

void MechanicAwareness::RegisterEnvironmentalHazard(const Position& location, float radius, uint32 duration)
{
    AOEZone hazard;
    hazard.center = location;
    hazard.radius = radius;
    hazard.startTime = GameTime::GetGameTimeMS();
    hazard.duration = duration;
    hazard.type = MechanicType::ENVIRONMENTAL;
    hazard.isPersistent = false;

    RegisterAOEZone(hazard);
}

bool MechanicAwareness::IsEnvironmentalHazard(const Position& pos)
{
    ::std::lock_guard lock(_mutex);
    uint32 currentTime = GameTime::GetGameTimeMS();

    for (const AOEZone& zone : _activeAOEZones)
    {
        if (zone.type == MechanicType::ENVIRONMENTAL && zone.IsPointInZone(pos, currentTime))
            return true;
    }

    return false;
}

::std::vector<Position> MechanicAwareness::GetEnvironmentalHazards() const
{
    ::std::lock_guard lock(_mutex);
    ::std::vector<Position> hazards;

    for (const AOEZone& zone : _activeAOEZones)
    {
        if (zone.type == MechanicType::ENVIRONMENTAL)
            hazards.push_back(zone.center);
    }

    return hazards;
}

void MechanicAwareness::SetReactionTime(uint32 minMs, uint32 maxMs)
{
    _minReactionTime = minMs;
    _maxReactionTime = maxMs;
}

void MechanicAwareness::LogMechanicDetection(const MechanicInfo& mechanic)
{
    TC_LOG_DEBUG("bot.playerbot", "Mechanic detected: Type={}, Spell={}, Urgency={}, Response={}",
                 uint32(mechanic.type), mechanic.spellId, uint32(mechanic.urgency), uint32(mechanic.response));
}

void MechanicAwareness::LogMechanicResponse(Player* bot, const MechanicInfo& mechanic, MechanicResponse response)
{
    if (!bot)
        return;

    TC_LOG_DEBUG("bot.playerbot", "Bot {} responding to mechanic: Type={}, Response={}",
                 bot->GetName(), uint32(mechanic.type), uint32(response));
}

bool MechanicAwareness::IsDangerousSpell(uint32 spellId)
{
    // Would check against database of dangerous spells
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return false;

    // Simple check for damage spells
    for (SpellEffectInfo const& effect : spellInfo->GetEffects())
    {
        switch (effect.Effect)
        {
            case SPELL_EFFECT_SCHOOL_DAMAGE:
            case SPELL_EFFECT_ENVIRONMENTAL_DAMAGE:
            case SPELL_EFFECT_INSTAKILL:
            case SPELL_EFFECT_KNOCK_BACK:
                return true;
            default:
                break;
        }
    }

    return false;
}

float MechanicAwareness::EstimateSpellDamage(uint32 spellId, Unit* caster, Unit* target)
{
    if (!caster || !target)
        return 0.0f;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return 0.0f;

    // Simplified damage calculation
    float damage = 0.0f;
    for (SpellEffectInfo const& effect : spellInfo->GetEffects())
    {
        if (effect.Effect == SPELL_EFFECT_SCHOOL_DAMAGE)
        {
            damage += effect.CalcValue(caster);
        }
    }

    return damage;
}

bool MechanicAwareness::RequiresPositioning(uint32 spellId)
{
    MechanicType type = GetSpellMechanicType(spellId);
    using T = std::underlying_type_t<MechanicType>;
    return (static_cast<T>(type) & (static_cast<T>(MechanicType::FRONTAL_CLEAVE) | static_cast<T>(MechanicType::TAIL_SWIPE) |
                   static_cast<T>(MechanicType::POSITIONAL) | static_cast<T>(MechanicType::AOE_DAMAGE))) != 0;
}

MechanicType MechanicAwareness::GetSpellMechanicType(uint32 spellId)
{
    return MechanicDatabase::Instance().GetSpellMechanicType(spellId);
}

MechanicInfo MechanicAwareness::DetectCastingMechanic(Unit* caster)
{
    MechanicInfo mechanic;

    if (!caster)
        return mechanic;

    Spell const* spell = caster->GetCurrentSpell(CURRENT_GENERIC_SPELL);
    if (!spell)
        return mechanic;

    return AnalyzeSpellMechanic(spell->GetSpellInfo()->Id, caster, spell->m_targets.GetUnitTarget());
}

MechanicInfo MechanicAwareness::DetectAreaTrigger(AreaTrigger* trigger)
{
    MechanicInfo mechanic;

    if (!trigger)
        return mechanic;

    mechanic.type = MechanicType::GROUND_EFFECT;
    mechanic.sourcePosition = trigger->GetPosition();
    mechanic.dangerRadius = 5.0f;  // Default radius
    mechanic.urgency = MechanicUrgency::URGENT;
    mechanic.response = MechanicResponse::MOVE_AWAY;
    mechanic.isActive = true;

    return mechanic;
}

MechanicInfo MechanicAwareness::DetectDebuffMechanic(Player* bot)
{
    MechanicInfo mechanic;

    if (!bot)
        return mechanic;

    // Check for dangerous debuffs
    Unit::AuraApplicationMap const& auras = bot->GetAppliedAuras();
    for (auto const& [spellId, aurApp] : auras)
    {
        Aura const* aura = aurApp->GetBase();
        if (!aura)
            continue;

        SpellInfo const* spellInfo = aura->GetSpellInfo();
        if (!spellInfo || spellInfo->IsPositive())
            continue;

        // Check for spread mechanics
        // Note: SPELL_ATTR1_DISPEL_AURAS_ON_IMMUNITY not available in this TrinityCore version
        /*
        if (spellInfo->HasAttribute(SPELL_ATTR1_DISPEL_AURAS_ON_IMMUNITY))
        {
            mechanic.type = MechanicType::DEBUFF_SPREAD;
            mechanic.spellId = spellId;
            mechanic.urgency = MechanicUrgency::HIGH;
            mechanic.response = MechanicResponse::SPREAD_OUT;
            mechanic.requiresGroupResponse = true;
            return mechanic;
        }
        */

        // Check for dispel requirements
    if (IsDispellableDebuff(spellId))
        {
            mechanic.type = MechanicType::DISPEL_REQUIRED;
            mechanic.spellId = spellId;
            mechanic.urgency = MechanicUrgency::URGENT;
            mechanic.response = MechanicResponse::DISPEL;
            return mechanic;
        }
    }

    return mechanic;
}

::std::vector<MechanicInfo> MechanicAwareness::ScanForThreats(Player* bot, float scanRadius)
{
    ::std::vector<MechanicInfo> threats;

    if (!bot)
        return threats;

    // Scan for active AOE zones
    uint32 currentTime = GameTime::GetGameTimeMS();
    for (const AOEZone& zone : _activeAOEZones)
    {
        if (zone.IsActive(currentTime) && bot->GetExactDistSq(zone.center) <= (scanRadius * scanRadius))
        {
            MechanicInfo threat;
            threat.type = zone.type;
            threat.sourcePosition = zone.center;
            threat.dangerRadius = zone.GetCurrentRadius(currentTime);
            threat.urgency = zone.IsPointInZone(bot->GetPosition(), currentTime) ?
                            MechanicUrgency::IMMEDIATE : MechanicUrgency::HIGH;
            threat.response = MechanicResponse::MOVE_AWAY;
            threat.isActive = true;
            threats.push_back(threat);
        }
    }

    // Scan for projectiles
    for (const ProjectileInfo& proj : _trackedProjectiles)
    {
        if (WillProjectileHit(proj, bot))
        {
            MechanicInfo threat;
            threat.type = MechanicType::PROJECTILE;
            threat.sourcePosition = proj.currentPosition;
            threat.urgency = proj.TimeToImpact(currentTime) < 1000 ?
                            MechanicUrgency::IMMEDIATE : MechanicUrgency::URGENT;
            threat.response = MechanicResponse::AVOID;
            threat.isActive = true;
            threats.push_back(threat);
        }
    }

    return threats;
}

float MechanicAwareness::EvaluatePositionSafety(const Position& pos, const ::std::vector<MechanicInfo>& threats)
{
    float safety = 100.0f;

    for (const MechanicInfo& threat : threats)
    {
        float distance = pos.GetExactDist(threat.sourcePosition);
        if (distance <= threat.dangerRadius)
        {
            safety -= 50.0f;  // In danger zone
        }
        else if (distance <= threat.safeDistance)
        {
            float penalty = (1.0f - (distance - threat.dangerRadius) /
                           (threat.safeDistance - threat.dangerRadius)) * 30.0f;
            safety -= penalty;
        }

        // Urgency modifier
    switch (threat.urgency)
        {
            case MechanicUrgency::IMMEDIATE:
                safety *= 0.5f;
                break;
            case MechanicUrgency::URGENT:
                safety *= 0.7f;
                break;
            case MechanicUrgency::HIGH:
                safety *= 0.85f;
                break;
            default:
                break;
        }
    }

    return ::std::max(0.0f, safety);
}

float MechanicAwareness::CalculateDangerScore(const Position& pos, const AOEZone& zone, uint32 currentTime)
{
    if (!zone.IsActive(currentTime))
        return 0.0f;

    if (!zone.IsPointInZone(pos, currentTime))
        return 0.0f;

    float danger = 100.0f;

    // Calculate expected damage
    uint32 timeInZone = zone.duration > 0 ? ::std::min(1000u, zone.duration) : 1000u;
    float expectedDamage = zone.EstimateDamage(timeInZone);

    // Scale danger by damage
    if (expectedDamage > 0)
        danger = ::std::min(100.0f, expectedDamage / 1000.0f * 10.0f);

    return danger;
}

bool MechanicAwareness::ValidateSafePosition(const Position& pos, Player* bot)
{
    if (!bot)
        return false;

    // Check if position is walkable
    // Would need actual pathfinding check

    // Check if position is reasonable distance
    if (bot->GetExactDistSq(pos) > (50.0f * 50.0f)) // 2500.0f
        return false;

    return true;
}

void MechanicAwareness::ExecuteMovementResponse(Player* bot, const Position& safePos, MechanicUrgency urgency)
{
    if (!bot)
        return;

    // Calculate reaction delay based on urgency
    uint32 reactionDelay = _minReactionTime;

    // Determine priority based on urgency
    PlayerBotMovementPriority priority = PlayerBotMovementPriority::BOSS_MECHANIC;

    switch (urgency)
    {
        case MechanicUrgency::IMMEDIATE:
            reactionDelay = _minReactionTime;
            priority = PlayerBotMovementPriority::BOSS_MECHANIC;  // CRITICAL priority (250)
            break;
        case MechanicUrgency::URGENT:
            reactionDelay = (_minReactionTime + _maxReactionTime) / 2;
            priority = PlayerBotMovementPriority::DUNGEON_MECHANIC;  // VERY_HIGH priority (205)
            break;
        default:
            reactionDelay = _maxReactionTime;
            priority = PlayerBotMovementPriority::OBSTACLE_AVOIDANCE_EMERGENCY;  // CRITICAL priority (245)
            break;
    }

    // PHASE 3 MIGRATION: Use Movement Arbiter with CRITICAL priority
    // This ensures boss mechanics ALWAYS override combat positioning, following, etc.
    BotAI* botAI = dynamic_cast<BotAI*>(bot->GetAI());
    if (botAI && botAI->GetUnifiedMovementCoordinator())
    {
        // Use Movement Arbiter for priority-based arbitration
        bool accepted = botAI->RequestPointMovement(
            priority,
            safePos,
            "Avoiding boss mechanic/AOE",
            "MechanicAwareness");

        if (accepted)
        {
            TC_LOG_DEBUG("playerbot.movement.arbiter",
                "MechanicAwareness: Bot {} requested movement to safe position ({:.2f}, {:.2f}, {:.2f}) with {} priority",
                bot->GetName(),
                safePos.GetPositionX(),
                safePos.GetPositionY(),
                safePos.GetPositionZ(),
                MovementPriorityMapper::GetPriorityName(priority));
        }
        else
        {
            TC_LOG_WARN("playerbot.movement.arbiter",
                "MechanicAwareness: Bot {} emergency movement request FILTERED (duplicate or lower priority active)",
                bot->GetName());
        }
    }
    else
    {
        // FALLBACK: Use BotMovementController with validated pathfinding
        TC_LOG_WARN("playerbot.movement.arbiter",
            "MechanicAwareness: Bot {} has no MovementArbiter - using fallback pathfinding",
            bot->GetName());

        if (BotAI* ai = GetBotAI(bot))
        {
            if (!ai->MoveTo(safePos, true))
            {
                // Final fallback to legacy if validation fails
                bot->GetMotionMaster()->MovePoint(0, safePos);
            }
        }
        else
        {
            // Non-bot player - use standard movement
            bot->GetMotionMaster()->MovePoint(0, safePos);
        }
    }

    // Track reaction time
    _metrics.reactionTimeTotal += reactionDelay;
    _metrics.reactionCount++;
}

void MechanicAwareness::ExecuteDefensiveResponse(Player* bot, const MechanicInfo& mechanic)
{
    if (!bot)
        return;

    // Would trigger defensive cooldowns based on mechanic type
    TC_LOG_DEBUG("bot.playerbot", "Bot {} using defensive for mechanic type {}",
                 bot->GetName(), uint32(mechanic.type));
}

void MechanicAwareness::ExecuteGroupResponse(const ::std::vector<Player*>& group, MechanicResponse response)
{
    for (Player* member : group)
    {
        // Execute response for each group member
    switch (response)
        {
            case MechanicResponse::USE_DEFENSIVE:
                ExecuteDefensiveResponse(member, {});
                break;
            default:
                break;
        }
    }
}

void MechanicAwareness::UpdateMechanicHistory(Unit* target, const MechanicInfo& mechanic)
{
    if (!target)
        return;

    ::std::lock_guard lock(_mutex);
    auto& history = _mechanicHistory[target->GetGUID()];
    history.push_back(mechanic);

    // Limit history size
    if (history.size() > _maxHistorySize)
        history.erase(history.begin());
}

float MechanicAwareness::AnalyzeMechanicPattern(Unit* target, MechanicType type)
{
    if (!target)
        return 0.0f;

    ::std::lock_guard lock(_mutex);
    auto it = _mechanicHistory.find(target->GetGUID());

    if (it == _mechanicHistory.end())
        return 0.0f;

    // Count occurrences of mechanic type
    int count = 0;
    for (const MechanicInfo& historic : it->second)
    {
        if (historic.type == type)
            count++;
    }

    // Simple probability based on frequency
    return it->second.empty() ? 0.0f : float(count) / it->second.size();
}

void MechanicAwareness::CleanupOldData(uint32 currentTime)
{
    UpdateAOEZones(currentTime);
    UpdateProjectiles(currentTime);

    // Cleanup mechanic history
    ::std::lock_guard lock(_mutex);
    for (auto& [guid, history] : _mechanicHistory)
    {
        history.erase(
            ::std::remove_if(history.begin(), history.end(),
                [currentTime](const MechanicInfo& info) {
                    return info.IsExpired(currentTime);
                }),
            history.end()
        );
    }
}

void MechanicAwareness::MergeOverlappingZones()
{
    // Merge AOE zones that overlap significantly
    // This helps reduce computation for overlapping effects
}

// MechanicDatabase implementation
MechanicDatabase& MechanicDatabase::Instance()
{
    static MechanicDatabase instance;
    return instance;
}

void MechanicDatabase::RegisterSpellMechanic(uint32 spellId, MechanicType type, float radius, float angle)
{
    ::std::lock_guard lock(_mutex);
    SpellMechanicData& data = _spellMechanics[spellId];
    data.type = type;
    data.radius = radius;
    data.angle = angle;
}

MechanicType MechanicDatabase::GetSpellMechanicType(uint32 spellId) const
{
    ::std::lock_guard lock(_mutex);
    auto it = _spellMechanics.find(spellId);
    return it != _spellMechanics.end() ? it->second.type : MechanicType::NONE;
}

float MechanicDatabase::GetSpellRadius(uint32 spellId) const
{
    ::std::lock_guard lock(_mutex);
    auto it = _spellMechanics.find(spellId);
    return it != _spellMechanics.end() ? it->second.radius : 0.0f;
}

float MechanicDatabase::GetSpellAngle(uint32 spellId) const
{
    ::std::lock_guard lock(_mutex);
    auto it = _spellMechanics.find(spellId);
    return it != _spellMechanics.end() ? it->second.angle : 0.0f;
}

void MechanicDatabase::LoadMechanicData()
{
    // Load WoW 12.0 mechanic data
    // This would be populated from database or configuration

    // Example entries for common mechanics
    RegisterSpellMechanic(1, MechanicType::FRONTAL_CLEAVE, 10.0f, 90.0f);  // Cleave
    RegisterSpellMechanic(2, MechanicType::WHIRLWIND, 8.0f, 360.0f);      // Whirlwind

    LoadDungeonMechanics();
    LoadRaidMechanics();
}

void MechanicDatabase::LoadDungeonMechanics()
{
    // Load dungeon-specific mechanics
    // Would be populated with actual WoW 12.0 dungeon mechanics
}

void MechanicDatabase::LoadRaidMechanics()
{
    // Load raid-specific mechanics
    // Would be populated with actual WoW 12.0 raid mechanics
}

} // namespace Playerbot