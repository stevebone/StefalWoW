#include "Followship_bots_utils.h"

#include "Followship_bots_druid.h"
#include "Followship_bots_paladin.h"
#include "Followship_bots_warlock.h"
#include "Followship_bots_warrior.h"

#include "Followship_bots_spells_handler.h"

namespace FSBSpells
{
    void HandleOnSpellCast(Creature* bot, FSB_Class botClass, uint32 spellId)
    {
        if (!bot || !spellId)
            return;

        HandleOnSpellCastGeneric(bot, spellId);

        switch (botClass)
        {
        case FSB_Class::Warrior:
            FSBWarrior::HandleOnSpellCast(bot, spellId);
            break;
        case FSB_Class::Priest:
            break;
        case FSB_Class::Mage:
            break;
        case FSB_Class::Rogue:
            break;
        case FSB_Class::Druid:
            FSBDruid::HandleOnSpellCast(bot, spellId);
            break;
        case FSB_Class::Paladin:
            FSBPaladin::HandleOnSpellCast(bot, spellId);
            break;
        case FSB_Class::Hunter:
            break;
        case FSB_Class::Warlock:
            FSBWarlock::HandleOnSpellCast(bot, spellId);
            break;
        default:
            break;
        }

    }

    void HandleOnSpellCastGeneric(Creature* bot, uint32 spellId)
    {
        if (!bot || !spellId)
            return;

        // Health potions
        switch (spellId)
        {
        case 439:
            bot->ModifyHealth(120);
            break;
        case 440:
            bot->ModifyHealth(160);
            break;
        case 4042:
            bot->ModifyHealth(500);
            break;
        case 17534:
            bot->ModifyHealth(650);
            break;
        case 28495:
            bot->ModifyHealth(800);
            break;
        case 43185:
            bot->ModifyHealth(1000);
            break;
        default:
            break;
        }
    }
}
