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

#include "Creature.h"
#include "EventMap.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "PhasingHandler.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"
#include "SpellInfo.h"
#include "TemporarySummon.h"

#include "Custom_Duskwood_Defines.h"

namespace Scripts::EasternKingdoms::Duskwood
{
    /*######
    ## npc_apprentice_fess
    ######*/

    struct npc_apprentice_fess : public ScriptedAI
    {
        npc_apprentice_fess(Creature* creature) : ScriptedAI(creature) { }

        void OnQuestAccept(Player* /*player*/, Quest const* quest) override
        {
            if (quest->GetQuestId() == Quests::TheYorgenWorgen)
                me->SummonCreature(Creatures::LurkingWorgen, Positions::LurkingWorgenSummonPos, TEMPSUMMON_MANUAL_DESPAWN);
        }
    };

    /*######
    ## npc_lurking_worgen
    ######*/

    struct npc_lurking_worgen : public ScriptedAI
    {
        npc_lurking_worgen(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            me->SetDisplayId(33511);
            _pounceTargetGuid.Clear();
            _events.Reset();
        }

        void SetData(uint32 id, uint32 value) override
        {
            if (id == Data::LurkingWorgenPounce && value == 1)
            {
                if (Player* player = me->SelectNearestPlayer(30.0f))
                {
                    _pounceTargetGuid = player->GetGUID();
                    me->GetMotionMaster()->MoveJump(EVENT_JUMP, player->GetPosition(), 20.0f);
                }
            }
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type == EFFECT_MOTION_TYPE && id == EVENT_JUMP)
            {
                if (Unit* target = ObjectAccessor::GetUnit(*me, _pounceTargetGuid))
                    me->CastSpell(target, Spells::StunningPounce, true);

                _events.ScheduleEvent(Events::LurkingWorgenMoveFlee, 1s);
            }
            else if (type == POINT_MOTION_TYPE && id == Points::LurkingWorgenFlee)
            {
                me->DespawnOrUnsummon();
            }
        }

        void UpdateAI(uint32 diff) override
        {
            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                case Events::LurkingWorgenMoveFlee:
                    me->GetMotionMaster()->MovePoint(Points::LurkingWorgenFlee, Positions::LurkingWorgenFleePos);
                    break;
                default:
                    break;
                }
            }
        }

        EventMap _events;
        ObjectGuid _pounceTargetGuid;
    };

    /*######
    ## 263 Ello Ebonlocke
    ######*/
    
    struct npc_ello_ebonlocke : public ScriptedAI
    {
        npc_ello_ebonlocke(Creature* creature) : ScriptedAI(creature) {}

        void OnQuestAccept(Player* player, Quest const* quest) override
        {
            if (quest->GetQuestId() == Quests::TheEmbalmersRevenge)
            {
                PhasingHandler::OnConditionChange(player, true);

                player->SummonCreature(Creatures::Stiches, Positions::StichesSpawn,
                    TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 90s);
            }
        }
    };

    /*######
    ## 43862 Stiches
    ######*/

    struct npc_stiches : public ScriptedAI
    {
        npc_stiches(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            me->CastSpell(me, Spells::AuraOfRot, true);
            EngageWatchers();
        }

        void DamageTaken(Unit* attacker, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo*/) override
        {
            if (attacker && attacker->GetTypeId() == TYPEID_UNIT)
                if (IsWatcher(attacker->GetEntry()))
                    damage = 0;
        }

        void DamageDealt(Unit* victim, uint32& damage, DamageEffectType /*damageType*/) override
        {
            if (victim && victim->GetTypeId() == TYPEID_UNIT)
                if (IsWatcher(victim->GetEntry()))
                    damage = 0;
        }

    private:
        static bool IsWatcher(uint32 entry)
        {
            switch (entry)
            {
            case Watchers::Brownell:
            case Watchers::Fraizer:
            case Watchers::Hartin:
            case Watchers::Jordan:
            case Watchers::Keefer:
            case Watchers::Ladimore:
                return true;
            default:
                return false;
            }
        }

        void EngageWatchers()
        {
            static constexpr uint32 watcherEntries[] =
            {
                Watchers::Brownell, Watchers::Fraizer, Watchers::Hartin,
                Watchers::Jordan, Watchers::Keefer, Watchers::Ladimore
            };

            for (uint32 entry : watcherEntries)
                if (Creature* watcher = me->FindNearestCreature(entry, 20.0f, true))
                    if (!watcher->IsInCombat())
                        watcher->AI()->AttackStart(me);
        }
    };

    /*######
    ## 315 Stalvan Mistmantle
    ######*/

    struct npc_stalvan_mistmantle : public ScriptedAI
    {
        npc_stalvan_mistmantle(Creature* creature) : ScriptedAI(creature)
        {
            me->SetUnitFlag(UNIT_FLAG_IMMUNE_TO_PC);
            me->SetUnitFlag(UNIT_FLAG_IMMUNE_TO_NPC);
        }

        void Reset() override
        {
            me->SetReactState(REACT_PASSIVE);
            _events.Reset();
            _events.ScheduleEvent(Events::StalvanStep1, 3s);
            _events.ScheduleEvent(Events::StalvanStep2, 8s);
            _events.ScheduleEvent(Events::StalvanStep3, 15s);
            _events.ScheduleEvent(Events::StalvanStep4, 23s);
            _events.ScheduleEvent(Events::StalvanStep5, 26s);
            _events.ScheduleEvent(Events::StalvanStep6, 32s);
        }

        void JustDied(Unit* /*killer*/) override
        {
            if (Creature* tobias = ObjectAccessor::GetCreature(*me, _tobiasGuid))
            {
                Talk(Talks::StalvanSay06, tobias);
                tobias->AI()->SetData(Data::StalvanDied, 1);
            }
        }

        Creature* GetTobias()
        {
            if (Creature* tobias = me->FindNearestCreature(Creatures::TobiasMistmantle, 10.0f, true))
                return tobias;

            return me->FindNearestCreature(Creatures::TobiasMistmantleWorgen, 10.0f, true);
        }

        void UpdateAI(uint32 diff) override
        {
            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                case Events::StalvanStep1:
                    me->SetWalk(true);
                    me->SetSpeed(MOVE_WALK, 2.5f);
                    me->GetMotionMaster()->MovePoint(0, Positions::StalvanMoveTo, true);
                    if (Creature* tobias = GetTobias())
                    {
                        _tobiasGuid = tobias->GetGUID();
                        Talk(Talks::StalvanSay00, tobias);
                    }
                    break;
                case Events::StalvanStep2:
                    Talk(Talks::StalvanSay01, GetTobias());
                    break;
                case Events::StalvanStep3:
                    Talk(Talks::StalvanSay02, GetTobias());
                    break;
                case Events::StalvanStep4:
                    Talk(Talks::StalvanSay03, GetTobias());
                    break;
                case Events::StalvanStep5:
                    Talk(Talks::StalvanSay04, GetTobias());
                    break;
                case Events::StalvanStep6:
                    Talk(Talks::StalvanSay05, GetTobias());
                    _events.ScheduleEvent(Events::StalvanCastSpell, 5s);
                    break;
                case Events::StalvanCastSpell:
                {
                    if(me->GetVictim() && !me->GetVictim()->HasAura(Spells::CurseOfStalvan))
                        DoCastVictim(Spells::CurseOfStalvan);

                    _events.ScheduleEvent(Events::StalvanCastSpell, 5s);

                    break;
                }
                default:
                    break;
                }
            }

            if (!UpdateVictim())
                return;

            me->DoMeleeAttackIfReady();
        }

    private:
        EventMap _events;
        ObjectGuid _tobiasGuid;
    };

    /*######
    ## 43453 Tobias Mistmantle
    ######*/

    struct npc_tobias_mistmantle : public ScriptedAI
    {
        npc_tobias_mistmantle(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            // this is needed to avoid the spawn in Darkshire to run the script
            // there may be another npc entry for Tobias human for this quest
            if (me->GetSpawnId() > 0)
                return;

            me->SetReactState(REACT_PASSIVE);
            me->RemoveNpcFlag(NPCFlags(UNIT_NPC_FLAG_QUESTGIVER | UNIT_NPC_FLAG_GOSSIP));
            me->SetWalk(true);
            me->SetSpeed(MOVE_WALK, 3.5f);
            me->GetMotionMaster()->MovePoint(0, Positions::TobiasMoveTo, true);

            _events.Reset();
            _events.ScheduleEvent(Events::TobiasStep1, 5s);
            _events.ScheduleEvent(Events::TobiasStep2, 9s);
            _events.ScheduleEvent(Events::TobiasStep3, 16s);
            _events.ScheduleEvent(Events::TobiasStep4, 35s);
        }

        Creature* GetStalvan()
        {
            return me->FindNearestCreature(Creatures::StalvanMistmantle, 10.0f, true);
        }

        void SetData(uint32 id, uint32 /*value*/) override
        {
            if (id == Data::StalvanDied)
            {
                if (!me->IsAlive())
                    return;

                me->SetReactState(REACT_PASSIVE);
                me->GetMotionMaster()->Clear();

                me->m_Events.AddEventAtOffset([this]()
                {
                    Talk(Talks::TobiasSay04);
                    me->SetWalk(false);
                    me->GetMotionMaster()->MovePoint(Points::TobiasSpawn, Positions::TobiasSpawn);
                }, 3s);
            }
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type != POINT_MOTION_TYPE)
                return;

            switch (id)
            {
            case Points::TobiasSpawn:
                me->GetMotionMaster()->MovePoint(Points::TobiasFlee, Positions::TobiasFlee);
                break;
            case Points::TobiasFlee:
                me->DespawnOrUnsummon();
                break;
            default:
                break;
            }
        }

        void UpdateAI(uint32 diff) override
        {
            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                case Events::TobiasStep1:
                    if (Creature* stalvan = GetStalvan())
                    {
                        me->SetFacingToObject(stalvan, true);
                        stalvan->SetFacingToObject(me, true);
                        Talk(Talks::TobiasSay00, stalvan);
                    }
                    break;
                case Events::TobiasStep2:
                    if (Creature* stalvan = GetStalvan())
                        Talk(Talks::TobiasSay01, stalvan);
                    break;
                case Events::TobiasStep3:
                    if (Creature* stalvan = GetStalvan())
                        Talk(Talks::TobiasSay02, stalvan);
                    break;
                case Events::TobiasStep4:
                    me->CastSpell(me, Spells::WorgenTransformVisual, true);
                    me->UpdateEntry(Creatures::TobiasMistmantleWorgen);
                    me->SetReactState(REACT_AGGRESSIVE);

                    if (Creature* stalvan = GetStalvan())
                    {
                        stalvan->RemoveUnitFlag(UNIT_FLAG_IMMUNE_TO_PC);
                        stalvan->RemoveUnitFlag(UNIT_FLAG_IMMUNE_TO_NPC);
                        stalvan->SetReactState(REACT_AGGRESSIVE);

                        if (me->Attack(stalvan, true))
                            me->GetMotionMaster()->MoveChase(stalvan);

                        if (stalvan->Attack(me, true))
                            stalvan->GetMotionMaster()->MoveChase(me);

                        Talk(Talks::TobiasSay03, me->GetOwner());
                    }
                    break;
                default:
                    break;
                }
            }

            if (!UpdateVictim())
                return;

            me->DoMeleeAttackIfReady();
        }

    private:
        EventMap _events;
    };
}

void AddSC_custom_duskwood_npcs()
{
    using namespace Scripts::EasternKingdoms::Duskwood;

    RegisterCreatureAI(npc_apprentice_fess);
    RegisterCreatureAI(npc_lurking_worgen);
    RegisterCreatureAI(npc_ello_ebonlocke);
    RegisterCreatureAI(npc_stiches);
    RegisterCreatureAI(npc_stalvan_mistmantle);
    RegisterCreatureAI(npc_tobias_mistmantle);
}
