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
    void ApplyBotRegen(Unit* unit, FSB_Class botClass, const FSBUtilsStatsMods& mods, bool doHealth, bool doMana);
    void ApplyMaxHealth(Unit* unit, const FSBUtilsStatsMods& mods);
    void ApplyMaxMana(Unit* unit, const FSBUtilsStatsMods& mods);
    void ProcessBotCustomRegenTick(Creature* creature, FSB_Class botClass, const FSBUtilsStatsMods& _baseStatsMods, const FSBUtilsStatsMods& _statsMods);


    void RecalculateMods(Unit* unit, const FSBUtilsStatsMods& mods);

    /**
     * @brief Updates the bot's level to match the player's level and recalculates stats.
     *
     * @param bot The creature/bot whose level to update.
     * @param regenMods Optional pointer to regeneration modifiers, if used for stat recalculation.
     */
    void UpdateBotLevelToPlayer(Creature* bot, const FSBUtilsStatsMods& regenMods);

    bool SpendManaPct(Creature* bot, float pct);
    bool SpendManaFlat(Creature* bot, int32 cost);

    Classes FSBToTCClass(FSB_Class botClass);
    Powers GetBotPowerType(Creature* bot);

    bool IsRageUser(Creature* bot);
    void GenerateRageFromDamageTaken(Creature* bot, uint32 damage);

}

