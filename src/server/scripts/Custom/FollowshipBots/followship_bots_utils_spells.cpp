#include "followship_bots_utils_spells.h"
#include "followship_bots_priest.h"

#include "ThreatManager.h"
#include "Unit.h"

// Mana Potion Spells
struct ManaPotionSpell
{
    uint8 minLevel;
    uint8 maxLevel;
    uint32 spellId;
};

static constexpr ManaPotionSpell ManaPotionTable[] =
{
    {  1,  15,  437 },   // Minor Mana Potion 120
    { 16,  29,  438 },   // Lesser Mana Potion 159
    { 30,  40,  17530 }, // Superior Mana Potion 494
    { 41,  50,  17531 }, // Major Mana Potion 512
    { 51,  60,  28499 }, // Super Mana Potion 682
    { 61,  80,  43186 } // Runic Mana Potion 956
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

    bool HasDispellableDebuff(Unit* unit)
    {
        if (!unit)
            return false;

        for (auto const& pair : unit->GetAppliedAuras())
        {
            AuraApplication const* aurApp = pair.second;
            if (!aurApp)
                continue;

            Aura const* aura = aurApp->GetBase();
            if (!aura)
                continue;

            // Skip positive auras
            if (aurApp->IsPositive())
                continue;

            SpellInfo const* info = aura->GetSpellInfo();
            if (!info)
                continue;

            // Check dispel type
            switch (info->Dispel)
            {
            case DISPEL_MAGIC:
            case DISPEL_DISEASE:
            case DISPEL_POISON:
            case DISPEL_CURSE:
                break;
            default:
                continue;
            }

            // Skip undispellable spells
            //if (info->HasAttribute(SPELL_ATTR0_CANT_BE_DISPELLED))
            //    continue;

            return true;
        }

        return false;
    }

}
