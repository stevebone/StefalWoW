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
#include <cstdint>

namespace Playerbot
{
namespace Dragonriding
{

// ============================================================================
// RETAIL SPELL IDS (from wowhead.com - WoW 12.0.7)
// ============================================================================
// Using REAL retail spell IDs ensures:
// - Client already has icons, names, tooltips, animations
// - No hotfix data needed for visual display
// - Spells are recognized by the client's spell system
// ============================================================================

// -----------------------------------------------------------------------------
// DRACTHYR RACIAL ABILITIES
// -----------------------------------------------------------------------------
constexpr uint32 SPELL_SOAR = 369536;           // Dracthyr Evoker racial - activates dragonriding
constexpr uint32 SPELL_GLIDE = 358733;          // Evoker glide ability (shares cooldown with Soar)

// -----------------------------------------------------------------------------
// DRAGONRIDING ACTION BAR ABILITIES (RETAIL IDs)
// These appear on the action bar during dragonriding/Soar
// Client already has full spell data: icons, tooltips, animations
// Source: wowhead.com spell database
// -----------------------------------------------------------------------------
constexpr uint32 SPELL_SURGE_FORWARD = 372608;  // "Flap forward" - 6 charges, 15s recharge
                                                 // Icon: ability_dragonriding_forwardflap01 (4640490)

constexpr uint32 SPELL_SKYWARD_ASCENT = 372610; // "Flap upward" - 6 charges, 15s recharge
                                                 // Icon: ability_dragonriding_upwardflap01 (4640498)

constexpr uint32 SPELL_WHIRLING_SURGE = 361584; // "Spiral forward" - 30s cooldown
                                                 // Icon: ability_dragonriding_barrelroll01 (4640477)
                                                 // Requires: Airborne Tumbling talent

constexpr uint32 SPELL_AERIAL_HALT = 403092;    // "Flap back, reduce forward movement" - 10s cooldown
                                                 // Icon: ability_dragonriding_haltthewinds01 (5003205)
                                                 // Requires: At Home Aloft talent

// -----------------------------------------------------------------------------
// DRAGONRIDING SYSTEM SPELLS (RETAIL IDs)
// These are internal system spells used by the dragonriding mechanics
// -----------------------------------------------------------------------------
constexpr uint32 SPELL_DRAGONRIDING = 376027;   // Base dragonriding unlock spell
constexpr uint32 SPELL_VIGOR = 383359;          // Skyriding charges resource (formerly "Vigor")
constexpr uint32 SPELL_THRILL_OF_THE_SKIES = 383366; // High-speed vigor regeneration buff
constexpr uint32 SPELL_FLIGHT_STYLE_SKYRIDING = 404464; // "Skyriding is currently enabled" - ENABLES ABILITIES
constexpr uint32 SPELL_FLIGHT_STYLE_STEADY = 404468;    // "Steady Flight is currently enabled" - MUST REMOVE for Soar

// -----------------------------------------------------------------------------
// CRITICAL: DRAGONRIDER ENERGY (372773)
// This aura is REQUIRED by Surge Forward (372608) and Skyward Ascent (372610)
// Both abilities have CasterAuraSpell = 372773 in their SpellInfo
// Without this aura, abilities show "You can't do that yet"
// This aura also enables the Alt Power bar (vigor UI display)
// -----------------------------------------------------------------------------
constexpr uint32 SPELL_DRAGONRIDER_ENERGY = 372773; // "Dragonrider Energy" - enables vigor UI + ability casting

// -----------------------------------------------------------------------------
// CUSTOM INTERNAL SPELLS (900000+ range)
// These are server-side only tracking spells that don't need client display
// Used for internal state management where no retail equivalent exists
// -----------------------------------------------------------------------------
constexpr uint32 SPELL_VIGOR_TRACKING = 900001;      // Internal vigor stack tracking (if needed)
constexpr uint32 SPELL_GROUND_SKIMMING_BUFF = 900002; // Near-ground regen visual (no retail equivalent)

// ============================================================================
// ACTION BAR OVERRIDE CONFIGURATION
// ============================================================================

// OverrideSpellData ID for the Soar action bar
// This ID references the override_spell_data table in hotfixes database
// The entry maps to retail ability spell IDs (372608, 372610, 361584, 403092)
constexpr uint32 OVERRIDE_SPELL_DATA_SOAR = 900001;

// Deprecated - keeping for backwards compatibility during transition
constexpr uint32 OVERRIDE_SPELL_DATA_DRAGONRIDING = OVERRIDE_SPELL_DATA_SOAR;

// ============================================================================
// FLIGHT CAPABILITY IDS
// From FlightCapability.db2 - these activate dragonriding physics
// ============================================================================

constexpr uint32 FLIGHT_CAPABILITY_SOAR = 1;          // Standard dragonriding physics
constexpr uint32 FLIGHT_CAPABILITY_NORMAL = 0;        // Normal flying (no dragonriding)

// ============================================================================
// RETAIL-ACCURATE BASE VALUES
// These match WoW 12.0.7 dragonriding mechanics
// ============================================================================

// Vigor/Charges (now called "Skyriding Charges" in 12.0.7)
constexpr uint32 BASE_MAX_VIGOR = 6;                  // All players start with 6 charges in 12.0.7
constexpr uint32 MAX_MAX_VIGOR = 6;                   // Maximum (talents removed in 12.0.7)

// Regeneration rates (milliseconds per charge)
// In 12.0.7, abilities have their own recharge timers:
// - Surge Forward/Skyward Ascent: 15 second recharge
// - Whirling Surge: 30 second cooldown
// - Aerial Halt: 10 second cooldown
constexpr uint32 BASE_REGEN_GROUNDED_MS = 30000;      // 30 seconds while grounded (no talents)
constexpr uint32 BASE_REGEN_FLYING_MS = 15000;        // 15 seconds while flying fast (Thrill)
constexpr uint32 BASE_REGEN_GROUND_SKIM_MS = 30000;   // 30 seconds near ground

// Upgraded regeneration rates (with talents or progression disabled)
constexpr uint32 UPGRADED_REGEN_GROUNDED_MS = 15000;  // 15 seconds (Yearning for the Sky talent)
constexpr uint32 UPGRADED_REGEN_FLYING_MS = 5000;     // 5 seconds (Thrill Seeker talent)

// ============================================================================
// TALENT IDS (Legacy - talents simplified in 12.0.7)
// Keeping for potential backwards compatibility
// ============================================================================

enum class DragonridingTalentId : uint32
{
    NONE = 0,

    // Vigor Capacity Branch (removed in 12.0.7 - all have 6 charges)
    TAKE_TO_THE_SKIES = 1,
    DRAGONRIDING_LEARNER = 2,
    BEYOND_INFINITY = 3,

    // Vigor Regen Branch (simplified in 12.0.7)
    DYNAMIC_STRETCHING = 10,
    RESTORATIVE_TRAVELS = 11,
    YEARNING_FOR_THE_SKY = 12,

    // Flying regen branch
    THRILL_CHASER = 20,
    THRILL_SEEKER = 21,

    // Utility & Abilities
    GROUND_SKIMMING = 30,
    AIRBORNE_TUMBLING = 31,      // Unlocks Whirling Surge
    AT_HOME_ALOFT = 32,          // Unlocks Aerial Halt

    MAX_TALENT = 33
};

// ============================================================================
// TALENT COSTS (Dragon Glyphs Required) - Legacy
// ============================================================================

struct TalentCost
{
    DragonridingTalentId talentId;
    uint32 glyphCost;
    DragonridingTalentId prerequisite;
    uint32 effectValue;
};

constexpr TalentCost TALENT_COSTS[] = {
    { DragonridingTalentId::TAKE_TO_THE_SKIES, 1, DragonridingTalentId::NONE, 4 },
    { DragonridingTalentId::DRAGONRIDING_LEARNER, 4, DragonridingTalentId::TAKE_TO_THE_SKIES, 5 },
    { DragonridingTalentId::BEYOND_INFINITY, 5, DragonridingTalentId::DRAGONRIDING_LEARNER, 6 },
    { DragonridingTalentId::DYNAMIC_STRETCHING, 3, DragonridingTalentId::NONE, 25000 },
    { DragonridingTalentId::RESTORATIVE_TRAVELS, 4, DragonridingTalentId::DYNAMIC_STRETCHING, 20000 },
    { DragonridingTalentId::YEARNING_FOR_THE_SKY, 5, DragonridingTalentId::RESTORATIVE_TRAVELS, 15000 },
    { DragonridingTalentId::THRILL_CHASER, 3, DragonridingTalentId::NONE, 10000 },
    { DragonridingTalentId::THRILL_SEEKER, 5, DragonridingTalentId::THRILL_CHASER, 5000 },
    { DragonridingTalentId::GROUND_SKIMMING, 4, DragonridingTalentId::NONE, 30000 },
    { DragonridingTalentId::AIRBORNE_TUMBLING, 3, DragonridingTalentId::NONE, SPELL_WHIRLING_SURGE },
    { DragonridingTalentId::AT_HOME_ALOFT, 2, DragonridingTalentId::NONE, SPELL_AERIAL_HALT },
};

constexpr size_t TALENT_COUNT = sizeof(TALENT_COSTS) / sizeof(TalentCost);

// ============================================================================
// ZONE CONFIGURATION
// ============================================================================

constexpr uint32 MAP_DRAGON_ISLES = 2444;
constexpr uint32 ZONE_WAKING_SHORES = 13644;
constexpr uint32 ZONE_OHNAHRAN_PLAINS = 13645;
constexpr uint32 ZONE_AZURE_SPAN = 13646;
constexpr uint32 ZONE_THALDRASZUS = 13647;
constexpr uint32 ZONE_FORBIDDEN_REACH = 14022;
constexpr uint32 ZONE_ZARALEK_CAVERN = 14529;

constexpr uint32 TOTAL_GLYPHS = 74;
constexpr float GLYPH_COLLECTION_RADIUS = 10.0f;

// ============================================================================
// PHYSICS THRESHOLDS
// ============================================================================

constexpr float THRILL_SPEED_THRESHOLD = 0.70f;  // 70% of max velocity triggers Thrill
constexpr float GROUND_SKIM_HEIGHT = 10.0f;       // Yards above ground for Ground Skimming

// ============================================================================
// UPDATE INTERVALS
// ============================================================================

constexpr uint32 VIGOR_UPDATE_INTERVAL_MS = 1000;
constexpr uint32 GLYPH_CHECK_INTERVAL_MS = 500;

// ============================================================================
// CONFIG KEYS (worldserver.conf)
// ============================================================================

constexpr const char* CONFIG_DRAGONRIDING_ENABLED = "Playerbot.GameSystems.Dragonriding.Enable";
constexpr const char* CONFIG_PROGRESSION_ENABLED = "Playerbot.GameSystems.Dragonriding.ProgressionEnabled";
constexpr const char* CONFIG_STARTING_GLYPHS = "Playerbot.GameSystems.Dragonriding.StartingGlyphs";
constexpr const char* CONFIG_BOT_AUTO_BOOST = "Playerbot.GameSystems.Dragonriding.Bot.AutoBoost";
constexpr const char* CONFIG_THRILL_SPEED_THRESHOLD = "Playerbot.GameSystems.Dragonriding.ThrillSpeedThreshold";
constexpr const char* CONFIG_GROUND_SKIM_HEIGHT = "Playerbot.GameSystems.Dragonriding.GroundSkimHeight";
constexpr const char* CONFIG_UPDATE_INTERVAL = "Playerbot.GameSystems.Dragonriding.UpdateInterval";
constexpr const char* CONFIG_GLYPH_CHECK_INTERVAL = "Playerbot.GameSystems.Dragonriding.GlyphCheckInterval";

// ============================================================================
// RACE/CLASS RESTRICTIONS
// ============================================================================

constexpr uint32 RACE_DRACTHYR_ALLIANCE = 52;
constexpr uint32 RACE_DRACTHYR_HORDE = 70;
constexpr uint32 CLASS_EVOKER = 13;

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

inline bool IsDracthyr(uint8 race)
{
    return race == RACE_DRACTHYR_ALLIANCE || race == RACE_DRACTHYR_HORDE;
}

inline bool IsEvoker(uint8 playerClass)
{
    return playerClass == CLASS_EVOKER;
}

inline bool CanUseSoar(uint8 race, uint8 playerClass)
{
    return IsDracthyr(race) && IsEvoker(playerClass);
}

inline const TalentCost* GetTalentCost(DragonridingTalentId talentId)
{
    for (size_t i = 0; i < TALENT_COUNT; ++i)
    {
        if (TALENT_COSTS[i].talentId == talentId)
            return &TALENT_COSTS[i];
    }
    return nullptr;
}

} // namespace Dragonriding
} // namespace Playerbot
