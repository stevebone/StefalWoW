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
            if (id == Data::DataPounce && value == 1)
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

                _events.ScheduleEvent(Events::MoveFlee, 1s);
            }
            else if (type == POINT_MOTION_TYPE && id == Points::PointFlee)
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
                case Events::MoveFlee:
                    me->GetMotionMaster()->MovePoint(Points::PointFlee, Positions::LurkingWorgenFleePos);
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
}

void AddSC_custom_duskwood_npcs()
{
    using namespace Scripts::EasternKingdoms::Duskwood;

    RegisterCreatureAI(npc_apprentice_fess);
    RegisterCreatureAI(npc_lurking_worgen);
    RegisterCreatureAI(npc_ello_ebonlocke);
    RegisterCreatureAI(npc_stiches);
}
