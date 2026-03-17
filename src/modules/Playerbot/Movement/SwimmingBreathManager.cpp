/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "SwimmingBreathManager.h"
#include "Player.h"
#include "SpellAuras.h"
#include "SpellAuraEffects.h"
#include "SpellHistory.h"
#include "Log.h"

namespace Playerbot
{

// ============================================================================
// CONSTRUCTOR
// ============================================================================

SwimmingBreathManager::SwimmingBreathManager(Player* bot)
    : _bot(bot)
{
    if (_bot)
        CheckForWaterBreathingAbilities();
}

// ============================================================================
// UPDATE
// ============================================================================

bool SwimmingBreathManager::Update(uint32 diff)
{
    if (!_bot || !_bot->IsInWorld() || !_bot->IsAlive())
        return false;

    _checkTimer += diff;
    if (_checkTimer < CHECK_INTERVAL_MS)
        return false;
    _checkTimer = 0;

    // Update water state
    WaterState previousState = _waterState;
    UpdateWaterState();

    // Log state transitions
    if (_waterState != previousState)
    {
        TC_LOG_DEBUG("module.playerbot", "SwimmingBreathManager [{}]: State {} -> {}",
            _bot->GetName(), static_cast<uint32>(previousState),
            GetWaterStateString());
    }

    // Track underwater time
    if (_waterState == WaterState::UNDERWATER || _waterState == WaterState::SURFACING)
    {
        _underwaterTimer += CHECK_INTERVAL_MS;
        _stats.totalUnderwaterTimeMs += CHECK_INTERVAL_MS;
    }
    else
    {
        _underwaterTimer = 0;
    }

    // Update breath tracking
    UpdateBreathTracking(diff);

    // Determine if we need to surface
    _needsSurfacing = false;

    if (_waterState == WaterState::UNDERWATER && !_hasWaterBreathing)
    {
        if (_breathPercent <= CRITICAL_THRESHOLD)
        {
            // Emergency - must surface immediately
            _needsSurfacing = true;
            _waterState = WaterState::SURFACING;
            _stats.nearDrowningEvents++;

            TC_LOG_WARN("module.playerbot", "SwimmingBreathManager [{}]: CRITICAL breath "
                "({:.1f}%), emergency surfacing!",
                _bot->GetName(), _breathPercent);
        }
        else if (_breathPercent <= SURFACE_THRESHOLD)
        {
            // Should start heading to surface
            _needsSurfacing = true;
            _waterState = WaterState::SURFACING;
            _stats.surfacingEvents++;

            TC_LOG_DEBUG("module.playerbot", "SwimmingBreathManager [{}]: Breath at {:.1f}%, "
                "surfacing for air",
                _bot->GetName(), _breathPercent);
        }
    }
    else if (_waterState == WaterState::SURFACING)
    {
        // Continue surfacing until we have enough breath
        if (_breathPercent < SAFE_THRESHOLD && !_hasWaterBreathing)
        {
            _needsSurfacing = true;
        }
        else
        {
            // We have enough air, back to underwater/swimming
            _waterState = _bot->IsUnderWater() ? WaterState::UNDERWATER : WaterState::SWIMMING;
            _needsSurfacing = false;
        }
    }

    // Try to use water breathing if available and underwater
    if (_waterState == WaterState::UNDERWATER && !_hasWaterBreathing)
    {
        TryUseWaterBreathing();
    }

    return _needsSurfacing;
}

// ============================================================================
// QUERIES
// ============================================================================

::std::string SwimmingBreathManager::GetWaterStateString() const
{
    switch (_waterState)
    {
        case WaterState::DRY:        return "DRY";
        case WaterState::WADING:     return "WADING";
        case WaterState::SWIMMING:   return "SWIMMING";
        case WaterState::UNDERWATER: return "UNDERWATER";
        case WaterState::SURFACING:  return "SURFACING";
        case WaterState::DROWNING:   return "DROWNING";
        default: return "UNKNOWN";
    }
}

bool SwimmingBreathManager::IsUnderwater() const
{
    return _waterState == WaterState::UNDERWATER ||
           _waterState == WaterState::SURFACING ||
           _waterState == WaterState::DROWNING;
}

bool SwimmingBreathManager::IsSwimming() const
{
    return _waterState != WaterState::DRY && _waterState != WaterState::WADING;
}

bool SwimmingBreathManager::HasWaterBreathing() const
{
    if (!_bot)
        return false;

    // Check for water breathing aura
    if (_bot->HasAuraType(SPELL_AURA_WATER_BREATHING))
        return true;

    return false;
}

bool SwimmingBreathManager::CanUseAquaticForm() const
{
    if (!_bot)
        return false;

    // Only druids have aquatic form
    if (_bot->GetClass() != CLASS_DRUID)
        return false;

    // Check if they know the spell and it's not on cooldown
    if (_bot->HasSpell(SPELL_AQUATIC_FORM) &&
        !_bot->GetSpellHistory()->HasCooldown(SPELL_AQUATIC_FORM))
        return true;

    return false;
}

// ============================================================================
// ACTIONS
// ============================================================================

bool SwimmingBreathManager::TryUseWaterBreathing()
{
    if (!_bot || !_bot->IsInWorld() || !_bot->IsAlive())
        return false;

    // Already have water breathing
    if (HasWaterBreathing())
    {
        _hasWaterBreathing = true;
        return true;
    }

    // Try Druid Aquatic Form first
    if (CanUseAquaticForm())
    {
        // Note: Actual spell casting would be done by the rotation/AI system
        // We just signal that this is available
        _stats.waterBreathingUsed++;
        TC_LOG_DEBUG("module.playerbot", "SwimmingBreathManager [{}]: Aquatic Form available, "
            "should use it", _bot->GetName());
        return true;
    }

    // Check if warlock in group could cast Unending Breath
    // (This is informational - actual casting coordination is handled elsewhere)

    return false;
}

bool SwimmingBreathManager::TryUseAquaticForm()
{
    if (!CanUseAquaticForm())
        return false;

    // Signal to AI that aquatic form should be used
    TC_LOG_DEBUG("module.playerbot", "SwimmingBreathManager [{}]: Requesting Aquatic Form",
        _bot->GetName());
    return true;
}

// ============================================================================
// INTERNAL
// ============================================================================

void SwimmingBreathManager::UpdateWaterState()
{
    if (!_bot)
        return;

    bool inWater = _bot->IsInWater();
    bool underWater = _bot->IsUnderWater();

    if (!inWater)
    {
        _waterState = WaterState::DRY;
        _hasWaterBreathing = false;
        _breathPercent = 100.0f;
    }
    else if (underWater)
    {
        if (_waterState != WaterState::SURFACING)
            _waterState = WaterState::UNDERWATER;
        // Check for water breathing
        _hasWaterBreathing = HasWaterBreathing();
    }
    else
    {
        // In water but not under - either wading or swimming on surface
        // Swimming is determined by movement flags, but for simplicity
        // if we're in water and the water is deep enough to submerge, we're swimming
        _waterState = WaterState::SWIMMING;
        _hasWaterBreathing = false;
        _breathPercent = 100.0f;  // Surface = full breath
    }
}

void SwimmingBreathManager::UpdateBreathTracking(uint32 /*diff*/)
{
    if (!_bot)
        return;

    if (_waterState != WaterState::UNDERWATER &&
        _waterState != WaterState::SURFACING &&
        _waterState != WaterState::DROWNING)
    {
        _breathPercent = 100.0f;
        return;
    }

    // If we have water breathing, breath doesn't decrease
    if (_hasWaterBreathing)
    {
        _breathPercent = 100.0f;
        return;
    }

    // Use the mirror timer if available
    // m_MirrorTimer[BREATH_TIMER] counts down from max to 0
    // We can access it through IsMirrorTimerActive check
    if (_bot->IsMirrorTimerActive(BREATH_TIMER))
    {
        // The mirror timer is active, meaning breath is being tracked
        // We estimate breath remaining based on underwater time
        // Default breath is 180 seconds (3 minutes)
        float maxBreathMs = static_cast<float>(MAX_BREATH_DURATION_MS);
        float elapsedMs = static_cast<float>(_underwaterTimer);
        _breathPercent = std::max(0.0f, (1.0f - (elapsedMs / maxBreathMs)) * 100.0f);

        if (_breathPercent <= 0.0f)
        {
            _waterState = WaterState::DROWNING;
            _stats.drowningDamageTaken++;
        }
    }
    else
    {
        // Mirror timer not active yet (just entered water) or has water breathing
        // Estimate based on underwater time
        if (_underwaterTimer > 0)
        {
            float maxBreathMs = static_cast<float>(MAX_BREATH_DURATION_MS);
            float elapsedMs = static_cast<float>(_underwaterTimer);
            _breathPercent = std::max(0.0f, (1.0f - (elapsedMs / maxBreathMs)) * 100.0f);
        }
        else
        {
            _breathPercent = 100.0f;
        }
    }
}

void SwimmingBreathManager::CheckForWaterBreathingAbilities()
{
    if (!_bot)
        return;

    uint8 botClass = _bot->GetClass();

    // Druids can use Aquatic Form
    if (botClass == CLASS_DRUID)
    {
        _canAquaticForm = _bot->HasSpell(SPELL_AQUATIC_FORM);
    }

    // Warlocks have Unending Breath
    if (botClass == CLASS_WARLOCK)
    {
        _waterBreathingSpellId = SPELL_UNENDING_BREATH;
    }

    // Shamans can use Water Walking (not exactly water breathing but related)
    // Undead have passive water breathing

    TC_LOG_DEBUG("module.playerbot", "SwimmingBreathManager [{}]: "
        "aquaticForm={}, waterBreathSpell={}",
        _bot->GetName(), _canAquaticForm, _waterBreathingSpellId);
}

} // namespace Playerbot
