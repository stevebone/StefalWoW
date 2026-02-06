#pragma once

#include "Followship_bots_utils_spells.h"

extern std::vector<FSBSpellDefinition> PaladinSpellsTable;

enum FSB_PALADIN_SELF_BUFFS
{
    // HEALER
    SPELL_PALADIN_RITE_OF_SANCTIFICATION = 433550,

    // TANK
    SPELL_PALADIN_FURY = 187218
};

enum FSB_PALADIN_COMBAT_SPELLS
{
    // HEALER
    SPELL_PALADIN_HOLY_LIGHT = 295698, //82326,
    SPELL_PALADIN_HOLY_SHOCK = 36340, //20473,

    SPELL_PALADIN_REDEMPTION = 7328,

    // DPS
    SPELL_PALADIN_CRUSADER_STRIKE = 35395,
    SPELL_PALADIN_JUDGEMENT = 20271,
    SPELL_PALADIN_CONSECRATION = 26573,

    SPELL_PALADIN_DIVINE_STORM = 79970,

    SPELL_PALADIN_AVENGING_WRATH = 231895,

    // PROTECTION
    SPELL_PALADIN_AVENGER_SHIELD = 31935,

    // ANY
    SPELL_PALADIN_HAMMER_OF_JUSTICE = 853,
    SPELL_PALADIN_HAND_OF_RECKONING = 62124,

    SPELL_PALADIN_BLESSING_PROTECTION = 1022,
    SPELL_PALADIN_FLASH_OF_LIGHT = 57766,

    SPELL_PALADIN_DIVINE_SHIELD = 642
};

enum FSB_PALADIN_AURAS
{
    SPELL_PALADIN_DEVOTION_AURA = 465,
    SPELL_PALADIN_RETRIBUTION_AURA = 8990,
    SPELL_PALADIN_CONCENTRATION_AURA = 79963
};

static const uint32 paladinAurasToRemoveForHealer[] =
{
    SPELL_PALADIN_DEVOTION_AURA,
    SPELL_PALADIN_RETRIBUTION_AURA,
    SPELL_PALADIN_FURY
};

static const uint32 paladinAurasToRemoveForDPS[] =
{
    SPELL_PALADIN_DEVOTION_AURA,
    SPELL_PALADIN_CONCENTRATION_AURA,
    SPELL_PALADIN_RITE_OF_SANCTIFICATION,
    SPELL_PALADIN_FURY
};

static const uint32 paladinAurasToRemoveForTank[] =
{
    SPELL_PALADIN_CONCENTRATION_AURA,
    SPELL_PALADIN_RETRIBUTION_AURA,
    SPELL_PALADIN_RITE_OF_SANCTIFICATION
};

namespace FSBPaladin
{
    bool BotHasDevotionAura(Creature* bot);

    void HandleOnSpellCast(Creature* bot, uint32 spellId);

    bool BotOOCHealOwner(Creature* bot, Player* player, uint32& globalCooldown);
    bool BotOOCBuffSelf(Creature* bot, uint32& globalCooldown, uint32& selfBuffTimer, uint32& outSpellId);

    void BotSetRoleAuras(Creature* bot, FSB_Roles role);
}
