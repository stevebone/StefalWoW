
#include "Followship_bots_regen_handler.h"
#include "Followship_bots_utils_stats.h"

namespace FSBRegen
{
    void ApplyBotRegen(Unit* unit, FSB_Class botClass, const FSBUtilsStatsMods& mods, bool doHealth, bool doMana)
    {
        if (!unit || !unit->IsAlive())
            return;

        bool inCombat = unit->IsInCombat() || unit->HasUnitFlag(UNIT_FLAG_IN_COMBAT);

        // Fetch class base stats
        FSB_ClassStats const* classStats = FSBUtilsStats::GetBotClassStats(botClass);
        if (!classStats)
            return;

        // ---------- HEALTH ----------
        if (doHealth && unit->GetHealthPct() < 100)
        {
            int32 baseHpRegen = inCombat
                ? classStats->baseHpRegenIC
                : classStats->baseHpRegenOOC;

            int32 amount = baseHpRegen + mods.flatHealthPerTick;

            if (mods.pctMaxHealthBonus > 0.0f)
            {
                uint32 maxHp = unit->GetMaxHealth();
                amount += int32(maxHp * mods.pctMaxHealthBonus);
            }

            if (mods.pctHealthPerTick > 0.0f)
            {
                uint32 maxHp = unit->GetMaxHealth();
                amount += int32(maxHp * mods.pctHealthPerTick);
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

            int32 amount = basePowerRegen + mods.flatManaPerTick;

            if (mods.pctMaxManaBonus > 0.0f)
            {
                uint32 maxPower = unit->GetMaxPower(POWER_MANA);
                amount += int32(maxPower * mods.pctMaxManaBonus);
            }

            if (mods.pctManaPerTick > 0.0f)
            {
                uint32 maxPower = unit->GetMaxPower(POWER_MANA);
                amount += int32(maxPower * mods.pctManaPerTick);
            }

            if (amount > 0)
                unit->ModifyPower(POWER_MANA, amount);

            //TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Custom Regen tick for bot {} (MP={}) in combat: {}", unit->GetName(), amount, inCombat);
        }

        else if (unit->GetPowerType() == POWER_RAGE)
        {
            int32 basePowerRegen = inCombat
                ? classStats->basePowerRegenIC
                : classStats->basePowerRegenOOC;

            int32 maxPower = int32(unit->GetMaxPower(POWER_RAGE));

            int32 amount = (basePowerRegen * maxPower) / 100;

            if (mods.flatRagePerTick > 0 && unit->IsInCombat())
                amount = classStats->basePowerRegenIC + mods.flatRagePerTick;

            if (amount != 0)
                unit->ModifyPower(POWER_RAGE, amount);

            TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Custom Regen tick for bot {} (RP={}) in combat: {}", unit->GetName(), amount, inCombat);
        }
    }

    void ProcessBotCustomRegenTick(Creature* creature, FSB_Class botClass, const FSBUtilsStatsMods& _baseStatsMods, const FSBUtilsStatsMods& _statsMods)
    {
        if (!creature || !creature->IsBot() || !creature->IsAlive())
            return;

        // Only apply regen if HP or power is not full
        if (creature->GetHealthPct() < 100 || creature->GetPowerPct(POWER_MANA) < 100 || creature->GetPower(POWER_RAGE))
        {
            // Merge base + aura mods
            FSBUtilsStatsMods finalMods = _baseStatsMods;
            finalMods += _statsMods;

            // Apply regen
            ApplyBotRegen(creature, botClass, finalMods, true, true);

        }
    }
}
