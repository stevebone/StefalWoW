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

#include "Player.h"
#include "Random.h"
#include "ScriptMgr.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"
#include "TemporarySummon.h"
#include "Unit.h"

#include "Custom_Mardum_Defines.h"

namespace Scripts::Custom::Mardum
{
    // 191466 - Illidari Reinforcements (5s periodic dummy, Mardum-only area aura)
    // In combat, periodically summons one of the three Illidari helpers for the
    // player - never a second one while any of them is still out.
    class spell_illidari_reinforcements : public AuraScript
    {
        void HandlePeriodic(AuraEffect const* /*aurEff*/)
        {
            Unit* target = GetTarget();
            if (!target->IsInCombat())
                return;

            for (uint32 entry : { Creatures::SummonedAshtongueStalker,
                                  Creatures::CoilskarSeaCaller,
                                  Creatures::ShivarraDestroyer })
            {
                std::list<TempSummon*> minions;
                target->GetAllMinionsByEntry(minions, entry);
                if (!minions.empty())
                    return;
            }

            static constexpr uint32 SummonSpells[3] =
            {
                Spells::SummonAshtongueStalker,
                Spells::SummonCoilskarSeaCaller,
                Spells::SummonShivarraDestroyer
            };
            target->CastSpell(target, SummonSpells[urand(0, 2)], true);
        }

        void Register() override
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_illidari_reinforcements::HandlePeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
        }
    };
}

void AddSC_custom_mardum_spells()
{
    using namespace Scripts::Custom::Mardum;

    RegisterSpellScript(spell_illidari_reinforcements);
}
