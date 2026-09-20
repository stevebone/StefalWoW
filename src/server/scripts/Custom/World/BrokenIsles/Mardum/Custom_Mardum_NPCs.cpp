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

#include "CellImpl.h"
#include "Conversation.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "GridNotifiers.h"
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

    // 96159 - Colossal Infernal (Molten Shore)
    // Summoned standing on the ground by npc_inquisitor_baleful_molten_shore but
    // hidden until a player is in Molten Shore. A hidden unit emits no spell
    // packets, so the meteor visual is cast by an invisible attacker bunny
    // hovering in the sky onto a target bunny at the coloss' feet.
    // Only the temp-summoned coloss takes part: spawnId 0 + Molten Shore marks
    // it via script string id so it can be looked up unambiguously.
    struct npc_colossal_infernal_molten_shore : public ScriptedAI
    {
        npc_colossal_infernal_molten_shore(Creature* creature) : ScriptedAI(creature) { }

        void JustAppeared() override
        {
            me->setActive(true);
            me->SetFarVisible(true);

            if (me->GetMapId() != Maps::Mardum || me->GetAreaId() != Areas::MoltenShore || me->GetSpawnId() != 0)
                return;

            me->SetScriptStringId(StringIds::ColossalInfernal);
            me->SetVisible(false);
        }

        void DoAction(int32 action) override
        {
            if (action != Actions::ColossalInfernalMeteor || me->IsVisible() || _meteorInFlight)
                return;

            _meteorInFlight = true;

            // Target bunny at the coloss' feet marks the meteor impact point.
            TempSummon* targetBunny = me->SummonCreature(Creatures::ElmGeneralPurposeBunny, me->GetPosition(), TEMPSUMMON_TIMED_DESPAWN, 10s);

            // Attacker bunny hovers at the sky anchor; kill gravity before it can fall.
            if (TempSummon* attackerBunny = me->SummonCreature(Creatures::ElmGeneralPurposeBunny, Positions::InfernalMeteorAttackerBunny, TEMPSUMMON_TIMED_DESPAWN, 10s))
            {
                attackerBunny->SetDisableGravity(true);
                if (targetBunny)
                    attackerBunny->CastSpell(targetBunny, Spells::ColossalInfernalMeteor, CastSpellExtraArgs(TRIGGERED_FULL_MASK));
            }

            _scheduler.Schedule(5s, [this](TaskContext const& /*context*/)
            {
                me->SetVisible(true);
                _meteorInFlight = false;
            });
        }

        void UpdateAI(uint32 diff) override
        {
            _scheduler.Update(diff);

            if (me->IsVisible() || _meteorInFlight)
                return;

            // Molten Shore/Despair Ridge are areas inside zone 7705, and
            // UpdateArea has no script hook - so the crossing is detected by
            // polling for a player standing in Molten Shore.
            _areaCheck -= Milliseconds(diff);
            if (_areaCheck > 0ms)
                return;

            _areaCheck = 1s;

            std::list<Player*> players;
            Trinity::AnyPlayerInPositionRangeCheck check(me, Misc::MeteorTriggerRange);
            Trinity::PlayerListSearcher<Trinity::AnyPlayerInPositionRangeCheck> searcher(me, players, check);
            Cell::VisitWorldObjects(me, searcher, Misc::MeteorTriggerRange);

            for (Player* player : players)
            {
                if (player->GetAreaId() == Areas::MoltenShore)
                {
                    DoAction(Actions::ColossalInfernalMeteor);
                    break;
                }
            }
        }

    private:
        TaskScheduler _scheduler;
        bool _meteorInFlight = false;
        Milliseconds _areaCheck = 0ms;
    };
}

void AddSC_custom_mardum_npcs()
{
    using namespace Scripts::Custom::Mardum;
    RegisterCreatureAI(npc_fel_spreader);
    RegisterCreatureAI(npc_colossal_infernal_molten_shore);
}
