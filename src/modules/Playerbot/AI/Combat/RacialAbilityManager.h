/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * RACIAL ABILITY MANAGER
 *
 * Manages intelligent usage of racial abilities for bots based on race,
 * combat context, and situational awareness. Each race in WoW has unique
 * active abilities that should be used at optimal times:
 *
 *   - Offensive racials during burst windows (Blood Fury, Berserking, etc.)
 *   - Defensive racials when health is low (Stoneform, Gift of the Naaru, etc.)
 *   - CC-breaking racials when crowd controlled (Will of the Forsaken, Every Man, etc.)
 *   - Utility racials for movement/stealth (Shadowmeld, Darkflight, etc.)
 *
 * Architecture:
 *   - Static database of racial abilities by race
 *   - Per-bot instance evaluates usage based on combat state
 *   - Integrates with CooldownStackingOptimizer for burst alignment
 *   - Uses SpellMgr for spell availability validation
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <array>

class Player;
class Unit;

namespace Playerbot
{

// ============================================================================
// RACIAL ABILITY CATEGORIES
// ============================================================================

enum class RacialCategory : uint8
{
    OFFENSIVE       = 0,    // DPS boost (Blood Fury, Berserking, Fireblood)
    DEFENSIVE       = 1,    // Damage reduction / self-heal (Stoneform, Gift of the Naaru)
    CC_BREAK        = 2,    // Crowd control removal (Will of the Forsaken, Every Man)
    UTILITY         = 3,    // Movement / stealth (Shadowmeld, Darkflight)
    RESOURCE        = 4,    // Resource restoration (Arcane Torrent)
    AOE_CC          = 5,    // AoE crowd control (War Stomp, Quaking Palm)
    CATEGORY_COUNT  = 6
};

// ============================================================================
// RACIAL ABILITY INFO
// ============================================================================

struct RacialAbilityInfo
{
    uint32 spellId{0};
    uint32 raceId{0};              // RACE_* enum value
    ::std::string name;
    RacialCategory category{RacialCategory::OFFENSIVE};
    float healthThreshold{0.0f};   // Use when HP below this % (for defensive)
    bool useDuringBurst{false};    // Align with burst windows
    bool useOnCooldown{false};     // Use whenever available
    bool requiresCombat{true};     // Must be in combat to use
    bool pvpOnly{false};           // Only useful in PvP
    uint32 cooldownMs{0};          // Base cooldown in ms
};

// ============================================================================
// RACIAL ABILITY MANAGER
// ============================================================================

class RacialAbilityManager
{
public:
    explicit RacialAbilityManager(Player* bot);
    ~RacialAbilityManager() = default;

    // ========================================================================
    // INITIALIZATION
    // ========================================================================

    /// Initialize racial abilities for this bot's race
    void Initialize();

    /// Check if initialization has been done
    bool IsInitialized() const { return _initialized; }

    // ========================================================================
    // UPDATE / EVALUATION
    // ========================================================================

    /// Main update - evaluates and uses racials as appropriate
    /// Returns spell ID to cast, or 0 if no racial should be used
    uint32 EvaluateRacials();

    /// Check if any offensive racial should be used now (for burst alignment)
    uint32 GetOffensiveRacial() const;

    /// Check if any defensive racial should be used now
    uint32 GetDefensiveRacial() const;

    /// Check if a CC-break racial should be used
    uint32 GetCCBreakRacial() const;

    /// Check if a resource racial should be used
    uint32 GetResourceRacial() const;

    /// Check if an AoE CC racial should be used
    uint32 GetAoECCRacial() const;

    // ========================================================================
    // QUERIES
    // ========================================================================

    /// Get all racial abilities for this bot
    ::std::vector<RacialAbilityInfo> const& GetRacials() const { return _racials; }

    /// Does this bot have an offensive racial?
    bool HasOffensiveRacial() const;

    /// Does this bot have a defensive racial?
    bool HasDefensiveRacial() const;

    /// Does this bot have a CC break racial?
    bool HasCCBreakRacial() const;

    /// Get the name of the bot's race for logging
    ::std::string GetRaceName() const;

    // ========================================================================
    // STATISTICS
    // ========================================================================

    struct RacialStats
    {
        uint32 totalUsed{0};
        uint32 offensiveUsed{0};
        uint32 defensiveUsed{0};
        uint32 ccBreakUsed{0};
        uint32 utilityUsed{0};
    };

    RacialStats const& GetStats() const { return _stats; }

private:
    // ========================================================================
    // INTERNAL
    // ========================================================================

    void LoadRacialsForRace(uint8 raceId);
    bool CanUseSpell(uint32 spellId) const;
    bool IsSpellReady(uint32 spellId) const;
    bool IsCrowdControlled() const;
    bool IsInBurstWindow() const;

    // ========================================================================
    // STATIC DATABASE
    // ========================================================================

    static ::std::vector<RacialAbilityInfo> const& GetRacialDatabase();

    // ========================================================================
    // MEMBER VARIABLES
    // ========================================================================

    Player* _bot;
    bool _initialized{false};
    ::std::vector<RacialAbilityInfo> _racials;  // This bot's available racials
    RacialStats _stats;

    // Cooldown tracking (to avoid spamming HasSpellCooldown)
    ::std::unordered_map<uint32, uint32> _lastUsedTime;
    static constexpr uint32 MIN_EVAL_INTERVAL = 500;  // Don't evaluate more than every 500ms
    uint32 _lastEvalTime{0};
};

} // namespace Playerbot
