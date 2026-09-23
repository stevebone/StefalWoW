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
#include "EventMap.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "ObjectGuid.h"
#include "PhasingHandler.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "ScriptMgr.h"
#include "SpellInfo.h"
#include "Spell.h"
#include "TaskScheduler.h"
#include "TemporarySummon.h"
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

    // 93230 - Mannethrel Darkstar (captive, shared/non-private spawn)
    struct npc_mannethrel_darkstar_captive : public ScriptedAI
    {
        npc_mannethrel_darkstar_captive(Creature* creature) : ScriptedAI(creature) { }

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
                Talk(CreatureText::MannethrelCaptiveGreeting, player);
        }

    private:
        GuidUnorderedSet _greetedPlayers;
    };

    // Factory used by MannethrelDarkstarFreedAISelector in zone_mardum.cpp
    // for the shared (non-private) spawn. Returns CreatureAI* so the concrete type
    // stays private to this translation unit.
    CreatureAI* CreateMannethrelDarkstarCaptiveAI(Creature* creature)
    {
        return new npc_mannethrel_darkstar_captive(creature);
    }

    // 93117 - Izal Whitemoon (captive, shared/non-private spawn)
    struct npc_izal_whitemoon_captive : public ScriptedAI
    {
        npc_izal_whitemoon_captive(Creature* creature) : ScriptedAI(creature) { }

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
                Talk(CreatureText::IzalCaptiveGreeting, player);
        }

    private:
        GuidUnorderedSet _greetedPlayers;
    };

    // Factory used by IzalWhitemoonFreedAISelector in zone_mardum.cpp
    // for the shared (non-private) spawn. Returns CreatureAI* so the concrete type
    // stays private to this translation unit.
    CreatureAI* CreateIzalWhitemoonCaptiveAI(Creature* creature)
    {
        return new npc_izal_whitemoon_captive(creature);
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

    // 99914 - Ashtongue Mystic (sacrifice scene)
    // Replaces the SAI scripts which do not support private objects properly
    struct npc_ashtongue_mystic : public ScriptedAI
    {
        npc_ashtongue_mystic(Creature* creature) : ScriptedAI(creature) { }

        bool OnGossipSelect(Player* player, uint32 menuId, uint32 gossipListId) override
        {
            if (menuId != Misc::MysticGossipMenu || gossipListId != 0)
                return false;

            player->CastSpell(me, Spells::AshtongueMysticSacrifice);
            CloseGossipMenuFor(player);
            return true;
        }

        void IsSummonedBy(WorldObject* /*summoner*/) override
        {
            PhasingHandler::ResetPhaseShift(me);

            me->SendPlaySpellVisualKit(Misc::MysticVisualKit, 0, 0);
            me->SetAIAnimKitId(Misc::MysticAnimKit);

            _scheduler.Schedule(1s, [this](TaskContext const& /*context*/)
            {
                me->CastSpell(me, Spells::PermanentFeignDeath, CastSpellExtraArgs(TRIGGERED_FULL_MASK));

                // 191664's implicit-target condition only resolves on entry 24021
                if (Creature* bunny = me->FindNearestCreatureWithOptions(50.f, { .CreatureId = Creatures::ELMGeneralPurposeBunny, .IgnorePhases = true }))
                {
                    // The bunny needs to be private object with the same owner as caster
                    // Otherwise cast produces NOTHING
                    bunny->SetPrivateObjectOwner(me->GetPrivateObjectOwner());
                    PhasingHandler::ResetPhaseShift(bunny);
                    _bunnyGuid = bunny->GetGUID();
                }
            });
            _scheduler.Schedule(2s, [this](TaskContext const& /*context*/) { CastSoulMissiles(); });
            _scheduler.Schedule(4s, [this](TaskContext const& /*context*/) { CastSoulMissiles(); });
            _scheduler.Schedule(5s, [this](TaskContext const& /*context*/) { CastSoulMissiles(); });
        }

        void UpdateAI(uint32 diff) override
        {
            _scheduler.Update(diff);
        }

    private:
        void CastSoulMissiles()
        {
            Creature* bunny = ObjectAccessor::GetCreature(*me, _bunnyGuid);
            me->CastSpell(bunny ? bunny : me, Spells::ShivarraSoulMissiles02, CastSpellExtraArgs(TRIGGERED_FULL_MASK));
        }

        TaskScheduler _scheduler;
        ObjectGuid _bunnyGuid = ObjectGuid::Empty;
    };

    // 93759 - Jace Darkweaver <Illidari>
    struct npc_jace_darkweaver : public ScriptedAI
    {
        npc_jace_darkweaver(Creature* creature) : ScriptedAI(creature)
        {
            me->CastSpell(me, Spells::FelChannelling, CastSpellExtraArgs(TRIGGERED_FULL_MASK));
        }

        void MoveInLineOfSight(Unit* who) override
        {
            Player* player = who->ToPlayer();
            if (!player)
                return;

            if (!me->IsWithinDist(player, Misc::JaceGreetingRange))
                return;

            if (player->GetQuestStatus(Quests::EyeOnThePrize) != QUEST_STATUS_COMPLETE)
                return;

            // greet each player only once
            if (_greetedPlayers.insert(player->GetGUID()).second)
                Talk(CreatureText::JaceEyeOnThePrizeGreeting, player);
        }

        void OnQuestAccept(Player* player, Quest const* quest) override
        {
            if (quest->GetQuestId() == Quests::MeetingWithTheQueen)
            {
                Talk(CreatureText::JaceMeetingWithTheQueenAccept, player);
            }

            if (quest->GetQuestId() == Quests::BeforeWereOverun)
            {
                Talk(CreatureText::JaceBeforeWereOverunAccept, player);

                _scheduler.Schedule(5s, [this, player](TaskContext const& /*context*/)
                    {
                        if (me && player)
                            Talk(CreatureText::JaceBeforeWereOverunAccept2, player);
                    });
            }
        }

        void UpdateAI(uint32 diff) override
        {
            _scheduler.Update(diff);
        }

    private:
        GuidUnorderedSet _greetedPlayers;
        TaskScheduler _scheduler;
    };

    // 96884 - Coilskar Sea-Caller (summoned by spell 191668, the player is the summoner)
    struct npc_coilskar_sea_caller : public ScriptedAI
    {
        npc_coilskar_sea_caller(Creature* creature) : ScriptedAI(creature)
        {
            _events.ScheduleEvent(Events::SeaCallerGreeting, 500ms);
            _events.ScheduleEvent(Events::SeaCallerSpellRotation, 5s);
        }

        void JustEngagedWith(Unit* who) override
        {
            Talk(CreatureText::SeaCallerEngage, who);
        }

        void UpdateAI(uint32 diff) override
        {
            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case Events::SeaCallerGreeting:
                        if (me->IsSummon())
                            if (Unit* summoner = me->ToTempSummon()->GetSummonerUnit())
                                if (Player* player = summoner->ToPlayer())
                                    Talk(CreatureText::SeaCallerGreeting, player);
                        break; // not rescheduled - fires once
                    case Events::SeaCallerSpellRotation:
                        if (!me->HasUnitState(UNIT_STATE_CASTING))
                            DoHealOrBolt();
                        _events.Repeat(5s);
                        break;
                    default:
                        break;
                }
            }

            if (!UpdateVictim())
                return;

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            me->DoMeleeAttackIfReady();
        }

    private:
        EventMap _events;

        void DoHealOrBolt()
        {
            Unit* owner = me->GetCharmerOrOwnerPlayerOrPlayerItself();
            bool selfNeedsHeal  = me->HealthBelowPct(50);
            bool ownerNeedsHeal = owner && owner->HealthBelowPct(50);

            // heal whoever is lower if both are hurt
            if (selfNeedsHeal && (!ownerNeedsHeal || me->GetHealthPct() <= owner->GetHealthPct()))
                me->CastSpell(me, Spells::SeaCallerHealingWave);
            else if (ownerNeedsHeal)
                me->CastSpell(owner, Spells::SeaCallerHealingWave);
            else if (Unit* victim = me->GetVictim())
                me->CastSpell(victim, Spells::SeaCallerLightningBolt);
        }
    };

    // 93221 - Doom Commander Beliash
    struct npc_doom_commander_beliash : public ScriptedAI
    {
        npc_doom_commander_beliash(Creature* creature) : ScriptedAI(creature)
        {
            // MoveInLineOfSight is gated by m_SightDistance (MonsterSight, 50y default) -
            // raise it so the 100y conversation trigger can actually fire.
            me->m_SightDistance = Misc::BeliashConversationRange;
        }

        void Reset() override
        {
            _events.Reset();
        }

        void MoveInLineOfSight(Unit* who) override
        {
            if (Player* player = who->ToPlayer())
                if(player->GetQuestStatus(Quests::BeforeWereOverun) == QUEST_STATUS_INCOMPLETE)
                    if (me->IsWithinDist(player, Misc::BeliashConversationRange))
                        if (_conversedPlayers.insert(player->GetGUID()).second)
                            Conversation::CreateConversation(Conversations::DoomCommanderBeliash, player, player->GetPosition(), { player->GetGUID() });

            // keep base aggro behavior - this is a hostile boss
            ScriptedAI::MoveInLineOfSight(who);
        }

        void JustEngagedWith(Unit* /*who*/) override
        {
            if (Creature* tyranna = me->FindNearestCreature(Creatures::QueenTyranna, Misc::BeliashConversationRange))
                tyranna->AI()->Talk(CreatureText::QueenTyrannaAggro, tyranna);

            me->m_Events.AddEventAtOffset([this]()
                {
                    if(me && me->IsAlive())
                        Talk(CreatureText::BeliashAggro);
                }, 5s);

            me->m_Events.AddEventAtOffset([this]()
                {
                    if (me && me->IsAlive())
                        Talk(CreatureText::BeliashAggro2);
                }, 8s);
            
            _events.ScheduleEvent(Events::BeliashShadowBlaze, 10s);
            _events.ScheduleEvent(Events::BeliashShadowBoltVolley, 8s);
            _events.ScheduleEvent(Events::BeliashShadowRetreat, 12s);            
        }

        void JustDied(Unit* killer) override
        {
            // covers kills by pets/summons/controlled units, not just the player directly
            if (killer)
                if (Player* player = killer->GetCharmerOrOwnerPlayerOrPlayerItself())
                    player->KilledMonsterCredit(Creatures::BeliashKillCredit);

            if (Creature* tyranna = me->FindNearestCreature(Creatures::QueenTyranna, Misc::BeliashConversationRange))
                tyranna->CastSpell(tyranna, Spells::BroodQueenTyrannaTeleport);

            me->DespawnOrUnsummon(15s, 5min);
        }

        void UpdateAI(uint32 diff) override
        {
            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case Events::BeliashShadowBlaze:
                        DoCast(Spells::ShadowBlaze);
                        _events.Repeat(10s);
                        break;
                    case Events::BeliashShadowBoltVolley:
                        DoCast(Spells::ShadowBoltVolley);
                        _events.Repeat(8s);
                        break;
                    case Events::BeliashShadowRetreat:
                        DoCast(Spells::ShadowRetreat);
                        _events.Repeat(12s);
                        break;
                    default:
                        break;
                }
            }

            if (!UpdateVictim())
                return;

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            me->DoMeleeAttackIfReady();
        }

    private:
        EventMap _events;
        GuidUnorderedSet _conversedPlayers;
    };

    // 100161 - Legion Devastator
    struct npc_legion_devastator : public ScriptedAI
    {
        npc_legion_devastator(Creature* creature) : ScriptedAI(creature)
        {
            // MoveInLineOfSight is gated by m_SightDistance (MonsterSight, 50y default) -
            // raise it so the 200y bombardment trigger can actually fire.
            me->m_SightDistance = Misc::BombardmentRange;
            me->setActive(true);
            me->SetFarVisible(true);
        }

        void Reset() override
        {
            _events.Reset();
            _bombardmentActive = false;
        }

        void MoveInLineOfSight(Unit* who) override
        {
            if (Player* player = who->ToPlayer())
                if (player->GetQuestStatus(Quests::EnterTheIllidariShivarra) == QUEST_STATUS_COMPLETE)
                    if (me->IsWithinDist(player, Misc::BombardmentRange, true, false))
                    {
                        if (!_bombardmentActive)
                        {
                            _bombardmentActive = true;
                            _bombardingPlayer = player->GetGUID();
                            _events.ScheduleEvent(Events::DevastatorBombardment, 0s);
                        }
                        if (_conversedPlayers.insert(player->GetGUID()).second)
                            _events.ScheduleEvent(Events::DevastatorBombardmentConversation, 5s);
                    }

            ScriptedAI::MoveInLineOfSight(who);
        }

        void UpdateAI(uint32 diff) override
        {
            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case Events::DevastatorBombardment:
                    {
                        Player* player = ObjectAccessor::GetPlayer(*me, _bombardingPlayer);
                        if (player && me->IsWithinDist(player, Misc::BombardmentRange, true, false))
                        {
                            me->CastSpell(nullptr, Spells::FelBombardment);
                            _events.Repeat(3s);
                        }
                        else
                            _bombardmentActive = false;
                        break;
                    }
                    case Events::DevastatorBombardmentConversation:
                        if (Player* player = ObjectAccessor::GetPlayer(*me, _bombardingPlayer))
                            Conversation::CreateConversation(Conversations::FelBombardments, player, player->GetPosition(), { player->GetGUID() });
                        break;
                    default:
                        break;
                }
            }
        }

    private:
        EventMap _events;
        ObjectGuid _bombardingPlayer;
        GuidUnorderedSet _conversedPlayers;
        bool _bombardmentActive = false;
    };
}

void AddSC_custom_mardum_npcs()
{
    using namespace Scripts::Custom::Mardum;

    RegisterCreatureAI(npc_fel_spreader);
    RegisterCreatureAI(npc_ashtongue_mystic);
    RegisterCreatureAI(npc_jace_darkweaver);
    RegisterCreatureAI(npc_coilskar_sea_caller);
    RegisterCreatureAI(npc_doom_commander_beliash);
    RegisterCreatureAI(npc_legion_devastator);
}
