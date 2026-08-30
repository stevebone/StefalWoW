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
#include "SpellAuras.h"
#include "ScriptMgr.h"
#include "TemporarySummon.h"

#include <queue>

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

    /*######
    ## 43300 Spawned Messner
    ######*/

    struct npc_spawned_messner : public ScriptedAI
    {
        npc_spawned_messner(Creature* creature) : ScriptedAI(creature) { }

        Player* GetPlayerOwner() const
        {
            if (me->IsSummon())
                if (Unit* owner = me->ToTempSummon()->GetOwner())
                    return owner->ToPlayer();
            return nullptr;
        }

        void Reset() override
        {
            _events.Reset();
            _talkSequenceStep = 0;
            _events.ScheduleEvent(Events::SpawnedMessnerRandomTalk, 40s, 60s);
            _events.ScheduleEvent(Events::SpawnedMessnerMoltenArmor, 10min);

            if (!me->HasAura(Spells::MoltenArmor))
                DoCastSelf(Spells::MoltenArmor);

            if (me->IsSummon())
            {
                if (Unit* owner = me->ToTempSummon()->GetOwner())
                {
                    float angle = frand(0.0f, float(M_PI * 2));
                    float dist = frand(1.0f, 3.0f);
                    me->GetMotionMaster()->Clear();
                    me->GetMotionMaster()->MoveFollow(owner, dist, ChaseAngle(angle));
                }
            }
        }

        void JustEngagedWith(Unit* /*who*/) override
        {
            _events.ScheduleEvent(Events::SpawnedMessnerCombatSpell, 5s);

            if (roll_chance(20))
                Talk(0);
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
                    case Events::SpawnedMessnerRandomTalk:
                    {
                        if (roll_chance(30))
                        {
                            uint8 talkRoll = urand(3, 13);
                            Talk(talkRoll, GetPlayerOwner());
                            if (talkRoll == 3)
                            {
                                _talkSequenceStep = 0;
                                _events.ScheduleEvent(Events::SpawnedMessnerTalkSequence, 2s, 3s);
                            }
                        }
                        _events.ScheduleEvent(Events::SpawnedMessnerRandomTalk, 40s, 60s);
                        break;
                    }
                    case Events::SpawnedMessnerTalkSequence:
                    {
                        if (_talkSequenceStep == 0)
                        {
                            Talk(1, GetPlayerOwner());
                            _talkSequenceStep = 1;
                            _events.ScheduleEvent(Events::SpawnedMessnerTalkSequence, 2s, 3s);
                        }
                        else if (_talkSequenceStep == 1)
                        {
                            Talk(2, GetPlayerOwner());
                            _talkSequenceStep = 0;
                        }
                        break;
                    }
                    case Events::SpawnedMessnerMoltenArmor:
                        if (!me->HasAura(Spells::MoltenArmor))
                            DoCastSelf(Spells::MoltenArmor);
                        _events.ScheduleEvent(Events::SpawnedMessnerMoltenArmor, 10min);
                        break;
                    case Events::SpawnedMessnerCombatSpell:
                    {
                        uint32 spell = RAND(Spells::BlastWave, Spells::FireBlast, Spells::Fireball, Spells::Flamestrike);
                        DoCastVictim(spell);
                        _events.ScheduleEvent(Events::SpawnedMessnerCombatSpell, 5s);
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
        uint8 _talkSequenceStep = 0;
    };

    /*######
    ## 43272 Jorgensen
    ######*/

    struct npc_jorgensen : public ScriptedAI
    {
        npc_jorgensen(Creature* creature) : ScriptedAI(creature) { }

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

            if (!player->IsActiveQuest(Quests::Jorgensen))
                return;

            if (me->GetDistance(who) > 5.0f)
                return;

            if (_talkCooldown)
                return;

            Talk(Talks::JorgensenSay00, player);

            _talkCooldown = true;
            _talkPlayerGuid = player->GetGUID();
            _events.ScheduleEvent(Events::JorgensenTalk1, 3s);
            _events.ScheduleEvent(Events::JorgensenClearTalkCooldown, 2min);
        }

        void OnQuestReward(Player* /*player*/, Quest const* quest, LootItemType /*type*/, uint32 /*opt*/) override
        {
            if (quest->GetQuestId() == Quests::Jorgensen)
            {
                if (GameObject* cage = me->FindNearestGameObject(GameObjects::JorgensensCage, 10.0f))
                    cage->SetGoState(GO_STATE_ACTIVE);
            }
        }

        void UpdateAI(uint32 diff) override
        {
            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case Events::JorgensenTalk1:
                    {
                        Player* player = ObjectAccessor::GetPlayer(*me, _talkPlayerGuid);
                        if (player)
                            Talk(Talks::JorgensenSay01, player);
                        break;
                    }
                    case Events::JorgensenClearTalkCooldown:
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
    ## 43305 Spawned Jorgensen
    ######*/

    struct npc_spawned_jorgensen : public ScriptedAI
    {
        npc_spawned_jorgensen(Creature* creature) : ScriptedAI(creature) { }

        Player* GetPlayerOwner() const
        {
            if (me->IsSummon())
                if (Unit* owner = me->ToTempSummon()->GetOwner())
                    return owner->ToPlayer();
            return nullptr;
        }

        void Reset() override
        {
            _events.Reset();
            _events.ScheduleEvent(Events::SpawnedJorgensenRandomTalk, 40s, 60s);
            _events.ScheduleEvent(Events::SpawnedJorgensenSealOfRighteousness, 10min);

            DoCastSelf(Spells::ConcentrationAura, CastSpellExtraArgs(TRIGGERED_FULL_MASK));

            if (!me->HasAura(Spells::SealOfRighteousness))
                DoCastSelf(Spells::SealOfRighteousness, CastSpellExtraArgs(TRIGGERED_FULL_MASK));

            if (me->IsSummon())
            {
                if (Unit* owner = me->ToTempSummon()->GetOwner())
                {
                    float angle = frand(0.0f, float(M_PI * 2));
                    float dist = frand(1.0f, 3.0f);
                    me->GetMotionMaster()->Clear();
                    me->GetMotionMaster()->MoveFollow(owner, dist, ChaseAngle(angle));
                }
            }
        }

        void JustEngagedWith(Unit* /*who*/) override
        {
            _events.ScheduleEvent(Events::SpawnedJorgensenCombatSpell, 5s);
            _events.ScheduleEvent(Events::SpawnedJorgensenHeal, 5s);

            if (roll_chance(20))
                Talk(0);
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
                    case Events::SpawnedJorgensenRandomTalk:
                    {
                        if (roll_chance(30))
                        {
                            if (me->FindNearestCreature(Creatures::Danforth, 20.0f, true))
                                Talk(urand(1, 12), GetPlayerOwner());
                            else
                                Talk(urand(2, 12), GetPlayerOwner());
                        }
                        _events.ScheduleEvent(Events::SpawnedJorgensenRandomTalk, 40s, 60s);
                        break;
                    }
                    case Events::SpawnedJorgensenSealOfRighteousness:
                        if (!me->HasAura(Spells::SealOfRighteousness))
                            DoCastSelf(Spells::SealOfRighteousness, CastSpellExtraArgs(TRIGGERED_FULL_MASK));
                        _events.ScheduleEvent(Events::SpawnedJorgensenSealOfRighteousness, 10min);
                        break;
                    case Events::SpawnedJorgensenCombatSpell:
                    {
                        uint32 spell = RAND(Spells::Exorcism, Spells::HolyShock);
                        DoCastVictim(spell);
                        _events.ScheduleEvent(Events::SpawnedJorgensenCombatSpell, 5s);
                        break;
                    }
                    case Events::SpawnedJorgensenHeal:
                        if (me->HealthBelowPct(50))
                            DoCastSelf(Spells::HolyLight);
                        _events.ScheduleEvent(Events::SpawnedJorgensenHeal, 5s);
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

    /*######
    ## 43303 Spawned Krakauer
    ######*/

    struct npc_spawned_krakauer : public ScriptedAI
    {
        npc_spawned_krakauer(Creature* creature) : ScriptedAI(creature) { }

        Player* GetPlayerOwner() const
        {
            if (me->IsSummon())
                if (Unit* owner = me->ToTempSummon()->GetOwner())
                    return owner->ToPlayer();
            return nullptr;
        }

        void Reset() override
        {
            _events.Reset();
            _events.ScheduleEvent(Events::SpawnedKrakauerRandomTalk, 40s, 60s);
            _events.ScheduleEvent(Events::SpawnedKrakauerBattleShout, 2min);

            if (!me->HasAura(Spells::BattleShout))
                DoCastSelf(Spells::BattleShout);

            if (me->IsSummon())
            {
                if (Unit* owner = me->ToTempSummon()->GetOwner())
                {
                    float angle = frand(0.0f, float(M_PI * 2));
                    float dist = frand(1.0f, 3.0f);
                    me->GetMotionMaster()->Clear();
                    me->GetMotionMaster()->MoveFollow(owner, dist, ChaseAngle(angle));
                }
            }
        }

        void JustEngagedWith(Unit* /*who*/) override
        {
            _events.ScheduleEvent(Events::SpawnedKrakauerCombatSpell, 5s);

            if (roll_chance(20))
                Talk(0);
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
                    case Events::SpawnedKrakauerRandomTalk:
                        if (roll_chance(30))
                            Talk(1, GetPlayerOwner());
                        _events.ScheduleEvent(Events::SpawnedKrakauerRandomTalk, 40s, 60s);
                        break;
                    case Events::SpawnedKrakauerBattleShout:
                        if (!me->HasAura(Spells::BattleShout))
                            DoCastSelf(Spells::BattleShout);
                        _events.ScheduleEvent(Events::SpawnedKrakauerBattleShout, 2min);
                        break;
                    case Events::SpawnedKrakauerCombatSpell:
                        DoCastVictim(Spells::Slam);
                        _events.ScheduleEvent(Events::SpawnedKrakauerCombatSpell, 5s);
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

    /*######
    ## 43275 Danforth Captured
    ######*/

    struct npc_danforth_captured : public ScriptedAI
    {
        npc_danforth_captured(Creature* creature) : ScriptedAI(creature)
        {
            _events.ScheduleEvent(Events::DanforthInitChains, 500ms);
        }

        void SetData(uint32 id, uint32 value) override
        {
            if (id == 1 && value == 1)
                _events.ScheduleEvent(Events::DanforthRelease, 1s);
        }

        void OnQuestReward(Player* /*player*/, Quest const* quest, LootItemType /*type*/, uint32 /*opt*/) override
        {
            if (quest->GetQuestId() == Quests::AndLastButNotLeastDanforth)
                me->DespawnOrUnsummon(2s);
        }

        void UpdateAI(uint32 diff) override
        {
            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case Events::DanforthInitChains:
                        me->SetEmoteState(Emote(420));
                        me->SetDisableGravity(true);

                        if (Creature* chainDummyRight = me->GetMap()->GetCreatureBySpawnId(Spawns::DanforthChainDummyRight))
                        {
                            // Stupid core - the dummies require the detect invis aura to be able to cast chain spell
                            chainDummyRight->AddAura(Spells::DetectInvis8, chainDummyRight);
                            chainDummyRight->CastSpell(me, Spells::CosmeticChainsRightHand, true);
                        }

                        if (Creature* chainDummyLeft = me->GetMap()->GetCreatureBySpawnId(Spawns::DanforthChainDummyLeft))
                        {
                            // Stupid core - the dummies require the detect invis aura to be able to cast chain spell
                            chainDummyLeft->AddAura(Spells::DetectInvis8, chainDummyLeft);
                            chainDummyLeft->CastSpell(me, Spells::CosmeticChainsLeftHand, true);
                        }
                        break;
                    case Events::DanforthRelease:
                        me->RemoveAurasDueToSpell(Spells::CosmeticChainsRightHand);
                        me->RemoveAurasDueToSpell(Spells::CosmeticChainsLeftHand);
                        me->SetDisableGravity(false);
                        me->GetMotionMaster()->MoveFall();
                        me->m_Events.AddEventAtOffset([this]()
                            {
                                me->SetEmoteState(EMOTE_STATE_SWIM_IDLE);
                            }, 1s);
                        break;
                    default:
                        break;
                }
            }
        }

    private:
        EventMap _events;
    };

    /*######
    ## 43302 Spawned Danforth
    ######*/

    struct npc_spawned_danforth : public ScriptedAI
    {
        npc_spawned_danforth(Creature* creature) : ScriptedAI(creature) { }

        Player* GetPlayerOwner() const
        {
            if (me->IsSummon())
                if (Unit* owner = me->ToTempSummon()->GetOwner())
                    return owner->ToPlayer();
            return nullptr;
        }

        void Reset() override
        {
            _events.Reset();
            _events.ScheduleEvent(Events::SpawnedDanforthRandomTalk, 40s, 60s);
            _events.ScheduleEvent(Events::SpawnedDanforthCommandingShout, 2min);

            if (!me->HasAura(Spells::CommandingShout))
                DoCastSelf(Spells::CommandingShout);

            if (me->IsSummon())
            {
                if (Unit* owner = me->ToTempSummon()->GetOwner())
                {
                    float angle = frand(0.0f, float(M_PI * 2));
                    float dist = frand(1.0f, 3.0f);
                    me->GetMotionMaster()->Clear();
                    me->GetMotionMaster()->MoveFollow(owner, dist, ChaseAngle(angle));
                }
            }
        }

        void JustEngagedWith(Unit* who) override
        {
            _events.ScheduleEvent(Events::SpawnedDanforthCombatSpell, 5s);

            if (roll_chance(20))
                Talk(0);

            DoCast(who, Spells::Charge);
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
                    case Events::SpawnedDanforthRandomTalk:
                        if (roll_chance(30))
                            Talk(1, GetPlayerOwner());
                        _events.ScheduleEvent(Events::SpawnedDanforthRandomTalk, 40s, 60s);
                        break;
                    case Events::SpawnedDanforthCommandingShout:
                        if (!me->HasAura(Spells::CommandingShout))
                            DoCastSelf(Spells::CommandingShout);
                        _events.ScheduleEvent(Events::SpawnedDanforthCommandingShout, 2min);
                        break;
                    case Events::SpawnedDanforthCombatSpell:
                        if (roll_chance(50))
                            DoCastVictim(Spells::Shockwave);
                        else
                            DoCastVictim(Spells::Thunderclap);
                        _events.ScheduleEvent(Events::SpawnedDanforthCombatSpell, 5s);
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

void AddSC_custom_redridge_mountains_npcs()
{
    using namespace Scripts::EasternKingdoms::RedridgeMountains;

    RegisterCreatureAI(npc_guard_bateman);
    RegisterCreatureAI(npc_messner);
    RegisterCreatureAI(npc_blackrock_battle_worg);
    RegisterCreatureAI(npc_spawned_messner);
    RegisterCreatureAI(npc_jorgensen);
    RegisterCreatureAI(npc_spawned_jorgensen);
    RegisterCreatureAI(npc_spawned_krakauer);
    RegisterCreatureAI(npc_danforth_captured);
    RegisterCreatureAI(npc_spawned_danforth);
}
