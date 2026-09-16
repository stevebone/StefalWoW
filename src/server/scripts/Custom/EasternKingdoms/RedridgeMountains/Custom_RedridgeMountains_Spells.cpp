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

#include "CreatureAI.h"
#include "GameObject.h"
#include "ObjectAccessor.h"
#include "PhasingHandler.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellAuras.h"
#include "TemporarySummon.h"

#include "Custom_RedridgeMountains_Defines.h"

namespace Scripts::EasternKingdoms::RedridgeMountains
{
    // 82580 - Bravo Company Field Kit
    class spell_bravo_company_field_kit : public SpellScript
    {
        SpellCastResult CheckCast()
        {
            Unit* caster = GetCaster();
            if (!caster)
                return SPELL_FAILED_DONT_REPORT;

            if (caster->HasAura(Spells::BravoCompanyFieldKit))
            {
                caster->CastSpell(caster, Spells::CancelBravoCompanyFieldKit, true);
                return SPELL_FAILED_DONT_REPORT;
            }

            return SPELL_CAST_OK;
        }

        void HandleHit()
        {
            Player* player = GetCaster()->ToPlayer();
            if (!player)
                return;

            std::list<TempSummon*> minions;
            player->GetAllMinionsByEntry(minions, Creatures::JorgensenGuardian);
            for (TempSummon* minion : minions)
            {
                // Am using SetData to trigger the broadcast
                minion->AI()->SetData(1, 0);
            }
        }

        void Register() override
        {
            OnCheckCast += SpellCheckCastFn(spell_bravo_company_field_kit::CheckCast);
            OnHit += SpellHitFn(spell_bravo_company_field_kit::HandleHit);
        }
    };

    // 82578 - Distraction
    class spell_distraction : public SpellScript
    {
        void HandleHit()
        {
            Player* player = GetCaster()->ToPlayer();
            if (!player)
                return;

            std::list<TempSummon*> minions;
            player->GetAllMinionsByEntry(minions, Creatures::JorgensenGuardian);
            for (TempSummon* minion : minions)
                minion->AI()->SetData(1, 6);
        }

        void Register() override
        {
            OnHit += SpellHitFn(spell_distraction::HandleHit);
        }
    };

    // 82587 - Bravo Company Field Kit
    class spell_bravo_company_field_kit2 : public SpellScript
    {
        SpellCastResult CheckCast()
        {
            Unit* caster = GetCaster();
            if (!caster)
                return SPELL_FAILED_DONT_REPORT;

            if (caster->HasAura(Spells::BravoCompanyFieldKit2))
            {
                caster->CastSpell(caster, Spells::CancelBravoCompanyFieldKit2, true);
                return SPELL_FAILED_DONT_REPORT;
            }

            return SPELL_CAST_OK;
        }

        void HandleHit()
        {
            Player* player = GetCaster()->ToPlayer();
            if (!player)
                return;

            std::list<TempSummon*> minions;
            player->GetAllMinionsByEntry(minions, Creatures::JorgensenGuardian);
            for (TempSummon* minion : minions)
            {
                // Am using SetData to trigger the broadcast
                minion->AI()->SetData(1, 9);
            }
        }

        void Register() override
        {
            OnCheckCast += SpellCheckCastFn(spell_bravo_company_field_kit2::CheckCast);
            OnHit += SpellHitFn(spell_bravo_company_field_kit2::HandleHit);
        }
    };

    // 82585 - Plant Seaforium
    class spell_plant_seaforium : public SpellScript
    {
        void HandleAfterCast()
        {
            Player* player = GetCaster()->ToPlayer();
            if (!player)
                return;

            GameObject* focusGO = player->FindNearestGameObject(GameObjects::PlantSeaforiumHere, 10.0f);
            if (!focusGO)
                return;

            if (GameObject* bombGO = player->SummonGameObject(GameObjects::SeaforiumBomb, focusGO->GetPosition(),
                QuaternionData::fromEulerAnglesZYX(focusGO->GetOrientation(), 0.0f, 0.0f), 0s))
            {
                ObjectGuid playerGuid = player->GetGUID();
                bombGO->m_Events.AddEventAtOffset([playerGuid, bombGO]()
                    {
                        Player* player = ObjectAccessor::FindPlayer(playerGuid);
                        if (player)
                        {
                            bombGO->Use(player);
                            bombGO->DespawnOrUnsummon(2min);
                        }
                    }, 1s);
            }

            GameObject* explosiveDevice = player->FindNearestGameObject(GameObjects::BlackrockExplosiveDevice, 10.0f);
            if (explosiveDevice)
            {
                player->KilledMonsterCredit(Creatures::MunitionsDump);
                if (Creature* jorgensen = player->FindNearestCreature(Creatures::JorgensenGuardian, 10.0f))
                    jorgensen->AI()->SetData(1, 10);
            }
            else
            {
                player->KilledMonsterCredit(Creatures::BlackrockTower);
                if (Creature* jorgensen = player->FindNearestCreature(Creatures::JorgensenGuardian, 10.0f))
                    jorgensen->AI()->SetData(1, 11);
            }
        }

        void Register() override
        {
            AfterCast += SpellCastFn(spell_plant_seaforium::HandleAfterCast);
        }
    };

    // 81888 - Ram (Bravo Company Siege Tank)
    class spell_bravo_company_siege_tank_ram : public SpellScript
    {
        void CalculateDamage(SpellEffectInfo const& /*spellEffectInfo*/, Unit* /*victim*/, int32& damage, int32& /*flatMod*/, float& /*pctMod*/) const
        {
            // Override raw base points (138) with the client tooltip value (11370),
            // which is level-independent and what the spell is intended to deal.
            damage = 11370;
        }

        void Register() override
        {
            CalcDamage += SpellCalcDamageFn(spell_bravo_company_siege_tank_ram::CalculateDamage);
        }
    };

    // 81870 - Machine Gun (Bravo Company Siege Tank - Keeshan's Gun)
    class spell_bravo_company_machine_gun : public SpellScript
    {
        void CalculateDamage(SpellEffectInfo const& /*spellEffectInfo*/, Unit* /*victim*/, int32& damage, int32& /*flatMod*/, float& /*pctMod*/) const
        {
            // Override raw base points (272)
            damage = 11370;
        }

        void Register() override
        {
            CalcDamage += SpellCalcDamageFn(spell_bravo_company_machine_gun::CalculateDamage);
        }
    };

    // 81808 - Summon Bravo Company Siege Tank
    // Blocks the spell's built-in summon effect (effect 1) and summons the tank (43734),
    // gun (43745) and Keeshan (43744) manually, then mounts them via delayed m_Events.
    class spell_summon_bravo_company_siege_tank : public SpellScript
    {
        void BlockSummonEffect(SpellEffIndex /*effIndex*/)
        {
            PreventHitDefaultEffect(EFFECT_1);
        }

        void HandleAfterCast()
        {
            Player* player = GetCaster()->ToPlayer();
            if (!player)
                return;

            TempSummon* tank = player->SummonCreature(Creatures::BravoCompanySiegeTankSpawn, Positions::SiegeTankSummon, TEMPSUMMON_MANUAL_DESPAWN, 0s, 989);
            if (!tank)
                return;

            TempSummon* gun = player->SummonCreature(Creatures::KeeshanGun, tank->GetPosition(), TEMPSUMMON_MANUAL_DESPAWN, 0s, 988);
            if (!gun)
                return;

            TempSummon* keeshan = player->SummonCreature(Creatures::Keeshan, gun->GetPosition());
            if (!keeshan)
                return;

            PhasingHandler::AddPhase(tank, 242, true);
            PhasingHandler::AddPhase(gun, 242, true);
            PhasingHandler::AddPhase(keeshan, 242, true);

            // Gun boards the tank on seat 1 after the gun is fully in-world.
            gun->m_Events.AddEventAtOffset([gun, tank]()
                {
                    if (gun && tank)
                        gun->EnterVehicle(tank, 1);
                }, 500ms);

            // Keeshan boards the gun on seat 0 after the gun has boarded the tank.
            keeshan->m_Events.AddEventAtOffset([keeshan, gun]()
                {
                    if (keeshan && gun)
                        keeshan->EnterVehicle(gun, 0);
                }, 1s);

            player->EnterVehicle(tank, 0);
        }

        void Register() override
        {
            OnEffectLaunch += SpellEffectFn(spell_summon_bravo_company_siege_tank::BlockSummonEffect, EFFECT_1, SPELL_EFFECT_SUMMON);
            AfterCast += SpellCastFn(spell_summon_bravo_company_siege_tank::HandleAfterCast);
        }
    };
}

void AddSC_custom_redridge_mountains_spells()
{
    using namespace Scripts::EasternKingdoms::RedridgeMountains;

    RegisterSpellScript(spell_bravo_company_field_kit);
    RegisterSpellScript(spell_distraction);
    RegisterSpellScript(spell_bravo_company_field_kit2);
    RegisterSpellScript(spell_plant_seaforium);
    RegisterSpellScript(spell_bravo_company_siege_tank_ram);
    RegisterSpellScript(spell_bravo_company_machine_gun);
    RegisterSpellScript(spell_summon_bravo_company_siege_tank);
}
