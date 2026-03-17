/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * SWIMMING & BREATH MANAGEMENT
 *
 * Manages bot behavior when in water, tracking breath timer and
 * initiating surface-seeking when running low on air. Bots should
 * behave like real players underwater:
 *
 *   - Track breath timer (BREATH_TIMER mirror timer)
 *   - Surface for air when breath gets low
 *   - Use water breathing abilities/items if available
 *   - Avoid drowning by surfacing proactively
 *   - Handle underwater combat (limited movement)
 *   - Use aquatic form (Druids) when appropriate
 *
 * Architecture:
 *   - Per-bot instance updated during movement/AI tick
 *   - Monitors Player::IsUnderWater() and mirror timer
 *   - Issues movement commands to seek surface when breath is low
 *   - Tracks water breathing buffs to suppress surfacing
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include <string>

class Player;

namespace Playerbot
{

// ============================================================================
// WATER STATE
// ============================================================================

enum class WaterState : uint8
{
    DRY             = 0,    // Not in water
    WADING          = 1,    // Feet in water but not swimming
    SWIMMING        = 2,    // Swimming on surface
    UNDERWATER      = 3,    // Fully submerged
    SURFACING       = 4,    // Actively moving to surface for air
    DROWNING        = 5     // Breath expired, taking damage
};

// ============================================================================
// SWIMMING STATISTICS
// ============================================================================

struct SwimmingStats
{
    uint32 totalUnderwaterTimeMs{0};
    uint32 surfacingEvents{0};
    uint32 waterBreathingUsed{0};
    uint32 nearDrowningEvents{0};    // Got below 25% breath
    uint32 drowningDamageTaken{0};
};

// ============================================================================
// SWIMMING & BREATH MANAGER
// ============================================================================

class SwimmingBreathManager
{
public:
    explicit SwimmingBreathManager(Player* bot);
    ~SwimmingBreathManager() = default;

    // ========================================================================
    // UPDATE
    // ========================================================================

    /// Main update - called every AI tick
    /// Returns true if the bot needs to surface for air (caller should
    /// override current movement to swim upward)
    bool Update(uint32 diff);

    // ========================================================================
    // QUERIES
    // ========================================================================

    /// Get current water state
    WaterState GetWaterState() const { return _waterState; }

    /// Get water state as string
    ::std::string GetWaterStateString() const;

    /// Is the bot currently underwater?
    bool IsUnderwater() const;

    /// Is the bot swimming (surface or underwater)?
    bool IsSwimming() const;

    /// Does the bot need to surface for air?
    bool NeedsSurfacing() const { return _needsSurfacing; }

    /// Get estimated breath remaining as percentage (0-100)
    float GetBreathPercent() const { return _breathPercent; }

    /// Does the bot have water breathing (buff/item/racial)?
    bool HasWaterBreathing() const;

    /// Can the bot use aquatic form? (Druid)
    bool CanUseAquaticForm() const;

    // ========================================================================
    // ACTIONS
    // ========================================================================

    /// Try to use water breathing ability if available
    bool TryUseWaterBreathing();

    /// Try to use aquatic form (Druid Travel Form with glyph)
    bool TryUseAquaticForm();

    // ========================================================================
    // STATISTICS
    // ========================================================================

    SwimmingStats const& GetStats() const { return _stats; }
    void ResetStats() { _stats = SwimmingStats{}; }

private:
    // ========================================================================
    // INTERNAL
    // ========================================================================

    void UpdateWaterState();
    void UpdateBreathTracking(uint32 diff);
    void CheckForWaterBreathingAbilities();

    // ========================================================================
    // MEMBER VARIABLES
    // ========================================================================

    Player* _bot;
    WaterState _waterState{WaterState::DRY};
    bool _needsSurfacing{false};
    float _breathPercent{100.0f};
    bool _hasWaterBreathing{false};

    // Ability tracking
    uint32 _waterBreathingSpellId{0};    // Cached water breathing spell
    bool _canAquaticForm{false};

    // Statistics
    SwimmingStats _stats;

    // Timers
    uint32 _checkTimer{0};
    uint32 _underwaterTimer{0};

    // Thresholds
    static constexpr uint32 CHECK_INTERVAL_MS = 1000;        // 1s
    static constexpr float SURFACE_THRESHOLD = 30.0f;        // Surface at 30% breath
    static constexpr float CRITICAL_THRESHOLD = 15.0f;       // Emergency surface at 15%
    static constexpr float SAFE_THRESHOLD = 80.0f;           // Safe to submerge above 80%
    static constexpr uint32 MAX_BREATH_DURATION_MS = 180000; // 3 minutes default

    // Water breathing spell IDs
    static constexpr uint32 SPELL_WATER_BREATHING = 131;     // Generic Water Breathing
    static constexpr uint32 SPELL_UNENDING_BREATH = 5697;    // Warlock Unending Breath
    static constexpr uint32 SPELL_AQUATIC_FORM = 783;        // Druid Aquatic Form
    static constexpr uint32 SPELL_TRAVEL_FORM = 783;         // Druid Travel Form (aquatic variant)
    static constexpr uint32 SPELL_DARKFLIGHT_WATER = 0;      // Placeholder
};

} // namespace Playerbot
