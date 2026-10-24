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

#include "Containers.h"
#include "Conversation.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "EventMap.h"
#include "GameObject.h"
#include "Map.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "ObjectGuid.h"
#include "PhasingHandler.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "ScriptMgr.h"
#include "SpellAuras.h"
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

    // 97059 - King Voras
    struct npc_king_voras : public ScriptedAI
    {
        npc_king_voras(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            _events.Reset();
            _lowHealthTalked = false;
        }

        void JustEngagedWith(Unit* who) override
        {
            Talk(CreatureText::VorasAggro, who);
            _events.ScheduleEvent(Events::VorasAcidSpit, 4s, 5s);
        }

        void DamageTaken(Unit* /*attacker*/, uint32& /*damage*/, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo*/) override
        {
            if (!_lowHealthTalked && me->HealthBelowPct(50))
            {
                _lowHealthTalked = true;
                Talk(CreatureText::VorasLowHealth);
            }
        }

        void JustDied(Unit* killer) override
        {
            Talk(CreatureText::VorasDeathEmote);
            Talk(CreatureText::VorasDeath);

            // covers kills by pets/summons/controlled units, not just the player directly
            if (killer)
                if (Player* player = killer->GetCharmerOrOwnerPlayerOrPlayerItself())
                    player->m_Events.AddEventAtOffset([player]()
                        {
                            Conversation::CreateConversation(Conversations::KingVorasDeath, player, player->GetPosition(), { player->GetGUID() });
                        }, 5s);
        }

        void UpdateAI(uint32 diff) override
        {
            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case Events::VorasAcidSpit:
                        DoCastVictim(Spells::AcidSpit);
                        _events.Repeat(4s, 5s);
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
        bool _lowHealthTalked = false;
    };

    // 96562 Ashtongue Stalker / 96564 Priestess of Dementia / 96561 Coilskar Harpooner
    // "Stop the Bombardment" (38727) - sabotage trigger npcs, activated by
    // SetData from go_mardum_illidari_banner
    struct npc_illidari_banner_trigger : public ScriptedAI
    {
        npc_illidari_banner_trigger(Creature* creature) : ScriptedAI(creature)
        {
            DetermineTargets();
        }

        void Reset() override
        {
            _scheduler.CancelAll();
        }

        void JustEngagedWith(Unit* /*who*/) override
        {
            if (me->GetEntry() != Creatures::CoilskarHarpooner)
                return;

            _scheduler.Schedule(5s, [this](TaskContext context)
            {
                DoCastVictim(Spells::CoilskarThrowSpear);
                context.Repeat();
            });
        }

        void SetData(uint32 id, uint32 value) override
        {
            if (_activated || id != Misc::BannerTriggered || value != 1)
                return;

            Creature* devastator = me->FindNearestCreature(_devastatorEntry, Misc::SearchRange);
            if (!devastator)
                return;

            _activated = true;
            _devastatorGuid = devastator->GetGUID();

            _scheduler.Schedule(3s, [this](TaskContext const& /*context*/)
            {
                if (Creature* devastator = ObjectAccessor::GetCreature(*me, _devastatorGuid))
                    me->GetMotionMaster()->MoveCloserAndStop(Movement::ReachedDevastator, devastator, Misc::DevastatorStandoff);
            });
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type != POINT_MOTION_TYPE || id != Movement::ReachedDevastator)
                return;

            Creature* devastator = ObjectAccessor::GetCreature(*me, _devastatorGuid);
            if (!devastator)
                return;

            devastator->CastSpell(devastator, Spells::StopBombardmentDevastator, true);
            devastator->CastSpell(devastator, Spells::StopBombardmentKnockback, true);

            _scheduler.Schedule(3s, [this](TaskContext const& /*context*/)
            {
                Creature* devastator = ObjectAccessor::GetCreature(*me, _devastatorGuid);
                if (!devastator)
                    return;

                std::vector<Player*> players;
                devastator->GetPlayerListInGrid(players, Misc::SearchRange);
                for (Player* player : players)
                    player->KilledMonsterCredit(_devastatorEntry);

                // TO-DO The devastator should play a special anim when dies/dead
                // devastator->KillSelf();
                devastator->DisappearAndDie();
            });

            me->GetMotionMaster()->MoveTargetedHome();
        }

        void UpdateAI(uint32 diff) override
        {
            _scheduler.Update(diff);
        }

    private:
        bool _activated = false;
        uint32 _devastatorEntry = 0;
        ObjectGuid _devastatorGuid;
        TaskScheduler _scheduler;

        void DetermineTargets()
        {
            switch (me->GetEntry())
            {
            case Creatures::AshtongueStalker:
                _devastatorEntry = Creatures::DevastatorDoomFortress;
                break;
            case Creatures::PriestessOfDementia:
                _devastatorEntry = Creatures::DevastatorForgeOfCorruption;
                break;
            case Creatures::CoilskarHarpooner:
                _devastatorEntry = Creatures::DevastatorSoulEngine;
                break;
            default:
                break;
            }
        }
    };

    // 101947 - Doom Fortress Stabilizer ("Their Numbers Are Legion" 38819)
    // npc_spellclick: player casts 201112 Destroying Stabilizer on it
    struct npc_doom_fortress_stabilizer : public ScriptedAI
    {
        npc_doom_fortress_stabilizer(Creature* creature) : ScriptedAI(creature) { }

        void SpellHit(WorldObject* caster, SpellInfo const* spellInfo) override
        {
            if (spellInfo->Id != Spells::DestroyingStabilizer)
                return;

            caster->CastSpell(me, Spells::StabilizerFelExplosion, true);
            me->DespawnOrUnsummon(1s);

            Player* player = caster->ToPlayer();
            if (!player)
                return;

            player->KilledMonsterCredit(Creatures::DoomFortressStabilizer);

            if (!player->IsQuestObjectiveComplete(Quests::TheirNumbersAreLegion, Objectives::FirstStabilizerDestroyed))
            {
                player->KilledMonsterCredit(Creatures::FirstStabilizerKillCredit);
                Conversation::CreateConversation(Conversations::DoomFortressStabilizer, player, *player, player->GetGUID());
            }
        }
    };

    // 97057 - Overseer Brutarg
    struct npc_overseer_brutarg : public ScriptedAI
    {
        npc_overseer_brutarg(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            _events.Reset();
            _equippedSword = false;
        }

        void JustEngagedWith(Unit* who) override
        {
            Talk(CreatureText::BrutargAggro, who);

            // who may be a companion/minion that pulled aggro first - resolve the owning player
            if (Player* player = who->GetCharmerOrOwnerPlayerOrPlayerItself())
            {
                if (!player->HasAura(Spells::SummonCoilskarSeaCaller))
                    player->CastSpell(player, Spells::SummonCoilskarSeaCaller, true);
                if (!player->HasAura(Spells::SummonShivarraDestroyer))
                    player->CastSpell(player, Spells::SummonShivarraDestroyer, true);
                if (!player->HasAura(Spells::SummonMatronMotherMalevolence))
                    player->CastSpell(player, Spells::SummonMatronMotherMalevolence, true);
            }

            me->CastSpell(who, Spells::BrutargCharge);

            _events.ScheduleEvent(Events::BrutargMeatGrinder, 10s, 15s);
        }

        void DamageTaken(Unit* /*attacker*/, uint32& /*damage*/, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo*/) override
        {
            if (!_equippedSword && me->HealthBelowPct(50))
            {
                _equippedSword = true;
                Talk(CreatureText::BrutargEquipSword, me->GetVictim());
                me->CastSpell(me, Spells::BrutargEquipSword, true);
            }
        }

        void JustDied(Unit* killer) override
        {
            Talk(CreatureText::BrutargDeath, killer);

            // dismiss the player's Shivarra Destroyer companion once Brutarg falls
            if (Player* player = killer->GetCharmerOrOwnerPlayerOrPlayerItself())
            {
                std::list<TempSummon*> minions;
                player->GetAllMinionsByEntry(minions, Creatures::ShivarraDestroyer);
                for (TempSummon* minion : minions)
                    minion->UnSummon();
            }
        }

        void OnChannelFinished(SpellInfo const* spellInfo) override
        {
            if (spellInfo->Id == Spells::BrutargMeatGrinder)
                DoCastSelf(Spells::BrutargDizzy);
        }

        void UpdateAI(uint32 diff) override
        {
            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case Events::BrutargMeatGrinder:
                        DoCastSelf(Spells::BrutargMeatGrinder);
                        _events.Repeat(10s, 15s);
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
        bool _equippedSword = false;
    };

    // 97058 - Count Nefarious
    struct npc_count_nefarious : public ScriptedAI
    {
        npc_count_nefarious(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            _events.Reset();
            _scheduler.CancelAll();
            me->RemoveUnitFlag(UNIT_FLAG_PET_IN_COMBAT);
            _felShieldActive = false;
            _aggroPlayerGuid = ObjectGuid::Empty;
        }

        void JustEngagedWith(Unit* who) override
        {
            ObjectGuid engagerGuid;
            if (Player* player = who->GetCharmerOrOwnerPlayerOrPlayerItself())
                engagerGuid = player->GetGUID();

            _scheduler.Schedule(500ms, [this, engagerGuid](TaskContext const& /*context*/)
                {
                    Player* player = ObjectAccessor::GetPlayer(*me, engagerGuid);
                    if (!player)
                        return;

                    _aggroPlayerGuid = engagerGuid;

                    std::list<TempSummon*> minions;
                    player->GetAllMinionsByEntry(minions, Creatures::SummonedAshtongueStalker);
                    if (minions.empty())
                        player->CastSpell(player, Spells::SummonAshtongueStalker, true);
                });

            Talk(CreatureText::NefariousAggro);

            _events.ScheduleEvent(Events::NefariousConversation, 5s);
            _events.ScheduleEvent(Events::NefariousVampiricCleave, 3s, 5s);
            _events.ScheduleEvent(Events::NefariousCarrionStorm, 10s, 15s);
        }

        void JustDied(Unit* killer) override
        {
            if (Player* player = killer->GetCharmerOrOwnerPlayerOrPlayerItself())
            {
                std::list<TempSummon*> minions;
                player->GetAllMinionsByEntry(minions, Creatures::SummonedAshtongueStalker);
                for (TempSummon* minion : minions)
                    minion->UnSummon();
            }
        }

        void SetData(uint32 id, uint32 /*value*/) override
        {
            if (id == Misc::WellOfSoulsOpened)
            {
                me->InterruptNonMeleeSpells(false);
                DoCastSelf(Spells::NefariousTorrentOfSouls);
            }
        }

        void OnAuraApplied(AuraApplication const* aurApp) override
        {
            if (aurApp->GetBase()->GetId() == Spells::NefariousCarrionStorm)
                me->AddAura(Spells::NefariousCarrionStormVisual, me);
        }

        void OnAuraRemoved(AuraApplication const* aurApp) override
        {
            if (aurApp->GetBase()->GetId() == Spells::NefariousTorrentOfSouls)
                me->RemoveAurasDueToSpell(Spells::NefariousTorrentOfSoulsBeam);
            else if (aurApp->GetBase()->GetId() == Spells::NefariousCarrionStorm)
                me->RemoveAurasDueToSpell(Spells::NefariousCarrionStormVisual);
        }

        void DamageTaken(Unit* /*attacker*/, uint32& /*damage*/, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo*/) override
        {
            if (!_felShieldActive && me->HealthBelowPct(30))
            {
                _felShieldActive = true;
                _events.ScheduleEvent(Events::NefariousFelShield, 0s);
            }
        }

        void OnSpellCast(SpellInfo const* spellInfo) override
        {
            if (spellInfo->Id == Spells::NefariousCarrionStorm)
                Talk(CreatureText::NefariousCarrionAfterCast);
            else if (spellInfo->Id == Spells::NefariousTorrentOfSouls)
                me->AddAura(Spells::NefariousTorrentOfSoulsBeam, me);
        }

        void UpdateAI(uint32 diff) override
        {
            _scheduler.Update(diff);

            if (!UpdateVictim())
                return;

            _events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case Events::NefariousConversation:
                        if (Player* player = ObjectAccessor::GetPlayer(*me, _aggroPlayerGuid))
                            Conversation::CreateConversation(Conversations::CountNefarious, player, *player, player->GetGUID());
                        break;
                    case Events::NefariousVampiricCleave:
                        DoCastVictim(Spells::NefariousVampiricCleave);
                        _events.Repeat(12s, 15s);
                        break;
                    case Events::NefariousCarrionStorm:
                        if (DoCastSelf(Spells::NefariousCarrionStorm) == SPELL_CAST_OK)
                            Talk(CreatureText::NefariousCarrionBeforeCast);
                        _events.Repeat(18s, 22s);
                        break;
                    case Events::NefariousFelShield:
                        me->CastSpell(me, Spells::NefariousFelShield);
                        _events.Repeat(22s, 25s);
                        break;
                    default:
                        break;
                }
            }

            me->DoMeleeAttackIfReady();
        }

    private:
        EventMap _events;
        TaskScheduler _scheduler;
        ObjectGuid _aggroPlayerGuid;
        bool _felShieldActive = false;
    };

    // 100549 - Wounded Demon Hunter
    // 100545 - Wounded Demon Hunter
    // 100543 - Wounded Demon Hunter
    // npc_spellclick_spells makes the clicker cast 192262 on it; the spell lands and wakes it.
    struct npc_wounded_demon_hunter : public ScriptedAI
    {
        npc_wounded_demon_hunter(Creature* creature) : ScriptedAI(creature)
        {
            me->SetNpcFlag(UNIT_NPC_FLAG_SPELLCLICK);
            me->SetStandState(UNIT_STAND_STATE_DEAD);
        }

        void SpellHit(WorldObject* caster, SpellInfo const* spellInfo) override
        {
            if (spellInfo->Id != Spells::WoundedDemonHunterRescue || _awakened)
                return;

            _awakened = true;

            if (caster->IsPlayer())
                caster->ToPlayer()->KilledMonsterCredit(Creatures::CreditWoundedDemonHunter);

            me->RemoveNpcFlag(UNIT_NPC_FLAG_SPELLCLICK);
            me->SetStandState(UNIT_STAND_STATE_STAND);
            me->SetFaction(Factions::Illidari);
            Talk(CreatureText::WoundedDemonHunterAwakened);
            me->DespawnOrUnsummon(60min);
        }

    private:
        bool _awakened = false;
    };

    /*######
    ## 102916 - Well of Souls ("Their Numbers Are Legion" 38819)
    ## Summoned by the Cylinder Collision gameobjects around Count Nefarious.
    ## npc_spellclick_spells makes the clicker cast 198185 Opening on it;
    ## when the spell lands it notifies Count Nefarious via SetData.
    ######*/
    struct npc_well_of_souls : public ScriptedAI
    {
        npc_well_of_souls(Creature* creature) : ScriptedAI(creature)
        {
            me->SetNpcFlag(UNIT_NPC_FLAG_SPELLCLICK);
        }

        void SpellHit(WorldObject* /*caster*/, SpellInfo const* spellInfo) override
        {
            if (spellInfo->Id != Spells::WellOfSoulsOpening)
                return;

            if (Creature* nefarious = me->FindNearestCreature(Creatures::CountNefarious, Misc::SearchRange))
                nefarious->AI()->SetData(Misc::WellOfSoulsOpened, 1);
        }
    };

    /*######
    ## 97370 - General Volroth (Molten Shore rare)
    ## Summons captive Coilskar guards in the constructor. On death one
    ## captive says a line, then they run off and despawn - the delayed
    ## work is scheduled on the captives' own event processors because
    ## this AI stops ticking once the boss is dead.
    ######*/
    struct VolrothCaptiveSpawn
    {
        uint32 Entry;
        Position Pos;
    };

    static VolrothCaptiveSpawn const VolrothCaptiveSpawns[] =
    {
        { Creatures::CoilskarSeaCallerCaptive, {1904.889f, 1444.656f, 108.0866f, 1.915215f } },
        { Creatures::CoilskarHarpooner, { 1901.706f, 1444.352f, 108.0862f, 2.292977f } },
        { Creatures::CoilskarMyrmidon,  { 1903.295f, 1449.102f, 108.0872f, 2.523181f } },
        { Creatures::CoilskarMyrmidon,  { 1905.094f, 1454.895f, 108.8267f, 3.366357f } },
        { Creatures::CoilskarHarpooner, { 1899.933f, 1451.458f, 108.0863f, 2.292977f } },
        // { PLACEHOLDER captive entry, { x, y, z, o } },
    };

    static Position const VolrothCaptiveExitPos = { 1762.78f, 1515.98f, 64.753f };

    struct npc_general_volroth : public ScriptedAI
    {
        npc_general_volroth(Creature* creature) : ScriptedAI(creature)
        {
            SpawnCaptives();
        }

        void Reset() override
        {
            _events.Reset();
            _scheduler.CancelAll();
            _engagerGuid = ObjectGuid::Empty;
        }

        void JustEngagedWith(Unit* who) override
        {
            Talk(CreatureText::VolrothAggro);

            // who may be a companion/minion that pulled aggro first - resolve the owning player
            if (Player* player = who->GetCharmerOrOwnerPlayerOrPlayerItself())
                _engagerGuid = player->GetGUID();

            _scheduler.Schedule(4s, [this](TaskContext const& /*context*/)
            {
                Player* player = ObjectAccessor::GetPlayer(*me, _engagerGuid);
                if (!player)
                    return;

                std::list<TempSummon*> minions;
                player->GetAllMinionsByEntry(minions, Creatures::CoilskarSeaCaller);
                if (minions.empty())
                    player->CastSpell(player, Spells::SummonCoilskarSeaCaller, true);
            });

            CombatCaptives();

            _events.ScheduleEvent(Events::VolrothBurnWithin, 10s);
            _events.ScheduleEvent(Events::VolrothFelBreath, 5s);
        }

        void JustDied(Unit* killer) override
        {
            Talk(CreatureText::VolrothDeath);

            // the killing blow may come from a captive or the player's own minion -
            // fall back through the threat list and the stored engager
            Player* player = killer ? killer->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr;

            if (!player)
                for (ThreatReference const* ref : me->GetThreatManager().GetUnsortedThreatList())
                    if ((player = ref->GetVictim()->GetCharmerOrOwnerPlayerOrPlayerItself()))
                        break;

            if (!player)
                player = ObjectAccessor::GetPlayer(*me, _engagerGuid);

            if (player)
            {
                std::list<TempSummon*> minions;
                player->GetAllMinionsByEntry(minions, Creatures::CoilskarSeaCaller);
                for (TempSummon* minion : minions)
                    minion->UnSummon();
            }

            ReleaseCaptives();
        }

        void OnSpellCast(SpellInfo const* spellInfo) override
        {
            if (spellInfo->Id == Spells::VolrothBurnWithin)
                Talk(CreatureText::VolrothBurnWithin);
        }

        void UpdateAI(uint32 diff) override
        {
            _scheduler.Update(diff);

            if (!UpdateVictim())
                return;

            _events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case Events::VolrothBurnWithin:
                        DoCastVictim(Spells::VolrothBurnWithin);
                        _events.Repeat(18s, 25s);
                        break;
                    case Events::VolrothFelBreath:
                        DoCastVictim(Spells::VolrothFelBreath);
                        _events.Repeat(12s, 15s);
                        break;
                    default:
                        break;
                }
            }

            me->DoMeleeAttackIfReady();
        }

    private:
        EventMap _events;
        TaskScheduler _scheduler;
        ObjectGuid _engagerGuid;
        std::vector<ObjectGuid> _captiveGUIDs;

        void SpawnCaptives()
        {
            for (VolrothCaptiveSpawn const& spawn : VolrothCaptiveSpawns)
                if (Creature* captive = me->SummonCreature(spawn.Entry, spawn.Pos))
                {
                    _captiveGUIDs.push_back(captive->GetGUID());
                    captive->SetReactState(REACT_PASSIVE);
                    captive->SetSessile(true);
                    PhasingHandler::SetAlwaysVisible(captive, true, true);
                }
        }

        void CombatCaptives()
        {
            for (ObjectGuid guid : _captiveGUIDs)
                if (Creature* captive = ObjectAccessor::GetCreature(*me, guid))
                    if (captive->IsAlive())
                    {
                        captive->SetReactState(REACT_AGGRESSIVE);
                        captive->RemoveUnitFlag(UNIT_FLAG_IMMUNE_TO_NPC);
                    }
        }

        void ReleaseCaptives()
        {
            std::vector<Creature*> captives;
            for (ObjectGuid guid : _captiveGUIDs)
                if (Creature* captive = ObjectAccessor::GetCreature(*me, guid))
                    if (captive->IsAlive())
                        captives.push_back(captive);

            if (captives.empty())
                return;

            if (captives.front()->AI())
                captives.front()->AI()->Talk(CreatureText::VolrothCaptiveReleased);

            for (Creature* captive : captives)
            {
                captive->m_Events.AddEventAtOffset([captive]()
                {
                    captive->SetSessile(false);
                    captive->SetControlled(false, UNIT_STATE_ROOT);   // sessile unroot never fires without a gravity aura
                    captive->SetDisableGravity(false, false, true);   // restore gravity disabled by sessile
                    captive->GetMotionMaster()->Clear();
                    captive->GetMotionMaster()->MovePoint(Movement::CaptivesDespawn, VolrothCaptiveExitPos);

                    captive->m_Events.AddEventAtOffset([captive]()
                    {
                        captive->DespawnOrUnsummon();
                    }, 30s);
                }, 5s);
            }
        }
    };

    // 98711 - Kor'vas Bloodthorn (Foul Creche)
    struct npc_korvas_bloodthorn_foul_creche : public ScriptedAI
    {
        npc_korvas_bloodthorn_foul_creche(Creature* creature) : ScriptedAI(creature) { }

        void OnQuestAccept(Player* player, Quest const* quest) override
        {
            if (quest->GetQuestId() != Quests::TheImpMothersTome)
                return;

            ObjectGuid playerGuid = player->GetGUID();

            _scheduler.Schedule(3s, [this, playerGuid](TaskContext const& /*context*/)
            {
                if (Player* player = ObjectAccessor::GetPlayer(*me, playerGuid))
                {
                    Talk(CreatureText::KorvasImpMothersTomeAccept, player);

                    _scheduler.Schedule(5s, [this, playerGuid](TaskContext const& /*context*/)
                    {
                        if (Player* player = ObjectAccessor::GetPlayer(*me, playerGuid))
                            Talk(CreatureText::KorvasImpMothersTomeAccept2, player);
                    });
                }
            });

            // Scheduled on the player's EventProcessor so it is dropped on logout
            player->m_Events.AddEventAtOffset([player]()
            {
                std::list<TempSummon*> minions;
                player->GetAllMinionsByEntry(minions, Creatures::ShivarraDestroyer);
                if (minions.empty())
                    player->CastSpell(player, Spells::SummonShivarraDestroyer, CastSpellExtraArgs(TRIGGERED_FULL_MASK));
            }, 30s);
        }

        void UpdateAI(uint32 diff) override
        {
            _scheduler.Update(diff);
        }

    private:
        TaskScheduler _scheduler;
    };

    // 98986 - Prolifica (Foul Creche rare)
    struct npc_prolifica : public ScriptedAI
    {
        npc_prolifica(Creature* creature) : ScriptedAI(creature)
        {
            me->SetSessile(true);

            // the tome can spawn after us in the same grid load - retry once deferred
            if (!SetTomeVisibility(true))
                _scheduler.Schedule(1s, [this](TaskContext const& /*context*/)
                {
                    SetTomeVisibility(true);
                });
        }

        void Reset() override
        {
            _events.Reset();
            _engagerGuid = ObjectGuid::Empty;
            _lowHealthTalked = false;
        }

        void EnterEvadeMode(EvadeReason why) override
        {
            SetTomeVisibility(true);
            ScriptedAI::EnterEvadeMode(why);
        }

        void JustEngagedWith(Unit* who) override
        {
            Talk(CreatureText::ProlificaAggro, who);

            if (Player* player = who->GetCharmerOrOwnerPlayerOrPlayerItself())
                _engagerGuid = player->GetGUID();

            SetTomeVisibility(false);

            _events.ScheduleEvent(Events::ProlificaFelGeyser, 10s);
            _events.ScheduleEvent(Events::ProlificaGaseousBreath, 10s);
        }

        void DamageTaken(Unit* /*attacker*/, uint32& /*damage*/, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
        {
            if (_lowHealthTalked || !me->HealthBelowPct(50))
                return;

            _lowHealthTalked = true;

            if (Player* player = ObjectAccessor::GetPlayer(*me, _engagerGuid))
                Talk(CreatureText::ProlificaLowHealth, player);
        }

        void JustDied(Unit* killer) override
        {
            // since we do not know the phase id for this npc we force it always visible before death for loot
            // it is now part of the quest giver phase 5344
            // current phase ends when quest is complete so we need to extend to be visible also when dead
            // a second respawn will no longer be visible for the same player
            PhasingHandler::SetAlwaysVisible(me, true, true);

            Talk(CreatureText::ProlificaDeath, killer);

            // the killing blow may come from a companion - resolve the owning player
            if (Player* player = killer->GetCharmerOrOwnerPlayerOrPlayerItself())
            {
                PhasingHandler::OnConditionChange(player, true);

                std::list<TempSummon*> minions;
                player->GetAllMinionsByEntry(minions, Creatures::ShivarraDestroyer);
                for (TempSummon* minion : minions)
                    minion->UnSummon();
            }

            // Scheduled on the player's EventProcessor so it is dropped on logout
            if (Player* player = ObjectAccessor::GetPlayer(*me, _engagerGuid))
                player->m_Events.AddEventAtOffset([player]()
                {
                    Conversation::CreateConversation(Conversations::ProlificaDeath, player, *player, player->GetGUID());
                }, 10s);
        }

        void UpdateAI(uint32 diff) override
        {
            _scheduler.Update(diff);

            if (!UpdateVictim())
                return;

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case Events::ProlificaFelGeyser:
                    {
                        std::list<Creature*> geysers;
                        me->GetCreatureListWithEntryInGrid(geysers, Creatures::FelGeyser, Misc::FelGeyserSearchRange);
                        if (Creature* geyser = Trinity::Containers::SelectRandomContainerElement(geysers))
                            DoCast(geyser, Spells::ProlificaFelGeyser);
                        _events.Repeat(15s, 20s);
                        break;
                    }
                    case Events::ProlificaGaseousBreath:
                        DoCastVictim(Spells::ProlificaGaseousBreath);
                        _events.Repeat(10s, 15s);
                        break;
                    default:
                        break;
                }
            }
        }

    private:
        bool SetTomeVisibility(bool visible)
        {
            FindGameObjectOptions options;
            options.GameObjectId = GameObjects::SecretTome;
            options.IgnorePhases = true;

            if (GameObject* tome = me->FindNearestGameObjectWithOptions(Misc::SearchRange, options))
            {
                PhasingHandler::SetAlwaysVisible(tome, visible, true);
                return true;
            }

            return false;
        }

        EventMap _events;
        TaskScheduler _scheduler;
        ObjectGuid _engagerGuid;
        bool _lowHealthTalked = false;
    };

    // 97676 - Lady S'theno (Soul Engine)
    struct npc_lady_stheno : public ScriptedAI
    {
        npc_lady_stheno(Creature* creature) : ScriptedAI(creature)
        {
            _scheduler.Schedule(5s, [this](TaskContext context)
            {
                MaintainAttackers();
                context.Repeat();
            });
        }

        void JustAppeared() override
        {
            SpawnAttackers();
        }

        void Reset() override
        {
            _events.Reset();
        }

        void MoveInLineOfSight(Unit* who) override
        {
            if (_greetingOnCooldown || !who->IsPlayer())
                return;

            _greetingOnCooldown = true;
            Talk(CreatureText::SthenoGreeting, who);

            _scheduler.Schedule(10min, [this](TaskContext const& /*context*/)
            {
                _greetingOnCooldown = false;
            });
        }

        void JustEngagedWith(Unit* /*who*/) override
        {
            _events.ScheduleEvent(Events::SthenoAqueousShield, 1s);
            _events.ScheduleEvent(Events::SthenoHealingWave, 15s);
        }

        void DamageTaken(Unit* attacker, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo*/) override
        {
            if (attacker->IsCreature() && me->HealthBelowPct(90))
                damage = 1;
        }

        void UpdateAI(uint32 diff) override
        {
            _scheduler.Update(diff);

            if (!UpdateVictim())
                return;

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case Events::SthenoAqueousShield:
                        DoCastSelf(Spells::SthenoAqueousShield);
                        _events.Repeat(15s);
                        break;
                    case Events::SthenoHealingWave:
                        if (me->HealthBelowPct(50))
                            DoCastSelf(Spells::SeaCallerHealingWave);
                        _events.Repeat(15s);
                        break;
                    default:
                        break;
                }

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;
            }

            me->DoMeleeAttackIfReady();
        }

    private:
        static constexpr uint32 AttackerPool[] =
        {
            Creatures::VileSoulmaster,
            Creatures::BurningSoulstalker,
            Creatures::VileSoulmaster2,
            Creatures::BurningSoulstalker,
            Creatures::BurningSoulstalker
        };

        void SpawnAttackers()
        {
            for (uint32 i = 0; i < Misc::SthenoAttackerCount; ++i)
                SpawnAttacker();
        }

        void SpawnAttacker()
        {
            uint32 entry = Trinity::Containers::SelectRandomContainerElement(AttackerPool);
            if (Creature* attacker = me->SummonCreature(entry, me->GetRandomNearPosition(Misc::SthenoAttackerSpawnRange)))
            {
                attacker->EngageWithTarget(me);
                _attackerGuids.push_back(attacker->GetGUID());
            }
        }

        void MaintainAttackers()
        {
            std::erase_if(_attackerGuids, [this](ObjectGuid guid)
            {
                Creature* attacker = ObjectAccessor::GetCreature(*me, guid);
                return !attacker || !attacker->IsAlive();
            });

            if (_attackerGuids.size() < Misc::SthenoAttackerCount)
                SpawnAttacker();
        }

        EventMap _events;
        TaskScheduler _scheduler;
        std::vector<ObjectGuid> _attackerGuids;
        bool _greetingOnCooldown = false;
    };

    // 97297 - Cyana Nightglaive (Fel Hammer)
    // On accepting "The Keystone" (38728) she talks, then the Fel Hammer
    // Illidari engage Brood Queen Tyranna.
    struct npc_cyana_nightglaive_fel_hammer : public ScriptedAI
    {
        npc_cyana_nightglaive_fel_hammer(Creature* creature) : ScriptedAI(creature) { }

        void OnQuestAccept(Player* player, Quest const* quest) override
        {
            if (quest->GetQuestId() != Quests::TheKeystone)
                return;

            Talk(CreatureText::CyanaFelHammerAccept, player);

            ObjectGuid playerGuid = player->GetGUID();
            _scheduler.Schedule(5s, [this, playerGuid](TaskContext const& /*context*/)
            {
                if (Player* player = ObjectAccessor::GetPlayer(*me, playerGuid))
                    Talk(CreatureText::CyanaFelHammerAccept2, player);
            });

            EngageIllidariOnTyranna();
        }

        void UpdateAI(uint32 diff) override
        {
            _scheduler.Update(diff);
        }

    private:
        static constexpr uint32 IllidariEntries[] =
        {
            Creatures::KorvasFelHammer,
            Creatures::JaceFelHammer,
            Creatures::KaynFelHammer,
            Creatures::AllariFelHammer
        };

        void EngageIllidariOnTyranna()
        {
            FindCreatureOptions tyrannaOptions;
            tyrannaOptions.CreatureId = Creatures::BroodQueenTyranna;
            tyrannaOptions.IgnorePhases = true;

            Creature* tyranna = me->FindNearestCreatureWithOptions(Misc::FelHammerSearchRange, tyrannaOptions);
            if (!tyranna)
                return;

            for (uint32 entry : IllidariEntries)
            {
                FindCreatureOptions options;
                options.CreatureId = entry;
                options.IgnorePhases = true;

                if (Creature* illidari = me->FindNearestCreatureWithOptions(Misc::FelHammerSearchRange, options))
                    illidari->EngageWithTarget(tyranna);
            }
        }

        TaskScheduler _scheduler;
    };

    // 93802 - Brood Queen Tyranna (Fel Hammer boss)
    struct npc_brood_queen_tyranna : public ScriptedAI
    {
        npc_brood_queen_tyranna(Creature* creature) : ScriptedAI(creature)
        {
            GatherSpiderSpawns();

            // the spiders can spawn after us in the same grid load - retry once deferred
            if (!_spidersGathered)
                _scheduler.Schedule(1s, [this](TaskContext const& /*context*/)
                {
                    GatherSpiderSpawns();
                });
        }

        void Reset() override
        {
            _events.Reset();
            _scheduler.CancelAll();
            ReleaseGrabbedPlayer();
            DespawnBrood();
        }

        void MoveInLineOfSight(Unit* who) override
        {
            if (Player* player = who->ToPlayer())
            {
                if (_greetedPlayers.insert(player->GetGUID()).second)
                {
                    Talk(CreatureText::TyrannaGreeting, player);

                    ObjectGuid playerGuid = player->GetGUID();
                    _scheduler.Schedule(5s, [this, playerGuid](TaskContext const& /*context*/)
                    {
                        if (Player* player = ObjectAccessor::GetPlayer(*me, playerGuid))
                            Talk(CreatureText::TyrannaGreeting2, player);
                    });
                }
            }

            ScriptedAI::MoveInLineOfSight(who);
        }

        void JustEngagedWith(Unit* /*who*/) override
        {
            _events.ScheduleEvent(Events::TyrannaQueensBite, 15s, 20s);
            _events.ScheduleEvent(Events::TyrannaBroodSwarm, 30s);
        }

        void JustDied(Unit* /*killer*/) override
        {
            Talk(CreatureText::TyrannaDeath);
            ReleaseGrabbedPlayer();
            DespawnBrood();

            FindCreatureOptions options;
            options.IgnorePhases = true;
            options.CreatureId = Creatures::KaynFelHammer;

            if (Creature* kayn = me->FindNearestCreatureWithOptions(Misc::FelHammerSearchRange, options))
            {
                kayn->m_Events.AddEventAtOffset([kayn]()
                {
                    kayn->AI()->Talk(0);

                    FindCreatureOptions options;
                    options.CreatureId = Creatures::KaynKeystone;
                    options.IgnorePhases = true;
                    if (Creature* keystoneKayn = kayn->FindNearestCreatureWithOptions(Misc::FelHammerSearchRange, options))
                        kayn->GetMotionMaster()->MovePoint(0, keystoneKayn->GetPosition());
                }, 3s);

                kayn->DespawnOrUnsummon(30s);
            }
        }

        void UpdateAI(uint32 diff) override
        {
            _scheduler.Update(diff);

            if (!UpdateVictim())
                return;

            _events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case Events::TyrannaQueensBite:
                    {
                        Unit* biteTarget = me->GetVictim() && me->GetVictim()->IsPlayer()
                            ? me->GetVictim()
                            : me->SelectNearestTarget(Misc::TyrannaBiteTargetRange, true);

                        _events.Repeat(15s, 20s);

                        if (!biteTarget)
                            break;

                        // she is a vehicle npc (VehicleId 4497) - the bite seizes the player in her jaws
                        Talk(CreatureText::TyrannaQueensBite);
                        //DoCastSelf(Spells::QueensBite);
                        me->AddAura(Spells::QueensBite, biteTarget);
                        me->CastSpell(biteTarget, 197493, true); // Tyranna's Bite (buff)
                        biteTarget->EnterVehicle(me);
                        _grabbedTarget = biteTarget->GetGUID();
                        _scheduler.Schedule(5s, [this](TaskContext const& /*context*/)
                        {
                            ReleaseGrabbedPlayer();
                        });
                        break;
                    }
                    case Events::TyrannaBroodSwarm:
                        DoCastVictim(Spells::BroodSwarm);
                        Talk(CreatureText::TyrannaBroodSwarm);
                        SummonBrood();
                        _scheduler.Schedule(1s, [this](TaskContext const& /*context*/)
                        {
                            Talk(CreatureText::TyrannaIntoTheShadows);
                            me->CastSpell(me, Spells::IntoTheShadows, true);
                        });
                        _events.Repeat(30s);
                        break;
                    default:
                        break;
                }
            }

            me->DoMeleeAttackIfReady();
        }

    private:
        static constexpr uint32 SpiderEntries[] =
        {
            Creatures::SkitteringBroodling,
            Creatures::TyrannasSpawn
        };

        void GatherSpiderSpawns()
        {
            if (_spidersGathered)
                return;

            _spidersGathered = true;

            for (uint32 entry : SpiderEntries)
            {
                std::list<Creature*> spiders;
                me->GetCreatureListWithEntryInGrid(spiders, entry, Misc::FelHammerSearchRange);
                if (spiders.empty())
                {
                    _spidersGathered = false;
                    continue;
                }

                for (Creature* spider : spiders)
                {
                    _spiderSpawns[entry].push_back(spider->GetPosition());
                    spider->SetVisible(false);
                    spider->SetReactState(REACT_PASSIVE);
                    spider->SetUnitFlag(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_IMMUNE_TO_PC | UNIT_FLAG_IMMUNE_TO_NPC);
                }
            }
        }

        void SummonBrood()
        {
            for (auto const& [entry, positions] : _spiderSpawns)
                for (Position const& pos : positions)
                    if (Creature* spider = me->SummonCreature(entry, pos))
                    {
                        _broodGuids.push_back(spider->GetGUID());

                        spider->SetReactState(REACT_PASSIVE);

                        ObjectGuid spiderGuid = spider->GetGUID();
                        _scheduler.Schedule(1s, 5s, [this, spiderGuid](TaskContext const& /*context*/)
                        {
                            if (Creature* spider = ObjectAccessor::GetCreature(*me, spiderGuid))
                            {
                                spider->SetReactState(REACT_AGGRESSIVE);
                                if (Unit* target = spider->SelectNearestTarget(Misc::FelHammerSearchRange))
                                    spider->EngageWithTarget(target);
                            }
                        });
                    }
        }

        void DespawnBrood()
        {
            for (ObjectGuid guid : _broodGuids)
                if (Creature* spider = ObjectAccessor::GetCreature(*me, guid))
                    spider->DespawnOrUnsummon();

            _broodGuids.clear();
        }

        void ReleaseGrabbedPlayer()
        {
            if (Unit* grabbed = ObjectAccessor::GetUnit(*me, _grabbedTarget))
            {
                grabbed->ExitVehicle();
                grabbed->RemoveAurasDueToSpell(Spells::QueensBite);
            }

            _grabbedTarget.Clear();
        }

        EventMap _events;
        TaskScheduler _scheduler;
        GuidUnorderedSet _greetedPlayers;
        ObjectGuid _grabbedTarget;
        std::unordered_map<uint32, std::vector<Position>> _spiderSpawns;
        std::vector<ObjectGuid> _broodGuids;
        bool _spidersGathered = false;
    };

    // 101760 - "The Keystone" Make Your Way Downstairs Quest Kill Credit
    struct npc_keystone_downstairs_kill_credit : public ScriptedAI
    {
        npc_keystone_downstairs_kill_credit(Creature* creature) : ScriptedAI(creature) { }

        void MoveInLineOfSight(Unit* who) override
        {
            if (Player* player = who->ToPlayer())
            {
                if (me->IsWithinDist(player, Misc::KeystoneKillCreditRange)
                    && player->GetQuestStatus(Quests::TheKeystone) == QUEST_STATUS_INCOMPLETE
                    && player->IsQuestObjectiveComplete(Quests::TheKeystone, Objectives::KeystoneMakeYourWayDownstairs)
                    && _creditedPlayers.insert(player->GetGUID()).second)
                {
                    player->KilledMonsterCredit(Creatures::KeystoneKillCredit, me->GetGUID());
                }
            }

            ScriptedAI::MoveInLineOfSight(who);
        }

    private:
        GuidUnorderedSet _creditedPlayers;
    };

    // 97303 - Kayn Sunfury (The Keystone turn-in / Return to the Black Temple)
    struct npc_kayn_sunfury_keystone : public ScriptedAI
    {
        npc_kayn_sunfury_keystone(Creature* creature) : ScriptedAI(creature) { }

        void OnQuestReward(Player* player, Quest const* quest, LootItemType /*type*/, uint32 /*opt*/) override
        {
            if (quest->GetQuestId() == Quests::TheKeystone)
                player->CastSpell(player, Spells::TheKeystoneQuestCompletion, true);
        }

        void OnQuestAccept(Player* player, Quest const* quest) override
        {
            if (quest->GetQuestId() == Quests::ReturnToTheBlackTemple)
            {
                Talk(0, player);
                me->SummonGameObject(GameObjects::SargeriteKeystone, Positions::SargeriteKeystone,
                    QuaternionData(), 0s, GO_SUMMON_TIMED_OR_CORPSE_DESPAWN, player->GetGUID());
            }
        }
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
    RegisterCreatureAI(npc_king_voras);
    RegisterCreatureAI(npc_illidari_banner_trigger);
    RegisterCreatureAI(npc_doom_fortress_stabilizer);
    RegisterCreatureAI(npc_overseer_brutarg);
    RegisterCreatureAI(npc_count_nefarious);
    RegisterCreatureAI(npc_wounded_demon_hunter);
    RegisterCreatureAI(npc_well_of_souls);
    RegisterCreatureAI(npc_general_volroth);
    RegisterCreatureAI(npc_korvas_bloodthorn_foul_creche);
    RegisterCreatureAI(npc_prolifica);
    RegisterCreatureAI(npc_lady_stheno);
    RegisterCreatureAI(npc_cyana_nightglaive_fel_hammer);
    RegisterCreatureAI(npc_brood_queen_tyranna);
    RegisterCreatureAI(npc_keystone_downstairs_kill_credit);
    RegisterCreatureAI(npc_kayn_sunfury_keystone);
}
