/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * World of Warcraft 12.0 (The War Within) Spell ID Validation - Part 2
 *
 * This file contains validated spell IDs for WoW 12.0 including
 * Hero Talent abilities for Paladin through Warrior specializations.
 */

#pragma once

namespace Playerbot
{
namespace WoW120Spells
{

// ============================================================================
// PALADIN SPELLS - Updated for 12.0
// ============================================================================

namespace Paladin
{
    // Core Abilities (All Specs)
    constexpr uint32 FLASH_OF_LIGHT = 19750;
    constexpr uint32 HOLY_LIGHT = 82326;
    constexpr uint32 WORD_OF_GLORY = 85673;
    constexpr uint32 DIVINE_SHIELD = 642;
    constexpr uint32 DIVINE_PROTECTION = 498;
    constexpr uint32 BLESSING_OF_FREEDOM = 1044;
    constexpr uint32 BLESSING_OF_PROTECTION = 1022;
    constexpr uint32 BLESSING_OF_SACRIFICE = 6940;
    constexpr uint32 LAY_ON_HANDS = 633;
    constexpr uint32 HAMMER_OF_JUSTICE = 853;
    constexpr uint32 HAMMER_OF_WRATH = 24275;
    constexpr uint32 REBUKE = 96231; // Interrupt
    constexpr uint32 CLEANSE = 4987; // Dispel
    constexpr uint32 CLEANSE_TOXINS = 213644; // Dispel poison/disease
    constexpr uint32 CRUSADER_STRIKE = 35395;
    constexpr uint32 JUDGMENT = 20271;
    constexpr uint32 HAND_OF_RECKONING = 62124; // Taunt
    constexpr uint32 TURN_EVIL = 10326; // CC Undead/Demon
    constexpr uint32 BLINDING_LIGHT = 115750;
    constexpr uint32 CONSECRATION = 26573;
    constexpr uint32 AVENGING_WRATH = 31884;

    // Auras
    constexpr uint32 DEVOTION_AURA = 465;
    constexpr uint32 CRUSADER_AURA = 32223;
    constexpr uint32 RETRIBUTION_AURA = 183435;
    constexpr uint32 CONCENTRATION_AURA = 317920;

    // Holy Specialization
    namespace Holy
    {
        // Core Rotation
        constexpr uint32 HOLY_SHOCK = 20473;
        constexpr uint32 LIGHT_OF_DAWN = 85222;
        constexpr uint32 BEACON_OF_LIGHT = 53563;
        constexpr uint32 DIVINE_FAVOR = 210294;
        constexpr uint32 HOLY_PRISM = 114165;
        constexpr uint32 LIGHTS_HAMMER = 114158;
        constexpr uint32 RULE_OF_LAW = 214202;
        constexpr uint32 AVENGING_CRUSADER = 216331;
        constexpr uint32 AURA_MASTERY = 31821;
        constexpr uint32 DIVINE_PURPOSE = 223817;
        constexpr uint32 BEACON_OF_FAITH = 156910;
        constexpr uint32 BEACON_OF_VIRTUE = 200025;
        constexpr uint32 DAYBREAK = 414170;
        constexpr uint32 BARRIER_OF_FAITH = 148039;
        constexpr uint32 BLESSING_OF_SUMMER = 388007;
        constexpr uint32 BLESSING_OF_AUTUMN = 388010;
        constexpr uint32 BLESSING_OF_WINTER = 388011;
        constexpr uint32 BLESSING_OF_SPRING = 388013;

        // Holy-Specific Abilities
        constexpr uint32 LIGHT_OF_THE_MARTYR = 183998;    // Self-damage heal
        constexpr uint32 HOLY_AVENGER = 105809;           // 2 min CD, HP generation boost
        constexpr uint32 GLIMMER_OF_LIGHT = 325966;       // HoT from Holy Shock
        constexpr uint32 AWAKENING = 248033;              // Avenging Wrath CDR talent
        constexpr uint32 UNBREAKABLE_SPIRIT = 114154;     // Reduced defensive CDs
        constexpr uint32 AVENGING_WRATH_HOLY = 31842;     // Holy-specific version (differs from common)
        constexpr uint32 JUDGMENT_HOLY = 275773;          // Holy-specific Judgment

        // Infusion of Light
        constexpr uint32 INFUSION_OF_LIGHT = 53576;

        // Hero Talents - Herald of the Sun
        constexpr uint32 DAWNLIGHT = 431380;
        constexpr uint32 SUNS_AVATAR = 431381;
        constexpr uint32 ETERNAL_FLAME = 431382;
        constexpr uint32 GLEAMING_RAYS = 431383;

        // Hero Talents - Lightsmith
        constexpr uint32 HOLY_ARMAMENT = 432459;
        constexpr uint32 SACRED_WEAPON = 432460;
        constexpr uint32 BLESSED_ASSURANCE = 432461;
    }

    // Protection Specialization
    namespace Protection
    {
        // Core Rotation
        constexpr uint32 AVENGERS_SHIELD = 31935;
        constexpr uint32 SHIELD_OF_THE_RIGHTEOUS = 53600;
        constexpr uint32 HAMMER_OF_THE_RIGHTEOUS = 53595;
        constexpr uint32 BLESSED_HAMMER = 204019;
        constexpr uint32 GRAND_CRUSADER = 85043;           // Proc for free Avenger's Shield
        constexpr uint32 ARDENT_DEFENDER = 31850;
        constexpr uint32 GUARDIAN_OF_ANCIENT_KINGS = 86659;
        constexpr uint32 LIGHT_OF_THE_PROTECTOR = 184092;
        constexpr uint32 REDOUBT = 280373;
        constexpr uint32 BULWARK_OF_ORDER = 209389;
        constexpr uint32 CRUSADERS_JUDGMENT = 204023;
        constexpr uint32 MOMENT_OF_GLORY = 327193;
        constexpr uint32 DIVINE_TOLL = 375576;
        constexpr uint32 SENTINEL = 389539;
        constexpr uint32 EYE_OF_TYR = 387174;

        // Protection-Specific Abilities
        constexpr uint32 JUDGMENT_PROT = 275779;           // Protection-specific Judgment
        constexpr uint32 BLESSING_OF_SPELLWARDING = 204018; // Magic immunity (replaces Divine Protection)
        constexpr uint32 FINAL_STAND = 204077;             // Increases Ardent Defender effectiveness
        constexpr uint32 SHINING_LIGHT = 327510;           // Proc: free Word of Glory
        constexpr uint32 SERAPHIM = 152262;                // 3 HP, all stats buff
        constexpr uint32 BULWARK_OF_RIGHTEOUS_FURY = 386653; // SotR extended duration
        constexpr uint32 FIRST_AVENGER = 203776;           // Avenger's Shield extra charge

        // Holy Power
        constexpr uint32 HOLY_POWER = 85804;

        // Hero Talents - Lightsmith
        constexpr uint32 PROT_HOLY_ARMAMENT = 432459;
        constexpr uint32 PROT_SACRED_WEAPON = 432460;
        constexpr uint32 PROT_HOLY_BULWARK = 432462;

        // Hero Talents - Templar
        constexpr uint32 LIGHTS_GUIDANCE = 427445;
        constexpr uint32 SACROSANCT_CRUSADE = 427446;
        constexpr uint32 UNDISPUTED_RULING = 427447;
    }

    // Retribution Specialization
    namespace Retribution
    {
        // Core Rotation
        constexpr uint32 BLADE_OF_JUSTICE = 184575;
        constexpr uint32 WAKE_OF_ASHES = 255937;
        constexpr uint32 TEMPLARS_VERDICT = 85256;
        constexpr uint32 FINAL_VERDICT = 383328;
        constexpr uint32 DIVINE_STORM = 53385;
        constexpr uint32 EXECUTION_SENTENCE = 343527;
        constexpr uint32 FINAL_RECKONING = 343721;
        constexpr uint32 SHIELD_OF_VENGEANCE = 184662;
        constexpr uint32 EYE_FOR_AN_EYE = 205191;
        constexpr uint32 CRUSADE = 231895;
        constexpr uint32 DIVINE_PURPOSE_RET = 408459;
        constexpr uint32 JUSTICARS_VENGEANCE = 215661;
        constexpr uint32 BLADE_OF_WRATH = 231832;

        // Art of War
        constexpr uint32 ART_OF_WAR = 267344;

        // Hero Talents - Templar
        constexpr uint32 RADIANT_GLORY = 427445;
        constexpr uint32 SHAKE_THE_HEAVENS = 427446;
        constexpr uint32 HIGHER_CALLING = 427447;

        // Hero Talents - Herald of the Sun
        constexpr uint32 RET_DAWNLIGHT = 431380;
        constexpr uint32 RET_SUNS_AVATAR = 431381;
        constexpr uint32 MORNING_STAR = 431384;
    }

    // Base AI Compatibility - Common Paladin spell aliases (12.0 only)
    namespace Common
    {
        // Interrupts
        constexpr uint32 REBUKE = 96231;
        constexpr uint32 HAMMER_OF_JUSTICE = 853;

        // Defensive Cooldowns
        constexpr uint32 DIVINE_SHIELD = 642;
        constexpr uint32 SHIELD_OF_VENGEANCE = 184662;
        constexpr uint32 BLESSING_OF_PROTECTION = 1022;
        constexpr uint32 ARDENT_DEFENDER = 31850;
        constexpr uint32 GUARDIAN_OF_ANCIENT_KINGS = 86659;
        constexpr uint32 LAY_ON_HANDS = 633;
        constexpr uint32 DIVINE_PROTECTION = 498;

        // Offensive Cooldowns
        constexpr uint32 AVENGING_WRATH = 31884;
        constexpr uint32 CRUSADE = 231895;
        constexpr uint32 HOLY_AVENGER = 105809;
        constexpr uint32 EXECUTION_SENTENCE = 343527;

        // Holy Power Generators
        constexpr uint32 CRUSADER_STRIKE = 35395;
        constexpr uint32 BLADE_OF_JUSTICE = 184575;
        constexpr uint32 HAMMER_OF_THE_RIGHTEOUS = 53595;
        constexpr uint32 JUDGMENT = 20271;
        constexpr uint32 WAKE_OF_ASHES = 255937;

        // Holy Power Spenders
        constexpr uint32 TEMPLARS_VERDICT = 85256;
        constexpr uint32 FINAL_VERDICT = 383328;
        constexpr uint32 DIVINE_STORM = 53385;
        constexpr uint32 SHIELD_OF_THE_RIGHTEOUS = 53600;
        constexpr uint32 WORD_OF_GLORY = 85673;

        // AoE Abilities
        constexpr uint32 CONSECRATION = 26573;

        // Auras
        constexpr uint32 DEVOTION_AURA = 465;
        constexpr uint32 CRUSADER_AURA = 32223;
        constexpr uint32 RETRIBUTION_AURA = 183435;

        // Blessings
        constexpr uint32 BLESSING_OF_FREEDOM = 1044;

        // Healing Abilities
        constexpr uint32 FLASH_OF_LIGHT = 19750;
        constexpr uint32 HOLY_LIGHT = 82326;
        constexpr uint32 HOLY_SHOCK = 20473;
        constexpr uint32 LIGHT_OF_DAWN = 85222;
        constexpr uint32 BEACON_OF_LIGHT = 53563;

        // Utility
        constexpr uint32 HAND_OF_RECKONING = 62124;
        constexpr uint32 CLEANSE = 4987;
        constexpr uint32 HAMMER_OF_WRATH = 24275;
        constexpr uint32 BLINDING_LIGHT = 115750;

        // Movement
        constexpr uint32 DIVINE_STEED = 190784;
    }
}

// ============================================================================
// PRIEST SPELLS - Updated for 12.0
// ============================================================================

namespace Priest
{
    // Core Abilities (All Specs)
    constexpr uint32 POWER_WORD_SHIELD = 17;
    constexpr uint32 POWER_WORD_FORTITUDE = 21562;
    constexpr uint32 SHADOW_WORD_PAIN = 589;
    constexpr uint32 SHADOW_WORD_DEATH = 32379;
    constexpr uint32 PSYCHIC_SCREAM = 8122;
    constexpr uint32 DISPEL_MAGIC = 527; // Offensive Dispel
    constexpr uint32 PURIFY = 527; // Defensive Dispel
    constexpr uint32 MASS_DISPEL = 32375;
    constexpr uint32 FADE = 586;
    constexpr uint32 DESPERATE_PRAYER = 19236;
    constexpr uint32 LEAP_OF_FAITH = 73325;
    constexpr uint32 MIND_CONTROL = 605; // CC
    constexpr uint32 SHACKLE_UNDEAD = 9484;
    constexpr uint32 LEVITATE = 1706;
    constexpr uint32 MIND_VISION = 2096;
    constexpr uint32 MIND_SOOTHE = 453;
    constexpr uint32 POWER_INFUSION = 10060;
    constexpr uint32 FLASH_HEAL = 2061;
    constexpr uint32 SMITE = 585; // Shared damage filler

    // Discipline Specialization
    namespace Discipline
    {
        // Core Rotation
        constexpr uint32 PENANCE = 47540;
        constexpr uint32 POWER_WORD_RADIANCE = 194509;
        constexpr uint32 SCHISM = 214621;
        constexpr uint32 MIND_BLAST = 8092;
        constexpr uint32 SHADOW_COVENANT = 314867;
        constexpr uint32 SPIRIT_SHELL = 109964;
        constexpr uint32 EVANGELISM = 246287;
        constexpr uint32 PAIN_SUPPRESSION = 33206;
        constexpr uint32 POWER_WORD_BARRIER = 62618;
        constexpr uint32 RAPTURE = 47536;
        constexpr uint32 SHADOWFIEND = 34433;
        constexpr uint32 MINDBENDER = 123040;
        constexpr uint32 SHADOW_WORD_PAIN_DISC = 589;
        constexpr uint32 PURGE_THE_WICKED = 204197;
        constexpr uint32 DIVINE_STAR = 110744;
        constexpr uint32 HALO = 120517;
        constexpr uint32 CONTRITION = 197419;
        constexpr uint32 SHADOW_MEND = 186263;
        constexpr uint32 ULTIMATE_PENITENCE = 421453;
        constexpr uint32 LUMINOUS_BARRIER = 271466;
        constexpr uint32 PREMONITION = 428924;

        // Atonement
        constexpr uint32 ATONEMENT = 81749;
        constexpr uint32 ATONEMENT_BUFF = 194384;

        // The War Within Abilities
        constexpr uint32 MINDGAMES = 323673; // Discipline version
        constexpr uint32 POWER_WORD_LIFE = 373481;

        // Hero Talents - Oracle
        constexpr uint32 PREEMPTIVE_CARE = 441315;
        constexpr uint32 DIVINE_PROVIDENCE = 441316;
        constexpr uint32 CLAIRVOYANCE = 441317;

        // Hero Talents - Voidweaver
        constexpr uint32 VOID_BLAST = 450405;
        constexpr uint32 VOID_LEECH = 450406;
        constexpr uint32 VOID_INFUSION = 450407;
    }

    // Holy Specialization
    namespace HolyPriest
    {
        // Core Rotation
        constexpr uint32 HEAL = 2060;
        constexpr uint32 HOLY_WORD_SERENITY = 2050;
        constexpr uint32 HOLY_WORD_SANCTIFY = 34861;
        constexpr uint32 HOLY_WORD_CHASTISE = 88625;
        constexpr uint32 PRAYER_OF_MENDING = 33076;
        constexpr uint32 PRAYER_OF_HEALING = 596;
        constexpr uint32 CIRCLE_OF_HEALING = 204883;
        constexpr uint32 HOLY_FIRE = 14914;
        constexpr uint32 DIVINE_HYMN = 64843;
        constexpr uint32 GUARDIAN_SPIRIT = 47788;
        constexpr uint32 SPIRIT_OF_REDEMPTION = 20711;
        constexpr uint32 HOLY_NOVA = 132157;
        constexpr uint32 BINDING_HEAL = 368275;
        constexpr uint32 RENEW = 139;
        constexpr uint32 DIVINE_PROVIDENCE = 415768;
        constexpr uint32 LIGHTWELL = 724;
        constexpr uint32 SYMBOL_OF_HOPE = 64901;
        constexpr uint32 APOTHEOSIS = 200183;
        constexpr uint32 HOLY_WORD_SALVATION = 265202;

        // Chakras
        constexpr uint32 CHAKRA_CHASTISE = 81209;
        constexpr uint32 CHAKRA_SANCTUARY = 81206;

        // The War Within Abilities
        constexpr uint32 EMPYREAL_BLAZE = 372616;
        constexpr uint32 EMPYREAL_BLAZE_AURA = 372617;
        constexpr uint32 POWER_SURGE = 453109;
        constexpr uint32 POWER_SURGE_PERIODIC = 453112;

        // Hero Talents - Oracle
        constexpr uint32 HOLY_PREEMPTIVE_CARE = 441315;
        constexpr uint32 HOLY_DIVINE_PROVIDENCE = 441316;
        constexpr uint32 HOLY_CLAIRVOYANCE = 441317;

        // Hero Talents - Archon
        constexpr uint32 POWER_OF_THE_LIGHT = 431695;
        constexpr uint32 DIVINE_HALO = 431696;
        constexpr uint32 RESONANT_ENERGY = 431697;
    }

    // Shadow Specialization
    namespace Shadow
    {
        // Core Rotation
        constexpr uint32 VAMPIRIC_TOUCH = 34914;
        constexpr uint32 DEVOURING_PLAGUE = 335467;
        constexpr uint32 MIND_BLAST_SHADOW = 8092;
        constexpr uint32 MIND_FLAY = 15407;
        constexpr uint32 MIND_SPIKE = 73510;
        constexpr uint32 MIND_SEAR = 48045;
        constexpr uint32 VOID_ERUPTION = 228260;
        constexpr uint32 VOID_BOLT = 205448;
        constexpr uint32 SHADOWFORM = 232698;
        constexpr uint32 VOIDFORM = 194249;
        constexpr uint32 DARK_ASCENSION = 391109;
        constexpr uint32 VOID_TORRENT = 263165;
        constexpr uint32 SHADOW_CRASH = 342834;
        constexpr uint32 PSYCHIC_HORROR = 64044;
        constexpr uint32 SILENCE = 15487;
        constexpr uint32 VAMPIRIC_EMBRACE = 15286;
        constexpr uint32 DISPERSION = 47585;
        constexpr uint32 SHADOWFIEND_SHADOW = 34433;
        constexpr uint32 MINDBENDER_SHADOW = 200174;
        constexpr uint32 SURRENDER_TO_MADNESS = 193223;
        constexpr uint32 DARK_VOID = 263346;
        constexpr uint32 PSYCHIC_LINK = 199484;
        constexpr uint32 SHADOW_WORD_VOID = 205351;

        // Insanity Resource
        constexpr uint32 INSANITY = 194022;

        // Proc Buffs
        constexpr uint32 SURGE_OF_INSANITY = 87160;      // Makes Mind Spike/Mind Flay instant
        constexpr uint32 MIND_FLAY_INSANITY = 391403;    // Instant Mind Flay (Surge of Insanity proc)
        constexpr uint32 MIND_SPIKE_INSANITY = 407466;   // Instant Mind Spike (Surge of Insanity proc)
        constexpr uint32 DEATHSPEAKER = 392507;          // SW:Death becomes usable regardless of HP

        // The War Within Abilities
        constexpr uint32 MINDGAMES_SHADOW = 375901; // Shadow version (different from Discipline)
        constexpr uint32 HALO_SHADOW = 120644; // Shadow-specific Halo ID
        constexpr uint32 MINDBENDER = 123040; // Primary Mindbender ID

        // Hero Talents - Voidweaver
        constexpr uint32 SHADOW_VOID_BLAST = 450405;
        constexpr uint32 SHADOW_VOID_LEECH = 450406;
        constexpr uint32 ENTROPIC_RIFT = 450408;

        // Hero Talents - Archon
        constexpr uint32 SHADOW_POWER_OF_THE_LIGHT = 431695;
        constexpr uint32 SHADOW_DIVINE_HALO = 431696;
        constexpr uint32 SHADOW_RESONANT_ENERGY = 431697;
    }

    // Base AI Compatibility - Common Priest spell aliases (12.0 only)
    namespace Common
    {
        // Interrupts
        constexpr uint32 SILENCE = 15487;
        constexpr uint32 PSYCHIC_SCREAM = 8122;

        // Defensive Cooldowns
        constexpr uint32 DESPERATE_PRAYER = 19236;
        constexpr uint32 FADE = 586;
        constexpr uint32 DISPERSION = 47585;
        constexpr uint32 POWER_WORD_SHIELD = 17;

        // Dispels
        constexpr uint32 DISPEL_MAGIC = 527;
        constexpr uint32 PURIFY = 527;
        constexpr uint32 MASS_DISPEL = 32375;

        // Buffs
        constexpr uint32 POWER_WORD_FORTITUDE = 21562;
        constexpr uint32 POWER_INFUSION = 10060;
        constexpr uint32 LEVITATE = 1706;

        // Healing Abilities
        constexpr uint32 FLASH_HEAL = 2061;
        constexpr uint32 HEAL = 2060;
        constexpr uint32 PRAYER_OF_MENDING = 33076;
        constexpr uint32 RENEW = 139;
        constexpr uint32 HOLY_WORD_SERENITY = 2050;
        constexpr uint32 HOLY_WORD_SANCTIFY = 34861;
        constexpr uint32 CIRCLE_OF_HEALING = 204883;

        // Damage Abilities
        constexpr uint32 SMITE = 585;
        constexpr uint32 SHADOW_WORD_PAIN = 589;
        constexpr uint32 SHADOW_WORD_DEATH = 32379;
        constexpr uint32 MIND_BLAST = 8092;
        constexpr uint32 HOLY_FIRE = 14914;
        constexpr uint32 VAMPIRIC_TOUCH = 34914;
        constexpr uint32 DEVOURING_PLAGUE = 335467;
        constexpr uint32 MIND_FLAY = 15407;

        // Utility
        constexpr uint32 LEAP_OF_FAITH = 73325;
        constexpr uint32 MIND_CONTROL = 605;
        constexpr uint32 SHACKLE_UNDEAD = 9484;

        // Forms
        constexpr uint32 SHADOWFORM = 232698;
    }
}

// ============================================================================
// ROGUE SPELLS - Updated for 12.0
// ============================================================================

namespace Rogue
{
    // Core Abilities (All Specs)
    constexpr uint32 STEALTH = 1784;
    constexpr uint32 VANISH = 1856;
    constexpr uint32 CHEAP_SHOT = 1833;
    constexpr uint32 KIDNEY_SHOT = 408;
    constexpr uint32 SAP = 6770;
    constexpr uint32 BLIND = 2094;
    constexpr uint32 KICK = 1766; // Interrupt
    constexpr uint32 SPRINT = 2983;
    constexpr uint32 EVASION = 5277;
    constexpr uint32 CLOAK_OF_SHADOWS = 31224;
    constexpr uint32 CRIMSON_VIAL = 185311;
    constexpr uint32 TRICKS_OF_THE_TRADE = 57934;
    constexpr uint32 DISTRACT = 1725;
    constexpr uint32 SHROUD_OF_CONCEALMENT = 114018;
    constexpr uint32 PICK_LOCK = 1804;
    constexpr uint32 PICK_POCKET = 921;
    constexpr uint32 SHADOWSTEP = 36554;
    constexpr uint32 SHIV = 5938;
    constexpr uint32 FEINT = 1966;
    constexpr uint32 FAN_OF_KNIVES = 51723;
    constexpr uint32 MARKED_FOR_DEATH = 137619;

    // Poisons
    constexpr uint32 INSTANT_POISON = 315584;
    constexpr uint32 CRIPPLING_POISON = 3408;
    constexpr uint32 NUMBING_POISON = 5761;
    constexpr uint32 WOUND_POISON = 8679;
    constexpr uint32 DEADLY_POISON = 2823;
    constexpr uint32 AMPLIFYING_POISON = 381664;
    constexpr uint32 ATROPHIC_POISON = 381637;

    // Assassination Specialization
    namespace Assassination
    {
        // Core Rotation
        constexpr uint32 MUTILATE = 1329;
        constexpr uint32 ENVENOM = 32645;
        constexpr uint32 GARROTE = 703;
        constexpr uint32 RUPTURE = 1943;
        constexpr uint32 POISONED_KNIFE = 185565;
        constexpr uint32 VENDETTA = 79140;
        constexpr uint32 TOXIC_BLADE = 245388;
        constexpr uint32 EXSANGUINATE = 200806;
        constexpr uint32 SHIV_ASSASSINATION = 5938;
        constexpr uint32 CRIMSON_TEMPEST = 121411;
        constexpr uint32 SEPSIS = 385408;
        constexpr uint32 SERRATED_BONE_SPIKE = 385424;
        constexpr uint32 DEATHMARK = 360194;
        constexpr uint32 KINGSBANE = 385627;
        constexpr uint32 INDISCRIMINATE_CARNAGE = 381802;

        // Energy Management
        constexpr uint32 ENERGY_REGEN = 196911;
        constexpr uint32 VENOMOUS_WOUNDS = 79134;

        // Hero Talents - Deathstalker
        constexpr uint32 DEATHSTALKERS_MARK = 457052;
        constexpr uint32 SHADEWALKER = 457053;
        constexpr uint32 SHROUDED_IN_DARKNESS = 457054;

        // Hero Talents - Fatebound
        constexpr uint32 HAND_OF_FATE = 452536;
        constexpr uint32 FATE_INTERTWINED = 452537;
        constexpr uint32 EDGE_OF_FATE = 452538;
    }

    // Outlaw Specialization
    namespace Outlaw
    {
        // Core Rotation
        constexpr uint32 SINISTER_STRIKE = 193315;
        constexpr uint32 PISTOL_SHOT = 185763;
        constexpr uint32 DISPATCH = 2098;
        constexpr uint32 BETWEEN_THE_EYES = 315341;
        constexpr uint32 SLICE_AND_DICE = 315496;
        constexpr uint32 ROLL_THE_BONES = 315508;
        constexpr uint32 BLADE_FLURRY = 13877;
        constexpr uint32 ADRENALINE_RUSH = 13750;
        constexpr uint32 BLADE_RUSH = 271877;
        constexpr uint32 GRAPPLING_HOOK = 195457;
        constexpr uint32 KILLING_SPREE = 51690;
        constexpr uint32 GHOSTLY_STRIKE = 196937;
        constexpr uint32 ECHOING_REPRIMAND = 385616;
        constexpr uint32 KEEP_IT_ROLLING = 381989;
        constexpr uint32 COUNT_THE_ODDS = 381982;
        constexpr uint32 DREADBLADES = 343142;

        // Combo Point Finishers
        constexpr uint32 COMBO_POINT_ROGUE = 193315;

        // Procs
        constexpr uint32 OPPORTUNITY_PROC = 195627;

        // Roll the Bones Buffs
        constexpr uint32 SKULL_AND_CROSSBONES = 199603;
        constexpr uint32 GRAND_MELEE = 193358;
        constexpr uint32 RUTHLESS_PRECISION = 193357;
        constexpr uint32 TRUE_BEARING = 193359;
        constexpr uint32 BURIED_TREASURE = 199600;
        constexpr uint32 BROADSIDE = 193356;

        // Hero Talents - Trickster
        constexpr uint32 UNSEEN_BLADE = 441146;
        constexpr uint32 SMOKE_SCREEN = 441147;
        constexpr uint32 SO_TRICKY = 441148;

        // Hero Talents - Fatebound
        constexpr uint32 OUTLAW_HAND_OF_FATE = 452536;
        constexpr uint32 OUTLAW_FATE_INTERTWINED = 452537;
    }

    // Subtlety Specialization
    namespace Subtlety
    {
        // Core Rotation
        constexpr uint32 BACKSTAB = 53;
        constexpr uint32 SHADOWSTRIKE = 185438;
        constexpr uint32 EVISCERATE = 196819;
        constexpr uint32 SHADOW_DANCE = 185313;
        constexpr uint32 SYMBOLS_OF_DEATH = 212283;
        constexpr uint32 SHADOW_BLADES = 121471;
        constexpr uint32 SHURIKEN_STORM = 197835;
        constexpr uint32 SHURIKEN_TOSS = 114014;
        constexpr uint32 BLACK_POWDER = 319175;
        constexpr uint32 SECRET_TECHNIQUE = 280719;
        constexpr uint32 SHURIKEN_TORNADO = 277925;
        constexpr uint32 SHADOW_VAULT = 185433;
        constexpr uint32 ECHOING_REPRIMAND_SUB = 385616;
        constexpr uint32 FLAGELLATION = 384631;
        constexpr uint32 COLD_BLOOD = 382245;
        constexpr uint32 SHADOWSTEP_SUB = 36554;
        constexpr uint32 GLOOMBLADE = 200758;

        // Shadow Techniques
        constexpr uint32 SHADOW_TECHNIQUES = 196911;
        constexpr uint32 FIND_WEAKNESS = 316220;

        // Talents
        constexpr uint32 DEEPER_STRATAGEM = 193531;
        constexpr uint32 DARK_SHADOW = 245687;

        // Procs and Buffs
        constexpr uint32 DANSE_MACABRE = 393969;
        constexpr uint32 DEEPER_DAGGERS = 383405;

        // Hero Talents - Deathstalker
        constexpr uint32 SUB_DEATHSTALKERS_MARK = 457052;
        constexpr uint32 SUB_SHADEWALKER = 457053;

        // Hero Talents - Trickster
        constexpr uint32 SUB_UNSEEN_BLADE = 441146;
        constexpr uint32 SUB_SMOKE_SCREEN = 441147;
    }

    // Base AI Compatibility - Common Rogue spell aliases (12.0 only)
    namespace Common
    {
        // Combo Point Builders
        constexpr uint32 SINISTER_STRIKE = 1752;
        constexpr uint32 BACKSTAB = 53;
        constexpr uint32 MUTILATE = 1329;
        constexpr uint32 AMBUSH = 8676;
        constexpr uint32 GARROTE = 703;
        constexpr uint32 CHEAP_SHOT = 1833;
        constexpr uint32 SHIV = 5938;
        constexpr uint32 SHADOWSTRIKE = 185438;

        // Combo Point Finishers
        constexpr uint32 SLICE_AND_DICE = 5171;
        constexpr uint32 RUPTURE = 1943;
        constexpr uint32 EVISCERATE = 2098;
        constexpr uint32 KIDNEY_SHOT = 408;
        constexpr uint32 ENVENOM = 32645;
        constexpr uint32 CRIMSON_TEMPEST = 121411;
        constexpr uint32 BETWEEN_THE_EYES = 315341;

        // Interrupts
        constexpr uint32 KICK = 1766;
        constexpr uint32 GOUGE = 1776;
        constexpr uint32 BLIND = 2094;

        // Defensive Cooldowns
        constexpr uint32 VANISH = 1856;
        constexpr uint32 CLOAK_OF_SHADOWS = 31224;
        constexpr uint32 EVASION = 5277;
        constexpr uint32 FEINT = 1966;
        constexpr uint32 CRIMSON_VIAL = 185311;

        // Offensive Cooldowns
        constexpr uint32 ADRENALINE_RUSH = 13750;
        constexpr uint32 BLADE_FLURRY = 13877;
        constexpr uint32 KILLING_SPREE = 51690;
        constexpr uint32 VENDETTA = 79140;
        constexpr uint32 SHADOW_BLADES = 121471;
        constexpr uint32 SHADOW_DANCE = 185313;
        constexpr uint32 SYMBOLS_OF_DEATH = 212283;
        constexpr uint32 COLD_BLOOD = 382245;

        // Utility
        constexpr uint32 STEALTH = 1784;
        constexpr uint32 SHADOWSTEP = 36554;
        constexpr uint32 SAP = 6770;
        constexpr uint32 SPRINT = 2983;
        constexpr uint32 TRICKS_OF_THE_TRADE = 57934;
        constexpr uint32 MARKED_FOR_DEATH = 137619;

        // Poisons
        constexpr uint32 DEADLY_POISON = 2823;
        constexpr uint32 INSTANT_POISON = 315584;
        constexpr uint32 WOUND_POISON = 8679;
        constexpr uint32 CRIPPLING_POISON = 3408;

        // AoE
        constexpr uint32 FAN_OF_KNIVES = 51723;
    }
}

// ============================================================================
// SHAMAN SPELLS - Updated for 12.0
// ============================================================================

namespace Shaman
{
    // Core Abilities (All Specs)
    constexpr uint32 LIGHTNING_BOLT = 188196;
    constexpr uint32 CHAIN_LIGHTNING = 188443;
    constexpr uint32 LAVA_BURST = 51505;
    constexpr uint32 FLAME_SHOCK = 188389;
    constexpr uint32 FROST_SHOCK = 196840;
    constexpr uint32 EARTH_SHOCK = 8042;
    constexpr uint32 HEALING_STREAM_TOTEM = 5394;
    constexpr uint32 CAPACITOR_TOTEM = 192058;
    constexpr uint32 EARTHBIND_TOTEM = 2484;
    constexpr uint32 TREMOR_TOTEM = 8143;
    constexpr uint32 WIND_SHEAR = 57994; // Interrupt
    constexpr uint32 PURGE = 370; // Dispel
    constexpr uint32 CLEANSE_SPIRIT = 51886; // Decurse (Resto)
    constexpr uint32 HEX = 51514; // CC
    constexpr uint32 BLOODLUST = 2825; // Heroism/Bloodlust
    constexpr uint32 HEROISM = 32182; // Alliance version
    constexpr uint32 GHOST_WOLF = 2645;
    constexpr uint32 WATER_WALKING = 546;
    constexpr uint32 FAR_SIGHT = 6196;
    constexpr uint32 ASTRAL_RECALL = 556;
    constexpr uint32 REINCARNATION = 20608;
    constexpr uint32 ANCESTRAL_SPIRIT = 2008; // Resurrect
    constexpr uint32 WIND_RUSH_TOTEM = 192077;
    constexpr uint32 SPIRIT_WALK = 58875;
    constexpr uint32 ASTRAL_SHIFT = 108271; // Defensive cooldown

    // Elemental Specialization
    namespace Elemental
    {
        // Core Rotation
        constexpr uint32 ELEMENTAL_BLAST = 117014;
        constexpr uint32 EARTHQUAKE = 61882;
        constexpr uint32 STORMKEEPER = 191634;
        constexpr uint32 ASCENDANCE = 114050;
        constexpr uint32 FIRE_ELEMENTAL = 198067;
        constexpr uint32 STORM_ELEMENTAL = 192249;
        constexpr uint32 ICEFURY = 210714;
        constexpr uint32 LIQUID_MAGMA_TOTEM = 192222;
        constexpr uint32 PRIMORDIAL_WAVE = 375982;
        constexpr uint32 ECHOES_OF_GREAT_SUNDERING = 336215;
        constexpr uint32 LAVA_BEAM = 114074;
        constexpr uint32 CHAIN_HARVEST = 320674;
        constexpr uint32 ELEMENTAL_EQUILIBRIUM = 378270;
        constexpr uint32 LIGHTNING_LASSO = 305483;
        constexpr uint32 TEMPEST = 454009;
        constexpr uint32 ECHOING_SHOCK = 320125;

        // Maelstrom Resource
        constexpr uint32 MAELSTROM = 470;

        // Hero Talents - Farseer
        constexpr uint32 ANCESTRAL_SWIFTNESS = 443454;
        constexpr uint32 ELEMENTAL_REVERB = 443455;
        constexpr uint32 OFFERING_FROM_BEYOND = 443456;

        // Hero Talents - Stormbringer
        constexpr uint32 TEMPEST_STRIKES = 428071;
        constexpr uint32 UNLIMITED_POWER = 428072;
        constexpr uint32 AWAKENING_STORMS = 428073;
    }

    // Enhancement Specialization
    namespace Enhancement
    {
        // Core Rotation
        constexpr uint32 STORMSTRIKE = 17364;
        constexpr uint32 WINDSTRIKE = 115356; // During Ascendance
        constexpr uint32 LAVA_LASH = 60103;
        constexpr uint32 ELEMENTAL_WEAPONS = 382027;
        constexpr uint32 FLAMETONGUE_WEAPON = 318038;
        constexpr uint32 WINDFURY_WEAPON = 33757;
        constexpr uint32 CRASH_LIGHTNING = 187874;
        constexpr uint32 SUNDERING = 197214;
        constexpr uint32 FIRE_NOVA = 333974;
        constexpr uint32 ICE_STRIKE = 342240;
        constexpr uint32 ELEMENTAL_SPIRITS = 262624;
        constexpr uint32 FERAL_SPIRIT = 51533;
        constexpr uint32 DOOM_WINDS = 384352;
        constexpr uint32 WINDFURY_TOTEM = 8512;
        constexpr uint32 PRIMORDIAL_WAVE_ENH = 375982;
        constexpr uint32 HOT_HAND = 201900;
        constexpr uint32 ELEMENTAL_BLAST_ENH = 117014;
        constexpr uint32 TEMPEST_STRIKES_ENH = 428071;
        constexpr uint32 ELEMENTAL_BLAST_ENH_NEW = 117014;
        constexpr uint32 ASCENDANCE_ENH = 114051;

        // Maelstrom Weapon
        constexpr uint32 MAELSTROM_WEAPON = 187880;

        // Hero Talents - Totemic
        constexpr uint32 SURGING_TOTEM = 444995;
        constexpr uint32 OVERSURGE = 444996;
        constexpr uint32 LAVA_LASH_SPREADS = 444997;

        // Hero Talents - Stormbringer
        constexpr uint32 ENH_TEMPEST_STRIKES = 428071;
        constexpr uint32 ENH_UNLIMITED_POWER = 428072;
    }

    // Restoration Specialization
    namespace Restoration
    {
        // Core Rotation
        constexpr uint32 HEALING_WAVE = 77472;
        constexpr uint32 HEALING_SURGE = 8004;
        constexpr uint32 CHAIN_HEAL = 1064;
        constexpr uint32 RIPTIDE = 61295;
        constexpr uint32 HEALING_RAIN = 73920;
        constexpr uint32 SPIRIT_LINK_TOTEM = 98008;
        constexpr uint32 HEALING_TIDE_TOTEM = 108280;
        constexpr uint32 EARTHEN_WALL_TOTEM = 198838;
        constexpr uint32 ANCESTRAL_PROTECTION_TOTEM = 207399;
        constexpr uint32 CLOUDBURST_TOTEM = 157153;
        constexpr uint32 WELLSPRING = 197995;
        constexpr uint32 UNLEASH_LIFE = 73685;
        constexpr uint32 PRIMORDIAL_WAVE_RESTO = 428332;
        constexpr uint32 SPIRITWALKERS_GRACE = 79206;
        constexpr uint32 MANA_TIDE_TOTEM = 16191;
        constexpr uint32 ASCENDANCE_RESTO = 114052;
        constexpr uint32 EARTH_ELEMENTAL = 198103;
        constexpr uint32 WATER_SHIELD = 52127;
        constexpr uint32 EARTH_SHIELD = 974;
        constexpr uint32 DOWNPOUR = 207778;
        constexpr uint32 VESPER_TOTEM = 324386;
        constexpr uint32 PURIFY_SPIRIT = 77130;

        // Tidal Waves
        constexpr uint32 TIDAL_WAVES = 53390;

        // Hero Talents - Farseer
        constexpr uint32 RESTO_ANCESTRAL_SWIFTNESS = 443454;
        constexpr uint32 RESTO_ELEMENTAL_REVERB = 443455;

        // Hero Talents - Totemic
        constexpr uint32 RESTO_SURGING_TOTEM = 444995;
        constexpr uint32 RESTO_OVERSURGE = 444996;
    }

    // Base AI Compatibility - Common Shaman spell aliases (12.0 only)
    namespace Common
    {
        // Interrupts
        constexpr uint32 WIND_SHEAR = 57994;
        constexpr uint32 CAPACITOR_TOTEM = 192058;

        // Shock Spells
        constexpr uint32 FLAME_SHOCK = 188389;
        constexpr uint32 FROST_SHOCK = 196840;
        constexpr uint32 EARTH_SHOCK = 8042;

        // Core Damage
        constexpr uint32 LIGHTNING_BOLT = 188196;
        constexpr uint32 CHAIN_LIGHTNING = 188443;
        constexpr uint32 LAVA_BURST = 51505;
        constexpr uint32 STORMSTRIKE = 17364;
        constexpr uint32 LAVA_LASH = 60103;
        constexpr uint32 CRASH_LIGHTNING = 187874;
        constexpr uint32 ELEMENTAL_BLAST = 117014;
        constexpr uint32 EARTHQUAKE = 61882;

        // Core Healing
        constexpr uint32 HEALING_WAVE = 77472;
        constexpr uint32 HEALING_SURGE = 8004;
        constexpr uint32 CHAIN_HEAL = 1064;
        constexpr uint32 RIPTIDE = 61295;
        constexpr uint32 HEALING_RAIN = 73920;

        // Offensive Cooldowns
        constexpr uint32 ASCENDANCE = 114050;
        constexpr uint32 STORMKEEPER = 191634;
        constexpr uint32 FIRE_ELEMENTAL = 198067;
        constexpr uint32 FERAL_SPIRIT = 51533;
        constexpr uint32 BLOODLUST = 2825;
        constexpr uint32 HEROISM = 32182;

        // Defensive Cooldowns
        constexpr uint32 ASTRAL_SHIFT = 108271;
        constexpr uint32 SPIRIT_LINK_TOTEM = 98008;
        constexpr uint32 HEALING_TIDE_TOTEM = 108280;
        constexpr uint32 EARTH_ELEMENTAL = 198103;

        // Utility Totems
        constexpr uint32 HEALING_STREAM_TOTEM = 5394;
        constexpr uint32 EARTHBIND_TOTEM = 2484;
        constexpr uint32 TREMOR_TOTEM = 8143;
        constexpr uint32 WIND_RUSH_TOTEM = 192077;
        constexpr uint32 WINDFURY_TOTEM = 8512;

        // Dispels and Utility
        constexpr uint32 PURGE = 370;
        constexpr uint32 CLEANSE_SPIRIT = 51886;
        constexpr uint32 PURIFY_SPIRIT = 77130;
        constexpr uint32 HEX = 51514;
        constexpr uint32 GHOST_WOLF = 2645;
        constexpr uint32 SPIRIT_WALK = 58875;
        constexpr uint32 REINCARNATION = 20608;
        constexpr uint32 ANCESTRAL_SPIRIT = 2008;

        // Weapon Imbues
        constexpr uint32 FLAMETONGUE_WEAPON = 318038;
        constexpr uint32 WINDFURY_WEAPON = 33757;

        // Shields
        constexpr uint32 EARTH_SHIELD = 974;
        constexpr uint32 WATER_SHIELD = 52127;
    }
}

// ============================================================================
// WARLOCK SPELLS - Updated for 12.0
// ============================================================================

namespace Warlock
{
    // Core Abilities (All Specs)
    constexpr uint32 SHADOW_BOLT = 686;
    constexpr uint32 CORRUPTION = 172;
    constexpr uint32 LIFE_TAP = 1454;
    constexpr uint32 DRAIN_LIFE = 234153;
    constexpr uint32 HEALTH_FUNNEL = 755;
    constexpr uint32 SUMMON_IMP = 688;
    constexpr uint32 SUMMON_VOIDWALKER = 697;
    constexpr uint32 SUMMON_SUCCUBUS = 712;
    constexpr uint32 SUMMON_FELHUNTER = 691;
    constexpr uint32 SUMMON_FELGUARD = 30146;
    constexpr uint32 UNENDING_RESOLVE = 104773;
    constexpr uint32 CREATE_HEALTHSTONE = 6201;
    constexpr uint32 CREATE_SOULWELL = 29893;
    constexpr uint32 RITUAL_OF_SUMMONING = 698;
    constexpr uint32 EYE_OF_KILROGG = 126;
    constexpr uint32 FEAR = 5782;
    constexpr uint32 HOWL_OF_TERROR = 5484;
    constexpr uint32 MORTAL_COIL = 6789;
    constexpr uint32 SHADOWFURY = 30283;
    constexpr uint32 SPELL_LOCK = 19647; // Felhunter Interrupt
    constexpr uint32 COMMAND_DEMON = 119898; // Pet ability
    constexpr uint32 BANISH = 710;
    constexpr uint32 CURSE_OF_WEAKNESS = 702;
    constexpr uint32 CURSE_OF_TONGUES = 1714;
    constexpr uint32 CURSE_OF_EXHAUSTION = 334275;
    constexpr uint32 DEMONIC_CIRCLE_SUMMON = 48018;
    constexpr uint32 DEMONIC_CIRCLE_TELEPORT = 48020;
    constexpr uint32 BURNING_RUSH = 111400;
    constexpr uint32 DEMONIC_GATEWAY = 111771;
    constexpr uint32 SOULSTONE = 20707;
    constexpr uint32 DEMONIC_CIRCLE = 48018; // Alias to summon circle
    constexpr uint32 UNENDING_BREATH = 5697; // Water breathing

    // Affliction Specialization
    namespace Affliction
    {
        // Core Rotation
        constexpr uint32 AGONY = 980;
        constexpr uint32 UNSTABLE_AFFLICTION = 316099;
        constexpr uint32 SEED_OF_CORRUPTION = 27243;
        constexpr uint32 HAUNT = 48181;
        constexpr uint32 MALEFIC_RAPTURE = 324536;
        constexpr uint32 DRAIN_SOUL = 198590;
        constexpr uint32 SIPHON_LIFE = 63106;
        constexpr uint32 PHANTOM_SINGULARITY = 205179;
        constexpr uint32 VILE_TAINT = 278350;
        constexpr uint32 DARK_SOUL_MISERY = 113860;
        constexpr uint32 SUMMON_DARKGLARE = 205180;
        constexpr uint32 SOUL_ROT = 386997;
        constexpr uint32 SHADOW_EMBRACE = 32388;
        constexpr uint32 ABSOLUTE_CORRUPTION = 196103;
        constexpr uint32 WRITHE_IN_AGONY = 196102;
        constexpr uint32 CREEPING_DEATH = 264000;
        constexpr uint32 INEVITABLE_DEMISE = 334319;

        // Soul Shards
        constexpr uint32 SOUL_SHARD = 205;

        // Hero Talents - Hellcaller
        constexpr uint32 WITHER = 445468;
        constexpr uint32 XALANS_CRUELTY = 445469;
        constexpr uint32 BLACKENED_SOUL = 445470;

        // Hero Talents - Soul Harvester
        constexpr uint32 DEMONIC_SOUL = 449614;
        constexpr uint32 NECROLYTE_TEACHINGS = 449615;
        constexpr uint32 FEAST_OF_SOULS = 449616;

        // Additional Talents and Procs
        constexpr uint32 SOULBURN = 385899;            // Instant Seed of Corruption
        constexpr uint32 DARK_PACT = 108416;           // 1 min CD, shield
        constexpr uint32 NIGHTFALL = 108558;           // Proc: free Shadow Bolt
        constexpr uint32 TORMENTED_CRESCENDO = 387079; // Stacking Malefic Rapture buff
        constexpr uint32 GRIMOIRE_OF_SACRIFICE = 108503; // Sacrifice pet for damage buff
        constexpr uint32 SOUL_CONDUIT = 215941;        // Chance to refund soul shards
    }

    // Demonology Specialization
    namespace Demonology
    {
        // Core Rotation
        constexpr uint32 DEMONBOLT = 264178;
        constexpr uint32 HAND_OF_GULDAN = 105174;
        constexpr uint32 CALL_DREADSTALKERS = 104316;
        constexpr uint32 IMPLOSION = 196277;
        constexpr uint32 SUMMON_DEMONIC_TYRANT = 265187;
        constexpr uint32 BILESCOURGE_BOMBERS = 267211;
        constexpr uint32 POWER_SIPHON = 264130;
        constexpr uint32 DOOM = 603;
        constexpr uint32 DEMONIC_STRENGTH = 267171;
        constexpr uint32 SOUL_STRIKE = 264057;
        constexpr uint32 SUMMON_VILEFIEND = 264119;
        constexpr uint32 GUILLOTINE = 386833;
        constexpr uint32 NETHER_PORTAL = 267217;
        constexpr uint32 GRIMOIRE_FELGUARD = 111898;
        constexpr uint32 DEMONIC_CONSUMPTION = 267215;
        constexpr uint32 INNER_DEMONS = 267216;

        // Demon Management
        constexpr uint32 WILD_IMP = 104317;
        constexpr uint32 DREADSTALKER = 104316;

        // Hero Talents - Diabolist
        constexpr uint32 DIABOLIC_RITUAL = 428514;
        constexpr uint32 CLOVEN_SOULS = 428515;
        constexpr uint32 SECRETS_OF_THE_COVEN = 428516;

        // Hero Talents - Soul Harvester
        constexpr uint32 DEMO_DEMONIC_SOUL = 449614;
        constexpr uint32 DEMO_NECROLYTE_TEACHINGS = 449615;

        // Additional Talents and Procs
        constexpr uint32 DEMONFIRE = 270569;           // DoT from Felguard
        constexpr uint32 DEMONIC_CORE = 267102;        // Proc: free Demonbolt
        constexpr uint32 DEMONIC_CALLING = 205145;     // Proc: reduced Dreadstalkers cost
        constexpr uint32 FEL_DOMINATION = 333889;      // Instant summon
        constexpr uint32 FROM_THE_SHADOWS = 267170;    // Dreadstalkers buff
        constexpr uint32 SOUL_CONDUIT = 215941;        // Chance to refund soul shards
        constexpr uint32 CARNIVOROUS_STALKERS = 386194; // Dreadstalkers extend duration
    }

    // Destruction Specialization
    namespace Destruction
    {
        // Core Rotation
        constexpr uint32 INCINERATE = 29722;
        constexpr uint32 IMMOLATE = 348;
        constexpr uint32 CONFLAGRATE = 17962;
        constexpr uint32 CHAOS_BOLT = 116858;
        constexpr uint32 RAIN_OF_FIRE = 5740;
        constexpr uint32 HAVOC = 80240;
        constexpr uint32 SHADOWBURN = 17877;
        constexpr uint32 CATACLYSM = 152108;
        constexpr uint32 CHANNEL_DEMONFIRE = 196447;
        constexpr uint32 DARK_SOUL_INSTABILITY = 113858;
        constexpr uint32 SUMMON_INFERNAL = 1122;
        constexpr uint32 SOUL_FIRE = 6353;
        constexpr uint32 SHADOWFURY_DESTRO = 30283;
        constexpr uint32 ROARING_BLAZE = 205184;
        constexpr uint32 INTERNAL_COMBUSTION = 266134;
        constexpr uint32 ERADICATION = 196412;
        constexpr uint32 FIRE_AND_BRIMSTONE = 196408;
        constexpr uint32 BACKDRAFT = 196406;

        // Soul Shards and Burning Embers
        constexpr uint32 SOUL_SHARD_DESTRO = 205;
        constexpr uint32 BURNING_EMBER = 175;

        // Hero Talents - Hellcaller
        constexpr uint32 DESTRO_WITHER = 445468;
        constexpr uint32 DESTRO_XALANS_CRUELTY = 445469;

        // Hero Talents - Diabolist
        constexpr uint32 DESTRO_DIABOLIC_RITUAL = 428514;
        constexpr uint32 DESTRO_CLOVEN_SOULS = 428515;

        // Additional Talents and Procs
        constexpr uint32 BACKDRAFT_BUFF = 117828;      // Buff from Conflagrate
        constexpr uint32 REVERSE_ENTROPY = 205148;     // Buff from Rain of Fire
        constexpr uint32 FLASHOVER = 267115;           // Backdraft on Conflagrate CD end
        constexpr uint32 GRIMOIRE_OF_SUPREMACY = 266086; // Better pets
        constexpr uint32 INFERNO = 270545;             // Rain of Fire stun
    }

    // Base AI Compatibility - Common Warlock spell aliases (12.0 only)
    namespace Common
    {
        // Core Damage
        constexpr uint32 SHADOW_BOLT = 686;
        constexpr uint32 CORRUPTION = 172;
        constexpr uint32 DRAIN_LIFE = 234153;
        constexpr uint32 AGONY = 980;
        constexpr uint32 UNSTABLE_AFFLICTION = 316099;
        constexpr uint32 DEMONBOLT = 264178;
        constexpr uint32 INCINERATE = 29722;
        constexpr uint32 IMMOLATE = 348;
        constexpr uint32 CONFLAGRATE = 17962;
        constexpr uint32 CHAOS_BOLT = 116858;

        // Interrupts / CC
        constexpr uint32 SPELL_LOCK = 19647;
        constexpr uint32 FEAR = 5782;
        constexpr uint32 HOWL_OF_TERROR = 5484;
        constexpr uint32 MORTAL_COIL = 6789;
        constexpr uint32 SHADOWFURY = 30283;
        constexpr uint32 BANISH = 710;

        // Defensive Cooldowns
        constexpr uint32 UNENDING_RESOLVE = 104773;
        constexpr uint32 DARK_PACT = 108416;

        // Summons and Pets
        constexpr uint32 SUMMON_IMP = 688;
        constexpr uint32 SUMMON_VOIDWALKER = 697;
        constexpr uint32 SUMMON_SUCCUBUS = 712;
        constexpr uint32 SUMMON_FELHUNTER = 691;
        constexpr uint32 SUMMON_FELGUARD = 30146;
        constexpr uint32 SUMMON_INFERNAL = 1122;
        constexpr uint32 SUMMON_DARKGLARE = 205180;
        constexpr uint32 SUMMON_DEMONIC_TYRANT = 265187;
        constexpr uint32 CALL_DREADSTALKERS = 104316;
        constexpr uint32 HAND_OF_GULDAN = 105174;
        constexpr uint32 HEALTH_FUNNEL = 755;

        // Offensive Cooldowns
        constexpr uint32 DARK_SOUL_MISERY = 113860;
        constexpr uint32 DARK_SOUL_INSTABILITY = 113858;
        constexpr uint32 SOUL_ROT = 386997;

        // Utility
        constexpr uint32 CREATE_HEALTHSTONE = 6201;
        constexpr uint32 CREATE_SOULWELL = 29893;
        constexpr uint32 RITUAL_OF_SUMMONING = 698;
        constexpr uint32 DEMONIC_CIRCLE_SUMMON = 48018;
        constexpr uint32 DEMONIC_CIRCLE_TELEPORT = 48020;
        constexpr uint32 DEMONIC_GATEWAY = 111771;
        constexpr uint32 BURNING_RUSH = 111400;
        constexpr uint32 SOULSTONE = 20707;

        // Curses
        constexpr uint32 CURSE_OF_WEAKNESS = 702;
        constexpr uint32 CURSE_OF_TONGUES = 1714;
        constexpr uint32 CURSE_OF_EXHAUSTION = 334275;

        // AoE
        constexpr uint32 SEED_OF_CORRUPTION = 27243;
        constexpr uint32 RAIN_OF_FIRE = 5740;
        constexpr uint32 IMPLOSION = 196277;
    }
}

// ============================================================================
// WARRIOR SPELLS - Updated for 12.0.7 (The War Within)
// ============================================================================

namespace Warrior
{
    // Core Abilities (All Specs)
    constexpr uint32 CHARGE = 100;
    constexpr uint32 HEROIC_LEAP = 6544;
    constexpr uint32 PUMMEL = 6552; // Interrupt
    constexpr uint32 INTIMIDATING_SHOUT = 5246;
    constexpr uint32 BATTLE_SHOUT = 6673;
    constexpr uint32 COMMANDING_SHOUT = 469;
    constexpr uint32 RALLYING_CRY = 97462;
    constexpr uint32 SPELL_REFLECTION = 23920;
    constexpr uint32 INTERVENE = 3411;
    constexpr uint32 HEROIC_THROW = 57755;
    constexpr uint32 SHATTERING_THROW = 64382;
    constexpr uint32 BERSERKER_RAGE = 18499;
    constexpr uint32 BERSERKER_SHOUT = 384100;
    constexpr uint32 BITTER_IMMUNITY = 383762;
    constexpr uint32 STORM_BOLT = 107570;
    constexpr uint32 SHOCKWAVE = 46968;
    constexpr uint32 TAUNT = 355; // Warrior Taunt
    constexpr uint32 HAMSTRING = 1715;
    constexpr uint32 PIERCING_HOWL = 12323;
    constexpr uint32 VICTORY_RUSH = 34428; // After killing enemy that yields XP/honor

    // Procs and Buffs (All Specs)
    constexpr uint32 OVERPOWER_PROC = 60503;      // Triggers Overpower availability
    constexpr uint32 SUDDEN_DEATH_PROC = 52437;   // Free Execute proc (Arms)
    constexpr uint32 REVENGE_PROC = 5302;         // Triggers Revenge availability (Prot)

    // Arms Specialization
    namespace Arms
    {
        // Core Rotation
        constexpr uint32 MORTAL_STRIKE = 12294;
        constexpr uint32 OVERPOWER = 7384;
        constexpr uint32 EXECUTE = 163201;
        constexpr uint32 SLAM = 1464;
        constexpr uint32 WHIRLWIND = 1680;
        constexpr uint32 BLADESTORM = 227847;
        constexpr uint32 COLOSSUS_SMASH = 167105;
        constexpr uint32 WARBREAKER = 262161;
        constexpr uint32 SWEEPING_STRIKES = 260708;
        constexpr uint32 DIE_BY_THE_SWORD = 118038;
        constexpr uint32 DEFENSIVE_STANCE = 197690;
        constexpr uint32 AVATAR = 107574;
        constexpr uint32 DEADLY_CALM = 262228;
        constexpr uint32 CLEAVE = 845;
        constexpr uint32 SKULLSPLITTER = 260643;
        constexpr uint32 RAVAGER = 152277;
        constexpr uint32 REND = 388539;
        constexpr uint32 THUNDEROUS_ROAR = 384318;
        constexpr uint32 CHAMPIONS_SPEAR = 376079;
        constexpr uint32 SPEAR_OF_BASTION = 376079; // Covenant ability

        // Deep Wounds
        constexpr uint32 DEEP_WOUNDS = 262304;
        constexpr uint32 DEEP_WOUNDS_DEBUFF = 262115;

        // Hero Talents - Slayer
        constexpr uint32 SLAYERS_STRIKE = 444930;
        constexpr uint32 OVERWHELMING_BLADES = 444931;
        constexpr uint32 SLAYERS_DOMINANCE = 444932;

        // Hero Talents - Colossus
        constexpr uint32 DEMOLISH = 436358;
        constexpr uint32 COLOSSAL_MIGHT = 436359;
        constexpr uint32 MARTIAL_PROWESS = 436360;
    }

    // Fury Specialization
    namespace Fury
    {
        // Core Rotation
        constexpr uint32 BLOODTHIRST = 23881;
        constexpr uint32 RAGING_BLOW = 85288;
        constexpr uint32 RAMPAGE = 184367;
        constexpr uint32 EXECUTE = 5308;              // Fury Execute
        constexpr uint32 WHIRLWIND = 190411;          // Fury Whirlwind
        constexpr uint32 BLADESTORM = 46924;          // Fury Bladestorm
        constexpr uint32 RECKLESSNESS = 1719;
        constexpr uint32 ENRAGED_REGENERATION = 184364;
        constexpr uint32 ONSLAUGHT = 315720;
        constexpr uint32 ODYN_FURY = 385059;
        constexpr uint32 RAVAGER = 228920;
        constexpr uint32 THUNDEROUS_ROAR = 384318;
        constexpr uint32 CHAMPIONS_SPEAR = 376079;
        constexpr uint32 ANNIHILATOR = 383916;
        constexpr uint32 TENDERIZE = 388933;
        constexpr uint32 CRUSHING_BLOW = 335097;      // Raging Blow enhancement

        // Enrage and Buffs
        constexpr uint32 ENRAGE = 184361;
        constexpr uint32 WHIRLWIND_BUFF = 85739;      // Whirlwind cleave buff (Meat Cleaver)

        // Hero Talents - Slayer
        constexpr uint32 SLAYERS_STRIKE = 444930;
        constexpr uint32 OVERWHELMING_BLADES = 444931;

        // Hero Talents - Mountain Thane
        constexpr uint32 THUNDER_BLAST = 435222;
        constexpr uint32 STORM_SHIELD = 435223;
        constexpr uint32 THUNDER_HAMMER = 435224;
    }

    // Protection Specialization
    namespace Protection
    {
        // Core Rotation
        constexpr uint32 SHIELD_SLAM = 23922;
        constexpr uint32 THUNDER_CLAP = 6343;
        constexpr uint32 REVENGE = 6572;
        constexpr uint32 DEVASTATE = 20243;           // Filler attack, applies Deep Wounds
        constexpr uint32 SHIELD_BLOCK = 2565;
        constexpr uint32 IGNORE_PAIN = 190456;
        constexpr uint32 DEMORALIZING_SHOUT = 1160;
        constexpr uint32 LAST_STAND = 12975;
        constexpr uint32 SHIELD_WALL = 871;
        constexpr uint32 AVATAR = 401150;             // Protection Avatar
        constexpr uint32 RAVAGER = 228920;            // Protection Ravager
        constexpr uint32 CHALLENGING_SHOUT = 1161;
        constexpr uint32 SPELL_BLOCK = 392966;
        constexpr uint32 CHAMPION_BULWARK = 386328;
        constexpr uint32 SHIELD_CHARGE = 385952;
        constexpr uint32 BOOMING_VOICE = 202743;
        constexpr uint32 DISRUPTING_SHOUT = 386071;
        constexpr uint32 THUNDEROUS_ROAR = 384318;
        constexpr uint32 CHAMPIONS_SPEAR = 376079;

        // Shield Block
        constexpr uint32 SHIELD_BLOCK_BUFF = 132404;

        // Hero Talents - Colossus
        constexpr uint32 DEMOLISH = 436358;
        constexpr uint32 COLOSSAL_MIGHT = 436359;

        // Hero Talents - Mountain Thane
        constexpr uint32 THUNDER_BLAST = 435222;
        constexpr uint32 STORM_SHIELD = 435223;
    }

    // Base AI Compatibility - Common Warrior spell aliases (12.0 only)
    namespace Common
    {
        // Stances (deprecated in 11.x but kept for compatibility)
        constexpr uint32 BATTLE_STANCE = 2457;
        constexpr uint32 DEFENSIVE_STANCE = 197690;
        constexpr uint32 BERSERKER_STANCE = 386196;

        // Basic Attacks
        constexpr uint32 HEROIC_STRIKE = 78;
        constexpr uint32 CLEAVE = 845;
        constexpr uint32 WHIRLWIND = 1680;
        constexpr uint32 SLAM = 1464;

        // Arms Abilities
        constexpr uint32 MORTAL_STRIKE = 12294;
        constexpr uint32 COLOSSUS_SMASH = 167105;
        constexpr uint32 WARBREAKER = 262161;
        constexpr uint32 OVERPOWER = 7384;
        constexpr uint32 REND = 388539;
        constexpr uint32 SWEEPING_STRIKES = 260708;
        constexpr uint32 BLADESTORM = 227847;

        // Fury Abilities
        constexpr uint32 BLOODTHIRST = 23881;
        constexpr uint32 RAMPAGE = 184367;
        constexpr uint32 RAGING_BLOW = 85288;
        constexpr uint32 EXECUTE = 5308;
        constexpr uint32 ONSLAUGHT = 315720;
        constexpr uint32 ODYN_FURY = 385059;
        constexpr uint32 ENRAGED_REGENERATION = 184364;

        // Protection Abilities
        constexpr uint32 SHIELD_SLAM = 23922;
        constexpr uint32 THUNDER_CLAP = 6343;
        constexpr uint32 REVENGE = 6572;
        constexpr uint32 DEVASTATE = 20243;
        constexpr uint32 SHIELD_BLOCK = 2565;
        constexpr uint32 IGNORE_PAIN = 190456;

        // Defensive Cooldowns
        constexpr uint32 SHIELD_WALL = 871;
        constexpr uint32 LAST_STAND = 12975;
        constexpr uint32 SPELL_REFLECTION = 23920;
        constexpr uint32 DIE_BY_THE_SWORD = 118038;
        constexpr uint32 RALLYING_CRY = 97462;

        // Offensive Cooldowns
        constexpr uint32 RECKLESSNESS = 1719;
        constexpr uint32 AVATAR = 107574;
        constexpr uint32 THUNDEROUS_ROAR = 384318;
        constexpr uint32 CHAMPIONS_SPEAR = 376079;
        constexpr uint32 RAVAGER = 152277;

        // Movement Abilities
        constexpr uint32 CHARGE = 100;
        constexpr uint32 HEROIC_LEAP = 6544;
        constexpr uint32 INTERVENE = 3411;

        // Interrupts and CC
        constexpr uint32 PUMMEL = 6552;
        constexpr uint32 STORM_BOLT = 107570;
        constexpr uint32 SHOCKWAVE = 46968;
        constexpr uint32 INTIMIDATING_SHOUT = 5246;
        constexpr uint32 HAMSTRING = 1715;
        constexpr uint32 PIERCING_HOWL = 12323;

        // Buffs
        constexpr uint32 BATTLE_SHOUT = 6673;
        constexpr uint32 COMMANDING_SHOUT = 469;
        constexpr uint32 BERSERKER_RAGE = 18499;

        // Utility
        constexpr uint32 TAUNT = 355;
        constexpr uint32 HEROIC_THROW = 57755;
        constexpr uint32 SHATTERING_THROW = 64382;
        constexpr uint32 VICTORY_RUSH = 34428;
    }
}

} // namespace WoW120Spells
} // namespace Playerbot