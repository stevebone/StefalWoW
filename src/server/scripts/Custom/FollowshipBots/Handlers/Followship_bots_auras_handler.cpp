#include "Followship_bots_utils.h"

#include "followship_bots_druid.h"
#include "followship_bots_mage.h"
#include "followship_bots_warlock.h"
#include "followship_bots_warrior.h"

#include "Followship_bots_auras_handler.h"
#include "Followship_bots_movement_handler.h"
#include "Followship_bots_recovery_handler.h"
#include "Followship_bots_regen_handler.h"
#include "Followship_bots_stats_handler.h"

namespace FSBAuras
{
    void BotOnAuraApplied(Creature* bot, AuraApplication const* aurApp, bool applied, FSBRegenMods& botRegenMods, bool& hasSS, FSBBotStats& botStats)
    {
        if (!bot)
            return;

        if (!aurApp || !aurApp->GetBase())
            return;

        // First check class specific spell auras
        FSB_Class cls = FSBUtils::GetBotClassForEntry(bot->GetEntry());

        switch (cls)
        {
        case FSB_Class::Warrior:
            break;
        case FSB_Class::Priest:
            break;
        case FSB_Class::Mage:
            break;
        case FSB_Class::Rogue:
            break;
        case FSB_Class::Druid:
            if (FSBDruid::BotOnAuraApplied(bot, aurApp, applied, botStats))
                break;
            break;
        case FSB_Class::Paladin:
            break;
        case FSB_Class::Hunter:
            break;
        case FSB_Class::Warlock:
            break;
        case FSB_Class::Shaman:
            break;
        default:
            break;
        }

        // Generic Spell Auras
        switch (aurApp->GetBase()->GetId())
        {
        case SPELL_DRINK_CONJURED_CRYSTAL_WATER:
        {
            int32 amount = FSBRecovery::GetDrinkFood(bot->GetLevel()) * 2;

            if(applied)
                botRegenMods.flatManaPerTick += amount;
            else if (!applied)
            {
                botRegenMods.flatManaPerTick -= amount;

                if (bot->GetStandState() == UNIT_STAND_STATE_SIT)
                {
                    float followDistance = frand(2.f, 8.f);
                    float followAngle = frand(0.0f, float(M_PI * 2.0f));

                    bot->SetStandState(UNIT_STAND_STATE_STAND);
                    FSBMovement::ResumeFollow(bot, followDistance, followAngle);
                }
            }

            break;
        }

        case SPELL_FOOD_SCALED_WITH_LVL:
        {
            int32 amount = FSBRecovery::GetDrinkFood(bot->GetLevel()) * 2;

            if(applied)
                botRegenMods.flatHealthPerTick += amount;
            else if (!applied)
            {
                botRegenMods.flatHealthPerTick -= amount;

                if (bot->GetStandState() == UNIT_STAND_STATE_SIT)
                {
                    float followDistance = frand(2.f, 8.f);
                    float followAngle = frand(0.0f, float(M_PI * 2.0f));

                    bot->SetStandState(UNIT_STAND_STATE_STAND);
                    FSBMovement::ResumeFollow(bot, followDistance, followAngle);
                }
            }

            break;
        }

            // Mage
        case SPELL_MAGE_CONJURED_MANA_PUDDING:
        {
            if (!applied && bot->GetStandState() == UNIT_STAND_STATE_SIT)
            {
                float followDistance = frand(2.f, 8.f);
                float followAngle = frand(0.0f, float(M_PI * 2.0f));

                bot->SetStandState(UNIT_STAND_STATE_STAND);
                FSBMovement::ResumeFollow(bot, followDistance, followAngle);
            }
            break;
        }

        case SPELL_MAGE_ARCANE_INTELLECT:
        {
            float pct = bot->GetPctModifierValue(UNIT_MOD_MANA, TOTAL_PCT);

            if(applied)
                bot->SetStatPctModifier(UNIT_MOD_MANA, TOTAL_PCT, pct + 0.03f);
            else if(!applied)
                bot->SetStatPctModifier(UNIT_MOD_MANA, TOTAL_PCT, pct - 0.03f);

            FSBStats::RecalculateStats(bot, false, false);

            break;
        }

        case SPELL_MAGE_EVOCATION:
        {
            if(applied)
                botRegenMods.pctManaPerTick += 1500.f;
            else if(!applied)
                botRegenMods.pctManaPerTick -= 1500.f;

            FSBStats::RecalculateStats(bot, false, false);

            break;
        }

            // Paladin
        case SPELL_PALADIN_BLESSING_KINGS:
        {
            float hpPct = bot->GetPctModifierValue(UNIT_MOD_HEALTH, TOTAL_PCT);
            float manaPct = bot->GetPctModifierValue(UNIT_MOD_MANA, TOTAL_PCT);
            float apPct = bot->GetPctModifierValue(UNIT_MOD_ATTACK_POWER, TOTAL_PCT);
            float rapPct = bot->GetPctModifierValue(UNIT_MOD_ATTACK_POWER_RANGED, TOTAL_PCT);

            if (applied)
            {
                bot->SetStatPctModifier(UNIT_MOD_HEALTH, TOTAL_PCT, hpPct + 0.1f);
                bot->SetStatPctModifier(UNIT_MOD_MANA, TOTAL_PCT, manaPct + 0.1f);
                bot->SetStatPctModifier(UNIT_MOD_ATTACK_POWER, TOTAL_PCT, apPct + 0.1f);
                bot->SetStatPctModifier(UNIT_MOD_ATTACK_POWER_RANGED, TOTAL_PCT, rapPct + 0.1f);
            }
            else if (!applied)
            {
                bot->SetStatPctModifier(UNIT_MOD_HEALTH, TOTAL_PCT, hpPct - 0.1f);
                bot->SetStatPctModifier(UNIT_MOD_MANA, TOTAL_PCT, manaPct - 0.1f);
                bot->SetStatPctModifier(UNIT_MOD_ATTACK_POWER, TOTAL_PCT, apPct - 0.1f);
                bot->SetStatPctModifier(UNIT_MOD_ATTACK_POWER_RANGED, TOTAL_PCT, rapPct - 0.1f);
            }

            FSBStats::RecalculateStats(bot, false, false);

            break;
        }
        case SPELL_PALADIN_RITE_OF_SANCTIFICATION:
        {
            float pct = bot->GetPctModifierValue(UNIT_MOD_HEALTH, TOTAL_PCT);

            if (applied)
            {
                bot->ApplyStatPctModifier(UNIT_MOD_ARMOR, TOTAL_PCT, 0.05f);
                bot->SetStatPctModifier(UNIT_MOD_HEALTH, TOTAL_PCT, pct + 0.02f);
            }
            else if (!applied)
            {
                bot->ApplyStatPctModifier(UNIT_MOD_ARMOR, TOTAL_PCT, -0.05f);
                bot->SetStatPctModifier(UNIT_MOD_HEALTH, TOTAL_PCT, pct - 0.02f);
            }

            FSBStats::RecalculateStats(bot, false, false);

            break;
        }

            // Priest
        case SPELL_PRIEST_POWER_WORD_FORTITUDE:
        {
            float pct = bot->GetPctModifierValue(UNIT_MOD_HEALTH, TOTAL_PCT);

            if(applied)
                bot->SetStatPctModifier(UNIT_MOD_HEALTH, TOTAL_PCT, pct + 0.1f);
            else if(!applied)
                bot->SetStatPctModifier(UNIT_MOD_HEALTH, TOTAL_PCT, pct - 0.1f);

            FSBStats::RecalculateStats(bot, false, false);

            break;
        }

            // Warrior
        case SPELL_WARRIOR_BATTLE_SHOUT:
        {
            if(applied)
                bot->ApplyStatPctModifier(UNIT_MOD_ATTACK_POWER, TOTAL_PCT, 0.05f);
            else if(!applied)
                bot->ApplyStatPctModifier(UNIT_MOD_ATTACK_POWER, TOTAL_PCT, -0.05f);

            FSBStats::RecalculateStats(bot, false, false);

            break;
        }

        case SPELL_WARRIOR_BATTLE_STANCE:
        {
            if(applied)
                botRegenMods.flatRagePerTick += 20;
            else if(!applied)
                botRegenMods.flatRagePerTick -= 20;

            break;
        }

            // Warlock
        case SPELL_WARLOCK_SOULSTONE:
        {
            if (applied)
                hasSS = true;
            break;
        }
        case SPELL_WARLOCK_DEMON_ARMOR:
        {
            float HpPct = bot->GetPctModifierValue(UNIT_MOD_HEALTH, TOTAL_PCT);
            float armorPct = bot->GetPctModifierValue(UNIT_MOD_ARMOR, TOTAL_PCT);

            if (applied)
            {
                bot->SetStatPctModifier(UNIT_MOD_HEALTH, TOTAL_PCT, HpPct + 0.1f);
                bot->SetStatPctModifier(UNIT_MOD_ARMOR, TOTAL_PCT, armorPct + 1.6f);
            }
            else if (!applied)
            {
                bot->SetStatPctModifier(UNIT_MOD_HEALTH, TOTAL_PCT, HpPct - 0.1f);
                bot->SetStatPctModifier(UNIT_MOD_ARMOR, TOTAL_PCT, armorPct - 1.6f);
            }

            break;
        }

        default:
            break;
        }

        
    }
}
