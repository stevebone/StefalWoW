#include "followship_bots_utils_spells.h"

#include "ThreatManager.h"

// Mana Potion Spells
struct ManaPotionSpell
{
    uint8 minLevel;
    uint8 maxLevel;
    uint32 spellId;
};

static constexpr ManaPotionSpell ManaPotionTable[] =
{
    {  1,  10,  437 },   // Minor Mana Potion 120
    { 11,  22,  438 },   // Lesser Mana Potion 159
    { 23,  37,  2023 },  // Mana Potion 100
    { 38,  45,  11448 }, // Greater Mana Potion 245
    { 46,  50,  17530 }, // Superior Mana Potion 494
    { 51,  59,  17531 }, // Major Mana Potion 512
    { 60,  69,  28499 }, // Super Mana Potion 682
    { 70,  80,  43186 } // Runic Mana Potion 956
};

// Gets the Mana Potion spell id on a provided level
uint32 GetManaPotionSpellForLevel(uint8 level)
{
    for (auto const& entry : ManaPotionTable)
    {
        if (level >= entry.minLevel && level <= entry.maxLevel)
            return entry.spellId;
    }
    return 0;
}

// Gets The Spell name based on a provided spell id
namespace FSBUtilsSpells
{
    std::string GetSpellName(uint32 spellId)
    {
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
        if (!spellInfo || !spellInfo->SpellName)
            return "unknown spell";

        std::string name = spellInfo->SpellName->Str[LOCALE_enUS];
        return name.empty() ? "unknown spell" : name;
    }

    bool CanCastNow(Unit* me, uint32 now, uint32 globalCooldownUntil)
    {
        return me
            && now >= globalCooldownUntil
            && !me->HasUnitState(UNIT_STATE_CASTING);
    }
}
