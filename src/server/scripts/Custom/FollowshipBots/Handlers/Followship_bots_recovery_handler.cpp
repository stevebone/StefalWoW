
#include "Followship_bots_recovery_handler.h"

namespace FSBRecovery
{
    BotRecoveryIntent DetermineRecoveryIntent(Creature* bot)
    {
        bool lowHP = bot->GetHealthPct() < BOT_RECOVERY_HP_PCT;
        bool lowMana = bot->GetPowerPct(POWER_MANA) < BOT_RECOVERY_MP_PCT;

        if (lowHP && lowMana)
            return BotRecoveryIntent::RecoverHealthAndMana;
        if (lowHP)
            return BotRecoveryIntent::RecoverHealth;
        if (lowMana)
            return BotRecoveryIntent::RecoverMana;

        return BotRecoveryIntent::None;
    }

    bool MatchesIntent(BotRecoveryType type, BotRecoveryIntent intent)
    {
        switch (intent)
        {
        case BotRecoveryIntent::RecoverHealth:
            return type == BotRecoveryType::Health || type == BotRecoveryType::Both;

        case BotRecoveryIntent::RecoverMana:
            return type == BotRecoveryType::Mana || type == BotRecoveryType::Both;

        case BotRecoveryIntent::RecoverHealthAndMana:
            return true;

        default:
            return false;
        }
    }

    void BuildRecoveryActions(Creature* bot, RecoveryActionList& _recoveryActions)
    {
        _recoveryActions.emplace_back(new ActionDrinkMana());

        if (FSBUtils::GetBotClassForEntry(bot->GetEntry()) == FSB_Class::Mage)
            _recoveryActions.emplace_back(new ActionMageConjuredDrink());

        if (FSBUtils::GetBotRaceForEntry(bot->GetEntry()) == FSB_Race::Human)
            _recoveryActions.emplace_back(new ActionHumanRecuperate());

        if (FSBUtils::GetBotClassForEntry(bot->GetEntry()) == FSB_Class::Priest)
            _recoveryActions.emplace_back(new ActionPriestHeal());
    }

    void TryRecovery(Creature* bot, RecoveryActionList& _recoveryActions, uint32 _globalCooldownUntil, bool& _isRecovering, uint32& _recoveryLockUntil)
    {
        if (bot->IsInCombat())
            return;

        if (!bot->IsAlive())
            return;

        BotRecoveryIntent intent = FSBRecovery::DetermineRecoveryIntent(bot);

        if (intent == BotRecoveryIntent::None)
            return;

        TC_LOG_DEBUG("scripts.ai.fsb", "FSB: RecoveryHandler TryRecover intent check for: {}", bot->GetName());

        // -- Shuffle the actions before executing --
        Trinity::Containers::RandomShuffle(_recoveryActions);

        TryRecover(bot, intent, _recoveryActions, _globalCooldownUntil, _isRecovering, _recoveryLockUntil);
    }

    void TryRecover(Creature* bot, BotRecoveryIntent intent, RecoveryActionList& _recoveryActions, uint32 _globalCooldownUntil, bool& _isRecovering, uint32& _recoveryLockUntil)
    {
        uint32 now = getMSTime();

        if (_isRecovering)
        {
            if (now < _recoveryLockUntil)
                return; // still busy

            _isRecovering = false; // finished recovery action
        }

        for (auto const& action : _recoveryActions)
        {
            if (!MatchesIntent(action->type, intent))
                continue;

            if (!action->CanExecute(bot, _globalCooldownUntil))
                continue;

            action->Execute(bot, _globalCooldownUntil);
            TC_LOG_DEBUG("scripts.ai.fsb", "FSB: RecoveryHandler TryRecover execute action for: {}", bot->GetName());

            _isRecovering = true;
            _recoveryLockUntil = getMSTime() + action->GetCooldownMs();

            break; // one action per tick
        }
    }

}
