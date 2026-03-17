/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
 *
 * Spell ID Validator implementation.
 * Validates all playerbot spell IDs against SpellDB at server startup.
 */

#include "SpellIdValidator.h"
#include "ClassSpellDatabase.h"
#include "SpellValidation_WoW120.h"
#include "SpellValidation_WoW120_Part2.h"
#include "SpellMgr.h"
#include "Log.h"
#include <algorithm>

namespace Playerbot
{

// ============================================================================
// Static Members
// ============================================================================

ValidationSummary SpellIdValidator::_lastResult;
bool SpellIdValidator::_validated = false;

// ============================================================================
// Main Validation Entry Point
// ============================================================================

ValidationSummary SpellIdValidator::ValidateAll()
{
    TC_LOG_INFO("module.playerbot", "SpellIdValidator: Starting spell ID validation against SpellDB...");

    ValidationSummary summary;

    // Phase 1: Validate all spells stored in ClassSpellDatabase
    ValidateClassSpellDatabase(summary);

    // Phase 2: Validate all constexpr spell IDs from SpellValidation headers
    ValidateConstexprSpellIds(summary);

    // Log the results
    LogResults(summary);

    _lastResult = summary;
    _validated = true;

    return summary;
}

// ============================================================================
// Phase 1: ClassSpellDatabase Validation
// ============================================================================

void SpellIdValidator::ValidateClassSpellDatabase(ValidationSummary& summary)
{
    // ClassSpellDatabase query methods call EnsureInitialized() internally,
    // so no explicit initialization needed here.

    // Iterate over all 13 classes x max 4 specs
    struct ClassSpecDef
    {
        WowClass classId;
        uint8 maxSpecs;
    };

    static constexpr ClassSpecDef classes[] = {
        { WowClass::WARRIOR,       3 },
        { WowClass::PALADIN,       3 },
        { WowClass::HUNTER,        3 },
        { WowClass::ROGUE,         3 },
        { WowClass::PRIEST,        3 },
        { WowClass::DEATH_KNIGHT,  3 },
        { WowClass::SHAMAN,        3 },
        { WowClass::MAGE,          3 },
        { WowClass::WARLOCK,       3 },
        { WowClass::MONK,          3 },
        { WowClass::DRUID,         4 },
        { WowClass::DEMON_HUNTER,  2 },
        { WowClass::EVOKER,        3 },
    };

    for (auto const& classDef : classes)
    {
        for (uint8 specId = 0; specId < classDef.maxSpecs; ++specId)
        {
            SpecValidationResult specResult;
            specResult.spec = { classDef.classId, specId };
            specResult.specName = GetSpecName(classDef.classId, specId);

            // Track unique spell IDs to avoid counting duplicates
            std::unordered_set<uint32> seenSpells;

            // --- Rotation spells ---
            auto const* rotation = ClassSpellDatabase::GetRotationTemplate(classDef.classId, specId);
            if (rotation)
            {
                for (auto const& phase : rotation->phases)
                {
                    for (auto const& spell : phase.spells)
                    {
                        if (spell.spellId == 0 || seenSpells.count(spell.spellId))
                            continue;
                        seenSpells.insert(spell.spellId);

                        specResult.totalSpells++;
                        if (IsSpellValid(spell.spellId))
                        {
                            specResult.validSpells++;
                        }
                        else
                        {
                            specResult.invalidSpells++;
                            specResult.invalidEntries.push_back({
                                spell.spellId,
                                "Rotation:" + spell.name,
                                specResult.specName,
                                false
                            });
                        }
                    }
                }
            }

            // --- Defensive spells ---
            auto const* defensives = ClassSpellDatabase::GetDefensiveSpells(classDef.classId, specId);
            if (defensives)
            {
                for (auto const& entry : *defensives)
                {
                    if (entry.spellId == 0 || seenSpells.count(entry.spellId))
                        continue;
                    seenSpells.insert(entry.spellId);

                    specResult.totalSpells++;
                    if (IsSpellValid(entry.spellId))
                    {
                        specResult.validSpells++;
                    }
                    else
                    {
                        specResult.invalidSpells++;
                        specResult.invalidEntries.push_back({
                            entry.spellId,
                            "Defensive:" + entry.name,
                            specResult.specName,
                            false
                        });
                    }
                }
            }

            // --- Cooldown spells ---
            auto const* cooldowns = ClassSpellDatabase::GetCooldownSpells(classDef.classId, specId);
            if (cooldowns)
            {
                for (auto const& entry : *cooldowns)
                {
                    if (entry.spellId == 0 || seenSpells.count(entry.spellId))
                        continue;
                    seenSpells.insert(entry.spellId);

                    specResult.totalSpells++;
                    if (IsSpellValid(entry.spellId))
                    {
                        specResult.validSpells++;
                    }
                    else
                    {
                        specResult.invalidSpells++;
                        specResult.invalidEntries.push_back({
                            entry.spellId,
                            "Cooldown:" + entry.name,
                            specResult.specName,
                            false
                        });
                    }
                }
            }

            // --- Healing tier spells ---
            auto const* healingTiers = ClassSpellDatabase::GetHealingTiers(classDef.classId, specId);
            if (healingTiers)
            {
                for (auto const& entry : *healingTiers)
                {
                    if (entry.spellId == 0 || seenSpells.count(entry.spellId))
                        continue;
                    seenSpells.insert(entry.spellId);

                    specResult.totalSpells++;
                    if (IsSpellValid(entry.spellId))
                    {
                        specResult.validSpells++;
                    }
                    else
                    {
                        specResult.invalidSpells++;
                        specResult.invalidEntries.push_back({
                            entry.spellId,
                            "HealingTier:" + entry.name,
                            specResult.specName,
                            false
                        });
                    }
                }
            }

            // --- Fallback chain spells ---
            auto const* fallbacks = ClassSpellDatabase::GetFallbackChains(classDef.classId, specId);
            if (fallbacks)
            {
                for (auto const& chain : *fallbacks)
                {
                    for (uint32 spellId : chain.spellIds)
                    {
                        if (spellId == 0 || seenSpells.count(spellId))
                            continue;
                        seenSpells.insert(spellId);

                        specResult.totalSpells++;
                        if (IsSpellValid(spellId))
                        {
                            specResult.validSpells++;
                        }
                        else
                        {
                            specResult.invalidSpells++;
                            specResult.invalidEntries.push_back({
                                spellId,
                                "Fallback:" + chain.chainName,
                                specResult.specName,
                                false
                            });
                        }
                    }
                }
            }

            // --- Interrupt spells ---
            auto const* interrupts = ClassSpellDatabase::GetInterruptSpells(classDef.classId, specId);
            if (interrupts)
            {
                for (uint32 spellId : *interrupts)
                {
                    if (spellId == 0 || seenSpells.count(spellId))
                        continue;
                    seenSpells.insert(spellId);

                    specResult.totalSpells++;
                    if (IsSpellValid(spellId))
                    {
                        specResult.validSpells++;
                    }
                    else
                    {
                        specResult.invalidSpells++;
                        specResult.invalidEntries.push_back({
                            spellId,
                            "Interrupt",
                            specResult.specName,
                            false
                        });
                    }
                }
            }

            // --- Primary interrupt ---
            uint32 primaryInterrupt = ClassSpellDatabase::GetPrimaryInterrupt(classDef.classId, specId);
            if (primaryInterrupt != 0 && !seenSpells.count(primaryInterrupt))
            {
                seenSpells.insert(primaryInterrupt);
                specResult.totalSpells++;
                if (IsSpellValid(primaryInterrupt))
                {
                    specResult.validSpells++;
                }
                else
                {
                    specResult.invalidSpells++;
                    specResult.invalidEntries.push_back({
                        primaryInterrupt,
                        "PrimaryInterrupt",
                        specResult.specName,
                        false
                    });
                }
            }

            // Add spec result to summary
            summary.totalSpellsChecked += specResult.totalSpells;
            summary.totalValid += specResult.validSpells;
            summary.totalInvalid += specResult.invalidSpells;
            summary.specsChecked++;
            if (specResult.invalidSpells > 0)
                summary.specsWithErrors++;

            summary.specResults.push_back(std::move(specResult));
        }
    }
}

// ============================================================================
// Phase 2: Constexpr Spell ID Validation
// ============================================================================

void SpellIdValidator::ValidateConstexprSpellIds(ValidationSummary& summary)
{
    // Collect all constexpr spell IDs from SpellValidation headers
    std::vector<std::pair<uint32, std::string>> allSpells;
    allSpells.reserve(2000);

    RegisterDeathKnightSpells(allSpells);
    RegisterDemonHunterSpells(allSpells);
    RegisterDruidSpells(allSpells);
    RegisterEvokerSpells(allSpells);
    RegisterHunterSpells(allSpells);
    RegisterMageSpells(allSpells);
    RegisterMonkSpells(allSpells);
    RegisterPaladinSpells(allSpells);
    RegisterPriestSpells(allSpells);
    RegisterRogueSpells(allSpells);
    RegisterShamanSpells(allSpells);
    RegisterWarlockSpells(allSpells);
    RegisterWarriorSpells(allSpells);

    // Deduplicate (some IDs appear in multiple namespace aliases)
    std::unordered_set<uint32> seen;
    for (auto const& [spellId, name] : allSpells)
    {
        if (spellId == 0 || seen.count(spellId))
        {
            summary.totalDuplicatesSkipped++;
            continue;
        }
        seen.insert(spellId);

        summary.constexprSpellsChecked++;
        if (IsSpellValid(spellId))
        {
            summary.constexprValid++;
        }
        else
        {
            summary.constexprInvalid++;
            summary.constexprInvalidEntries.push_back({
                spellId,
                "Constexpr:" + name,
                "SpellValidation_WoW120",
                false
            });
        }
    }
}

// ============================================================================
// Spell Validation
// ============================================================================

bool SpellIdValidator::IsSpellValid(uint32 spellId)
{
    if (spellId == 0)
        return false;

    // Check against SpellMgr with DIFFICULTY_NONE (base difficulty)
    return sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE) != nullptr;
}

// ============================================================================
// Human-Readable Names
// ============================================================================

std::string SpellIdValidator::GetClassName(WowClass classId)
{
    switch (classId)
    {
        case WowClass::WARRIOR:       return "Warrior";
        case WowClass::PALADIN:       return "Paladin";
        case WowClass::HUNTER:        return "Hunter";
        case WowClass::ROGUE:         return "Rogue";
        case WowClass::PRIEST:        return "Priest";
        case WowClass::DEATH_KNIGHT:  return "Death Knight";
        case WowClass::SHAMAN:        return "Shaman";
        case WowClass::MAGE:          return "Mage";
        case WowClass::WARLOCK:       return "Warlock";
        case WowClass::MONK:          return "Monk";
        case WowClass::DRUID:         return "Druid";
        case WowClass::DEMON_HUNTER:  return "Demon Hunter";
        case WowClass::EVOKER:        return "Evoker";
        default:                      return "Unknown";
    }
}

std::string SpellIdValidator::GetSpecName(WowClass classId, uint8 specId)
{
    std::string className = GetClassName(classId);

    switch (classId)
    {
        case WowClass::WARRIOR:
            switch (specId) {
                case 0: return "Arms " + className;
                case 1: return "Fury " + className;
                case 2: return "Protection " + className;
                default: return className + " Spec" + std::to_string(specId);
            }
        case WowClass::PALADIN:
            switch (specId) {
                case 0: return "Holy " + className;
                case 1: return "Protection " + className;
                case 2: return "Retribution " + className;
                default: return className + " Spec" + std::to_string(specId);
            }
        case WowClass::HUNTER:
            switch (specId) {
                case 0: return "Beast Mastery " + className;
                case 1: return "Marksmanship " + className;
                case 2: return "Survival " + className;
                default: return className + " Spec" + std::to_string(specId);
            }
        case WowClass::ROGUE:
            switch (specId) {
                case 0: return "Assassination " + className;
                case 1: return "Outlaw " + className;
                case 2: return "Subtlety " + className;
                default: return className + " Spec" + std::to_string(specId);
            }
        case WowClass::PRIEST:
            switch (specId) {
                case 0: return "Discipline " + className;
                case 1: return "Holy " + className;
                case 2: return "Shadow " + className;
                default: return className + " Spec" + std::to_string(specId);
            }
        case WowClass::DEATH_KNIGHT:
            switch (specId) {
                case 0: return "Blood " + className;
                case 1: return "Frost " + className;
                case 2: return "Unholy " + className;
                default: return className + " Spec" + std::to_string(specId);
            }
        case WowClass::SHAMAN:
            switch (specId) {
                case 0: return "Elemental " + className;
                case 1: return "Enhancement " + className;
                case 2: return "Restoration " + className;
                default: return className + " Spec" + std::to_string(specId);
            }
        case WowClass::MAGE:
            switch (specId) {
                case 0: return "Arcane " + className;
                case 1: return "Fire " + className;
                case 2: return "Frost " + className;
                default: return className + " Spec" + std::to_string(specId);
            }
        case WowClass::WARLOCK:
            switch (specId) {
                case 0: return "Affliction " + className;
                case 1: return "Demonology " + className;
                case 2: return "Destruction " + className;
                default: return className + " Spec" + std::to_string(specId);
            }
        case WowClass::MONK:
            switch (specId) {
                case 0: return "Brewmaster " + className;
                case 1: return "Mistweaver " + className;
                case 2: return "Windwalker " + className;
                default: return className + " Spec" + std::to_string(specId);
            }
        case WowClass::DRUID:
            switch (specId) {
                case 0: return "Balance " + className;
                case 1: return "Feral " + className;
                case 2: return "Guardian " + className;
                case 3: return "Restoration " + className;
                default: return className + " Spec" + std::to_string(specId);
            }
        case WowClass::DEMON_HUNTER:
            switch (specId) {
                case 0: return "Havoc " + className;
                case 1: return "Vengeance " + className;
                default: return className + " Spec" + std::to_string(specId);
            }
        case WowClass::EVOKER:
            switch (specId) {
                case 0: return "Devastation " + className;
                case 1: return "Preservation " + className;
                case 2: return "Augmentation " + className;
                default: return className + " Spec" + std::to_string(specId);
            }
        default:
            return "Unknown Spec" + std::to_string(specId);
    }
}

// ============================================================================
// Logging
// ============================================================================

void SpellIdValidator::LogResults(ValidationSummary const& summary)
{
    TC_LOG_INFO("module.playerbot", "SpellIdValidator: ==========================================");
    TC_LOG_INFO("module.playerbot", "SpellIdValidator: SPELL ID VALIDATION RESULTS");
    TC_LOG_INFO("module.playerbot", "SpellIdValidator: ==========================================");

    // Per-spec results (only log specs with issues)
    for (auto const& specResult : summary.specResults)
    {
        if (specResult.invalidSpells > 0)
        {
            TC_LOG_WARN("module.playerbot",
                "SpellIdValidator: [{}] {}/{} valid ({} INVALID)",
                specResult.specName,
                specResult.validSpells,
                specResult.totalSpells,
                specResult.invalidSpells);

            for (auto const& entry : specResult.invalidEntries)
            {
                TC_LOG_WARN("module.playerbot",
                    "SpellIdValidator:   - SpellID {} ({}) NOT FOUND in SpellDB",
                    entry.spellId,
                    entry.source);
            }
        }
    }

    // Constexpr validation results
    if (summary.constexprInvalid > 0)
    {
        TC_LOG_WARN("module.playerbot",
            "SpellIdValidator: [SpellValidation_WoW120] {}/{} constexpr spells valid ({} INVALID)",
            summary.constexprValid,
            summary.constexprSpellsChecked,
            summary.constexprInvalid);

        // Log at most 50 constexpr invalid entries to avoid log spam
        uint32 shown = 0;
        for (auto const& entry : summary.constexprInvalidEntries)
        {
            if (shown >= 50)
            {
                TC_LOG_WARN("module.playerbot",
                    "SpellIdValidator:   ... and {} more invalid constexpr spell IDs",
                    summary.constexprInvalid - 50);
                break;
            }
            TC_LOG_WARN("module.playerbot",
                "SpellIdValidator:   - SpellID {} ({}) NOT FOUND in SpellDB",
                entry.spellId,
                entry.source);
            ++shown;
        }
    }

    // Summary line
    uint32 totalChecked = summary.totalSpellsChecked + summary.constexprSpellsChecked;
    uint32 totalValid = summary.totalValid + summary.constexprValid;
    uint32 totalInvalid = summary.totalInvalid + summary.constexprInvalid;

    TC_LOG_INFO("module.playerbot", "SpellIdValidator: ------------------------------------------");
    TC_LOG_INFO("module.playerbot", "SpellIdValidator: ClassSpellDatabase: {}/{} spells valid across {} specs ({} specs with errors)",
        summary.totalValid, summary.totalSpellsChecked,
        summary.specsChecked, summary.specsWithErrors);
    TC_LOG_INFO("module.playerbot", "SpellIdValidator: Constexpr SpellIDs: {}/{} valid ({} duplicates skipped)",
        summary.constexprValid, summary.constexprSpellsChecked,
        summary.totalDuplicatesSkipped);
    TC_LOG_INFO("module.playerbot", "SpellIdValidator: TOTAL: {}/{} spell IDs valid ({} invalid)",
        totalValid, totalChecked, totalInvalid);

    if (totalInvalid == 0)
    {
        TC_LOG_INFO("module.playerbot", "SpellIdValidator: ALL SPELL IDS VALIDATED SUCCESSFULLY");
    }
    else
    {
        TC_LOG_WARN("module.playerbot",
            "SpellIdValidator: WARNING: {} invalid spell IDs detected - these spells will silently fail in rotations",
            totalInvalid);
    }

    TC_LOG_INFO("module.playerbot", "SpellIdValidator: ==========================================");
}

// ============================================================================
// Constexpr Spell ID Registration - Death Knight
// ============================================================================

void SpellIdValidator::RegisterDeathKnightSpells(std::vector<std::pair<uint32, std::string>>& spells)
{
    namespace DK = WoW120Spells::DeathKnight;

    // Core
    spells.push_back({ DK::DEATH_STRIKE, "DK::DEATH_STRIKE" });
    spells.push_back({ DK::DEATH_AND_DECAY, "DK::DEATH_AND_DECAY" });
    spells.push_back({ DK::DEATH_GRIP, "DK::DEATH_GRIP" });
    spells.push_back({ DK::ANTI_MAGIC_SHELL, "DK::ANTI_MAGIC_SHELL" });
    spells.push_back({ DK::ANTI_MAGIC_ZONE, "DK::ANTI_MAGIC_ZONE" });
    spells.push_back({ DK::ICEBOUND_FORTITUDE, "DK::ICEBOUND_FORTITUDE" });
    spells.push_back({ DK::CHAINS_OF_ICE, "DK::CHAINS_OF_ICE" });
    spells.push_back({ DK::MIND_FREEZE, "DK::MIND_FREEZE" });
    spells.push_back({ DK::PATH_OF_FROST, "DK::PATH_OF_FROST" });
    spells.push_back({ DK::RAISE_DEAD, "DK::RAISE_DEAD" });
    spells.push_back({ DK::SACRIFICIAL_PACT, "DK::SACRIFICIAL_PACT" });
    spells.push_back({ DK::DEATH_COIL, "DK::DEATH_COIL" });
    spells.push_back({ DK::DARK_COMMAND, "DK::DARK_COMMAND" });
    spells.push_back({ DK::RAISE_ALLY, "DK::RAISE_ALLY" });
    spells.push_back({ DK::CONTROL_UNDEAD, "DK::CONTROL_UNDEAD" });
    spells.push_back({ DK::DEATHS_ADVANCE, "DK::DEATHS_ADVANCE" });
    spells.push_back({ DK::ASPHYXIATE, "DK::ASPHYXIATE" });

    // Blood
    spells.push_back({ DK::Blood::MARROWREND, "DK::Blood::MARROWREND" });
    spells.push_back({ DK::Blood::HEART_STRIKE, "DK::Blood::HEART_STRIKE" });
    spells.push_back({ DK::Blood::BLOOD_BOIL, "DK::Blood::BLOOD_BOIL" });
    spells.push_back({ DK::Blood::RUNE_TAP, "DK::Blood::RUNE_TAP" });
    spells.push_back({ DK::Blood::VAMPIRIC_BLOOD, "DK::Blood::VAMPIRIC_BLOOD" });
    spells.push_back({ DK::Blood::DANCING_RUNE_WEAPON, "DK::Blood::DANCING_RUNE_WEAPON" });
    spells.push_back({ DK::Blood::BLOODDRINKER, "DK::Blood::BLOODDRINKER" });
    spells.push_back({ DK::Blood::BONESTORM, "DK::Blood::BONESTORM" });
    spells.push_back({ DK::Blood::CONSUMPTION, "DK::Blood::CONSUMPTION" });
    spells.push_back({ DK::Blood::GOREFIENDS_GRASP, "DK::Blood::GOREFIENDS_GRASP" });
    spells.push_back({ DK::Blood::TOMBSTONE, "DK::Blood::TOMBSTONE" });
    spells.push_back({ DK::Blood::BLOOD_TAP, "DK::Blood::BLOOD_TAP" });
    spells.push_back({ DK::Blood::DEATHS_CARESS, "DK::Blood::DEATHS_CARESS" });
    spells.push_back({ DK::Blood::BONE_SHIELD, "DK::Blood::BONE_SHIELD" });
    spells.push_back({ DK::Blood::BLOOD_PLAGUE, "DK::Blood::BLOOD_PLAGUE" });
    spells.push_back({ DK::Blood::CRIMSON_SCOURGE, "DK::Blood::CRIMSON_SCOURGE" });
    spells.push_back({ DK::Blood::HEMOSTASIS, "DK::Blood::HEMOSTASIS" });
    // Hero Talents
    spells.push_back({ DK::Blood::REAPER_MARK, "DK::Blood::REAPER_MARK" });
    spells.push_back({ DK::Blood::WAVE_OF_SOULS, "DK::Blood::WAVE_OF_SOULS" });
    spells.push_back({ DK::Blood::EXTERMINATE, "DK::Blood::EXTERMINATE" });
    spells.push_back({ DK::Blood::VAMPIRIC_STRIKE, "DK::Blood::VAMPIRIC_STRIKE" });

    // Frost
    spells.push_back({ DK::Frost::FROST_STRIKE, "DK::Frost::FROST_STRIKE" });
    spells.push_back({ DK::Frost::HOWLING_BLAST, "DK::Frost::HOWLING_BLAST" });
    spells.push_back({ DK::Frost::OBLITERATE, "DK::Frost::OBLITERATE" });
    spells.push_back({ DK::Frost::REMORSELESS_WINTER, "DK::Frost::REMORSELESS_WINTER" });
    spells.push_back({ DK::Frost::PILLAR_OF_FROST, "DK::Frost::PILLAR_OF_FROST" });
    spells.push_back({ DK::Frost::EMPOWER_RUNE_WEAPON, "DK::Frost::EMPOWER_RUNE_WEAPON" });
    spells.push_back({ DK::Frost::FROSTSCYTHE, "DK::Frost::FROSTSCYTHE" });
    spells.push_back({ DK::Frost::GLACIAL_ADVANCE, "DK::Frost::GLACIAL_ADVANCE" });
    spells.push_back({ DK::Frost::BREATH_OF_SINDRAGOSA, "DK::Frost::BREATH_OF_SINDRAGOSA" });

    // Unholy
    spells.push_back({ DK::Unholy::FESTERING_STRIKE, "DK::Unholy::FESTERING_STRIKE" });
    spells.push_back({ DK::Unholy::SCOURGE_STRIKE, "DK::Unholy::SCOURGE_STRIKE" });
    spells.push_back({ DK::Unholy::OUTBREAK, "DK::Unholy::OUTBREAK" });
    spells.push_back({ DK::Unholy::DARK_TRANSFORMATION, "DK::Unholy::DARK_TRANSFORMATION" });
    spells.push_back({ DK::Unholy::APOCALYPSE, "DK::Unholy::APOCALYPSE" });
    spells.push_back({ DK::Unholy::ARMY_OF_THE_DEAD, "DK::Unholy::ARMY_OF_THE_DEAD" });
    spells.push_back({ DK::Unholy::EPIDEMIC, "DK::Unholy::EPIDEMIC" });
    spells.push_back({ DK::Unholy::UNHOLY_BLIGHT, "DK::Unholy::UNHOLY_BLIGHT" });
    spells.push_back({ DK::Unholy::SOUL_REAPER, "DK::Unholy::SOUL_REAPER" });
    spells.push_back({ DK::Unholy::SUMMON_GARGOYLE, "DK::Unholy::SUMMON_GARGOYLE" });
}

// ============================================================================
// Constexpr Spell ID Registration - Demon Hunter
// ============================================================================

void SpellIdValidator::RegisterDemonHunterSpells(std::vector<std::pair<uint32, std::string>>& spells)
{
    namespace DH = WoW120Spells::DemonHunter;

    // Core
    spells.push_back({ DH::DISRUPT, "DH::DISRUPT" });
    spells.push_back({ DH::CONSUME_MAGIC, "DH::CONSUME_MAGIC" });
    spells.push_back({ DH::CHAOS_NOVA, "DH::CHAOS_NOVA" });
    spells.push_back({ DH::DARKNESS, "DH::DARKNESS" });
    spells.push_back({ DH::METAMORPHOSIS_HAVOC, "DH::METAMORPHOSIS_HAVOC" });
    spells.push_back({ DH::IMMOLATION_AURA, "DH::IMMOLATION_AURA" });
    spells.push_back({ DH::SPECTRAL_SIGHT, "DH::SPECTRAL_SIGHT" });

    // Havoc
    spells.push_back({ DH::Havoc::DEMONS_BITE, "DH::Havoc::DEMONS_BITE" });
    spells.push_back({ DH::Havoc::CHAOS_STRIKE, "DH::Havoc::CHAOS_STRIKE" });
    spells.push_back({ DH::Havoc::BLADE_DANCE, "DH::Havoc::BLADE_DANCE" });
    spells.push_back({ DH::Havoc::EYE_BEAM, "DH::Havoc::EYE_BEAM" });
    spells.push_back({ DH::FEL_RUSH, "DH::FEL_RUSH" });
    spells.push_back({ DH::VENGEFUL_RETREAT, "DH::VENGEFUL_RETREAT" });
    spells.push_back({ DH::THROW_GLAIVE, "DH::THROW_GLAIVE" });
    spells.push_back({ DH::Havoc::THE_HUNT, "DH::Havoc::THE_HUNT" });
    spells.push_back({ DH::Havoc::ESSENCE_BREAK, "DH::Havoc::ESSENCE_BREAK" });
    spells.push_back({ DH::Havoc::GLAIVE_TEMPEST, "DH::Havoc::GLAIVE_TEMPEST" });

    // Vengeance
    spells.push_back({ DH::Vengeance::SHEAR, "DH::Vengeance::SHEAR" });
    spells.push_back({ DH::Vengeance::SOUL_CLEAVE, "DH::Vengeance::SOUL_CLEAVE" });
    spells.push_back({ DH::Vengeance::DEMON_SPIKES, "DH::Vengeance::DEMON_SPIKES" });
    spells.push_back({ DH::Vengeance::FIERY_BRAND, "DH::Vengeance::FIERY_BRAND" });
    spells.push_back({ DH::SIGIL_OF_FLAME, "DH::SIGIL_OF_FLAME" });
    spells.push_back({ DH::Vengeance::INFERNAL_STRIKE, "DH::Vengeance::INFERNAL_STRIKE" });
    spells.push_back({ DH::Vengeance::FEL_DEVASTATION, "DH::Vengeance::FEL_DEVASTATION" });
    spells.push_back({ DH::Vengeance::SPIRIT_BOMB, "DH::Vengeance::SPIRIT_BOMB" });
}

// ============================================================================
// Constexpr Spell ID Registration - Druid
// ============================================================================

void SpellIdValidator::RegisterDruidSpells(std::vector<std::pair<uint32, std::string>>& spells)
{
    namespace Dr = WoW120Spells::Druid;

    // Core (Druid root namespace)
    spells.push_back({ Dr::MOONFIRE, "Dr::MOONFIRE" });
    spells.push_back({ Dr::SUNFIRE, "Dr::SUNFIRE" });
    spells.push_back({ Dr::REGROWTH, "Dr::REGROWTH" });
    spells.push_back({ Dr::REJUVENATION, "Dr::REJUVENATION" });
    spells.push_back({ Dr::WILD_GROWTH, "Dr::WILD_GROWTH" });
    spells.push_back({ Dr::INNERVATE, "Dr::INNERVATE" });
    spells.push_back({ Dr::BARKSKIN, "Dr::BARKSKIN" });
    spells.push_back({ Dr::DASH, "Dr::DASH" });
    spells.push_back({ Dr::STAMPEDING_ROAR, "Dr::STAMPEDING_ROAR" });
    spells.push_back({ Dr::ENTANGLING_ROOTS, "Dr::ENTANGLING_ROOTS" });
    spells.push_back({ Dr::HIBERNATE, "Dr::HIBERNATE" });
    spells.push_back({ Dr::REBIRTH, "Dr::REBIRTH" });
    spells.push_back({ Dr::SOOTHE, "Dr::SOOTHE" });
    spells.push_back({ Dr::SKULL_BASH, "Dr::SKULL_BASH" });
    spells.push_back({ Dr::SURVIVAL_INSTINCTS, "Dr::SURVIVAL_INSTINCTS" });

    // Forms
    spells.push_back({ Dr::MOONKIN_FORM, "Dr::MOONKIN_FORM" });
    spells.push_back({ Dr::CAT_FORM, "Dr::CAT_FORM" });
    spells.push_back({ Dr::BEAR_FORM, "Dr::BEAR_FORM" });
    spells.push_back({ Dr::TRAVEL_FORM, "Dr::TRAVEL_FORM" });

    // Balance (sub-namespace)
    spells.push_back({ Dr::Balance::WRATH, "Dr::Balance::WRATH" });
    spells.push_back({ Dr::Balance::STARFIRE, "Dr::Balance::STARFIRE" });
    spells.push_back({ Dr::Balance::STARSURGE, "Dr::Balance::STARSURGE" });
    spells.push_back({ Dr::Balance::STARFALL, "Dr::Balance::STARFALL" });
    spells.push_back({ Dr::Balance::SOLAR_ECLIPSE, "Dr::Balance::SOLAR_ECLIPSE" });
    spells.push_back({ Dr::Balance::LUNAR_ECLIPSE, "Dr::Balance::LUNAR_ECLIPSE" });
    spells.push_back({ Dr::Balance::CELESTIAL_ALIGNMENT, "Dr::Balance::CELESTIAL_ALIGNMENT" });
    spells.push_back({ Dr::Balance::STELLAR_FLARE, "Dr::Balance::STELLAR_FLARE" });

    // Feral (sub-namespace)
    spells.push_back({ Dr::Feral::SHRED, "Dr::Feral::SHRED" });
    spells.push_back({ Dr::Feral::RAKE, "Dr::Feral::RAKE" });
    spells.push_back({ Dr::Feral::RIP, "Dr::Feral::RIP" });
    spells.push_back({ Dr::Feral::FEROCIOUS_BITE, "Dr::Feral::FEROCIOUS_BITE" });
    spells.push_back({ Dr::Feral::TIGERS_FURY, "Dr::Feral::TIGERS_FURY" });
    spells.push_back({ Dr::Feral::BERSERK, "Dr::Feral::BERSERK" });

    // Guardian (sub-namespace)
    spells.push_back({ Dr::Guardian::MANGLE, "Dr::Guardian::MANGLE" });
    spells.push_back({ Dr::Guardian::THRASH_BEAR, "Dr::Guardian::THRASH_BEAR" });
    spells.push_back({ Dr::Guardian::IRONFUR, "Dr::Guardian::IRONFUR" });
    spells.push_back({ Dr::Guardian::SWIPE_BEAR, "Dr::Guardian::SWIPE_BEAR" });
    spells.push_back({ Dr::Guardian::FRENZIED_REGENERATION, "Dr::Guardian::FRENZIED_REGENERATION" });
    spells.push_back({ Dr::Guardian::BERSERK_GUARDIAN, "Dr::Guardian::BERSERK_GUARDIAN" });

    // Restoration (sub-namespace)
    spells.push_back({ Dr::Restoration::LIFEBLOOM, "Dr::Restoration::LIFEBLOOM" });
    spells.push_back({ Dr::Restoration::EFFLORESCENCE, "Dr::Restoration::EFFLORESCENCE" });
    spells.push_back({ Dr::Restoration::TRANQUILITY, "Dr::Restoration::TRANQUILITY" });
    spells.push_back({ Dr::Restoration::IRONBARK, "Dr::Restoration::IRONBARK" });
    spells.push_back({ Dr::Restoration::CENARION_WARD, "Dr::Restoration::CENARION_WARD" });
    spells.push_back({ Dr::Restoration::NATURES_SWIFTNESS, "Dr::Restoration::NATURES_SWIFTNESS" });
}

// ============================================================================
// Constexpr Spell ID Registration - Evoker
// ============================================================================

void SpellIdValidator::RegisterEvokerSpells(std::vector<std::pair<uint32, std::string>>& spells)
{
    namespace Ev = WoW120Spells::Evoker;

    // Core
    spells.push_back({ Ev::DISINTEGRATE, "Ev::DISINTEGRATE" });
    spells.push_back({ Ev::AZURE_STRIKE, "Ev::AZURE_STRIKE" });
    spells.push_back({ Ev::LIVING_FLAME, "Ev::LIVING_FLAME" });
    spells.push_back({ Ev::FIRE_BREATH, "Ev::FIRE_BREATH" });
    spells.push_back({ Ev::HOVER, "Ev::HOVER" });
    spells.push_back({ Ev::DEEP_BREATH, "Ev::DEEP_BREATH" });
    spells.push_back({ Ev::QUELL, "Ev::QUELL" });
    spells.push_back({ Ev::WING_BUFFET, "Ev::WING_BUFFET" });
    spells.push_back({ Ev::TAIL_SWIPE, "Ev::TAIL_SWIPE" });
    spells.push_back({ Ev::OBSIDIAN_SCALES, "Ev::OBSIDIAN_SCALES" });
    spells.push_back({ Ev::RESCUE, "Ev::RESCUE" });
    spells.push_back({ Ev::VERDANT_EMBRACE, "Ev::VERDANT_EMBRACE" });

    // Devastation
    spells.push_back({ Ev::Devastation::ETERNITY_SURGE, "Ev::Devastation::ETERNITY_SURGE" });
    spells.push_back({ Ev::Devastation::SHATTERING_STAR, "Ev::Devastation::SHATTERING_STAR" });
    spells.push_back({ Ev::Devastation::DRAGONRAGE, "Ev::Devastation::DRAGONRAGE" });
    spells.push_back({ Ev::Devastation::PYRE, "Ev::Devastation::PYRE" });

    // Preservation
    spells.push_back({ Ev::Preservation::DREAM_BREATH, "Ev::Preservation::DREAM_BREATH" });
    spells.push_back({ Ev::Preservation::SPIRITBLOOM, "Ev::Preservation::SPIRITBLOOM" });
    spells.push_back({ Ev::Preservation::REVERSION, "Ev::Preservation::REVERSION" });
    spells.push_back({ Ev::Preservation::ECHO, "Ev::Preservation::ECHO" });
    spells.push_back({ Ev::Preservation::TEMPORAL_ANOMALY, "Ev::Preservation::TEMPORAL_ANOMALY" });
    spells.push_back({ Ev::Preservation::EMERALD_COMMUNION, "Ev::Preservation::EMERALD_COMMUNION" });
    spells.push_back({ Ev::Preservation::STASIS, "Ev::Preservation::STASIS" });
    spells.push_back({ Ev::Preservation::REWIND, "Ev::Preservation::REWIND" });

    // Augmentation
    spells.push_back({ Ev::Augmentation::EBON_MIGHT, "Ev::Augmentation::EBON_MIGHT" });
    spells.push_back({ Ev::Augmentation::ERUPTION, "Ev::Augmentation::ERUPTION" });
    spells.push_back({ Ev::Augmentation::UPHEAVAL, "Ev::Augmentation::UPHEAVAL" });
    spells.push_back({ Ev::Augmentation::PRESCIENCE, "Ev::Augmentation::PRESCIENCE" });
    spells.push_back({ Ev::Augmentation::BLISTERING_SCALES, "Ev::Augmentation::BLISTERING_SCALES" });
}

// ============================================================================
// Constexpr Spell ID Registration - Hunter
// ============================================================================

void SpellIdValidator::RegisterHunterSpells(std::vector<std::pair<uint32, std::string>>& spells)
{
    namespace Hu = WoW120Spells::Hunter;

    // Core
    spells.push_back({ Hu::ARCANE_SHOT, "Hu::ARCANE_SHOT" });
    spells.push_back({ Hu::STEADY_SHOT, "Hu::STEADY_SHOT" });
    spells.push_back({ Hu::KILL_SHOT, "Hu::KILL_SHOT" });
    spells.push_back({ Hu::MULTI_SHOT, "Hu::MULTI_SHOT" });
    spells.push_back({ Hu::COUNTER_SHOT, "Hu::COUNTER_SHOT" });
    spells.push_back({ Hu::MISDIRECTION, "Hu::MISDIRECTION" });
    spells.push_back({ Hu::FEIGN_DEATH, "Hu::FEIGN_DEATH" });
    spells.push_back({ Hu::DISENGAGE, "Hu::DISENGAGE" });
    spells.push_back({ Hu::EXHILARATION, "Hu::EXHILARATION" });
    spells.push_back({ Hu::ASPECT_OF_THE_CHEETAH, "Hu::ASPECT_OF_THE_CHEETAH" });
    spells.push_back({ Hu::ASPECT_OF_THE_TURTLE, "Hu::ASPECT_OF_THE_TURTLE" });
    spells.push_back({ Hu::FREEZING_TRAP, "Hu::FREEZING_TRAP" });
    spells.push_back({ Hu::TAR_TRAP, "Hu::TAR_TRAP" });

    // Beast Mastery
    spells.push_back({ Hu::BeastMastery::BARBED_SHOT, "Hu::BM::BARBED_SHOT" });
    spells.push_back({ Hu::BeastMastery::BESTIAL_WRATH, "Hu::BM::BESTIAL_WRATH" });
    spells.push_back({ Hu::BeastMastery::KILL_COMMAND, "Hu::BM::KILL_COMMAND" });
    spells.push_back({ Hu::BeastMastery::COBRA_SHOT, "Hu::BM::COBRA_SHOT" });
    spells.push_back({ Hu::BeastMastery::DIRE_BEAST, "Hu::BM::DIRE_BEAST" });
    spells.push_back({ Hu::BeastMastery::ASPECT_OF_THE_WILD, "Hu::BM::ASPECT_OF_THE_WILD" });

    // Marksmanship
    spells.push_back({ Hu::Marksmanship::AIMED_SHOT_MM, "Hu::MM::AIMED_SHOT_MM" });
    spells.push_back({ Hu::Marksmanship::RAPID_FIRE_MM, "Hu::MM::RAPID_FIRE_MM" });
    spells.push_back({ Hu::Marksmanship::TRUESHOT, "Hu::MM::TRUESHOT" });
    spells.push_back({ Hu::Marksmanship::VOLLEY, "Hu::MM::VOLLEY" });
    spells.push_back({ Hu::Marksmanship::TRICK_SHOTS, "Hu::MM::TRICK_SHOTS" });

    // Survival
    spells.push_back({ Hu::Survival::KILL_COMMAND_SURVIVAL, "Hu::SV::KILL_COMMAND_SURVIVAL" });
    spells.push_back({ Hu::Survival::WILDFIRE_BOMB, "Hu::SV::WILDFIRE_BOMB" });
    spells.push_back({ Hu::Survival::RAPTOR_STRIKE, "Hu::SV::RAPTOR_STRIKE" });
    spells.push_back({ Hu::Survival::HARPOON, "Hu::SV::HARPOON" });
    spells.push_back({ Hu::Survival::COORDINATED_ASSAULT, "Hu::SV::COORDINATED_ASSAULT" });
    spells.push_back({ Hu::Survival::MONGOOSE_BITE, "Hu::SV::MONGOOSE_BITE" });
}

// ============================================================================
// Constexpr Spell ID Registration - Mage
// ============================================================================

void SpellIdValidator::RegisterMageSpells(std::vector<std::pair<uint32, std::string>>& spells)
{
    namespace Ma = WoW120Spells::Mage;

    // Core
    spells.push_back({ Ma::FROSTBOLT, "Ma::FROSTBOLT" });
    spells.push_back({ Ma::FIREBALL, "Ma::FIREBALL" });
    spells.push_back({ Ma::Arcane::ARCANE_BLAST, "Ma::Arcane::ARCANE_BLAST" });
    spells.push_back({ Ma::FROST_NOVA, "Ma::FROST_NOVA" });
    spells.push_back({ Ma::BLINK, "Ma::BLINK" });
    spells.push_back({ Ma::COUNTERSPELL, "Ma::COUNTERSPELL" });
    spells.push_back({ Ma::ICE_BLOCK, "Ma::ICE_BLOCK" });
    spells.push_back({ Ma::MIRROR_IMAGE, "Ma::MIRROR_IMAGE" });
    spells.push_back({ Ma::POLYMORPH, "Ma::POLYMORPH" });
    spells.push_back({ Ma::SPELLSTEAL, "Ma::SPELLSTEAL" });
    spells.push_back({ Ma::REMOVE_CURSE, "Ma::REMOVE_CURSE" });
    spells.push_back({ Ma::TIME_WARP, "Ma::TIME_WARP" });
    spells.push_back({ Ma::INVISIBILITY, "Ma::INVISIBILITY" });

    // Arcane
    spells.push_back({ Ma::Arcane::ARCANE_MISSILES, "Ma::Arcane::ARCANE_MISSILES" });
    spells.push_back({ Ma::Arcane::ARCANE_BARRAGE, "Ma::Arcane::ARCANE_BARRAGE" });
    spells.push_back({ Ma::Arcane::ARCANE_EXPLOSION, "Ma::Arcane::ARCANE_EXPLOSION" });
    spells.push_back({ Ma::Arcane::ARCANE_POWER, "Ma::Arcane::ARCANE_POWER" });
    spells.push_back({ Ma::Arcane::EVOCATION, "Ma::Arcane::EVOCATION" });
    spells.push_back({ Ma::Arcane::TOUCH_OF_THE_MAGI, "Ma::Arcane::TOUCH_OF_THE_MAGI" });
    spells.push_back({ Ma::Arcane::ARCANE_SURGE, "Ma::Arcane::ARCANE_SURGE" });

    // Fire
    spells.push_back({ Ma::Fire::FIRE_BLAST, "Ma::Fire::FIRE_BLAST" });
    spells.push_back({ Ma::Fire::PYROBLAST, "Ma::Fire::PYROBLAST" });
    spells.push_back({ Ma::Fire::COMBUSTION, "Ma::Fire::COMBUSTION" });
    spells.push_back({ Ma::Fire::PHOENIX_FLAMES, "Ma::Fire::PHOENIX_FLAMES" });
    spells.push_back({ Ma::Fire::FLAMESTRIKE, "Ma::Fire::FLAMESTRIKE" });
    spells.push_back({ Ma::DRAGONS_BREATH, "Ma::DRAGONS_BREATH" });
    spells.push_back({ Ma::Fire::SCORCH, "Ma::Fire::SCORCH" });

    // Frost
    spells.push_back({ Ma::Frost::ICE_LANCE, "Ma::Frost::ICE_LANCE" });
    spells.push_back({ Ma::Frost::FLURRY, "Ma::Frost::FLURRY" });
    spells.push_back({ Ma::Frost::FROZEN_ORB, "Ma::Frost::FROZEN_ORB" });
    spells.push_back({ Ma::Frost::BLIZZARD, "Ma::Frost::BLIZZARD" });
    spells.push_back({ Ma::Frost::ICY_VEINS, "Ma::Frost::ICY_VEINS" });
    spells.push_back({ Ma::Frost::CONE_OF_COLD, "Ma::Frost::CONE_OF_COLD" });
    spells.push_back({ Ma::Frost::GLACIAL_SPIKE, "Ma::Frost::GLACIAL_SPIKE" });
    spells.push_back({ Ma::Frost::COMET_STORM, "Ma::Frost::COMET_STORM" });
    spells.push_back({ Ma::Frost::RAY_OF_FROST, "Ma::Frost::RAY_OF_FROST" });
}

// ============================================================================
// Constexpr Spell ID Registration - Monk
// ============================================================================

void SpellIdValidator::RegisterMonkSpells(std::vector<std::pair<uint32, std::string>>& spells)
{
    namespace Mo = WoW120Spells::Monk;

    // Core
    spells.push_back({ Mo::TIGER_PALM, "Mo::TIGER_PALM" });
    spells.push_back({ Mo::Brewmaster::BLACKOUT_KICK, "Mo::BM::BLACKOUT_KICK" });
    spells.push_back({ Mo::Mistweaver::RISING_SUN_KICK, "Mo::MW::RISING_SUN_KICK" });
    spells.push_back({ Mo::ROLL, "Mo::ROLL" });
    spells.push_back({ Mo::Mistweaver::VIVIFY, "Mo::MW::VIVIFY" });
    spells.push_back({ Mo::DETOX, "Mo::DETOX" });
    spells.push_back({ Mo::LEG_SWEEP, "Mo::LEG_SWEEP" });
    spells.push_back({ Mo::PARALYSIS, "Mo::PARALYSIS" });
    spells.push_back({ Mo::SPEAR_HAND_STRIKE, "Mo::SPEAR_HAND_STRIKE" });
    spells.push_back({ Mo::FORTIFYING_BREW, "Mo::FORTIFYING_BREW" });
    spells.push_back({ Mo::EXPEL_HARM, "Mo::EXPEL_HARM" });

    // Brewmaster
    spells.push_back({ Mo::Brewmaster::KEG_SMASH, "Mo::BM::KEG_SMASH" });
    spells.push_back({ Mo::Brewmaster::BREATH_OF_FIRE, "Mo::BM::BREATH_OF_FIRE" });
    spells.push_back({ Mo::Brewmaster::PURIFYING_BREW, "Mo::BM::PURIFYING_BREW" });
    spells.push_back({ Mo::Brewmaster::CELESTIAL_BREW, "Mo::BM::CELESTIAL_BREW" });
    spells.push_back({ Mo::Brewmaster::INVOKE_NIUZAO, "Mo::BM::INVOKE_NIUZAO" });
    spells.push_back({ Mo::Brewmaster::SPINNING_CRANE_KICK, "Mo::BM::SPINNING_CRANE_KICK" });

    // Mistweaver
    spells.push_back({ Mo::Mistweaver::ENVELOPING_MIST, "Mo::MW::ENVELOPING_MIST" });
    spells.push_back({ Mo::Mistweaver::RENEWING_MIST, "Mo::MW::RENEWING_MIST" });
    spells.push_back({ Mo::Mistweaver::ESSENCE_FONT, "Mo::MW::ESSENCE_FONT" });
    spells.push_back({ Mo::Mistweaver::SOOTHING_MIST, "Mo::MW::SOOTHING_MIST" });
    spells.push_back({ Mo::Mistweaver::REVIVAL, "Mo::MW::REVIVAL" });
    spells.push_back({ Mo::Mistweaver::THUNDER_FOCUS_TEA, "Mo::MW::THUNDER_FOCUS_TEA" });
    spells.push_back({ Mo::Mistweaver::LIFE_COCOON, "Mo::MW::LIFE_COCOON" });
    spells.push_back({ Mo::Mistweaver::INVOKE_YULON, "Mo::MW::INVOKE_YULON" });

    // Windwalker
    spells.push_back({ Mo::Windwalker::FISTS_OF_FURY, "Mo::WW::FISTS_OF_FURY" });
    spells.push_back({ Mo::Windwalker::SPINNING_CRANE_KICK_WW, "Mo::WW::SPINNING_CRANE_KICK_WW" });
    spells.push_back({ Mo::Windwalker::STRIKE_OF_THE_WINDLORD, "Mo::WW::STRIKE_OF_THE_WINDLORD" });
    spells.push_back({ Mo::Windwalker::STORM_EARTH_AND_FIRE, "Mo::WW::STORM_EARTH_AND_FIRE" });
    spells.push_back({ Mo::Windwalker::WHIRLING_DRAGON_PUNCH, "Mo::WW::WHIRLING_DRAGON_PUNCH" });
    spells.push_back({ Mo::TOUCH_OF_DEATH, "Mo::TOUCH_OF_DEATH" });
    spells.push_back({ Mo::Windwalker::INVOKE_XUEN, "Mo::WW::INVOKE_XUEN" });
}

// ============================================================================
// Constexpr Spell ID Registration - Paladin
// ============================================================================

void SpellIdValidator::RegisterPaladinSpells(std::vector<std::pair<uint32, std::string>>& spells)
{
    namespace Pa = WoW120Spells::Paladin;

    // Core
    spells.push_back({ Pa::FLASH_OF_LIGHT, "Pa::FLASH_OF_LIGHT" });
    spells.push_back({ Pa::WORD_OF_GLORY, "Pa::WORD_OF_GLORY" });
    spells.push_back({ Pa::DIVINE_SHIELD, "Pa::DIVINE_SHIELD" });
    spells.push_back({ Pa::DIVINE_PROTECTION, "Pa::DIVINE_PROTECTION" });
    spells.push_back({ Pa::BLESSING_OF_FREEDOM, "Pa::BLESSING_OF_FREEDOM" });
    spells.push_back({ Pa::BLESSING_OF_PROTECTION, "Pa::BLESSING_OF_PROTECTION" });
    spells.push_back({ Pa::BLESSING_OF_SACRIFICE, "Pa::BLESSING_OF_SACRIFICE" });
    spells.push_back({ Pa::LAY_ON_HANDS, "Pa::LAY_ON_HANDS" });
    spells.push_back({ Pa::HAMMER_OF_JUSTICE, "Pa::HAMMER_OF_JUSTICE" });
    spells.push_back({ Pa::HAMMER_OF_WRATH, "Pa::HAMMER_OF_WRATH" });
    spells.push_back({ Pa::REBUKE, "Pa::REBUKE" });
    spells.push_back({ Pa::CRUSADER_STRIKE, "Pa::CRUSADER_STRIKE" });
    spells.push_back({ Pa::JUDGMENT, "Pa::JUDGMENT" });
    spells.push_back({ Pa::CONSECRATION, "Pa::CONSECRATION" });
    spells.push_back({ Pa::AVENGING_WRATH, "Pa::AVENGING_WRATH" });

    // Holy
    spells.push_back({ Pa::Holy::HOLY_SHOCK, "Pa::Holy::HOLY_SHOCK" });
    spells.push_back({ Pa::Holy::LIGHT_OF_DAWN, "Pa::Holy::LIGHT_OF_DAWN" });
    spells.push_back({ Pa::Holy::BEACON_OF_LIGHT, "Pa::Holy::BEACON_OF_LIGHT" });
    spells.push_back({ Pa::Holy::AURA_MASTERY, "Pa::Holy::AURA_MASTERY" });

    // Protection
    spells.push_back({ Pa::Protection::SHIELD_OF_THE_RIGHTEOUS, "Pa::Prot::SHIELD_OF_THE_RIGHTEOUS" });
    spells.push_back({ Pa::Protection::AVENGERS_SHIELD, "Pa::Prot::AVENGERS_SHIELD" });
    spells.push_back({ Pa::Protection::GUARDIAN_OF_ANCIENT_KINGS, "Pa::Prot::GUARDIAN_OF_ANCIENT_KINGS" });
    spells.push_back({ Pa::Protection::ARDENT_DEFENDER, "Pa::Prot::ARDENT_DEFENDER" });

    // Retribution
    spells.push_back({ Pa::Retribution::TEMPLARS_VERDICT, "Pa::Ret::TEMPLARS_VERDICT" });
    spells.push_back({ Pa::Retribution::DIVINE_STORM, "Pa::Ret::DIVINE_STORM" });
    spells.push_back({ Pa::Retribution::BLADE_OF_JUSTICE, "Pa::Ret::BLADE_OF_JUSTICE" });
    spells.push_back({ Pa::Retribution::WAKE_OF_ASHES, "Pa::Ret::WAKE_OF_ASHES" });
    spells.push_back({ Pa::Retribution::CRUSADE, "Pa::Ret::CRUSADE" });
    spells.push_back({ Pa::Retribution::FINAL_VERDICT, "Pa::Ret::FINAL_VERDICT" });
}

// ============================================================================
// Constexpr Spell ID Registration - Priest
// ============================================================================

void SpellIdValidator::RegisterPriestSpells(std::vector<std::pair<uint32, std::string>>& spells)
{
    namespace Pr = WoW120Spells::Priest;

    // Core
    spells.push_back({ Pr::SHADOW_WORD_PAIN, "Pr::SHADOW_WORD_PAIN" });
    spells.push_back({ Pr::SHADOW_WORD_DEATH, "Pr::SHADOW_WORD_DEATH" });
    spells.push_back({ Pr::POWER_WORD_SHIELD, "Pr::POWER_WORD_SHIELD" });
    spells.push_back({ Pr::POWER_WORD_FORTITUDE, "Pr::POWER_WORD_FORTITUDE" });
    spells.push_back({ Pr::FLASH_HEAL, "Pr::FLASH_HEAL" });
    spells.push_back({ Pr::SMITE, "Pr::SMITE" });
    spells.push_back({ Pr::HolyPriest::HOLY_NOVA, "Pr::HolyPriest::HOLY_NOVA" });
    spells.push_back({ Pr::FADE, "Pr::FADE" });
    spells.push_back({ Pr::PSYCHIC_SCREAM, "Pr::PSYCHIC_SCREAM" });
    spells.push_back({ Pr::MASS_DISPEL, "Pr::MASS_DISPEL" });
    spells.push_back({ Pr::LEAP_OF_FAITH, "Pr::LEAP_OF_FAITH" });
    spells.push_back({ Pr::DISPEL_MAGIC, "Pr::DISPEL_MAGIC" });
    spells.push_back({ Pr::MIND_CONTROL, "Pr::MIND_CONTROL" });

    // Discipline
    spells.push_back({ Pr::Discipline::PENANCE, "Pr::Disc::PENANCE" });
    spells.push_back({ Pr::Discipline::POWER_WORD_RADIANCE, "Pr::Disc::POWER_WORD_RADIANCE" });
    spells.push_back({ Pr::Discipline::SCHISM, "Pr::Disc::SCHISM" });
    spells.push_back({ Pr::Discipline::RAPTURE, "Pr::Disc::RAPTURE" });
    spells.push_back({ Pr::Discipline::PAIN_SUPPRESSION, "Pr::Disc::PAIN_SUPPRESSION" });
    spells.push_back({ Pr::Discipline::POWER_WORD_BARRIER, "Pr::Disc::POWER_WORD_BARRIER" });
    spells.push_back({ Pr::Discipline::EVANGELISM, "Pr::Disc::EVANGELISM" });
    spells.push_back({ Pr::Discipline::SPIRIT_SHELL, "Pr::Disc::SPIRIT_SHELL" });

    // Holy (namespace is HolyPriest in the header)
    spells.push_back({ Pr::HolyPriest::RENEW, "Pr::HolyPriest::RENEW" });
    spells.push_back({ Pr::HolyPriest::PRAYER_OF_HEALING, "Pr::HolyPriest::PRAYER_OF_HEALING" });
    spells.push_back({ Pr::HolyPriest::PRAYER_OF_MENDING, "Pr::HolyPriest::PRAYER_OF_MENDING" });
    spells.push_back({ Pr::HolyPriest::CIRCLE_OF_HEALING, "Pr::HolyPriest::CIRCLE_OF_HEALING" });
    spells.push_back({ Pr::HolyPriest::HOLY_WORD_SERENITY, "Pr::HolyPriest::HOLY_WORD_SERENITY" });
    spells.push_back({ Pr::HolyPriest::HOLY_WORD_SANCTIFY, "Pr::HolyPriest::HOLY_WORD_SANCTIFY" });
    spells.push_back({ Pr::HolyPriest::HOLY_WORD_SALVATION, "Pr::HolyPriest::HOLY_WORD_SALVATION" });
    spells.push_back({ Pr::HolyPriest::GUARDIAN_SPIRIT, "Pr::HolyPriest::GUARDIAN_SPIRIT" });
    spells.push_back({ Pr::HolyPriest::DIVINE_HYMN, "Pr::HolyPriest::DIVINE_HYMN" });
    spells.push_back({ Pr::HolyPriest::APOTHEOSIS, "Pr::HolyPriest::APOTHEOSIS" });

    // Shadow (uses _SHADOW suffix for shared spell names)
    spells.push_back({ Pr::Shadow::MIND_BLAST_SHADOW, "Pr::Shadow::MIND_BLAST_SHADOW" });
    spells.push_back({ Pr::Shadow::MIND_FLAY, "Pr::Shadow::MIND_FLAY" });
    spells.push_back({ Pr::Shadow::VAMPIRIC_TOUCH, "Pr::Shadow::VAMPIRIC_TOUCH" });
    spells.push_back({ Pr::Shadow::DEVOURING_PLAGUE, "Pr::Shadow::DEVOURING_PLAGUE" });
    spells.push_back({ Pr::Shadow::VOID_ERUPTION, "Pr::Shadow::VOID_ERUPTION" });
    spells.push_back({ Pr::Shadow::SHADOWFIEND_SHADOW, "Pr::Shadow::SHADOWFIEND_SHADOW" });
    spells.push_back({ Pr::Shadow::SILENCE, "Pr::Shadow::SILENCE" });
    spells.push_back({ Pr::Shadow::MIND_SEAR, "Pr::Shadow::MIND_SEAR" });
    spells.push_back({ Pr::Shadow::SHADOW_CRASH, "Pr::Shadow::SHADOW_CRASH" });
    spells.push_back({ Pr::Shadow::DARK_ASCENSION, "Pr::Shadow::DARK_ASCENSION" });
    spells.push_back({ Pr::Shadow::VOID_TORRENT, "Pr::Shadow::VOID_TORRENT" });
}

// ============================================================================
// Constexpr Spell ID Registration - Rogue
// ============================================================================

void SpellIdValidator::RegisterRogueSpells(std::vector<std::pair<uint32, std::string>>& spells)
{
    namespace Ro = WoW120Spells::Rogue;

    // Core
    spells.push_back({ Ro::KICK, "Ro::KICK" });
    spells.push_back({ Ro::VANISH, "Ro::VANISH" });
    spells.push_back({ Ro::STEALTH, "Ro::STEALTH" });
    spells.push_back({ Ro::EVASION, "Ro::EVASION" });
    spells.push_back({ Ro::SPRINT, "Ro::SPRINT" });
    spells.push_back({ Ro::KIDNEY_SHOT, "Ro::KIDNEY_SHOT" });
    spells.push_back({ Ro::CHEAP_SHOT, "Ro::CHEAP_SHOT" });
    spells.push_back({ Ro::SAP, "Ro::SAP" });
    spells.push_back({ Ro::BLIND, "Ro::BLIND" });
    spells.push_back({ Ro::CLOAK_OF_SHADOWS, "Ro::CLOAK_OF_SHADOWS" });
    spells.push_back({ Ro::CRIMSON_VIAL, "Ro::CRIMSON_VIAL" });
    spells.push_back({ Ro::SHADOWSTEP, "Ro::SHADOWSTEP" });
    spells.push_back({ Ro::TRICKS_OF_THE_TRADE, "Ro::TRICKS_OF_THE_TRADE" });

    // Assassination
    spells.push_back({ Ro::Assassination::MUTILATE, "Ro::Sin::MUTILATE" });
    spells.push_back({ Ro::Assassination::ENVENOM, "Ro::Sin::ENVENOM" });
    spells.push_back({ Ro::Assassination::GARROTE, "Ro::Sin::GARROTE" });
    spells.push_back({ Ro::Assassination::RUPTURE, "Ro::Sin::RUPTURE" });
    spells.push_back({ Ro::Assassination::VENDETTA, "Ro::Sin::VENDETTA" });
    spells.push_back({ Ro::Assassination::KINGSBANE, "Ro::Sin::KINGSBANE" });

    // Outlaw
    spells.push_back({ Ro::Outlaw::SINISTER_STRIKE, "Ro::Out::SINISTER_STRIKE" });
    spells.push_back({ Ro::Outlaw::DISPATCH, "Ro::Out::DISPATCH" });
    spells.push_back({ Ro::Outlaw::BETWEEN_THE_EYES, "Ro::Out::BETWEEN_THE_EYES" });
    spells.push_back({ Ro::Outlaw::BLADE_FLURRY, "Ro::Out::BLADE_FLURRY" });
    spells.push_back({ Ro::Outlaw::ROLL_THE_BONES, "Ro::Out::ROLL_THE_BONES" });
    spells.push_back({ Ro::Outlaw::ADRENALINE_RUSH, "Ro::Out::ADRENALINE_RUSH" });
    spells.push_back({ Ro::Outlaw::KILLING_SPREE, "Ro::Out::KILLING_SPREE" });

    // Subtlety
    spells.push_back({ Ro::Subtlety::BACKSTAB, "Ro::Sub::BACKSTAB" });
    spells.push_back({ Ro::Subtlety::EVISCERATE, "Ro::Sub::EVISCERATE" });
    spells.push_back({ Ro::Subtlety::SHADOW_DANCE, "Ro::Sub::SHADOW_DANCE" });
    spells.push_back({ Ro::Subtlety::SYMBOLS_OF_DEATH, "Ro::Sub::SYMBOLS_OF_DEATH" });
    spells.push_back({ Ro::Subtlety::SECRET_TECHNIQUE, "Ro::Sub::SECRET_TECHNIQUE" });
    spells.push_back({ Ro::Subtlety::SHURIKEN_STORM, "Ro::Sub::SHURIKEN_STORM" });
    spells.push_back({ Ro::Subtlety::SHADOW_BLADES, "Ro::Sub::SHADOW_BLADES" });
}

// ============================================================================
// Constexpr Spell ID Registration - Shaman
// ============================================================================

void SpellIdValidator::RegisterShamanSpells(std::vector<std::pair<uint32, std::string>>& spells)
{
    namespace Sh = WoW120Spells::Shaman;

    // Core
    spells.push_back({ Sh::LIGHTNING_BOLT, "Sh::LIGHTNING_BOLT" });
    spells.push_back({ Sh::CHAIN_LIGHTNING, "Sh::CHAIN_LIGHTNING" });
    spells.push_back({ Sh::FLAME_SHOCK, "Sh::FLAME_SHOCK" });
    spells.push_back({ Sh::FROST_SHOCK, "Sh::FROST_SHOCK" });
    spells.push_back({ Sh::Restoration::HEALING_SURGE, "Sh::Resto::HEALING_SURGE" });
    spells.push_back({ Sh::ASTRAL_SHIFT, "Sh::ASTRAL_SHIFT" });
    spells.push_back({ Sh::WIND_SHEAR, "Sh::WIND_SHEAR" });
    spells.push_back({ Sh::HEX, "Sh::HEX" });
    spells.push_back({ Sh::HEROISM, "Sh::HEROISM" });
    spells.push_back({ Sh::BLOODLUST, "Sh::BLOODLUST" });
    spells.push_back({ Sh::GHOST_WOLF, "Sh::GHOST_WOLF" });
    spells.push_back({ Sh::PURGE, "Sh::PURGE" });
    spells.push_back({ Sh::Restoration::EARTH_ELEMENTAL, "Sh::Resto::EARTH_ELEMENTAL" });
    spells.push_back({ Sh::CAPACITOR_TOTEM, "Sh::CAPACITOR_TOTEM" });

    // Elemental
    spells.push_back({ Sh::EARTH_SHOCK, "Sh::EARTH_SHOCK" });
    spells.push_back({ Sh::Elemental::EARTHQUAKE, "Sh::Ele::EARTHQUAKE" });
    spells.push_back({ Sh::LAVA_BURST, "Sh::LAVA_BURST" });
    spells.push_back({ Sh::Elemental::FIRE_ELEMENTAL, "Sh::Ele::FIRE_ELEMENTAL" });
    spells.push_back({ Sh::Elemental::STORMKEEPER, "Sh::Ele::STORMKEEPER" });
    spells.push_back({ Sh::Elemental::ICEFURY, "Sh::Ele::ICEFURY" });

    // Enhancement
    spells.push_back({ Sh::Enhancement::STORMSTRIKE, "Sh::Enh::STORMSTRIKE" });
    spells.push_back({ Sh::Enhancement::LAVA_LASH, "Sh::Enh::LAVA_LASH" });
    spells.push_back({ Sh::Enhancement::CRASH_LIGHTNING, "Sh::Enh::CRASH_LIGHTNING" });
    spells.push_back({ Sh::Enhancement::FERAL_SPIRIT, "Sh::Enh::FERAL_SPIRIT" });
    spells.push_back({ Sh::Enhancement::WINDFURY_TOTEM, "Sh::Enh::WINDFURY_TOTEM" });
    spells.push_back({ Sh::Enhancement::SUNDERING, "Sh::Enh::SUNDERING" });

    // Restoration
    spells.push_back({ Sh::Restoration::RIPTIDE, "Sh::Resto::RIPTIDE" });
    spells.push_back({ Sh::Restoration::HEALING_WAVE, "Sh::Resto::HEALING_WAVE" });
    spells.push_back({ Sh::Restoration::CHAIN_HEAL, "Sh::Resto::CHAIN_HEAL" });
    spells.push_back({ Sh::Restoration::HEALING_RAIN, "Sh::Resto::HEALING_RAIN" });
    spells.push_back({ Sh::Restoration::SPIRIT_LINK_TOTEM, "Sh::Resto::SPIRIT_LINK_TOTEM" });
    spells.push_back({ Sh::Restoration::HEALING_TIDE_TOTEM, "Sh::Resto::HEALING_TIDE_TOTEM" });
    spells.push_back({ Sh::Restoration::MANA_TIDE_TOTEM, "Sh::Resto::MANA_TIDE_TOTEM" });
    spells.push_back({ Sh::Restoration::CLOUDBURST_TOTEM, "Sh::Resto::CLOUDBURST_TOTEM" });
}

// ============================================================================
// Constexpr Spell ID Registration - Warlock
// ============================================================================

void SpellIdValidator::RegisterWarlockSpells(std::vector<std::pair<uint32, std::string>>& spells)
{
    namespace Wl = WoW120Spells::Warlock;

    // Core
    spells.push_back({ Wl::SHADOW_BOLT, "Wl::SHADOW_BOLT" });
    spells.push_back({ Wl::CORRUPTION, "Wl::CORRUPTION" });
    spells.push_back({ Wl::DRAIN_LIFE, "Wl::DRAIN_LIFE" });
    spells.push_back({ Wl::FEAR, "Wl::FEAR" });
    spells.push_back({ Wl::HEALTH_FUNNEL, "Wl::HEALTH_FUNNEL" });
    spells.push_back({ Wl::CREATE_HEALTHSTONE, "Wl::CREATE_HEALTHSTONE" });
    spells.push_back({ Wl::UNENDING_RESOLVE, "Wl::UNENDING_RESOLVE" });
    spells.push_back({ Wl::BANISH, "Wl::BANISH" });
    spells.push_back({ Wl::MORTAL_COIL, "Wl::MORTAL_COIL" });
    spells.push_back({ Wl::SOULSTONE, "Wl::SOULSTONE" });
    spells.push_back({ Wl::DEMONIC_GATEWAY, "Wl::DEMONIC_GATEWAY" });
    spells.push_back({ Wl::RITUAL_OF_SUMMONING, "Wl::RITUAL_OF_SUMMONING" });

    // Affliction
    spells.push_back({ Wl::Affliction::AGONY, "Wl::Aff::AGONY" });
    spells.push_back({ Wl::Affliction::UNSTABLE_AFFLICTION, "Wl::Aff::UNSTABLE_AFFLICTION" });
    spells.push_back({ Wl::Affliction::SEED_OF_CORRUPTION, "Wl::Aff::SEED_OF_CORRUPTION" });
    spells.push_back({ Wl::Affliction::MALEFIC_RAPTURE, "Wl::Aff::MALEFIC_RAPTURE" });
    spells.push_back({ Wl::Affliction::SUMMON_DARKGLARE, "Wl::Aff::SUMMON_DARKGLARE" });
    spells.push_back({ Wl::Affliction::PHANTOM_SINGULARITY, "Wl::Aff::PHANTOM_SINGULARITY" });
    spells.push_back({ Wl::Affliction::VILE_TAINT, "Wl::Aff::VILE_TAINT" });
    spells.push_back({ Wl::Affliction::SOUL_ROT, "Wl::Aff::SOUL_ROT" });
    spells.push_back({ Wl::Affliction::HAUNT, "Wl::Aff::HAUNT" });
    spells.push_back({ Wl::Affliction::SIPHON_LIFE, "Wl::Aff::SIPHON_LIFE" });

    // Demonology
    spells.push_back({ Wl::Demonology::HAND_OF_GULDAN, "Wl::Demo::HAND_OF_GULDAN" });
    spells.push_back({ Wl::Demonology::CALL_DREADSTALKERS, "Wl::Demo::CALL_DREADSTALKERS" });
    spells.push_back({ Wl::Demonology::DEMONBOLT, "Wl::Demo::DEMONBOLT" });
    spells.push_back({ Wl::Demonology::SUMMON_DEMONIC_TYRANT, "Wl::Demo::SUMMON_DEMONIC_TYRANT" });
    spells.push_back({ Wl::Demonology::IMPLOSION, "Wl::Demo::IMPLOSION" });
    spells.push_back({ Wl::Demonology::POWER_SIPHON, "Wl::Demo::POWER_SIPHON" });
    spells.push_back({ Wl::Demonology::GRIMOIRE_FELGUARD, "Wl::Demo::GRIMOIRE_FELGUARD" });
    spells.push_back({ Wl::Demonology::SOUL_STRIKE, "Wl::Demo::SOUL_STRIKE" });
    spells.push_back({ Wl::Demonology::NETHER_PORTAL, "Wl::Demo::NETHER_PORTAL" });
    spells.push_back({ Wl::Demonology::SUMMON_VILEFIEND, "Wl::Demo::SUMMON_VILEFIEND" });

    // Destruction
    spells.push_back({ Wl::Destruction::INCINERATE, "Wl::Dest::INCINERATE" });
    spells.push_back({ Wl::Destruction::CHAOS_BOLT, "Wl::Dest::CHAOS_BOLT" });
    spells.push_back({ Wl::Destruction::IMMOLATE, "Wl::Dest::IMMOLATE" });
    spells.push_back({ Wl::Destruction::CONFLAGRATE, "Wl::Dest::CONFLAGRATE" });
    spells.push_back({ Wl::Destruction::RAIN_OF_FIRE, "Wl::Dest::RAIN_OF_FIRE" });
    spells.push_back({ Wl::Destruction::HAVOC, "Wl::Dest::HAVOC" });
    spells.push_back({ Wl::Destruction::SUMMON_INFERNAL, "Wl::Dest::SUMMON_INFERNAL" });
    spells.push_back({ Wl::Destruction::CHANNEL_DEMONFIRE, "Wl::Dest::CHANNEL_DEMONFIRE" });
}

// ============================================================================
// Constexpr Spell ID Registration - Warrior
// ============================================================================

void SpellIdValidator::RegisterWarriorSpells(std::vector<std::pair<uint32, std::string>>& spells)
{
    namespace Wr = WoW120Spells::Warrior;

    // Core
    spells.push_back({ Wr::CHARGE, "Wr::CHARGE" });
    spells.push_back({ Wr::Arms::EXECUTE, "Wr::Arms::EXECUTE" });
    spells.push_back({ Wr::HEROIC_LEAP, "Wr::HEROIC_LEAP" });
    spells.push_back({ Wr::HEROIC_THROW, "Wr::HEROIC_THROW" });
    spells.push_back({ Wr::PUMMEL, "Wr::PUMMEL" });
    spells.push_back({ Wr::RALLYING_CRY, "Wr::RALLYING_CRY" });
    spells.push_back({ Wr::INTIMIDATING_SHOUT, "Wr::INTIMIDATING_SHOUT" });
    spells.push_back({ Wr::SPELL_REFLECTION, "Wr::SPELL_REFLECTION" });
    spells.push_back({ Wr::BERSERKER_RAGE, "Wr::BERSERKER_RAGE" });
    spells.push_back({ Wr::BATTLE_SHOUT, "Wr::BATTLE_SHOUT" });
    spells.push_back({ Wr::Arms::WHIRLWIND, "Wr::Arms::WHIRLWIND" });
    spells.push_back({ Wr::HAMSTRING, "Wr::HAMSTRING" });
    spells.push_back({ Wr::PIERCING_HOWL, "Wr::PIERCING_HOWL" });
    spells.push_back({ Wr::STORM_BOLT, "Wr::STORM_BOLT" });
    spells.push_back({ Wr::VICTORY_RUSH, "Wr::VICTORY_RUSH" });

    // Arms
    spells.push_back({ Wr::Arms::MORTAL_STRIKE, "Wr::Arms::MORTAL_STRIKE" });
    spells.push_back({ Wr::Arms::OVERPOWER, "Wr::Arms::OVERPOWER" });
    spells.push_back({ Wr::Arms::SLAM, "Wr::Arms::SLAM" });
    spells.push_back({ Wr::Arms::COLOSSUS_SMASH, "Wr::Arms::COLOSSUS_SMASH" });
    spells.push_back({ Wr::Arms::BLADESTORM, "Wr::Arms::BLADESTORM" });
    spells.push_back({ Wr::Arms::SWEEPING_STRIKES, "Wr::Arms::SWEEPING_STRIKES" });
    spells.push_back({ Wr::Arms::DIE_BY_THE_SWORD, "Wr::Arms::DIE_BY_THE_SWORD" });
    spells.push_back({ Wr::Arms::WARBREAKER, "Wr::Arms::WARBREAKER" });
    spells.push_back({ Wr::Arms::CLEAVE, "Wr::Arms::CLEAVE" });
    spells.push_back({ Wr::Arms::SKULLSPLITTER, "Wr::Arms::SKULLSPLITTER" });
    spells.push_back({ Wr::Arms::AVATAR, "Wr::Arms::AVATAR" });
    spells.push_back({ Wr::Arms::THUNDEROUS_ROAR, "Wr::Arms::THUNDEROUS_ROAR" });

    // Fury
    spells.push_back({ Wr::Fury::BLOODTHIRST, "Wr::Fury::BLOODTHIRST" });
    spells.push_back({ Wr::Fury::RAGING_BLOW, "Wr::Fury::RAGING_BLOW" });
    spells.push_back({ Wr::Fury::RAMPAGE, "Wr::Fury::RAMPAGE" });
    spells.push_back({ Wr::Fury::ENRAGE, "Wr::Fury::ENRAGE" });
    spells.push_back({ Wr::Fury::RECKLESSNESS, "Wr::Fury::RECKLESSNESS" });
    spells.push_back({ Wr::Fury::ODYN_FURY, "Wr::Fury::ODYN_FURY" });
    spells.push_back({ Wr::Fury::RAVAGER, "Wr::Fury::RAVAGER" });

    // Protection
    spells.push_back({ Wr::Protection::SHIELD_SLAM, "Wr::Prot::SHIELD_SLAM" });
    spells.push_back({ Wr::Protection::THUNDER_CLAP, "Wr::Prot::THUNDER_CLAP" });
    spells.push_back({ Wr::Protection::SHIELD_BLOCK, "Wr::Prot::SHIELD_BLOCK" });
    spells.push_back({ Wr::Protection::IGNORE_PAIN, "Wr::Prot::IGNORE_PAIN" });
    spells.push_back({ Wr::Protection::SHIELD_WALL, "Wr::Prot::SHIELD_WALL" });
    spells.push_back({ Wr::Protection::LAST_STAND, "Wr::Prot::LAST_STAND" });
    spells.push_back({ Wr::Protection::REVENGE, "Wr::Prot::REVENGE" });
    spells.push_back({ Wr::Protection::DEVASTATE, "Wr::Prot::DEVASTATE" });
    spells.push_back({ Wr::Protection::DEMORALIZING_SHOUT, "Wr::Prot::DEMORALIZING_SHOUT" });
    spells.push_back({ Wr::Protection::AVATAR, "Wr::Prot::AVATAR" });
}

} // namespace Playerbot
