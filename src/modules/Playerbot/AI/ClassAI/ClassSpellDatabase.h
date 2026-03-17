/*
 * Copyright (C) 2024-2026 TrinityCore <https://www.trinitycore.org/>
 *
 * ClassSpellDatabase: Centralized, static, read-only database of per-class/spec
 * spell metadata. Follows the InterruptDatabase pattern (static Initialize(),
 * static maps, static query methods).
 *
 * Contains:
 *   - Rotation templates (ordered spell lists per phase per spec)
 *   - Stat weights per spec
 *   - Defensive, interrupt, and cooldown spell lists per spec
 *   - Healing spell tier mappings (for HealingEfficiencyManager)
 *   - Spell fallback chain definitions per spec
 */

#pragma once

#include "Define.h"
#include "ClassBehaviorTreeRegistry.h" // WowClass, ClassSpec, ClassSpecHash, SpecRole
#include "ActionPriority.h"            // ActionPriority
#include <unordered_map>
#include <vector>
#include <string>

namespace Playerbot
{

// ============================================================================
// Enumerations
// ============================================================================

/// Stat types for spell database stat weight tables.
/// Named SpellStatType to avoid collision with Equipment::StatType.
enum class SpellStatType : uint8
{
    STRENGTH = 0,
    AGILITY,
    INTELLECT,
    STAMINA,
    CRITICAL_STRIKE,
    HASTE,
    MASTERY,
    VERSATILITY,
    LEECH,
    AVOIDANCE,
    SPEED,

    MAX_STAT_TYPE
};

/// Healing spell efficiency tier (mirrors HealingSpellTierData.h)
enum class SpellEfficiencyTier : uint8
{
    EMERGENCY   = 0,  // Always allowed (e.g. Guardian Spirit, Lay on Hands)
    VERY_HIGH   = 1,  // Always allowed (e.g. Heal, Renew)
    HIGH        = 2,  // Blocked below 30% mana
    MEDIUM      = 3,  // Blocked below 50% mana
    LOW         = 4,  // Blocked below 70% mana

    MAX_TIER
};

/// Defensive spell category
enum class DefensiveCategory : uint8
{
    PERSONAL_MAJOR = 0,   // Major personal CD (Icebound Fortitude, Divine Shield)
    PERSONAL_MINOR,       // Minor personal CD (Anti-Magic Shell, Barkskin)
    EXTERNAL_MAJOR,       // Major external CD on ally (Guardian Spirit, Blessing of Sacrifice)
    EXTERNAL_MINOR,       // Minor external CD on ally (Ironbark)
    RAID_WIDE,            // Raid-wide CD (Spirit Link Totem, Aura Mastery)
    SELF_HEAL,            // Self-heal ability (Death Strike, Victory Rush)

    MAX_CATEGORY
};

/// Cooldown spell category
enum class CooldownCategory : uint8
{
    OFFENSIVE_MAJOR = 0,  // Major DPS CD (Pillar of Frost, Avenging Wrath)
    OFFENSIVE_MINOR,      // Minor DPS CD (Mirror Image, Berserking)
    UTILITY,              // Utility CD (Stampeding Roar, Heroic Leap)
    RESOURCE,             // Resource CD (Innervate, Empower Rune Weapon)

    MAX_CATEGORY
};

// ============================================================================
// Data Structures
// ============================================================================

/// A single spell in a rotation phase
struct RotationSpell
{
    uint32 spellId;
    float basePriority;         // Priority within this phase (higher = more important)
    bool requiresTarget;        // Needs a valid target
    bool requiresMelee;         // Must be in melee range
    uint32 minResourceCost;     // Minimum resource to consider (0 = unchecked)
    ::std::string name;         // Human-readable name for logging

    RotationSpell()
        : spellId(0), basePriority(0.0f), requiresTarget(true)
        , requiresMelee(false), minResourceCost(0) {}

    RotationSpell(uint32 id, float prio, bool target, bool melee, uint32 resource,
                  ::std::string n)
        : spellId(id), basePriority(prio), requiresTarget(target)
        , requiresMelee(melee), minResourceCost(resource), name(::std::move(n)) {}
};

/// Rotation template for one phase of combat
struct RotationPhase
{
    ActionPriority priority;                // Phase priority
    ::std::vector<RotationSpell> spells;    // Ordered spell list for this phase
};

/// Complete rotation template for a specialization
struct SpecRotationTemplate
{
    ClassSpec spec;
    SpecRole role;
    ::std::vector<RotationPhase> phases;    // Phases in priority order
};

/// Stat weight entry for a specialization
struct SpecStatWeights
{
    ClassSpec spec;
    float weights[static_cast<size_t>(SpellStatType::MAX_STAT_TYPE)];

    SpecStatWeights()
    {
        for (auto& w : weights)
            w = 0.0f;
    }

    float GetWeight(SpellStatType stat) const
    {
        return weights[static_cast<size_t>(stat)];
    }

    void SetWeight(SpellStatType stat, float value)
    {
        weights[static_cast<size_t>(stat)] = value;
    }
};

/// Defensive spell entry
struct DefensiveSpellEntry
{
    uint32 spellId;
    DefensiveCategory category;
    float healthThreshold;      // Use when health < this % (0 = manual use only)
    float cooldownSeconds;
    ::std::string name;

    DefensiveSpellEntry()
        : spellId(0), category(DefensiveCategory::PERSONAL_MAJOR)
        , healthThreshold(0.0f), cooldownSeconds(0.0f) {}

    DefensiveSpellEntry(uint32 id, DefensiveCategory cat, float threshold, float cd,
                        ::std::string n)
        : spellId(id), category(cat), healthThreshold(threshold)
        , cooldownSeconds(cd), name(::std::move(n)) {}
};

/// Cooldown spell entry
struct CooldownSpellEntry
{
    uint32 spellId;
    CooldownCategory category;
    float cooldownSeconds;
    bool useOnCooldown;         // True = use ASAP, false = save for burst windows
    ::std::string name;

    CooldownSpellEntry()
        : spellId(0), category(CooldownCategory::OFFENSIVE_MAJOR)
        , cooldownSeconds(0.0f), useOnCooldown(false) {}

    CooldownSpellEntry(uint32 id, CooldownCategory cat, float cd, bool onCD,
                       ::std::string n)
        : spellId(id), category(cat), cooldownSeconds(cd)
        , useOnCooldown(onCD), name(::std::move(n)) {}
};

/// Healing tier entry for HealingEfficiencyManager integration
struct HealingTierEntry
{
    uint32 spellId;
    SpellEfficiencyTier tier;
    ::std::string name;

    HealingTierEntry() : spellId(0), tier(SpellEfficiencyTier::MEDIUM) {}
    HealingTierEntry(uint32 id, SpellEfficiencyTier t, ::std::string n)
        : spellId(id), tier(t), name(::std::move(n)) {}
};

/// Fallback chain entry: ordered list of alternative spells
struct FallbackChainEntry
{
    ::std::string chainName;            // E.g. "single_target_heal", "aoe_damage"
    ::std::vector<uint32> spellIds;     // Ordered from primary to last resort

    FallbackChainEntry() = default;
    FallbackChainEntry(::std::string name, ::std::vector<uint32> ids)
        : chainName(::std::move(name)), spellIds(::std::move(ids)) {}
};

// ============================================================================
// ClassSpellDatabase - Static Database
// ============================================================================

class TC_GAME_API ClassSpellDatabase
{
public:
    /// Initialize the database with all class/spec data.
    /// Called once at server startup (from PlayerbotModule initialization).
    static void Initialize();

    /// Check if initialized.
    static bool IsInitialized() { return _initialized; }

    // ========================================================================
    // Rotation Queries
    // ========================================================================

    /// Get the rotation template for a class/spec.
    /// Returns nullptr if not registered.
    static SpecRotationTemplate const* GetRotationTemplate(WowClass classId, uint8 specId);

    /// Get spells for a specific phase of a spec's rotation.
    static ::std::vector<RotationSpell> const* GetPhaseSpells(
        WowClass classId, uint8 specId, ActionPriority phase);

    // ========================================================================
    // Stat Weight Queries
    // ========================================================================

    /// Get stat weights for a class/spec.
    static SpecStatWeights const* GetStatWeights(WowClass classId, uint8 specId);

    /// Get the primary stat for a class/spec (highest weighted).
    static SpellStatType GetPrimaryStat(WowClass classId, uint8 specId);

    /// Get secondary stats in priority order for a class/spec.
    static ::std::vector<SpellStatType> GetSecondaryStatPriority(WowClass classId, uint8 specId);

    // ========================================================================
    // Defensive Spell Queries
    // ========================================================================

    /// Get all defensive spells for a class/spec.
    static ::std::vector<DefensiveSpellEntry> const* GetDefensiveSpells(
        WowClass classId, uint8 specId);

    /// Get defensive spells filtered by category.
    static ::std::vector<DefensiveSpellEntry> GetDefensiveSpellsByCategory(
        WowClass classId, uint8 specId, DefensiveCategory category);

    /// Get defensive spell to use at a given health percentage.
    static DefensiveSpellEntry const* GetDefensiveForHealth(
        WowClass classId, uint8 specId, float healthPct);

    // ========================================================================
    // Cooldown Spell Queries
    // ========================================================================

    /// Get all cooldown spells for a class/spec.
    static ::std::vector<CooldownSpellEntry> const* GetCooldownSpells(
        WowClass classId, uint8 specId);

    /// Get cooldown spells filtered by category.
    static ::std::vector<CooldownSpellEntry> GetCooldownSpellsByCategory(
        WowClass classId, uint8 specId, CooldownCategory category);

    // ========================================================================
    // Healing Tier Queries
    // ========================================================================

    /// Get all healing tier entries for a healer spec.
    static ::std::vector<HealingTierEntry> const* GetHealingTiers(
        WowClass classId, uint8 specId);

    /// Get the efficiency tier for a specific spell.
    static SpellEfficiencyTier GetSpellTier(WowClass classId, uint8 specId, uint32 spellId);

    // ========================================================================
    // Fallback Chain Queries
    // ========================================================================

    /// Get all fallback chains for a class/spec.
    static ::std::vector<FallbackChainEntry> const* GetFallbackChains(
        WowClass classId, uint8 specId);

    /// Get a specific named fallback chain.
    static FallbackChainEntry const* GetFallbackChain(
        WowClass classId, uint8 specId, ::std::string const& chainName);

    // ========================================================================
    // Interrupt Spell Queries (per-class, supplements InterruptDatabase)
    // ========================================================================

    /// Get the primary interrupt spell for a class/spec.
    static uint32 GetPrimaryInterrupt(WowClass classId, uint8 specId);

    /// Get all interrupt/CC spells for a class/spec.
    static ::std::vector<uint32> const* GetInterruptSpells(WowClass classId, uint8 specId);

private:
    // Per-class initialization methods
    static void InitializeDeathKnight();
    static void InitializeDemonHunter();
    static void InitializeDruid();
    static void InitializeEvoker();
    static void InitializeHunter();
    static void InitializeMage();
    static void InitializeMonk();
    static void InitializePaladin();
    static void InitializePriest();
    static void InitializeRogue();
    static void InitializeShaman();
    static void InitializeWarlock();
    static void InitializeWarrior();

    // Helper to ensure initialization
    static void EnsureInitialized();

    // Storage maps (keyed by ClassSpec)
    static ::std::unordered_map<ClassSpec, SpecRotationTemplate, ClassSpecHash> _rotations;
    static ::std::unordered_map<ClassSpec, SpecStatWeights, ClassSpecHash> _statWeights;
    static ::std::unordered_map<ClassSpec, ::std::vector<DefensiveSpellEntry>, ClassSpecHash> _defensiveSpells;
    static ::std::unordered_map<ClassSpec, ::std::vector<CooldownSpellEntry>, ClassSpecHash> _cooldownSpells;
    static ::std::unordered_map<ClassSpec, ::std::vector<HealingTierEntry>, ClassSpecHash> _healingTiers;
    static ::std::unordered_map<ClassSpec, ::std::vector<FallbackChainEntry>, ClassSpecHash> _fallbackChains;
    static ::std::unordered_map<ClassSpec, ::std::vector<uint32>, ClassSpecHash> _interruptSpells;
    static ::std::unordered_map<ClassSpec, uint32, ClassSpecHash> _primaryInterrupts;
    static bool _initialized;
};

} // namespace Playerbot
