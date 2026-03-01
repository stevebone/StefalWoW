
#include "Chat.h"
#include "ChatCommand.h"
#include "Pet.h"
#include "WorldSession.h"

#include "Followship_bots_mgr.h"
#include "Followship_bots_utils.h"

#include "Followship_bots_hunter.h"

#include "Followship_bots_outofcombat_handler.h"
#include "Followship_bots_stats_handler.h"

using namespace Trinity::ChatCommands;

class followship_bots_commandscript : public CommandScript
{
public:
    followship_bots_commandscript() : CommandScript("followship_bots_commandscript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable fsbCommandTable =
        {
            { "info",      HandleFSBInfo,            rbac::RBAC_PERM_COMMAND_GM, Console::No },
            { "stats",     HandleFSBStats,           rbac::RBAC_PERM_COMMAND_GM, Console::No },
            { "damage",    HandleFSBDamage,          rbac::RBAC_PERM_COMMAND_GM, Console::No },
            { "afkaction", HandleFSBAfkAction,       rbac::RBAC_PERM_COMMAND_GM, Console::No },
            { "playsound", HandleFSBPlaySound,       rbac::RBAC_PERM_COMMAND_GM, Console::No },
            { "summonpet", HandleFSBSummonPet,       rbac::RBAC_PERM_COMMAND_GM, Console::No },
            { "castSpell", HandleFSBCastSpellOnTarget, rbac::RBAC_PERM_COMMAND_GM, Console::No },
        };

        static ChatCommandTable commandTable =
        {
            { "fsb", fsbCommandTable }
        };



        return commandTable;
    }

    static bool HandleFSBStats(ChatHandler* handler)
    {
        Creature* target = handler->getSelectedCreature();

        if (!target)
        {
            handler->SendSysMessage(LANG_SELECT_CREATURE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        Creature* bot = target->ToCreature();
        if (!bot || !bot->IsBot())
        {
            handler->SendSysMessage("Target is not a Followship bot.");
            return false;
        }

        handler->PSendSysMessage("=== Followship Bot Stats ===");

        float baseHealth = bot->GetFlatModifierValue(UNIT_MOD_HEALTH, BASE_VALUE);
        float totalHealth = bot->GetTotalAuraModValue(UNIT_MOD_HEALTH);

        handler->PSendSysMessage("Health: Base: %.1f Max: %.1f (%.1f%%)", baseHealth, totalHealth, bot->GetHealthPct());

        Powers powerType = bot->GetPowerType();

        float currentPower = bot->GetPower(powerType);
        float maxPower = bot->GetMaxPower(powerType);
        float basePower = bot->GetFlatModifierValue(UnitMods(UNIT_MOD_POWER_START + AsUnderlyingType(powerType)), BASE_VALUE);
        float totalPower = bot->GetTotalAuraModValue(UnitMods(UNIT_MOD_POWER_START + AsUnderlyingType(powerType)));

        handler->PSendSysMessage("Power (%s): Base %u / %u", FSBUtils::PowerTypeToString(powerType), (int32)basePower, (int32)totalPower);
        handler->PSendSysMessage("Current Power (%s): Now %u / Max %u", FSBUtils::PowerTypeToString(powerType), (int32)currentPower, (int32)maxPower);


        float baseArmor = bot->GetFlatModifierValue(UNIT_MOD_ARMOR, BASE_VALUE);
        float totalArmor = bot->GetTotalAuraModValue(UNIT_MOD_ARMOR);

        handler->PSendSysMessage("Armor: Base: %.1f, Total: %.1f, Bonus: %.1f", baseArmor, totalArmor, totalArmor - baseArmor);

        return true;
    }

    static bool HandleFSBDamage(ChatHandler* handler)
    {
        Creature* target = handler->getSelectedCreature();

        if (!target)
        {
            handler->SendSysMessage(LANG_SELECT_CREATURE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        Creature* bot = target->ToCreature();
        if (!bot || !bot->IsBot())
        {
            handler->SendSysMessage("Target is not a Followship bot.");
            return false;
        }

        uint8 level = bot->GetLevel();
        uint8 eLevel = bot->GetLevelForTarget(handler->GetPlayer());

        handler->PSendSysMessage("=== Followship Bot Damage Stats ===");
        handler->PSendSysMessage("Bot %s with level: %u and effective level: %u", bot->GetName(), level, eLevel);

        float totalAttackPower = bot->GetTotalAuraModValue(UNIT_MOD_ATTACK_POWER);
        float base_attPower = bot->GetFlatModifierValue(UNIT_MOD_ATTACK_POWER, BASE_VALUE) * bot->GetPctModifierValue(UNIT_MOD_ATTACK_POWER, BASE_PCT);

        handler->PSendSysMessage("Attack Power: Base %.1f / Total %.1f", base_attPower, totalAttackPower);

        float totalRAttackPower = bot->GetTotalAuraModValue(UNIT_MOD_ATTACK_POWER_RANGED);
        float base_RattPower = bot->GetFlatModifierValue(UNIT_MOD_ATTACK_POWER_RANGED, BASE_VALUE) * bot->GetPctModifierValue(UNIT_MOD_ATTACK_POWER_RANGED, BASE_PCT);

        handler->PSendSysMessage("Ranged Attack Power: Base %.1f / Total %.1f", base_RattPower, totalRAttackPower);

        float weaponDamageMin = bot->GetWeaponDamageRange(BASE_ATTACK, MINDAMAGE);
        float weaponDamageMax = bot->GetWeaponDamageRange(BASE_ATTACK, MAXDAMAGE);

        float basedamageL = bot->GetBaseDamageForLevel(level);
        float basedamageEL = bot->GetBaseDamageForLevel(eLevel);

        handler->PSendSysMessage("Weapon Damage: Min %.1f / Max %.1f", weaponDamageMin, weaponDamageMax);
        handler->PSendSysMessage("Base Damage: Level %.1f / E Level %.1f", basedamageL, basedamageEL);

        int32 spellPower = FSBStats::BotGetSpellPower(bot);
        handler->PSendSysMessage("Spell Power: %u", spellPower);

        return true;
    }

    static bool HandleFSBInfo(ChatHandler* handler)
    {
        Creature* target = handler->getSelectedCreature();

        if (!target)
        {
            handler->SendSysMessage(LANG_SELECT_CREATURE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        Creature* bot = target->ToCreature();
        if (!bot || !bot->IsBot())
        {
            handler->SendSysMessage("Target is not a Followship bot.");
            return false;
        }

        // --- Fetch bot metadata ---
        FSB_Class botClass = FSBMgr::Get()->GetBotClassForEntry(bot->GetEntry());
        FSB_Race botRace = FSBMgr::Get()->GetBotRaceForEntry(bot->GetEntry());

        

        handler->PSendSysMessage("=== Followship Bot Info ===");
        handler->PSendSysMessage("Name: %s", bot->GetName().c_str());
        handler->PSendSysMessage("Level: %u", bot->GetLevel());
        handler->PSendSysMessage("Class: %s", FSBUtils::BotClassToString(botClass));
        handler->PSendSysMessage("Race: %s", FSBUtils::BotRaceToString(botRace));

        

        handler->PSendSysMessage("Attack Power: %.1f and Ranged Attack Power: %.1f ", bot->GetTotalAttackPowerValue(BASE_ATTACK), bot->GetTotalAttackPowerValue(RANGED_ATTACK));

        handler->PSendSysMessage("Damage: Base: %.1f Min: %.1f Max: %.1f", bot->GetBaseDamageForLevel(bot->GetLevel()), bot->GetWeaponDamageRange(BASE_ATTACK, MINDAMAGE), bot->GetWeaponDamageRange(BASE_ATTACK, MAXDAMAGE));
        handler->PSendSysMessage("Ranged Damage: Base: %.1f Min: %.1f Max: %.1f", bot->GetBaseDamageForLevel(bot->GetLevel()), bot->GetWeaponDamageRange(RANGED_ATTACK, MINDAMAGE), bot->GetWeaponDamageRange(RANGED_ATTACK, MAXDAMAGE));
        handler->PSendSysMessage("Offhand Damage: Base: %.1f Min: %.1f Max: %.1f", bot->GetBaseDamageForLevel(bot->GetLevel()), bot->GetWeaponDamageRange(OFF_ATTACK, MINDAMAGE), bot->GetWeaponDamageRange(OFF_ATTACK, MAXDAMAGE));
        handler->PSendSysMessage("Mainhand DMG mod: Base: %.1f, Mod: %.1f, BasePct: %.1f, ModPct: %.1f",
            bot->GetFlatModifierValue(UNIT_MOD_DAMAGE_MAINHAND, BASE_VALUE),
            bot->GetFlatModifierValue(UNIT_MOD_DAMAGE_MAINHAND, TOTAL_VALUE),
            bot->GetPctModifierValue(UNIT_MOD_DAMAGE_MAINHAND, BASE_PCT),
            bot->GetPctModifierValue(UNIT_MOD_DAMAGE_MAINHAND, TOTAL_PCT));

        handler->PSendSysMessage("In Combat: %s", bot->IsInCombat() ? "Yes" : "No");

        return true;
    }

    static bool HandleFSBAfkAction(ChatHandler* handler)
    {
        Creature* target = handler->getSelectedCreature();

        if (!target)
        {
            handler->SendSysMessage(LANG_SELECT_CREATURE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        Creature* bot = target->ToCreature();
        if (!bot || !bot->IsBot())
        {
            handler->SendSysMessage("Target is not a Followship bot.");
            return false;
        }

        // --- Fetch bot metadata ---
        handler->PSendSysMessage("=== Followship Bot Random AFK Action ===");
        handler->PSendSysMessage("Returned: %s", FSBOOC::BotOOCActionPlayerAFK(bot, true) ? "true" : "false");

        return true;
    }

    static bool HandleFSBPlaySound(ChatHandler* handler, uint32 soundId)
    {
        Creature* target = handler->getSelectedCreature();

        if (!target)
        {
            handler->SendSysMessage(LANG_SELECT_CREATURE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        Creature* bot = target->ToCreature();
        if (!bot || !bot->IsBot())
        {
            handler->SendSysMessage("Target is not a Followship bot.");
            return false;
        }

        // --- Fetch bot metadata ---
        handler->PSendSysMessage("=== Followship Bot Play Sound ID ===");
        handler->PSendSysMessage("Playing Sound Id: %u", soundId);

        bot->PlayDistanceSound(soundId, handler->GetPlayer());

        return true;
    }

    static bool HandleFSBSummonPet(ChatHandler* handler)
    {
        Creature* target = handler->getSelectedCreature();

        if (!target)
        {
            handler->SendSysMessage(LANG_SELECT_CREATURE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        Creature* bot = target->ToCreature();
        if (!bot || !bot->IsBot())
        {
            handler->SendSysMessage("Target is not a Followship bot.");
            return false;
        }

        // --- Fetch bot metadata ---
        handler->PSendSysMessage("=== Followship Bot Summon Pet ===");

        FSBHunter::BotSummonPet(bot);
        
        return true;
    }

    static bool HandleFSBCastSpellOnTarget(ChatHandler* handler, uint32 spellId, std::string name)
    {
        Creature* target = handler->getSelectedCreature();

        if (!target)
        {
            handler->SendSysMessage(LANG_SELECT_CREATURE);
            handler->SetSentErrorMessage(true);
            return false;
        }

        Creature* bot = target->ToCreature();
        if (!bot || !bot->IsBot())
        {
            handler->SendSysMessage("Target is not a Followship bot.");
            return false;
        }

        // --- Fetch bot metadata ---
        handler->PSendSysMessage("=== Followship Bot Cast Spell On Target ===");
        Unit* spellTarget = FSBUtils::FindCreatureByName(bot, name);
        if (!spellTarget)
        {
            handler->PSendSysMessage("Cast Failed: Target Not Found");
            handler->SetSentErrorMessage(true);
            return false;
        }

        if(!spellId)
        {
            handler->PSendSysMessage("Cast Failed: No valid spell id");
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (FSBSpells::BotCastSpell(bot, spellId, spellTarget))
        {
            handler->PSendSysMessage("Cast Success");
        }
        else
        {
            handler->PSendSysMessage("Cast Failed");
            handler->SetSentErrorMessage(true);
            return false;
        }

        return true;
    }

};

void AddSC_followship_bots_commandscript()
{
    new followship_bots_commandscript();
}
