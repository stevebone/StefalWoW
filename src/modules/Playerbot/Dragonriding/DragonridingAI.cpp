/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "DragonridingAI.h"
#include "DragonridingMgr.h"
#include "Log.h"
#include "Map.h"
#include "MotionMaster.h"
#include "Player.h"
#include "Spell.h"
#include "SpellAuras.h"
#include "SpellHistory.h"
#include "SpellMgr.h"
#include "WorldSession.h"

namespace Playerbot
{
namespace Dragonriding
{

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

DragonridingAI::DragonridingAI(Player* bot)
    : _bot(bot)
    , _state(DragonridingState::IDLE)
    , _hasDestination(false)
    , _autoBoostEnabled(true)
    , _minVigorReserve(1)
    , _updateTimer(0)
    , _boostCooldown(0)
    , _lastBoostTime(0)
    , _targetAltitude(0.0f)
    , _desiredSpeed(1.0f)
    , _isDescending(false)
{
    // Load auto-boost setting from DragonridingMgr config
    if (sDragonridingMgr->IsBotAutoBoostEnabled())
        _autoBoostEnabled = true;
}

DragonridingAI::~DragonridingAI() = default;

// ============================================================================
// MAIN UPDATE
// ============================================================================

void DragonridingAI::Update(uint32 diff)
{
    if (!_bot || !_bot->IsInWorld())
        return;

    // Update timers
    _updateTimer += diff;
    if (_boostCooldown > 0)
        _boostCooldown = (_boostCooldown > diff) ? (_boostCooldown - diff) : 0;
    _lastBoostTime += diff;

    // Rate limit updates
    if (_updateTimer < UPDATE_INTERVAL_MS)
        return;

    _updateTimer = 0;

    // Update state based on current conditions
    UpdateState();

    // If not active, nothing else to do
    if (!IsActive())
        return;

    // Update navigation if we have a destination
    if (_hasDestination)
        UpdateNavigation(diff);

    // Auto-boost if enabled
    if (_autoBoostEnabled)
        UpdateAutoBoost(diff);
}

// ============================================================================
// STATE QUERIES
// ============================================================================

uint32 DragonridingAI::GetCurrentVigor() const
{
    if (!_bot)
        return 0;

    Aura* vigorAura = _bot->GetAura(SPELL_VIGOR); // 383359 - retail vigor spell
    return vigorAura ? vigorAura->GetStackAmount() : 0;
}

uint32 DragonridingAI::GetMaxVigor() const
{
    if (!_bot || !_bot->GetSession())
        return BASE_MAX_VIGOR;

    return sDragonridingMgr->GetMaxVigor(_bot->GetSession()->GetAccountId());
}

bool DragonridingAI::HasVigor() const
{
    return GetCurrentVigor() > _minVigorReserve;
}

// ============================================================================
// CONTROL INTERFACE
// ============================================================================

bool DragonridingAI::StartSoaring()
{
    if (!_bot)
        return false;

    // Check if already soaring
    if (_state != DragonridingState::IDLE)
        return false;

    // Check if bot can use Soar (Dracthyr Evoker only)
    if (!CanUseSoar(_bot->GetRace(), _bot->GetClass()))
    {
        TC_LOG_DEBUG("playerbot.dragonriding.ai", "Bot {} cannot use Soar (not Dracthyr Evoker)",
            _bot->GetName());
        return false;
    }

    // Cast Soar
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(SPELL_SOAR, DIFFICULTY_NONE);
    if (!spellInfo)
    {
        TC_LOG_WARN("playerbot.dragonriding.ai", "Bot {}: Soar spell {} not found",
            _bot->GetName(), SPELL_SOAR);
        return false;
    }

    // Check if on cooldown
    if (_bot->GetSpellHistory()->HasCooldown(SPELL_SOAR))
    {
        TC_LOG_DEBUG("playerbot.dragonriding.ai", "Bot {} Soar is on cooldown",
            _bot->GetName());
        return false;
    }

    // Cast the spell
    _bot->CastSpell(_bot, SPELL_SOAR, false);

    TC_LOG_INFO("playerbot.dragonriding.ai", "Bot {} started Soar",
        _bot->GetName());

    return true;
}

bool DragonridingAI::StopSoaring()
{
    if (!_bot)
        return false;

    if (_state == DragonridingState::IDLE)
        return false;

    // Remove Soar aura
    _bot->RemoveAura(SPELL_SOAR);

    _state = DragonridingState::IDLE;

    TC_LOG_INFO("playerbot.dragonriding.ai", "Bot {} stopped Soar",
        _bot->GetName());

    return true;
}

void DragonridingAI::SetDestination(Position const& dest)
{
    _destination = dest;
    _hasDestination = true;
    _targetAltitude = dest.GetPositionZ();

    TC_LOG_DEBUG("playerbot.dragonriding.ai", "Bot {} set destination to ({:.1f}, {:.1f}, {:.1f})",
        _bot->GetName(), dest.GetPositionX(), dest.GetPositionY(), dest.GetPositionZ());
}

void DragonridingAI::ClearDestination()
{
    _hasDestination = false;
}

// ============================================================================
// ABILITY USAGE
// ============================================================================

bool DragonridingAI::UseSurgeForward()
{
    if (!CanUseSurgeForward())
        return false;

    _bot->CastSpell(_bot, SPELL_SURGE_FORWARD, false);
    _boostCooldown = BOOST_COOLDOWN_MS;
    _lastBoostTime = 0;
    _state = DragonridingState::BOOSTING;

    TC_LOG_DEBUG("playerbot.dragonriding.ai", "Bot {} used Surge Forward",
        _bot->GetName());

    return true;
}

bool DragonridingAI::UseSkywardAscent()
{
    if (!CanUseSkywardAscent())
        return false;

    _bot->CastSpell(_bot, SPELL_SKYWARD_ASCENT, false);
    _boostCooldown = BOOST_COOLDOWN_MS;
    _lastBoostTime = 0;
    _state = DragonridingState::ASCENDING;

    TC_LOG_DEBUG("playerbot.dragonriding.ai", "Bot {} used Skyward Ascent",
        _bot->GetName());

    return true;
}

bool DragonridingAI::UseWhirlingSurge()
{
    if (!CanUseWhirlingSurge())
        return false;

    _bot->CastSpell(_bot, SPELL_WHIRLING_SURGE, false);
    _boostCooldown = BOOST_COOLDOWN_MS;
    _lastBoostTime = 0;

    TC_LOG_DEBUG("playerbot.dragonriding.ai", "Bot {} used Whirling Surge",
        _bot->GetName());

    return true;
}

bool DragonridingAI::UseAerialHalt()
{
    if (!CanUseAerialHalt())
        return false;

    _bot->CastSpell(_bot, SPELL_AERIAL_HALT, false);
    _state = DragonridingState::GLIDING;

    TC_LOG_DEBUG("playerbot.dragonriding.ai", "Bot {} used Aerial Halt",
        _bot->GetName());

    return true;
}

bool DragonridingAI::AutoBoost()
{
    BoostDecision decision = DecideBoost();

    switch (decision)
    {
        case BoostDecision::SURGE_FORWARD:
            return UseSurgeForward();
        case BoostDecision::SKYWARD_ASCENT:
            return UseSkywardAscent();
        case BoostDecision::WHIRLING_SURGE:
            return UseWhirlingSurge();
        case BoostDecision::AERIAL_HALT:
            return UseAerialHalt();
        case BoostDecision::NONE:
        default:
            return false;
    }
}

// ============================================================================
// INTERNAL METHODS
// ============================================================================

void DragonridingAI::UpdateState()
{
    if (!_bot)
        return;

    // Check if we're in dragonriding mode
    int32 flightCapId = _bot->GetFlightCapabilityID();

    if (flightCapId == 0 || flightCapId == FLIGHT_CAPABILITY_NORMAL)
    {
        _state = DragonridingState::IDLE;
        return;
    }

    // We're in dragonriding mode
    if (_state == DragonridingState::IDLE)
    {
        _state = DragonridingState::SOARING;
    }

    // Check current flight conditions
    float speed = GetCurrentSpeed();
    float altitude = GetCurrentAltitude();

    // Determine state based on speed and altitude
    if (_state == DragonridingState::BOOSTING || _state == DragonridingState::ASCENDING)
    {
        // Check if boost has ended
        if (_lastBoostTime > 2000) // Boost effects typically last ~2 seconds
        {
            _state = DragonridingState::SOARING;
        }
    }
    else if (altitude < LANDING_ALTITUDE && !_bot->IsFlying())
    {
        _state = DragonridingState::LANDING;
    }
    else if (_isDescending && altitude > MIN_ALTITUDE_FOR_DIVE)
    {
        _state = DragonridingState::DIVING;
    }
    else if (speed < SPEED_THRESHOLD_LOW)
    {
        _state = DragonridingState::GLIDING;
    }
    else
    {
        _state = DragonridingState::SOARING;
    }
}

void DragonridingAI::UpdateNavigation(uint32 /*diff*/)
{
    if (!_hasDestination || !_bot)
        return;

    float dist = GetDistanceToDestination();
    float altDiff = GetAltitudeToDestination();

    // Check if we've reached destination
    if (dist < 10.0f && std::abs(altDiff) < 5.0f)
    {
        TC_LOG_DEBUG("playerbot.dragonriding.ai", "Bot {} reached destination",
            _bot->GetName());
        ClearDestination();
        return;
    }

    // Determine if we need to ascend or descend
    _isDescending = (altDiff < -20.0f); // Need to go down more than 20 yards

    // Set target altitude
    _targetAltitude = _destination.GetPositionZ();
}

void DragonridingAI::UpdateAutoBoost(uint32 /*diff*/)
{
    if (!_autoBoostEnabled || !IsActive())
        return;

    // Don't boost if on cooldown
    if (_boostCooldown > 0)
        return;

    // Don't boost if we don't have vigor to spare
    if (!HasVigor())
        return;

    // Decide and execute boost
    AutoBoost();
}

BoostDecision DragonridingAI::DecideBoost() const
{
    // Emergency stop takes priority
    if (NeedsEmergencyStop())
        return BoostDecision::AERIAL_HALT;

    // Need altitude?
    if (ShouldBoostForAltitude())
        return BoostDecision::SKYWARD_ASCENT;

    // Need speed?
    if (ShouldBoostForSpeed())
        return BoostDecision::SURGE_FORWARD;

    return BoostDecision::NONE;
}

bool DragonridingAI::ShouldBoostForSpeed() const
{
    if (!_hasDestination)
        return false;

    float speed = GetCurrentSpeed();
    float dist = GetDistanceToDestination();

    // Boost if we're slow and far from destination
    if (speed < SPEED_THRESHOLD_LOW && dist > 100.0f)
        return true;

    // Boost if we're at medium speed and very far
    if (speed < SPEED_THRESHOLD_HIGH && dist > 500.0f)
        return true;

    return false;
}

bool DragonridingAI::ShouldBoostForAltitude() const
{
    if (!_hasDestination)
        return false;

    float altDiff = GetAltitudeToDestination();
    float currentAlt = GetCurrentAltitude();

    // Need to go up significantly
    if (altDiff > 50.0f)
        return true;

    // Low altitude and need to go up
    if (currentAlt < MIN_ALTITUDE_FOR_DIVE && altDiff > 20.0f)
        return true;

    return false;
}

bool DragonridingAI::NeedsEmergencyStop() const
{
    // Check if approaching obstacle
    if (IsApproachingObstacle())
        return true;

    // Check if approaching destination and going too fast
    if (_hasDestination)
    {
        float dist = GetDistanceToDestination();
        float speed = GetCurrentSpeed();

        if (dist < 50.0f && speed > SPEED_THRESHOLD_HIGH)
            return true;
    }

    return false;
}

float DragonridingAI::GetDistanceToDestination() const
{
    if (!_hasDestination || !_bot)
        return 0.0f;

    return _bot->GetDistance(_destination);
}

float DragonridingAI::GetAltitudeToDestination() const
{
    if (!_hasDestination || !_bot)
        return 0.0f;

    return _destination.GetPositionZ() - _bot->GetPositionZ();
}

float DragonridingAI::GetCurrentSpeed() const
{
    if (!_bot)
        return 0.0f;

    float maxSpeed = _bot->GetSpeedRate(MOVE_FLIGHT);
    float currentSpeed = _bot->GetSpeed(MOVE_FLIGHT);

    if (maxSpeed <= 0.0f)
        return 0.0f;

    return currentSpeed / maxSpeed;
}

float DragonridingAI::GetCurrentAltitude() const
{
    if (!_bot || !_bot->GetMap())
        return 0.0f;

    float groundZ = _bot->GetMap()->GetHeight(
        _bot->GetPhaseShift(),
        _bot->GetPositionX(),
        _bot->GetPositionY(),
        _bot->GetPositionZ());

    return _bot->GetPositionZ() - groundZ;
}

bool DragonridingAI::IsApproachingObstacle() const
{
    if (!_bot || !_bot->GetMap())
        return false;

    // Simple forward obstacle check
    float facing = _bot->GetOrientation();
    float checkX = _bot->GetPositionX() + std::cos(facing) * OBSTACLE_CHECK_RANGE;
    float checkY = _bot->GetPositionY() + std::sin(facing) * OBSTACLE_CHECK_RANGE;
    float checkZ = _bot->GetPositionZ();

    float groundZ = _bot->GetMap()->GetHeight(
        _bot->GetPhaseShift(),
        checkX, checkY, checkZ + 50.0f); // Check from above

    // Obstacle if ground is higher than our current position
    return groundZ > _bot->GetPositionZ() + 5.0f;
}

bool DragonridingAI::CanUseSurgeForward() const
{
    if (!_bot || _state == DragonridingState::IDLE)
        return false;

    if (_boostCooldown > 0)
        return false;

    if (!HasVigor())
        return false;

    if (_bot->GetSpellHistory()->HasCooldown(SPELL_SURGE_FORWARD))
        return false;

    return true;
}

bool DragonridingAI::CanUseSkywardAscent() const
{
    if (!_bot || _state == DragonridingState::IDLE)
        return false;

    if (_boostCooldown > 0)
        return false;

    if (!HasVigor())
        return false;

    if (_bot->GetSpellHistory()->HasCooldown(SPELL_SKYWARD_ASCENT))
        return false;

    return true;
}

bool DragonridingAI::CanUseWhirlingSurge() const
{
    if (!_bot || _state == DragonridingState::IDLE)
        return false;

    if (!_bot->GetSession())
        return false;

    uint32 accountId = _bot->GetSession()->GetAccountId();

    // Check if talent is learned
    if (!sDragonridingMgr->HasWhirlingSurge(accountId))
        return false;

    if (_boostCooldown > 0)
        return false;

    if (!HasVigor())
        return false;

    if (_bot->GetSpellHistory()->HasCooldown(SPELL_WHIRLING_SURGE))
        return false;

    return true;
}

bool DragonridingAI::CanUseAerialHalt() const
{
    if (!_bot || _state == DragonridingState::IDLE)
        return false;

    if (!_bot->GetSession())
        return false;

    uint32 accountId = _bot->GetSession()->GetAccountId();

    // Check if talent is learned
    if (!sDragonridingMgr->HasAerialHalt(accountId))
        return false;

    if (_bot->GetSpellHistory()->HasCooldown(SPELL_AERIAL_HALT))
        return false;

    return true;
}

// ============================================================================
// AI FACTORY
// ============================================================================

std::unique_ptr<DragonridingAI> DragonridingAIFactory::Create(Player* bot)
{
    if (!bot)
        return nullptr;

    // Only create AI for Dracthyr Evokers
    if (!CanUseSoar(bot->GetRace(), bot->GetClass()))
        return nullptr;

    return std::make_unique<DragonridingAI>(bot);
}

} // namespace Dragonriding
} // namespace Playerbot
