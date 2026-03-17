/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 */

#include "MovementIntegration.h"
#include "GameTime.h"
#include "PositionManager.h"  // Enterprise-grade positioning algorithms
#include "LineOfSightManager.h"
#include "Player.h"
#include "Unit.h"
#include "Group.h"  // For healer group member queries
#include "MotionMaster.h"
#include "Log.h"
#include "../BotAI.h"
#include "Core/PlayerBotHelpers.h"
#include "../../Movement/BotMovementUtil.h"
#include <algorithm>
#include <cmath>

namespace Playerbot
{

MovementIntegration::MovementIntegration(Player* bot, PositionManager* positionManager)
    : _bot(bot)
    , _positionManager(positionManager)
    , _lastUpdate(0)
    , _currentSituation(static_cast<CombatSituation>(0))
{
    // CRITICAL: Do NOT access bot->GetName() in constructor!
    // Bot's internal data (m_name) is not initialized during constructor chain.
    // Accessing it causes ACCESS_VIOLATION crash in string construction.
    if (!_positionManager)
    {
        TC_LOG_ERROR("playerbot", "MovementIntegration: PositionManager is null");
    }
}

void MovementIntegration::Update(uint32 diff, CombatSituation situation)
{
    if (!_bot)
        return;

    _lastUpdate += diff;
    _currentSituation = situation;

    if (_lastUpdate < UPDATE_INTERVAL)
        return;

    _lastUpdate = 0;

    // Update danger zones (remove expired)
    UpdateDangerZones();

    // Evaluate positioning needs (priority order)
    MovementCommand command;

    // 1. Check danger (EMERGENCY)
    command = CheckDanger();
    if (command.urgency == MovementUrgency::EMERGENCY)
    {
        _currentCommand = command;
        return;
    }

    // 2. Check line of sight (HIGH)
    command = CheckLineOfSight();
    if (command.urgency == MovementUrgency::HIGH)
    {
        _currentCommand = command;
        return;
    }

    // 3. Check range (HIGH)
    command = CheckRange();
    if (command.urgency == MovementUrgency::HIGH)
    {
        _currentCommand = command;
        return;
    }

    // 4. Evaluate general positioning (MEDIUM/LOW)
    command = EvaluatePositioning();
    _currentCommand = command;
}

void MovementIntegration::Reset()
{
    _dangerZones.clear();
    _currentCommand = MovementCommand();
    _lastUpdate = 0;
}

bool MovementIntegration::NeedsMovement()
{
    return _currentCommand.IsValid();
}

bool MovementIntegration::NeedsUrgentMovement()
{
    return _currentCommand.IsValid() && _currentCommand.urgency == MovementUrgency::HIGH;
}

bool MovementIntegration::NeedsEmergencyMovement()
{
    return _currentCommand.IsValid() && _currentCommand.urgency == MovementUrgency::EMERGENCY;
}

bool MovementIntegration::NeedsRepositioning()
{
    return _currentCommand.IsValid() &&
           (_currentCommand.urgency == MovementUrgency::MEDIUM ||
            _currentCommand.urgency == MovementUrgency::LOW);
}

bool MovementIntegration::ShouldMoveToPosition(const Position& pos)
{
    if (!_bot)
        return false;

    // Check if position is better than current
    float currentDanger = GetDangerLevel(*_bot);
    float newDanger = GetDangerLevel(pos);

    // New position must be safer
    return newDanger < currentDanger;
}

bool MovementIntegration::IsPositionSafe(const Position& pos)
{
    return GetDangerLevel(pos) == 0.0f;
}

Position MovementIntegration::GetOptimalPosition()
{
    if (!_bot)
        return Position();

    // If in danger, find nearest safe position
    if (GetDangerLevel(*_bot) > 0.0f)
        return FindNearestSafePosition(*_bot, 5.0f);

    // Otherwise, use role-based positioning
    return CalculateRolePosition();
}

Position MovementIntegration::GetTargetPosition()
{
    if (_currentCommand.IsValid())
        return _currentCommand.destination;

    return Position();
}

float MovementIntegration::GetOptimalRange(Unit* target)
{
    if (!_bot || !target)
        return BOT_MELEE_RANGE;

    CombatRole role = GetCombatRole();

    switch (role)
    {
        case CombatRole::TANK:
        case CombatRole::MELEE_DPS:
            return BOT_MELEE_RANGE;

        case CombatRole::HEALER:
            return 30.0f;  // Healing range

        case CombatRole::RANGED_DPS:
            return RANGED_OPTIMAL;

        default:
            return BOT_MELEE_RANGE;
    }
}

void MovementIntegration::MoveToPosition(const Position& pos, bool urgent)
{
    if (!_bot)
        return;

    // Check if path is safe
    if (!IsPathSafe(*_bot, pos))
    {
        TC_LOG_DEBUG("playerbot", "MovementIntegration: Unsafe path from ({}, {}) to ({}, {})",
            _bot->GetPositionX(), _bot->GetPositionY(),
            pos.GetPositionX(), pos.GetPositionY());
        return;
    }

    // Execute movement
    TC_LOG_DEBUG("playerbot", "MovementIntegration: {} moving to ({}, {}, {}) - Urgent: {}",
        _bot->GetName(), pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), urgent ? "YES" : "NO");

    // Stop current movement if urgent
    if (urgent)
    {
        // Clear current movement to immediately respond to emergency
        _bot->GetMotionMaster()->Clear(MOTION_PRIORITY_NORMAL);

        // Stop any casting for emergency movement
        if (_bot->HasUnitState(UNIT_STATE_CASTING))
        {
            _bot->InterruptNonMeleeSpells(false);
            TC_LOG_DEBUG("playerbot", "MovementIntegration: {} interrupted cast for emergency movement",
                _bot->GetName());
        }

        // Use highest priority for emergency movement
        if (BotAI* ai = GetBotAI(_bot))
        {
            if (!ai->MoveTo(pos, true))
            {
                // Final fallback to legacy if validation fails
                _bot->GetMotionMaster()->MovePoint(MOTION_PRIORITY_HIGHEST, pos, true);
            }
        }
        else
        {
            // Non-bot player - use standard movement
            _bot->GetMotionMaster()->MovePoint(MOTION_PRIORITY_HIGHEST, pos, true);
        }
    }
    else
    {
        // Normal priority movement - doesn't interrupt current actions
        if (BotAI* ai = GetBotAI(_bot))
        {
            if (!ai->MoveTo(pos, true))
            {
                // Final fallback to legacy if validation fails
                _bot->GetMotionMaster()->MovePoint(0, pos);
            }
        }
        else
        {
            // Non-bot player - use standard movement
            _bot->GetMotionMaster()->MovePoint(0, pos);
        }
    }

    // Update current command tracking
    _currentCommand.destination = pos;
    _currentCommand.urgency = urgent ? MovementUrgency::EMERGENCY : MovementUrgency::MEDIUM;
    _currentCommand.expiryTime = GameTime::GetGameTimeMS() + 10000; // 10 second timeout

    TC_LOG_DEBUG("playerbot", "MovementIntegration: {} movement command issued - distance: {:.1f} yards",
        _bot->GetName(), _bot->GetExactDist2d(&pos));
}

void MovementIntegration::RegisterDangerZone(const Position& center, float radius, uint32 duration, float dangerLevel)
{
    // Keep local _dangerZones for backward compatibility with GetDangerZones() queries
    DangerZone zone;
    zone.center = center;
    zone.radius = radius;
    zone.expiryTime = GameTime::GetGameTimeMS() + duration;
    zone.dangerLevel = dangerLevel;

    _dangerZones.push_back(zone);

    // Also register with PositionManager for integrated position validation
    if (_positionManager)
    {
        AoEZone aoeZone;
        aoeZone.center = center;
        aoeZone.radius = radius;
        aoeZone.spellId = 0;  // Generic danger zone (not spell-specific)
        aoeZone.startTime = GameTime::GetGameTimeMS();
        aoeZone.duration = duration;
        aoeZone.damageRating = dangerLevel;
        aoeZone.isActive = true;

        _positionManager->RegisterAoEZone(aoeZone);
    }

    TC_LOG_DEBUG("playerbot", "MovementIntegration: Registered danger zone at ({}, {}) radius {} danger {} (synced to PositionManager)",
        center.GetPositionX(), center.GetPositionY(), radius, dangerLevel);
}

void MovementIntegration::ClearDangerZones()
{
    _dangerZones.clear();

    // Also clear PositionManager's AoE zones for consistency
    if (_positionManager)
    {
        _positionManager->ClearExpiredZones(GameTime::GetGameTimeMS());
    }
}

std::vector<DangerZone> MovementIntegration::GetDangerZones() const
{
    std::vector<DangerZone> active;

    for (const auto& zone : _dangerZones)
    {
        if (!zone.IsExpired())
            active.push_back(zone);
    }

    return active;
}

float MovementIntegration::GetDangerLevel(const Position& pos) const
{
    float maxDanger = 0.0f;

    for (const auto& zone : _dangerZones)
    {
        if (zone.IsExpired())
            continue;

        if (zone.IsInDanger(pos))
            maxDanger = std::max(maxDanger, zone.dangerLevel);
    }

    return maxDanger;
}

Position MovementIntegration::FindNearestSafePosition(const Position& from, float minDistance)
{
    if (!_positionManager)
        return from;

    // Use PositionManager's FindSafePosition with comprehensive validation
    // - Walkable terrain check
    // - Line of sight validation
    // - Obstacle avoidance
    // - AoE zone threat levels
    // - Movement cost analysis
    // - Escape route assessment
    return _positionManager->FindSafePosition(from, minDistance);
}

bool MovementIntegration::ShouldKite(Unit* target)
{
    if (!_bot || !target)
        return false;

    // Only ranged classes should kite
    CombatRole role = GetCombatRole();
    if (role != CombatRole::RANGED_DPS && role != CombatRole::HEALER)
        return false;

    // Check if target is melee
    if (target->GetPowerType() == POWER_MANA)
        return false;  // Likely a caster

    // Check distance
    float distance = _bot->GetDistance(target);

    // Kite if target is too close
    return distance < KITING_DISTANCE;
}

Position MovementIntegration::GetKitingPosition(Unit* target)
{
    if (!_bot || !target || !_positionManager)
        return Position();

    // Use PositionManager's FindKitingPosition with threat escape logic
    // - Maintains optimal kiting distance (15 yards default)
    // - Validates position safety
    // - Checks for obstacles and escape routes
    // - Considers current distance to avoid unnecessary movement
    return _positionManager->FindKitingPosition(target, KITING_DISTANCE);
}

// Private helper functions

void MovementIntegration::UpdateDangerZones()
{
    _dangerZones.erase(
        std::remove_if(_dangerZones.begin(), _dangerZones.end(),
            [](const DangerZone& zone) { return zone.IsExpired(); }),
        _dangerZones.end()
    );

    // Also update PositionManager's AoE zones
    if (_positionManager)
    {
        _positionManager->UpdateAoEZones(GameTime::GetGameTimeMS());
    }
}

MovementCommand MovementIntegration::EvaluatePositioning()
{
    MovementCommand command;

    if (!_bot)
        return command;

    // Check if current position is optimal
    Position optimal = CalculateRolePosition();
    float distance = _bot->GetDistance(optimal);

    // If too far from optimal position
    if (distance > 5.0f)
    {
        command.destination = optimal;
        command.urgency = MovementUrgency::MEDIUM;
        command.reason = MovementReason::OPTIMIZE_POSITION;
        command.acceptableRadius = 2.0f;
        command.expiryTime = GameTime::GetGameTimeMS() + 5000;  // 5 second validity
    }

    return command;
}

MovementCommand MovementIntegration::CheckRange()
{
    MovementCommand command;

    if (!_bot)
        return command;

    Unit* target = _bot->GetVictim();
    if (!target)
        return command;

    float optimalRange = GetOptimalRange(target);
    float currentDistance = _bot->GetDistance(target);

    // Out of range?
    if (currentDistance > optimalRange + 5.0f)
    {
        // Move closer
        float angle = _bot->GetAbsoluteAngle(target);
        float moveDistance = currentDistance - optimalRange;

        float x = _bot->GetPositionX() + moveDistance * std::cos(angle);
        float y = _bot->GetPositionY() + moveDistance * std::sin(angle);

        command.destination = Position(x, y, _bot->GetPositionZ());
        command.urgency = MovementUrgency::HIGH;
        command.reason = MovementReason::MAINTAIN_RANGE;
        command.acceptableRadius = 2.0f;
        command.expiryTime = GameTime::GetGameTimeMS() + 3000;
    }
    // Too close for ranged?
    else if (GetCombatRole() == CombatRole::RANGED_DPS && currentDistance < optimalRange - 10.0f)
    {
        // Move back
        command.destination = GetKitingPosition(target);
        command.urgency = MovementUrgency::HIGH;
        command.reason = MovementReason::KITING;
        command.acceptableRadius = 2.0f;
        command.expiryTime = GameTime::GetGameTimeMS() + 3000;
    }

    return command;
}

MovementCommand MovementIntegration::CheckDanger()
{
    MovementCommand command;

    if (!_bot)
        return command;

    // Check if in danger
    float danger = GetDangerLevel(*_bot);

    if (danger > 0.0f)
    {
        // Find nearest safe position
        Position safe = FindNearestSafePosition(*_bot, 5.0f);

        command.destination = safe;
        command.urgency = MovementUrgency::EMERGENCY;
        command.reason = MovementReason::AVOID_DANGER;
        command.acceptableRadius = 1.0f;
        command.expiryTime = GameTime::GetGameTimeMS() + 2000;  // 2 second validity

        TC_LOG_DEBUG("playerbot", "MovementIntegration: {} in danger (level {}), moving to safety",
            _bot->GetName(), danger);
    }

    return command;
}

MovementCommand MovementIntegration::CheckLineOfSight()
{
    MovementCommand command;

    if (!_bot)
        return command;

    Unit* target = _bot->GetVictim();
    if (!target)
        return command;

    // Check LoS using TrinityCore native check
    if (!_bot->IsWithinLOSInMap(target))
    {
        // Use LineOfSightManager for smart position finding when available
        // This considers terrain, buildings, preferred range vs. naive "move toward target"
        LineOfSightManager losMgr(_bot);
        Position losPos = losMgr.FindBestLineOfSightPosition(target, 0.0f);

        // If FindBest returned our own position (no candidates found), fall back to
        // stepping toward the target
        if (losPos.GetExactDist(_bot) < 2.0f)
        {
            losPos = losMgr.GetClosestUnblockedPosition(target);
        }

        // Final fallback: move directly toward target
        if (losPos.GetExactDist(_bot) < 2.0f)
        {
            float angle = _bot->GetAbsoluteAngle(target);
            float moveDistance = std::min(10.0f, _bot->GetDistance(target) * 0.5f);
            float x = _bot->GetPositionX() + moveDistance * std::cos(angle);
            float y = _bot->GetPositionY() + moveDistance * std::sin(angle);
            losPos = Position(x, y, _bot->GetPositionZ());
        }

        // Correct Z to ground level to prevent hovering/falling through terrain
        BotMovementUtil::CorrectPositionToGround(_bot, losPos);

        command.destination = losPos;
        command.urgency = MovementUrgency::HIGH;
        command.reason = MovementReason::LINE_OF_SIGHT;
        command.acceptableRadius = 2.0f;
        command.expiryTime = GameTime::GetGameTimeMS() + 3000;
    }

    return command;
}

Position MovementIntegration::CalculateRolePosition()
{
    if (!_bot || !_positionManager)
        return Position();

    Unit* target = _bot->GetVictim();
    if (!target)
        return *_bot;  // No target, stay in place

    // Use PositionManager's enterprise-grade positioning algorithms
    CombatRole role = GetCombatRole();

    switch (role)
    {
        case CombatRole::TANK:
            // Tank positioning with frontal cone avoidance and alternative angle fallbacks
            return _positionManager->FindTankPosition(target);

        case CombatRole::HEALER:
        {
            // Healer positioning with spatial grid optimization for ally visibility
            std::vector<Player*> groupMembers;
            if (Group* group = _bot->GetGroup())
            {
                for (GroupReference const& itr : group->GetMembers())
                {
                    if (Player* member = itr.GetSource())
                        groupMembers.push_back(member);
                }
            }
            return _positionManager->FindHealerPosition(groupMembers);
        }

        case CombatRole::RANGED_DPS:
            // Ranged DPS positioning with optimal range calculation
            return _positionManager->FindDpsPosition(target, PositionType::RANGED_DPS);

        case CombatRole::MELEE_DPS:
        default:
            // Melee DPS positioning with prefer-behind logic
            return _positionManager->FindDpsPosition(target, PositionType::MELEE_COMBAT);
    }
}

MovementIntegration::CombatRole MovementIntegration::GetCombatRole() const
{
    if (!_bot)
        return CombatRole::MELEE_DPS;

    // Simple class-based heuristic — full role detection from spec/talents handled by ClassAI
    switch (_bot->GetClass())
    {
        case CLASS_WARRIOR:
        case CLASS_PALADIN:
        case CLASS_DEATH_KNIGHT:
            return CombatRole::TANK;

        case CLASS_PRIEST:
        case CLASS_SHAMAN:
            return CombatRole::HEALER;

        case CLASS_HUNTER:
        case CLASS_MAGE:
        case CLASS_WARLOCK:
            return CombatRole::RANGED_DPS;

        case CLASS_ROGUE:
        case CLASS_DRUID:
        case CLASS_MONK:
            return CombatRole::MELEE_DPS;

        default:
            return CombatRole::MELEE_DPS;
    }
}

bool MovementIntegration::IsPathSafe(const Position& from, const Position& to) const
{
    // Simple check: sample points along path
    const int samples = 5;
    for (int i = 0; i <= samples; ++i)
    {
        float t = static_cast<float>(i) / samples;
        float x = from.GetPositionX() + t * (to.GetPositionX() - from.GetPositionX());
        float y = from.GetPositionY() + t * (to.GetPositionY() - from.GetPositionY());
        float z = from.GetPositionZ() + t * (to.GetPositionZ() - from.GetPositionZ());

        Position sample(x, y, z);

        if (GetDangerLevel(sample) > 0.0f)
            return false;
    }

    return true;
}

float MovementIntegration::GetMovementSpeed() const
{
    if (!_bot)
        return 7.0f;  // Default run speed

    return _bot->GetSpeed(MOVE_RUN);
}

} // namespace Playerbot
