#pragma once

#include "Define.h"
#include "Unit.h"

#include "Followship_bots_defines.h"

struct FSB_ClassStats
{
    FSB_Class classId;

    Powers powerType;

    // Base stats at level 1 (important)
    uint32 baseHealth;
    uint32 basePower;

    // Per-level scaling (pre-mods)
    uint32 healthPerLevel;
    uint32 powerPerLevel;

    // Regen (flat, before auras/mods)
    int32 baseHpRegenOOC;
    int32 basePowerRegenOOC;

    // Regen (flat, before auras/mods)
    int32 baseHpRegenIC;
    int32 basePowerRegenIC;

    int32 baseAttackPower;
    int32 baseRangedAttackPower;
    int32 attackPowerPerLevel;

    float baseClassDamageVariance;

    int32 baseAttackTime;
    int32 baseRangedAttackTime;
};

static constexpr FSB_ClassStats BotClassStatsTable[] =
{
    // PALADIN
    {
        .classId = FSB_Class::Paladin,
        .powerType = POWER_MANA,
        .baseHealth = 140,    // base HP
        .basePower = 160,      // base power
        .healthPerLevel = 65,     // HP per level
        .powerPerLevel = 30,      // Power per level
        .baseHpRegenOOC = 3,      // HP regen %
        .basePowerRegenOOC = 2,       // Power regen %
        .baseHpRegenIC = 0,
        .basePowerRegenIC = 1,
        .baseAttackPower = 10,
        .baseRangedAttackPower = 0,
        .attackPowerPerLevel = 4,
        .baseClassDamageVariance = 0.9f,
        .baseAttackTime = 2000,
        .baseRangedAttackTime = 2000
    },

    // WARRIOR
    {
        .classId = FSB_Class::Warrior,
        .powerType = POWER_RAGE,
        .baseHealth = 150,    // base HP
        .basePower = 0,      // base power
        .healthPerLevel = 66,     // HP per level
        .powerPerLevel = 0,      // Power per level
        .baseHpRegenOOC = 5,      // HP regen %
        .basePowerRegenOOC = -2,       // Power regen %
        .baseHpRegenIC = 0,
        .basePowerRegenIC = 0,
        .baseAttackPower = 10,
        .baseRangedAttackPower = 0,
        .attackPowerPerLevel = 4,
        .baseClassDamageVariance = 0.9f,
        .baseAttackTime = 2000,
        .baseRangedAttackTime = 2000
    },

    // PRIEST
    {
        .classId = FSB_Class::Priest,
        .powerType = POWER_MANA,
        .baseHealth = 90,             // base HP
        .basePower = 180,            // base Power
        .healthPerLevel = 52,             // HP per level
        .powerPerLevel = 35,             // Power per level
        .baseHpRegenOOC = 3,              // HP regen %
        .basePowerRegenOOC = 4,              // Power regen %
        .baseHpRegenIC = 0,
        .basePowerRegenIC = 2,
        .baseAttackPower = 5,
        .baseRangedAttackPower = 0,
        .attackPowerPerLevel = 1,
        .baseClassDamageVariance = 0.2f,
        .baseAttackTime = 2500,
        .baseRangedAttackTime = 2500
    },

    // MAGE
    {
        .classId = FSB_Class::Mage,
        .powerType = POWER_MANA,
        .baseHealth = 80,
        .basePower = 200,
        .healthPerLevel = 50,
        .powerPerLevel = 40,
        .baseHpRegenOOC = 2,
        .basePowerRegenOOC = 6,
        .baseHpRegenIC = 0,
        .basePowerRegenIC = 2,
        .baseAttackPower = 5,
        .attackPowerPerLevel = 1,
        .baseClassDamageVariance = 0.2f,
        .baseAttackTime = 2500,
        .baseRangedAttackTime = 2500
    },

    // ROGUE
    {
        .classId = FSB_Class::Rogue,
        .powerType = POWER_ENERGY,
        .baseHealth = 100,
        .basePower = 100,
        .healthPerLevel = 22,
        .powerPerLevel = 10,
        .baseHpRegenOOC = 4,
        .basePowerRegenOOC = 10,
        .baseHpRegenIC = 0,
        .basePowerRegenIC = 2,
        .baseAttackPower = 60,
        .baseRangedAttackPower = 0,
        .attackPowerPerLevel = 10,
        .baseClassDamageVariance = 1.2f,
        .baseAttackTime = 1500,
        .baseRangedAttackTime = 2500
    },
};

struct FSBUtilsStatsMods
{
    // REGEN
    int32 flatHealthPerTick = 0;
    int32 flatManaPerTick = 0;
    int32 flatRagePerTick = 0;

    float pctHealthPerTick = 0;
    float pctManaPerTick = 0;

    


    // MAX STATS
    int32 flatMaxHealth = 0;
    int32 flatMaxMana = 0;

    float pctMaxHealthBonus = 0.0f;
    float pctMaxManaBonus = 0.0f;

    // Merge another mods struct into this one
    FSBUtilsStatsMods& operator+=(FSBUtilsStatsMods const& rhs)
    {
        flatHealthPerTick += rhs.flatHealthPerTick;
        flatManaPerTick += rhs.flatManaPerTick;
        flatRagePerTick += rhs.flatRagePerTick;

        pctHealthPerTick += rhs.pctHealthPerTick;
        pctManaPerTick += rhs.pctManaPerTick;

        pctMaxHealthBonus += rhs.pctMaxHealthBonus;
        pctMaxManaBonus += rhs.pctMaxManaBonus;

        flatMaxHealth += rhs.flatMaxHealth;
        pctMaxHealthBonus += rhs.pctMaxHealthBonus;

        return *this;
    }
};

inline FSBUtilsStatsMods operator+(FSBUtilsStatsMods lhs, FSBUtilsStatsMods const& rhs)
{
    lhs += rhs;
    return lhs;
}

namespace FSBUtilsStats
{
    FSB_ClassStats const* GetBotClassStats(FSB_Class botClass);
    void ApplyBotBaseClassStats(Creature* creature, FSB_Class botClass);
    void UpdateBotLevelDependantStats(Creature* bot);

    void ApplyBotAttackPower(Unit* unit);
    void ApplyBotDamage(Unit* unit);
    


    void RecalculateMods(Unit* unit/*, const FSBUtilsStatsMods& mods*/);

    /**
     * @brief Updates the bot's level to match the player's level and recalculates stats.
     *
     * @param bot The creature/bot whose level to update.
     * @param regenMods Optional pointer to regeneration modifiers, if used for stat recalculation.
     */
    void UpdateBotLevelToPlayer(Creature* bot, const FSBUtilsStatsMods& regenMods);

    Classes FSBToTCClass(FSB_Class botClass);
    
    float ApplyBotDamageTakenReduction(Creature* bot);
    float ApplyBotDamageDoneReduction(Creature* bot);
    

}

