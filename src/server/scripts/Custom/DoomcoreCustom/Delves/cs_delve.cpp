/*
 * DoomCore Scripts - Delve GM Commands
 *
 * .delve objective complete - Completes the current Scenario step in the player's map.
 */

#include "Chat.h"
#include "ChatCommand.h"
#include "Define.h"
#include "Player.h"
#include "RBAC.h"
#include "Scenario.h"
#include "ScriptMgr.h"
#include "WorldSession.h"

using namespace Trinity::ChatCommands;

class delve_commandscript : public CommandScript
{
public:
    delve_commandscript() : CommandScript("delve_commandscript") {}

    std::span<ChatCommandBuilder const> GetCommands() const override
    {
        static ChatCommandTable objectiveTable =
        {
            { "complete", HandleObjectiveComplete, rbac::RBAC_PERM_COMMAND_GM, Console::No },
        };

        static ChatCommandTable delveTable =
        {
            { "objective", objectiveTable },
        };

        static ChatCommandTable commandTable =
        {
            { "delve", delveTable },
        };
        return commandTable;
    }

    static bool HandleObjectiveComplete(ChatHandler* handler)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!player)
        {
            handler->SendSysMessage("[Delve] No player found.");
            handler->SetSentErrorMessage(true);
            return false;
        }

        Scenario* scenario = player->GetScenario();
        if (!scenario)
        {
            handler->SendSysMessage("[Delve] You are not in a Scenario.");
            handler->SetSentErrorMessage(true);
            return false;
        }

        ScenarioStepEntry const* currentStep = scenario->GetStep();
        if (!currentStep)
        {
            handler->SendSysMessage("[Delve] Scenario has no current step (already complete?).");
            handler->SetSentErrorMessage(true);
            return false;
        }

        handler->PSendSysMessage("[Delve] Completing scenario step %u (Order %u): Scenario %u",
            currentStep->ID, currentStep->OrderIndex, currentStep->ScenarioID);

        scenario->SetStepState(currentStep, SCENARIO_STEP_DONE);
        scenario->CompleteStep(currentStep);
        ScenarioStepEntry const* newStep = scenario->GetStep();
        if (newStep)
            handler->PSendSysMessage("[Delve] New current step: %u (Order %u)", newStep->ID, newStep->OrderIndex);
        else if (scenario->IsComplete())
            handler->SendSysMessage("[Delve] Scenario is now complete!");
        else
            handler->SendSysMessage("[Delve] No next step found.");

        return true;
    }

};

void AddSC_delve_commands()
{
    new delve_commandscript();
}
