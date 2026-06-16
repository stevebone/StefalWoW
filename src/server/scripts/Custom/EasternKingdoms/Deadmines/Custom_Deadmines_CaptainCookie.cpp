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

#include "AchievementMgr.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "DB2Stores.h"
#include "InstanceScript.h"
#include "MotionMaster.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellAuras.h"
#include "SpellScript.h"

#include "Custom_Instance_Deadmines.h"

namespace Scripts::EasternKingdoms::Deadmines
{
    static constexpr uint32 ThrowFoodSpells[12] =
    {
        Spells::ThrowFood01,
        Spells::ThrowFood02,
        Spells::ThrowFood03,
        Spells::ThrowFood04,
        Spells::ThrowFood05,
        Spells::ThrowFood06,
        Spells::ThrowFood07,
        Spells::ThrowFood08,
        Spells::ThrowFood09,
        Spells::ThrowFood10,
        Spells::ThrowFood11,
        Spells::ThrowFood12,
    };

    static constexpr uint32 POINT_MOVE = 1;

    struct boss_captain_cookie : public BossAI
    {
        boss_captain_cookie(Creature* creature) : BossAI(creature, DataTypes::BOSS_CAPTAIN_COOKIE)
        {
            ApplyCrowdControlImmunities(me);
            me->setActive(true);
        }

        void Reset() override
        {
            BossAI::Reset();
            if (InstanceScript* instance = me->GetInstanceScript())
                instance->SetData(Misc::CookieDietFailed, 0);
            me->SetVisible(false);
            me->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE);
            me->SetUnitFlag(UNIT_FLAG_UNINTERACTIBLE);
            me->SetReactState(REACT_PASSIVE);
            DoCastSelf(Spells::WhoIsThat, true);
        }

        void MoveInLineOfSight(Unit* who) override
        {
            if (!who->IsPlayer())
                return;

            if (instance->GetBossState(DataTypes::BOSS_ADMIRAL_RIPSNARL) != DONE)
                return;

            if (me->GetDistance(who) > 5.0f)
                return;

            me->SetVisible(true);
            me->RemoveUnitFlag(UNIT_FLAG_NON_ATTACKABLE);
            me->RemoveUnitFlag(UNIT_FLAG_UNINTERACTIBLE);
            CreatureAI::MoveInLineOfSight(who);
        }

        void JustEngagedWith(Unit* who) override
        {
            BossAI::JustEngagedWith(who);
            me->RemoveAurasDueToSpell(Spells::WhoIsThat);
            me->AttackStop();
            me->SetReactState(REACT_PASSIVE);
            me->RemoveUnitFlag(UNIT_FLAG_NON_ATTACKABLE);
            me->RemoveUnitFlag(UNIT_FLAG_UNINTERACTIBLE);
            DoCastSelf(Spells::CookieAchievCredit, true);
            _events.RescheduleEvent(Events::CookieMoveToCauldron, 1s);
            DoZoneInCombat();
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type == POINT_MOTION_TYPE && id == POINT_MOVE)
                _events.RescheduleEvent(Events::CookieSummonCauldron, 2s);
        }

        void JustDied(Unit* /*killer*/) override
        {
            if (InstanceScript* instance = me->GetInstanceScript())
                if (!instance->GetData(Misc::CookieDietFailed))
                    if (AchievementEntry const* achievement = sAchievementStore.LookupEntry(Achievements::ImOnADiet))
                        instance->instance->DoOnPlayers([achievement](Player* player)
                        {
                            if (!player->HasAchieved(achievement->ID))
                                player->CompletedAchievement(achievement);
                        });

            if (IsHeroic())
                me->SummonCreature(Creatures::VanessaNote, Positions::VanessaNoteSpawn, TEMPSUMMON_MANUAL_DESPAWN);
        }

        void EnterEvadeMode(EvadeReason why) override
        {
            BossAI::EnterEvadeMode(why);
            me->SetVisible(false);
            me->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE);
            me->SetUnitFlag(UNIT_FLAG_UNINTERACTIBLE);
            me->SetReactState(REACT_PASSIVE);
        }

        void UpdateAI(uint32 diff) override
        {
            _events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case Events::CookieMoveToCauldron:
                        me->GetMotionMaster()->MovePoint(POINT_MOVE, Positions::CookieDeckCenter);
                        break;
                    case Events::CookieSummonCauldron:
                        if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 100.0f, true))
                            DoCast(target, Spells::CauldronSummon, true);
                        _events.RescheduleEvent(Events::CookieJumpToCauldron, 2s);
                        break;
                    case Events::CookieJumpToCauldron:
                        if (Creature* cauldron = me->FindNearestCreature(Creatures::Cauldron, 20.0f))
                            me->GetMotionMaster()->MoveJump(0, cauldron->GetPosition(), 5.0f, 10.0f);
                        else
                            me->GetMotionMaster()->MoveJump(0, Positions::CookieCauldronJumpTarget, 5.0f, 10.0f);
                        _events.RescheduleEvent(Events::CookieThrowFood, 3s);
                        break;
                    case Events::CookieThrowFood:
                        if (Unit* target = SelectTarget(SelectTargetMethod::Random, 0, 100.0f, true))
                            DoCast(target, ThrowFoodSpells[urand(0, 11)]);
                        _events.RescheduleEvent(Events::CookieThrowFood, 3s);
                        break;
                }
            }

            if (!UpdateVictim())
                return;
        }

        private:
            EventMap _events;
    };

    struct npc_captain_cookie_cauldron : public ScriptedAI
    {
        npc_captain_cookie_cauldron(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            me->SetReactState(REACT_PASSIVE);
            me->SetUnitFlag(UNIT_FLAG_UNINTERACTIBLE);
            DoCastSelf(Spells::CauldronVisual, true);
            DoCastSelf(Spells::CauldronFire, true);
        }
    };

    struct npc_captain_cookie_good_food : public ScriptedAI
    {
        npc_captain_cookie_good_food(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            me->SetReactState(REACT_PASSIVE);
        }

        bool OnGossipHello(Player* player) override
        {
            InstanceScript* instance = me->GetInstanceScript();
            if (!instance)
                return true;
            if (instance->GetBossState(DataTypes::BOSS_CAPTAIN_COOKIE) != IN_PROGRESS)
                return true;

            player->CastSpell(player, Spells::Satiated, true);
            me->DespawnOrUnsummon();
            return true;
        }
    };

    struct npc_captain_cookie_bad_food : public ScriptedAI
    {
        npc_captain_cookie_bad_food(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            me->SetReactState(REACT_PASSIVE);
            DoCastSelf(Spells::RottenAura, true);
        }

        bool OnGossipHello(Player* player) override
        {
            InstanceScript* instance = me->GetInstanceScript();
            if (!instance)
                return true;
            if (instance->GetBossState(DataTypes::BOSS_CAPTAIN_COOKIE) != IN_PROGRESS)
                return true;

            player->CastSpell(player, Spells::Nauseated, true);
            me->DespawnOrUnsummon();
            return true;
        }
    };

    class spell_captain_cookie_satiated : public SpellScript
    {
        void HandleScript(SpellEffIndex /*effIndex*/)
        {
            if (Unit* target = GetHitUnit())
                target->RemoveAuraFromStack(Spells::Nauseated);
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_captain_cookie_satiated::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
        }
    };

    class spell_captain_cookie_nauseated : public SpellScript
    {
        void HandleScript(SpellEffIndex /*effIndex*/)
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            caster->RemoveAuraFromStack(Spells::Satiated);

            if (Aura* aura = caster->GetAura(Spells::Nauseated))
            {
                if (aura->GetStackAmount() > 1)
                {
                    caster->CastSpell(caster, Spells::CookieAchievRemove, true);
                    if (InstanceScript* instance = caster->GetInstanceScript())
                        instance->SetData(Misc::CookieDietFailed, 1);
                }
            }
        }

        void Register() override
        {
            OnEffectHitTarget += SpellEffectFn(spell_captain_cookie_nauseated::HandleScript, EFFECT_0, SPELL_EFFECT_APPLY_AURA);
        }
    };
}

void AddSC_custom_deadmines_captain_cookie()
{
    using namespace Scripts::EasternKingdoms::Deadmines;

    RegisterCreatureAI(boss_captain_cookie);
    RegisterCreatureAI(npc_captain_cookie_cauldron);
    RegisterCreatureAI(npc_captain_cookie_good_food);
    RegisterCreatureAI(npc_captain_cookie_bad_food);
    RegisterSpellScript(spell_captain_cookie_satiated);
    RegisterSpellScript(spell_captain_cookie_nauseated);
}
