#include "Followship_bots_utils.h"

#include "Followship_bots_druid.h"
#include "Followship_bots_paladin.h"
#include "Followship_bots_warlock.h"
#include "Followship_bots_warrior.h"

#include "Followship_bots_group_handler.h"
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

    bool BotTryDispel(Creature* bot)
    {
        if (!bot)
            return false;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!baseAI)
            return false;

        FSB_Class cls = baseAI->botClass;

        auto it = DispelTable.find(cls);
        if (it == DispelTable.end())
            return false; // class cannot dispel anything

        const DispelAbility& ability = it->second;

        if (!FSBSpellsUtils::IsSpellReady(ability.spellId))
            return false;

        Unit* target = FSBGroup::BotGetDispelMember(bot, ability);
        if (!target)
            return false;

        if (bot->GetDistance(target) > 30.0f)
            return false;

        return BotCastSpellWithCooldown(bot, ability.spellId, target, ability.cooldown);
    }

    bool BotTryOffensiveDispel(Creature* bot)
    {
        if (!bot)
            return false;

        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!baseAI)
            return false;

        FSB_Class cls = baseAI->botClass;

        auto it = OffensiveDispelTable.find(cls);
        if (it == OffensiveDispelTable.end())
            return false; // class cannot offensively dispel

        const OffensiveDispelAbility& ability = it->second;

        Unit* target = bot->GetVictim();
        if (!target)
            return false;

        // Check if target has a buff we can remove/steal
        Aura* buff = FSBSpellsUtils::FindEnemyBuffToDispel(target);
        if (!buff)
            return false;

        // Optional: avoid dispelling trivial buffs
        if (buff->GetSpellInfo()->Dispel != DISPEL_MAGIC)
            return false;

        // Cast the spell
        return BotCastSpell(bot, ability.spellId, target);
    }

    bool BotCastSpell(Creature* bot, uint32 spellId, Unit* target)
    {
        if (!bot)
            return false;

        if (!spellId)
            return false;

        SpellCastResult result = bot->CastSpell(target, spellId, true);

        if (result == SPELL_CAST_OK)
        {
            TC_LOG_DEBUG("scripts.ai.fsb", "FSB Bot {} casted spell {} on target {}", bot->GetName(), FSBSpellsUtils::GetSpellName(spellId), target->GetName());
            return true;
        }

        return false;
    }

    bool BotCastSpellWithCooldown(Creature* bot, uint32 spellId, Unit* target, uint32 cooldown)
    {
        if (!bot)
            return false;

        if (!spellId)
            return false;

        SpellCastResult result = bot->CastSpell(target, spellId, true);

        if (result == SPELL_CAST_OK)
        {
            FSBSpellsUtils::PutSpellOnCooldown(spellId, cooldown);
            TC_LOG_DEBUG("scripts.ai.fsb", "FSB Bot {} casted spell {} on target {}", bot->GetName(), FSBSpellsUtils::GetSpellName(spellId), target->GetName());
            return true;
        }

        return false;
    }
}
