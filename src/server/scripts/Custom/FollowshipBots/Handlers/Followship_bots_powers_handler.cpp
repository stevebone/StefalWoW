/*
 * This file is part of the Stefal WoW Project.
 * It is designed to work exclusively with the TrinityCore framework.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * This code is provided for personal and educational use within the
 * Stefal WoW Project. It is not intended for commercial distribution,
 * resale, or any form of monetization.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Log.h"

#include "Followship_bots_powers_handler.h"
#include "Followship_bots_stats_handler.h"

#include "Followship_bots_mgr.h"
#include "Followship_bots_utils.h"

namespace FSBPowers
{
    Powers GetBotPowerType(Creature* bot)
    {
        if (!bot)
            return POWER_MANA;

        FSB_Class cls = FSBMgr::Get()->GetBotClassForEntry(bot->GetEntry());

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

        bot->ModifyPower(power, -int32(cost), true);

        TC_LOG_DEBUG("scripts.fsb.spells", "FSB: SpendPowerPct: Reduced power {} by {} for bot: {}", FSBUtils::PowerTypeToString(power), cost, bot->GetName());

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
        return FSBMgr::Get()->GetBotClassForEntry(bot->GetEntry()) == FSB_Class::Warrior || bot->HasAura(SPELL_DRUID_BEAR);
    }

    bool IsEnergyUser(Creature* bot)
    {
        auto baseAI = dynamic_cast<FSB_BaseAI*>(bot->AI());
        if (!baseAI)
            return false;

        auto botClass = baseAI->botClass;
        auto botRole = baseAI->botRole;

        return botClass == FSB_Class::Rogue || bot->HasAura(SPELL_DRUID_CAT) || (botClass == FSB_Class::Monk && botRole == FSB_ROLE_MELEE_DAMAGE);
    }

    bool IsFocusUser(Creature* bot)
    {
        return FSBMgr::Get()->GetBotClassForEntry(bot->GetEntry()) == FSB_Class::Hunter;
    }

    void SetBotToEnergy(Creature* bot)
    {
        if (!bot || !bot->IsAlive())
            return;

        bot->SetPowerType(POWER_ENERGY, true);
        bot->SetMaxPower(POWER_ENERGY, 100);
    }

    void SetBotToMana(Creature* bot)
    {
        if (!bot || !bot->IsAlive())
            return;

        bot->SetPowerType(POWER_MANA, true);
        bot->SetMaxPower(POWER_MANA, bot->GetMaxPower(POWER_MANA));
        bot->SetPower(POWER_MANA, bot->GetMaxPower(POWER_MANA));
    }

    void SetBotToChi(Creature* bot)
    {
        if (!bot || !bot->IsAlive())
            return;

        bot->SetPowerType(POWER_CHI, true);
        bot->SetMaxPower(POWER_CHI, 5);
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

        TC_LOG_DEBUG("scripts.fsb.combat",
            "FSB: GenerateRageFromDamageTaken Bot {} gained {} rage from taking {} damage (now {})",
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
            "scripts.fsb.combat",
            "FSB: GenerateRageFromDamageTaken Bot {} gained {} rage from dealing {} damage (now {})",
            bot->GetName(),
            rageGain,
            damage,
            bot->GetPower(POWER_RAGE)
        );
    }
}
