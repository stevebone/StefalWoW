
#include "Followship_bots_recovery_handler.h"
#include "Followship_bots_mgr.h"

namespace FSBRecovery
{
    BotRecoveryIntent DetermineRecoveryIntent(Creature* bot)
    {
        if (!bot)
            return BotRecoveryIntent::None;

        //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: RecoveryHandler TryRecover intent check for: {}", bot->GetName());

        bool lowHP = bot->GetHealthPct() < BOT_RECOVERY_HP_PCT;

        // Determine if this bot uses mana at all
        bool usesMana = bot->GetMaxPower(POWER_MANA) > 0;

        bool lowMana = false;
        if (usesMana)
            lowMana = bot->GetPowerPct(POWER_MANA) < BOT_RECOVERY_MP_PCT;

        if (lowHP && lowMana)
            return BotRecoveryIntent::RecoverHealthAndMana;

        if (lowHP)
            return BotRecoveryIntent::RecoverHealth;

        if (lowMana)
            return BotRecoveryIntent::RecoverMana;

        return BotRecoveryIntent::None;
    }

    BotRecoverAction GetRecoveryAction(Creature* bot, BotRecoveryIntent intent)
    {
        if (!bot || intent == BotRecoveryIntent::None)
            return BotRecoverAction::None;

        FSB_Class botClass = FSBUtils::GetBotClassForEntry(bot->GetEntry());
        FSB_Race botRace = FSBUtils::GetBotRaceForEntry(bot->GetEntry());

        std::vector<BotRecoverAction> recoveryActions;

        switch (intent)
        {
        case BotRecoveryIntent::RecoverHealth:
            recoveryActions.emplace_back(BotRecoverAction::Eat);
            recoveryActions.emplace_back(BotRecoverAction::Recuperate);

            if (botClass == FSB_Class::Priest || botClass == FSB_Class::Paladin)
                recoveryActions.emplace_back(BotRecoverAction::ClassHeal);
            if (botClass == FSB_Class::Mage)
                recoveryActions.emplace_back(BotRecoverAction::ClassDrinkEat);
                
            if (botRace == FSB_Race::Draenei)
                recoveryActions.emplace_back(BotRecoverAction::RaceHeal);

            break;
        case BotRecoveryIntent::RecoverMana:
            recoveryActions.emplace_back(BotRecoverAction::Drink);
            if (botClass == FSB_Class::Mage)
                recoveryActions.emplace_back(BotRecoverAction::ClassDrinkEat);
            break;
        case BotRecoveryIntent::RecoverHealthAndMana:
            recoveryActions.emplace_back(BotRecoverAction::DrinkEat);
            if (botClass == FSB_Class::Mage)
                recoveryActions.emplace_back(BotRecoverAction::ClassDrinkEat);
            break;
        default:
            recoveryActions.emplace_back(BotRecoverAction::None);
        }

        return Trinity::Containers::SelectRandomContainerElement(recoveryActions);
    }

    bool BotActionDrinkEat(Creature* bot, uint32& globalCooldown, uint32& outSpell, uint8 drinkOrEat)
    {
        if (!bot)
            return false;

        if (bot->IsInCombat())
            return false;

        if (!bot->IsAlive())
            return false;

        uint32 now = getMSTime();

        uint32 spellId = 0;

        FSB_Class botClass = FSBUtils::GetBotClassForEntry(bot->GetEntry());
        FSB_Race botRace = FSBUtils::GetBotRaceForEntry(bot->GetEntry());

        if (drinkOrEat == DRINK_EAT)
        {
            if (bot->HasAura(SPELL_DRINK_CONJURED_CRYSTAL_WATER) || bot->HasAura(SPELL_FOOD_SCALED_WITH_LVL))
                return false;

            FSBUtilsMovement::StopFollow(bot);

            SpellCastResult result = bot->CastSpell(bot, SPELL_DRINK_CONJURED_CRYSTAL_WATER, false);
            SpellCastResult result2 = bot->CastSpell(bot, SPELL_FOOD_SCALED_WITH_LVL, false);

            if (result == SPELL_CAST_OK && result2 == SPELL_CAST_OK)
            {
                globalCooldown = now + 30000; // set cooldown to 30s to not interrup the drink spell which lasts 30 seconds max
                outSpell = RAND(SPELL_DRINK_CONJURED_CRYSTAL_WATER, SPELL_FOOD_SCALED_WITH_LVL);
                return true;
            }
        }

        if (drinkOrEat == DRINK)
        {
            spellId = SPELL_DRINK_CONJURED_CRYSTAL_WATER;

            if (bot->HasAura(spellId))
                return false;

            FSBUtilsMovement::StopFollow(bot);

            SpellCastResult result = bot->CastSpell(bot, spellId, false);

            if (result == SPELL_CAST_OK)
            {
                globalCooldown = now + 30000; // set cooldown to 30s to not interrup the drink spell which lasts 30 seconds max
                outSpell = spellId;
                return true;
            }
        }

        if (drinkOrEat == EAT)
        {
            spellId = SPELL_FOOD_SCALED_WITH_LVL;

            if (bot->HasAura(spellId))
                return false;

            FSBUtilsMovement::StopFollow(bot);

            SpellCastResult result = bot->CastSpell(bot, spellId, false);

            if (result == SPELL_CAST_OK)
            {
                globalCooldown = now + 30000; // set cooldown to 30s to not interrup the drink spell which lasts 30 seconds max
                outSpell = spellId;
                return true;
            }
        }

        if (drinkOrEat == CLASS_DRINK_EAT)
        {
            if (botClass == FSB_Class::Mage)
                spellId = SPELL_MAGE_CONJURED_MANA_PUDDING;
            else return false;

            if (bot->HasAura(spellId))
                return false;

            FSBUtilsMovement::StopFollow(bot);

            SpellCastResult result = bot->CastSpell(bot, spellId, false);

            if (result == SPELL_CAST_OK)
            {
                globalCooldown = now + 30000; // set cooldown to 30s to not interrup the drink spell which lasts 30 seconds max
                outSpell = spellId;
                return true;
            }
        }

        if (drinkOrEat == RECUPERATE)
        {
            spellId = SPELL_RECUPERATE;

            if (bot->HasAura(spellId))
                return false;

            FSBUtilsMovement::StopFollow(bot);

            SpellCastResult result = bot->CastSpell(bot, spellId, false);

            if (result == SPELL_CAST_OK)
            {
                globalCooldown = now + 10000; // set cooldown to 30s to not interrup the drink spell which lasts 30 seconds max
                outSpell = spellId;
                return true;
            }
        }

        if (drinkOrEat == CLASS_HEAL)
        {
            if (botClass == FSB_Class::Priest)
                spellId = RAND(SPELL_PRIEST_HEAL, SPELL_PRIEST_FLASH_HEAL);
            else if (botClass == FSB_Class::Paladin)
                spellId = RAND(SPELL_PALADIN_HOLY_LIGHT, SPELL_PALADIN_FLASH_OF_LIGHT);
            else return false;

            if (bot->HasAura(spellId))
                return false;

            FSBUtilsMovement::StopFollow(bot);

            SpellCastResult result = bot->CastSpell(bot, spellId, false);

            if (result == SPELL_CAST_OK)
            {
                globalCooldown = now + 1500; // set cooldown to 30s to not interrup the drink spell which lasts 30 seconds max
                outSpell = spellId;
                return true;
            }
        }

        if (drinkOrEat == RACE_HEAL)
        {
            if (botRace == FSB_Race::Draenei)
                spellId = SPELL_DRAENEI_GIFT_NAARU;
            else return false;

            if (bot->HasAura(spellId))
                return false;

            FSBUtilsMovement::StopFollow(bot);

            SpellCastResult result = bot->CastSpell(bot, spellId, false);

            if (result == SPELL_CAST_OK)
            {
                TC_LOG_DEBUG("scripts.ai.fsb", "FSB: RecoveryAction Race Heal cast result: {}", result);
                globalCooldown = now + 1500; // set cooldown to 30s to not interrup the drink spell which lasts 30 seconds max
                outSpell = spellId;
                return true;
            }
            else TC_LOG_DEBUG("scripts.ai.fsb", "FSB: RecoveryAction Race Heal cast result not ok: {}", result);
        }

        return false;
    }

    bool TryRecoverAction(Creature* bot, BotRecoverAction action, uint32& globalCooldown)
    {
        if (!bot)
            return false;

        if (bot->IsInCombat())
            return false;

        if (!bot->IsAlive())
            return false;

        bool check = false;
        uint32 spellId = 0;
        uint8 drinkOrEat = 0;

        switch (action)
        {
        case BotRecoverAction::None:
            return false;
        case BotRecoverAction::DrinkEat:
            drinkOrEat = DRINK_EAT;
            if (BotActionDrinkEat(bot, globalCooldown, spellId, drinkOrEat))
                check = true;
            break;
        case BotRecoverAction::Drink:
            drinkOrEat = DRINK;
            if (BotActionDrinkEat(bot, globalCooldown, spellId, drinkOrEat))
                check = true;
            break;
        case BotRecoverAction::ClassDrinkEat:
            drinkOrEat = CLASS_DRINK_EAT;
            if (BotActionDrinkEat(bot, globalCooldown, spellId, drinkOrEat))
                check = true;
            break;
        case BotRecoverAction::Eat:
            drinkOrEat = EAT;
            if (BotActionDrinkEat(bot, globalCooldown, spellId, drinkOrEat))
                check = true;
            break;
        case BotRecoverAction::Recuperate:
            drinkOrEat = RECUPERATE;
            if (BotActionDrinkEat(bot, globalCooldown, spellId, drinkOrEat))
                check = true;
            break;
        case BotRecoverAction::ClassHeal:
            drinkOrEat = CLASS_HEAL;
            if (BotActionDrinkEat(bot, globalCooldown, spellId, drinkOrEat))
                check = true;
            break;
        case BotRecoverAction::RaceHeal:
            drinkOrEat = RACE_HEAL;
            if (BotActionDrinkEat(bot, globalCooldown, spellId, drinkOrEat))
                check = true;
            break;
        default:
            break;
        }

        if (check)
        {
            if (urand(0, 99) <= FollowshipBotsConfig::configFSBChatterRate)
            {
                std::string pname = "";
                Player* player = FSBMgr::GetBotOwner(bot);
                if (player)
                    pname = player->GetName();

                std::string msg = FSBUtilsTexts::BuildNPCSayText(pname, NULL, FSBSayType::OOCRecovery, FSBUtilsSpells::GetSpellName(spellId));
                bot->Say(msg, LANG_UNIVERSAL);
            }

            return true;
        }

        return false;

    }

    bool TryOOCRecovery(Creature* bot, BotRecoveryIntent intent, uint32& globalCooldown)
    {
        if (!bot)
            return false;

        if (bot->IsInCombat())
            return false;

        if (!bot->IsAlive())
            return false;

        if (intent == BotRecoveryIntent::None)
            return false;


        BotRecoverAction action = GetRecoveryAction(bot, intent);
        
        if (TryRecoverAction(bot, action, globalCooldown))
            return true;

        return false;
    }

    int32 GetDrinkFoodPerLevel(uint8 level, DrinkFoodPerLevel const* table, size_t tableSize)
    {
        for (size_t i = 0; i < tableSize; ++i)
        {
            if (level >= table[i].minLevel && level <= table[i].maxLevel)
                return table[i].value;
        }

        return 0;
    }

    // Wrapper for GetDrinkFoodPerLevel
    int32 GetDrinkFood(uint16 level)
    {
        return GetDrinkFoodPerLevel(
            level,
            DrinkFoodTable,
            std::size(DrinkFoodTable)
        );
    }
}
