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
#include "CreatureAIImpl.h"
#include "EventMap.h"
#include "GameObject.h"
#include "ObjectAccessor.h"
#include "ObjectGuid.h"
#include "PhasingHandler.h"
#include "Player.h"
#include "ScriptedCreature.h"
#include "Unit.h"
#include "Vehicle.h"
#include "SpellAuras.h"
#include "ScriptMgr.h"
#include "TemporarySummon.h"
#include "MotionMaster.h"
#include "WaypointDefines.h"
#include "Map.h"
#include "Vehicle.h"
#include "ScriptActions.h"
#include "CellImpl.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"

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
    ## 43827 Jorgensen (spawned at camp)
    ######*/

    struct npc_jorgensen_camp_guardian : public ScriptedAI
    {
        npc_jorgensen_camp_guardian(Creature* creature) : ScriptedAI(creature)
        {
            _talk0Done = false;
            _talk1Done = false;
            _talk2Done = false;
            _talk4Done = false;
            _talk5Done = false;
            _talk6Done = false;
            _talk9Done = false;
            _talk10Done = false;
            _talk11Done = false;

            me->SetReactState(REACT_ASSIST);
        }

        Player* GetPlayerOwner() const
        {
            if (me->IsSummon())
                if (Unit* owner = me->ToTempSummon()->GetOwner())
                    return owner->ToPlayer();
            return nullptr;
        }

        void Reset() override
        {
            if (me->GetSpawnId() != 0)
                return;

            _events.Reset();

            _events.ScheduleEvent(Events::JorgensenGuardianRandomTalk, 40s, 60s);
            _events.ScheduleEvent(Events::JorgensenGuardianSealOfRighteousness, 10min);

            DoCastSelf(Spells::ConcentrationAura, CastSpellExtraArgs(TRIGGERED_FULL_MASK));

            if (!me->HasAura(Spells::SealOfRighteousness))
                DoCastSelf(Spells::SealOfRighteousness, CastSpellExtraArgs(TRIGGERED_FULL_MASK));
        }

        void SetData(uint32 id, uint32 value) override
        {
            if (id != 1)
                return;

            switch (value)
            {
                case 0:
                    if (!_talk0Done)
                    {
                        Talk(0, GetPlayerOwner());
                        _talk0Done = true;
                    }
                    break;
                case 1:
                    if (!_talk1Done)
                    {
                        Talk(1, GetPlayerOwner());
                        _talk1Done = true;
                    }
                    break;
                case 2:
                    if (!_talk2Done)
                    {
                        Talk(2, GetPlayerOwner());
                        _events.ScheduleEvent(Events::JorgensenGuardianTalk2Followup, 3s);
                        _talk2Done = true;
                    }
                    break;
                case 4:
                    if (!_talk4Done)
                    {
                        Talk(4, GetPlayerOwner());
                        _events.ScheduleEvent(Events::JorgensenGuardianTalk4Followup, 3s);
                        _talk4Done = true;
                    }
                    break;
                case 6:
                    if (!_talk6Done)
                    {
                        Talk(6, GetPlayerOwner());
                        _talk6Done = true;
                    }
                    break;
                case 9:
                    if (!_talk9Done)
                    {
                        Talk(9, GetPlayerOwner());
                        _talk9Done = true;
                    }
                    break;
                case 10:
                    if (!_talk10Done)
                    {
                        Talk(10, GetPlayerOwner());
                        _talk10Done = true;
                    }
                    break;
                case 11:
                    if (!_talk11Done)
                    {
                        Talk(11, GetPlayerOwner());
                        _talk11Done = true;
                    }
                    break;
                default:
                    break;
            }
        }

        void OnAuraApplied(AuraApplication const* aurApp) override
        {
            if (aurApp->GetBase()->GetId() == Spells::Camouflage && !_talk5Done)
            {
                Talk(5, GetPlayerOwner());
                _talk5Done = true;
            }
        }

        void JustEngagedWith(Unit* /*who*/) override
        {
            _events.ScheduleEvent(Events::JorgensenGuardianCombatSpell, 5s);
            _events.ScheduleEvent(Events::JorgensenGuardianHeal, 5s);
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
                    case Events::JorgensenGuardianTalk2Followup:
                        Talk(3, GetPlayerOwner());
                        break;
                    case Events::JorgensenGuardianTalk4Followup:
                        Talk(8, GetPlayerOwner());
                        break;
                    case Events::JorgensenGuardianRandomTalk:
                        if (roll_chance(30))
                            Talk(7, GetPlayerOwner());
                        _events.ScheduleEvent(Events::JorgensenGuardianRandomTalk, 40s, 60s);
                        break;
                    case Events::JorgensenGuardianSealOfRighteousness:
                        if (!me->HasAura(Spells::SealOfRighteousness))
                            DoCastSelf(Spells::SealOfRighteousness, CastSpellExtraArgs(TRIGGERED_FULL_MASK));
                        _events.ScheduleEvent(Events::JorgensenGuardianSealOfRighteousness, 10min);
                        break;
                    case Events::JorgensenGuardianCombatSpell:
                        DoCastVictim(RAND(Spells::Exorcism, Spells::HolyShock));
                        _events.ScheduleEvent(Events::JorgensenGuardianCombatSpell, 5s);
                        break;
                    case Events::JorgensenGuardianHeal:
                        if (me->HealthBelowPct(50))
                            DoCastSelf(Spells::HolyLight);
                        else if (Player* owner = GetPlayerOwner())
                            if (owner->IsAlive() && owner->HealthBelowPct(50))
                                DoCast(owner, Spells::HolyLight);
                        _events.ScheduleEvent(Events::JorgensenGuardianHeal, 5s);
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
        bool _talk0Done = false;
        bool _talk1Done = false;
        bool _talk2Done = false;
        bool _talk4Done = false;
        bool _talk5Done = false;
        bool _talk6Done = false;
        bool _talk9Done = false;
        bool _talk10Done = false;
        bool _talk11Done = false;
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

    /*######
    ## 43450 Keeshan Riverboat
    ######*/

    WaypointPath const BoatPath(0, {
        { 0, -9356.31f, -2414.29f, 55.5f },
        { 1, -9467.38f, -2563.55f, 55.5f },
        { 2, -9395.82f, -2813.74f, 55.5f },
        { 3, -9425.49f, -2836.49f, 56.1617f },
    });

    struct npc_keeshan_riverboat : public ScriptedAI
    {
        npc_keeshan_riverboat(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            me->SetOrientation(3.818652f);
            _events.Reset();
            _playerGuid.Clear();
        }

        void PassengerBoarded(Unit* who, int8 /*seatId*/, bool apply) override
        {
            if (!apply || !who->IsPlayer())
                return;

            _playerGuid = who->GetGUID();
            _events.ScheduleEvent(Events::RiverboatKeeshanTalk, 1s);
            _events.ScheduleEvent(Events::RiverboatMessnerTalk, 3s);
            _events.ScheduleEvent(Events::RiverboatMessnerEngine, 4s);
            _events.ScheduleEvent(Events::RiverboatStartPath, 5s);
        }

        void UpdateAI(uint32 diff) override
        {
            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case Events::RiverboatKeeshanTalk:
                        if (Player* player = ObjectAccessor::GetPlayer(*me, _playerGuid))
                            if (Unit* keeshan = me->GetVehicleKit()->GetPassenger(2))
                                if (keeshan->IsCreature())
                                    keeshan->ToCreature()->AI()->Talk(0, player);
                        break;
                    case Events::RiverboatMessnerTalk:
                        if (Player* player = ObjectAccessor::GetPlayer(*me, _playerGuid))
                            if (Unit* messner = me->GetVehicleKit()->GetPassenger(1))
                                if (messner->IsCreature())
                                    messner->ToCreature()->AI()->Talk(0, player);
                        break;
                    case Events::RiverboatMessnerEngine:
                        if (Unit* messner = me->GetVehicleKit()->GetPassenger(1))
                            messner->CastSpell(me, Spells::MessnerBoatEngine, true);
                        break;
                    case Events::RiverboatStartPath:
                        me->GetMotionMaster()->MovePath(BoatPath, false, {}, 13.0f);
                        break;
                    default:
                        break;
                }
            }
        }

        void WaypointPathEnded(uint32 /*nodeId*/, uint32 /*pathId*/) override
        {
            if (Unit* messner = me->GetVehicleKit()->GetPassenger(1))
                messner->RemoveAurasDueToSpell(Spells::MessnerBoatEngine);

            if (Player* player = ObjectAccessor::GetPlayer(*me, _playerGuid))
            {
                if (Unit* keeshan = me->GetVehicleKit()->GetPassenger(2))
                    if (keeshan->IsCreature())
                        keeshan->ToCreature()->AI()->Talk(1, player);

                // Spell for quest credit requires player to be in seat 5?????
                // me->CastSpell(player, Spells::RiverboatQuestCredit);
                player->CompleteQuest(Quests::ItsNeverOver);
            }

            me->m_Events.AddEventAtOffset([this]()
                {
                    me->DespawnOrUnsummon(1s);
                }, 1s);
        }

    private:
        EventMap _events;
        ObjectGuid _playerGuid;
    };

    /*######
    ## 43518 Wild Rat
    ######*/

    class WildRatKneelEvent : public BasicEvent
    {
    public:
        WildRatKneelEvent(Creature* creature, std::shared_ptr<Scripting::v2::ActionResult<MovementStopReason>> const& action)
            : _creature(creature), _action(action) { }

        bool Execute(uint64 /*time*/, uint32 /*diff*/) override
        {
            if (!_action->IsReady())
            {
                _creature->m_Events.AddEventAtOffset(this, 500ms);
                return false;
            }

            _creature->StopMoving();
            _creature->GetMotionMaster()->Clear();
            _creature->AddUnitState(UNIT_STATE_ROOT);
            _creature->HandleEmoteCommand(EMOTE_ONESHOT_KNEEL);
            _creature->m_Events.AddEventAtOffset([creature = _creature]()
                {
                    if (creature && creature->IsAlive())
                    {
                        creature->ClearUnitState(UNIT_STATE_ROOT);
                        creature->GetMotionMaster()->MoveTargetedHome();
                    }
                }, 15s);

            return true;
        }

    private:
        Creature* _creature;
        std::shared_ptr<Scripting::v2::ActionResult<MovementStopReason>> _action;
    };

    struct npc_wild_rat : public ScriptedAI
    {
        npc_wild_rat(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            DoCastSelf(Spells::DistractionVisual, CastSpellExtraArgs(TRIGGERED_FULL_MASK));

            static constexpr uint32 blackrockEntries[] =
            {
                Creatures::BlackrockHunter,
                Creatures::BlackrockSummoner,
                Creatures::BlackrockGuard,
                Creatures::BlackrockDrakeRider,
                Creatures::BlackrockWarden
            };

            for (uint32 entry : blackrockEntries)
            {
                std::list<Creature*> creatureList;
                me->GetCreatureListWithEntryInGrid(creatureList, entry, 30.0f);
                for (Creature* creature : creatureList)
                {
                    if (!creature->IsAlive())
                        continue;

                    if (!me->IsWithinLOSInMap(creature))
                        continue;

                    Position dest = me->GetNearPosition(1.0f, me->GetAbsoluteAngle(creature));
                    std::shared_ptr<Scripting::v2::ActionResult<MovementStopReason>> action =
                        std::make_shared<Scripting::v2::ActionResult<MovementStopReason>>();
                    Scripting::v2::ActionResultSetter<MovementStopReason> actionResultSetter =
                        Scripting::v2::ActionResult<MovementStopReason>::GetResultSetter(action);
                    creature->GetMotionMaster()->MovePoint(1, dest, true, Optional<float>{}, 15.0f, MovementWalkRunSpeedSelectionMode::Default, Optional<float>{}, Optional<MovementFadeObject>{}, std::move(actionResultSetter));
                    creature->m_Events.AddEventAtOffset(new WildRatKneelEvent(creature, action), 500ms);
                }
            }
        }

    };

    /*######
    ## 43572 / 43571 Kidnapped Redridge Citizen
    ######*/

    struct npc_kidnapped_redridge_citizen : public ScriptedAI
    {
        npc_kidnapped_redridge_citizen(Creature* creature) : ScriptedAI(creature) { }

        void SetData(uint32 id, uint32 /*value*/) override
        {
            if (id == 1)
            {
                me->SetWalk(false);
                me->GetMotionMaster()->MovePoint(1, Positions::CitizenFleePoint1);
            }
            else if (id == 2)
                Talk(0);
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type != POINT_MOTION_TYPE)
                return;

            switch (id)
            {
                case 1:
                    me->GetMotionMaster()->MovePoint(2, Positions::CitizenFleePoint2);
                    break;
                case 2:
                    me->DespawnOrUnsummon(1s);
                    break;
                default:
                    break;
            }
        }
    };

    /*######
    ## 43611 Keeshan (Canyon)
    ######*/

    struct npc_keeshan_canyon : public ScriptedAI
    {
        npc_keeshan_canyon(Creature* creature) : ScriptedAI(creature) { }

        void OnQuestAccept(Player* player, Quest const* quest) override
        {
            if (quest->GetQuestId() != Quests::Detonation)
                return;

            // Temporary hack to complete quest since scene is not yet implemented
            player->CastSpell(player, Spells::TeleportToShalewindCanyon, true);
            PhasingHandler::OnConditionChange(player, true);
        }

        void OnQuestReward(Player* player, Quest const* quest, LootItemType /*type*/, uint32 /*opt*/) override
        {
            if (quest->GetQuestId() == Quests::Detonation)
            {

                if (Creature* messner = me->FindNearestCreature(Creatures::MessnerCanyon, 10.0f))
                    messner->AI()->Talk(Talks::MessnerCanyonSay00, player);

                me->m_Events.AddEventAtOffset([this, player]()
                    {
                        if (me && player)
                            Talk(Talks::KeeshanCanyonSay00, player);
                    }, 5s);
            }

            if (quest->GetQuestId() == Quests::TheGrandMagusDoane)
            {
                Talk(Talks::KeeshanCanyonSay01, player);

                player->SummonCreature(Creatures::BravoCompanySiegeTank, Positions::SiegeTankSpawn);

                if (Creature* danforth = me->FindNearestCreature(Creatures::DanforthCanyon, 10.0f))
                {
                    me->m_Events.AddEventAtOffset([danforth, player]()
                        {
                            if(danforth && player)
                                danforth->AI()->Talk(Talks::DanforthCanyonSay00, player);
                        }, 3s);

                    me->m_Events.AddEventAtOffset([danforth, player]()
                        {
                            if (danforth && player)
                            {
                                danforth->AI()->Talk(Talks::DanforthCanyonSay01, player);
                                if (Creature* troteman = player->FindNearestCreature(Creatures::ColonelTrotemanSiegeTank, 20.0f))
                                    troteman->AI()->SetData(1, 0);
                            }
                        }, 11s);
                }

                me->m_Events.AddEventAtOffset([this, player]()
                    {
                        if (me && player)
                            Talk(Talks::KeeshanCanyonSay02, player);
                    }, 8s);

            }
        }

    private:
        EventMap _events;
    };

    /*######
    ## 397 Grand Magus Doane
    ######*/

    struct npc_grand_magus_doane : public ScriptedAI
    {
        npc_grand_magus_doane(Creature* creature) : ScriptedAI(creature) { }

        void Reset() override
        {
            _events.Reset();
            _summonedMinion = false;
            _despawning = false;
            _minionGuid.Clear();

            if (!me->HasAura(Spells::DemonArmor))
                DoCastSelf(Spells::DemonArmor);

            _events.ScheduleEvent(Events::DoaneDemonArmorCheck, 10min);
        }

        void JustEngagedWith(Unit* who) override
        {
            if (Player* player = who->ToPlayer())
            {
                _playerGuid = player->GetGUID();
                Talk(Talks::DoaneSay00, player);
            }

            _events.ScheduleEvent(Events::DoaneFireballCombat, 3s, 5s);
        }

        void JustSummoned(Creature* summon) override
        {
            if (summon->GetEntry() == Creatures::MinionOfDoane)
                _minionGuid = summon->GetGUID();
        }

        void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo*/) override
        {
            // Once the despawn sequence has started, Doane must not die before the 1s despawn fires.
            if (_despawning)
            {
                damage = 0;
                return;
            }

            // At <= 10% HP, freeze health and trigger the despawn sequence.
            // Setting damage = 0 prevents death without needing SetImmuneToAll/SetUnkillable,
            // which would otherwise break the Doane Credit spell's AoE enemy-target search.
            if (me->HealthBelowPctDamaged(10, damage))
            {
                _despawning = true;
                damage = 0;

                // Detach the minion so it survives Doane's despawn
                if (!_minionGuid.IsEmpty())
                {
                    if (Creature* minion = ObjectAccessor::GetCreature(*me, _minionGuid))
                    {
                        if (TempSummon* summon = minion->ToTempSummon())
                        {
                            if (summon->HasUnitTypeMask(UNIT_MASK_MINION))
                                me->SetMinion(static_cast<Minion*>(summon), false);
                        }
                    }
                }

                if (Player* player = ObjectAccessor::GetPlayer(*me, _playerGuid))
                {
                    Talk(Talks::DoaneSay02, player);
                    me->CastSpell(player, Spells::DoaneCredit, true);
                }

                me->CastSpell(me, Spells::TeleportVisualOnly, true);
                me->DespawnOrUnsummon(1s);
            }
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
                    case Events::DoaneDemonArmorCheck:
                        if (!me->HasAura(Spells::DemonArmor))
                            DoCastSelf(Spells::DemonArmor);
                        _events.ScheduleEvent(Events::DoaneDemonArmorCheck, 10min);
                        break;
                    case Events::DoaneFireballCombat:
                        DoCastVictim(Spells::DoaneFireball);
                        _events.ScheduleEvent(Events::DoaneFireballCombat, 3s, 5s);
                        break;
                    default:
                        break;
                }
            }

            if (_despawning)
                return;

            if (!_summonedMinion && me->HealthBelowPct(60))
            {
                _summonedMinion = true;
                if (Player* player = ObjectAccessor::GetPlayer(*me, _playerGuid))
                    Talk(Talks::DoaneSay01, player);
                DoCastSelf(Spells::MinionOfDoane);
            }

            if (!UpdateVictim())
                return;

            me->DoMeleeAttackIfReady();
        }

    private:
        EventMap _events;
        ObjectGuid _playerGuid;
        ObjectGuid _minionGuid;
        bool _summonedMinion = false;
        bool _despawning = false;
    };

    /*######
    ## 43714 Bravo Company Siege Tank
    ######*/

    struct npc_bravo_company_siege_tank : public ScriptedAI
    {
        npc_bravo_company_siege_tank(Creature* creature) : ScriptedAI(creature)
        {
            // Spawn in air with parachute, then fall to the ground.
            me->CastSpell(me, Spells::ParachuteVisual, true);
            me->CastSpell(me, Spells::Parachute, true);
            me->GetMotionMaster()->MoveFall(MovementPoints::SiegeTankFall);
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type == EFFECT_MOTION_TYPE && id == MovementPoints::SiegeTankFall)
                HandleLanding();
        }

        void UpdateAI(uint32 diff) override
        {
            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case Events::SiegeTankEnableSpellClick:
                        me->SetNpcFlag(UNIT_NPC_FLAG_SPELLCLICK);
                        break;
                    default:
                        break;
                }
            }
        }

    private:
        void HandleLanding()
        {
            if (_landingHandled)
                return;
            _landingHandled = true;

            // Landed: remove parachute, eject passenger, schedule spellclick flag.
            me->RemoveAurasDueToSpell(Spells::Parachute);
            me->HandleEmoteCommand(EMOTE_ONESHOT_ATTACK_THROWN);

            if (Vehicle* vehicle = me->GetVehicleKit())
            {
                if (Unit* passenger = vehicle->GetPassenger(1))
                    passenger->ExitVehicle(&Positions::SiegeTankEjectPos);
            }

            _events.ScheduleEvent(Events::SiegeTankEnableSpellClick, 1s);
        }

        EventMap _events;
        bool _wasFalling = false;
        bool _landingHandled = false;
    };

    /*######
    ## 43728 Colonel Troteman (Siege Tank)
    ######*/

    struct npc_colonel_troteman_siege_tank : public ScriptedAI
    {
        npc_colonel_troteman_siege_tank(Creature* creature) : ScriptedAI(creature) { }

        void SetData(uint32 id, uint32 /*value*/) override
        {
            if (id == 1)
            {
                Talk(Talks::ColonelTrotemanSiegeTankSay00);
                _events.ScheduleEvent(Events::ColonelTrotemanUpdateEntry, 5s);
            }
        }

        void UpdateAI(uint32 diff) override
        {
            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                if (eventId == Events::ColonelTrotemanUpdateEntry)
                    me->UpdateEntry(Creatures::ColonelTroteman);
            }

            if (!UpdateVictim())
                return;

            me->DoMeleeAttackIfReady();
        }

    private:
        EventMap _events;
    };

    /*######
    ## 43745 Siege Tank Gun (Keeshan's Gun)
    ## Rides the Bravo Company Siege Tank (43734) in seat 1.
    ## Keeshan (43744) rides the gun in seat 0. The player drives the tank in seat 0.
    ######*/

    struct npc_siege_tank_gun : public ScriptedAI
    {
        npc_siege_tank_gun(Creature* creature) : ScriptedAI(creature)
        {
            _machineGunOnCooldown = false;
            _events.ScheduleEvent(Events::KeeshanGunScanInvaders, 1s);
        }

        void KilledUnit(Unit* victim) override
        {
            if (victim->GetEntry() != Creatures::BlackrockInvader && victim->GetEntry() != Creatures::BlackrockDrake)
                return;

            if (Player* player = GetPlayerDriver())
                player->CastSpell(player, Spells::KillCreditBlackrockInvader, true);
        }

        void UpdateAI(uint32 diff) override
        {
            _events.Update(diff);

            while (uint32 eventId = _events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case Events::KeeshanGunScanInvaders:
                        ScanForInvaders();
                        _events.ScheduleEvent(Events::KeeshanGunScanInvaders, 1s);
                        break;
                    case Events::KeeshanGunMachineGunCooldown:
                        _machineGunOnCooldown = false;
                        break;
                    default:
                        break;
                }
            }
        }

    private:
        // The player drives the base vehicle (siege tank) in seat 0; the gun rides it in seat 1.
        Player* GetPlayerDriver() const
        {
            if (Unit* vehicle = me->GetVehicleCreatureBase())
                if (Vehicle* tank = vehicle->GetVehicleKit())
                    if (Unit* passenger = tank->GetPassenger(0))
                        return passenger->ToPlayer();
            return nullptr;
        }

        // Predicate: alive Blackrock invaders/drakes within range of the gun.
        struct BlackrockInvaderCheck
        {
            BlackrockInvaderCheck(WorldObject const* obj, float range) : _obj(obj), _range(range) { }

            bool operator()(Unit* u) const
            {
                if (!u->IsAlive())
                    return false;

                if (u->GetEntry() != Creatures::BlackrockInvader && u->GetEntry() != Creatures::BlackrockDrake)
                    return false;

                return _obj->IsWithinDist(u, _range);
            }

            WorldObject const* _obj;
            float _range;
        };

        void ScanForInvaders()
        {
            Unit* vehicle = me->GetVehicleCreatureBase();
            if (!vehicle)
                return;

            std::list<Unit*> targets;
            BlackrockInvaderCheck check(me, 60.0f);
            Trinity::UnitListSearcher<BlackrockInvaderCheck> searcher(me, targets, check);
            Cell::VisitAllObjects(me, searcher, 60.0f);

            if (targets.empty())
                return;

            bool firedThisTick = false;

            for (Unit* target : targets)
            {
                Creature* invader = target->ToCreature();
                if (!invader)
                    continue;

                invader->SetWalk(false);

                // Only (re)issue the attack order if it's not already chasing the tank,
                // otherwise you'll stomp its movement generator every second.
                if (!invader->IsEngaged() || invader->GetVictim() != vehicle)
                    invader->AI()->AttackStart(vehicle);

                if (!firedThisTick && !_machineGunOnCooldown && me->GetDistance(invader) <= 30.0f)
                {
                    me->SetFacingToObject(invader);

                    if (Vehicle* gun = me->GetVehicleKit())
                        if (Unit* keeshan = gun->GetPassenger(0))
                            if (Creature* keeshanCreature = keeshan->ToCreature())
                                keeshanCreature->AI()->Talk(Talks::KeeshanSiegeTankSay00, GetPlayerDriver());

                    me->CastSpell(invader, Spells::MachineGun, true);
                    _machineGunOnCooldown = true;
                    _events.ScheduleEvent(Events::KeeshanGunMachineGunCooldown, 10s);
                    firedThisTick = true; // one shot per cooldown cycle, keep looping to still AttackStart the rest
                }
            }
        }

        EventMap _events;
        bool _machineGunOnCooldown = false;
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
    RegisterCreatureAI(npc_jorgensen_camp_guardian);
    RegisterCreatureAI(npc_spawned_krakauer);
    RegisterCreatureAI(npc_danforth_captured);
    RegisterCreatureAI(npc_spawned_danforth);
    RegisterCreatureAI(npc_keeshan_riverboat);
    RegisterCreatureAI(npc_wild_rat);
    RegisterCreatureAI(npc_kidnapped_redridge_citizen);
    RegisterCreatureAI(npc_keeshan_canyon);
    RegisterCreatureAI(npc_grand_magus_doane);
    RegisterCreatureAI(npc_bravo_company_siege_tank);
    RegisterCreatureAI(npc_colonel_troteman_siege_tank);
    RegisterCreatureAI(npc_siege_tank_gun);
}
