
#include "Chat.h"
#include "ChatCommand.h"
#include "WorldSession.h"

#include "Followship_bots_utils.h"

#include "Followship_bots_stats_handler.h"

using namespace Trinity::ChatCommands;

class followship_bots_commandscript : public CommandScript
{
public:
    followship_bots_commandscript() : CommandScript("followship_bots_commandscript") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> fsbCommandTable =
        {
            { "info", rbac::RBAC_PERM_COMMAND_GM, true, &HandleFSBInfo, "This is help text?"},
            { "stats", rbac::RBAC_PERM_COMMAND_GM, true, &HandleFSBStats, "This is help text?"},
        };

        static std::vector<ChatCommand> commandTable =
        {
            { "fsb", rbac::RBAC_PERM_COMMAND_GM, true, nullptr, "", fsbCommandTable }
        };

        return commandTable;
    }

    static bool HandleFSBStats(ChatHandler* handler, char const* /*args*/)
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
        float basePower = bot->GetFlatModifierValue(UnitMods(UNIT_MOD_POWER_START + AsUnderlyingType(powerType)), BASE_VALUE);
        float totalPower = bot->GetTotalAuraModValue(UnitMods(UNIT_MOD_POWER_START + AsUnderlyingType(powerType)));

        handler->PSendSysMessage("Power (%s): Base %u / %u", FSBUtils::PowerTypeToString(powerType), (int32)basePower, (int32)totalPower);


        float baseArmor = bot->GetFlatModifierValue(UNIT_MOD_ARMOR, BASE_VALUE);
        float totalArmor = bot->GetTotalAuraModValue(UNIT_MOD_ARMOR);

        handler->PSendSysMessage("Armor: Base: %.1f, Total: %.1f, Bonus: %.1f", baseArmor, totalArmor, totalArmor - baseArmor);

        float baseAttackPower = bot->GetFlatModifierValue(UNIT_MOD_ATTACK_POWER, BASE_VALUE);
        float totalAttackPower = bot->GetTotalAuraModValue(UNIT_MOD_ATTACK_POWER);

        handler->PSendSysMessage("Attack Power: Base %.1f / Total %.1f", baseAttackPower, totalAttackPower);

        float baseRAttackPower = bot->GetFlatModifierValue(UNIT_MOD_ATTACK_POWER_RANGED, BASE_VALUE);
        float totalRAttackPower = bot->GetTotalAuraModValue(UNIT_MOD_ATTACK_POWER_RANGED);

        handler->PSendSysMessage("Ranged Attack Power: Base %.1f / Total %.1f", baseRAttackPower, totalRAttackPower);

        int32 spellPower = FSBStats::BotGetSpellPower(bot);

        handler->PSendSysMessage("Spell Power: %u", spellPower);

        return true;
    }

    static bool HandleFSBInfo(ChatHandler* handler, char const* /*args*/)
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
        FSB_Class botClass = FSBUtils::GetBotClassForEntry(bot->GetEntry());
        FSB_Race botRace = FSBUtils::GetBotRaceForEntry(bot->GetEntry());

        

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

};

void AddSC_followship_bots_commandscript()
{
    new followship_bots_commandscript();
}
