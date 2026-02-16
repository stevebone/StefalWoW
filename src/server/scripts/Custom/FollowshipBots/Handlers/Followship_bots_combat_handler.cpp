#include "Followship_bots_combat_handler.h"

namespace FSBCombat
{
    bool ShouldSwitchToMelee(Creature* bot)
    {
        if (!bot)
            return false;

        if (!bot->IsInCombat())
            return false;

        Unit* target = bot->GetVictim();
        if (!target)
            return false;

        // Mana threshold - tweak as needed
        uint32 mana = bot->GetPower(POWER_MANA);
        uint32 maxMana = bot->GetMaxPower(POWER_MANA);

        if (maxMana > 0 && mana > maxMana * 0.05f) // >5% mana
            return false;

        // Already in melee range?
        if (bot->IsWithinMeleeRange(target))
            return false;

        return true;
    }

    void EnterMeleeMode(Creature* bot)
    {
        if (!bot)
            return;

        Unit* target = bot->GetVictim();
        if (!target)
            return;

        // Stop casting
        bot->InterruptNonMeleeSpells(false);

        // Move into melee range
        bot->GetMotionMaster()->Clear();
        bot->GetMotionMaster()->MoveChase(target, 0.5f);

        // Enable auto-attack
        bot->Attack(target, true);

        TC_LOG_DEBUG("scripts.ai.fsb", "FSB: {} switching to melee mode (OOM)", bot->GetName());
    }
}
