/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * SPELL ID VALIDATOR
 *
 * Validates all spell IDs used by the Playerbot module against the SpellDB
 * at server startup. Catches stale/invalid spell IDs early instead of
 * silently failing during combat.
 *
 * Sources validated:
 *   1. ClassSpellDatabase - rotation, defensive, cooldown, healing, fallback,
 *      and interrupt spell entries for all 39 specs
 *   2. SpellValidation_WoW120.h - All constexpr spell ID definitions
 *   3. SpellValidation_WoW120_Part2.h - Additional constexpr spell ID definitions
 *
 * Architecture:
 *   - Static singleton, called once during ClassSpellDatabase::Initialize()
 *   - Uses sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE) for validation
 *   - Logs per-spec breakdowns and aggregated summary
 *   - Thread-safe (called on main thread during startup only)
 */

#pragma once

#include "Define.h"
#include "ClassBehaviorTreeRegistry.h" // WowClass, ClassSpec
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Playerbot
{

// ============================================================================
// VALIDATION RESULT STRUCTURES
// ============================================================================

/// Result for a single spell ID validation
struct SpellValidationEntry
{
    uint32 spellId = 0;
    std::string source;     // Where this spell ID was referenced (e.g., "Rotation:HIGH", "Defensive")
    std::string specName;   // Human-readable spec name (e.g., "Arms Warrior")
    bool isValid = false;
};

/// Aggregated results for one class/spec
struct SpecValidationResult
{
    ClassSpec spec{};
    std::string specName;
    uint32 totalSpells = 0;
    uint32 validSpells = 0;
    uint32 invalidSpells = 0;
    std::vector<SpellValidationEntry> invalidEntries;
};

/// Overall validation summary
struct ValidationSummary
{
    uint32 totalSpellsChecked = 0;
    uint32 totalValid = 0;
    uint32 totalInvalid = 0;
    uint32 totalDuplicatesSkipped = 0;
    uint32 specsChecked = 0;
    uint32 specsWithErrors = 0;
    std::vector<SpecValidationResult> specResults;

    // Constexpr validation (from SpellValidation_WoW120.h files)
    uint32 constexprSpellsChecked = 0;
    uint32 constexprValid = 0;
    uint32 constexprInvalid = 0;
    std::vector<SpellValidationEntry> constexprInvalidEntries;
};

// ============================================================================
// SPELL ID VALIDATOR
// ============================================================================

class TC_GAME_API SpellIdValidator
{
public:
    /// Run full validation of all playerbot spell IDs against SpellDB.
    /// Should be called once during ClassSpellDatabase::Initialize().
    /// @return ValidationSummary with complete results
    static ValidationSummary ValidateAll();

    /// Get the last validation summary (empty if never run).
    static ValidationSummary const& GetLastResult() { return _lastResult; }

    /// Check if validation has been run
    static bool HasValidated() { return _validated; }

private:
    // ========================================================================
    // VALIDATION METHODS
    // ========================================================================

    /// Validate all spell IDs stored in ClassSpellDatabase maps
    static void ValidateClassSpellDatabase(ValidationSummary& summary);

    /// Validate all constexpr spell IDs from SpellValidation_WoW120.h files
    static void ValidateConstexprSpellIds(ValidationSummary& summary);

    /// Check a single spell ID against SpellMgr
    /// @return true if spell exists in SpellDB
    static bool IsSpellValid(uint32 spellId);

    // ========================================================================
    // HELPER METHODS
    // ========================================================================

    /// Get human-readable class name
    static std::string GetClassName(WowClass classId);

    /// Get human-readable spec name for a class/spec combination
    static std::string GetSpecName(WowClass classId, uint8 specId);

    /// Log validation results to module.playerbot channel
    static void LogResults(ValidationSummary const& summary);

    /// Register all constexpr spell IDs from DeathKnight namespace
    static void RegisterDeathKnightSpells(std::vector<std::pair<uint32, std::string>>& spells);

    /// Register all constexpr spell IDs from DemonHunter namespace
    static void RegisterDemonHunterSpells(std::vector<std::pair<uint32, std::string>>& spells);

    /// Register all constexpr spell IDs from Druid namespace
    static void RegisterDruidSpells(std::vector<std::pair<uint32, std::string>>& spells);

    /// Register all constexpr spell IDs from Evoker namespace
    static void RegisterEvokerSpells(std::vector<std::pair<uint32, std::string>>& spells);

    /// Register all constexpr spell IDs from Hunter namespace
    static void RegisterHunterSpells(std::vector<std::pair<uint32, std::string>>& spells);

    /// Register all constexpr spell IDs from Mage namespace
    static void RegisterMageSpells(std::vector<std::pair<uint32, std::string>>& spells);

    /// Register all constexpr spell IDs from Monk namespace
    static void RegisterMonkSpells(std::vector<std::pair<uint32, std::string>>& spells);

    /// Register all constexpr spell IDs from Paladin namespace
    static void RegisterPaladinSpells(std::vector<std::pair<uint32, std::string>>& spells);

    /// Register all constexpr spell IDs from Priest namespace
    static void RegisterPriestSpells(std::vector<std::pair<uint32, std::string>>& spells);

    /// Register all constexpr spell IDs from Rogue namespace
    static void RegisterRogueSpells(std::vector<std::pair<uint32, std::string>>& spells);

    /// Register all constexpr spell IDs from Shaman namespace
    static void RegisterShamanSpells(std::vector<std::pair<uint32, std::string>>& spells);

    /// Register all constexpr spell IDs from Warlock namespace
    static void RegisterWarlockSpells(std::vector<std::pair<uint32, std::string>>& spells);

    /// Register all constexpr spell IDs from Warrior namespace
    static void RegisterWarriorSpells(std::vector<std::pair<uint32, std::string>>& spells);

    // ========================================================================
    // STATE
    // ========================================================================

    static ValidationSummary _lastResult;
    static bool _validated;
};

} // namespace Playerbot
