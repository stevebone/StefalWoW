/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "ThreatAbilities.h"

namespace Playerbot
{

void ThreatAbilitiesDB::Initialize()
{
    using namespace ThreatSpells;

    // === WARRIOR ABILITIES ===
    // Tank abilities
    _abilities[TAUNT] = ThreatAbilityData(TAUNT, ThreatAbilityType::TAUNT, 10000.0f, 8000, 3000, 0, 30.0f, true, false, 1, "Taunt");
    _abilities[SHIELD_SLAM] = ThreatAbilityData(SHIELD_SLAM, ThreatAbilityType::HIGH_THREAT, 3.0f, 9000, 0, 15, 5.0f, true, false, 15, "Shield Slam");
    _abilities[REVENGE] = ThreatAbilityData(REVENGE, ThreatAbilityType::HIGH_THREAT, 2.5f, 0, 0, 0, 5.0f, false, false, 14, "Revenge");
    _abilities[THUNDER_CLAP] = ThreatAbilityData(THUNDER_CLAP, ThreatAbilityType::AOE_THREAT, 1.75f, 6000, 0, 5, 8.0f, false, false, 11, "Thunder Clap");
    _abilities[DEMORALIZING_SHOUT] = ThreatAbilityData(DEMORALIZING_SHOUT, ThreatAbilityType::AOE_THREAT, 1.5f, 0, 8000, 10, 10.0f, false, false, 22, "Demoralizing Shout");
    _abilities[CHALLENGING_SHOUT] = ThreatAbilityData(CHALLENGING_SHOUT, ThreatAbilityType::TAUNT, 5000.0f, 180000, 6000, 0, 10.0f, false, false, 40, "Challenging Shout");
    _abilities[IGNORE_PAIN] = ThreatAbilityData(IGNORE_PAIN, ThreatAbilityType::HIGH_THREAT, 1.5f, 0, 0, 40, 0.0f, false, false, 35, "Ignore Pain");

    // DPS threat management
    _abilities[INTIMIDATING_SHOUT] = ThreatAbilityData(INTIMIDATING_SHOUT, ThreatAbilityType::THREAT_REDUCTION, 0.5f, 90000, 8000, 0, 8.0f, false, false, 52, "Intimidating Shout");

    // === PALADIN ABILITIES ===
    // Tank abilities
    _abilities[HAND_OF_RECKONING] = ThreatAbilityData(HAND_OF_RECKONING, ThreatAbilityType::TAUNT, 10000.0f, 8000, 3000, 0, 30.0f, true, false, 14, "Hand of Reckoning");
    _abilities[AVENGERS_SHIELD] = ThreatAbilityData(AVENGERS_SHIELD, ThreatAbilityType::HIGH_THREAT, 4.0f, 15000, 0, 0, 30.0f, true, false, 50, "Avenger's Shield");
    _abilities[HAMMER_OF_THE_RIGHTEOUS] = ThreatAbilityData(HAMMER_OF_THE_RIGHTEOUS, ThreatAbilityType::AOE_THREAT, 2.0f, 0, 0, 0, 5.0f, true, false, 20, "Hammer of the Righteous");
    _abilities[CONSECRATION] = ThreatAbilityData(CONSECRATION, ThreatAbilityType::AOE_THREAT, 1.5f, 4500, 12000, 0, 8.0f, false, false, 32, "Consecration");
    _abilities[JUDGMENT] = ThreatAbilityData(JUDGMENT, ThreatAbilityType::HIGH_THREAT, 2.0f, 6000, 0, 0, 30.0f, true, false, 3, "Judgment");
    _abilities[BLESSED_HAMMER] = ThreatAbilityData(BLESSED_HAMMER, ThreatAbilityType::AOE_THREAT, 1.75f, 0, 0, 0, 10.0f, false, false, 60, "Blessed Hammer");

    // Threat reduction
    _abilities[DIVINE_SHIELD] = ThreatAbilityData(DIVINE_SHIELD, ThreatAbilityType::THREAT_DROP, 0.0f, 300000, 8000, 0, 0.0f, false, false, 38, "Divine Shield");
    _abilities[BLESSING_OF_PROTECTION] = ThreatAbilityData(BLESSING_OF_PROTECTION, ThreatAbilityType::THREAT_REDUCTION, 0.0f, 300000, 10000, 0, 40.0f, true, false, 18, "Blessing of Protection");

    // === DEATH KNIGHT ABILITIES ===
    // Tank abilities
    _abilities[DARK_COMMAND] = ThreatAbilityData(DARK_COMMAND, ThreatAbilityType::TAUNT, 10000.0f, 8000, 3000, 0, 30.0f, true, false, 55, "Dark Command");
    _abilities[DEATH_AND_DECAY] = ThreatAbilityData(DEATH_AND_DECAY, ThreatAbilityType::AOE_THREAT, 2.0f, 30000, 10000, 1, 30.0f, false, false, 56, "Death and Decay");
    _abilities[BLOOD_BOIL] = ThreatAbilityData(BLOOD_BOIL, ThreatAbilityType::AOE_THREAT, 2.5f, 0, 0, 1, 10.0f, false, false, 56, "Blood Boil");
    _abilities[MARROWREND] = ThreatAbilityData(MARROWREND, ThreatAbilityType::HIGH_THREAT, 3.0f, 0, 0, 2, 5.0f, true, false, 55, "Marrowrend");
    _abilities[HEART_STRIKE] = ThreatAbilityData(HEART_STRIKE, ThreatAbilityType::HIGH_THREAT, 2.5f, 0, 0, 1, 5.0f, true, false, 55, "Heart Strike");
    _abilities[DEATHS_CARESS] = ThreatAbilityData(DEATHS_CARESS, ThreatAbilityType::HIGH_THREAT, 2.0f, 0, 0, 1, 30.0f, true, false, 56, "Death's Caress");
    _abilities[GOREFIENDS_GRASP] = ThreatAbilityData(GOREFIENDS_GRASP, ThreatAbilityType::HIGH_THREAT, 3.0f, 120000, 0, 0, 20.0f, false, false, 60, "Gorefiend's Grasp");

    // Utility
    _abilities[DEATH_GRIP] = ThreatAbilityData(DEATH_GRIP, ThreatAbilityType::HIGH_THREAT, 2.0f, 25000, 0, 0, 30.0f, true, false, 55, "Death Grip");

    // === DEMON HUNTER ABILITIES ===
    // Tank abilities
    _abilities[TORMENT] = ThreatAbilityData(TORMENT, ThreatAbilityType::TAUNT, 10000.0f, 8000, 3000, 0, 30.0f, true, false, 98, "Torment");
    _abilities[IMMOLATION_AURA] = ThreatAbilityData(IMMOLATION_AURA, ThreatAbilityType::AOE_THREAT, 2.0f, 0, 6000, 30, 8.0f, false, false, 99, "Immolation Aura");
    _abilities[SIGIL_OF_FLAME] = ThreatAbilityData(SIGIL_OF_FLAME, ThreatAbilityType::AOE_THREAT, 2.5f, 30000, 0, 0, 30.0f, false, false, 100, "Sigil of Flame");
    _abilities[INFERNAL_STRIKE] = ThreatAbilityData(INFERNAL_STRIKE, ThreatAbilityType::AOE_THREAT, 1.5f, 20000, 0, 0, 30.0f, false, false, 98, "Infernal Strike");
    _abilities[THROW_GLAIVE] = ThreatAbilityData(THROW_GLAIVE, ThreatAbilityType::HIGH_THREAT, 2.0f, 9000, 0, 0, 30.0f, true, false, 99, "Throw Glaive");

    // Threat reduction
    _abilities[BLUR] = ThreatAbilityData(BLUR, ThreatAbilityType::THREAT_REDUCTION, 0.8f, 60000, 10000, 0, 0.0f, false, false, 98, "Blur");

    // === MONK ABILITIES ===
    // Tank abilities
    _abilities[PROVOKE] = ThreatAbilityData(PROVOKE, ThreatAbilityType::TAUNT, 10000.0f, 8000, 3000, 0, 30.0f, true, false, 12, "Provoke");
    _abilities[KEG_SMASH] = ThreatAbilityData(KEG_SMASH, ThreatAbilityType::AOE_THREAT, 3.0f, 8000, 0, 40, 15.0f, false, false, 21, "Keg Smash");
    _abilities[BREATH_OF_FIRE] = ThreatAbilityData(BREATH_OF_FIRE, ThreatAbilityType::AOE_THREAT, 2.0f, 15000, 0, 0, 12.0f, false, false, 18, "Breath of Fire");
    _abilities[RUSHING_JADE_WIND] = ThreatAbilityData(RUSHING_JADE_WIND, ThreatAbilityType::AOE_THREAT, 1.5f, 6000, 6000, 0, 8.0f, false, false, 50, "Rushing Jade Wind");
    _abilities[BLACK_OX_STATUE] = ThreatAbilityData(BLACK_OX_STATUE, ThreatAbilityType::TAUNT, 5000.0f, 0, 900000, 0, 40.0f, false, false, 35, "Black Ox Statue");

    // === DRUID ABILITIES ===
    // Tank abilities
    _abilities[GROWL] = ThreatAbilityData(GROWL, ThreatAbilityType::TAUNT, 10000.0f, 8000, 3000, 0, 30.0f, true, false, 15, "Growl");
    _abilities[SWIPE_BEAR] = ThreatAbilityData(SWIPE_BEAR, ThreatAbilityType::AOE_THREAT, 2.0f, 0, 0, 0, 8.0f, false, false, 16, "Swipe");
    _abilities[THRASH_BEAR] = ThreatAbilityData(THRASH_BEAR, ThreatAbilityType::AOE_THREAT, 2.5f, 6000, 0, 0, 8.0f, false, false, 14, "Thrash");
    _abilities[MANGLE] = ThreatAbilityData(MANGLE, ThreatAbilityType::HIGH_THREAT, 3.0f, 6000, 0, 0, 5.0f, true, false, 8, "Mangle");
    _abilities[MAUL] = ThreatAbilityData(MAUL, ThreatAbilityType::HIGH_THREAT, 2.0f, 0, 0, 30, 5.0f, true, false, 15, "Maul");
    _abilities[MOONFIRE_BEAR] = ThreatAbilityData(MOONFIRE_BEAR, ThreatAbilityType::HIGH_THREAT, 1.5f, 0, 0, 0, 40.0f, true, false, 6, "Moonfire");
    _abilities[INCAPACITATING_ROAR] = ThreatAbilityData(INCAPACITATING_ROAR, ThreatAbilityType::AOE_THREAT, 1.0f, 30000, 3000, 0, 10.0f, false, false, 28, "Incapacitating Roar");

    // Damage reduction
    _abilities[BARKSKIN] = ThreatAbilityData(BARKSKIN, ThreatAbilityType::THREAT_REDUCTION, 0.9f, 60000, 12000, 0, 0.0f, false, false, 24, "Barkskin");

    // === ROGUE ABILITIES ===
    // Threat management
    _abilities[VANISH] = ThreatAbilityData(VANISH, ThreatAbilityType::THREAT_DROP, 0.0f, 120000, 3000, 0, 0.0f, false, false, 48, "Vanish");
    _abilities[FEINT] = ThreatAbilityData(FEINT, ThreatAbilityType::THREAT_REDUCTION, 0.5f, 0, 5000, 35, 0.0f, false, false, 28, "Feint");
    _abilities[TRICKS_OF_THE_TRADE] = ThreatAbilityData(TRICKS_OF_THE_TRADE, ThreatAbilityType::THREAT_TRANSFER, 1.0f, 30000, 6000, 0, 20.0f, true, false, 75, "Tricks of the Trade");
    _abilities[EVASION] = ThreatAbilityData(EVASION, ThreatAbilityType::THREAT_REDUCTION, 0.8f, 120000, 10000, 0, 0.0f, false, false, 8, "Evasion");
    _abilities[CLOAK_OF_SHADOWS] = ThreatAbilityData(CLOAK_OF_SHADOWS, ThreatAbilityType::THREAT_REDUCTION, 0.7f, 120000, 5000, 0, 0.0f, false, false, 58, "Cloak of Shadows");

    // === HUNTER ABILITIES ===
    // Pet and threat management
    _abilities[PET_GROWL] = ThreatAbilityData(PET_GROWL, ThreatAbilityType::TAUNT, 10000.0f, 8000, 3000, 0, 30.0f, true, false, 10, "Growl (Pet)");
    _abilities[MISDIRECTION] = ThreatAbilityData(MISDIRECTION, ThreatAbilityType::THREAT_TRANSFER, 1.0f, 30000, 8000, 0, 40.0f, true, false, 42, "Misdirection");
    _abilities[FEIGN_DEATH] = ThreatAbilityData(FEIGN_DEATH, ThreatAbilityType::THREAT_DROP, 0.0f, 30000, 6000, 0, 0.0f, false, false, 28, "Feign Death");
    _abilities[DISENGAGE] = ThreatAbilityData(DISENGAGE, ThreatAbilityType::THREAT_REDUCTION, 0.9f, 20000, 0, 0, 0.0f, false, false, 14, "Disengage");

    // === MAGE ABILITIES ===
    // Threat management
    _abilities[INVISIBILITY] = ThreatAbilityData(INVISIBILITY, ThreatAbilityType::THREAT_DROP, 0.0f, 300000, 3000, 0, 0.0f, false, false, 56, "Invisibility");
    _abilities[GREATER_INVISIBILITY] = ThreatAbilityData(GREATER_INVISIBILITY, ThreatAbilityType::THREAT_REDUCTION, 0.1f, 120000, 20000, 0, 0.0f, false, false, 60, "Greater Invisibility");
    _abilities[ICE_BLOCK] = ThreatAbilityData(ICE_BLOCK, ThreatAbilityType::THREAT_DROP, 0.0f, 240000, 10000, 0, 0.0f, false, false, 30, "Ice Block");
    _abilities[MIRROR_IMAGE] = ThreatAbilityData(MIRROR_IMAGE, ThreatAbilityType::THREAT_TRANSFER, 0.5f, 120000, 40000, 0, 0.0f, false, false, 50, "Mirror Image");

    // === WARLOCK ABILITIES ===
    // Pet threat
    _abilities[VOIDWALKER_TAUNT] = ThreatAbilityData(VOIDWALKER_TAUNT, ThreatAbilityType::TAUNT, 10000.0f, 8000, 3000, 0, 30.0f, true, false, 10, "Suffering");
    _abilities[SOULBURN] = ThreatAbilityData(SOULBURN, ThreatAbilityType::THREAT_INCREASE, 2.0f, 0, 0, 1, 0.0f, false, false, 19, "Soulburn");
    _abilities[UNENDING_RESOLVE] = ThreatAbilityData(UNENDING_RESOLVE, ThreatAbilityType::THREAT_REDUCTION, 0.9f, 180000, 8000, 0, 0.0f, false, false, 49, "Unending Resolve");

    // === PRIEST ABILITIES ===
    // Threat reduction
    _abilities[FADE] = ThreatAbilityData(FADE, ThreatAbilityType::THREAT_REDUCTION, 0.1f, 30000, 10000, 0, 0.0f, false, false, 8, "Fade");
    _abilities[PSYCHIC_SCREAM] = ThreatAbilityData(PSYCHIC_SCREAM, ThreatAbilityType::THREAT_REDUCTION, 0.8f, 60000, 8000, 0, 8.0f, false, false, 12, "Psychic Scream");
    _abilities[POWER_WORD_SHIELD] = ThreatAbilityData(POWER_WORD_SHIELD, ThreatAbilityType::THREAT_REDUCTION, 0.5f, 0, 0, 0, 40.0f, true, false, 4, "Power Word: Shield");

    // === SHAMAN ABILITIES ===
    // Threat management
    _abilities[EARTH_ELEMENTAL] = ThreatAbilityData(EARTH_ELEMENTAL, ThreatAbilityType::TAUNT, 5000.0f, 300000, 60000, 0, 0.0f, false, false, 58, "Earth Elemental");
    _abilities[WIND_SHEAR] = ThreatAbilityData(WIND_SHEAR, ThreatAbilityType::HIGH_THREAT, 1.5f, 12000, 0, 0, 30.0f, true, false, 12, "Wind Shear");
    _abilities[THUNDERSTORM] = ThreatAbilityData(THUNDERSTORM, ThreatAbilityType::AOE_THREAT, 1.0f, 45000, 0, 0, 10.0f, false, false, 32, "Thunderstorm");

    // === EVOKER ABILITIES ===
    // Threat management (12.0 specific)
    _abilities[WING_BUFFET] = ThreatAbilityData(WING_BUFFET, ThreatAbilityType::AOE_THREAT, 1.5f, 90000, 0, 0, 8.0f, false, false, 58, "Wing Buffet");
    _abilities[TAIL_SWIPE] = ThreatAbilityData(TAIL_SWIPE, ThreatAbilityType::AOE_THREAT, 1.5f, 90000, 0, 0, 8.0f, false, false, 58, "Tail Swipe");
    _abilities[LANDSLIDE] = ThreatAbilityData(LANDSLIDE, ThreatAbilityType::HIGH_THREAT, 2.0f, 90000, 0, 0, 30.0f, false, false, 58, "Landslide");
    _abilities[QUELL] = ThreatAbilityData(QUELL, ThreatAbilityType::HIGH_THREAT, 1.5f, 25000, 0, 0, 25.0f, true, false, 58, "Quell");
    _abilities[BLESSING_OF_THE_BRONZE] = ThreatAbilityData(BLESSING_OF_THE_BRONZE, ThreatAbilityType::THREAT_REDUCTION, 0.85f, 15000, 0, 0, 40.0f, true, false, 60, "Blessing of the Bronze");

    // === Build class ability maps ===

    // Warrior
    _classAbilities[CLASS_WARRIOR] = { TAUNT, SHIELD_SLAM, REVENGE, THUNDER_CLAP, DEMORALIZING_SHOUT,
                                       CHALLENGING_SHOUT, IGNORE_PAIN, INTIMIDATING_SHOUT };

    // Paladin
    _classAbilities[CLASS_PALADIN] = { HAND_OF_RECKONING, AVENGERS_SHIELD, HAMMER_OF_THE_RIGHTEOUS,
                                       CONSECRATION, JUDGMENT, BLESSED_HAMMER, DIVINE_SHIELD, BLESSING_OF_PROTECTION };

    // Death Knight
    _classAbilities[CLASS_DEATH_KNIGHT] = { DARK_COMMAND, DEATH_AND_DECAY, BLOOD_BOIL, MARROWREND,
                                            HEART_STRIKE, DEATHS_CARESS, GOREFIENDS_GRASP, DEATH_GRIP };

    // Demon Hunter
    _classAbilities[CLASS_DEMON_HUNTER] = { TORMENT, IMMOLATION_AURA, SIGIL_OF_FLAME, INFERNAL_STRIKE,
                                            THROW_GLAIVE, BLUR };

    // Monk
    _classAbilities[CLASS_MONK] = { PROVOKE, KEG_SMASH, BREATH_OF_FIRE, RUSHING_JADE_WIND, BLACK_OX_STATUE };

    // Druid
    _classAbilities[CLASS_DRUID] = { GROWL, SWIPE_BEAR, THRASH_BEAR, MANGLE, MAUL, MOONFIRE_BEAR,
                                     INCAPACITATING_ROAR, BARKSKIN };

    // Rogue
    _classAbilities[CLASS_ROGUE] = { VANISH, FEINT, TRICKS_OF_THE_TRADE, EVASION, CLOAK_OF_SHADOWS };

    // Hunter
    _classAbilities[CLASS_HUNTER] = { PET_GROWL, MISDIRECTION, FEIGN_DEATH, DISENGAGE };

    // Mage
    _classAbilities[CLASS_MAGE] = { INVISIBILITY, GREATER_INVISIBILITY, ICE_BLOCK, MIRROR_IMAGE };

    // Warlock
    _classAbilities[CLASS_WARLOCK] = { VOIDWALKER_TAUNT, SOULBURN, UNENDING_RESOLVE };

    // Priest
    _classAbilities[CLASS_PRIEST] = { FADE, PSYCHIC_SCREAM, POWER_WORD_SHIELD };

    // Shaman
    _classAbilities[CLASS_SHAMAN] = { EARTH_ELEMENTAL, WIND_SHEAR, THUNDERSTORM };

    // Evoker
    _classAbilities[CLASS_EVOKER] = { WING_BUFFET, TAIL_SWIPE, LANDSLIDE, QUELL, BLESSING_OF_THE_BRONZE };
}

::std::vector<ThreatAbilityData> ThreatAbilitiesDB::GetClassAbilities(Classes playerClass, uint32 spec) const
{
    ::std::vector<ThreatAbilityData> result;

    auto it = _classAbilities.find(playerClass);
    if (it != _classAbilities.end())
    {
        for (uint32 spellId : it->second)
        {
            auto abilityIt = _abilities.find(spellId);
            if (abilityIt != _abilities.end())
            {
                result.push_back(abilityIt->second);
            }
        }
    }

    return result;
}

ThreatAbilityData const* ThreatAbilitiesDB::GetAbility(uint32 spellId) const
{
    auto it = _abilities.find(spellId);
    return it != _abilities.end() ? &it->second : nullptr;
}

::std::vector<ThreatAbilityData> ThreatAbilitiesDB::GetAbilitiesByType(ThreatAbilityType type) const
{
    ::std::vector<ThreatAbilityData> result;

    for (const auto& [spellId, ability] : _abilities)
    {
        if (ability.type == type)
        {
            result.push_back(ability);
        }
    }

    return result;
}

} // namespace Playerbot