#pragma once


struct FSBRegenMods
{
    // REGEN
    int32 flatHealthPerTick = 0;
    int32 flatManaPerTick = 0;
    int32 flatRagePerTick = 0;

    float pctHealthPerTick = 0;
    float pctManaPerTick = 0;
};

namespace FSBRegen
{
    void ApplyBotRegen(Unit* unit, FSB_Class botClass, FSBRegenMods regenMods, bool doHealth, bool doMana);
    void ProcessBotCustomRegenTick(Creature* creature, FSB_Class botClass, FSBRegenMods regenMods);
}
