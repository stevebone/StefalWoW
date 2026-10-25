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
#include "GameObject.h"
#include "GameObjectAI.h"
#include "ObjectAccessor.h"
#include "PhasingHandler.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "TaskScheduler.h"
#include "TemporarySummon.h"

#include "Custom_Mardum_Defines.h"

namespace Scripts::Custom::Mardum
{
    /*######
    ## 244439 Legion Communicator #1
    ## 244440 Legion Communicator #2
    ## CMSG_GAMEOBJECT_REPORT_USE -> talking-head conversation, then the
    ## gameobject is marked not selectable after use and remains spawned
    ######*/

    struct go_legion_communicator : public GameObjectAI
    {
        go_legion_communicator(GameObject* go) : GameObjectAI(go)
        {
            DetermineCommunicator();
        }

        bool OnReportUse(Player* player) override
        {
            if (_used)
                return true;

            _used = true;

            // Read the objective before awarding the credit that completes it.
            if (!player->IsQuestObjectiveComplete(Quests::AssaultOnMardum, _questObjective))
                Conversation::CreateConversation(_conversationId, player, *player, player->GetGUID());

            player->KilledMonsterCredit(_killCreditEntry);
            me->SetFlag(GO_FLAG_NOT_SELECTABLE);
            return false;
        }

    private:
        bool _used = false;
        uint32 _questObjective = 0;
        uint32 _conversationId = 0;
        uint32 _killCreditEntry = 0;

        void DetermineCommunicator()
        {
            switch (me->GetEntry())
            {
            case GameObjects::LegionCommunicator1:
                _questObjective = Objectives::LegionCommunicator1;
                _conversationId = Conversations::LegionCommunicatorReport1;
                _killCreditEntry = Creatures::LegionCommunicator1;
                break;
            case GameObjects::LegionCommunicator2:
                _questObjective = Objectives::LegionCommunicator2;
                _conversationId = Conversations::LegionCommunicatorReport2;
                _killCreditEntry = Creatures::LegionCommunicator2;
                break;
            default:
                break;
            }
        }
    };

    /*######
    ## 243968 Banner Doom Fortress      -> 96562 Ashtongue Stalker
    ## 243967 Banner Forge of Corruption -> 96564 Priestess of Dementia
    ## 243965 Banner Soul Engine         -> 96561 Coilskar Harpooner
    ## CMSG_GAMEOBJECT_REPORT_USE -> SetData on the trigger npc, which then
    ## sabotages the nearby Legion Devastator (npc_illidari_banner_trigger)
    ######*/

    struct go_mardum_illidari_banner : public GameObjectAI
    {
        go_mardum_illidari_banner(GameObject* go) : GameObjectAI(go)
        {
            DetermineTrigger();
        }

        bool OnReportUse(Player* player) override
        {
            if (_used)
                return true;

            _used = true;
            me->SetFlag(GO_FLAG_NOT_SELECTABLE);

            player->KilledMonsterCredit(_killCreditEntry);

            std::list<Creature*> triggers;
            player->GetCreatureListWithEntryInGrid(triggers, _triggerEntry, Misc::SearchRange);
            for (Creature* trigger : triggers)
                trigger->AI()->SetData(Misc::BannerTriggered, 1);

            return false;
        }

    private:
        bool _used = false;
        uint32 _triggerEntry = 0;
        uint32 _killCreditEntry = 0;

        void DetermineTrigger()
        {
            switch (me->GetEntry())
            {
            case GameObjects::BannerDoomFortress:
                _triggerEntry = Creatures::AshtongueStalker;
                _killCreditEntry = Creatures::BannerKillCreditDoomFortress;
                break;
            case GameObjects::BannerForgeOfCorruption:
                _triggerEntry = Creatures::PriestessOfDementia;
                _killCreditEntry = Creatures::BannerKillCreditForgeOfCorruption;
                break;
            case GameObjects::BannerSoulEngine:
                _triggerEntry = Creatures::CoilskarHarpooner;
                _killCreditEntry = Creatures::BannerKillCreditSoulEngine;
                break;
            default:
                break;
            }
        }
    };

    /*######
    ## 245995 Spire Stabilizer ("Their Numbers Are Legion" 38819)
    ## 246012 Spire Stabilizer ("Their Numbers Are Legion" 38819)
    ## On spawn/respawn: every Spire of Woe Fire (101318,101321) in range channels
    ## 199707 Fel Beam onto the nearest Spire of Woe Bunny (101320).
    ## On use: the fire npcs and bunny despawn, the stabilizer follows 1s later.
    ######*/

    struct go_spire_stabilizer : public GameObjectAI
    {
        go_spire_stabilizer(GameObject* go) : GameObjectAI(go) { }

        void Reset() override
        {
            _used = false;
            _fireGuids.clear();
            _bunnyGuid.Clear();

            // Deferred one tick: Reset runs before AddToMap, and at initial
            // spawn the creatures may not be loaded in the grid yet.
            _scheduler.Schedule(1s, [this](TaskContext const& /*context*/)
            {
                SetupBeams();
            });
        }

        bool OnReportUse(Player* /*player*/) override
        {
            if (_used)
                return true;

            _used = true;

            for (ObjectGuid const& fireGuid : _fireGuids)
                if (Creature* fire = ObjectAccessor::GetCreature(*me, fireGuid))
                    fire->DespawnOrUnsummon();

            if (Creature* bunny = ObjectAccessor::GetCreature(*me, _bunnyGuid))
                bunny->DespawnOrUnsummon();

            me->DespawnOrUnsummon(7200s, 1s);
            return false;
        }

        void UpdateAI(uint32 diff) override
        {
            _scheduler.Update(diff);
        }

    private:
        bool _used = false;
        std::vector<ObjectGuid> _fireGuids;
        ObjectGuid _bunnyGuid;
        TaskScheduler _scheduler;

        void SetupBeams()
        {
            Creature* bunny = me->FindNearestCreature(Creatures::SpireOfWoeBunny, Misc::SearchRange);
            if (!bunny)
                return;

            _bunnyGuid = bunny->GetGUID();

            std::list<Creature*> fires;
            me->GetCreatureListWithEntryInGrid(fires, Creatures::SpireOfWoeFire, Misc::SearchRange);
            me->GetCreatureListWithEntryInGrid(fires, Creatures::SpireOfWoeFire2, Misc::SearchRange);
            for (Creature* fire : fires)
            {
                _fireGuids.push_back(fire->GetGUID());
                fire->CastSpell(bunny, Spells::SpireOfWoeFelBeam, true);
            }
        }
    };

    /*######
    ## 246280 Cylinder Collision ("Their Numbers Are Legion" 38819)
    ## On spawn: summons the Well of Souls (102916) at the cylinder's position.
    ## GameObject::Create sets map/position before AIM_Initialize, so the
    ## position is already valid in the constructor.
    ######*/

    struct go_cylinder_collision : public GameObjectAI
    {
        go_cylinder_collision(GameObject* go) : GameObjectAI(go)
        {
            if (Creature* well = me->SummonCreature(Creatures::WellOfSouls, me->GetPosition()))
                PhasingHandler::SetAlwaysVisible(well, true, true);
        }
    };

    /*######
    ## 245728 Sargerite Keystone ("Return to the Black Temple" 38729)
    ## On use: 5s later the two phase-0 ELM bunnies beside it link with
    ## Fel Beam (first -> second); 5s after that the user is credited.
    ######*/

    struct go_sargerite_keystone : public GameObjectAI
    {
        go_sargerite_keystone(GameObject* go) : GameObjectAI(go) { }

        bool OnReportUse(Player* player) override
        {
            if (_used)
                return true;

            _used = true;
            me->SetFlag(GO_FLAG_NOT_SELECTABLE);

            ObjectGuid playerGuid = player->GetGUID();
            _scheduler.Schedule(5s, [this](TaskContext const& /*context*/)
            {
                std::list<Creature*> bunnies;
                me->GetCreatureListWithEntryInGrid(bunnies, Creatures::KeystoneFelBeamBunny, Misc::KeystoneBunnySearchRange);
                if (bunnies.size() >= 2)
                    bunnies.front()->CastSpell(bunnies.back(), Spells::KeystoneFelBeam, true);
            });
            _scheduler.Schedule(10s, [this, playerGuid](TaskContext const& /*context*/)
            {
                if (Player* player = ObjectAccessor::GetPlayer(*me, playerGuid))
                    player->KilledMonsterCredit(Creatures::KeystoneActivatedCredit);
            });
            return false;
        }

        void UpdateAI(uint32 diff) override
        {
            _scheduler.Update(diff);
        }

    private:
        bool _used = false;
        TaskScheduler _scheduler;
    };

    /*######
    ## 244466 Fel Portal ("Return to the Black Temple" 38729)
    ## On reward: Back to Black Temple (192140) applies the fel screen effect and
    ## plays movie 471. player_mardum_back_to_black_temple::OnMovieComplete then
    ## removes the aura, teleports the player out and sets their homebind.
    ######*/

    struct go_fel_portal : public GameObjectAI
    {
        go_fel_portal(GameObject* go) : GameObjectAI(go) { }

        void OnQuestReward(Player* player, Quest const* quest, LootItemType /*type*/, uint32 /*opt*/) override
        {
            if (quest->GetQuestId() != Quests::ReturnToTheBlackTemple)
                return;

            player->CastSpell(player, Spells::BackToBlackTemple, true);

            // Fallback: if the client kills the movie without ever sending
            // CMSG_COMPLETE_MOVIE, run the completion once it can no longer be
            // playing anyway so the player is not stranded under the overlay.
            // This was an observed bug within the client where after movie plays
            // for the first time it will stop working on subsequent runs
            player->m_Events.AddEventAtOffset([player]()
            {
                if (player->HasAura(Spells::BackToBlackTemple))
                    sScriptMgr->OnMovieComplete(player, Movies::BackToBlackTemple);
            }, 180s);
        }
    };
}

void AddSC_custom_mardum_objects()
{
    using namespace Scripts::Custom::Mardum;

    RegisterGameObjectAI(go_legion_communicator);
    RegisterGameObjectAI(go_mardum_illidari_banner);
    RegisterGameObjectAI(go_spire_stabilizer);
    RegisterGameObjectAI(go_cylinder_collision);
    RegisterGameObjectAI(go_sargerite_keystone);
    RegisterGameObjectAI(go_fel_portal);
}
