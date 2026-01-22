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
};

struct FSBUtilsStatsMods
{
    // REGEN
    int32 flatHealthPerTick = 0;
    int32 flatManaPerTick = 0;

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
    void ApplyBotBaseRegen(Creature* me, FSBUtilsStatsMods& mods, FSB_Class botClass);

    // Full mod-aware version
    void ApplyBotRegen(Unit* unit, FSB_Class botClass, const FSBUtilsStatsMods& mods, bool doHealth, bool doMana);
    void ApplyMaxHealth(Unit* unit, const FSBUtilsStatsMods& mods);
    void ProcessBotCustomRegenTick(Creature* creature, FSB_Class botClass, const FSBUtilsStatsMods& _baseStatsMods, const FSBUtilsStatsMods& _statsMods);


    void RecalculateMods(Unit* unit, const FSBUtilsStatsMods& mods);

    /**
     * @brief Updates the bot's level to match the player's level and recalculates stats.
     *
     * @param bot The creature/bot whose level to update.
     * @param regenMods Optional pointer to regeneration modifiers, if used for stat recalculation.
     */
    void UpdateBotLevelToPlayer(Creature* bot, const FSBUtilsStatsMods& regenMods);
}

