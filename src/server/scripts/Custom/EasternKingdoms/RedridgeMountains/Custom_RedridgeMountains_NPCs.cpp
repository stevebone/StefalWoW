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
#include "GameObject.h"
#include "ObjectAccessor.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "Unit.h"

#include "Custom_RedridgeMountains_Defines.h"

namespace Scripts::EasternKingdoms::RedridgeMountains
{
    /*######
    ## 43081 Guard Bateman
    ######*/

    struct npc_guard_bateman : public ScriptedAI
    {
        npc_guard_bateman(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            _events.Reset();
            _cooldownPlayers.clear();
            _cooldownQueue = std::queue<ObjectGuid>();
        }

        void MoveInLineOfSight(Unit* who) override
        {
            ScriptedAI::MoveInLineOfSight(who);

            Player* player = who->ToPlayer();
            if (!player)
                return;

            if (me->GetDistance(who) > 15.f)
                return;

            if (!player->HasAuraType(SPELL_AURA_MOUNTED) && !player->IsFlying())
                return;

            ObjectGuid playerGuid = player->GetGUID();
            if (_cooldownPlayers.find(playerGuid) != _cooldownPlayers.end())
                return;

            Talk(Talks::GuardBatemanSay00, player);

            _cooldownPlayers.insert(playerGuid);
            _cooldownQueue.push(playerGuid);
            _events.ScheduleEvent(Events::GuardBatemanClearCooldown, 5min);
        }

        void UpdateAI(uint32 diff) override
        {
            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case Events::GuardBatemanClearCooldown:
                    {
                        if (!_cooldownQueue.empty())
                        {
                            _cooldownPlayers.erase(_cooldownQueue.front());
                            _cooldownQueue.pop();
                        }
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
        GuidUnorderedSet _cooldownPlayers;
        std::queue<ObjectGuid> _cooldownQueue;
    };

    /*######
    ## 43270 Messner
    ######*/

    struct npc_messner : public ScriptedAI
    {
        npc_messner(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            _events.Reset();
            _talkCooldown = false;
        }

        void MoveInLineOfSight(Unit* who) override
        {
            ScriptedAI::MoveInLineOfSight(who);

            Player* player = who->ToPlayer();
            if (!player)
                return;

            if (!player->IsActiveQuest(Quests::InSearchOfBravoCompany))
                return;

            if (_talkCooldown)
                return;

            if (!me->FindNearestCreature(Creatures::BlackrockScout, 10.0f, true))
                return;

            Talk(Talks::MessnerSay00, player);

            _talkCooldown = true;
            _events.ScheduleEvent(Events::MessnerClearTalkCooldown, 5min);
        }

        void OnQuestReward(Player* player, Quest const* quest, LootItemType /*type*/, uint32 /*opt*/) override
        {
            if (quest->GetQuestId() == Quests::InSearchOfBravoCompany)
            {
                _talkPlayerGuid = player->GetGUID();
                _events.ScheduleEvent(Events::MessnerTalk1, 1s);
            }
            else if (quest->GetQuestId() == Quests::BreakingOutIsHardToDo)
            {
                if (GameObject* cage = me->FindNearestGameObject(GameObjects::MessnersCage, 10.0f))
                    cage->SetGoState(GO_STATE_ACTIVE);
            }
        }

        void OnQuestAccept(Player* player, Quest const* quest) override
        {
            if (quest->GetQuestId() == Quests::BreakingOutIsHardToDo)
                Talk(Talks::MessnerSay02, player);
        }

        void UpdateAI(uint32 diff) override
        {
            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case Events::MessnerTalk1:
                    {
                        Player* player = ObjectAccessor::GetPlayer(*me, _talkPlayerGuid);
                        if (player)
                            Talk(Talks::MessnerSay01, player);
                        break;
                    }
                    case Events::MessnerClearTalkCooldown:
                        _talkCooldown = false;
                        break;
                    default:
                        break;
                }
            }
        }

    private:
        EventMap _events;
        bool _talkCooldown = false;
        ObjectGuid _talkPlayerGuid;
    };

    /*######
    ## 43340 Blackrock Battle Worg
    ######*/

    struct npc_blackrock_battle_worg : public ScriptedAI
    {
        npc_blackrock_battle_worg(Creature* creature) : ScriptedAI(creature) { }

        void OnAuraApplied(AuraApplication const* aurApp) override
        {
            if (aurApp->GetBase()->GetId() == Spells::CosmeticSleep)
                me->SetReactState(REACT_PASSIVE);
        }

        void OnAuraRemoved(AuraApplication const* aurApp) override
        {
            if (aurApp->GetBase()->GetId() == Spells::CosmeticSleep)
                me->SetReactState(REACT_AGGRESSIVE);
        }

        void MoveInLineOfSight(Unit* who) override
        {
            if (!who->ToPlayer())
                return;

            if (me->GetDistance(who) <= 3.0f)
                me->RemoveAurasDueToSpell(Spells::CosmeticSleep);
            else if (me->GetDistance(who) <= 20.0f && who->HasAura(Spells::HuntersMark))
                me->RemoveAurasDueToSpell(Spells::CosmeticSleep);
            else
                return;

            ScriptedAI::MoveInLineOfSight(who);
            me->EngageWithTarget(who);
        }

        void UpdateAI(uint32 /*diff*/) override
        {
            if (!UpdateVictim())
                return;

            me->DoMeleeAttackIfReady();
        }
    };
}

void AddSC_custom_redridge_mountains_npcs()
{
    using namespace Scripts::EasternKingdoms::RedridgeMountains;

    RegisterCreatureAI(npc_guard_bateman);
    RegisterCreatureAI(npc_messner);
    RegisterCreatureAI(npc_blackrock_battle_worg);
}
