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
};

static constexpr FSB_ClassStats BotClassStatsTable[] =
{
    // WARRIOR
    {
        .classId = FSB_Class::Warrior,
        .powerType = POWER_RAGE,
        .baseHealth = 120,    // base HP
        .basePower = 0,      // base power
        .healthPerLevel = 60,     // HP per level
        .powerPerLevel = 0,      // Power per level
        .baseHpRegenOOC = 5,      // HP regen %
        .basePowerRegenOOC = -2,       // Power regen %
        .baseHpRegenIC = 0,
        .basePowerRegenIC = 0,
        .baseAttackPower = 100,
        .baseRangedAttackPower = 50
    },

    // PRIEST
    {
        .classId = FSB_Class::Priest,
        .powerType = POWER_MANA,
        .baseHealth = 90,             // base HP
        .basePower = 160,            // base Power
        .healthPerLevel = 48,             // HP per level
        .powerPerLevel = 30,             // Power per level
        .baseHpRegenOOC = 3,              // HP regen %
        .basePowerRegenOOC = 4,              // Power regen %
        .baseHpRegenIC = 0,
        .basePowerRegenIC = 2,
        .baseAttackPower = 10,
        .baseRangedAttackPower = 0
    },

    // MAGE
    {
        .classId = FSB_Class::Mage,
        .powerType = POWER_MANA,
        .baseHealth = 80,
        .basePower = 200,
        .healthPerLevel = 45,
        .powerPerLevel = 35,
        .baseHpRegenOOC = 2,
        .basePowerRegenOOC = 6,
        .baseHpRegenIC = 0,
        .basePowerRegenIC = 2,
        .baseAttackPower = 10,
        .baseRangedAttackPower = 0
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
        .baseAttackPower = 80,
        .baseRangedAttackPower = 50
    },
};

struct FSBUtilsStatsMods
{
    // REGEN
    int32 flatHealthPerTick = 0;
    int32 flatManaPerTick = 0;

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

    // Full mod-aware version
    
    void ApplyMaxHealth(Unit* unit, const FSBUtilsStatsMods& mods);
    void ApplyMaxMana(Unit* unit, const FSBUtilsStatsMods& mods);
    


    void RecalculateMods(Unit* unit, const FSBUtilsStatsMods& mods);

    /**
     * @brief Updates the bot's level to match the player's level and recalculates stats.
     *
     * @param bot The creature/bot whose level to update.
     * @param regenMods Optional pointer to regeneration modifiers, if used for stat recalculation.
     */
    void UpdateBotLevelToPlayer(Creature* bot, const FSBUtilsStatsMods& regenMods);

    Classes FSBToTCClass(FSB_Class botClass);
    

    

}

