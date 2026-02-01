
#include "Chat.h"
#include "ChatCommand.h"
#include "WorldSession.h"

#include "Followship_bots_utils.h"

using namespace Trinity::ChatCommands;

class followship_bots_commandscript : public CommandScript
{
public:
    followship_bots_commandscript() : CommandScript("followship_bots_commandscript") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> fsbCommandTable =
        {
            { "info", rbac::RBAC_PERM_COMMAND_GM, true, &HandleFSBInfo, "" }
        };

        static std::vector<ChatCommand> commandTable =
        {
            { "fsb", rbac::RBAC_PERM_COMMAND_GM, true, nullptr, "", fsbCommandTable }
        };

        return commandTable;
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

        Powers powerType = bot->GetPowerType();

        handler->PSendSysMessage("=== Followship Bot Info ===");
        handler->PSendSysMessage("Name: %s", bot->GetName().c_str());
        handler->PSendSysMessage("Level: %u", bot->GetLevel());
        handler->PSendSysMessage("Class: %s", FSBUtils::BotClassToString(botClass));
        handler->PSendSysMessage("Race: %s", FSBUtils::BotRaceToString(botRace));

        handler->PSendSysMessage("Health: Created: %u Max: %u (%.1f%%)",
            bot->GetCreateHealth(),
            bot->GetMaxHealth(),
            bot->GetHealthPct());

        handler->PSendSysMessage("Power (%s): %u / %u",
            FSBUtils::PowerTypeToString(powerType),
            bot->GetPower(powerType),
            bot->GetMaxPower(powerType));

        handler->PSendSysMessage("Attack Power: %.1f and Ranged Attack Power: %.1f ", bot->GetTotalAttackPowerValue(BASE_ATTACK), bot->GetTotalAttackPowerValue(RANGED_ATTACK));
        handler->PSendSysMessage("Armor: Base %.1f Total %u", bot->GetBaseArmorForLevel(bot->GetLevel()), bot->GetArmor());
        handler->PSendSysMessage("Damage: Base: %.1f Min: %.1f Max: %.1f", bot->GetBaseDamageForLevel(bot->GetLevel()), bot->GetWeaponDamageRange(BASE_ATTACK, MINDAMAGE), bot->GetWeaponDamageRange(BASE_ATTACK, MAXDAMAGE));
        handler->PSendSysMessage("Ranged Damage: Base: %.1f Min: %.1f Max: %.1f", bot->GetBaseDamageForLevel(bot->GetLevel()), bot->GetWeaponDamageRange(RANGED_ATTACK, MINDAMAGE), bot->GetWeaponDamageRange(RANGED_ATTACK, MAXDAMAGE));
        handler->PSendSysMessage("Offhand Damage: Base: %.1f Min: %.1f Max: %.1f", bot->GetBaseDamageForLevel(bot->GetLevel()), bot->GetWeaponDamageRange(OFF_ATTACK, MINDAMAGE), bot->GetWeaponDamageRange(OFF_ATTACK, MAXDAMAGE));

        handler->PSendSysMessage("In Combat: %s", bot->IsInCombat() ? "Yes" : "No");

        return true;
    }

};

void AddSC_followship_bots_commandscript()
{
    new followship_bots_commandscript();
}
