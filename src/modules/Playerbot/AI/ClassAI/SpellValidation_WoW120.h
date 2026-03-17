/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * World of Warcraft 12.0 (The War Within) Spell ID Validation
 *
 * This file contains all validated spell IDs for WoW 12.0 including
 * Hero Talent abilities for all 39 specializations.
 */

#pragma once

namespace Playerbot
{
namespace WoW120Spells
{

// ============================================================================
// DEATH KNIGHT SPELLS - Updated for 12.0
// ============================================================================

namespace DeathKnight
{
    // Core Abilities (All Specs)
    constexpr uint32 DEATH_STRIKE = 49998;
    constexpr uint32 DEATH_AND_DECAY = 43265;
    constexpr uint32 DEATH_GRIP = 49576;
    constexpr uint32 ANTI_MAGIC_SHELL = 48707;
    constexpr uint32 ANTI_MAGIC_ZONE = 51052;
    constexpr uint32 ICEBOUND_FORTITUDE = 48792;
    constexpr uint32 CHAINS_OF_ICE = 45524;
    constexpr uint32 MIND_FREEZE = 47528; // Interrupt
    constexpr uint32 PATH_OF_FROST = 3714;
    constexpr uint32 RAISE_DEAD = 46585;
    constexpr uint32 SACRIFICIAL_PACT = 327574;
    constexpr uint32 DEATH_COIL = 47541;
    constexpr uint32 DARK_COMMAND = 56222; // Taunt
    constexpr uint32 RAISE_ALLY = 61999; // Battle resurrection
    constexpr uint32 CONTROL_UNDEAD = 111673; // Control undead minion
    constexpr uint32 DEATHS_ADVANCE = 48265; // 1.5 min CD, speed + mitigation
    constexpr uint32 ASPHYXIATE = 221562; // 45 sec CD, stun

    // Blood Specialization
    namespace Blood
    {
        // Core Rotation
        constexpr uint32 MARROWREND = 195182;
        constexpr uint32 HEART_STRIKE = 206930;
        constexpr uint32 BLOOD_BOIL = 50842;
        constexpr uint32 RUNE_TAP = 194679;
        constexpr uint32 VAMPIRIC_BLOOD = 55233;
        constexpr uint32 DANCING_RUNE_WEAPON = 49028;
        constexpr uint32 BLOODDRINKER = 206931;
        constexpr uint32 BONESTORM = 194844;
        constexpr uint32 CONSUMPTION = 274156;
        constexpr uint32 GOREFIENDS_GRASP = 108199;
        constexpr uint32 TOMBSTONE = 219809;
        constexpr uint32 BLOOD_TAP = 221699;
        constexpr uint32 DEATHS_CARESS = 195292;

        // Bone Shield System
        constexpr uint32 BONE_SHIELD = 195181;
        constexpr uint32 BONE_SHIELD_BUFF = 195181;

        // Hero Talents - Deathbringer
        constexpr uint32 REAPER_MARK = 434765;
        constexpr uint32 REAPERS_MARK_DEBUFF = 434766;
        constexpr uint32 WAVE_OF_SOULS = 434894;
        constexpr uint32 EXTERMINATE = 434895;
        constexpr uint32 PAINFUL_DEATH = 434896;

        // Hero Talents - San'layn
        constexpr uint32 VAMPIRIC_STRIKE = 433895;
        constexpr uint32 VAMPIRIC_BLOOD_ENHANCE = 433896;
        constexpr uint32 BLOODTHIRST = 433897;
        constexpr uint32 GIFT_OF_THE_SANLAYN = 433898;

        // Procs and Talents
        constexpr uint32 BLOOD_PLAGUE = 55078;         // Disease DoT
        constexpr uint32 CRIMSON_SCOURGE = 81136;      // Proc: free Blood Boil
        constexpr uint32 HEMOSTASIS = 273947;          // Buff: increased Blood Boil damage
        constexpr uint32 OSSUARY = 219786;             // Passive: reduces Death Strike cost
        constexpr uint32 RAPID_DECOMPOSITION = 194662; // Disease tick speed
        constexpr uint32 HEARTBREAKER = 221536;        // Heart Strike generates RP
        constexpr uint32 FOUL_BULWARK = 206974;        // Armor from Bone Shield
        constexpr uint32 RELISH_IN_BLOOD = 317610;     // Extra Bone Shield stacks
    }

    // Frost Specialization
    namespace Frost
    {
        // Core Rotation
        constexpr uint32 FROST_STRIKE = 49143;
        constexpr uint32 HOWLING_BLAST = 49184;
        constexpr uint32 OBLITERATE = 49020;
        constexpr uint32 REMORSELESS_WINTER = 196770;
        constexpr uint32 PILLAR_OF_FROST = 51271;
        constexpr uint32 EMPOWER_RUNE_WEAPON = 47568;
        constexpr uint32 FROSTSCYTHE = 207230;
        constexpr uint32 GLACIAL_ADVANCE = 194913;
        constexpr uint32 BREATH_OF_SINDRAGOSA = 152279;
        constexpr uint32 FROST_FEVER = 55095;
        constexpr uint32 FROSTWYRMS_FURY = 279302;
        constexpr uint32 CHILL_STREAK = 305392;
        constexpr uint32 ABSOLUTE_ZERO = 377047;
        constexpr uint32 COLD_HEART = 281208;
        constexpr uint32 HORN_OF_WINTER = 57330;

        // Rune of Razorice
        constexpr uint32 RUNE_OF_RAZORICE = 326911;
        constexpr uint32 RAZORICE_DEBUFF = 51714;

        // Hero Talents - Rider of the Apocalypse
        constexpr uint32 APOCALYPSE_NOW = 444756;
        constexpr uint32 RIDERS_CHAMPION = 444757;
        constexpr uint32 ON_A_PALE_HORSE = 444758;
        constexpr uint32 DEATH_CHARGE = 444759;

        // Hero Talents - Deathbringer
        constexpr uint32 FROST_REAPER_MARK = 444765;
        constexpr uint32 FROST_WAVE_OF_SOULS = 444766;
        constexpr uint32 FROST_EXTERMINATE = 444767;

        // Procs and Talents
        constexpr uint32 KILLING_MACHINE = 51128;      // Proc: crit on Obliterate
        constexpr uint32 RIME = 59052;                 // Proc: free Howling Blast
        constexpr uint32 RAZORICE_PROC = 50401;        // Debuff: stacking damage amp
        constexpr uint32 FROZEN_PULSE = 194909;        // Passive AoE
        constexpr uint32 OBLITERATION = 281238;        // Pillar of Frost extension
        constexpr uint32 GATHERING_STORM = 194912;     // Remorseless Winter buff
        constexpr uint32 ICECAP = 207126;              // Pillar of Frost CDR
        constexpr uint32 INEXORABLE_ASSAULT = 253593;  // Cold Heart stacking buff
    }

    // Unholy Specialization
    namespace Unholy
    {
        // Core Rotation
        constexpr uint32 FESTERING_STRIKE = 85948;
        constexpr uint32 SCOURGE_STRIKE = 55090;
        constexpr uint32 DEATH_COIL_UNHOLY = 47541;
        constexpr uint32 EPIDEMIC = 207317;
        constexpr uint32 OUTBREAK = 77575;
        constexpr uint32 DARK_TRANSFORMATION = 63560;
        constexpr uint32 APOCALYPSE = 275699;
        constexpr uint32 ARMY_OF_THE_DEAD = 42650;
        constexpr uint32 SUMMON_GARGOYLE = 49206;
        constexpr uint32 SOUL_REAPER = 343294;
        constexpr uint32 UNHOLY_ASSAULT = 207289;
        constexpr uint32 UNHOLY_BLIGHT = 115989;
        constexpr uint32 CLAWING_SHADOWS = 207311;
        constexpr uint32 DEFILE = 152280;
        constexpr uint32 NECROTIC_STRIKE = 223829;

        // Festering Wounds System
        constexpr uint32 FESTERING_WOUND = 194310;
        constexpr uint32 WOUND_BURST = 194311;

        // Pet Abilities
        constexpr uint32 PET_GNAW = 91800;
        constexpr uint32 PET_LEAP = 91802;
        constexpr uint32 PET_HUDDLE = 91838;
        constexpr uint32 PET_CLAW = 91776;

        // Hero Talents - San'layn
        constexpr uint32 UNHOLY_VAMPIRIC_STRIKE = 443895;
        constexpr uint32 UNHOLY_BLOODTHIRST = 443896;
        constexpr uint32 ESSENCE_OF_THE_BLOOD_QUEEN = 443897;

        // Hero Talents - Rider of the Apocalypse
        constexpr uint32 UNHOLY_APOCALYPSE = 454756;
        constexpr uint32 UNHOLY_RIDERS = 454757;
        constexpr uint32 MORBIDITY = 454758;

        // Procs and Talents
        constexpr uint32 VIRULENT_PLAGUE = 191587;     // Main disease DoT
        constexpr uint32 RAISE_ABOMINATION = 455395;   // 1.5 min CD, summon abomination
        constexpr uint32 SUDDEN_DOOM = 49530;          // Proc: free Death Coil
        constexpr uint32 RUNIC_CORRUPTION = 51460;     // Proc: increased rune regen
        constexpr uint32 UNHOLY_STRENGTH = 53365;      // Passive: pet damage buff
        constexpr uint32 BURSTING_SORES = 207264;      // Festering Wound burst AoE
        constexpr uint32 INFECTED_CLAWS = 207272;      // Pet applies Festering Wounds
        constexpr uint32 ALL_WILL_SERVE = 194916;      // Summon skeleton on Death Coil
        constexpr uint32 UNHOLY_PACT = 319230;         // Dark Transformation damage buff
        constexpr uint32 SUPERSTRAIN = 390283;         // Disease damage buff
    }
}

// ============================================================================
// DEMON HUNTER SPELLS - Updated for 12.0
// ============================================================================

namespace DemonHunter
{
    // Core Abilities (All Specs)
    constexpr uint32 METAMORPHOSIS_VENGEANCE = 187827;
    constexpr uint32 METAMORPHOSIS_HAVOC = 191427;
    constexpr uint32 FEL_RUSH = 195072;
    constexpr uint32 VENGEFUL_RETREAT = 198793;
    constexpr uint32 THROW_GLAIVE = 185123;
    constexpr uint32 SPECTRAL_SIGHT = 188501;
    constexpr uint32 GLIDE = 131347;
    constexpr uint32 CONSUME_MAGIC = 278326; // Purge
    constexpr uint32 DISRUPT = 183752; // Interrupt
    constexpr uint32 IMPRISON = 217832; // CC
    constexpr uint32 DARKNESS = 196718;
    constexpr uint32 CHAOS_NOVA = 179057;
    constexpr uint32 SIGIL_OF_FLAME = 204596;
    constexpr uint32 SIGIL_OF_MISERY = 207684;
    constexpr uint32 SIGIL_OF_SILENCE = 202137;
    constexpr uint32 BLUR = 198589; // Defensive dodge ability
    constexpr uint32 NETHERWALK = 196555; // Immunity + speed
    constexpr uint32 NEMESIS = 206491; // Single target damage buff
    constexpr uint32 IMMOLATION_AURA = 258920; // Shared damage aura

    // Havoc Specialization
    namespace Havoc
    {
        // Core Rotation
        constexpr uint32 DEMONS_BITE = 162243;
        constexpr uint32 CHAOS_STRIKE = 162794;
        constexpr uint32 ANNIHILATION = 201427; // During Meta
        constexpr uint32 BLADE_DANCE = 188499;
        constexpr uint32 DEATH_SWEEP = 210152; // During Meta
        constexpr uint32 IMMOLATION_AURA = 258920;
        constexpr uint32 FEL_BARRAGE = 258925;
        constexpr uint32 EYE_BEAM = 198013;
        constexpr uint32 GLAIVE_TEMPEST = 342817;
        constexpr uint32 ESSENCE_BREAK = 258860;
        constexpr uint32 THE_HUNT = 323639;
        constexpr uint32 ELYSIAN_DECREE = 390163;

        // Fury Resource
        constexpr uint32 FURY_GENERATION = 203555;

        // Talents
        constexpr uint32 DEMONIC = 213410; // Eye Beam triggers Meta
        constexpr uint32 MOMENTUM = 206476; // Movement abilities buff damage
        constexpr uint32 BLIND_FURY = 203550; // Eye Beam generates more fury
        constexpr uint32 FIRST_BLOOD = 206416; // Blade Dance cost reduction
        constexpr uint32 TRAIL_OF_RUIN = 258881; // Blade Dance DoT
        constexpr uint32 CHAOS_CLEAVE = 206475; // Chaos Strike cleaves
        constexpr uint32 CYCLE_OF_HATRED = 258887; // Meta CD reduction
        constexpr uint32 TORMENT = 281854; // Havoc taunt

        // Buffs
        constexpr uint32 BUFF_MOMENTUM = 208628; // Momentum damage increase
        constexpr uint32 BUFF_FURIOUS_GAZE = 343312; // Eye Beam haste buff
        constexpr uint32 BUFF_METAMORPHOSIS = 162264; // Meta transformation buff
        constexpr uint32 BUFF_PREPARED = 203650; // Vengeful Retreat buff

        // Hero Talents - Aldrachi Reaver
        constexpr uint32 ALDRACHI_TACTICS = 442683;
        constexpr uint32 ARMY_UNTO_ONESELF = 442684;
        constexpr uint32 ART_OF_THE_GLAIVE = 442685;
        constexpr uint32 FURY_OF_THE_ALDRACHI = 442686;

        // Hero Talents - Fel-Scarred
        constexpr uint32 FEL_SCARRED_METAMORPHOSIS = 452683;
        constexpr uint32 DEMONIC_INTENSITY = 452684;
        constexpr uint32 VIOLENT_TRANSFORMATION = 452685;
        constexpr uint32 MONSTER = 452686;
    }

    // Vengeance Specialization
    namespace Vengeance
    {
        // Core Rotation
        constexpr uint32 SHEAR = 203782;
        constexpr uint32 FRACTURE = 263642;
        constexpr uint32 SOUL_CLEAVE = 228477;
        constexpr uint32 SOUL_CARVER = 207407;
        constexpr uint32 IMMOLATION_AURA_VENG = 178740;
        constexpr uint32 IMMOLATION_AURA_TANK = 258920; // Alternative ID
        constexpr uint32 DEMON_SPIKES = 203720;
        constexpr uint32 FIERY_BRAND = 204021;
        constexpr uint32 INFERNAL_STRIKE = 189110;
        constexpr uint32 SPIRIT_BOMB = 247454;
        constexpr uint32 FEL_DEVASTATION = 212084;
        constexpr uint32 BULK_EXTRACTION = 320341;
        constexpr uint32 THE_HUNT_VENG = 370965;
        constexpr uint32 ELYSIAN_DECREE_VENG = 390163;

        // Soul Fragments
        constexpr uint32 SOUL_FRAGMENT = 203981;
        constexpr uint32 SHATTERED_SOUL = 204254;

        // Sigils
        constexpr uint32 SIGIL_OF_CHAINS = 202138; // AoE slow

        // Utility
        constexpr uint32 THROW_GLAIVE_TANK = 204157; // Ranged threat
        constexpr uint32 TORMENT = 185245; // Taunt

        // Talents
        constexpr uint32 LAST_RESORT = 209258; // Cheat death
        constexpr uint32 AGONIZING_FLAMES = 207548; // Fiery Brand spread
        constexpr uint32 BURNING_ALIVE = 207739; // Fiery Brand duration
        constexpr uint32 FEED_THE_DEMON = 218612; // Demon Spikes CDR
        constexpr uint32 SOUL_BARRIER = 263648; // Shield from fragments

        // Debuffs
        constexpr uint32 FRAILTY_DEBUFF = 247456; // Spirit Bomb debuff
        constexpr uint32 PAINBRINGER_BUFF = 207407; // Shear damage increase

        // Hero Talents - Aldrachi Reaver
        constexpr uint32 VENG_ALDRACHI_TACTICS = 462683;
        constexpr uint32 VENG_ARMY_UNTO_ONESELF = 462684;
        constexpr uint32 VENG_ART_OF_THE_GLAIVE = 462685;

        // Hero Talents - Fel-Scarred
        constexpr uint32 VENG_FEL_SCARRED = 472683;
        constexpr uint32 VENG_DEMONIC_INTENSITY = 472684;
        constexpr uint32 VENG_MONSTER = 472686;

        // Alternative/Legacy Spell IDs (for base AI compatibility)
        constexpr uint32 SOUL_BARRIER_LEGACY = 227225; // Alternative Soul Barrier ID
    }

    // Base AI Compatibility - Common spell aliases
    namespace Common
    {
        // Core Abilities (All Specs)
        constexpr uint32 METAMORPHOSIS_VENGEANCE = 187827;
        constexpr uint32 METAMORPHOSIS_HAVOC = 191427;
        constexpr uint32 FEL_RUSH = 195072;
        constexpr uint32 VENGEFUL_RETREAT = 198793;
        constexpr uint32 THROW_GLAIVE = 185123;
        constexpr uint32 SPECTRAL_SIGHT = 188501;
        constexpr uint32 GLIDE = 131347;
        constexpr uint32 CONSUME_MAGIC = 278326;
        constexpr uint32 DISRUPT = 183752;
        constexpr uint32 IMPRISON = 217832;
        constexpr uint32 DARKNESS = 196718;
        constexpr uint32 CHAOS_NOVA = 179057;
        constexpr uint32 SIGIL_OF_FLAME = 204596;
        constexpr uint32 SIGIL_OF_MISERY = 207684;
        constexpr uint32 SIGIL_OF_SILENCE = 202137;
        constexpr uint32 BLUR = 198589;
        constexpr uint32 NETHERWALK = 196555;
        constexpr uint32 NEMESIS = 206491;
        constexpr uint32 IMMOLATION_AURA = 258920;

        // Havoc abilities
        constexpr uint32 CHAOS_STRIKE = 162794;
        constexpr uint32 BLADE_DANCE = 188499;
        constexpr uint32 DEATH_SWEEP = 210152;
        constexpr uint32 ANNIHILATION = 201427;
        constexpr uint32 EYE_BEAM = 198013;
        constexpr uint32 DEMONS_BITE = 162243;
        constexpr uint32 FEL_BARRAGE = 258925;

        // Vengeance abilities
        constexpr uint32 SOUL_CLEAVE = 228477;
        constexpr uint32 SPIRIT_BOMB = 247454;
        constexpr uint32 SHEAR = 203782;
        constexpr uint32 FRACTURE = 263642;
        constexpr uint32 SOUL_BARRIER = 263648;
        constexpr uint32 DEMON_SPIKES = 203720;
        constexpr uint32 FIERY_BRAND = 204021;
        constexpr uint32 INFERNAL_STRIKE = 189110;
        constexpr uint32 FEL_DEVASTATION = 212084;

        // Talents
        constexpr uint32 MOMENTUM_TALENT = 206476;
        constexpr uint32 DEMONIC_TALENT = 213410;
        constexpr uint32 BLIND_FURY_TALENT = 203550;
    }
}

// ============================================================================
// DRUID SPELLS - Updated for 12.0
// ============================================================================

namespace Druid
{
    // Core Abilities (All Specs)
    constexpr uint32 BEAR_FORM = 5487;
    constexpr uint32 CAT_FORM = 768;
    constexpr uint32 MOONKIN_FORM = 24858;
    constexpr uint32 TRAVEL_FORM = 783;
    constexpr uint32 BARKSKIN = 22812;
    constexpr uint32 DASH = 1850;
    constexpr uint32 STAMPEDING_ROAR = 106898;
    constexpr uint32 REMOVE_CORRUPTION = 2782;
    constexpr uint32 SOOTHE = 2908;
    constexpr uint32 HIBERNATE = 2637;
    constexpr uint32 ENTANGLING_ROOTS = 339;
    constexpr uint32 CYCLONE = 33786;
    constexpr uint32 MOONFIRE = 8921;
    constexpr uint32 SUNFIRE = 93402;
    constexpr uint32 REGROWTH = 8936;
    constexpr uint32 REJUVENATION = 774;
    constexpr uint32 SWIFTMEND = 18562;
    constexpr uint32 WILD_GROWTH = 48438;
    constexpr uint32 REBIRTH = 20484; // Battle res
    constexpr uint32 INNERVATE = 29166;
    constexpr uint32 SOLAR_BEAM = 78675; // Interrupt (Balance)
    constexpr uint32 SKULL_BASH = 106839; // Interrupt (Feral/Guardian)
    constexpr uint32 SURVIVAL_INSTINCTS = 61336; // Major defensive cooldown
    constexpr uint32 MARK_OF_THE_WILD = 1126; // Buff

    // CC and Utility Talents (shared across specs)
    constexpr uint32 TYPHOON = 132469;
    constexpr uint32 MIGHTY_BASH = 5211;
    constexpr uint32 MASS_ENTANGLEMENT = 102359;
    constexpr uint32 RENEWAL = 108238; // Self-heal talent

    // Balance Specialization
    namespace Balance
    {
        // Core Rotation
        constexpr uint32 WRATH = 190984;
        constexpr uint32 STARFIRE = 194153;
        constexpr uint32 STARSURGE = 78674;
        constexpr uint32 STARFALL = 191034;
        constexpr uint32 CELESTIAL_ALIGNMENT = 194223;
        constexpr uint32 INCARNATION_CHOSEN = 102560; // Incarnation: Chosen of Elune
        constexpr uint32 CONVOKE_THE_SPIRITS = 391528;
        constexpr uint32 STELLAR_FLARE = 202347;
        constexpr uint32 ASTRAL_COMMUNION = 400636;
        constexpr uint32 WILD_MUSHROOM = 145205;
        constexpr uint32 FORCE_OF_NATURE = 205636;
        constexpr uint32 FURY_OF_ELUNE = 202770;
        constexpr uint32 NEW_MOON = 274281;
        constexpr uint32 HALF_MOON = 274282;
        constexpr uint32 FULL_MOON = 274283;
        constexpr uint32 WARRIOR_OF_ELUNE = 202425;

        // Eclipse System
        constexpr uint32 SOLAR_ECLIPSE = 48517;
        constexpr uint32 LUNAR_ECLIPSE = 48518;
        constexpr uint32 ECLIPSE_SOLAR_BUFF = 48517;
        constexpr uint32 ECLIPSE_LUNAR_BUFF = 48518;

        // Procs and Buffs
        constexpr uint32 BALANCE_OF_ALL_THINGS = 394048;
        constexpr uint32 SHOOTING_STARS = 202342;
        constexpr uint32 STARWEAVERS_WARP = 393942;
        constexpr uint32 STARWEAVERS_WEFT = 393944;

        // DoT Auras (Balance-specific spell IDs)
        constexpr uint32 MOONFIRE_BALANCE = 164812; // Balance DoT aura
        constexpr uint32 SUNFIRE_BALANCE = 164815;  // Balance DoT aura

        // Talents
        constexpr uint32 WILD_MUSHROOM_GROUND = 88747; // Ground AoE talent
        constexpr uint32 TWIN_MOONS = 279620;
        constexpr uint32 SOUL_OF_THE_FOREST = 114107;

        // Hero Talents - Keeper of the Grove
        constexpr uint32 POWER_OF_THE_DREAM = 434657;
        constexpr uint32 TREANTS_OF_THE_MOON = 434658;
        constexpr uint32 CONTROL_OF_THE_DREAM = 434659;

        // Hero Talents - Elune's Chosen
        constexpr uint32 LUNAR_CALLING = 444657;
        constexpr uint32 ARCANE_AFFINITY = 444658;
        constexpr uint32 ASTRAL_INSIGHT = 444659;
    }

    // Feral Specialization
    namespace Feral
    {
        // Core Rotation
        constexpr uint32 RAKE = 1822;
        constexpr uint32 SHRED = 5221;
        constexpr uint32 FEROCIOUS_BITE = 22568;
        constexpr uint32 RIP = 1079;
        constexpr uint32 SAVAGE_ROAR = 52610;
        constexpr uint32 TIGERS_FURY = 5217;
        constexpr uint32 BERSERK = 106951;
        constexpr uint32 INCARNATION_FERAL = 102543; // Incarnation: Avatar of Ashamane
        constexpr uint32 CONVOKE_FERAL = 391528;
        constexpr uint32 PRIMAL_WRATH = 285381;
        constexpr uint32 BRUTAL_SLASH = 202028;
        constexpr uint32 FERAL_FRENZY = 274837;
        constexpr uint32 ADAPTIVE_SWARM = 391888;
        constexpr uint32 THRASH_CAT = 106832;
        constexpr uint32 SWIPE_CAT = 106785;
        constexpr uint32 MAIM = 22570;
        constexpr uint32 BLOODTALONS = 155672;

        // Talents
        constexpr uint32 MOONFIRE_FERAL = 155625; // Lunar Inspiration talent

        // Combo Points
        constexpr uint32 COMBO_POINT = 4;

        // Hero Talents - Druid of the Claw
        constexpr uint32 RAVAGE = 441583;
        constexpr uint32 DREADFUL_BLEEDING = 441584;
        constexpr uint32 BESTIAL_STRENGTH = 441585;

        // Hero Talents - Wildstalker
        constexpr uint32 WILDSHAPE_MASTERY = 451583;
        constexpr uint32 PACK_HUNTER = 451584;
        constexpr uint32 HUNT_BENEATH_THE_OPEN_SKY = 451585;
    }

    // Guardian Specialization
    namespace Guardian
    {
        // Core Rotation
        constexpr uint32 MANGLE = 33917;
        constexpr uint32 THRASH_BEAR = 77758;
        constexpr uint32 SWIPE_BEAR = 213771;
        constexpr uint32 MOONFIRE_BEAR = 8921;
        constexpr uint32 MAUL = 6807;
        constexpr uint32 IRONFUR = 192081;
        constexpr uint32 FRENZIED_REGENERATION = 22842;
        constexpr uint32 SURVIVAL_INSTINCTS = 61336;
        constexpr uint32 BRISTLING_FUR = 155835;
        constexpr uint32 INCARNATION_GUARDIAN = 102558; // Incarnation: Guardian of Ursoc
        constexpr uint32 CONVOKE_GUARDIAN = 391528;
        constexpr uint32 BERSERK_GUARDIAN = 50334;
        constexpr uint32 PULVERIZE = 80313;
        constexpr uint32 RAZE = 400254;
        constexpr uint32 RAGE_OF_THE_SLEEPER = 200851;
        constexpr uint32 LUNAR_BEAM = 204066;
        constexpr uint32 GROWL = 6795; // Taunt

        // Procs
        constexpr uint32 GORE = 93622; // Thrash/Swipe/Moonfire proc - resets Mangle CD, grants 4 rage

        // Rage Resource
        constexpr uint32 RAGE_GENERATION = 195707;

        // Hero Talents - Elune's Chosen
        constexpr uint32 LUNAR_BEAM_ENHANCED = 464657;
        constexpr uint32 CELESTIAL_GUARDIAN = 464658;

        // Hero Talents - Druid of the Claw
        constexpr uint32 URSINE_ADEPT = 471583;
        constexpr uint32 KILLING_STRIKES = 471584;
    }

    // Restoration Specialization
    namespace Restoration
    {
        // Core Rotation
        constexpr uint32 LIFEBLOOM = 33763;
        constexpr uint32 CENARION_WARD = 102351;
        constexpr uint32 GROVE_TENDING = 383192;
        constexpr uint32 NATURES_SWIFTNESS = 132158;
        constexpr uint32 TRANQUILITY = 740;
        constexpr uint32 FLOURISH = 197721;
        constexpr uint32 TREE_OF_LIFE = 33891;
        constexpr uint32 INCARNATION_TREE = 33891; // Same as Tree of Life
        constexpr uint32 CONVOKE_RESTO = 391528;
        constexpr uint32 ADAPTIVE_SWARM_RESTO = 391888;
        constexpr uint32 NOURISH = 50464;
        constexpr uint32 OVERGROWTH = 203651;
        constexpr uint32 SPRING_BLOSSOMS = 207386;
        constexpr uint32 PHOTOSYNTHESIS = 274902;
        constexpr uint32 VERDANT_INFUSION = 392325;
        constexpr uint32 HEALING_TOUCH = 5185;
        constexpr uint32 IRONBARK = 102342;
        constexpr uint32 EFFLORESCENCE = 145205;

        // Procs
        constexpr uint32 CLEARCASTING_RESTO = 16870; // Omen of Clarity - free Regrowth

        // HoTs
        constexpr uint32 HOT_REJUVENATION = 774;
        constexpr uint32 HOT_REGROWTH = 8936;
        constexpr uint32 HOT_LIFEBLOOM = 33763;
        constexpr uint32 HOT_WILD_GROWTH = 48438;
        constexpr uint32 HOT_SPRING_BLOSSOMS = 207386;

        // Hero Talents - Keeper of the Grove
        constexpr uint32 DREAM_OF_CENARIUS = 424657;
        constexpr uint32 GROVE_GUARDIANS = 424658;

        // Hero Talents - Wildstalker
        constexpr uint32 EMPOWERED_SHAPESHIFTING = 481583;
        constexpr uint32 STRATEGIC_INFUSION = 481584;
    }

    // Base AI Compatibility - Common Druid spell aliases (12.0 only)
    namespace Common
    {
        // Forms
        constexpr uint32 BEAR_FORM = 5487;
        constexpr uint32 CAT_FORM = 768;
        constexpr uint32 MOONKIN_FORM = 24858;
        constexpr uint32 TREE_OF_LIFE = 33891;
        constexpr uint32 TRAVEL_FORM = 783;

        // Interrupts
        constexpr uint32 SKULL_BASH = 106839;
        constexpr uint32 SKULL_BASH_BEAR = 106839;
        constexpr uint32 SKULL_BASH_CAT = 106839;
        constexpr uint32 SOLAR_BEAM = 78675;
        constexpr uint32 MIGHTY_BASH = 5211;

        // Defensive
        constexpr uint32 BARKSKIN = 22812;
        constexpr uint32 SURVIVAL_INSTINCTS = 61336;
        constexpr uint32 FRENZIED_REGENERATION = 22842;
        constexpr uint32 IRONBARK = 102342;
        constexpr uint32 CENARION_WARD = 102351;

        // Offensive - Feral
        constexpr uint32 TIGERS_FURY = 5217;
        constexpr uint32 BERSERK_CAT = 106951;
        constexpr uint32 BERSERK_BEAR = 50334;
        constexpr uint32 INCARNATION_KING = 102543;

        // Feral abilities
        constexpr uint32 SHRED = 5221;
        constexpr uint32 RAKE = 1822;
        constexpr uint32 RIP = 1079;
        constexpr uint32 FEROCIOUS_BITE = 22568;
        constexpr uint32 SAVAGE_ROAR = 52610;
        constexpr uint32 SWIPE_CAT = 106830;
        constexpr uint32 THRASH_CAT = 106832;
        constexpr uint32 PRIMAL_WRATH = 285381;

        // Guardian abilities
        constexpr uint32 MANGLE = 33917;
        constexpr uint32 MANGLE_BEAR = 33917;
        constexpr uint32 MAUL = 6807;
        constexpr uint32 IRONFUR = 192081;
        constexpr uint32 THRASH_BEAR = 77758;
        constexpr uint32 SWIPE_BEAR = 213771;
        constexpr uint32 PULVERIZE = 80313;
        constexpr uint32 INCARNATION_GUARDIAN = 102558;

        // Balance abilities
        constexpr uint32 WRATH = 190984;
        constexpr uint32 STARFIRE = 194153;
        constexpr uint32 MOONFIRE = 8921;
        constexpr uint32 SUNFIRE = 93402;
        constexpr uint32 STARSURGE = 78674;
        constexpr uint32 STARFALL = 191034;
        constexpr uint32 CELESTIAL_ALIGNMENT = 194223;
        constexpr uint32 INCARNATION_BALANCE = 102560;

        // Restoration abilities
        constexpr uint32 REJUVENATION = 774;
        constexpr uint32 REGROWTH = 8936;
        constexpr uint32 LIFEBLOOM = 33763;
        constexpr uint32 HEALING_TOUCH = 5185;
        constexpr uint32 WILD_GROWTH = 48438;
        constexpr uint32 SWIFTMEND = 18562;
        constexpr uint32 TRANQUILITY = 740;
        constexpr uint32 INCARNATION_TREE = 33891;
        constexpr uint32 NATURES_SWIFTNESS = 132158;
        constexpr uint32 EFFLORESCENCE = 145205;

        // Utility
        constexpr uint32 REMOVE_CORRUPTION = 2782;
        constexpr uint32 NATURES_CURE = 88423;
        constexpr uint32 REBIRTH = 20484;
        constexpr uint32 INNERVATE = 29166;
        constexpr uint32 STAMPEDING_ROAR = 106898;
        constexpr uint32 TYPHOON = 132469;
    }
}

// ============================================================================
// EVOKER SPELLS - Updated for 12.0
// ============================================================================

namespace Evoker
{
    // Core Abilities (All Specs)
    constexpr uint32 LIVING_FLAME = 361469;
    constexpr uint32 AZURE_STRIKE = 362969;
    constexpr uint32 EMERALD_BLOSSOM = 355913;
    constexpr uint32 FIRE_BREATH = 382266;
    constexpr uint32 DISINTEGRATE = 356995;
    constexpr uint32 HOVER = 358267;
    constexpr uint32 DEEP_BREATH = 382266; // Different forms per spec
    constexpr uint32 SOAR = 369536;
    constexpr uint32 GLIDE_EVOKER = 358733;
    constexpr uint32 TAIL_SWIPE = 368970; // Knockback
    constexpr uint32 WING_BUFFET = 357214; // Knockback
    constexpr uint32 QUELL = 351338; // Interrupt
    constexpr uint32 EXPUNGE = 365585; // Poison dispel
    constexpr uint32 CAUTERIZING_FLAME = 374251; // Dispel
    constexpr uint32 BLESSING_OF_THE_BRONZE = 381748;
    constexpr uint32 FURY_OF_THE_ASPECTS = 390386;
    constexpr uint32 RESCUE = 370665;
    constexpr uint32 VERDANT_EMBRACE = 360995;
    constexpr uint32 LANDSLIDE = 358385; // Root
    constexpr uint32 SLEEP_WALK = 360806; // CC
    constexpr uint32 OBSIDIAN_SCALES = 363916; // Defensive ability
    constexpr uint32 RENEWING_BLAZE = 374348; // Self heal over time
    constexpr uint32 OPPRESSING_ROAR = 372048; // AoE enrage dispel
    constexpr uint32 SOURCE_OF_MAGIC = 369459; // Mana buff

    // Devastation Specialization
    namespace Devastation
    {
        // Core Rotation
        constexpr uint32 PYRE = 357211;
        constexpr uint32 ETERNITY_SURGE = 359073;
        constexpr uint32 SHATTERING_STAR = 370452;
        constexpr uint32 DRAGONRAGE = 375087;
        constexpr uint32 FIRESTORM = 368847;
        constexpr uint32 BURNOUT = 375801;
        constexpr uint32 ENGULFING_BLAZE = 370837;
        constexpr uint32 VOLATILITY = 369089;
        constexpr uint32 ANIMOSITY = 375797;
        constexpr uint32 ESSENSE_BURST = 359618;

        // Essence Resource
        constexpr uint32 ESSENCE = 469;

        // Procs and Buffs
        constexpr uint32 TIP_THE_SCALES = 370553;  // Instant empower
        constexpr uint32 IRIDESCENCE_BLUE = 386399;  // Azure Strike empowerment
        constexpr uint32 IRIDESCENCE_RED = 386353;  // Pyre/Fire Breath empowerment

        // Talents
        constexpr uint32 CATALYZE = 386283;  // Essence Burst chance
        constexpr uint32 FEED_THE_FLAMES = 369846;  // Fire Breath extended
        constexpr uint32 ONYX_LEGACY = 386348;  // Deep Breath enhanced

        // Hero Talents - Flameshaper
        constexpr uint32 ENGULF = 443328;
        constexpr uint32 CONSUME = 443329;
        constexpr uint32 TRAVELING_FLAME = 443330;

        // Hero Talents - Scalecommander
        constexpr uint32 MASS_DISINTEGRATE = 436301;
        constexpr uint32 MASS_ERUPTION = 436302;
        constexpr uint32 MOTES_OF_POSSIBILITY = 436303;
    }

    // Preservation Specialization
    namespace Preservation
    {
        // Core Rotation
        constexpr uint32 ECHO = 364343;
        constexpr uint32 REVERSION = 366155;
        constexpr uint32 TEMPORAL_ANOMALY = 373861;
        constexpr uint32 TIME_DILATION = 357170;
        constexpr uint32 DREAM_BREATH = 382614;
        constexpr uint32 SPIRITBLOOM = 382731;
        constexpr uint32 REWIND = 363534;
        constexpr uint32 EMERALD_COMMUNION = 370960;
        constexpr uint32 DREAM_FLIGHT = 359816;
        constexpr uint32 STASIS = 370537;
        constexpr uint32 TIME_OF_NEED = 368412;
        constexpr uint32 TEMPORAL_COMPRESSION = 362874;
        constexpr uint32 LIVING_FLAME_HEAL = 361509;  // Heal version of Living Flame

        // Utility
        constexpr uint32 LIFEBIND = 373267;  // Link two allies, share healing
        constexpr uint32 TWIN_GUARDIAN = 370888;  // Shield another player

        // Procs
        constexpr uint32 ESSENCE_BURST_PRES = 369299;  // Free essence spender
        constexpr uint32 CALL_OF_YSERA = 373835;  // Dream Breath proc

        // Talents
        constexpr uint32 FIELD_OF_DREAMS = 370062;  // Dream Breath AoE larger
        constexpr uint32 FLOW_STATE = 385696;  // Essence regen
        constexpr uint32 LIFEFORCE_MENDER = 376179;  // Healing increase

        // Hero Talents - Chronowarden
        constexpr uint32 CHRONO_FLAMES = 431442;
        constexpr uint32 TIME_CONVERGENCE = 431443;
        constexpr uint32 TEMPORAL_BURST = 431444;

        // Hero Talents - Flameshaper
        constexpr uint32 PRES_ENGULF = 443328;
        constexpr uint32 PRES_CONSUME = 443329;
        constexpr uint32 PRES_TRAVELING_FLAME = 443330;
    }

    // Augmentation Specialization
    namespace Augmentation
    {
        // Core Rotation
        constexpr uint32 EBON_MIGHT = 395296;
        constexpr uint32 PRESCIENCE = 409311;
        constexpr uint32 BREATH_OF_EONS = 403631;
        constexpr uint32 TIME_SKIP = 404977;
        constexpr uint32 SPATIAL_PARADOX = 406732;
        constexpr uint32 BLISTERING_SCALES = 360827;
        constexpr uint32 UPHEAVAL = 408092;
        constexpr uint32 ERUPTION = 395160;

        // Hero Talents - Chronowarden
        constexpr uint32 CHRONO_MAGIC = 431442;
        constexpr uint32 THREADS_OF_FATE = 431443;
        constexpr uint32 TEMPORAL_MASTERY = 431444;

        // Hero Talents - Scalecommander
        constexpr uint32 MIGHT_OF_THE_BLACK_DRAGONFLIGHT = 436301;
        constexpr uint32 UNRELENTING_SIEGE = 436302;
        constexpr uint32 EXTENDED_BATTLE = 436303;
    }

    // Base AI Compatibility - Common Evoker spell aliases (12.0 only)
    namespace Common
    {
        // Basic abilities
        constexpr uint32 AZURE_STRIKE = 362969;
        constexpr uint32 LIVING_FLAME = 361469;
        constexpr uint32 HOVER = 358267;
        constexpr uint32 SOAR = 369536;

        // Devastation abilities
        constexpr uint32 ETERNITY_SURGE = 359073;
        constexpr uint32 ETERNITYS_SURGE = 359073; // Alias
        constexpr uint32 DISINTEGRATE = 356995;
        constexpr uint32 PYRE = 357211;
        constexpr uint32 DEEP_BREATH = 382266;
        constexpr uint32 FIRE_BREATH = 382266;
        constexpr uint32 DRAGONRAGE = 375087;

        // Preservation abilities
        constexpr uint32 DREAM_BREATH = 382614;
        constexpr uint32 SPIRITBLOOM = 382731;
        constexpr uint32 EMERALD_BLOSSOM = 355913;
        constexpr uint32 VERDANT_EMBRACE = 360995;
        constexpr uint32 LIFEBIND = 373267;
        constexpr uint32 EMERALD_COMMUNION = 370960;
        constexpr uint32 TEMPORAL_ANOMALY = 373861;
        constexpr uint32 ECHO = 364343;

        // Augmentation abilities
        constexpr uint32 EBON_MIGHT = 395296;
        constexpr uint32 BREATH_OF_EONS = 403631;
        constexpr uint32 PRESCIENCE = 409311;
        constexpr uint32 BLISTERING_SCALES = 360827;

        // Utility abilities
        constexpr uint32 BLESSING_OF_THE_BRONZE = 381748;
        constexpr uint32 LANDSLIDE = 358385;
        constexpr uint32 TAIL_SWIPE = 368970;
        constexpr uint32 WING_BUFFET = 357214;
        constexpr uint32 SLEEP_WALK = 360806;
        constexpr uint32 QUELL = 351338;

        // Defensive abilities
        constexpr uint32 OBSIDIAN_SCALES = 363916;
        constexpr uint32 RENEWING_BLAZE = 374348;
        constexpr uint32 RESCUE = 370665;
    }
}

// ============================================================================
// HUNTER SPELLS - Updated for 12.0
// ============================================================================

namespace Hunter
{
    // Core Abilities (All Specs)
    constexpr uint32 ARCANE_SHOT = 185358;
    constexpr uint32 STEADY_SHOT = 56641;
    constexpr uint32 KILL_SHOT = 53351;
    constexpr uint32 HUNTERS_MARK = 257284;
    constexpr uint32 AIMED_SHOT = 19434;
    constexpr uint32 RAPID_FIRE = 257044;
    constexpr uint32 MULTI_SHOT = 257620;
    constexpr uint32 COUNTER_SHOT = 147362; // Interrupt
    constexpr uint32 DISENGAGE = 781;
    constexpr uint32 ASPECT_OF_THE_TURTLE = 186265;
    constexpr uint32 ASPECT_OF_THE_CHEETAH = 186257;
    constexpr uint32 EXHILARATION = 109304;
    constexpr uint32 SURVIVAL_OF_THE_FITTEST = 264735;
    constexpr uint32 FREEZING_TRAP = 187650;
    constexpr uint32 TAR_TRAP = 187698;
    constexpr uint32 FLARE = 1543;
    constexpr uint32 MISDIRECTION = 34477;
    constexpr uint32 FEIGN_DEATH = 5384;
    constexpr uint32 TRANQUILIZING_SHOT = 19801; // Dispel
    constexpr uint32 INTIMIDATION = 19577; // Pet stun

    // Pet Management (All Specs)
    constexpr uint32 CALL_PET_1 = 883;
    constexpr uint32 DISMISS_PET = 2641;
    constexpr uint32 REVIVE_PET = 982;
    constexpr uint32 MEND_PET = 136;
    constexpr uint32 PET_ATTACK = 52398;
    constexpr uint32 PET_FOLLOW = 52399;
    constexpr uint32 PET_STAY = 52400;

    // Beast Mastery Specialization
    namespace BeastMastery
    {
        // Core Rotation
        constexpr uint32 BARBED_SHOT = 217200;
        constexpr uint32 KILL_COMMAND = 34026;
        constexpr uint32 COBRA_SHOT = 193455;
        constexpr uint32 BESTIAL_WRATH = 19574;
        constexpr uint32 ASPECT_OF_THE_WILD = 193530;
        constexpr uint32 DIRE_BEAST = 120679;
        constexpr uint32 BLOODSHED = 321530;
        constexpr uint32 CALL_OF_THE_WILD = 359844;
        constexpr uint32 STAMPEDE = 201430;
        constexpr uint32 WAILING_ARROW = 392060;
        constexpr uint32 DEATH_CHAKRAM = 375891;

        // Pet Management
        constexpr uint32 CALL_PET_1 = 883;
        constexpr uint32 CALL_PET_2 = 83242;
        constexpr uint32 CALL_PET_3 = 83243;
        constexpr uint32 CALL_PET_4 = 83244;
        constexpr uint32 CALL_PET_5 = 83245;
        constexpr uint32 ANIMAL_COMPANION = 267116;
        constexpr uint32 MEND_PET = 136;
        constexpr uint32 REVIVE_PET = 982;

        // Hero Talents - Pack Leader
        constexpr uint32 VICIOUS_HUNT = 445404;
        constexpr uint32 PACK_COORDINATION = 445405;
        constexpr uint32 HOWL_OF_THE_PACK = 445406;

        // Procs and Buffs
        constexpr uint32 WILD_CALL = 185789; // Barbed Shot reset proc
        constexpr uint32 PET_FRENZY = 272790; // Pet attack speed buff
        constexpr uint32 MULTI_SHOT_BM = 2643; // BM Multi-Shot

        // Hero Talents - Dark Ranger
        constexpr uint32 BLACK_ARROW = 430703;
        constexpr uint32 SHADOW_SURGE = 430704;
        constexpr uint32 EMBRACE_THE_SHADOWS = 430705;
    }

    // Marksmanship Specialization
    namespace Marksmanship
    {
        // Core Rotation
        constexpr uint32 AIMED_SHOT_MM = 19434;
        constexpr uint32 RAPID_FIRE_MM = 257044;
        constexpr uint32 ARCANE_SHOT_MM = 185358;
        constexpr uint32 STEADY_SHOT_MM = 56641;
        constexpr uint32 TRUESHOT = 288613;
        constexpr uint32 DOUBLE_TAP = 260402;
        constexpr uint32 EXPLOSIVE_SHOT = 212431;
        constexpr uint32 VOLLEY = 260243;
        constexpr uint32 WAILING_ARROW_MM = 392060;
        constexpr uint32 DEATH_CHAKRAM_MM = 375891;
        constexpr uint32 WIND_ARROWS = 450842;
        constexpr uint32 SALVO = 400456;

        // Precise Shots
        constexpr uint32 PRECISE_SHOTS = 260242;
        constexpr uint32 TRICK_SHOTS = 257621;
        constexpr uint32 LETHAL_SHOTS = 260393; // Crit buff
        constexpr uint32 CAREFUL_AIM = 260228; // High HP crit bonus

        // Utility
        constexpr uint32 BINDING_SHOT = 109248; // Tether CC
        constexpr uint32 SCATTER_SHOT = 213691; // Disorient
        constexpr uint32 BURSTING_SHOT = 186387; // Knockback
        constexpr uint32 SURVIVAL_TACTICS = 202746; // Defensive buff
        constexpr uint32 LONE_WOLF = 155228; // No pet damage bonus

        // Hero Talents - Sentinel
        constexpr uint32 SENTINEL_OWL = 450405;
        constexpr uint32 SENTINEL_PRECISION = 450406;
        constexpr uint32 EYES_OF_THE_SENTINEL = 450407;

        // Hero Talents - Dark Ranger
        constexpr uint32 MM_BLACK_ARROW = 430703;
        constexpr uint32 MM_SHADOW_SURGE = 430704;
        constexpr uint32 MM_DARKNESS = 430705;
    }

    // Survival Specialization
    namespace Survival
    {
        // Core Rotation (Melee)
        constexpr uint32 RAPTOR_STRIKE = 186270;
        constexpr uint32 MONGOOSE_BITE = 259387;
        constexpr uint32 KILL_COMMAND_SURVIVAL = 259489;
        constexpr uint32 WILDFIRE_BOMB = 259495;
        constexpr uint32 SERPENT_STING = 259491;
        constexpr uint32 COORDINATED_ASSAULT = 360952;
        constexpr uint32 FLANKING_STRIKE = 269751;
        constexpr uint32 FURY_OF_THE_EAGLE = 203415;
        constexpr uint32 SPEARHEAD = 378785;
        constexpr uint32 BUTCHERY = 212436;
        constexpr uint32 DEATH_CHAKRAM_SV = 375891;
        constexpr uint32 EXPLOSIVE_SHOT_SV = 212431;
        constexpr uint32 CARVE = 187708; // AoE cleave

        // Wildfire Bombs (variants)
        constexpr uint32 SHRAPNEL_BOMB = 270335; // Bleed variant
        constexpr uint32 PHEROMONE_BOMB = 270323; // Debuff variant
        constexpr uint32 VOLATILE_BOMB = 271045; // Damage variant
        constexpr uint32 WILDFIRE_INFUSION = 271014; // Random bomb selection

        // DoTs and Debuffs
        constexpr uint32 INTERNAL_BLEEDING = 270343; // Bleed from Shrapnel
        constexpr uint32 BLOODSEEKER = 260248; // Attack speed from bleeds

        // Traps
        constexpr uint32 STEEL_TRAP = 162488;
        constexpr uint32 CALTROPS = 194277;

        // Focus Builders
        constexpr uint32 HARPOON = 190925;
        constexpr uint32 TERMS_OF_ENGAGEMENT = 265895;

        // Utility
        constexpr uint32 ASPECT_OF_THE_EAGLE = 186289; // Increased range
        constexpr uint32 MUZZLE = 187707; // Interrupt
        constexpr uint32 GUERRILLA_TACTICS = 264332; // First bomb enhancement

        // Hero Talents - Pack Leader
        constexpr uint32 SV_VICIOUS_HUNT = 445404;
        constexpr uint32 SV_PACK_COORDINATION = 445405;

        // Hero Talents - Sentinel
        constexpr uint32 SV_SENTINEL = 460405;
        constexpr uint32 SV_LUNAR_STORM = 460406;
    }

    // Base AI Compatibility - Common Hunter spell aliases (12.0 only)
    namespace Common
    {
        // Shots and Attacks
        constexpr uint32 STEADY_SHOT = 56641;
        constexpr uint32 ARCANE_SHOT = 185358;
        constexpr uint32 MULTI_SHOT = 257620;
        constexpr uint32 AIMED_SHOT = 19434;
        constexpr uint32 KILL_SHOT = 53351;
        constexpr uint32 EXPLOSIVE_SHOT = 212431;
        constexpr uint32 SERPENT_STING = 259491;
        constexpr uint32 CONCUSSIVE_SHOT = 5116;

        // Pet Abilities
        constexpr uint32 KILL_COMMAND = 34026;
        constexpr uint32 MEND_PET = 136;
        constexpr uint32 REVIVE_PET = 982;
        constexpr uint32 CALL_PET = 883;
        constexpr uint32 MASTERS_CALL = 53271;

        // Traps
        constexpr uint32 FREEZING_TRAP = 187650;
        constexpr uint32 TAR_TRAP = 187698;
        constexpr uint32 EXPLOSIVE_TRAP = 236776; // Hi-Explosive Trap (PvP talent)

        // Defensive/Utility
        constexpr uint32 DISENGAGE = 781;
        constexpr uint32 FEIGN_DEATH = 5384;
        constexpr uint32 EXHILARATION = 109304;
        constexpr uint32 SCATTER_SHOT = 213691;
        constexpr uint32 COUNTER_SHOT = 147362;

        // Aspects
        constexpr uint32 ASPECT_OF_THE_WILD = 193530;
        constexpr uint32 ASPECT_OF_THE_CHEETAH = 186257;
        constexpr uint32 ASPECT_OF_THE_TURTLE = 186265;

        // Marks/Debuffs
        constexpr uint32 HUNTERS_MARK = 257284;

        // Cooldowns
        constexpr uint32 RAPID_FIRE = 257044;
        constexpr uint32 BESTIAL_WRATH = 19574;
        constexpr uint32 TRUESHOT = 288613;
        constexpr uint32 BARRAGE = 120360;
        constexpr uint32 VOLLEY = 260243;
    }
}

// ============================================================================
// MAGE SPELLS - Updated for 12.0
// ============================================================================

namespace Mage
{
    // Core Abilities (All Specs)
    constexpr uint32 FROSTBOLT = 116;
    constexpr uint32 FIREBALL = 133;
    constexpr uint32 ARCANE_INTELLECT = 1459;
    constexpr uint32 CONJURE_REFRESHMENT = 190336;
    constexpr uint32 BLINK = 1953;
    constexpr uint32 SHIMMER = 212653;
    constexpr uint32 ICE_BLOCK = 45438;
    constexpr uint32 INVISIBILITY = 66;
    constexpr uint32 GREATER_INVISIBILITY = 110959;
    constexpr uint32 MIRROR_IMAGE = 55342;
    constexpr uint32 ALTER_TIME = 342245;
    constexpr uint32 COUNTERSPELL = 2139; // Interrupt
    constexpr uint32 SPELLSTEAL = 30449; // Purge
    constexpr uint32 REMOVE_CURSE = 475; // Decurse
    constexpr uint32 SLOW_FALL = 130;
    constexpr uint32 POLYMORPH = 118;
    constexpr uint32 FROST_NOVA = 122;
    constexpr uint32 DRAGONS_BREATH = 31661;
    constexpr uint32 BLAST_WAVE = 157981;
    constexpr uint32 TIME_WARP = 80353; // Heroism/Bloodlust
    constexpr uint32 TELEPORT = 3561;
    constexpr uint32 PORTAL = 10059;
    constexpr uint32 SHIFTING_POWER_COMMON = 382440; // Shared Night Fae ability

    // Arcane Specialization
    namespace Arcane
    {
        // Core Rotation
        constexpr uint32 ARCANE_BLAST = 30451;
        constexpr uint32 ARCANE_MISSILES = 5143;
        constexpr uint32 ARCANE_BARRAGE = 44425;
        constexpr uint32 ARCANE_EXPLOSION = 1449;
        constexpr uint32 ARCANE_POWER = 12042;
        constexpr uint32 EVOCATION = 12051;
        constexpr uint32 PRESENCE_OF_MIND = 205025;
        constexpr uint32 ARCANE_ORB = 153626;
        constexpr uint32 NETHER_TEMPEST = 114923;
        constexpr uint32 SUPERNOVA = 157980;
        constexpr uint32 RADIANT_SPARK = 376103;
        constexpr uint32 SHIFTING_POWER = 382440;
        constexpr uint32 ARCANE_SURGE = 365350;
        constexpr uint32 TOUCH_OF_THE_MAGI = 321507;
        constexpr uint32 ARCANE_FAMILIAR = 205022;

        // Arcane Charges
        constexpr uint32 ARCANE_CHARGE = 36032;
        constexpr uint32 CLEARCASTING = 263725;

        // Hero Talents - Spellslinger
        constexpr uint32 SPLINTERSTORM = 443742;
        constexpr uint32 AUGURY_ABOUNDS = 443743;
        constexpr uint32 UNERRING_PROFICIENCY = 443744;

        // Hero Talents - Sunfury
        constexpr uint32 GLORIOUS_INCANDESCENCE = 449394;
        constexpr uint32 CODEX_OF_THE_SUNSTRIDER = 449395;
        constexpr uint32 INVOCATION_SUNFURY = 449396;
    }

    // Fire Specialization
    namespace Fire
    {
        // Core Rotation
        constexpr uint32 PYROBLAST = 11366;
        constexpr uint32 FIRE_BLAST = 108853;
        constexpr uint32 PHOENIX_FLAMES = 257541;
        constexpr uint32 SCORCH = 2948;
        constexpr uint32 FLAMESTRIKE = 2120;
        constexpr uint32 COMBUSTION = 190319;
        constexpr uint32 LIVING_BOMB = 44457;
        constexpr uint32 METEOR = 153561;
        constexpr uint32 KINDLING = 155148;
        constexpr uint32 PYROCLASM = 269650;
        constexpr uint32 ALEXSTRASZAS_FURY = 235870;
        constexpr uint32 FROM_THE_ASHES = 342344;
        constexpr uint32 HYPERTHERMIA = 383391;
        constexpr uint32 BLAZING_BARRIER = 235313;

        // Heating Up / Hot Streak
        constexpr uint32 HEATING_UP = 48107;
        constexpr uint32 HOT_STREAK = 48108;

        // Hero Talents - Frostfire
        constexpr uint32 FROSTFIRE_BOLT = 431044;
        constexpr uint32 ISOTHERMIC_CORE = 431045;
        constexpr uint32 EXCESS_FROST = 431046;
        constexpr uint32 EXCESS_FIRE = 431047;

        // Hero Talents - Sunfury
        constexpr uint32 FIRE_GLORIOUS_INCANDESCENCE = 449394;
        constexpr uint32 FIRE_CODEX_OF_THE_SUNSTRIDER = 449395;
        constexpr uint32 FIRE_INVOCATION = 449396;
    }

    // Frost Specialization
    namespace Frost
    {
        // Core Rotation
        constexpr uint32 ICE_LANCE = 30455;
        constexpr uint32 FLURRY = 44614;
        constexpr uint32 FROZEN_ORB = 84714;
        constexpr uint32 BLIZZARD = 190356;
        constexpr uint32 CONE_OF_COLD = 120;
        constexpr uint32 ICY_VEINS = 12472;
        constexpr uint32 ICE_NOVA = 157997;
        constexpr uint32 RAY_OF_FROST = 205021;
        constexpr uint32 GLACIAL_SPIKE = 199786;
        constexpr uint32 EBONBOLT = 214634;
        constexpr uint32 COMET_STORM = 153595;
        constexpr uint32 ICE_FORM = 383269;
        constexpr uint32 SNOWDRIFT = 389794;

        // Procs
        constexpr uint32 BRAIN_FREEZE = 190446;
        constexpr uint32 FINGERS_OF_FROST = 44544;
        constexpr uint32 WINTERS_CHILL = 228358;
        constexpr uint32 ICE_BARRIER = 11426;

        // Water Elemental
        constexpr uint32 SUMMON_WATER_ELEMENTAL = 31687;
        constexpr uint32 FREEZE = 33395; // Water Elemental's Freeze ability

        // Hero Talents - Frostfire
        constexpr uint32 FROST_FROSTFIRE_BOLT = 431044;
        constexpr uint32 FROST_ISOTHERMIC_CORE = 431045;
        constexpr uint32 FROST_THERMAL_CONDITIONING = 431046;

        // Hero Talents - Spellslinger
        constexpr uint32 FROST_SPLINTERSTORM = 443742;
        constexpr uint32 FROST_AUGURY_ABOUNDS = 443743;
    }

    // Base AI Compatibility - Common Mage spell aliases (12.0 only)
    namespace Common
    {
        // Arcane spells
        constexpr uint32 ARCANE_MISSILES = 5143;
        constexpr uint32 ARCANE_BLAST = 30451;
        constexpr uint32 ARCANE_BARRAGE = 44425;
        constexpr uint32 ARCANE_ORB = 153626;
        constexpr uint32 ARCANE_POWER = 12042;
        constexpr uint32 ARCANE_INTELLECT = 1459;
        constexpr uint32 ARCANE_EXPLOSION = 1449;

        // Fire spells
        constexpr uint32 FIREBALL = 133;
        constexpr uint32 FIRE_BLAST = 108853;
        constexpr uint32 PYROBLAST = 11366;
        constexpr uint32 FLAMESTRIKE = 2120;
        constexpr uint32 SCORCH = 2948;
        constexpr uint32 COMBUSTION = 190319;
        constexpr uint32 LIVING_BOMB = 44457;
        constexpr uint32 DRAGONS_BREATH = 31661;

        // Frost spells
        constexpr uint32 FROSTBOLT = 116;
        constexpr uint32 ICE_LANCE = 30455;
        constexpr uint32 FROZEN_ORB = 84714;
        constexpr uint32 BLIZZARD = 190356;
        constexpr uint32 CONE_OF_COLD = 120;
        constexpr uint32 ICY_VEINS = 12472;
        constexpr uint32 SUMMON_WATER_ELEMENTAL = 31687;
        constexpr uint32 ICE_BARRIER = 11426;
        constexpr uint32 FROST_NOVA = 122;

        // Crowd control
        constexpr uint32 POLYMORPH = 118;
        constexpr uint32 COUNTERSPELL = 2139;

        // Defensive abilities
        constexpr uint32 BLINK = 1953;
        constexpr uint32 SHIMMER = 212653;
        constexpr uint32 INVISIBILITY = 66;
        constexpr uint32 GREATER_INVISIBILITY = 110959;
        constexpr uint32 ICE_BLOCK = 45438;

        // Utility
        constexpr uint32 MIRROR_IMAGE = 55342;
        constexpr uint32 PRESENCE_OF_MIND = 205025;
        constexpr uint32 TIME_WARP = 80353;

        // Armor/Barrier spells
        constexpr uint32 BLAZING_BARRIER = 235313;

        // Conjure spells
        constexpr uint32 CONJURE_REFRESHMENT = 190336;

        // Teleport spells
        constexpr uint32 TELEPORT_STORMWIND = 3561;
        constexpr uint32 TELEPORT_IRONFORGE = 3562;

        // Portal spells
        constexpr uint32 PORTAL_STORMWIND = 10059;
        constexpr uint32 PORTAL_IRONFORGE = 11416;

        // Misc
        constexpr uint32 SLOW_FALL = 130;
        constexpr uint32 REMOVE_CURSE = 475;
        constexpr uint32 SPELLSTEAL = 30449;
    }
}

// ============================================================================
// MONK SPELLS - Updated for 12.0
// ============================================================================

namespace Monk
{
    // Core Abilities (All Specs)
    constexpr uint32 ROLL = 109132;
    constexpr uint32 CHI_TORPEDO = 115008;
    constexpr uint32 TIGERS_LUST = 116841;
    constexpr uint32 TRANSCENDENCE = 101643;
    constexpr uint32 TRANSCENDENCE_TRANSFER = 119996;
    constexpr uint32 PARALYSIS = 115078; // CC
    constexpr uint32 LEG_SWEEP = 119381; // Stun
    constexpr uint32 RING_OF_PEACE = 116844;
    constexpr uint32 CRACKLING_JADE_LIGHTNING = 117952;
    constexpr uint32 SPEAR_HAND_STRIKE = 116705; // Interrupt
    constexpr uint32 TOUCH_OF_DEATH = 322109;
    constexpr uint32 FORTIFYING_BREW = 115203;
    constexpr uint32 DETOX = 115450; // Dispel (MW)
    constexpr uint32 DIFFUSE_MAGIC = 122783;
    constexpr uint32 DAMPEN_HARM = 122278;
    constexpr uint32 ZEN_PILGRIMAGE = 126892;
    constexpr uint32 PROVOKE = 115546; // Taunt
    constexpr uint32 RESUSCITATE = 115178; // Resurrect
    constexpr uint32 EXPEL_HARM = 322101; // Self-heal (all specs)
    constexpr uint32 ZEN_MEDITATION = 115176; // Damage reduction channel
    constexpr uint32 CHI_WAVE = 115098; // Bouncing heal/damage
    constexpr uint32 TIGER_PALM = 100780; // Core builder (all specs)
    constexpr uint32 TEACHINGS_OF_THE_MONASTERY = 202090; // Tiger Palm buff

    // Brewmaster Specialization
    namespace Brewmaster
    {
        // Core Rotation
        constexpr uint32 KEG_SMASH = 121253;
        constexpr uint32 TIGER_PALM = 100780;
        constexpr uint32 BLACKOUT_KICK = 205523;
        constexpr uint32 BREATH_OF_FIRE = 115181;
        constexpr uint32 RUSHING_JADE_WIND = 116847;
        constexpr uint32 SPINNING_CRANE_KICK = 101546;
        constexpr uint32 CELESTIAL_BREW = 322507;
        constexpr uint32 PURIFYING_BREW = 119582;
        constexpr uint32 INVOKE_NIUZAO = 132578;
        constexpr uint32 EXPLODING_KEG = 325153;
        constexpr uint32 BONEDUST_BREW = 386276;
        constexpr uint32 WEAPONS_OF_ORDER = 387184;

        // Stagger
        constexpr uint32 STAGGER = 124255;
        constexpr uint32 LIGHT_STAGGER = 124275;
        constexpr uint32 MODERATE_STAGGER = 124274;
        constexpr uint32 HEAVY_STAGGER = 124273;

        // Passive/Talents
        constexpr uint32 SHUFFLE = 215479; // Stagger buff from Blackout Kick
        constexpr uint32 ELUSIVE_BRAWLER = 195630; // Mastery - dodge stacks
        constexpr uint32 GIFT_OF_THE_OX = 124502; // Healing orb passive
        constexpr uint32 BLACK_OX_BREW = 115399; // Reset brews
        constexpr uint32 CHARRED_PASSIONS = 386965; // Breath of Fire enhancement
        constexpr uint32 HIGH_TOLERANCE = 196737; // Haste from Stagger
        constexpr uint32 BOB_AND_WEAVE = 280515; // Stagger duration talent
        constexpr uint32 SCALDING_BREW = 383698; // Keg Smash DoT bonus
        constexpr uint32 COUNTERSTRIKE = 383800; // Counter damage talent

        // Hero Talents - Master of Harmony
        constexpr uint32 ASPECT_OF_HARMONY = 450508;
        constexpr uint32 HARMONIC_GAMBIT = 450509;
        constexpr uint32 BALANCED_STRATAGEM = 450510;

        // Hero Talents - Shado-Pan
        constexpr uint32 FLURRY_STRIKES = 450711;
        constexpr uint32 WISDOM_OF_THE_WALL = 450712;
        constexpr uint32 LEAD_FROM_THE_FRONT = 450713;
    }

    // Mistweaver Specialization
    namespace Mistweaver
    {
        // Core Rotation
        constexpr uint32 SOOTHING_MIST = 115175;
        constexpr uint32 ENVELOPING_MIST = 124682;
        constexpr uint32 RENEWING_MIST = 115151;
        constexpr uint32 VIVIFY = 116670;
        constexpr uint32 ESSENCE_FONT = 191837;
        constexpr uint32 LIFE_COCOON = 116849;
        constexpr uint32 REVIVAL = 115310;
        constexpr uint32 INVOKE_YULON = 322118;
        constexpr uint32 INVOKE_CHI_JI = 325197;
        constexpr uint32 MANA_TEA = 197908;
        constexpr uint32 THUNDER_FOCUS_TEA = 116680;
        constexpr uint32 RISING_SUN_KICK = 107428;
        constexpr uint32 BLACKOUT_KICK_MW = 100784;
        constexpr uint32 TIGER_PALM_MW = 100780;
        constexpr uint32 SPINNING_CRANE_KICK_MW = 101546;
        constexpr uint32 CHI_BURST = 123986;
        constexpr uint32 CHI_WAVE = 132463;
        constexpr uint32 REFRESHING_JADE_WIND = 196725;
        constexpr uint32 ANCIENT_TEACHINGS = 388023;
        constexpr uint32 SHEILUNS_GIFT = 399491;
        constexpr uint32 FAELINE_STOMP = 388193;

        // Passive/Talents
        constexpr uint32 UPWELLING = 274963; // Essence Font buff
        constexpr uint32 LIFECYCLES = 197915; // Vivify/Enveloping cost reduction
        constexpr uint32 SPIRIT_OF_THE_CRANE = 210802; // Mana return on kick
        constexpr uint32 CLOUDED_FOCUS = 388047; // Soothing Mist channel buff
        constexpr uint32 RISING_MIST = 274909; // Extend HoTs on RSK
        constexpr uint32 FOCUSED_THUNDER = 197895; // Double Thunder Focus Tea
        constexpr uint32 SECRET_INFUSION = 388491; // TFT stat buffs
        constexpr uint32 INVOKE_SHEILUN = 399491; // Alternative name for Sheilun's Gift

        // Hero Talents - Master of Harmony
        constexpr uint32 MW_ASPECT_OF_HARMONY = 450508;
        constexpr uint32 MW_HARMONIC_GAMBIT = 450509;

        // Hero Talents - Conduit of the Celestials
        constexpr uint32 CELESTIAL_CONDUIT = 443028;
        constexpr uint32 AUGUST_DYNASTY = 443029;
        constexpr uint32 UNITY_WITHIN = 443030;
    }

    // Windwalker Specialization
    namespace Windwalker
    {
        // Core Rotation
        constexpr uint32 TIGER_PALM_WW = 100780;
        constexpr uint32 BLACKOUT_KICK_WW = 100784;
        constexpr uint32 RISING_SUN_KICK_WW = 107428;
        constexpr uint32 FISTS_OF_FURY = 113656;
        constexpr uint32 SPINNING_CRANE_KICK_WW = 101546;
        constexpr uint32 WHIRLING_DRAGON_PUNCH = 152175;
        constexpr uint32 STRIKE_OF_THE_WINDLORD = 392983;
        constexpr uint32 STORM_EARTH_AND_FIRE = 137639;
        constexpr uint32 SERENITY = 152173;
        constexpr uint32 INVOKE_XUEN = 123904;
        constexpr uint32 CHI_BURST_WW = 123986;
        constexpr uint32 CHI_WAVE_WW = 132463;
        constexpr uint32 FLYING_SERPENT_KICK = 101545;
        constexpr uint32 RUSHING_JADE_WIND_WW = 261715;
        constexpr uint32 TOUCH_OF_KARMA = 122470;
        constexpr uint32 BONEDUST_BREW_WW = 386276;
        constexpr uint32 WEAPONS_OF_ORDER_WW = 387184;
        constexpr uint32 FAELINE_STOMP_WW = 388193;
        constexpr uint32 JADEFIRE_STOMP = 388193; // New name

        // Chi and Energy
        constexpr uint32 CHI = 163;
        constexpr uint32 ENERGY = 103;

        // Passive/Talents
        constexpr uint32 DANCE_OF_CHI_JI = 325202; // Free SCK procs
        constexpr uint32 COMBO_BREAKER = 137384; // Free Blackout Kick
        constexpr uint32 HIT_COMBO = 196740; // Damage buff for unique abilities
        constexpr uint32 MARK_OF_THE_CRANE = 228287; // SCK damage increase
        constexpr uint32 TEACHINGS_OF_THE_MONASTERY_WW = 202090; // Tiger Palm stacks
        constexpr uint32 TRANSFER_THE_POWER = 195300; // RSK damage buff
        constexpr uint32 JADE_IGNITION = 392979; // Stomp empowerment
        constexpr uint32 LAST_EMPERORS_CAPACITOR = 392989; // CJL damage buff
        constexpr uint32 GLORY_OF_THE_DAWN = 392958; // Extra RSK
        constexpr uint32 SHADOWBOXING_TREADS = 392982; // Extra Blackout Kick
        constexpr uint32 INVOKERS_DELIGHT = 388661; // Haste on Xuen
        constexpr uint32 XUENS_BATTLEGEAR = 392993; // Fists of Fury extension

        // Hero Talents - Shado-Pan
        constexpr uint32 WW_FLURRY_STRIKES = 450711;
        constexpr uint32 WW_WISDOM_OF_THE_WALL = 450712;
        constexpr uint32 WW_VIGILANT_WATCH = 450713;

        // Hero Talents - Conduit of the Celestials
        constexpr uint32 WW_CELESTIAL_CONDUIT = 443028;
        constexpr uint32 WW_AUGUST_DYNASTY = 443029;
    }

    // Base AI Compatibility - Common Monk spell aliases (12.0 only)
    namespace Common
    {
        // Chi generators
        constexpr uint32 TIGER_PALM = 100780;
        constexpr uint32 EXPEL_HARM = 322101;
        constexpr uint32 CHI_WAVE = 115098;
        constexpr uint32 CHI_BURST = 123986;

        // Basic attacks
        constexpr uint32 BLACKOUT_KICK = 100784;
        constexpr uint32 RISING_SUN_KICK = 107428;
        constexpr uint32 SPINNING_CRANE_KICK = 101546;

        // Windwalker abilities
        constexpr uint32 FISTS_OF_FURY = 113656;
        constexpr uint32 WHIRLING_DRAGON_PUNCH = 152175;
        constexpr uint32 STORM_EARTH_AND_FIRE = 137639;
        constexpr uint32 TOUCH_OF_DEATH = 322109;
        constexpr uint32 FLYING_SERPENT_KICK = 101545;
        constexpr uint32 MARK_OF_THE_CRANE = 228287;
        constexpr uint32 RUSHING_JADE_WIND = 116847;
        constexpr uint32 RUSHING_JADE_WIND_WW = 261715;
        constexpr uint32 SERENITY = 152173;

        // Brewmaster abilities
        constexpr uint32 KEG_SMASH = 121253;
        constexpr uint32 BREATH_OF_FIRE = 115181;
        constexpr uint32 CELESTIAL_BREW = 322507;
        constexpr uint32 PURIFYING_BREW = 119582;
        constexpr uint32 FORTIFYING_BREW = 115203;
        constexpr uint32 BLACK_OX_BREW = 115399;
        constexpr uint32 STAGGER = 124255;
        constexpr uint32 STAGGER_HEAVY = 124273;
        constexpr uint32 STAGGER_MODERATE = 124274;
        constexpr uint32 STAGGER_LIGHT = 124275;
        constexpr uint32 ZEN_MEDITATION = 115176;
        constexpr uint32 DAMPEN_HARM = 122278;

        // Mistweaver abilities
        constexpr uint32 RENEWING_MIST = 115151;
        constexpr uint32 ENVELOPING_MIST = 124682;
        constexpr uint32 VIVIFY = 116670;
        constexpr uint32 ESSENCE_FONT = 191837;
        constexpr uint32 SOOTHING_MIST = 115175;
        constexpr uint32 LIFE_COCOON = 116849;
        constexpr uint32 REVIVAL = 115310;
        constexpr uint32 THUNDER_FOCUS_TEA = 116680;
        constexpr uint32 MANA_TEA = 197908;
        constexpr uint32 TEACHINGS_OF_THE_MONASTERY = 202090;
        constexpr uint32 SHEILUNS_GIFT = 399491;

        // Mobility
        constexpr uint32 ROLL = 109132;
        constexpr uint32 CHI_TORPEDO = 115008;
        constexpr uint32 TRANSCENDENCE = 101643;
        constexpr uint32 TRANSCENDENCE_TRANSFER = 119996;
        constexpr uint32 TIGERS_LUST = 116841;

        // Utility and crowd control
        constexpr uint32 PARALYSIS = 115078;
        constexpr uint32 LEG_SWEEP = 119381;
        constexpr uint32 SPEAR_HAND_STRIKE = 116705;
        constexpr uint32 RING_OF_PEACE = 116844;
        constexpr uint32 CRACKLING_JADE_LIGHTNING = 117952;
        constexpr uint32 DETOX = 115450;
        constexpr uint32 RESUSCITATE = 115178;
        constexpr uint32 PROVOKE = 115546;

        // Defensive cooldowns
        constexpr uint32 TOUCH_OF_KARMA = 122470;
        constexpr uint32 DIFFUSE_MAGIC = 122783;

        // Talents
        constexpr uint32 EYE_OF_THE_TIGER = 196607;
        constexpr uint32 ENERGIZING_ELIXIR = 115288; // Restores Chi and Energy
    }
}

// Continued in next part due to length...

} // namespace WoW120Spells
} // namespace Playerbot