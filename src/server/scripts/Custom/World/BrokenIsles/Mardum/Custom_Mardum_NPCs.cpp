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

#include "Conversation.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "ObjectAccessor.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"
#include "SpellInfo.h"
#include "Spell.h"
#include "TaskScheduler.h"
#include "Unit.h"

#include "Custom_Mardum_Defines.h"

namespace Scripts::Custom::Mardum
{
    // 99916 - Sevis Brightflame (Ashtongue Gateway, shared/non-private spawn)
    struct npc_sevis_brightflame_ashtongue_gateway : public ScriptedAI
    {
        npc_sevis_brightflame_ashtongue_gateway(Creature* creature) : ScriptedAI(creature) { }

        void MoveInLineOfSight(Unit* who) override
        {
            Player* player = who->ToPlayer();
            if (!player)
                return;

            if (!me->IsWithinDist(player, Misc::TalkDistance))
                return;

            if (player->GetQuestStatus(Quests::EnterTheIllidariAshtongue) != QUEST_STATUS_INCOMPLETE)
                return;

            // greet each player only once
            if (_greetedPlayers.insert(player->GetGUID()).second)
                Talk(CreatureText::SevisAshtongueGreeting, player);
        }

    private:
        GuidUnorderedSet _greetedPlayers;
    };

    // Factory used by SevisBrightflameAshtongueGatewayAISelector in zone_mardum.cpp
    // for the shared (non-private) spawn. Returns CreatureAI* so the concrete type
    // stays private to this translation unit.
    CreatureAI* CreateSevisBrightflameAshtongueGatewayAI(Creature* creature)
    {
        return new npc_sevis_brightflame_ashtongue_gateway(creature);
    }

    // 94377 - Cyana Nightglaive (captive, shared/non-private spawn)
    struct npc_cyana_nightglaive_captive : public ScriptedAI
    {
        npc_cyana_nightglaive_captive(Creature* creature) : ScriptedAI(creature) { }

        void MoveInLineOfSight(Unit* who) override
        {
            Player* player = who->ToPlayer();
            if (!player)
                return;

            if (!me->IsWithinDist(player, Misc::CaptiveGreetingRange))
                return;

            if (player->GetQuestStatus(Quests::SetThemFree) != QUEST_STATUS_INCOMPLETE)
                return;

            // greet each player only once
            if (_greetedPlayers.insert(player->GetGUID()).second)
                Talk(CreatureText::CyanaCaptiveGreeting, player);
        }

    private:
        GuidUnorderedSet _greetedPlayers;
    };

    // Factory used by CyanaNightglaiveFreedAISelector in zone_mardum.cpp
    // for the shared (non-private) spawn. Returns CreatureAI* so the concrete type
    // stays private to this translation unit.
    CreatureAI* CreateCyanaNightglaiveCaptiveAI(Creature* creature)
    {
        return new npc_cyana_nightglaive_captive(creature);
    }

    // 94400 - Belath Dawnblade (captive, shared/non-private spawn)
    struct npc_belath_dawnblade_captive : public ScriptedAI
    {
        npc_belath_dawnblade_captive(Creature* creature) : ScriptedAI(creature) { }

        void MoveInLineOfSight(Unit* who) override
        {
            Player* player = who->ToPlayer();
            if (!player)
                return;

            if (!me->IsWithinDist(player, Misc::CaptiveGreetingRange))
                return;

            if (player->GetQuestStatus(Quests::SetThemFree) != QUEST_STATUS_INCOMPLETE)
                return;

            // greet each player only once
            if (_greetedPlayers.insert(player->GetGUID()).second)
                Talk(CreatureText::BelathCaptiveGreeting, player);
        }

    private:
        GuidUnorderedSet _greetedPlayers;
    };

    // Factory used by BelathDawnbladeFreedAISelector in zone_mardum.cpp
    // for the shared (non-private) spawn. Returns CreatureAI* so the concrete type
    // stays private to this translation unit.
    CreatureAI* CreateBelathDawnbladeCaptiveAI(Creature* creature)
    {
        return new npc_belath_dawnblade_captive(creature);
    }

    // 99917 - Sevis Brightflame (Coilskar Gateway, shared/non-private spawn)
    struct npc_sevis_brightflame_coilskar_gateway : public ScriptedAI
    {
        npc_sevis_brightflame_coilskar_gateway(Creature* creature) : ScriptedAI(creature) { }

        void MoveInLineOfSight(Unit* who) override
        {
            Player* player = who->ToPlayer();
            if (!player)
                return;

            if (!me->IsWithinDist(player, Misc::CaptiveGreetingRange))
                return;

            if (player->GetQuestStatus(Quests::EnterTheIllidariCoilskar) != QUEST_STATUS_INCOMPLETE)
                return;

            if (player->IsQuestObjectiveComplete(Quests::EnterTheIllidariCoilskar, Objectives::SoulSacrificed))
                return;

            // greet each player only once
            if (!_greetedPlayers.insert(player->GetGUID()).second)
                return;

            Talk(CreatureText::SevisCoilskarGreeting, player);

            ObjectGuid playerGuid = player->GetGUID();
            _scheduler.Schedule(5s, [this, playerGuid](TaskContext const& /*context*/)
            {
                if (Player* player = ObjectAccessor::GetPlayer(*me, playerGuid))
                    Talk(CreatureText::SevisCoilskarFollowUp, player);
            });
        }

        void UpdateAI(uint32 diff) override
        {
            _scheduler.Update(diff);
        }

    private:
        TaskScheduler _scheduler;
        GuidUnorderedSet _greetedPlayers;
    };

    // Factory used by SevisBrightflameCoilskarGatewayAISelector in zone_mardum.cpp
    // for the shared (non-private) spawn. Returns CreatureAI* so the concrete type
    // stays private to this translation unit.
    CreatureAI* CreateSevisBrightflameCoilskarGatewayAI(Creature* creature)
    {
        return new npc_sevis_brightflame_coilskar_gateway(creature);
    }

    // 97142 - Fel Spreader
    // npc_spellclick_spells makes the clicker cast 191827 (2s cast) on the spreader.
    // SpellHit fires exactly when that cast lands; the scheduler task is a fallback
    // in case the spell never produces a unit-target hit on the creature.
    struct npc_fel_spreader : public ScriptedAI
    {
        npc_fel_spreader(Creature* creature) : ScriptedAI(creature) { }

        void OnSpellClick(Unit* clicker, bool spellClickHandled) override
        {
            Player* player = clicker ? clicker->ToPlayer() : nullptr;
            if (!spellClickHandled || !player)
                return;

            ObjectGuid clickerGuid = player->GetGUID();
            _scheduler.Schedule(2200ms, [this, clickerGuid](TaskContext const& /*context*/)
            {
                if (Player* player = ObjectAccessor::GetPlayer(*me, clickerGuid))
                    FinishInteraction(player);
            });
        }

        void SpellHit(WorldObject* caster, SpellInfo const* spellInfo) override
        {
            if (spellInfo->Id != Spells::DestroyingFelSpreader)
                return;

            if (Unit* unitCaster = caster ? caster->ToUnit() : nullptr)
                FinishInteraction(unitCaster);
        }

        void UpdateAI(uint32 diff) override
        {
            _scheduler.Update(diff);
        }

    private:
        void FinishInteraction(Unit* clicker)
        {
            Player* player = clicker->ToPlayer();
            if (!player)
                return;

            if (!_processedClickers.insert(player->GetGUID()).second)
                return;

            // Triggered: the player may still be finishing the 191827 cast slot.
            player->CastSpell(me, Spells::FelSpreaderExplosion, CastSpellExtraArgs(TRIGGERED_FULL_MASK));

            // Read the objective before awarding the credits that complete it.
            // FirstFelSpreader credit completes objective 279930, which gates the
            // conversation so it only triggers on the first spreader.
            bool firstSpreader = !player->IsQuestObjectiveComplete(Quests::AssaultOnMardum, Objectives::FelSpreaderDestroyed);
            player->KilledMonsterCredit(Creatures::FelSpreader, me->GetGUID());

            if (firstSpreader)
            {
                player->KilledMonsterCredit(Creatures::FirstFelSpreader, me->GetGUID());
                Conversation::CreateConversation(Conversations::FelSpreaderDestroyed, player, *player, player->GetGUID());
            }

            me->DespawnOrUnsummon(1s, 5min);
        }

        TaskScheduler _scheduler;
        GuidUnorderedSet _processedClickers;
    };
}

void AddSC_custom_mardum_npcs()
{
    using namespace Scripts::Custom::Mardum;

    RegisterCreatureAI(npc_fel_spreader);
}
