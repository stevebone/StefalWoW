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
#include "CreatureAI.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "ObjectGuid.h"
#include "PhasingHandler.h"
#include "Player.h"
#include "QuestDef.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"
#include "TaskScheduler.h"
#include "TemporarySummon.h"

#include "Custom_VaultWardens_Defines.h"

namespace Scripts::Custom::VaultOfTheWardens
{
    using namespace Creatures;
    using namespace Quests;
    using namespace Texts;
    using namespace AnimKits;
    using namespace Paths;

    // 92718 - Maiev Shadowsong (Vault of Silence cell, phase 5121)
    struct npc_maiev_shadowsong_vault_silence : public ScriptedAI
    {
        npc_maiev_shadowsong_vault_silence(Creature* creature) : ScriptedAI(creature) { }

        void OnQuestAccept(Player* player, Quest const* quest) override
        {
            if (quest->GetQuestId() != BreakingOut)
                return;

            PhasingHandler::OnConditionChange(player);

            Creature* maievObject = GetClosestCreatureWithOptions(player, 10.0f, { .CreatureId = MaievShadowsong, .IgnorePhases = true });
            Creature* wardObject = GetClosestCreatureWithOptions(player, 40.0f, { .CreatureId = WardOfTheHunters, .IgnorePhases = true });
            if (!maievObject || !wardObject)
                return;

            TempSummon* maievClone = maievObject->SummonPersonalClone(maievObject->GetPosition(), TEMPSUMMON_MANUAL_DESPAWN, 0s, 0, 0, player);
            TempSummon* wardClone = wardObject->SummonPersonalClone(wardObject->GetPosition(), TEMPSUMMON_MANUAL_DESPAWN, 0s, 0, 0, player);
            if (!maievClone || !wardClone)
                return;
        }
    };

    // 92718 - Maiev Shadowsong (personal clone, freed scene)
    struct npc_maiev_shadowsong_vault_silence_private : public ScriptedAI
    {
        npc_maiev_shadowsong_vault_silence_private(Creature* creature) : ScriptedAI(creature) { }

        void JustAppeared() override
        {
            Unit* privateObjectOwner = ObjectAccessor::GetUnit(*me, me->GetPrivateObjectOwner());
            if (!privateObjectOwner)
                return;

            Talk(SayMaievFreed, privateObjectOwner);

            // strip the questgiver flag after the create block has gone out so the
            // client doesn't show the clone as a quest giver
            _scheduler.Schedule(500ms, [this](TaskContext& /*task*/)
            {
                me->ReplaceAllNpcFlags(UNIT_NPC_FLAG_NONE);
                //me->RemoveNpcFlag(UNIT_NPC_FLAG_QUESTGIVER);
                me->UpdateNearbyPlayersInteractions();
            });

            _scheduler.Schedule(1s, [this](TaskContext& /*task*/)
            {
                me->GetMotionMaster()->MovePath(MaievVaultOfSilenceFreed, false);
                me->DespawnOrUnsummon(20s);
            });
        }

        void UpdateAI(uint32 diff) override
        {
            _scheduler.Update(diff);
        }

    private:
        TaskScheduler _scheduler;
    };

    CreatureAI* MaievVaultOfSilenceAISelector(Creature* creature)
    {
        if (creature->IsPrivateObject())
            return new npc_maiev_shadowsong_vault_silence_private(creature);
        return new npc_maiev_shadowsong_vault_silence(creature);
    }

    // 100168 - Ward of the Hunters (public spawn, closed door blocker)
    struct npc_ward_of_the_hunters_vault : public ScriptedAI
    {
        npc_ward_of_the_hunters_vault(Creature* creature) : ScriptedAI(creature) { }
    };

    // 100168 - Ward of the Hunters (personal clone, plays the door-open anim)
    struct npc_ward_of_the_hunters_vault_private : public ScriptedAI
    {
        npc_ward_of_the_hunters_vault_private(Creature* creature) : ScriptedAI(creature) { }

        void JustAppeared() override
        {
            _scheduler.Schedule(1s, [this](TaskContext& /*task*/)
            {
                me->SetAIAnimKitId(WardOpen);
            });

            me->DespawnOrUnsummon(7s);
        }

        void UpdateAI(uint32 diff) override
        {
            _scheduler.Update(diff);
        }

    private:
        TaskScheduler _scheduler;
    };

    CreatureAI* WardOfTheHuntersVaultAISelector(Creature* creature)
    {
        if (creature->IsPrivateObject())
            return new npc_ward_of_the_hunters_vault_private(creature);
        return new npc_ward_of_the_hunters_vault(creature);
    }
}

void AddSC_custom_vault_wardens_npcs()
{
    using namespace Scripts::Custom::VaultOfTheWardens;

    new FactoryCreatureScript<CreatureAI, &MaievVaultOfSilenceAISelector>("npc_maiev_shadowsong_vault_silence");
    new FactoryCreatureScript<CreatureAI, &WardOfTheHuntersVaultAISelector>("npc_ward_of_the_hunters_vault");
}
