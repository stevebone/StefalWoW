#pragma once

#include "Define.h"
#include "Unit.h"

struct FSBUtilsStatsRegen
{
    float healthPctPerTickCombat = 0.0f;    // % max HP per tick while in combat
    float healthPctPerTickOutOfCombat = 0.0f;

    float manaPctPerTickCombat = 0.0f;      // % max mana per tick while in combat
    float manaPctPerTickOutOfCombat = 0.0f;

    bool hasClassSpecific = false;          // true if this regen is class-specific
};

struct FSBUtilsStatsMods
{
    // Fixed points regen
    int32 flatHealthPerTick = 0;
    int32 flatManaPerTick = 0;

    // % points 
    float healthPctBonus = 0.0f; // future use
    float manaPctBonus = 0.0f;

    // Fixed max hp points increase
    int32 flatMaxHealth = 0;      // e.g., +2000 HP
    float pctMaxHealthBonus = 0.0f; // e.g., +10%

    // Fixed mana points
};


namespace FSBUtilsStats
{
    // Regen presets
    extern const FSBUtilsStatsRegen DefaultRegen;
    extern const FSBUtilsStatsRegen PriestRegen;

    // Basic regen helpers
    void ApplyHealthRegen(Unit* unit);
    void ApplyManaRegen(Unit* unit);
    void ApplyRegen(Unit* unit);

    // Full mod-aware version
    void ApplyRegen(Unit* unit, const FSBUtilsStatsMods& mods, bool doHealth = true, bool doMana = true);
    void ApplyMaxHealth(Unit* unit, const FSBUtilsStatsMods& mods);

    void RecalculateStats(Unit* unit, const FSBUtilsStatsMods& mods);
}

