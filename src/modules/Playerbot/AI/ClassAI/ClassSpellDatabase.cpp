/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * ClassSpellDatabase: Centralized static read-only database of per-class/spec
 * spell metadata. Follows the InterruptDatabase pattern.
 */

#include "ClassSpellDatabase.h"
#include "SpellIdValidator.h"
#include "SpellValidation_WoW120.h"
#include "SpellValidation_WoW120_Part2.h"
#include "Log.h"

namespace Playerbot
{

// ============================================================================
// Static Member Definitions
// ============================================================================

std::unordered_map<ClassSpec, SpecRotationTemplate, ClassSpecHash> ClassSpellDatabase::_rotations;
std::unordered_map<ClassSpec, SpecStatWeights, ClassSpecHash> ClassSpellDatabase::_statWeights;
std::unordered_map<ClassSpec, std::vector<DefensiveSpellEntry>, ClassSpecHash> ClassSpellDatabase::_defensiveSpells;
std::unordered_map<ClassSpec, std::vector<CooldownSpellEntry>, ClassSpecHash> ClassSpellDatabase::_cooldownSpells;
std::unordered_map<ClassSpec, std::vector<HealingTierEntry>, ClassSpecHash> ClassSpellDatabase::_healingTiers;
std::unordered_map<ClassSpec, std::vector<FallbackChainEntry>, ClassSpecHash> ClassSpellDatabase::_fallbackChains;
std::unordered_map<ClassSpec, std::vector<uint32>, ClassSpecHash> ClassSpellDatabase::_interruptSpells;
std::unordered_map<ClassSpec, uint32, ClassSpecHash> ClassSpellDatabase::_primaryInterrupts;
bool ClassSpellDatabase::_initialized = false;

// ============================================================================
// Namespace aliases for brevity
// ============================================================================

namespace DK = WoW120Spells::DeathKnight;
namespace DH = WoW120Spells::DemonHunter;
namespace Dr = WoW120Spells::Druid;
namespace Ev = WoW120Spells::Evoker;
namespace Hu = WoW120Spells::Hunter;
namespace Ma = WoW120Spells::Mage;
namespace Mo = WoW120Spells::Monk;
namespace Pa = WoW120Spells::Paladin;
namespace Pr = WoW120Spells::Priest;
namespace Ro = WoW120Spells::Rogue;
namespace Sh = WoW120Spells::Shaman;
namespace Wl = WoW120Spells::Warlock;
namespace Wr = WoW120Spells::Warrior;

// ============================================================================
// Public API
// ============================================================================

void ClassSpellDatabase::Initialize()
{
    if (_initialized)
        return;

    TC_LOG_INFO("module.playerbot", "ClassSpellDatabase: Initializing spell data for all 13 classes...");

    InitializeDeathKnight();
    InitializeDemonHunter();
    InitializeDruid();
    InitializeEvoker();
    InitializeHunter();
    InitializeMage();
    InitializeMonk();
    InitializePaladin();
    InitializePriest();
    InitializeRogue();
    InitializeShaman();
    InitializeWarlock();
    InitializeWarrior();

    _initialized = true;

    TC_LOG_INFO("module.playerbot", "ClassSpellDatabase: Initialized %zu rotation templates, %zu stat weights, "
        "%zu defensive spell sets, %zu cooldown spell sets, %zu healing tier sets, %zu fallback chains",
        _rotations.size(), _statWeights.size(), _defensiveSpells.size(),
        _cooldownSpells.size(), _healingTiers.size(), _fallbackChains.size());

    // Validate all spell IDs against SpellDB at startup
    SpellIdValidator::ValidateAll();
}

void ClassSpellDatabase::EnsureInitialized()
{
    if (!_initialized)
        Initialize();
}

SpecRotationTemplate const* ClassSpellDatabase::GetRotationTemplate(WowClass classId, uint8 specId)
{
    EnsureInitialized();
    auto it = _rotations.find({classId, specId});
    return it != _rotations.end() ? &it->second : nullptr;
}

std::vector<RotationSpell> const* ClassSpellDatabase::GetPhaseSpells(
    WowClass classId, uint8 specId, ActionPriority phase)
{
    EnsureInitialized();
    auto const* tmpl = GetRotationTemplate(classId, specId);
    if (!tmpl)
        return nullptr;

    for (auto const& p : tmpl->phases)
        if (p.priority == phase)
            return &p.spells;
    return nullptr;
}

SpecStatWeights const* ClassSpellDatabase::GetStatWeights(WowClass classId, uint8 specId)
{
    EnsureInitialized();
    auto it = _statWeights.find({classId, specId});
    return it != _statWeights.end() ? &it->second : nullptr;
}

SpellStatType ClassSpellDatabase::GetPrimaryStat(WowClass classId, uint8 specId)
{
    auto const* w = GetStatWeights(classId, specId);
    if (!w)
        return SpellStatType::STRENGTH;

    float best = -1.0f;
    SpellStatType bestStat = SpellStatType::STRENGTH;
    // Only check primary stats (STR, AGI, INT)
    for (uint8 i = 0; i < 3; ++i)
    {
        if (w->weights[i] > best)
        {
            best = w->weights[i];
            bestStat = static_cast<SpellStatType>(i);
        }
    }
    return bestStat;
}

std::vector<SpellStatType> ClassSpellDatabase::GetSecondaryStatPriority(WowClass classId, uint8 specId)
{
    std::vector<SpellStatType> result;
    auto const* w = GetStatWeights(classId, specId);
    if (!w)
        return result;

    // Collect secondary stats (indices 4-7: Crit, Haste, Mastery, Vers)
    struct Pair { SpellStatType stat; float weight; };
    std::vector<Pair> secondaries;
    for (uint8 i = static_cast<uint8>(SpellStatType::CRITICAL_STRIKE);
         i <= static_cast<uint8>(SpellStatType::VERSATILITY); ++i)
    {
        if (w->weights[i] > 0.0f)
            secondaries.push_back({static_cast<SpellStatType>(i), w->weights[i]});
    }

    std::sort(secondaries.begin(), secondaries.end(),
        [](Pair const& a, Pair const& b) { return a.weight > b.weight; });

    for (auto const& s : secondaries)
        result.push_back(s.stat);
    return result;
}

std::vector<DefensiveSpellEntry> const* ClassSpellDatabase::GetDefensiveSpells(
    WowClass classId, uint8 specId)
{
    EnsureInitialized();
    auto it = _defensiveSpells.find({classId, specId});
    return it != _defensiveSpells.end() ? &it->second : nullptr;
}

std::vector<DefensiveSpellEntry> ClassSpellDatabase::GetDefensiveSpellsByCategory(
    WowClass classId, uint8 specId, DefensiveCategory category)
{
    std::vector<DefensiveSpellEntry> result;
    auto const* all = GetDefensiveSpells(classId, specId);
    if (!all)
        return result;

    for (auto const& e : *all)
        if (e.category == category)
            result.push_back(e);
    return result;
}

DefensiveSpellEntry const* ClassSpellDatabase::GetDefensiveForHealth(
    WowClass classId, uint8 specId, float healthPct)
{
    auto const* all = GetDefensiveSpells(classId, specId);
    if (!all)
        return nullptr;

    DefensiveSpellEntry const* best = nullptr;
    float bestThreshold = 0.0f;

    for (auto const& e : *all)
    {
        if (e.healthThreshold > 0.0f && healthPct <= e.healthThreshold)
        {
            // Pick the one with the highest threshold that matches (closest to current health)
            if (!best || e.healthThreshold > bestThreshold)
            {
                best = &e;
                bestThreshold = e.healthThreshold;
            }
        }
    }
    return best;
}

std::vector<CooldownSpellEntry> const* ClassSpellDatabase::GetCooldownSpells(
    WowClass classId, uint8 specId)
{
    EnsureInitialized();
    auto it = _cooldownSpells.find({classId, specId});
    return it != _cooldownSpells.end() ? &it->second : nullptr;
}

std::vector<CooldownSpellEntry> ClassSpellDatabase::GetCooldownSpellsByCategory(
    WowClass classId, uint8 specId, CooldownCategory category)
{
    std::vector<CooldownSpellEntry> result;
    auto const* all = GetCooldownSpells(classId, specId);
    if (!all)
        return result;

    for (auto const& e : *all)
        if (e.category == category)
            result.push_back(e);
    return result;
}

std::vector<HealingTierEntry> const* ClassSpellDatabase::GetHealingTiers(
    WowClass classId, uint8 specId)
{
    EnsureInitialized();
    auto it = _healingTiers.find({classId, specId});
    return it != _healingTiers.end() ? &it->second : nullptr;
}

SpellEfficiencyTier ClassSpellDatabase::GetSpellTier(WowClass classId, uint8 specId, uint32 spellId)
{
    auto const* tiers = GetHealingTiers(classId, specId);
    if (!tiers)
        return SpellEfficiencyTier::MEDIUM;

    for (auto const& e : *tiers)
        if (e.spellId == spellId)
            return e.tier;
    return SpellEfficiencyTier::MEDIUM;
}

std::vector<FallbackChainEntry> const* ClassSpellDatabase::GetFallbackChains(
    WowClass classId, uint8 specId)
{
    EnsureInitialized();
    auto it = _fallbackChains.find({classId, specId});
    return it != _fallbackChains.end() ? &it->second : nullptr;
}

FallbackChainEntry const* ClassSpellDatabase::GetFallbackChain(
    WowClass classId, uint8 specId, std::string const& chainName)
{
    auto const* chains = GetFallbackChains(classId, specId);
    if (!chains)
        return nullptr;

    for (auto const& c : *chains)
        if (c.chainName == chainName)
            return &c;
    return nullptr;
}

uint32 ClassSpellDatabase::GetPrimaryInterrupt(WowClass classId, uint8 specId)
{
    EnsureInitialized();
    auto it = _primaryInterrupts.find({classId, specId});
    return it != _primaryInterrupts.end() ? it->second : 0;
}

std::vector<uint32> const* ClassSpellDatabase::GetInterruptSpells(WowClass classId, uint8 specId)
{
    EnsureInitialized();
    auto it = _interruptSpells.find({classId, specId});
    return it != _interruptSpells.end() ? &it->second : nullptr;
}

// ============================================================================
// Helper macros for concise initialization
// ============================================================================

#define SPEC(cls, id) ClassSpec{WowClass::cls, id}

// ============================================================================
// Death Knight Initialization (Blood=0, Frost=1, Unholy=2)
// ============================================================================

void ClassSpellDatabase::InitializeDeathKnight()
{
    // --- Blood (Tank) ---
    {
        auto spec = SPEC(DEATH_KNIGHT, 0);

        // Stat weights
        SpecStatWeights sw;
        sw.spec = spec;
        sw.SetWeight(SpellStatType::STRENGTH, 1.0f);
        sw.SetWeight(SpellStatType::STAMINA, 0.9f);
        sw.SetWeight(SpellStatType::HASTE, 0.85f);
        sw.SetWeight(SpellStatType::VERSATILITY, 0.8f);
        sw.SetWeight(SpellStatType::CRITICAL_STRIKE, 0.65f);
        sw.SetWeight(SpellStatType::MASTERY, 0.6f);
        _statWeights[spec] = sw;

        // Defensive spells
        _defensiveSpells[spec] = {
            {DK::ICEBOUND_FORTITUDE, DefensiveCategory::PERSONAL_MAJOR, 40.0f, 180.0f, "Icebound Fortitude"},
            {DK::Blood::VAMPIRIC_BLOOD, DefensiveCategory::PERSONAL_MAJOR, 50.0f, 90.0f, "Vampiric Blood"},
            {DK::ANTI_MAGIC_SHELL, DefensiveCategory::PERSONAL_MINOR, 60.0f, 60.0f, "Anti-Magic Shell"},
            {DK::Blood::RUNE_TAP, DefensiveCategory::PERSONAL_MINOR, 70.0f, 25.0f, "Rune Tap"},
            {DK::Blood::DANCING_RUNE_WEAPON, DefensiveCategory::PERSONAL_MAJOR, 55.0f, 120.0f, "Dancing Rune Weapon"},
            {DK::DEATH_STRIKE, DefensiveCategory::SELF_HEAL, 80.0f, 0.0f, "Death Strike"},
        };

        // Cooldown spells
        _cooldownSpells[spec] = {
            {DK::Blood::DANCING_RUNE_WEAPON, CooldownCategory::OFFENSIVE_MAJOR, 120.0f, true, "Dancing Rune Weapon"},
        };

        // Interrupt spells
        _primaryInterrupts[spec] = DK::MIND_FREEZE;
        _interruptSpells[spec] = {DK::MIND_FREEZE, DK::ASPHYXIATE};
    }

    // --- Frost (Melee DPS) ---
    {
        auto spec = SPEC(DEATH_KNIGHT, 1);

        SpecStatWeights sw;
        sw.spec = spec;
        sw.SetWeight(SpellStatType::STRENGTH, 1.0f);
        sw.SetWeight(SpellStatType::MASTERY, 0.9f);
        sw.SetWeight(SpellStatType::CRITICAL_STRIKE, 0.85f);
        sw.SetWeight(SpellStatType::HASTE, 0.75f);
        sw.SetWeight(SpellStatType::VERSATILITY, 0.7f);
        _statWeights[spec] = sw;

        _defensiveSpells[spec] = {
            {DK::ICEBOUND_FORTITUDE, DefensiveCategory::PERSONAL_MAJOR, 35.0f, 180.0f, "Icebound Fortitude"},
            {DK::ANTI_MAGIC_SHELL, DefensiveCategory::PERSONAL_MINOR, 60.0f, 60.0f, "Anti-Magic Shell"},
            {DK::DEATH_STRIKE, DefensiveCategory::SELF_HEAL, 50.0f, 0.0f, "Death Strike"},
        };

        _cooldownSpells[spec] = {
            {DK::Frost::PILLAR_OF_FROST, CooldownCategory::OFFENSIVE_MAJOR, 60.0f, true, "Pillar of Frost"},
            {DK::Frost::EMPOWER_RUNE_WEAPON, CooldownCategory::RESOURCE, 120.0f, false, "Empower Rune Weapon"},
            {DK::Frost::FROSTWYRMS_FURY, CooldownCategory::OFFENSIVE_MINOR, 180.0f, false, "Frostwyrm's Fury"},
        };

        _primaryInterrupts[spec] = DK::MIND_FREEZE;
        _interruptSpells[spec] = {DK::MIND_FREEZE, DK::ASPHYXIATE};

        _fallbackChains[spec] = {
            {"single_target", {DK::Frost::OBLITERATE, DK::Frost::FROST_STRIKE, DK::Frost::HOWLING_BLAST}},
            {"aoe", {DK::Frost::REMORSELESS_WINTER, DK::Frost::FROSTSCYTHE, DK::Frost::GLACIAL_ADVANCE, DK::Frost::HOWLING_BLAST}},
        };
    }

    // --- Unholy (Melee DPS) ---
    {
        auto spec = SPEC(DEATH_KNIGHT, 2);

        SpecStatWeights sw;
        sw.spec = spec;
        sw.SetWeight(SpellStatType::STRENGTH, 1.0f);
        sw.SetWeight(SpellStatType::MASTERY, 0.9f);
        sw.SetWeight(SpellStatType::HASTE, 0.85f);
        sw.SetWeight(SpellStatType::CRITICAL_STRIKE, 0.75f);
        sw.SetWeight(SpellStatType::VERSATILITY, 0.7f);
        _statWeights[spec] = sw;

        _defensiveSpells[spec] = {
            {DK::ICEBOUND_FORTITUDE, DefensiveCategory::PERSONAL_MAJOR, 35.0f, 180.0f, "Icebound Fortitude"},
            {DK::ANTI_MAGIC_SHELL, DefensiveCategory::PERSONAL_MINOR, 60.0f, 60.0f, "Anti-Magic Shell"},
            {DK::DEATH_STRIKE, DefensiveCategory::SELF_HEAL, 50.0f, 0.0f, "Death Strike"},
        };

        _cooldownSpells[spec] = {
            {DK::Unholy::DARK_TRANSFORMATION, CooldownCategory::OFFENSIVE_MAJOR, 60.0f, true, "Dark Transformation"},
            {DK::Unholy::APOCALYPSE, CooldownCategory::OFFENSIVE_MAJOR, 75.0f, false, "Apocalypse"},
            {DK::Unholy::ARMY_OF_THE_DEAD, CooldownCategory::OFFENSIVE_MAJOR, 480.0f, false, "Army of the Dead"},
            {DK::Unholy::SUMMON_GARGOYLE, CooldownCategory::OFFENSIVE_MINOR, 180.0f, false, "Summon Gargoyle"},
            {DK::Unholy::UNHOLY_ASSAULT, CooldownCategory::OFFENSIVE_MINOR, 90.0f, true, "Unholy Assault"},
        };

        _primaryInterrupts[spec] = DK::MIND_FREEZE;
        _interruptSpells[spec] = {DK::MIND_FREEZE, DK::ASPHYXIATE};

        _fallbackChains[spec] = {
            {"single_target", {DK::Unholy::SCOURGE_STRIKE, DK::Unholy::FESTERING_STRIKE, DK::Unholy::DEATH_COIL_UNHOLY}},
            {"aoe", {DK::Unholy::EPIDEMIC, DK::Unholy::SCOURGE_STRIKE, DK::DEATH_AND_DECAY}},
        };
    }
}

// ============================================================================
// Demon Hunter Initialization (Havoc=0, Vengeance=1)
// ============================================================================

void ClassSpellDatabase::InitializeDemonHunter()
{
    // --- Havoc (Melee DPS) ---
    {
        auto spec = SPEC(DEMON_HUNTER, 0);

        SpecStatWeights sw;
        sw.spec = spec;
        sw.SetWeight(SpellStatType::AGILITY, 1.0f);
        sw.SetWeight(SpellStatType::CRITICAL_STRIKE, 0.85f);
        sw.SetWeight(SpellStatType::HASTE, 0.8f);
        sw.SetWeight(SpellStatType::VERSATILITY, 0.75f);
        sw.SetWeight(SpellStatType::MASTERY, 0.7f);
        _statWeights[spec] = sw;

        _defensiveSpells[spec] = {
            {DH::BLUR, DefensiveCategory::PERSONAL_MAJOR, 40.0f, 60.0f, "Blur"},
            {DH::DARKNESS, DefensiveCategory::RAID_WIDE, 50.0f, 300.0f, "Darkness"},
            {DH::NETHERWALK, DefensiveCategory::PERSONAL_MAJOR, 30.0f, 180.0f, "Netherwalk"},
        };

        _cooldownSpells[spec] = {
            {DH::METAMORPHOSIS_HAVOC, CooldownCategory::OFFENSIVE_MAJOR, 240.0f, false, "Metamorphosis"},
            {DH::Havoc::EYE_BEAM, CooldownCategory::OFFENSIVE_MINOR, 30.0f, true, "Eye Beam"},
            {DH::Havoc::ESSENCE_BREAK, CooldownCategory::OFFENSIVE_MINOR, 40.0f, false, "Essence Break"},
            {DH::Havoc::THE_HUNT, CooldownCategory::OFFENSIVE_MINOR, 90.0f, false, "The Hunt"},
        };

        _primaryInterrupts[spec] = DH::DISRUPT;
        _interruptSpells[spec] = {DH::DISRUPT, DH::CHAOS_NOVA};

        _fallbackChains[spec] = {
            {"single_target", {DH::Havoc::CHAOS_STRIKE, DH::Havoc::DEMONS_BITE, DH::THROW_GLAIVE}},
            {"aoe", {DH::Havoc::BLADE_DANCE, DH::Havoc::EYE_BEAM, DH::Havoc::FEL_BARRAGE, DH::Havoc::IMMOLATION_AURA}},
        };
    }

    // --- Vengeance (Tank) ---
    {
        auto spec = SPEC(DEMON_HUNTER, 1);

        SpecStatWeights sw;
        sw.spec = spec;
        sw.SetWeight(SpellStatType::AGILITY, 1.0f);
        sw.SetWeight(SpellStatType::STAMINA, 0.9f);
        sw.SetWeight(SpellStatType::HASTE, 0.85f);
        sw.SetWeight(SpellStatType::VERSATILITY, 0.8f);
        sw.SetWeight(SpellStatType::CRITICAL_STRIKE, 0.7f);
        sw.SetWeight(SpellStatType::MASTERY, 0.65f);
        _statWeights[spec] = sw;

        _defensiveSpells[spec] = {
            {DH::Vengeance::DEMON_SPIKES, DefensiveCategory::PERSONAL_MINOR, 80.0f, 20.0f, "Demon Spikes"},
            {DH::Vengeance::FIERY_BRAND, DefensiveCategory::PERSONAL_MAJOR, 50.0f, 60.0f, "Fiery Brand"},
            {DH::METAMORPHOSIS_VENGEANCE, DefensiveCategory::PERSONAL_MAJOR, 30.0f, 180.0f, "Metamorphosis"},
            {DH::Vengeance::FEL_DEVASTATION, DefensiveCategory::SELF_HEAL, 60.0f, 60.0f, "Fel Devastation"},
        };

        _cooldownSpells[spec] = {
            {DH::METAMORPHOSIS_VENGEANCE, CooldownCategory::OFFENSIVE_MAJOR, 180.0f, false, "Metamorphosis"},
        };

        _primaryInterrupts[spec] = DH::DISRUPT;
        _interruptSpells[spec] = {DH::DISRUPT, DH::SIGIL_OF_SILENCE, DH::CHAOS_NOVA};
    }
}

// ============================================================================
// Druid Initialization (Balance=0, Feral=1, Guardian=2, Restoration=3)
// ============================================================================

void ClassSpellDatabase::InitializeDruid()
{
    // --- Balance (Ranged DPS) ---
    {
        auto spec = SPEC(DRUID, 0);

        SpecStatWeights sw;
        sw.spec = spec;
        sw.SetWeight(SpellStatType::INTELLECT, 1.0f);
        sw.SetWeight(SpellStatType::MASTERY, 0.85f);
        sw.SetWeight(SpellStatType::HASTE, 0.8f);
        sw.SetWeight(SpellStatType::CRITICAL_STRIKE, 0.75f);
        sw.SetWeight(SpellStatType::VERSATILITY, 0.7f);
        _statWeights[spec] = sw;

        _defensiveSpells[spec] = {
            {Dr::BARKSKIN, DefensiveCategory::PERSONAL_MINOR, 60.0f, 60.0f, "Barkskin"},
            {Dr::SURVIVAL_INSTINCTS, DefensiveCategory::PERSONAL_MAJOR, 30.0f, 180.0f, "Survival Instincts"},
        };

        _cooldownSpells[spec] = {
            {Dr::Balance::CELESTIAL_ALIGNMENT, CooldownCategory::OFFENSIVE_MAJOR, 180.0f, false, "Celestial Alignment"},
            {Dr::Balance::FURY_OF_ELUNE, CooldownCategory::OFFENSIVE_MINOR, 60.0f, true, "Fury of Elune"},
            {Dr::Balance::WARRIOR_OF_ELUNE, CooldownCategory::OFFENSIVE_MINOR, 45.0f, true, "Warrior of Elune"},
            {Dr::INNERVATE, CooldownCategory::RESOURCE, 180.0f, false, "Innervate"},
        };

        _primaryInterrupts[spec] = Dr::SOLAR_BEAM;
        _interruptSpells[spec] = {Dr::SOLAR_BEAM, Dr::TYPHOON, Dr::MIGHTY_BASH};

        _fallbackChains[spec] = {
            {"single_target", {Dr::Balance::STARSURGE, Dr::Balance::WRATH, Dr::Balance::STARFIRE}},
            {"aoe", {Dr::Balance::STARFALL, Dr::Balance::STARFIRE, Dr::Balance::WRATH}},
        };
    }

    // --- Feral (Melee DPS) ---
    {
        auto spec = SPEC(DRUID, 1);

        SpecStatWeights sw;
        sw.spec = spec;
        sw.SetWeight(SpellStatType::AGILITY, 1.0f);
        sw.SetWeight(SpellStatType::CRITICAL_STRIKE, 0.9f);
        sw.SetWeight(SpellStatType::MASTERY, 0.85f);
        sw.SetWeight(SpellStatType::HASTE, 0.7f);
        sw.SetWeight(SpellStatType::VERSATILITY, 0.65f);
        _statWeights[spec] = sw;

        _defensiveSpells[spec] = {
            {Dr::BARKSKIN, DefensiveCategory::PERSONAL_MINOR, 60.0f, 60.0f, "Barkskin"},
            {Dr::SURVIVAL_INSTINCTS, DefensiveCategory::PERSONAL_MAJOR, 30.0f, 180.0f, "Survival Instincts"},
        };

        _cooldownSpells[spec] = {
            {Dr::Feral::BERSERK, CooldownCategory::OFFENSIVE_MAJOR, 180.0f, false, "Berserk"},
            {Dr::Feral::TIGERS_FURY, CooldownCategory::OFFENSIVE_MINOR, 30.0f, true, "Tiger's Fury"},
            {Dr::Feral::FERAL_FRENZY, CooldownCategory::OFFENSIVE_MINOR, 45.0f, true, "Feral Frenzy"},
        };

        _primaryInterrupts[spec] = Dr::SKULL_BASH;
        _interruptSpells[spec] = {Dr::SKULL_BASH, Dr::MIGHTY_BASH};

        _fallbackChains[spec] = {
            {"single_target", {Dr::Feral::FEROCIOUS_BITE, Dr::Feral::RIP, Dr::Feral::RAKE, Dr::Feral::SHRED}},
            {"aoe", {Dr::Feral::PRIMAL_WRATH, Dr::Feral::BRUTAL_SLASH, Dr::Feral::THRASH_CAT, Dr::Feral::SWIPE_CAT}},
        };
    }

    // --- Guardian (Tank) ---
    {
        auto spec = SPEC(DRUID, 2);

        SpecStatWeights sw;
        sw.spec = spec;
        sw.SetWeight(SpellStatType::AGILITY, 1.0f);
        sw.SetWeight(SpellStatType::STAMINA, 0.9f);
        sw.SetWeight(SpellStatType::VERSATILITY, 0.85f);
        sw.SetWeight(SpellStatType::MASTERY, 0.8f);
        sw.SetWeight(SpellStatType::HASTE, 0.7f);
        sw.SetWeight(SpellStatType::CRITICAL_STRIKE, 0.6f);
        _statWeights[spec] = sw;

        _defensiveSpells[spec] = {
            {Dr::Guardian::IRONFUR, DefensiveCategory::PERSONAL_MINOR, 80.0f, 0.5f, "Ironfur"},
            {Dr::Guardian::FRENZIED_REGENERATION, DefensiveCategory::SELF_HEAL, 60.0f, 36.0f, "Frenzied Regeneration"},
            {Dr::BARKSKIN, DefensiveCategory::PERSONAL_MINOR, 50.0f, 60.0f, "Barkskin"},
            {Dr::SURVIVAL_INSTINCTS, DefensiveCategory::PERSONAL_MAJOR, 30.0f, 180.0f, "Survival Instincts"},
            {Dr::Guardian::RAGE_OF_THE_SLEEPER, DefensiveCategory::PERSONAL_MAJOR, 40.0f, 90.0f, "Rage of the Sleeper"},
        };

        _cooldownSpells[spec] = {
            {Dr::Guardian::BERSERK_GUARDIAN, CooldownCategory::OFFENSIVE_MAJOR, 180.0f, false, "Berserk"},
            {Dr::STAMPEDING_ROAR, CooldownCategory::UTILITY, 120.0f, false, "Stampeding Roar"},
        };

        _primaryInterrupts[spec] = Dr::SKULL_BASH;
        _interruptSpells[spec] = {Dr::SKULL_BASH, Dr::MIGHTY_BASH};
    }

    // --- Restoration (Healer) ---
    {
        auto spec = SPEC(DRUID, 3);

        SpecStatWeights sw;
        sw.spec = spec;
        sw.SetWeight(SpellStatType::INTELLECT, 1.0f);
        sw.SetWeight(SpellStatType::HASTE, 0.85f);
        sw.SetWeight(SpellStatType::MASTERY, 0.8f);
        sw.SetWeight(SpellStatType::VERSATILITY, 0.75f);
        sw.SetWeight(SpellStatType::CRITICAL_STRIKE, 0.65f);
        _statWeights[spec] = sw;

        _defensiveSpells[spec] = {
            {Dr::BARKSKIN, DefensiveCategory::PERSONAL_MINOR, 60.0f, 60.0f, "Barkskin"},
            {Dr::Restoration::IRONBARK, DefensiveCategory::EXTERNAL_MINOR, 50.0f, 90.0f, "Ironbark"},
        };

        _cooldownSpells[spec] = {
            {Dr::Restoration::TRANQUILITY, CooldownCategory::OFFENSIVE_MAJOR, 180.0f, false, "Tranquility"},
            {Dr::Restoration::TREE_OF_LIFE, CooldownCategory::OFFENSIVE_MAJOR, 180.0f, false, "Incarnation: Tree of Life"},
            {Dr::INNERVATE, CooldownCategory::RESOURCE, 180.0f, false, "Innervate"},
        };

        _primaryInterrupts[spec] = Dr::SOLAR_BEAM;
        _interruptSpells[spec] = {Dr::SOLAR_BEAM, Dr::TYPHOON, Dr::MIGHTY_BASH};

        // Healing tiers
        _healingTiers[spec] = {
            {Dr::REJUVENATION, SpellEfficiencyTier::VERY_HIGH, "Rejuvenation"},
            {Dr::Restoration::LIFEBLOOM, SpellEfficiencyTier::VERY_HIGH, "Lifebloom"},
            {Dr::REGROWTH, SpellEfficiencyTier::HIGH, "Regrowth"},
            {Dr::Restoration::CENARION_WARD, SpellEfficiencyTier::HIGH, "Cenarion Ward"},
            {Dr::WILD_GROWTH, SpellEfficiencyTier::MEDIUM, "Wild Growth"},
            {Dr::SWIFTMEND, SpellEfficiencyTier::LOW, "Swiftmend"},
            {Dr::Restoration::FLOURISH, SpellEfficiencyTier::LOW, "Flourish"},
            {Dr::Restoration::TRANQUILITY, SpellEfficiencyTier::LOW, "Tranquility"},
            {Dr::Restoration::NATURES_SWIFTNESS, SpellEfficiencyTier::EMERGENCY, "Nature's Swiftness"},
            {Dr::Restoration::IRONBARK, SpellEfficiencyTier::EMERGENCY, "Ironbark"},
        };

        _fallbackChains[spec] = {
            {"single_target_heal", {Dr::SWIFTMEND, Dr::REGROWTH, Dr::REJUVENATION, Dr::Restoration::LIFEBLOOM}},
            {"aoe_heal", {Dr::WILD_GROWTH, Dr::Restoration::TRANQUILITY, Dr::Restoration::FLOURISH}},
        };
    }
}

// ============================================================================
// Evoker Initialization (Devastation=0, Preservation=1, Augmentation=2)
// ============================================================================

void ClassSpellDatabase::InitializeEvoker()
{
    // --- Devastation (Ranged DPS) ---
    {
        auto spec = SPEC(EVOKER, 0);

        SpecStatWeights sw;
        sw.spec = spec;
        sw.SetWeight(SpellStatType::INTELLECT, 1.0f);
        sw.SetWeight(SpellStatType::MASTERY, 0.85f);
        sw.SetWeight(SpellStatType::CRITICAL_STRIKE, 0.8f);
        sw.SetWeight(SpellStatType::HASTE, 0.75f);
        sw.SetWeight(SpellStatType::VERSATILITY, 0.7f);
        _statWeights[spec] = sw;

        _defensiveSpells[spec] = {
            {Ev::OBSIDIAN_SCALES, DefensiveCategory::PERSONAL_MINOR, 50.0f, 90.0f, "Obsidian Scales"},
            {Ev::RENEWING_BLAZE, DefensiveCategory::SELF_HEAL, 60.0f, 90.0f, "Renewing Blaze"},
        };

        _cooldownSpells[spec] = {
            {Ev::Devastation::DRAGONRAGE, CooldownCategory::OFFENSIVE_MAJOR, 120.0f, false, "Dragonrage"},
            {Ev::Devastation::SHATTERING_STAR, CooldownCategory::OFFENSIVE_MINOR, 15.0f, true, "Shattering Star"},
        };

        _primaryInterrupts[spec] = Ev::QUELL;
        _interruptSpells[spec] = {Ev::QUELL, Ev::TAIL_SWIPE, Ev::WING_BUFFET};

        _fallbackChains[spec] = {
            {"single_target", {Ev::DISINTEGRATE, Ev::LIVING_FLAME, Ev::AZURE_STRIKE}},
            {"aoe", {Ev::Devastation::PYRE, Ev::Devastation::ETERNITY_SURGE, Ev::FIRE_BREATH, Ev::AZURE_STRIKE}},
        };
    }

    // --- Preservation (Healer) ---
    {
        auto spec = SPEC(EVOKER, 1);

        SpecStatWeights sw;
        sw.spec = spec;
        sw.SetWeight(SpellStatType::INTELLECT, 1.0f);
        sw.SetWeight(SpellStatType::MASTERY, 0.85f);
        sw.SetWeight(SpellStatType::HASTE, 0.8f);
        sw.SetWeight(SpellStatType::CRITICAL_STRIKE, 0.7f);
        sw.SetWeight(SpellStatType::VERSATILITY, 0.75f);
        _statWeights[spec] = sw;

        _defensiveSpells[spec] = {
            {Ev::OBSIDIAN_SCALES, DefensiveCategory::PERSONAL_MINOR, 50.0f, 90.0f, "Obsidian Scales"},
            {Ev::RENEWING_BLAZE, DefensiveCategory::SELF_HEAL, 60.0f, 90.0f, "Renewing Blaze"},
            {Ev::Preservation::TIME_DILATION, DefensiveCategory::EXTERNAL_MINOR, 50.0f, 60.0f, "Time Dilation"},
        };

        _cooldownSpells[spec] = {
            {Ev::Preservation::REWIND, CooldownCategory::OFFENSIVE_MAJOR, 240.0f, false, "Rewind"},
            {Ev::Preservation::DREAM_FLIGHT, CooldownCategory::OFFENSIVE_MAJOR, 120.0f, false, "Dream Flight"},
        };

        _primaryInterrupts[spec] = Ev::QUELL;
        _interruptSpells[spec] = {Ev::QUELL, Ev::TAIL_SWIPE};

        _healingTiers[spec] = {
            {Ev::LIVING_FLAME, SpellEfficiencyTier::VERY_HIGH, "Living Flame (heal)"},
            {Ev::Preservation::REVERSION, SpellEfficiencyTier::VERY_HIGH, "Reversion"},
            {Ev::Preservation::ECHO, SpellEfficiencyTier::HIGH, "Echo"},
            {Ev::EMERALD_BLOSSOM, SpellEfficiencyTier::HIGH, "Emerald Blossom"},
            {Ev::Preservation::DREAM_BREATH, SpellEfficiencyTier::MEDIUM, "Dream Breath"},
            {Ev::Preservation::SPIRITBLOOM, SpellEfficiencyTier::MEDIUM, "Spiritbloom"},
            {Ev::Preservation::TEMPORAL_ANOMALY, SpellEfficiencyTier::LOW, "Temporal Anomaly"},
            {Ev::Preservation::EMERALD_COMMUNION, SpellEfficiencyTier::EMERGENCY, "Emerald Communion"},
            {Ev::Preservation::REWIND, SpellEfficiencyTier::EMERGENCY, "Rewind"},
        };

        _fallbackChains[spec] = {
            {"single_target_heal", {Ev::Preservation::SPIRITBLOOM, Ev::EMERALD_BLOSSOM, Ev::LIVING_FLAME}},
            {"aoe_heal", {Ev::Preservation::DREAM_BREATH, Ev::Preservation::EMERALD_COMMUNION, Ev::Preservation::REWIND}},
        };
    }

    // --- Augmentation (Support DPS) ---
    {
        auto spec = SPEC(EVOKER, 2);

        SpecStatWeights sw;
        sw.spec = spec;
        sw.SetWeight(SpellStatType::INTELLECT, 1.0f);
        sw.SetWeight(SpellStatType::HASTE, 0.85f);
        sw.SetWeight(SpellStatType::CRITICAL_STRIKE, 0.8f);
        sw.SetWeight(SpellStatType::MASTERY, 0.75f);
        sw.SetWeight(SpellStatType::VERSATILITY, 0.7f);
        _statWeights[spec] = sw;

        _defensiveSpells[spec] = {
            {Ev::OBSIDIAN_SCALES, DefensiveCategory::PERSONAL_MINOR, 50.0f, 90.0f, "Obsidian Scales"},
            {Ev::RENEWING_BLAZE, DefensiveCategory::SELF_HEAL, 60.0f, 90.0f, "Renewing Blaze"},
        };

        _cooldownSpells[spec] = {
            {Ev::Augmentation::BREATH_OF_EONS, CooldownCategory::OFFENSIVE_MAJOR, 120.0f, false, "Breath of Eons"},
            {Ev::Augmentation::TIME_SKIP, CooldownCategory::RESOURCE, 180.0f, false, "Time Skip"},
        };

        _primaryInterrupts[spec] = Ev::QUELL;
        _interruptSpells[spec] = {Ev::QUELL, Ev::TAIL_SWIPE};
    }
}

// ============================================================================
// Hunter Initialization (BM=0, MM=1, Survival=2)
// ============================================================================

void ClassSpellDatabase::InitializeHunter()
{
    // --- Beast Mastery (Ranged DPS) ---
    {
        auto spec = SPEC(HUNTER, 0);

        SpecStatWeights sw;
        sw.spec = spec;
        sw.SetWeight(SpellStatType::AGILITY, 1.0f);
        sw.SetWeight(SpellStatType::HASTE, 0.85f);
        sw.SetWeight(SpellStatType::CRITICAL_STRIKE, 0.8f);
        sw.SetWeight(SpellStatType::MASTERY, 0.75f);
        sw.SetWeight(SpellStatType::VERSATILITY, 0.7f);
        _statWeights[spec] = sw;

        _defensiveSpells[spec] = {
            {Hu::ASPECT_OF_THE_TURTLE, DefensiveCategory::PERSONAL_MAJOR, 20.0f, 180.0f, "Aspect of the Turtle"},
            {Hu::EXHILARATION, DefensiveCategory::SELF_HEAL, 50.0f, 120.0f, "Exhilaration"},
            {Hu::FEIGN_DEATH, DefensiveCategory::PERSONAL_MINOR, 70.0f, 30.0f, "Feign Death"},
        };

        _cooldownSpells[spec] = {
            {Hu::BeastMastery::BESTIAL_WRATH, CooldownCategory::OFFENSIVE_MAJOR, 90.0f, true, "Bestial Wrath"},
            {Hu::BeastMastery::ASPECT_OF_THE_WILD, CooldownCategory::OFFENSIVE_MAJOR, 120.0f, false, "Aspect of the Wild"},
            {Hu::BeastMastery::CALL_OF_THE_WILD, CooldownCategory::OFFENSIVE_MAJOR, 120.0f, false, "Call of the Wild"},
        };

        _primaryInterrupts[spec] = Hu::COUNTER_SHOT;
        _interruptSpells[spec] = {Hu::COUNTER_SHOT, Hu::INTIMIDATION};

        _fallbackChains[spec] = {
            {"single_target", {Hu::BeastMastery::KILL_COMMAND, Hu::BeastMastery::BARBED_SHOT, Hu::BeastMastery::COBRA_SHOT}},
            {"aoe", {Hu::MULTI_SHOT, Hu::BeastMastery::KILL_COMMAND, Hu::BeastMastery::BARBED_SHOT}},
        };
    }

    // --- Marksmanship (Ranged DPS) ---
    {
        auto spec = SPEC(HUNTER, 1);

        SpecStatWeights sw;
        sw.spec = spec;
        sw.SetWeight(SpellStatType::AGILITY, 1.0f);
        sw.SetWeight(SpellStatType::MASTERY, 0.85f);
        sw.SetWeight(SpellStatType::CRITICAL_STRIKE, 0.8f);
        sw.SetWeight(SpellStatType::HASTE, 0.75f);
        sw.SetWeight(SpellStatType::VERSATILITY, 0.7f);
        _statWeights[spec] = sw;

        _defensiveSpells[spec] = {
            {Hu::ASPECT_OF_THE_TURTLE, DefensiveCategory::PERSONAL_MAJOR, 20.0f, 180.0f, "Aspect of the Turtle"},
            {Hu::EXHILARATION, DefensiveCategory::SELF_HEAL, 50.0f, 120.0f, "Exhilaration"},
            {Hu::FEIGN_DEATH, DefensiveCategory::PERSONAL_MINOR, 70.0f, 30.0f, "Feign Death"},
        };

        _cooldownSpells[spec] = {
            {Hu::Marksmanship::TRUESHOT, CooldownCategory::OFFENSIVE_MAJOR, 120.0f, false, "Trueshot"},
            {Hu::Marksmanship::VOLLEY, CooldownCategory::OFFENSIVE_MINOR, 45.0f, true, "Volley"},
        };

        _primaryInterrupts[spec] = Hu::COUNTER_SHOT;
        _interruptSpells[spec] = {Hu::COUNTER_SHOT, Hu::INTIMIDATION};

        _fallbackChains[spec] = {
            {"single_target", {Hu::AIMED_SHOT, Hu::RAPID_FIRE, Hu::ARCANE_SHOT, Hu::STEADY_SHOT}},
            {"aoe", {Hu::Marksmanship::VOLLEY, Hu::MULTI_SHOT, Hu::Marksmanship::EXPLOSIVE_SHOT, Hu::AIMED_SHOT}},
        };
    }

    // --- Survival (Melee DPS) ---
    {
        auto spec = SPEC(HUNTER, 2);

        SpecStatWeights sw;
        sw.spec = spec;
        sw.SetWeight(SpellStatType::AGILITY, 1.0f);
        sw.SetWeight(SpellStatType::HASTE, 0.85f);
        sw.SetWeight(SpellStatType::CRITICAL_STRIKE, 0.8f);
        sw.SetWeight(SpellStatType::VERSATILITY, 0.75f);
        sw.SetWeight(SpellStatType::MASTERY, 0.7f);
        _statWeights[spec] = sw;

        _defensiveSpells[spec] = {
            {Hu::ASPECT_OF_THE_TURTLE, DefensiveCategory::PERSONAL_MAJOR, 20.0f, 180.0f, "Aspect of the Turtle"},
            {Hu::EXHILARATION, DefensiveCategory::SELF_HEAL, 50.0f, 120.0f, "Exhilaration"},
            {Hu::FEIGN_DEATH, DefensiveCategory::PERSONAL_MINOR, 70.0f, 30.0f, "Feign Death"},
        };

        _cooldownSpells[spec] = {
            {Hu::Survival::COORDINATED_ASSAULT, CooldownCategory::OFFENSIVE_MAJOR, 120.0f, false, "Coordinated Assault"},
            {Hu::Survival::SPEARHEAD, CooldownCategory::OFFENSIVE_MINOR, 90.0f, false, "Spearhead"},
        };

        _primaryInterrupts[spec] = Hu::Survival::MUZZLE;
        _interruptSpells[spec] = {Hu::Survival::MUZZLE, Hu::INTIMIDATION};

        _fallbackChains[spec] = {
            {"single_target", {Hu::Survival::RAPTOR_STRIKE, Hu::Survival::KILL_COMMAND_SURVIVAL, Hu::Survival::WILDFIRE_BOMB}},
            {"aoe", {Hu::Survival::BUTCHERY, Hu::Survival::CARVE, Hu::Survival::WILDFIRE_BOMB}},
        };
    }
}

// ============================================================================
// Mage Initialization (Arcane=0, Fire=1, Frost=2)
// ============================================================================

void ClassSpellDatabase::InitializeMage()
{
    // --- Arcane ---
    {
        auto spec = SPEC(MAGE, 0);

        SpecStatWeights sw;
        sw.spec = spec;
        sw.SetWeight(SpellStatType::INTELLECT, 1.0f);
        sw.SetWeight(SpellStatType::MASTERY, 0.85f);
        sw.SetWeight(SpellStatType::HASTE, 0.8f);
        sw.SetWeight(SpellStatType::CRITICAL_STRIKE, 0.75f);
        sw.SetWeight(SpellStatType::VERSATILITY, 0.7f);
        _statWeights[spec] = sw;

        _defensiveSpells[spec] = {
            {Ma::ICE_BLOCK, DefensiveCategory::PERSONAL_MAJOR, 20.0f, 240.0f, "Ice Block"},
            {Ma::GREATER_INVISIBILITY, DefensiveCategory::PERSONAL_MINOR, 50.0f, 120.0f, "Greater Invisibility"},
            {Ma::ALTER_TIME, DefensiveCategory::PERSONAL_MINOR, 40.0f, 60.0f, "Alter Time"},
        };

        _cooldownSpells[spec] = {
            {Ma::Arcane::ARCANE_SURGE, CooldownCategory::OFFENSIVE_MAJOR, 90.0f, false, "Arcane Surge"},
            {Ma::Arcane::TOUCH_OF_THE_MAGI, CooldownCategory::OFFENSIVE_MINOR, 45.0f, true, "Touch of the Magi"},
            {Ma::Arcane::EVOCATION, CooldownCategory::RESOURCE, 90.0f, false, "Evocation"},
            {Ma::MIRROR_IMAGE, CooldownCategory::OFFENSIVE_MINOR, 120.0f, true, "Mirror Image"},
        };

        _primaryInterrupts[spec] = Ma::COUNTERSPELL;
        _interruptSpells[spec] = {Ma::COUNTERSPELL};

        _fallbackChains[spec] = {
            {"single_target", {Ma::Arcane::ARCANE_BLAST, Ma::Arcane::ARCANE_MISSILES, Ma::Arcane::ARCANE_BARRAGE}},
            {"aoe", {Ma::Arcane::ARCANE_ORB, Ma::Arcane::ARCANE_EXPLOSION, Ma::Arcane::ARCANE_BARRAGE}},
        };
    }

    // --- Fire ---
    {
        auto spec = SPEC(MAGE, 1);

        SpecStatWeights sw;
        sw.spec = spec;
        sw.SetWeight(SpellStatType::INTELLECT, 1.0f);
        sw.SetWeight(SpellStatType::HASTE, 0.85f);
        sw.SetWeight(SpellStatType::CRITICAL_STRIKE, 0.82f);
        sw.SetWeight(SpellStatType::MASTERY, 0.75f);
        sw.SetWeight(SpellStatType::VERSATILITY, 0.7f);
        _statWeights[spec] = sw;

        _defensiveSpells[spec] = {
            {Ma::ICE_BLOCK, DefensiveCategory::PERSONAL_MAJOR, 20.0f, 240.0f, "Ice Block"},
            {Ma::Fire::BLAZING_BARRIER, DefensiveCategory::PERSONAL_MINOR, 60.0f, 25.0f, "Blazing Barrier"},
            {Ma::ALTER_TIME, DefensiveCategory::PERSONAL_MINOR, 40.0f, 60.0f, "Alter Time"},
        };

        _cooldownSpells[spec] = {
            {Ma::Fire::COMBUSTION, CooldownCategory::OFFENSIVE_MAJOR, 120.0f, false, "Combustion"},
            {Ma::MIRROR_IMAGE, CooldownCategory::OFFENSIVE_MINOR, 120.0f, true, "Mirror Image"},
        };

        _primaryInterrupts[spec] = Ma::COUNTERSPELL;
        _interruptSpells[spec] = {Ma::COUNTERSPELL};

        _fallbackChains[spec] = {
            {"single_target", {Ma::Fire::PYROBLAST, Ma::Fire::FIRE_BLAST, Ma::FIREBALL, Ma::Fire::SCORCH}},
            {"aoe", {Ma::Fire::FLAMESTRIKE, Ma::Fire::PHOENIX_FLAMES, Ma::Fire::LIVING_BOMB}},
        };
    }

    // --- Frost ---
    {
        auto spec = SPEC(MAGE, 2);

        SpecStatWeights sw;
        sw.spec = spec;
        sw.SetWeight(SpellStatType::INTELLECT, 1.0f);
        sw.SetWeight(SpellStatType::HASTE, 0.85f);
        sw.SetWeight(SpellStatType::CRITICAL_STRIKE, 0.8f);
        sw.SetWeight(SpellStatType::VERSATILITY, 0.75f);
        sw.SetWeight(SpellStatType::MASTERY, 0.7f);
        _statWeights[spec] = sw;

        _defensiveSpells[spec] = {
            {Ma::ICE_BLOCK, DefensiveCategory::PERSONAL_MAJOR, 20.0f, 240.0f, "Ice Block"},
            {Ma::Frost::ICE_BARRIER, DefensiveCategory::PERSONAL_MINOR, 60.0f, 25.0f, "Ice Barrier"},
            {Ma::ALTER_TIME, DefensiveCategory::PERSONAL_MINOR, 40.0f, 60.0f, "Alter Time"},
        };

        _cooldownSpells[spec] = {
            {Ma::Frost::ICY_VEINS, CooldownCategory::OFFENSIVE_MAJOR, 180.0f, false, "Icy Veins"},
            {Ma::MIRROR_IMAGE, CooldownCategory::OFFENSIVE_MINOR, 120.0f, true, "Mirror Image"},
            {Ma::Frost::FROZEN_ORB, CooldownCategory::OFFENSIVE_MINOR, 60.0f, true, "Frozen Orb"},
        };

        _primaryInterrupts[spec] = Ma::COUNTERSPELL;
        _interruptSpells[spec] = {Ma::COUNTERSPELL};

        _fallbackChains[spec] = {
            {"single_target", {Ma::Frost::GLACIAL_SPIKE, Ma::Frost::ICE_LANCE, Ma::Frost::FLURRY, Ma::FROSTBOLT}},
            {"aoe", {Ma::Frost::BLIZZARD, Ma::Frost::FROZEN_ORB, Ma::Frost::COMET_STORM, Ma::Frost::ICE_LANCE}},
        };
    }
}

// ============================================================================
// Monk Initialization (Brewmaster=0, Mistweaver=1, Windwalker=2)
// ============================================================================

void ClassSpellDatabase::InitializeMonk()
{
    // --- Brewmaster (Tank) ---
    {
        auto spec = SPEC(MONK, 0);

        SpecStatWeights sw;
        sw.spec = spec;
        sw.SetWeight(SpellStatType::AGILITY, 1.0f);
        sw.SetWeight(SpellStatType::STAMINA, 0.9f);
        sw.SetWeight(SpellStatType::VERSATILITY, 0.85f);
        sw.SetWeight(SpellStatType::CRITICAL_STRIKE, 0.8f);
        sw.SetWeight(SpellStatType::MASTERY, 0.75f);
        sw.SetWeight(SpellStatType::HASTE, 0.7f);
        _statWeights[spec] = sw;

        _defensiveSpells[spec] = {
            {Mo::Brewmaster::CELESTIAL_BREW, DefensiveCategory::PERSONAL_MINOR, 60.0f, 60.0f, "Celestial Brew"},
            {Mo::Brewmaster::PURIFYING_BREW, DefensiveCategory::PERSONAL_MINOR, 75.0f, 20.0f, "Purifying Brew"},
            {Mo::FORTIFYING_BREW, DefensiveCategory::PERSONAL_MAJOR, 35.0f, 360.0f, "Fortifying Brew"},
            {Mo::DAMPEN_HARM, DefensiveCategory::PERSONAL_MINOR, 50.0f, 120.0f, "Dampen Harm"},
            {Mo::ZEN_MEDITATION, DefensiveCategory::PERSONAL_MAJOR, 25.0f, 300.0f, "Zen Meditation"},
            {Mo::EXPEL_HARM, DefensiveCategory::SELF_HEAL, 80.0f, 15.0f, "Expel Harm"},
        };

        _cooldownSpells[spec] = {
            {Mo::Brewmaster::INVOKE_NIUZAO, CooldownCategory::OFFENSIVE_MAJOR, 180.0f, false, "Invoke Niuzao"},
            {Mo::Brewmaster::WEAPONS_OF_ORDER, CooldownCategory::OFFENSIVE_MAJOR, 120.0f, false, "Weapons of Order"},
        };

        _primaryInterrupts[spec] = Mo::SPEAR_HAND_STRIKE;
        _interruptSpells[spec] = {Mo::SPEAR_HAND_STRIKE, Mo::LEG_SWEEP};
    }

    // --- Mistweaver (Healer) ---
    {
        auto spec = SPEC(MONK, 1);

        SpecStatWeights sw;
        sw.spec = spec;
        sw.SetWeight(SpellStatType::INTELLECT, 1.0f);
        sw.SetWeight(SpellStatType::HASTE, 0.85f);
        sw.SetWeight(SpellStatType::CRITICAL_STRIKE, 0.8f);
        sw.SetWeight(SpellStatType::VERSATILITY, 0.75f);
        sw.SetWeight(SpellStatType::MASTERY, 0.7f);
        _statWeights[spec] = sw;

        _defensiveSpells[spec] = {
            {Mo::FORTIFYING_BREW, DefensiveCategory::PERSONAL_MAJOR, 35.0f, 360.0f, "Fortifying Brew"},
            {Mo::DAMPEN_HARM, DefensiveCategory::PERSONAL_MINOR, 50.0f, 120.0f, "Dampen Harm"},
            {Mo::DIFFUSE_MAGIC, DefensiveCategory::PERSONAL_MINOR, 60.0f, 90.0f, "Diffuse Magic"},
            {Mo::Mistweaver::LIFE_COCOON, DefensiveCategory::EXTERNAL_MAJOR, 40.0f, 120.0f, "Life Cocoon"},
        };

        _cooldownSpells[spec] = {
            {Mo::Mistweaver::REVIVAL, CooldownCategory::OFFENSIVE_MAJOR, 180.0f, false, "Revival"},
            {Mo::Mistweaver::INVOKE_YULON, CooldownCategory::OFFENSIVE_MAJOR, 180.0f, false, "Invoke Yu'lon"},
            {Mo::Mistweaver::MANA_TEA, CooldownCategory::RESOURCE, 45.0f, true, "Mana Tea"},
        };

        _primaryInterrupts[spec] = Mo::SPEAR_HAND_STRIKE;
        _interruptSpells[spec] = {Mo::SPEAR_HAND_STRIKE, Mo::LEG_SWEEP};

        _healingTiers[spec] = {
            {Mo::Mistweaver::RENEWING_MIST, SpellEfficiencyTier::VERY_HIGH, "Renewing Mist"},
            {Mo::Mistweaver::SOOTHING_MIST, SpellEfficiencyTier::VERY_HIGH, "Soothing Mist"},
            {Mo::Mistweaver::VIVIFY, SpellEfficiencyTier::HIGH, "Vivify"},
            {Mo::Mistweaver::ENVELOPING_MIST, SpellEfficiencyTier::MEDIUM, "Enveloping Mist"},
            {Mo::Mistweaver::ESSENCE_FONT, SpellEfficiencyTier::MEDIUM, "Essence Font"},
            {Mo::Mistweaver::SHEILUNS_GIFT, SpellEfficiencyTier::LOW, "Sheilun's Gift"},
            {Mo::Mistweaver::REVIVAL, SpellEfficiencyTier::EMERGENCY, "Revival"},
            {Mo::Mistweaver::LIFE_COCOON, SpellEfficiencyTier::EMERGENCY, "Life Cocoon"},
        };

        _fallbackChains[spec] = {
            {"single_target_heal", {Mo::Mistweaver::ENVELOPING_MIST, Mo::Mistweaver::VIVIFY, Mo::Mistweaver::SOOTHING_MIST}},
            {"aoe_heal", {Mo::Mistweaver::ESSENCE_FONT, Mo::Mistweaver::REVIVAL, Mo::Mistweaver::REFRESHING_JADE_WIND}},
        };
    }

    // --- Windwalker (Melee DPS) ---
    {
        auto spec = SPEC(MONK, 2);

        SpecStatWeights sw;
        sw.spec = spec;
        sw.SetWeight(SpellStatType::AGILITY, 1.0f);
        sw.SetWeight(SpellStatType::VERSATILITY, 0.85f);
        sw.SetWeight(SpellStatType::CRITICAL_STRIKE, 0.8f);
        sw.SetWeight(SpellStatType::MASTERY, 0.75f);
        sw.SetWeight(SpellStatType::HASTE, 0.7f);
        _statWeights[spec] = sw;

        _defensiveSpells[spec] = {
            {Mo::Windwalker::TOUCH_OF_KARMA, DefensiveCategory::PERSONAL_MAJOR, 40.0f, 90.0f, "Touch of Karma"},
            {Mo::FORTIFYING_BREW, DefensiveCategory::PERSONAL_MAJOR, 30.0f, 360.0f, "Fortifying Brew"},
            {Mo::DIFFUSE_MAGIC, DefensiveCategory::PERSONAL_MINOR, 60.0f, 90.0f, "Diffuse Magic"},
        };

        _cooldownSpells[spec] = {
            {Mo::Windwalker::STORM_EARTH_AND_FIRE, CooldownCategory::OFFENSIVE_MAJOR, 90.0f, false, "Storm, Earth, and Fire"},
            {Mo::Windwalker::INVOKE_XUEN, CooldownCategory::OFFENSIVE_MAJOR, 120.0f, false, "Invoke Xuen"},
            {Mo::TOUCH_OF_DEATH, CooldownCategory::OFFENSIVE_MINOR, 180.0f, false, "Touch of Death"},
        };

        _primaryInterrupts[spec] = Mo::SPEAR_HAND_STRIKE;
        _interruptSpells[spec] = {Mo::SPEAR_HAND_STRIKE, Mo::LEG_SWEEP};

        _fallbackChains[spec] = {
            {"single_target", {Mo::Windwalker::RISING_SUN_KICK_WW, Mo::Windwalker::FISTS_OF_FURY, Mo::Windwalker::BLACKOUT_KICK_WW, Mo::TIGER_PALM}},
            {"aoe", {Mo::Windwalker::SPINNING_CRANE_KICK_WW, Mo::Windwalker::FISTS_OF_FURY, Mo::Windwalker::WHIRLING_DRAGON_PUNCH}},
        };
    }
}

// ============================================================================
// Paladin Initialization (Holy=0, Protection=1, Retribution=2)
// ============================================================================

void ClassSpellDatabase::InitializePaladin()
{
    // --- Holy (Healer) ---
    {
        auto spec = SPEC(PALADIN, 0);

        SpecStatWeights sw;
        sw.spec = spec;
        sw.SetWeight(SpellStatType::INTELLECT, 1.0f);
        sw.SetWeight(SpellStatType::HASTE, 0.85f);
        sw.SetWeight(SpellStatType::MASTERY, 0.8f);
        sw.SetWeight(SpellStatType::CRITICAL_STRIKE, 0.75f);
        sw.SetWeight(SpellStatType::VERSATILITY, 0.7f);
        _statWeights[spec] = sw;

        _defensiveSpells[spec] = {
            {Pa::DIVINE_SHIELD, DefensiveCategory::PERSONAL_MAJOR, 15.0f, 300.0f, "Divine Shield"},
            {Pa::DIVINE_PROTECTION, DefensiveCategory::PERSONAL_MINOR, 50.0f, 60.0f, "Divine Protection"},
            {Pa::LAY_ON_HANDS, DefensiveCategory::EXTERNAL_MAJOR, 15.0f, 600.0f, "Lay on Hands"},
            {Pa::BLESSING_OF_SACRIFICE, DefensiveCategory::EXTERNAL_MINOR, 40.0f, 120.0f, "Blessing of Sacrifice"},
            {Pa::BLESSING_OF_PROTECTION, DefensiveCategory::EXTERNAL_MAJOR, 25.0f, 300.0f, "Blessing of Protection"},
            {Pa::Holy::AURA_MASTERY, DefensiveCategory::RAID_WIDE, 40.0f, 180.0f, "Aura Mastery"},
        };

        _cooldownSpells[spec] = {
            {Pa::AVENGING_WRATH, CooldownCategory::OFFENSIVE_MAJOR, 120.0f, false, "Avenging Wrath"},
            {Pa::Holy::HOLY_AVENGER, CooldownCategory::OFFENSIVE_MINOR, 120.0f, false, "Holy Avenger"},
        };

        _primaryInterrupts[spec] = Pa::REBUKE;
        _interruptSpells[spec] = {Pa::REBUKE, Pa::HAMMER_OF_JUSTICE};

        _healingTiers[spec] = {
            {Pa::Holy::HOLY_SHOCK, SpellEfficiencyTier::VERY_HIGH, "Holy Shock"},
            {Pa::WORD_OF_GLORY, SpellEfficiencyTier::VERY_HIGH, "Word of Glory"},
            {Pa::FLASH_OF_LIGHT, SpellEfficiencyTier::HIGH, "Flash of Light"},
            {Pa::HOLY_LIGHT, SpellEfficiencyTier::VERY_HIGH, "Holy Light"},
            {Pa::Holy::LIGHT_OF_DAWN, SpellEfficiencyTier::MEDIUM, "Light of Dawn"},
            {Pa::Holy::HOLY_PRISM, SpellEfficiencyTier::MEDIUM, "Holy Prism"},
            {Pa::Holy::LIGHTS_HAMMER, SpellEfficiencyTier::LOW, "Light's Hammer"},
            {Pa::LAY_ON_HANDS, SpellEfficiencyTier::EMERGENCY, "Lay on Hands"},
            {Pa::DIVINE_SHIELD, SpellEfficiencyTier::EMERGENCY, "Divine Shield"},
            {Pa::Holy::AURA_MASTERY, SpellEfficiencyTier::EMERGENCY, "Aura Mastery"},
        };

        _fallbackChains[spec] = {
            {"single_target_heal", {Pa::Holy::HOLY_SHOCK, Pa::FLASH_OF_LIGHT, Pa::HOLY_LIGHT, Pa::WORD_OF_GLORY}},
            {"aoe_heal", {Pa::Holy::LIGHT_OF_DAWN, Pa::Holy::HOLY_PRISM, Pa::Holy::LIGHTS_HAMMER}},
        };
    }

    // --- Protection (Tank) ---
    {
        auto spec = SPEC(PALADIN, 1);

        SpecStatWeights sw;
        sw.spec = spec;
        sw.SetWeight(SpellStatType::STRENGTH, 1.0f);
        sw.SetWeight(SpellStatType::STAMINA, 0.9f);
        sw.SetWeight(SpellStatType::HASTE, 0.85f);
        sw.SetWeight(SpellStatType::MASTERY, 0.8f);
        sw.SetWeight(SpellStatType::VERSATILITY, 0.75f);
        sw.SetWeight(SpellStatType::CRITICAL_STRIKE, 0.65f);
        _statWeights[spec] = sw;

        _defensiveSpells[spec] = {
            {Pa::DIVINE_SHIELD, DefensiveCategory::PERSONAL_MAJOR, 15.0f, 300.0f, "Divine Shield"},
            {Pa::Protection::ARDENT_DEFENDER, DefensiveCategory::PERSONAL_MAJOR, 35.0f, 120.0f, "Ardent Defender"},
            {Pa::Protection::GUARDIAN_OF_ANCIENT_KINGS, DefensiveCategory::PERSONAL_MAJOR, 25.0f, 300.0f, "Guardian of Ancient Kings"},
            {Pa::Protection::SHIELD_OF_THE_RIGHTEOUS, DefensiveCategory::PERSONAL_MINOR, 80.0f, 0.0f, "Shield of the Righteous"},
            {Pa::WORD_OF_GLORY, DefensiveCategory::SELF_HEAL, 60.0f, 0.0f, "Word of Glory"},
            {Pa::LAY_ON_HANDS, DefensiveCategory::SELF_HEAL, 15.0f, 600.0f, "Lay on Hands"},
        };

        _cooldownSpells[spec] = {
            {Pa::AVENGING_WRATH, CooldownCategory::OFFENSIVE_MAJOR, 120.0f, false, "Avenging Wrath"},
            {Pa::Protection::DIVINE_TOLL, CooldownCategory::OFFENSIVE_MINOR, 60.0f, true, "Divine Toll"},
            {Pa::Protection::EYE_OF_TYR, CooldownCategory::OFFENSIVE_MINOR, 60.0f, true, "Eye of Tyr"},
        };

        _primaryInterrupts[spec] = Pa::REBUKE;
        _interruptSpells[spec] = {Pa::REBUKE, Pa::HAMMER_OF_JUSTICE, Pa::Protection::AVENGERS_SHIELD};
    }

    // --- Retribution (Melee DPS) ---
    {
        auto spec = SPEC(PALADIN, 2);

        SpecStatWeights sw;
        sw.spec = spec;
        sw.SetWeight(SpellStatType::STRENGTH, 1.0f);
        sw.SetWeight(SpellStatType::HASTE, 0.85f);
        sw.SetWeight(SpellStatType::MASTERY, 0.8f);
        sw.SetWeight(SpellStatType::CRITICAL_STRIKE, 0.75f);
        sw.SetWeight(SpellStatType::VERSATILITY, 0.7f);
        _statWeights[spec] = sw;

        _defensiveSpells[spec] = {
            {Pa::DIVINE_SHIELD, DefensiveCategory::PERSONAL_MAJOR, 15.0f, 300.0f, "Divine Shield"},
            {Pa::DIVINE_PROTECTION, DefensiveCategory::PERSONAL_MINOR, 50.0f, 60.0f, "Divine Protection"},
            {Pa::Retribution::SHIELD_OF_VENGEANCE, DefensiveCategory::PERSONAL_MINOR, 60.0f, 90.0f, "Shield of Vengeance"},
            {Pa::WORD_OF_GLORY, DefensiveCategory::SELF_HEAL, 50.0f, 0.0f, "Word of Glory"},
            {Pa::LAY_ON_HANDS, DefensiveCategory::SELF_HEAL, 15.0f, 600.0f, "Lay on Hands"},
        };

        _cooldownSpells[spec] = {
            {Pa::AVENGING_WRATH, CooldownCategory::OFFENSIVE_MAJOR, 120.0f, false, "Avenging Wrath"},
            {Pa::Retribution::CRUSADE, CooldownCategory::OFFENSIVE_MAJOR, 120.0f, false, "Crusade"},
            {Pa::Retribution::EXECUTION_SENTENCE, CooldownCategory::OFFENSIVE_MINOR, 60.0f, true, "Execution Sentence"},
            {Pa::Retribution::FINAL_RECKONING, CooldownCategory::OFFENSIVE_MINOR, 60.0f, false, "Final Reckoning"},
        };

        _primaryInterrupts[spec] = Pa::REBUKE;
        _interruptSpells[spec] = {Pa::REBUKE, Pa::HAMMER_OF_JUSTICE};

        _fallbackChains[spec] = {
            {"single_target", {Pa::Retribution::TEMPLARS_VERDICT, Pa::Retribution::BLADE_OF_JUSTICE, Pa::Retribution::WAKE_OF_ASHES, Pa::JUDGMENT, Pa::CRUSADER_STRIKE}},
            {"aoe", {Pa::Retribution::DIVINE_STORM, Pa::Retribution::WAKE_OF_ASHES, Pa::CONSECRATION}},
        };
    }
}

// ============================================================================
// Priest Initialization (Discipline=0, Holy=1, Shadow=2)
// ============================================================================

void ClassSpellDatabase::InitializePriest()
{
    // --- Discipline (Healer) ---
    {
        auto spec = SPEC(PRIEST, 0);

        SpecStatWeights sw;
        sw.spec = spec;
        sw.SetWeight(SpellStatType::INTELLECT, 1.0f);
        sw.SetWeight(SpellStatType::HASTE, 0.85f);
        sw.SetWeight(SpellStatType::CRITICAL_STRIKE, 0.8f);
        sw.SetWeight(SpellStatType::MASTERY, 0.75f);
        sw.SetWeight(SpellStatType::VERSATILITY, 0.7f);
        _statWeights[spec] = sw;

        _defensiveSpells[spec] = {
            {Pr::DESPERATE_PRAYER, DefensiveCategory::SELF_HEAL, 40.0f, 90.0f, "Desperate Prayer"},
            {Pr::Discipline::PAIN_SUPPRESSION, DefensiveCategory::EXTERNAL_MAJOR, 30.0f, 180.0f, "Pain Suppression"},
            {Pr::Discipline::POWER_WORD_BARRIER, DefensiveCategory::RAID_WIDE, 40.0f, 180.0f, "Power Word: Barrier"},
            {Pr::Discipline::RAPTURE, DefensiveCategory::RAID_WIDE, 50.0f, 90.0f, "Rapture"},
            {Pr::POWER_WORD_SHIELD, DefensiveCategory::EXTERNAL_MINOR, 70.0f, 0.0f, "Power Word: Shield"},
        };

        _cooldownSpells[spec] = {
            {Pr::POWER_INFUSION, CooldownCategory::OFFENSIVE_MAJOR, 120.0f, false, "Power Infusion"},
            {Pr::Discipline::EVANGELISM, CooldownCategory::OFFENSIVE_MINOR, 90.0f, false, "Evangelism"},
            {Pr::Discipline::SHADOWFIEND, CooldownCategory::RESOURCE, 180.0f, false, "Shadowfiend"},
        };

        _primaryInterrupts[spec] = 0; // Discipline has no interrupt
        _interruptSpells[spec] = {Pr::PSYCHIC_SCREAM};

        _healingTiers[spec] = {
            {Pr::POWER_WORD_SHIELD, SpellEfficiencyTier::VERY_HIGH, "Power Word: Shield"},
            {Pr::Discipline::PENANCE, SpellEfficiencyTier::VERY_HIGH, "Penance"},
            {Pr::Discipline::POWER_WORD_RADIANCE, SpellEfficiencyTier::HIGH, "Power Word: Radiance"},
            {Pr::Discipline::SHADOW_MEND, SpellEfficiencyTier::HIGH, "Shadow Mend"},
            {Pr::FLASH_HEAL, SpellEfficiencyTier::MEDIUM, "Flash Heal"},
            {Pr::Discipline::RAPTURE, SpellEfficiencyTier::LOW, "Rapture"},
            {Pr::Discipline::PAIN_SUPPRESSION, SpellEfficiencyTier::EMERGENCY, "Pain Suppression"},
            {Pr::Discipline::POWER_WORD_BARRIER, SpellEfficiencyTier::EMERGENCY, "Power Word: Barrier"},
        };

        _fallbackChains[spec] = {
            {"single_target_heal", {Pr::POWER_WORD_SHIELD, Pr::Discipline::PENANCE, Pr::Discipline::SHADOW_MEND, Pr::FLASH_HEAL}},
            {"aoe_heal", {Pr::Discipline::POWER_WORD_RADIANCE, Pr::Discipline::EVANGELISM, Pr::Discipline::RAPTURE}},
        };
    }

    // --- Holy Priest (Healer) ---
    {
        auto spec = SPEC(PRIEST, 1);

        SpecStatWeights sw;
        sw.spec = spec;
        sw.SetWeight(SpellStatType::INTELLECT, 1.0f);
        sw.SetWeight(SpellStatType::MASTERY, 0.85f);
        sw.SetWeight(SpellStatType::CRITICAL_STRIKE, 0.8f);
        sw.SetWeight(SpellStatType::HASTE, 0.75f);
        sw.SetWeight(SpellStatType::VERSATILITY, 0.7f);
        _statWeights[spec] = sw;

        _defensiveSpells[spec] = {
            {Pr::DESPERATE_PRAYER, DefensiveCategory::SELF_HEAL, 40.0f, 90.0f, "Desperate Prayer"},
            {Pr::HolyPriest::GUARDIAN_SPIRIT, DefensiveCategory::EXTERNAL_MAJOR, 20.0f, 180.0f, "Guardian Spirit"},
            {Pr::HolyPriest::DIVINE_HYMN, DefensiveCategory::RAID_WIDE, 40.0f, 180.0f, "Divine Hymn"},
            {Pr::HolyPriest::SYMBOL_OF_HOPE, DefensiveCategory::RAID_WIDE, 60.0f, 180.0f, "Symbol of Hope"},
        };

        _cooldownSpells[spec] = {
            {Pr::POWER_INFUSION, CooldownCategory::OFFENSIVE_MAJOR, 120.0f, false, "Power Infusion"},
            {Pr::HolyPriest::APOTHEOSIS, CooldownCategory::OFFENSIVE_MAJOR, 120.0f, false, "Apotheosis"},
            {Pr::HolyPriest::HOLY_WORD_SALVATION, CooldownCategory::OFFENSIVE_MAJOR, 720.0f, false, "Holy Word: Salvation"},
        };

        _primaryInterrupts[spec] = 0; // Holy Priest has no interrupt
        _interruptSpells[spec] = {Pr::PSYCHIC_SCREAM, Pr::HolyPriest::HOLY_WORD_CHASTISE};

        _healingTiers[spec] = {
            {Pr::HolyPriest::RENEW, SpellEfficiencyTier::VERY_HIGH, "Renew"},
            {Pr::HolyPriest::HEAL, SpellEfficiencyTier::VERY_HIGH, "Heal"},
            {Pr::HolyPriest::PRAYER_OF_MENDING, SpellEfficiencyTier::VERY_HIGH, "Prayer of Mending"},
            {Pr::FLASH_HEAL, SpellEfficiencyTier::HIGH, "Flash Heal"},
            {Pr::HolyPriest::CIRCLE_OF_HEALING, SpellEfficiencyTier::HIGH, "Circle of Healing"},
            {Pr::HolyPriest::HOLY_WORD_SERENITY, SpellEfficiencyTier::LOW, "Holy Word: Serenity"},
            {Pr::HolyPriest::HOLY_WORD_SANCTIFY, SpellEfficiencyTier::LOW, "Holy Word: Sanctify"},
            {Pr::HolyPriest::PRAYER_OF_HEALING, SpellEfficiencyTier::MEDIUM, "Prayer of Healing"},
            {Pr::HolyPriest::DIVINE_HYMN, SpellEfficiencyTier::EMERGENCY, "Divine Hymn"},
            {Pr::HolyPriest::GUARDIAN_SPIRIT, SpellEfficiencyTier::EMERGENCY, "Guardian Spirit"},
        };

        _fallbackChains[spec] = {
            {"single_target_heal", {Pr::HolyPriest::HOLY_WORD_SERENITY, Pr::FLASH_HEAL, Pr::HolyPriest::HEAL, Pr::HolyPriest::RENEW}},
            {"aoe_heal", {Pr::HolyPriest::HOLY_WORD_SANCTIFY, Pr::HolyPriest::CIRCLE_OF_HEALING, Pr::HolyPriest::PRAYER_OF_HEALING}},
        };
    }

    // --- Shadow (Ranged DPS) ---
    {
        auto spec = SPEC(PRIEST, 2);

        SpecStatWeights sw;
        sw.spec = spec;
        sw.SetWeight(SpellStatType::INTELLECT, 1.0f);
        sw.SetWeight(SpellStatType::HASTE, 0.85f);
        sw.SetWeight(SpellStatType::MASTERY, 0.8f);
        sw.SetWeight(SpellStatType::CRITICAL_STRIKE, 0.75f);
        sw.SetWeight(SpellStatType::VERSATILITY, 0.7f);
        _statWeights[spec] = sw;

        _defensiveSpells[spec] = {
            {Pr::Shadow::DISPERSION, DefensiveCategory::PERSONAL_MAJOR, 25.0f, 120.0f, "Dispersion"},
            {Pr::DESPERATE_PRAYER, DefensiveCategory::SELF_HEAL, 40.0f, 90.0f, "Desperate Prayer"},
            {Pr::Shadow::VAMPIRIC_EMBRACE, DefensiveCategory::SELF_HEAL, 60.0f, 120.0f, "Vampiric Embrace"},
        };

        _cooldownSpells[spec] = {
            {Pr::Shadow::VOID_ERUPTION, CooldownCategory::OFFENSIVE_MAJOR, 90.0f, false, "Void Eruption"},
            {Pr::Shadow::DARK_ASCENSION, CooldownCategory::OFFENSIVE_MAJOR, 60.0f, false, "Dark Ascension"},
            {Pr::POWER_INFUSION, CooldownCategory::OFFENSIVE_MAJOR, 120.0f, false, "Power Infusion"},
            {Pr::Shadow::SHADOWFIEND_SHADOW, CooldownCategory::RESOURCE, 180.0f, false, "Shadowfiend"},
            {Pr::Shadow::MINDBENDER_SHADOW, CooldownCategory::RESOURCE, 60.0f, true, "Mindbender"},
        };

        _primaryInterrupts[spec] = Pr::Shadow::SILENCE;
        _interruptSpells[spec] = {Pr::Shadow::SILENCE, Pr::Shadow::PSYCHIC_HORROR, Pr::PSYCHIC_SCREAM};

        _fallbackChains[spec] = {
            {"single_target", {Pr::Shadow::DEVOURING_PLAGUE, Pr::Shadow::MIND_BLAST_SHADOW, Pr::Shadow::MIND_FLAY, Pr::Shadow::VAMPIRIC_TOUCH}},
            {"aoe", {Pr::Shadow::SHADOW_CRASH, Pr::Shadow::MIND_SEAR, Pr::Shadow::VAMPIRIC_TOUCH}},
        };
    }
}

// ============================================================================
// Rogue Initialization (Assassination=0, Outlaw=1, Subtlety=2)
// ============================================================================

void ClassSpellDatabase::InitializeRogue()
{
    // --- Assassination ---
    {
        auto spec = SPEC(ROGUE, 0);

        SpecStatWeights sw;
        sw.spec = spec;
        sw.SetWeight(SpellStatType::AGILITY, 1.0f);
        sw.SetWeight(SpellStatType::MASTERY, 0.9f);
        sw.SetWeight(SpellStatType::CRITICAL_STRIKE, 0.8f);
        sw.SetWeight(SpellStatType::HASTE, 0.75f);
        sw.SetWeight(SpellStatType::VERSATILITY, 0.7f);
        _statWeights[spec] = sw;

        _defensiveSpells[spec] = {
            {Ro::CLOAK_OF_SHADOWS, DefensiveCategory::PERSONAL_MAJOR, 30.0f, 120.0f, "Cloak of Shadows"},
            {Ro::EVASION, DefensiveCategory::PERSONAL_MAJOR, 40.0f, 120.0f, "Evasion"},
            {Ro::CRIMSON_VIAL, DefensiveCategory::SELF_HEAL, 60.0f, 30.0f, "Crimson Vial"},
            {Ro::FEINT, DefensiveCategory::PERSONAL_MINOR, 75.0f, 15.0f, "Feint"},
            {Ro::VANISH, DefensiveCategory::PERSONAL_MAJOR, 25.0f, 120.0f, "Vanish"},
        };

        _cooldownSpells[spec] = {
            {Ro::Assassination::VENDETTA, CooldownCategory::OFFENSIVE_MAJOR, 120.0f, false, "Vendetta"},
            {Ro::Assassination::DEATHMARK, CooldownCategory::OFFENSIVE_MAJOR, 120.0f, false, "Deathmark"},
            {Ro::Assassination::KINGSBANE, CooldownCategory::OFFENSIVE_MINOR, 60.0f, true, "Kingsbane"},
        };

        _primaryInterrupts[spec] = Ro::KICK;
        _interruptSpells[spec] = {Ro::KICK, Ro::KIDNEY_SHOT, Ro::BLIND};

        _fallbackChains[spec] = {
            {"single_target", {Ro::Assassination::ENVENOM, Ro::Assassination::MUTILATE, Ro::Assassination::GARROTE, Ro::Assassination::RUPTURE}},
            {"aoe", {Ro::Assassination::CRIMSON_TEMPEST, Ro::FAN_OF_KNIVES, Ro::Assassination::GARROTE}},
        };
    }

    // --- Outlaw ---
    {
        auto spec = SPEC(ROGUE, 1);

        SpecStatWeights sw;
        sw.spec = spec;
        sw.SetWeight(SpellStatType::AGILITY, 1.0f);
        sw.SetWeight(SpellStatType::HASTE, 0.85f);
        sw.SetWeight(SpellStatType::VERSATILITY, 0.8f);
        sw.SetWeight(SpellStatType::CRITICAL_STRIKE, 0.75f);
        sw.SetWeight(SpellStatType::MASTERY, 0.7f);
        _statWeights[spec] = sw;

        _defensiveSpells[spec] = {
            {Ro::CLOAK_OF_SHADOWS, DefensiveCategory::PERSONAL_MAJOR, 30.0f, 120.0f, "Cloak of Shadows"},
            {Ro::EVASION, DefensiveCategory::PERSONAL_MAJOR, 40.0f, 120.0f, "Evasion"},
            {Ro::CRIMSON_VIAL, DefensiveCategory::SELF_HEAL, 60.0f, 30.0f, "Crimson Vial"},
            {Ro::FEINT, DefensiveCategory::PERSONAL_MINOR, 75.0f, 15.0f, "Feint"},
            {Ro::VANISH, DefensiveCategory::PERSONAL_MAJOR, 25.0f, 120.0f, "Vanish"},
        };

        _cooldownSpells[spec] = {
            {Ro::Outlaw::ADRENALINE_RUSH, CooldownCategory::OFFENSIVE_MAJOR, 180.0f, false, "Adrenaline Rush"},
            {Ro::Outlaw::BLADE_FLURRY, CooldownCategory::OFFENSIVE_MINOR, 30.0f, true, "Blade Flurry"},
            {Ro::Outlaw::KILLING_SPREE, CooldownCategory::OFFENSIVE_MINOR, 120.0f, false, "Killing Spree"},
        };

        _primaryInterrupts[spec] = Ro::KICK;
        _interruptSpells[spec] = {Ro::KICK, Ro::Outlaw::BETWEEN_THE_EYES, Ro::BLIND};

        _fallbackChains[spec] = {
            {"single_target", {Ro::Outlaw::DISPATCH, Ro::Outlaw::SINISTER_STRIKE, Ro::Outlaw::PISTOL_SHOT}},
            {"aoe", {Ro::Outlaw::BLADE_FLURRY, Ro::FAN_OF_KNIVES, Ro::Outlaw::SINISTER_STRIKE}},
        };
    }

    // --- Subtlety ---
    {
        auto spec = SPEC(ROGUE, 2);

        SpecStatWeights sw;
        sw.spec = spec;
        sw.SetWeight(SpellStatType::AGILITY, 1.0f);
        sw.SetWeight(SpellStatType::VERSATILITY, 0.85f);
        sw.SetWeight(SpellStatType::CRITICAL_STRIKE, 0.8f);
        sw.SetWeight(SpellStatType::MASTERY, 0.75f);
        sw.SetWeight(SpellStatType::HASTE, 0.7f);
        _statWeights[spec] = sw;

        _defensiveSpells[spec] = {
            {Ro::CLOAK_OF_SHADOWS, DefensiveCategory::PERSONAL_MAJOR, 30.0f, 120.0f, "Cloak of Shadows"},
            {Ro::EVASION, DefensiveCategory::PERSONAL_MAJOR, 40.0f, 120.0f, "Evasion"},
            {Ro::CRIMSON_VIAL, DefensiveCategory::SELF_HEAL, 60.0f, 30.0f, "Crimson Vial"},
            {Ro::FEINT, DefensiveCategory::PERSONAL_MINOR, 75.0f, 15.0f, "Feint"},
            {Ro::VANISH, DefensiveCategory::PERSONAL_MAJOR, 25.0f, 120.0f, "Vanish"},
        };

        _cooldownSpells[spec] = {
            {Ro::Subtlety::SHADOW_BLADES, CooldownCategory::OFFENSIVE_MAJOR, 180.0f, false, "Shadow Blades"},
            {Ro::Subtlety::SHADOW_DANCE, CooldownCategory::OFFENSIVE_MINOR, 60.0f, true, "Shadow Dance"},
            {Ro::Subtlety::SYMBOLS_OF_DEATH, CooldownCategory::OFFENSIVE_MINOR, 30.0f, true, "Symbols of Death"},
            {Ro::Subtlety::COLD_BLOOD, CooldownCategory::OFFENSIVE_MINOR, 45.0f, true, "Cold Blood"},
        };

        _primaryInterrupts[spec] = Ro::KICK;
        _interruptSpells[spec] = {Ro::KICK, Ro::KIDNEY_SHOT, Ro::BLIND};

        _fallbackChains[spec] = {
            {"single_target", {Ro::Subtlety::EVISCERATE, Ro::Subtlety::SHADOWSTRIKE, Ro::Subtlety::BACKSTAB}},
            {"aoe", {Ro::Subtlety::SHURIKEN_STORM, Ro::Subtlety::BLACK_POWDER, Ro::Subtlety::SECRET_TECHNIQUE}},
        };
    }
}

// ============================================================================
// Shaman Initialization (Elemental=0, Enhancement=1, Restoration=2)
// ============================================================================

void ClassSpellDatabase::InitializeShaman()
{
    // --- Elemental (Ranged DPS) ---
    {
        auto spec = SPEC(SHAMAN, 0);

        SpecStatWeights sw;
        sw.spec = spec;
        sw.SetWeight(SpellStatType::INTELLECT, 1.0f);
        sw.SetWeight(SpellStatType::CRITICAL_STRIKE, 0.85f);
        sw.SetWeight(SpellStatType::HASTE, 0.8f);
        sw.SetWeight(SpellStatType::VERSATILITY, 0.75f);
        sw.SetWeight(SpellStatType::MASTERY, 0.7f);
        _statWeights[spec] = sw;

        _defensiveSpells[spec] = {
            {Sh::ASTRAL_SHIFT, DefensiveCategory::PERSONAL_MAJOR, 40.0f, 90.0f, "Astral Shift"},
        };

        _cooldownSpells[spec] = {
            {Sh::Elemental::STORMKEEPER, CooldownCategory::OFFENSIVE_MAJOR, 60.0f, false, "Stormkeeper"},
            {Sh::Elemental::ASCENDANCE, CooldownCategory::OFFENSIVE_MAJOR, 180.0f, false, "Ascendance"},
            {Sh::Elemental::FIRE_ELEMENTAL, CooldownCategory::OFFENSIVE_MAJOR, 150.0f, false, "Fire Elemental"},
            {Sh::BLOODLUST, CooldownCategory::OFFENSIVE_MAJOR, 300.0f, false, "Bloodlust"},
        };

        _primaryInterrupts[spec] = Sh::WIND_SHEAR;
        _interruptSpells[spec] = {Sh::WIND_SHEAR, Sh::CAPACITOR_TOTEM};

        _fallbackChains[spec] = {
            {"single_target", {Sh::LAVA_BURST, Sh::EARTH_SHOCK, Sh::LIGHTNING_BOLT}},
            {"aoe", {Sh::Elemental::EARTHQUAKE, Sh::CHAIN_LIGHTNING, Sh::LAVA_BURST}},
        };
    }

    // --- Enhancement (Melee DPS) ---
    {
        auto spec = SPEC(SHAMAN, 1);

        SpecStatWeights sw;
        sw.spec = spec;
        sw.SetWeight(SpellStatType::AGILITY, 1.0f);
        sw.SetWeight(SpellStatType::HASTE, 0.85f);
        sw.SetWeight(SpellStatType::MASTERY, 0.8f);
        sw.SetWeight(SpellStatType::CRITICAL_STRIKE, 0.75f);
        sw.SetWeight(SpellStatType::VERSATILITY, 0.7f);
        _statWeights[spec] = sw;

        _defensiveSpells[spec] = {
            {Sh::ASTRAL_SHIFT, DefensiveCategory::PERSONAL_MAJOR, 40.0f, 90.0f, "Astral Shift"},
        };

        _cooldownSpells[spec] = {
            {Sh::Enhancement::FERAL_SPIRIT, CooldownCategory::OFFENSIVE_MAJOR, 90.0f, false, "Feral Spirit"},
            {Sh::Enhancement::ASCENDANCE_ENH, CooldownCategory::OFFENSIVE_MAJOR, 180.0f, false, "Ascendance"},
            {Sh::Enhancement::DOOM_WINDS, CooldownCategory::OFFENSIVE_MINOR, 60.0f, true, "Doom Winds"},
            {Sh::BLOODLUST, CooldownCategory::OFFENSIVE_MAJOR, 300.0f, false, "Bloodlust"},
        };

        _primaryInterrupts[spec] = Sh::WIND_SHEAR;
        _interruptSpells[spec] = {Sh::WIND_SHEAR, Sh::CAPACITOR_TOTEM};

        _fallbackChains[spec] = {
            {"single_target", {Sh::Enhancement::STORMSTRIKE, Sh::Enhancement::LAVA_LASH, Sh::Enhancement::ICE_STRIKE, Sh::FLAME_SHOCK}},
            {"aoe", {Sh::Enhancement::CRASH_LIGHTNING, Sh::Enhancement::SUNDERING, Sh::Enhancement::FIRE_NOVA}},
        };
    }

    // --- Restoration (Healer) ---
    {
        auto spec = SPEC(SHAMAN, 2);

        SpecStatWeights sw;
        sw.spec = spec;
        sw.SetWeight(SpellStatType::INTELLECT, 1.0f);
        sw.SetWeight(SpellStatType::CRITICAL_STRIKE, 0.85f);
        sw.SetWeight(SpellStatType::VERSATILITY, 0.8f);
        sw.SetWeight(SpellStatType::HASTE, 0.75f);
        sw.SetWeight(SpellStatType::MASTERY, 0.7f);
        _statWeights[spec] = sw;

        _defensiveSpells[spec] = {
            {Sh::ASTRAL_SHIFT, DefensiveCategory::PERSONAL_MAJOR, 40.0f, 90.0f, "Astral Shift"},
            {Sh::Restoration::SPIRIT_LINK_TOTEM, DefensiveCategory::RAID_WIDE, 40.0f, 180.0f, "Spirit Link Totem"},
            {Sh::Restoration::HEALING_TIDE_TOTEM, DefensiveCategory::RAID_WIDE, 50.0f, 180.0f, "Healing Tide Totem"},
            {Sh::Restoration::EARTHEN_WALL_TOTEM, DefensiveCategory::RAID_WIDE, 60.0f, 60.0f, "Earthen Wall Totem"},
        };

        _cooldownSpells[spec] = {
            {Sh::Restoration::ASCENDANCE_RESTO, CooldownCategory::OFFENSIVE_MAJOR, 180.0f, false, "Ascendance"},
            {Sh::Restoration::MANA_TIDE_TOTEM, CooldownCategory::RESOURCE, 180.0f, false, "Mana Tide Totem"},
            {Sh::Restoration::SPIRITWALKERS_GRACE, CooldownCategory::UTILITY, 120.0f, false, "Spiritwalker's Grace"},
            {Sh::BLOODLUST, CooldownCategory::OFFENSIVE_MAJOR, 300.0f, false, "Bloodlust"},
        };

        _primaryInterrupts[spec] = Sh::WIND_SHEAR;
        _interruptSpells[spec] = {Sh::WIND_SHEAR, Sh::CAPACITOR_TOTEM};

        _healingTiers[spec] = {
            {Sh::Restoration::RIPTIDE, SpellEfficiencyTier::VERY_HIGH, "Riptide"},
            {Sh::Restoration::HEALING_WAVE, SpellEfficiencyTier::VERY_HIGH, "Healing Wave"},
            {Sh::Restoration::EARTH_SHIELD, SpellEfficiencyTier::VERY_HIGH, "Earth Shield"},
            {Sh::Restoration::HEALING_SURGE, SpellEfficiencyTier::HIGH, "Healing Surge"},
            {Sh::Restoration::CHAIN_HEAL, SpellEfficiencyTier::MEDIUM, "Chain Heal"},
            {Sh::Restoration::HEALING_RAIN, SpellEfficiencyTier::MEDIUM, "Healing Rain"},
            {Sh::Restoration::WELLSPRING, SpellEfficiencyTier::LOW, "Wellspring"},
            {Sh::Restoration::HEALING_TIDE_TOTEM, SpellEfficiencyTier::EMERGENCY, "Healing Tide Totem"},
            {Sh::Restoration::SPIRIT_LINK_TOTEM, SpellEfficiencyTier::EMERGENCY, "Spirit Link Totem"},
        };

        _fallbackChains[spec] = {
            {"single_target_heal", {Sh::Restoration::RIPTIDE, Sh::Restoration::HEALING_SURGE, Sh::Restoration::HEALING_WAVE}},
            {"aoe_heal", {Sh::Restoration::CHAIN_HEAL, Sh::Restoration::HEALING_RAIN, Sh::Restoration::HEALING_TIDE_TOTEM}},
        };
    }
}

// ============================================================================
// Warlock Initialization (Affliction=0, Demonology=1, Destruction=2)
// ============================================================================

void ClassSpellDatabase::InitializeWarlock()
{
    // --- Affliction ---
    {
        auto spec = SPEC(WARLOCK, 0);

        SpecStatWeights sw;
        sw.spec = spec;
        sw.SetWeight(SpellStatType::INTELLECT, 1.0f);
        sw.SetWeight(SpellStatType::HASTE, 0.85f);
        sw.SetWeight(SpellStatType::MASTERY, 0.8f);
        sw.SetWeight(SpellStatType::CRITICAL_STRIKE, 0.75f);
        sw.SetWeight(SpellStatType::VERSATILITY, 0.7f);
        _statWeights[spec] = sw;

        _defensiveSpells[spec] = {
            {Wl::UNENDING_RESOLVE, DefensiveCategory::PERSONAL_MAJOR, 35.0f, 180.0f, "Unending Resolve"},
            {Wl::Affliction::DARK_PACT, DefensiveCategory::PERSONAL_MINOR, 50.0f, 60.0f, "Dark Pact"},
            {Wl::DRAIN_LIFE, DefensiveCategory::SELF_HEAL, 50.0f, 0.0f, "Drain Life"},
        };

        _cooldownSpells[spec] = {
            {Wl::Affliction::DARK_SOUL_MISERY, CooldownCategory::OFFENSIVE_MAJOR, 120.0f, false, "Dark Soul: Misery"},
            {Wl::Affliction::SUMMON_DARKGLARE, CooldownCategory::OFFENSIVE_MAJOR, 120.0f, false, "Summon Darkglare"},
            {Wl::Affliction::SOUL_ROT, CooldownCategory::OFFENSIVE_MINOR, 60.0f, true, "Soul Rot"},
        };

        _primaryInterrupts[spec] = Wl::SPELL_LOCK;
        _interruptSpells[spec] = {Wl::SPELL_LOCK, Wl::SHADOWFURY, Wl::FEAR};

        _fallbackChains[spec] = {
            {"single_target", {Wl::Affliction::MALEFIC_RAPTURE, Wl::Affliction::DRAIN_SOUL, Wl::SHADOW_BOLT}},
            {"aoe", {Wl::Affliction::SEED_OF_CORRUPTION, Wl::Affliction::VILE_TAINT, Wl::Affliction::MALEFIC_RAPTURE}},
        };
    }

    // --- Demonology ---
    {
        auto spec = SPEC(WARLOCK, 1);

        SpecStatWeights sw;
        sw.spec = spec;
        sw.SetWeight(SpellStatType::INTELLECT, 1.0f);
        sw.SetWeight(SpellStatType::HASTE, 0.85f);
        sw.SetWeight(SpellStatType::CRITICAL_STRIKE, 0.8f);
        sw.SetWeight(SpellStatType::MASTERY, 0.75f);
        sw.SetWeight(SpellStatType::VERSATILITY, 0.7f);
        _statWeights[spec] = sw;

        _defensiveSpells[spec] = {
            {Wl::UNENDING_RESOLVE, DefensiveCategory::PERSONAL_MAJOR, 35.0f, 180.0f, "Unending Resolve"},
            {Wl::DRAIN_LIFE, DefensiveCategory::SELF_HEAL, 50.0f, 0.0f, "Drain Life"},
        };

        _cooldownSpells[spec] = {
            {Wl::Demonology::SUMMON_DEMONIC_TYRANT, CooldownCategory::OFFENSIVE_MAJOR, 90.0f, false, "Summon Demonic Tyrant"},
            {Wl::Demonology::NETHER_PORTAL, CooldownCategory::OFFENSIVE_MAJOR, 180.0f, false, "Nether Portal"},
            {Wl::Demonology::GRIMOIRE_FELGUARD, CooldownCategory::OFFENSIVE_MINOR, 120.0f, false, "Grimoire: Felguard"},
        };

        _primaryInterrupts[spec] = Wl::SPELL_LOCK;
        _interruptSpells[spec] = {Wl::SPELL_LOCK, Wl::SHADOWFURY, Wl::FEAR};

        _fallbackChains[spec] = {
            {"single_target", {Wl::Demonology::DEMONBOLT, Wl::Demonology::HAND_OF_GULDAN, Wl::Demonology::CALL_DREADSTALKERS, Wl::SHADOW_BOLT}},
            {"aoe", {Wl::Demonology::IMPLOSION, Wl::Demonology::HAND_OF_GULDAN, Wl::Demonology::BILESCOURGE_BOMBERS}},
        };
    }

    // --- Destruction ---
    {
        auto spec = SPEC(WARLOCK, 2);

        SpecStatWeights sw;
        sw.spec = spec;
        sw.SetWeight(SpellStatType::INTELLECT, 1.0f);
        sw.SetWeight(SpellStatType::HASTE, 0.85f);
        sw.SetWeight(SpellStatType::CRITICAL_STRIKE, 0.8f);
        sw.SetWeight(SpellStatType::MASTERY, 0.75f);
        sw.SetWeight(SpellStatType::VERSATILITY, 0.7f);
        _statWeights[spec] = sw;

        _defensiveSpells[spec] = {
            {Wl::UNENDING_RESOLVE, DefensiveCategory::PERSONAL_MAJOR, 35.0f, 180.0f, "Unending Resolve"},
            {Wl::DRAIN_LIFE, DefensiveCategory::SELF_HEAL, 50.0f, 0.0f, "Drain Life"},
        };

        _cooldownSpells[spec] = {
            {Wl::Destruction::DARK_SOUL_INSTABILITY, CooldownCategory::OFFENSIVE_MAJOR, 120.0f, false, "Dark Soul: Instability"},
            {Wl::Destruction::SUMMON_INFERNAL, CooldownCategory::OFFENSIVE_MAJOR, 180.0f, false, "Summon Infernal"},
        };

        _primaryInterrupts[spec] = Wl::SPELL_LOCK;
        _interruptSpells[spec] = {Wl::SPELL_LOCK, Wl::SHADOWFURY, Wl::FEAR};

        _fallbackChains[spec] = {
            {"single_target", {Wl::Destruction::CHAOS_BOLT, Wl::Destruction::CONFLAGRATE, Wl::Destruction::INCINERATE, Wl::Destruction::IMMOLATE}},
            {"aoe", {Wl::Destruction::RAIN_OF_FIRE, Wl::Destruction::CATACLYSM, Wl::Destruction::CHANNEL_DEMONFIRE, Wl::Destruction::INCINERATE}},
        };
    }
}

// ============================================================================
// Warrior Initialization (Arms=0, Fury=1, Protection=2)
// ============================================================================

void ClassSpellDatabase::InitializeWarrior()
{
    // --- Arms (Melee DPS) ---
    {
        auto spec = SPEC(WARRIOR, 0);

        SpecStatWeights sw;
        sw.spec = spec;
        sw.SetWeight(SpellStatType::STRENGTH, 1.0f);
        sw.SetWeight(SpellStatType::CRITICAL_STRIKE, 0.85f);
        sw.SetWeight(SpellStatType::MASTERY, 0.8f);
        sw.SetWeight(SpellStatType::HASTE, 0.75f);
        sw.SetWeight(SpellStatType::VERSATILITY, 0.7f);
        _statWeights[spec] = sw;

        _defensiveSpells[spec] = {
            {Wr::Arms::DIE_BY_THE_SWORD, DefensiveCategory::PERSONAL_MAJOR, 35.0f, 180.0f, "Die by the Sword"},
            {Wr::SPELL_REFLECTION, DefensiveCategory::PERSONAL_MINOR, 60.0f, 25.0f, "Spell Reflection"},
            {Wr::RALLYING_CRY, DefensiveCategory::RAID_WIDE, 50.0f, 180.0f, "Rallying Cry"},
            {Wr::VICTORY_RUSH, DefensiveCategory::SELF_HEAL, 70.0f, 0.0f, "Victory Rush"},
        };

        _cooldownSpells[spec] = {
            {Wr::Arms::AVATAR, CooldownCategory::OFFENSIVE_MAJOR, 90.0f, false, "Avatar"},
            {Wr::Arms::BLADESTORM, CooldownCategory::OFFENSIVE_MAJOR, 90.0f, false, "Bladestorm"},
            {Wr::Arms::COLOSSUS_SMASH, CooldownCategory::OFFENSIVE_MINOR, 45.0f, true, "Colossus Smash"},
            {Wr::Arms::WARBREAKER, CooldownCategory::OFFENSIVE_MINOR, 45.0f, true, "Warbreaker"},
            {Wr::Arms::THUNDEROUS_ROAR, CooldownCategory::OFFENSIVE_MINOR, 90.0f, false, "Thunderous Roar"},
            {Wr::Arms::CHAMPIONS_SPEAR, CooldownCategory::OFFENSIVE_MINOR, 90.0f, false, "Champion's Spear"},
        };

        _primaryInterrupts[spec] = Wr::PUMMEL;
        _interruptSpells[spec] = {Wr::PUMMEL, Wr::STORM_BOLT, Wr::SHOCKWAVE};

        _fallbackChains[spec] = {
            {"single_target", {Wr::Arms::MORTAL_STRIKE, Wr::Arms::OVERPOWER, Wr::Arms::EXECUTE, Wr::Arms::SLAM}},
            {"aoe", {Wr::Arms::BLADESTORM, Wr::Arms::WHIRLWIND, Wr::Arms::CLEAVE}},
        };
    }

    // --- Fury (Melee DPS) ---
    {
        auto spec = SPEC(WARRIOR, 1);

        SpecStatWeights sw;
        sw.spec = spec;
        sw.SetWeight(SpellStatType::STRENGTH, 1.0f);
        sw.SetWeight(SpellStatType::HASTE, 0.85f);
        sw.SetWeight(SpellStatType::MASTERY, 0.8f);
        sw.SetWeight(SpellStatType::CRITICAL_STRIKE, 0.75f);
        sw.SetWeight(SpellStatType::VERSATILITY, 0.7f);
        _statWeights[spec] = sw;

        _defensiveSpells[spec] = {
            {Wr::Fury::ENRAGED_REGENERATION, DefensiveCategory::SELF_HEAL, 45.0f, 120.0f, "Enraged Regeneration"},
            {Wr::SPELL_REFLECTION, DefensiveCategory::PERSONAL_MINOR, 60.0f, 25.0f, "Spell Reflection"},
            {Wr::RALLYING_CRY, DefensiveCategory::RAID_WIDE, 50.0f, 180.0f, "Rallying Cry"},
            {Wr::VICTORY_RUSH, DefensiveCategory::SELF_HEAL, 70.0f, 0.0f, "Victory Rush"},
        };

        _cooldownSpells[spec] = {
            {Wr::Fury::RECKLESSNESS, CooldownCategory::OFFENSIVE_MAJOR, 90.0f, false, "Recklessness"},
            {Wr::Fury::BLADESTORM, CooldownCategory::OFFENSIVE_MAJOR, 90.0f, false, "Bladestorm"},
            {Wr::Fury::ODYN_FURY, CooldownCategory::OFFENSIVE_MINOR, 45.0f, true, "Odyn's Fury"},
            {Wr::Fury::THUNDEROUS_ROAR, CooldownCategory::OFFENSIVE_MINOR, 90.0f, false, "Thunderous Roar"},
            {Wr::Fury::CHAMPIONS_SPEAR, CooldownCategory::OFFENSIVE_MINOR, 90.0f, false, "Champion's Spear"},
        };

        _primaryInterrupts[spec] = Wr::PUMMEL;
        _interruptSpells[spec] = {Wr::PUMMEL, Wr::STORM_BOLT, Wr::SHOCKWAVE};

        _fallbackChains[spec] = {
            {"single_target", {Wr::Fury::RAMPAGE, Wr::Fury::BLOODTHIRST, Wr::Fury::RAGING_BLOW, Wr::Fury::EXECUTE}},
            {"aoe", {Wr::Fury::WHIRLWIND, Wr::Fury::BLADESTORM, Wr::Fury::RAMPAGE}},
        };
    }

    // --- Protection (Tank) ---
    {
        auto spec = SPEC(WARRIOR, 2);

        SpecStatWeights sw;
        sw.spec = spec;
        sw.SetWeight(SpellStatType::STRENGTH, 1.0f);
        sw.SetWeight(SpellStatType::STAMINA, 0.9f);
        sw.SetWeight(SpellStatType::HASTE, 0.85f);
        sw.SetWeight(SpellStatType::VERSATILITY, 0.8f);
        sw.SetWeight(SpellStatType::MASTERY, 0.75f);
        sw.SetWeight(SpellStatType::CRITICAL_STRIKE, 0.65f);
        _statWeights[spec] = sw;

        _defensiveSpells[spec] = {
            {Wr::Protection::SHIELD_BLOCK, DefensiveCategory::PERSONAL_MINOR, 80.0f, 16.0f, "Shield Block"},
            {Wr::Protection::IGNORE_PAIN, DefensiveCategory::PERSONAL_MINOR, 70.0f, 0.0f, "Ignore Pain"},
            {Wr::Protection::SHIELD_WALL, DefensiveCategory::PERSONAL_MAJOR, 25.0f, 240.0f, "Shield Wall"},
            {Wr::Protection::LAST_STAND, DefensiveCategory::PERSONAL_MAJOR, 35.0f, 180.0f, "Last Stand"},
            {Wr::Protection::DEMORALIZING_SHOUT, DefensiveCategory::PERSONAL_MINOR, 60.0f, 45.0f, "Demoralizing Shout"},
            {Wr::SPELL_REFLECTION, DefensiveCategory::PERSONAL_MINOR, 65.0f, 25.0f, "Spell Reflection"},
            {Wr::RALLYING_CRY, DefensiveCategory::RAID_WIDE, 50.0f, 180.0f, "Rallying Cry"},
        };

        _cooldownSpells[spec] = {
            {Wr::Protection::AVATAR, CooldownCategory::OFFENSIVE_MAJOR, 90.0f, false, "Avatar"},
            {Wr::Protection::THUNDEROUS_ROAR, CooldownCategory::OFFENSIVE_MINOR, 90.0f, false, "Thunderous Roar"},
            {Wr::Protection::CHAMPIONS_SPEAR, CooldownCategory::OFFENSIVE_MINOR, 90.0f, false, "Champion's Spear"},
        };

        _primaryInterrupts[spec] = Wr::PUMMEL;
        _interruptSpells[spec] = {Wr::PUMMEL, Wr::STORM_BOLT, Wr::SHOCKWAVE, Wr::Protection::DISRUPTING_SHOUT};

        _fallbackChains[spec] = {
            {"single_target", {Wr::Protection::SHIELD_SLAM, Wr::Protection::THUNDER_CLAP, Wr::Protection::REVENGE, Wr::Protection::DEVASTATE}},
            {"aoe", {Wr::Protection::THUNDER_CLAP, Wr::Protection::REVENGE, Wr::Protection::SHIELD_SLAM}},
        };
    }
}

#undef SPEC

} // namespace Playerbot
