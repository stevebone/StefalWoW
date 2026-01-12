#include "followship_bots_utils_stats.h"

namespace FSBUtilsStats
{
    // ---- Default regen for all bots ----
    const FSBUtilsStatsRegen DefaultRegen{
        0.0f,  // healthPctPerTickCombat
        1.0f,  // healthPctPerTickOutOfCombat
        1.0f,  // manaPctPerTickCombat
        2.0f,  // manaPctPerTickOutOfCombat
        false  // hasClassSpecific
    };

    // ---- Priest-specific regen ----
    const FSBUtilsStatsRegen PriestRegen{
        0.0f,  // healthPctPerTickCombat
        1.5f,  // healthPctPerTickOutOfCombat
        2.0f,  // manaPctPerTickCombat
        3.0f,  // manaPctPerTickOutOfCombat
        true
    };

    // ---------- Basic helpers ----------
    void ApplyHealthRegen(Unit* unit)
    {
        FSBUtilsStatsMods emptyMods;
        ApplyRegen(unit, emptyMods, true, false);
    }

    void ApplyManaRegen(Unit* unit)
    {
        FSBUtilsStatsMods emptyMods;
        ApplyRegen(unit, emptyMods, false, true);
    }

    void ApplyRegen(Unit* unit)
    {
        FSBUtilsStatsMods emptyMods;
        ApplyRegen(unit, emptyMods, true, true);
    }

    // ---------- Full function with mods ----------
    void ApplyRegen(Unit* unit, const FSBUtilsStatsMods& mods, bool doHealth, bool doMana)
    {
        if (!unit || !unit->IsAlive())
            return;

        const FSBUtilsStatsRegen* params = &DefaultRegen;
        if (unit->GetClass() == CLASS_PRIEST)
            params = &PriestRegen;

        bool inCombat = unit->IsInCombat();

        // ---------- HEALTH ----------
        if (doHealth)
        {
            uint32 maxHp = unit->GetMaxHealth();
            if (maxHp)
            {
                float pct = inCombat ? params->healthPctPerTickCombat : params->healthPctPerTickOutOfCombat;
                pct += mods.healthPctBonus;

                int32 amount = int32(maxHp * (pct / 100.f)) + mods.flatHealthPerTick;

                if (amount > 0)
                    unit->ModifyHealth(amount);
            }
        }

        // ---------- MANA ----------
        if (doMana && unit->GetPowerType() == POWER_MANA)
        {
            uint32 maxMana = unit->GetMaxPower(POWER_MANA);
            if (maxMana)
            {
                float pct = inCombat ? params->manaPctPerTickCombat : params->manaPctPerTickOutOfCombat;
                pct += mods.manaPctBonus;

                int32 amount = int32(maxMana * (pct / 100.f)) + mods.flatManaPerTick;

                if (amount > 0)
                    unit->ModifyPower(POWER_MANA, amount);
            }
        }
    }

    void ApplyMaxHealth(Unit* unit, const FSBUtilsStatsMods& mods)
    {
        if (!unit)
            return;

        uint32 baseMaxHealth = unit->GetCreateHealth();
        int32 bonusFlat = mods.flatMaxHealth;
        float bonusPct = mods.pctMaxHealthBonus;

        uint32 newMaxHealth = baseMaxHealth + bonusFlat;
        if (bonusPct > 0.0f)
            newMaxHealth += uint32(float(baseMaxHealth) * bonusPct);

        if (unit->GetMaxHealth() != newMaxHealth)
        {
            unit->SetMaxHealth(newMaxHealth);

            if (unit->GetHealth() > newMaxHealth)
                unit->SetHealth(newMaxHealth);
        }
    }

    void RecalculateStats(Unit* unit, const FSBUtilsStatsMods& mods)
    {
        ApplyMaxHealth(unit, mods);
        // Future:
        // ApplyArmor(...)
        // ApplySpellPower(...)
    }

    void UpdateBotLevelToPlayer(Creature* bot, Player* player, const FSBUtilsStatsMods& regenMods)
    {
        if (!bot || !player)
            return;

        uint8 pLevel = player->GetLevel();
        if (bot->GetLevel() == pLevel)
            return; // nothing to do

        bot->SetLevel(pLevel);

        // Update stats for new level
        bot->UpdateLevelDependantStats();

        // Reset health & power
        bot->SetHealth(bot->GetMaxHealth());
        bot->SetPower(bot->GetPowerType(), bot->GetMaxPower(bot->GetPowerType()));

        // Recalculate any additional stats/modifiers
        RecalculateStats(bot, regenMods);
    }
}
