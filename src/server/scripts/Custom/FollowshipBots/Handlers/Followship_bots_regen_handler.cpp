#include "Followship_bots_utils.h"

#include "Followship_bots_powers_handler.h"
#include "Followship_bots_regen_handler.h"
#include "Followship_bots_stats_handler.h"


namespace FSBRegen
{
    void ApplyBotRegen(Unit* unit, FSB_Class botClass, FSBRegenMods regenMods, bool doHealth, bool doMana)
    {
        if (!unit || !unit->IsAlive())
            return;

        bool inCombat = unit->IsInCombat() || unit->HasUnitFlag(UNIT_FLAG_IN_COMBAT);

        // Fetch class base stats
        auto const* classStats = FSBStats::GetBotClassStats(botClass);
        if (!classStats)
            return;

        // ---------- HEALTH ----------
        if (doHealth && unit->GetHealthPct() < 100)
        {
            int32 baseHpRegen = inCombat
                ? classStats->baseHpRegenIC
                : classStats->baseHpRegenOOC;

            int32 amount = baseHpRegen + regenMods.flatHealthPerTick;


            if (regenMods.pctHealthPerTick > 0.0f)
            {
                uint32 maxHp = unit->GetMaxHealth();
                amount += int32(maxHp * regenMods.pctHealthPerTick);
            }

            if (amount > 0)
                unit->ModifyHealth(amount);

            //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Custom Regen tick for bot {} (HP={}) in combat: {}", unit->GetName(), amount, inCombat);
        }

        // ---------- POWER ----------
        if (doMana && unit->GetPowerType() == POWER_MANA && unit->GetPowerPct(POWER_MANA) < 100)
        {
            int32 basePowerRegen = inCombat
                ? classStats->basePowerRegenIC
                : classStats->basePowerRegenOOC;

            int32 amount = basePowerRegen + regenMods.flatManaPerTick;

            if (regenMods.pctManaPerTick > 0.0f)
            {
                uint32 maxPower = unit->GetMaxPower(POWER_MANA);
                amount += int32(maxPower * regenMods.pctManaPerTick);
            }

            if (amount > 0)
                unit->ModifyPower(POWER_MANA, amount);

            //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Custom Regen tick for bot {} (MP={}) in combat: {}", unit->GetName(), amount, inCombat);
        }

        else if (FSBPowers::IsRageUser(unit->ToCreature()))
        {
            // need this for druid bear form which has positive power regen
            int32 rageRegenOOC = classStats->basePowerRegenOOC;
            if (rageRegenOOC > 0)
                rageRegenOOC = -2;


            int32 basePowerRegen = inCombat
                ? classStats->basePowerRegenIC
                : rageRegenOOC;

            int32 maxPower = int32(unit->GetMaxPower(POWER_RAGE));

            int32 amount = (basePowerRegen * maxPower) / 100;

            if (regenMods.flatRagePerTick > 0 && unit->IsInCombat())
                amount = basePowerRegen + regenMods.flatRagePerTick;

            if (amount != 0)
                unit->ModifyPower(POWER_RAGE, amount);

            //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Custom Regen tick for bot {} (RP={}) in combat: {}", unit->GetName(), amount, inCombat);
        }
    }

    void ProcessBotCustomRegenTick(Creature* creature, FSB_Class botClass, FSBRegenMods regenMods)
    {
        if (!creature || !creature->IsBot() || !creature->IsAlive())
            return;

        // Only apply regen if HP or power is not full
        if (creature->GetHealthPct() < 100 || creature->GetPowerPct(POWER_MANA) < 100 || creature->GetPower(POWER_RAGE))
        {
            // Apply regen
            ApplyBotRegen(creature, botClass, regenMods, true, true);

        }
    }
}
