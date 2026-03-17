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
#include "SharedDefines.h"
#include <unordered_map>
#include <vector>
#include <string>

namespace Playerbot
{

// Threat ability categories
enum class ThreatAbilityType : uint8
{
    TAUNT = 0,              // Forces target to attack (Taunt, Dark Command)
    HIGH_THREAT = 1,        // Generates significant threat (Shield Slam, Revenge)
    THREAT_INCREASE = 2,    // Increases threat generation (Righteous Fury)
    THREAT_REDUCTION = 3,   // Reduces threat (Fade, Feint)
    THREAT_TRANSFER = 4,    // Transfers threat (Misdirection, Tricks of the Trade)
    THREAT_DROP = 5,        // Completely drops threat (Vanish, Feign Death)
    AOE_THREAT = 6          // Multi-target threat (Thunder Clap, Swipe)
};

// Individual threat ability data
struct ThreatAbilityData
{
    uint32 spellId;
    ThreatAbilityType type;
    float threatModifier;       // Multiplier or fixed threat value
    uint32 cooldownMs;
    uint32 durationMs;          // For buffs/debuffs
    uint32 resourceCost;        // Rage/Energy/Mana etc
    float range;
    bool requiresTarget;
    bool isPassive;
    uint32 minLevel;
    ::std::string name;

    ThreatAbilityData() = default;
    ThreatAbilityData(uint32 id, ThreatAbilityType t, float mod, uint32 cd,
                     uint32 dur = 0, uint32 cost = 0, float r = 5.0f,
                     bool target = true, bool passive = false, uint32 level = 1,
                     ::std::string const& n = "")
        : spellId(id), type(t), threatModifier(mod), cooldownMs(cd),
          durationMs(dur), resourceCost(cost), range(r), requiresTarget(target),
          isPassive(passive), minLevel(level), name(n) {}
};

// WoW 12.0 Threat Abilities Database
class TC_GAME_API ThreatAbilitiesDB
{
public:
    static ThreatAbilitiesDB& Instance()
    {
        static ThreatAbilitiesDB instance;
        return instance;
    }

    // Get abilities by class and specialization
    ::std::vector<ThreatAbilityData> GetClassAbilities(Classes playerClass, uint32 spec = 0) const;

    // Get specific ability data
    ThreatAbilityData const* GetAbility(uint32 spellId) const;

    // Get abilities by type
    ::std::vector<ThreatAbilityData> GetAbilitiesByType(ThreatAbilityType type) const;

private:
    ThreatAbilitiesDB() { Initialize(); }
    void Initialize();

    ::std::unordered_map<uint32, ThreatAbilityData> _abilities;
    ::std::unordered_map<Classes, ::std::vector<uint32>> _classAbilities;
};

// WoW 12.0 Spell IDs for threat abilities
namespace ThreatSpells
{
    // === WARRIOR ===
    // Protection
    constexpr uint32 TAUNT = 355;                    // Force target to attack
    constexpr uint32 SHIELD_SLAM = 23922;            // High threat attack
    constexpr uint32 REVENGE = 6572;                 // Counter-attack with high threat
    constexpr uint32 THUNDER_CLAP = 6343;            // AoE threat
    constexpr uint32 DEMORALIZING_SHOUT = 1160;      // AoE debuff threat
    constexpr uint32 CHALLENGING_SHOUT = 1161;       // AoE taunt
    constexpr uint32 IGNORE_PAIN = 190456;           // Absorb shield
    constexpr uint32 LAST_STAND = 12975;             // Health increase

    // Arms/Fury threat management
    constexpr uint32 INTIMIDATING_SHOUT = 5246;      // Fear nearby enemies
    constexpr uint32 BERSERKER_RAGE = 18499;         // Immunity and threat

    // === PALADIN ===
    // Protection
    constexpr uint32 HAND_OF_RECKONING = 62124;     // Taunt
    constexpr uint32 AVENGERS_SHIELD = 31935;        // High threat ranged
    constexpr uint32 HAMMER_OF_THE_RIGHTEOUS = 53595; // AoE threat
    constexpr uint32 CONSECRATION = 26573;           // Ground AoE threat
    constexpr uint32 JUDGMENT = 275779;              // Ranged threat
    constexpr uint32 BLESSED_HAMMER = 204019;        // Spinning hammers
    constexpr uint32 RIGHTEOUS_DEFENSE = 31789;      // Taunt up to 3

    // Holy/Retribution threat management
    constexpr uint32 DIVINE_SHIELD = 642;            // Complete immunity
    constexpr uint32 BLESSING_OF_PROTECTION = 1022;  // Physical immunity
    constexpr uint32 BLESSING_OF_SALVATION = 25895;  // Threat reduction (Classic)

    // === DEATH KNIGHT ===
    // Blood
    constexpr uint32 DARK_COMMAND = 56222;           // Taunt
    constexpr uint32 DEATH_AND_DECAY = 43265;        // Ground AoE threat
    constexpr uint32 BLOOD_BOIL = 50842;             // AoE threat
    constexpr uint32 MARROWREND = 195182;            // High threat, bone shield
    constexpr uint32 HEART_STRIKE = 206930;          // Cleave threat
    constexpr uint32 DEATHS_CARESS = 195292;         // Ranged threat
    constexpr uint32 GOREFIENDS_GRASP = 108199;      // Mass grip
    constexpr uint32 BLOODDRINKER = 206931;          // Channel threat

    // Frost/Unholy threat management
    constexpr uint32 DEATH_GRIP = 49576;             // Pull target
    constexpr uint32 CHAINS_OF_ICE = 45524;          // Slow and threat

    // === DEMON HUNTER ===
    // Vengeance
    constexpr uint32 TORMENT = 185245;               // Taunt
    constexpr uint32 IMMOLATION_AURA = 258920;       // AoE threat aura
    constexpr uint32 SIGIL_OF_FLAME = 204596;        // Ground AoE threat
    constexpr uint32 INFERNAL_STRIKE = 189110;       // Leap with threat
    constexpr uint32 THROW_GLAIVE = 204157;          // Ranged threat
    constexpr uint32 SIGIL_OF_SILENCE = 202137;      // AoE silence threat
    constexpr uint32 SIGIL_OF_CHAINS = 202138;       // AoE grip

    // Havoc threat management
    constexpr uint32 BLUR = 198589;                  // Dodge increase
    constexpr uint32 DARKNESS = 196718;              // AoE damage reduction

    // === MONK ===
    // Brewmaster
    constexpr uint32 PROVOKE = 115546;               // Taunt
    constexpr uint32 KEG_SMASH = 121253;             // AoE threat
    constexpr uint32 BREATH_OF_FIRE = 115181;        // Cone AoE threat
    constexpr uint32 RUSHING_JADE_WIND = 116847;     // Spinning AoE threat
    constexpr uint32 BLACK_OX_STATUE = 115315;       // Threat statue
    constexpr uint32 INVOKE_NIUZAO = 132578;         // Guardian pet threat

    // Windwalker/Mistweaver threat management
    constexpr uint32 PARALYSIS = 115078;             // Incapacitate
    constexpr uint32 DISABLE = 116095;               // Slow and root

    // === DRUID ===
    // Guardian
    constexpr uint32 GROWL = 6795;                   // Taunt
    constexpr uint32 SWIPE_BEAR = 213771;            // AoE threat
    constexpr uint32 THRASH_BEAR = 77758;            // AoE bleed threat
    constexpr uint32 MANGLE = 33917;                 // High threat attack
    constexpr uint32 MAUL = 6807;                    // Rage dump threat
    constexpr uint32 MOONFIRE_BEAR = 8921;           // Ranged threat DoT
    constexpr uint32 INCAPACITATING_ROAR = 99;       // AoE incapacitate
    constexpr uint32 CHALLENGING_ROAR = 5209;        // AoE taunt (removed in modern)

    // Other specs threat management
    constexpr uint32 BARKSKIN = 22812;               // Damage reduction
    constexpr uint32 DASH = 1850;                    // Speed increase

    // === ROGUE ===
    // Threat reduction
    constexpr uint32 VANISH = 1856;                  // Complete threat drop
    constexpr uint32 FEINT = 1966;                   // Threat reduction
    constexpr uint32 TRICKS_OF_THE_TRADE = 57934;    // Threat redirect
    constexpr uint32 EVASION = 5277;                 // Dodge increase
    constexpr uint32 CLOAK_OF_SHADOWS = 31224;       // Magic immunity
    constexpr uint32 SHADOWSTEP = 36554;             // Teleport behind

    // === HUNTER ===
    // Pet threat
    constexpr uint32 PET_GROWL = 2649;               // Pet taunt
    constexpr uint32 MISDIRECTION = 34477;           // Threat redirect
    constexpr uint32 FEIGN_DEATH = 5384;             // Complete threat drop
    constexpr uint32 DISENGAGE = 781;                // Leap back
    constexpr uint32 FREEZING_TRAP = 3355;           // CC trap
    constexpr uint32 TAR_TRAP = 187698;              // Slow trap
    constexpr uint32 BINDING_SHOT = 109248;          // Tether stun

    // === MAGE ===
    // Threat management
    constexpr uint32 INVISIBILITY = 66;              // Threat drop over time
    constexpr uint32 GREATER_INVISIBILITY = 110959;  // Instant threat reduction
    constexpr uint32 ICE_BLOCK = 45438;              // Complete immunity
    constexpr uint32 MIRROR_IMAGE = 55342;           // Threat copies
    constexpr uint32 FROST_NOVA = 122;               // Root
    constexpr uint32 RING_OF_FROST = 113724;         // AoE freeze
    constexpr uint32 DRAGONS_BREATH = 31661;         // Cone disorient

    // === WARLOCK ===
    // Pet threat
    constexpr uint32 VOIDWALKER_TAUNT = 17735;       // Voidwalker taunt
    constexpr uint32 SOULBURN = 74434;               // Instant threat for pet
    constexpr uint32 DARK_PACT = 108416;             // Sacrificial pact
    constexpr uint32 UNENDING_RESOLVE = 104773;      // Damage reduction
    constexpr uint32 MORTAL_COIL = 6789;             // Horror effect
    constexpr uint32 HOWL_OF_TERROR = 5484;          // AoE fear

    // === PRIEST ===
    // Threat reduction
    constexpr uint32 FADE = 586;                     // Temporary threat reduction
    constexpr uint32 SPECTRAL_GUISE = 108968;        // Threat transfer (removed)
    constexpr uint32 PSYCHIC_SCREAM = 8122;          // AoE fear
    constexpr uint32 MIND_CONTROL = 605;             // Take control
    constexpr uint32 SHACKLE_UNDEAD = 9484;          // Undead CC
    constexpr uint32 POWER_WORD_SHIELD = 17;         // Absorb shield

    // === SHAMAN ===
    // Threat management
    constexpr uint32 EARTH_ELEMENTAL = 198103;       // Tank pet summon
    constexpr uint32 EARTHBIND_TOTEM = 2484;         // Slow totem
    constexpr uint32 CAPACITOR_TOTEM = 192058;       // Stun totem
    constexpr uint32 WIND_SHEAR = 57994;             // Interrupt (generates threat)
    constexpr uint32 THUNDERSTORM = 51490;           // Knockback
    constexpr uint32 HEX = 51514;                    // Polymorph

    // === EVOKER ===
    // Threat management (12.0 specific)
    constexpr uint32 WING_BUFFET = 357214;           // Knockback
    constexpr uint32 TAIL_SWIPE = 368970;            // Knockback
    constexpr uint32 LANDSLIDE = 358385;             // Root
    constexpr uint32 QUELL = 351338;                 // Interrupt
    constexpr uint32 BLESSING_OF_THE_BRONZE = 381748; // Threat reduction buff
}

// Hero Talent threat modifiers (12.0)
namespace HeroTalentThreat
{
    // Death Knight
    constexpr float DEATHBRINGER_THREAT_MOD = 1.15f;   // Blood DK hero talent
    constexpr float SANLAYN_THREAT_MOD = 1.10f;        // Vampiric blood theme

    // Demon Hunter
    constexpr float ALDRACHI_REAVER_THREAT_MOD = 1.20f; // Vengeance hero talent
    constexpr float FEL_SCARRED_THREAT_MOD = 1.15f;     // Metamorphosis focus

    // Warrior
    constexpr float MOUNTAIN_THANE_THREAT_MOD = 1.25f;  // Thunder focus
    constexpr float COLOSSUS_THREAT_MOD = 1.30f;        // Size and intimidation

    // Paladin
    constexpr float LIGHTSMITH_THREAT_MOD = 1.10f;      // Holy weapon enhancement
    constexpr float TEMPLAR_THREAT_MOD = 1.20f;         // Defensive stance

    // Druid
    constexpr float DRUID_OF_THE_CLAW_THREAT_MOD = 1.25f; // Enhanced bear form
    constexpr float ELUNES_CHOSEN_THREAT_MOD = 0.90f;   // Moonkin threat reduction

    // Monk
    constexpr float MASTER_OF_HARMONY_THREAT_MOD = 1.15f; // Balanced threat
    constexpr float SHADO_PAN_THREAT_MOD = 1.20f;       // Defensive mastery
}

// Mythic+ scaling for threat (12.0)
namespace MythicPlusThreat
{
    constexpr float BASE_MYTHIC_THREAT_SCALAR = 1.0f;
    constexpr float THREAT_SCALAR_PER_LEVEL = 0.08f;    // 8% per M+ level

    inline float GetMythicThreatScalar(uint32 mythicLevel)
    {
        return BASE_MYTHIC_THREAT_SCALAR * (1.0f + (THREAT_SCALAR_PER_LEVEL * mythicLevel));
    }
}

} // namespace Playerbot