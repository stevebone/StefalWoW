/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "DragonridingDefines.h"
#include "Define.h"
#include "Position.h"
#include <memory>

class Player;

namespace Playerbot
{
namespace Dragonriding
{

// Forward declarations
class DragonridingMgr;

// ============================================================================
// DRAGONRIDING STATE
// Tracks the current dragonriding state for a bot
// ============================================================================

enum class DragonridingState : uint8
{
    IDLE = 0,           // Not dragonriding
    SOARING,            // Actively dragonriding
    BOOSTING,           // Using boost ability
    ASCENDING,          // Using Skyward Ascent
    GLIDING,            // Conserving momentum
    DIVING,             // Diving for speed
    LANDING             // Preparing to land
};

// ============================================================================
// BOOST DECISION
// AI decision for which boost ability to use
// ============================================================================

enum class BoostDecision : uint8
{
    NONE = 0,           // No boost needed
    SURGE_FORWARD,      // Need horizontal speed
    SKYWARD_ASCENT,     // Need vertical lift
    WHIRLING_SURGE,     // Barrel roll for combat/evasion
    AERIAL_HALT         // Need to stop/hover
};

// ============================================================================
// DRAGONRIDING AI
// Manages bot dragonriding behavior
// ============================================================================

class TC_GAME_API DragonridingAI
{
public:
    explicit DragonridingAI(Player* bot);
    ~DragonridingAI();

    DragonridingAI(DragonridingAI const&) = delete;
    DragonridingAI& operator=(DragonridingAI const&) = delete;

    // ========================================================================
    // MAIN UPDATE
    // Called every AI tick to update dragonriding behavior
    // ========================================================================

    void Update(uint32 diff);

    // ========================================================================
    // STATE QUERIES
    // ========================================================================

    bool IsActive() const { return _state != DragonridingState::IDLE; }
    DragonridingState GetState() const { return _state; }
    uint32 GetCurrentVigor() const;
    uint32 GetMaxVigor() const;
    bool HasVigor() const;

    // ========================================================================
    // CONTROL INTERFACE
    // Called by bot AI to control dragonriding
    // ========================================================================

    // Start dragonriding (cast Soar)
    bool StartSoaring();

    // Stop dragonriding (cancel Soar)
    bool StopSoaring();

    // Set destination for navigation
    void SetDestination(Position const& dest);
    void ClearDestination();
    bool HasDestination() const { return _hasDestination; }
    Position const& GetDestination() const { return _destination; }

    // ========================================================================
    // ABILITY USAGE
    // ========================================================================

    // Use boost abilities (returns true if ability was used)
    bool UseSurgeForward();
    bool UseSkywardAscent();
    bool UseWhirlingSurge();
    bool UseAerialHalt();

    // Auto-boost (AI decides which ability to use)
    bool AutoBoost();

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    // Enable/disable auto-boost
    void SetAutoBoostEnabled(bool enabled) { _autoBoostEnabled = enabled; }
    bool IsAutoBoostEnabled() const { return _autoBoostEnabled; }

    // Set minimum vigor reserve (won't boost below this)
    void SetMinVigorReserve(uint32 reserve) { _minVigorReserve = reserve; }
    uint32 GetMinVigorReserve() const { return _minVigorReserve; }

private:
    // ========================================================================
    // INTERNAL METHODS
    // ========================================================================

    void UpdateState();
    void UpdateNavigation(uint32 diff);
    void UpdateAutoBoost(uint32 diff);

    // Decision making
    BoostDecision DecideBoost() const;
    bool ShouldBoostForSpeed() const;
    bool ShouldBoostForAltitude() const;
    bool NeedsEmergencyStop() const;

    // Navigation helpers
    float GetDistanceToDestination() const;
    float GetAltitudeToDestination() const;
    float GetCurrentSpeed() const;
    float GetCurrentAltitude() const;
    bool IsApproachingObstacle() const;

    // Ability checks
    bool CanUseSurgeForward() const;
    bool CanUseSkywardAscent() const;
    bool CanUseWhirlingSurge() const;
    bool CanUseAerialHalt() const;

    // ========================================================================
    // MEMBER VARIABLES
    // ========================================================================

    Player* _bot;                           // The bot player
    DragonridingState _state;               // Current dragonriding state
    Position _destination;                   // Target destination
    bool _hasDestination;                    // Whether we have a destination set

    // Configuration
    bool _autoBoostEnabled;                  // Whether auto-boost is enabled
    uint32 _minVigorReserve;                 // Minimum vigor to keep in reserve

    // Timing
    uint32 _updateTimer;                     // Time since last update
    uint32 _boostCooldown;                   // Cooldown between boosts
    uint32 _lastBoostTime;                   // Time since last boost

    // Navigation
    float _targetAltitude;                   // Desired altitude
    float _desiredSpeed;                     // Desired speed
    bool _isDescending;                      // Currently descending

    // Constants
    static constexpr uint32 UPDATE_INTERVAL_MS = 250;       // AI update interval
    static constexpr uint32 BOOST_COOLDOWN_MS = 1000;       // Minimum time between boosts
    static constexpr float MIN_ALTITUDE_FOR_DIVE = 100.0f;  // Minimum altitude to start diving
    static constexpr float LANDING_ALTITUDE = 20.0f;        // Start landing procedure
    static constexpr float SPEED_THRESHOLD_LOW = 0.5f;      // Low speed threshold
    static constexpr float SPEED_THRESHOLD_HIGH = 0.9f;     // High speed threshold
    static constexpr float OBSTACLE_CHECK_RANGE = 50.0f;    // Range to check for obstacles
};

// ============================================================================
// AI FACTORY
// Creates DragonridingAI instances for bots
// ============================================================================

class TC_GAME_API DragonridingAIFactory
{
public:
    static std::unique_ptr<DragonridingAI> Create(Player* bot);
};

} // namespace Dragonriding
} // namespace Playerbot
