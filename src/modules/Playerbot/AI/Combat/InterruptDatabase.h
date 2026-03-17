/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * WoW 12.0 (The War Within) Interrupt Priority Database
 * Comprehensive spell interrupt priorities and configurations
 */

#pragma once

#include "InterruptManager.h"
#include <unordered_map>
#include <string>
#include <vector>

namespace Playerbot
{

// WoW 12.0 Spell Categories for interrupt prioritization
enum class SpellCategory : uint8
{
    // Healing
    HEAL_SINGLE = 0,
    HEAL_GROUP = 1,
    HEAL_EMERGENCY = 2,
    HEAL_HOT = 3,

    // Damage
    DAMAGE_NUKE = 10,
    DAMAGE_DOT = 11,
    DAMAGE_AOE = 12,
    DAMAGE_CHANNEL = 13,

    // Crowd Control
    CC_HARD = 20,
    CC_SOFT = 21,
    CC_ROOT = 22,
    CC_SLOW = 23,

    // Buffs/Debuffs
    BUFF_DAMAGE = 30,
    BUFF_DEFENSIVE = 31,
    BUFF_UTILITY = 32,
    DEBUFF_DAMAGE = 33,
    DEBUFF_DEFENSIVE = 34,

    // Special
    SUMMON = 40,
    TELEPORT = 41,
    RESURRECT = 42,
    SPECIAL_MECHANIC = 43
};

// Spell interrupt configuration
struct SpellInterruptConfig
{
    uint32 spellId;
    ::std::string spellName;
    InterruptPriority basePriority;
    SpellCategory category;
    bool alwaysInterrupt;      // Must always be interrupted
    bool mythicPlusOnly;        // Only interrupt in M+
    uint8 minMythicLevel;       // Minimum M+ level to interrupt
    float castTimeThreshold;    // Only interrupt if cast time > threshold
    bool requiresQuickResponse; // Needs sub-200ms reaction
    uint32 schoolMask;          // Spell school for lockout
    ::std::string notes;          // Developer notes

    SpellInterruptConfig() : spellId(0), basePriority(InterruptPriority::MODERATE),
                            category(SpellCategory::DAMAGE_NUKE), alwaysInterrupt(false),
                            mythicPlusOnly(false), minMythicLevel(0), castTimeThreshold(0.0f),
                            requiresQuickResponse(false), schoolMask(0) {}

    SpellInterruptConfig(uint32 id, ::std::string name, InterruptPriority priority, SpellCategory cat,
                        bool always, bool mythicOnly, uint8 mythicLevel, float castTime,
                        bool quickResponse, uint32 school, ::std::string note)
        : spellId(id), spellName(name), basePriority(priority), category(cat),
          alwaysInterrupt(always), mythicPlusOnly(mythicOnly), minMythicLevel(mythicLevel),
          castTimeThreshold(castTime), requiresQuickResponse(quickResponse), schoolMask(school), notes(note) {}
};

class TC_GAME_API InterruptDatabase
{
public:
    // Initialize the database with WoW 12.0 spell data
    static void Initialize();

    // Query methods
    static SpellInterruptConfig const* GetSpellConfig(uint32 spellId);
    static InterruptPriority GetSpellPriority(uint32 spellId, uint8 mythicLevel = 0);
    static bool ShouldAlwaysInterrupt(uint32 spellId);
    static bool IsQuickResponseRequired(uint32 spellId);
    static ::std::vector<uint32> GetCriticalSpells();
    static ::std::vector<uint32> GetSpellsByCategory(SpellCategory category);

    // Dungeon/Raid specific configurations
    static void LoadDungeonOverrides(uint32 mapId);
    static void LoadRaidOverrides(uint32 encounterId);
    static void LoadMythicPlusAffixOverrides(uint32 affixId);

private:
    static void LoadGeneralSpells();
    static void LoadDungeonSpells();
    static void LoadRaidSpells();
    static void LoadPvPSpells();
    static void LoadClassSpells();

    static ::std::unordered_map<uint32, SpellInterruptConfig> _spellDatabase;
    static ::std::unordered_map<uint32, ::std::unordered_map<uint32, InterruptPriority>> _dungeonOverrides;
    static bool _initialized;
};

// WoW 12.0 Class Interrupt Abilities
struct ClassInterruptAbility
{
    uint32 spellId;
    ::std::string name;
    uint8 playerClass;
    uint32 specialization;      // 0 = all specs
    InterruptMethod method;
    float range;
    float cooldown;
    uint32 lockoutDuration;
    uint32 schoolMask;
    bool isHeroTalent;
    uint32 heroTalentId;        // Hero talent tree ID
    Powers resourceType;
    uint32 resourceCost;
    bool offGCD;
    uint8 charges;

    ClassInterruptAbility(uint32 id, ::std::string n, uint8 cls, uint32 spec,
                         InterruptMethod m, float r, float cd, uint32 lockout, uint32 school,
                         bool hero, uint32 heroId, Powers resType, uint32 resCost, bool gcd, uint8 chr)
        : spellId(id), name(n), playerClass(cls), specialization(spec), method(m),
          range(r), cooldown(cd), lockoutDuration(lockout), schoolMask(school),
          isHeroTalent(hero), heroTalentId(heroId), resourceType(resType),
          resourceCost(resCost), offGCD(gcd), charges(chr) {}
};

// WoW 12.0 Interrupt ability database
namespace InterruptAbilities
{
    // Death Knight
    static constexpr uint32 MIND_FREEZE = 47528;
    static constexpr uint32 STRANGULATE = 47476;
    static constexpr uint32 ASPHYXIATE = 221562;
    static constexpr uint32 DEATH_GRIP = 49576;

    // Demon Hunter
    static constexpr uint32 DISRUPT = 183752;
    static constexpr uint32 CHAOS_NOVA = 179057;
    static constexpr uint32 FEL_ERUPTION = 211881;
    static constexpr uint32 SIGIL_OF_SILENCE = 202137;

    // Druid
    static constexpr uint32 SKULL_BASH = 106839;
    static constexpr uint32 SOLAR_BEAM = 78675;
    static constexpr uint32 TYPHOON = 132469;
    static constexpr uint32 INCAPACITATING_ROAR = 99;

    // Evoker
    static constexpr uint32 QUELL = 351338;
    static constexpr uint32 TAIL_SWIPE = 368970;
    static constexpr uint32 WING_BUFFET = 357210;
    static constexpr uint32 OPPRESSING_ROAR = 372048;

    // Hunter
    static constexpr uint32 COUNTER_SHOT = 147362;
    static constexpr uint32 MUZZLE = 187707;
    static constexpr uint32 FREEZING_TRAP = 187650;
    static constexpr uint32 INTIMIDATION = 19577;

    // Mage
    static constexpr uint32 COUNTERSPELL = 2139;
    static constexpr uint32 SPELLSTEAL = 30449;
    static constexpr uint32 DRAGONS_BREATH = 31661;
    static constexpr uint32 RING_OF_FROST = 113724;

    // Monk
    static constexpr uint32 SPEAR_HAND_STRIKE = 116705;
    static constexpr uint32 PARALYSIS = 115078;
    static constexpr uint32 LEG_SWEEP = 119381;
    static constexpr uint32 RING_OF_PEACE = 116844;

    // Paladin
    static constexpr uint32 REBUKE = 96231;
    static constexpr uint32 HAMMER_OF_JUSTICE = 853;
    static constexpr uint32 BLINDING_LIGHT = 115750;
    static constexpr uint32 AVENGERS_SHIELD = 31935; // Protection only

    // Priest
    static constexpr uint32 SILENCE = 15487; // Shadow only
    static constexpr uint32 PSYCHIC_SCREAM = 8122;
    static constexpr uint32 PSYCHIC_HORROR = 64044;
    static constexpr uint32 MIND_BOMB = 205369;

    // Rogue
    static constexpr uint32 KICK = 1766;
    static constexpr uint32 CHEAP_SHOT = 1833;
    static constexpr uint32 KIDNEY_SHOT = 408;
    static constexpr uint32 BLIND = 2094;

    // Shaman
    static constexpr uint32 WIND_SHEAR = 57994;
    static constexpr uint32 CAPACITOR_TOTEM = 192058;
    static constexpr uint32 THUNDERSTORM = 51490;
    static constexpr uint32 SUNDERING = 197214; // Enhancement

    // Warlock
    static constexpr uint32 SPELL_LOCK = 19647; // Felhunter
    static constexpr uint32 SHADOW_FURY = 30283;
    static constexpr uint32 MORTAL_COIL = 6789;
    static constexpr uint32 HOWL_OF_TERROR = 5484;

    // Warrior
    static constexpr uint32 PUMMEL = 6552;
    static constexpr uint32 STORM_BOLT = 107570;
    static constexpr uint32 SHOCKWAVE = 46968;
    static constexpr uint32 INTIMIDATING_SHOUT = 5246;
    static constexpr uint32 DISRUPTING_SHOUT = 386071; // Mountain Thane

    // Get all interrupt abilities for a class/spec
    ::std::vector<ClassInterruptAbility> GetClassInterrupts(uint8 playerClass, uint32 spec = 0);
    ClassInterruptAbility const* GetAbilityInfo(uint32 spellId);
    ::std::vector<uint32> GetAvailableInterrupts(Player* player);
    float GetOptimalRange(uint8 playerClass);
    uint32 GetSchoolLockoutDuration(uint32 spellId);
}

// WoW 12.0 Critical Spells Database (must interrupt)
namespace CriticalSpells
{
    // Dungeon spells
    namespace Dungeons
    {
        // The Stonevault
        static constexpr uint32 VOID_DISCHARGE = 428269;
        static constexpr uint32 SEISMIC_WAVE = 428703;
        static constexpr uint32 MOLTEN_MORTAR = 428120;

        // City of Threads
        static constexpr uint32 UMBRAL_WEAVE = 439341;
        static constexpr uint32 DARK_BARRAGE = 439401;
        static constexpr uint32 SHADOWY_DECAY = 439419;

        // Ara-Kara, City of Echoes
        static constexpr uint32 ECHOING_HOWL = 438471;
        static constexpr uint32 WEB_WRAP = 438473;
        static constexpr uint32 POISON_BOLT = 438343;

        // The Dawnbreaker
        static constexpr uint32 SHADOW_SHROUD = 426734;
        static constexpr uint32 ABYSSAL_BLAST = 426736;
        static constexpr uint32 DARK_ORB = 426865;

        // Cinderbrew Meadery
        static constexpr uint32 HONEY_MARINADE = 439365;
        static constexpr uint32 CINDERBREW_TOSS = 440134;

        // Darkflame Cleft
        static constexpr uint32 SHADOW_VOLLEY = 428086;
        static constexpr uint32 DARK_EMPOWERMENT = 428089;

        // The Rookery
        static constexpr uint32 TEMPEST = 427285;
        static constexpr uint32 LIGHTNING_TORRENT = 427291;

        // Priory of the Sacred Flame
        static constexpr uint32 HOLY_SMITE = 424431;
        static constexpr uint32 INNER_FLAME = 424419;
    }

    // Raid spells
    namespace Raids
    {
        // Nerub-ar Palace
        static constexpr uint32 VENOMOUS_RAIN = 438200;
        static constexpr uint32 WEB_TERROR = 437700;
        static constexpr uint32 SILKEN_TOMB = 438656;
        static constexpr uint32 VOID_DEGENERATION = 440001;

        // Queen Ansurek specific
        static constexpr uint32 REACTIVE_TOXIN = 437592;
        static constexpr uint32 VENOM_NOVA = 437586;
        static constexpr uint32 FEAST = 444829;
        static constexpr uint32 ABYSSAL_INFUSION = 443903;
    }

    // Mythic+ Affix related
    namespace Affixes
    {
        static constexpr uint32 INCORPOREAL_CAST = 408556;
        static constexpr uint32 AFFLICTED_CRY = 409465;
        static constexpr uint32 SPITEFUL_FIXATE = 350163;
    }

    // PvP critical casts
    namespace PvP
    {
        static constexpr uint32 GREATER_HEAL = 48782;
        static constexpr uint32 CHAOS_BOLT = 116858;
        static constexpr uint32 GREATER_PYROBLAST = 203286;
        static constexpr uint32 CONVOKE_SPIRITS = 391528;
        static constexpr uint32 DIVINE_HYMN = 64843;
    }
}

// WoW 12.0 Mythic+ scaling configurations
struct MythicPlusConfig
{
    uint8 level;
    float interruptWindowReduction;  // How much faster we need to interrupt
    float priorityModifier;          // Priority adjustment for spells
    bool requiresRotation;           // Needs coordinated rotation
    uint32 minInterruptersRequired;  // Minimum interrupters needed
};

class TC_GAME_API MythicPlusInterruptScaling
{
public:
    static MythicPlusConfig const* GetConfig(uint8 level);
    static float GetReactionTimeModifier(uint8 level);
    static InterruptPriority AdjustPriorityForLevel(InterruptPriority base, uint8 level);
    static bool RequiresCoordinatedInterrupts(uint8 level);
    static uint32 GetRequiredInterrupters(uint8 level);

private:
    static ::std::unordered_map<uint8, MythicPlusConfig> _configs;
};

// Interrupt rotation templates for different group compositions
struct RotationTemplate
{
    ::std::string name;
    ::std::vector<uint8> requiredClasses;
    ::std::vector<::std::pair<uint32, uint32>> rotationPairs;  // SpellID -> AssignedClass
    uint32 rotationInterval;
    bool useBackupSystem;
};

namespace RotationTemplates
{
    // Standard 5-man templates
    extern RotationTemplate const MELEE_HEAVY;      // 3+ melee
    extern RotationTemplate const RANGED_HEAVY;     // 3+ ranged
    extern RotationTemplate const BALANCED;         // Mixed comp
    extern RotationTemplate const DOUBLE_INTERRUPT; // 2 dedicated interrupters
    extern RotationTemplate const CASTER_GROUP;     // All casters

    // Get optimal template for group
    RotationTemplate const* GetOptimalTemplate(::std::vector<Player*> const& group);
    void CustomizeForEncounter(RotationTemplate& templ, uint32 encounterId);
}

// Performance tracking for interrupt optimization
struct InterruptPerformanceData
{
    uint32 spellId;
    uint32 successCount;
    uint32 failCount;
    float averageReactionTime;
    float successRate;
    uint32 lastUpdated;

    float GetEfficiency() const
    {
        return successCount > 0 ? successRate * (1.0f / averageReactionTime) : 0.0f;
    }
};

class TC_GAME_API InterruptOptimizer
{
public:
    // Learning system for improving interrupt timing
    static void RecordInterruptAttempt(uint32 spellId, bool success, float reactionTime);
    static float GetOptimalTiming(uint32 spellId);
    static float GetPredictedCastTime(uint32 spellId, Unit* caster);

    // Optimization algorithms
    static void OptimizeRotationForGroup(::std::vector<Player*> const& group);
    static void AdjustPrioritiesForPerformance();
    static void GenerateInterruptReport(Player* requester);

private:
    static ::std::unordered_map<uint32, InterruptPerformanceData> _performanceData;
    static constexpr uint32 MIN_SAMPLES = 10;  // Minimum attempts before optimization
};

} // namespace Playerbot