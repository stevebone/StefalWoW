
#include "Followship_bots_powers_handler.h"
#include "Followship_bots_stats_handler.h"
#include "Followship_bots_utils.h"

#include "Followship_bots_druid.h"

namespace FSBPowers
{
    void UpdateMaxPower(Creature* bot, Powers power)
    {
        if (bot->GetPowerIndex(power) == MAX_POWERS)
            return;

        UnitMods unitMod = UnitMods(UNIT_MOD_POWER_START + AsUnderlyingType(power));

        float value = bot->GetFlatModifierValue(unitMod, BASE_VALUE);
        value *= bot->GetPctModifierValue(unitMod, BASE_PCT);
        value += bot->GetFlatModifierValue(unitMod, TOTAL_VALUE);
        value *= bot->GetPctModifierValue(unitMod, TOTAL_PCT);

        if (IsRageUser(bot))
        {
            power = POWER_RAGE;
            value = 1000.f;
        }

        if (IsEnergyUser(bot))
        {
            power = POWER_ENERGY;
            value = 1000.f;
        }

        bot->SetMaxPower(power, (int32)std::lroundf(value));
    }

    Powers GetBotPowerType(Creature* bot)
    {
        if (!bot)
            return POWER_MANA;

        FSB_Class cls = FSBUtils::GetBotClassForEntry(bot->GetEntry());

        for (auto const& stats : BotClassStatsTable)
        {
            if (stats.classId == cls)
                return stats.powerType;
        }

        return POWER_MANA; // safe fallback
    }

    bool SpendPowerPct(Creature* bot, float pct)
    {
        if (!bot || pct <= 0.0f)
            return false;

        Powers power = bot->GetPowerType();
        uint32 maxPower = bot->GetMaxPower(power);
        if (!maxPower)
            return false;

        int32 cost = uint32(maxPower * pct);

        if (bot->GetPower(power) < cost)
            return false;

        bot->ModifyPower(power, -int32(cost));

        TC_LOG_DEBUG("scripts.ai.fsb", "FSB: Combat spellcasting: Reduced power mana by {} for bot: {}", cost, bot->GetName());

        return true;
    }

    bool SpendManaFlat(Creature* bot, int32 cost)
    {
        if (!bot)
            return false;

        if (bot->GetPower(POWER_MANA) < cost)
            return false;

        bot->ModifyPower(POWER_MANA, -int32(cost));
        return true;
    }

    bool IsRageUser(Creature* bot)
    {
        return FSBUtils::GetBotClassForEntry(bot->GetEntry()) == FSB_Class::Warrior || bot->HasAura(SPELL_DRUID_BEAR);
    }

    bool IsEnergyUser(Creature* bot)
    {
        return FSBUtils::GetBotClassForEntry(bot->GetEntry()) == FSB_Class::Rogue || bot->HasAura(SPELL_DRUID_CAT);
    }

    void GenerateRageFromDamageTaken(Creature* bot, uint32 damage)
    {
        if (!damage || !bot->IsAlive())
            return;

        if (!IsRageUser(bot))
            return;

        // Rage from damage taken
        uint32 rageGain = uint32(damage * RAGE_FROM_DAMAGE_COEFF);

        if (rageGain == 0)
            rageGain = 1; // always give *something*

        uint32 currentRage = bot->GetPower(POWER_RAGE);
        uint32 maxRage = bot->GetMaxPower(POWER_RAGE);

        uint32 newRage = std::min(currentRage + rageGain, maxRage);

        bot->ModifyPower(POWER_RAGE, rageGain, false);

        TC_LOG_DEBUG("scripts.ai.fsb",
            "FSB: {} gained {} rage from taking {} damage (now {})",
            bot->GetName(), rageGain, damage, newRage);
    }

    void GenerateRageFromDamageDone(Creature* bot, uint32 damage)
    {
        if (!bot || damage == 0 || !bot->IsAlive())
            return;

        if (!IsRageUser(bot))
            return;

        int32 maxRage = int32(bot->GetMaxPower(POWER_RAGE));
        if (maxRage <= 0)
            return;

        // Tunable coefficient
        int32 rageGain = int32(float(damage) * RAGE_FROM_DAMAGE_COEFF);

        if (rageGain <= 0)
            rageGain = 1; // minimum feedback

        bot->ModifyPower(POWER_RAGE, rageGain);

        TC_LOG_DEBUG(
            "scripts.ai.fsb",
            "FSB: {} gained {} rage from dealing {} damage (now {})",
            bot->GetName(),
            rageGain,
            damage,
            bot->GetPower(POWER_RAGE)
        );
    }
}
