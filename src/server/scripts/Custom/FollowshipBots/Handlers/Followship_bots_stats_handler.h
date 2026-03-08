#include "../Followship_bots_defines.h"

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

    int32 baseSpellPower;
    int32 spellPowerPerLevel;

    int32 armorPerLevel;
};

static constexpr FSB_ClassStats BotClassStatsTable[] =
{
    // PALADIN
    {
        .classId = FSB_Class::Paladin,
        .powerType = POWER_MANA,
        .baseHealth = 60,    // base HP
        .basePower = 600,      // base power
        .healthPerLevel = 10,     // HP per level
        .powerPerLevel = 35,      // Power per level
        .baseHpRegenOOC = 3,      // HP regen %
        .basePowerRegenOOC = 2,       // Power regen %
        .baseHpRegenIC = 0,
        .basePowerRegenIC = 1,
        .baseAttackPower = 1,
        .baseRangedAttackPower = 0,
        .attackPowerPerLevel = 4,
        .baseClassDamageVariance = 0.9f,
        .baseAttackTime = 2000,
        .baseRangedAttackTime = 2000,
        .baseSpellPower = 8,
        .spellPowerPerLevel = 2,
        .armorPerLevel = 4
    },

    // WARRIOR
    {
        .classId = FSB_Class::Warrior,
        .powerType = POWER_RAGE,
        .baseHealth = 90,    // base HP
        .basePower = 0,      // base power
        .healthPerLevel = 10,     // HP per level
        .powerPerLevel = 0,      // Power per level
        .baseHpRegenOOC = 5,      // HP regen %
        .basePowerRegenOOC = -2,       // Power regen %
        .baseHpRegenIC = 0,
        .basePowerRegenIC = 0,
        .baseAttackPower = 1,
        .baseRangedAttackPower = 0,
        .attackPowerPerLevel = 4,
        .baseClassDamageVariance = 0.9f,
        .baseAttackTime = 2000,
        .baseRangedAttackTime = 2000,
        .baseSpellPower = 2,
        .spellPowerPerLevel = 1,
        .armorPerLevel = 4
    },

    // PRIEST
    {
        .classId = FSB_Class::Priest,
        .powerType = POWER_MANA,
        .baseHealth = 50,             // base HP
        .basePower = 180,            // base Power
        .healthPerLevel = 52,             // HP per level
        .powerPerLevel = 35,             // Power per level
        .baseHpRegenOOC = 3,              // HP regen %
        .basePowerRegenOOC = 4,              // Power regen %
        .baseHpRegenIC = 0,
        .basePowerRegenIC = 2,
        .baseAttackPower = 1,
        .baseRangedAttackPower = 0,
        .attackPowerPerLevel = 1,
        .baseClassDamageVariance = 0.2f,
        .baseAttackTime = 3500,
        .baseRangedAttackTime = 2500,
        .baseSpellPower = 8,
        .spellPowerPerLevel = 3,
        .armorPerLevel = 1
    },

    // WARLOCK
    {
        .classId = FSB_Class::Warlock,
        .powerType = POWER_MANA,
        .baseHealth = 50,             // base HP
        .basePower = 180,            // base Power
        .healthPerLevel = 52,             // HP per level
        .powerPerLevel = 35,             // Power per level
        .baseHpRegenOOC = 3,              // HP regen %
        .basePowerRegenOOC = 4,              // Power regen %
        .baseHpRegenIC = 0,
        .basePowerRegenIC = 2,
        .baseAttackPower = 1,
        .baseRangedAttackPower = 0,
        .attackPowerPerLevel = 1,
        .baseClassDamageVariance = 0.2f,
        .baseAttackTime = 3500,
        .baseRangedAttackTime = 2500,
        .baseSpellPower = 9,
        .spellPowerPerLevel = 4,
        .armorPerLevel = 1
    },

    // MAGE
    {
        .classId = FSB_Class::Mage,
        .powerType = POWER_MANA,
        .baseHealth = 50,
        .basePower = 200,
        .healthPerLevel = 50,
        .powerPerLevel = 40,
        .baseHpRegenOOC = 2,
        .basePowerRegenOOC = 6,
        .baseHpRegenIC = 0,
        .basePowerRegenIC = 2,
        .baseAttackPower = 1,
        .attackPowerPerLevel = 1,
        .baseClassDamageVariance = 0.2f,
        .baseAttackTime = 3500,
        .baseRangedAttackTime = 2500,
        .baseSpellPower = 10,
        .spellPowerPerLevel = 5,
        .armorPerLevel = 1
    },

    // MONK
    {
        .classId = FSB_Class::Monk,
        .powerType = POWER_MANA,
        .baseHealth = 60,
        .basePower = 180,
        .healthPerLevel = 52,
        .powerPerLevel = 32,
        .baseHpRegenOOC = 3,
        .basePowerRegenOOC = 5,
        .baseHpRegenIC = 0,
        .basePowerRegenIC = 2,
        .baseAttackPower = 1,
        .baseRangedAttackPower = 0,
        .attackPowerPerLevel = 3,
        .baseClassDamageVariance = 1.f,
        .baseAttackTime = 2000,
        .baseRangedAttackTime = 2500,
        .baseSpellPower = 8,
        .spellPowerPerLevel = 3,
        .armorPerLevel = 2
    },

    // DRUID
    {
        .classId = FSB_Class::Druid,
        .powerType = POWER_MANA,
        .baseHealth = 60,
        .basePower = 180,
        .healthPerLevel = 52,
        .powerPerLevel = 32,
        .baseHpRegenOOC = 3,
        .basePowerRegenOOC = 5,
        .baseHpRegenIC = 0,
        .basePowerRegenIC = 2,
        .baseAttackPower = 1,
        .baseRangedAttackPower = 0,
        .attackPowerPerLevel = 3,
        .baseClassDamageVariance = 0.7f,
        .baseAttackTime = 2000,
        .baseRangedAttackTime = 2500,
        .baseSpellPower = 8,
        .spellPowerPerLevel = 3,
        .armorPerLevel = 2
    },

    // ROGUE
    {
        .classId = FSB_Class::Rogue,
        .powerType = POWER_ENERGY,
        .baseHealth = 60,
        .basePower = 100,
        .healthPerLevel = 52,
        .powerPerLevel = 0,
        .baseHpRegenOOC = 3,
        .basePowerRegenOOC = 10,
        .baseHpRegenIC = 0,
        .basePowerRegenIC = 10,
        .baseAttackPower = 1, //10,
        .baseRangedAttackPower = 0,
        .attackPowerPerLevel = 5,
        .baseClassDamageVariance = 1.2f,
        .baseAttackTime = 1500,
        .baseRangedAttackTime = 2500,
        .baseSpellPower = 1,
        .spellPowerPerLevel = 1,
        .armorPerLevel = 2
    },

    // HUNTER
    {
        .classId = FSB_Class::Hunter,
        .powerType = POWER_FOCUS,
        .baseHealth = 80,
        .basePower = 100,
        .healthPerLevel = 52,
        .powerPerLevel = 0,
        .baseHpRegenOOC = 3,
        .basePowerRegenOOC = 5,
        .baseHpRegenIC = 0,
        .basePowerRegenIC = 5,
        .baseAttackPower = 1,
        .baseRangedAttackPower = 10,
        .attackPowerPerLevel = 4,
        .baseClassDamageVariance = 1.f,
        .baseAttackTime = 2500,
        .baseRangedAttackTime = 2000,
        .baseSpellPower = 1,
        .spellPowerPerLevel = 1,
        .armorPerLevel = 3
    },

    // SHAMAN
    {
        .classId = FSB_Class::Shaman,
        .powerType = POWER_MANA,
        .baseHealth = 50,    // base HP
        .basePower = 160,      // base power
        .healthPerLevel = 65,     // HP per level
        .powerPerLevel = 30,      // Power per level
        .baseHpRegenOOC = 3,      // HP regen %
        .basePowerRegenOOC = 2,       // Power regen %
        .baseHpRegenIC = 0,
        .basePowerRegenIC = 1,
        .baseAttackPower = 1, //5,
        .baseRangedAttackPower = 0,
        .attackPowerPerLevel = 3, //6,
        .baseClassDamageVariance = 0.9f,
        .baseAttackTime = 2000,
        .baseRangedAttackTime = 2000,
        .baseSpellPower = 2,
        .spellPowerPerLevel = 4,
        .armorPerLevel = 3
    },
};

namespace FSBStats
{
    FSB_ClassStats const* GetBotClassStats(FSB_Class botClass);
    void ApplyBotBaseClassStats(Creature* creature, FSB_Class botClass);
    void ApplyBotBasePower(Creature* bot, FSB_Class botClass);

    void ApplyBotHealth(Creature* bot, FSB_Class botClass, bool updateHealth);
    void ApplyBotPower(Creature* bot, FSB_Class botClass, bool updatePower);
    void ApplyBotAttackPower(Creature* bot, FSB_Class botClass);
    void ApplyBotDamage(Creature* bot, FSB_Class botClass);
    void ApplyBotArmor(Creature* bot, FSB_Class botClass);

    void ApplyDynamicDamageDealt(Creature* bot, Unit* victim, uint32& damage);

    float ApplyBotDamageTakenReduction(Creature* bot);
    float ApplyBotDamageDoneReduction(Creature* bot);

    void UpdateBotLevelToPlayer(Creature* bot);
    void RecalculateStats(Creature* bot, bool updateHealth, bool updatePower);

    int32 BotGetSpellPower(const Creature* bot);

    float GetBotDamageMultiplier(uint8 level);
    int32 CalculateScaledBotDamage(Creature* bot, Unit* victim, int32 rawDamage);
}
