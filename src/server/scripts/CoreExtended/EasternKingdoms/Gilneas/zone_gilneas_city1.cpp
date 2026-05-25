/*
 * Copyright (C) 2017-2018 AshamaneProject <https://github.com/AshamaneProject>
 * Copyright (C) 2011-2016 ArkCORE <http://www.arkania.net/>
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
 *
 * Attempted fix for RoleplayCore, KamiliaBlow
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
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Cell.h"
#include "CellImpl.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "CreatureAIImpl.h"
#include "GameObjectAI.h"
#include "GameObject.h"
#include "GridNotifiers.h"
#include "ObjectAccessor.h"
#include "MotionMaster.h"
#include "PassiveAI.h"
#include "Pet.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "ScriptedEscortAI.h"
#include "ScriptedFollowerAI.h"
#include "SpellScript.h"
#include "Unit.h"
#include "Vehicle.h"
#include "MoveSplineInit.h"
#include "MoveSpline.h"
#include "CharmInfo.h"
#include "Object.h"
#include "ObjectMgr.h"
#include "PhasingHandler.h"
#include "zone_gilneas.h"

enum eZoneGilneas
{
    NPC_LORD_GODFREY_35115                       = 35115,
    NPC_BLOODFANG_WORGEN_35118                   = 35118,
    NPC_WORGEN_ALPHA_C2                          = 35167,
    NPC_WORGEN_ALPHA_C1                          = 35170,
    NPC_WORGEN_RUNT_C1                           = 35188,
    NPC_BLOODFANG_STALKER                        = 35229,
    NPC_DARIUS_CROWLEY                           = 35230,
    NPC_CROWLEYS_HORSE                           = 35231,
    NPC_JOSIAH_AVERY_35370                       = 35370,
    NPC_LORNA_CROWLEY_35378                      = 35378,
    NPC_WORGEN_RUNT_C2                           = 35456,
    NPC_BLOODFANG_RIPPER_35505                   = 35505,
    NPC_PRINCE_GREYMANE                          = 35551,
    NPC_BLOODFANG_STALKER_CREDIT                 = 35582,
    NPC_RAMPAGING_WORGEN_35660                   = 35660,
    NPC_KRENNAN_ARANAS_TREE                      = 35753,
    NPC_GRAYMANE_HORSE_35905                     = 35905,
    NPC_KRENNAN_ARANAS                           = 35907,
    NPC_KING_GREYMANE                            = 35911,
    NPC_COMMANDEERED_CANNON                      = 35914,

    QUEST_THE_REBEL_LORDS_ARSENAL                = 14159,
    QUEST_FROM_THE_SHADOWS                       = 14204,
    QUEST_SACRIFICES                             = 14212,
    QUEST_LAST_STAND                             = 14222,
    QUEST_SAVE_KRENNAN_ARANAS                    = 14293,
    QUEST_TIME_TO_REGROUP                        = 14294,
    QUEST_ESCAPE_BY_BLOOD                        = 26129,

    SPELL_SHADOWSTALKER_STEALTH                  = 5916,
    SPELL_SHOOT                                  = 6660,
    SPELL_ENRAGE_8599                            = 8599,
    SPELL_BY_THE_SKIN                            = 66914,
    SPELL_THROW_TORCH                            = 67063,
    SPELL_GET_SHOT                               = 67349,
    SPELL_FORCE_CAST_SUMMON_JOSIAH               = 67352,
    SPELL_ATTACK_LURKER                          = 67805,
    SPELL_SUMMON_GILNEAN_MASTIFF                 = 67807,
    SPELL_FORCECAST_SUMMON_GRAYMANE_HORSE        = 68232,
    SPELL_CANNON_FIRE                            = 68235,
    SPELL_CURSE_OF_THE_WORGEN                    = 68630,
    SPELL_COSMETIC_COMBAT_ATTACK                 = 69873,
    SPELL_WORGEN_BITE                            = 72870,
    SPELL_INFECTED_BITE                          = 72872,
    SPELL_HIDEOUS_BITE_WOUND                     = 76642,
    SPELL_RIDE_BUNNY_SEAT2                       = 84275,
    SPELL_GILNEAS_CANNON_CAMERA                  = 93555,
    SPELL_ALTERED_FORM                           = 94293,
    SPELL_ALTERED_FORM2                          = 97709,
    SPELL_FORCE_WORGEN_ALTERED_FORM              = 98274,
    SPELL_SUMMON_CROWLEY_HORSE                   = 67001,
    SPELL_LAST_STAND_COMPLETE                    = 72799,

    SPELL_GENERIC_QUEST_INVISIBILITY_DETECTION_1 = 49416,
    SPELL_PHASE_QUEST_ZONE_SPECIFIC_02           = 59074, // phase: 171
    SPELL_PHASE_QUEST_ZONE_SPECIFIC_03           = 59087, // phase: 172
    SPELL_PHASE_QUEST_ZONE_SPECIFIC_06           = 68481, // phase: 181

};

//gilnean crow 50260
class npc_gilnean_crow : public CreatureScript
{
public:
    npc_gilnean_crow() : CreatureScript("npc_gilnean_crow") {}

    struct Coord
    {
        float x;
        float y;
        float z;
    };

    struct CrowFlyPosition
    {
        Coord FirstCoord;
        Coord SecondCoord;
    };

    struct npc_gilnean_crow50260AI : public ScriptedAI
    {
        npc_gilnean_crow50260AI(Creature* creature) : ScriptedAI(creature) {}

        uint8 pointId;
        bool flying = false;

        const CrowFlyPosition CrowFlyPos[12] =
        {
            {{-1407.20f, 1441.68f, 39.6586f}, {-1407.20f, 1441.68f, 67.7066f}},
            {{-1619.61f, 1310.61f, 27.7544f}, {-1619.61f, 1310.61f, 54.9702f}},
            {{-1799.56f, 1552.02f, 34.9408f}, {-1799.56f, 1552.02f, 38.4683f}},
            {{-1837.99f, 2289.09f, 50.2894f}, {-1837.99f, 2289.09f, 52.4776f}},
            {{-1970.88f, 2326.11f, 36.5107f}, {-1970.88f, 2326.11f, 38.8598f}},
            {{-1918.45f, 2406.86f, 37.4498f}, {-1918.45f, 2406.86f, 39.2891f}},
            {{-1845.37f, 2322.57f, 47.8401f}, {-1845.37f, 2322.57f, 50.0315f}},
            {{-1844.33f, 2492.53f, 6.67603f}, {-1844.33f, 2492.53f, 9.67311f}},
            {{-2035.53f, 2289.68f, 28.7353f}, {-2035.53f, 2289.68f, 32.0705f}},
            {{-2164.88f, 2222.65f, 27.4170f}, {-2164.88f, 2222.65f, 29.1592f}},
            {{-1781.95f, 2382.97f, 51.9086f}, {-1781.95f, 2382.97f, 55.8622f}},
            {{-1654.78f, 2503.14f, 109.893f}, {-1654.78f, 2503.14f, 115.819f}},
        };

        void InitializeAI()
        {
            FindFlyId();
        }

        void FindFlyId()
        {
            float dist = std::numeric_limits<float>::max();
            int i = 0;

            for (int j = 0; j < 12; ++j)
            {
                float _dist = me->GetDistance2d(CrowFlyPos[j].FirstCoord.x, CrowFlyPos[j].FirstCoord.y);

                if (dist > _dist)
                {
                    dist = _dist;
                    i = j;
                }
            }

            pointId = i;
        }

        void FlyAway()
        {
            flying = true;

            Movement::MoveSplineInit init(me);

            G3D::Vector3 vertice0(
                CrowFlyPos[pointId].FirstCoord.x + irand(-4, 4),
                CrowFlyPos[pointId].FirstCoord.y + irand(-4, 4),
                CrowFlyPos[pointId].FirstCoord.z + irand(-4, 4));
            init.Path().push_back(vertice0);

            G3D::Vector3 vertice1(
                CrowFlyPos[pointId].SecondCoord.x + irand(-4, 4),
                CrowFlyPos[pointId].SecondCoord.y + irand(-4, 4),
                CrowFlyPos[pointId].SecondCoord.z + irand(-4, 4));
            init.Path().push_back(vertice1);

            init.SetFly();
            init.SetSmooth();
            init.SetVelocity(7.5f);
            init.SetUncompressed();
            init.Launch();
            init.DisableTransportPathTransformations();
            me->DespawnOrUnsummon(10s);
        }

        void JustRespawned()
        {
            flying = false;
        }

        void MoveInLineOfSight(Unit* who)
        {
            if (!flying && who->GetTypeId() == TYPEID_PLAYER && me->IsWithinDistInMap(who, 15.0f))
                FlyAway();
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_gilnean_crow50260AI(creature);
    }

};

// 35840
class npc_gwen_armstead_35840 : public CreatureScript
{
public:
    npc_gwen_armstead_35840() : CreatureScript("npc_gwen_armstead_35840") {}

    enum eNpc
    {
        EVENT_CHECK_TALK = 101,
        EVENT_DO_NEXT_TALK,
        EVENT_DO_LAST_TALK,
    };

    struct npc_gwen_armstead_35840AI : public ScriptedAI
    {
        npc_gwen_armstead_35840AI(Creature* creature) : ScriptedAI(creature) {}

        EventMap m_events;
        uint8 m_say;

        void Reset() override
        {
            m_events.Reset();
            m_events.ScheduleEvent(EVENT_CHECK_TALK, 2500ms);
            m_say = 0;
        }

        void UpdateAI(uint32 diff) override
        {
            m_events.Update(diff);

            while (uint32 eventId = m_events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_CHECK_TALK:
                {
                    if (me->SelectNearestPlayer(15.0f))
                    {
                        Talk(m_say);
                        m_say += 1;
                        m_events.ScheduleEvent(EVENT_DO_NEXT_TALK, 6s);
                        break;
                    }
                    m_events.ScheduleEvent(EVENT_CHECK_TALK, 2500ms);
                    break;
                }
                case EVENT_DO_NEXT_TALK:
                {
                    Talk(m_say);
                    m_say += 1;
                    if (m_say < 2)
                        m_events.ScheduleEvent(EVENT_DO_NEXT_TALK, 6s);
                    else
                        m_events.ScheduleEvent(EVENT_DO_LAST_TALK, 6s);
                    break;
                }
                case EVENT_DO_LAST_TALK:
                {
                    Talk(m_say);
                    m_say = 0;
                    m_events.ScheduleEvent(EVENT_CHECK_TALK, 30s);
                    break;
                }
                }
            }

            if (!UpdateVictim())
                return;
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_gwen_armstead_35840AI(creature);
    }
};

/* 35077 - QUEST: 14154 - By The Skin of His Teeth - START */
class npc_lord_darius_crowley_35077 : public CreatureScript
{
public:
    npc_lord_darius_crowley_35077() : CreatureScript("npc_lord_darius_crowley_35077") { }

    enum eNpc
    {
        ACTION_START_EVENT = 101,
    };

    bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest) override
    {
        if (quest->GetQuestId() == 14154)
        {
            if (CAST_AI(npc_lord_darius_crowley_35077AI, creature->AI())->m_playerGUID.IsEmpty())
            {
                creature->AI()->SetGUID(player->GetGUID(), PLAYER_GUID);
                creature->AI()->DoAction(ACTION_START_EVENT);
                creature->CastSpell(player, SPELL_BY_THE_SKIN, true);
            }
            return true;
        }
        return false;
    }

    struct npc_lord_darius_crowley_35077AI : public ScriptedAI
    {
        npc_lord_darius_crowley_35077AI(Creature* creature) : ScriptedAI(creature), m_summons(me) { Init(); }

        enum eQ14154
        {
            EventCheckPlayer = 1,
            EventSummonNextWave,
            EventHelpPlayer,
            EventTimeout,
        };

        ObjectGuid m_playerGUID;
        EventMap m_events;
        SummonList m_summons;
        uint32 m_phase;
        uint32 m_waveCount;

        void Init()
        {
            m_events.Reset();
            m_summons.DespawnAll();
            m_phase = 0;
            m_waveCount = 0;
            m_playerGUID = ObjectGuid();
        }

        void Reset() override
        {
            if (m_phase)
                return;
            Init();
        }

        void JustSummoned(Creature* summoned) override
        {
            m_summons.Summon(summoned);
        }

        void SummonedCreatureDies(Creature* summon, Unit* /*killer*/) override
        {
            m_summons.Despawn(summon);
        }

        void DoAction(int32 /*action*/) override
        {
            m_phase = 1;
            m_events.ScheduleEvent(EventCheckPlayer, 1000ms);
            m_events.ScheduleEvent(EventSummonNextWave, 1000ms);
            m_events.ScheduleEvent(EventHelpPlayer, 250ms);
            m_events.ScheduleEvent(EventTimeout, 120000ms);
        }

        void SetGUID(ObjectGuid const& guid, int32 /*id*/) override
        {
            m_playerGUID = guid;
        }

        void UpdateAI(uint32 diff) override
        {
            m_events.Update(diff);

            while (uint32 eventId = m_events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EventCheckPlayer:
                    {
                        bool shouldStop = false;
                        if (!m_playerGUID.IsEmpty() && m_phase)
                        {
                            if (Player* player = ObjectAccessor::GetPlayer(*me, m_playerGUID))
                            {
                                if (!player->IsInWorld() || !player->HasAura(68218))
                                    shouldStop = true;
                            }
                            else
                                shouldStop = true;
                        }
                        else
                            shouldStop = true;

                        if (shouldStop)
                        {
                            m_phase = 0;
                            Init();
                            ScriptedAI::EnterEvadeMode(EvadeReason::NoHostiles);
                        }
                        else
                            m_events.ScheduleEvent(EventCheckPlayer, 1000ms);
                        break;
                    }
                    case EventTimeout:
                        m_phase = 0;
                        Init();
                        ScriptedAI::EnterEvadeMode(EvadeReason::Other);
                        break;
                    case EventSummonNextWave:
                    {
                        m_waveCount++;
                        for (int i = 0; i < 4; i++)
                        {
                            uint32 w = RAND(NPC_WORGEN_RUNT_C1, NPC_WORGEN_RUNT_C2, NPC_WORGEN_ALPHA_C1, NPC_WORGEN_ALPHA_C2);
                            if (m_waveCount % 2 == 1)
                            {
                                if (Creature* creature = me->SummonCreature(w, -1718.01f, 1516.81f, 55.40f, 4.6f, TEMPSUMMON_CORPSE_DESPAWN, 10s))
                                {
                                    creature->AI()->SetGUID(m_playerGUID, PLAYER_GUID);
                                    creature->AI()->DoAction(2);
                                    creature->SetBaseAttackTime(BASE_ATTACK, 3000);
                                }
                            }
                            else
                            {
                                if (Creature* creature = me->SummonCreature(w, -1610.39f, 1507.16f, 74.99f, 3.94f, TEMPSUMMON_CORPSE_DESPAWN, 10s))
                                {
                                    creature->AI()->SetGUID(m_playerGUID, PLAYER_GUID);
                                    creature->AI()->DoAction(1);
                                    creature->SetBaseAttackTime(BASE_ATTACK, 3000);
                                }
                            }
                        }

                        m_events.ScheduleEvent(EventSummonNextWave, 30000ms);
                        break;
                    }
                    case EventHelpPlayer:
                    {
                        if (!me->IsInCombat() && !me->IsInEvadeMode())
                        {
                            Creature* creature = nullptr;
                            creature = me->FindNearestCreature(NPC_WORGEN_RUNT_C1, 5.0f);
                            if (!creature)
                                creature = me->FindNearestCreature(NPC_WORGEN_RUNT_C2, 5.0f);
                            if (!creature)
                                creature = me->FindNearestCreature(NPC_WORGEN_ALPHA_C1, 5.0f);
                            if (!creature)
                                creature = me->FindNearestCreature(NPC_WORGEN_ALPHA_C2, 5.0f);
                            if (creature)
                                AttackStart(creature);
                        }

                        m_events.ScheduleEvent(EventHelpPlayer, 250ms);
                        break;
                    }
                }
            }

            if (!UpdateVictim())
                return;
        }

    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_lord_darius_crowley_35077AI(creature);
    }
};

// 35124
class npc_tobias_mistmantle_35124 : public CreatureScript
{
public:
    npc_tobias_mistmantle_35124() : CreatureScript("npc_tobias_mistmantle_35124") { }

    struct npc_tobias_mistmantle_35124AI : public ScriptedAI
    {
        npc_tobias_mistmantle_35124AI(Creature* creature) : ScriptedAI(creature) { }

        void DamageTaken(Unit* /*who*/, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
        {
            damage = 0;
        }

        void UpdateAI(uint32 /*diff*/) override
        {
            if (!UpdateVictim())
                return;
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_tobias_mistmantle_35124AI(creature);
    }
};

// 35188
class npc_worgen_runt_35188 : public CreatureScript
{
public:
    npc_worgen_runt_35188() : CreatureScript("npc_worgen_runt_35188") {}

    struct npc_worgen_runt_35188AI : public ScriptedAI
    {
        npc_worgen_runt_35188AI(Creature* creature) : ScriptedAI(creature) { Init(); }

        ObjectGuid m_playerGUID;
        EventMap m_events;
        uint32 m_phase;
        Position jump;
        Position JumpW1[3];
        Position LandingW1[3];
        Position LandingW2[4];

        void Init()
        {
            m_phase = 0;
            m_playerGUID = ObjectGuid();
            m_events.Reset();
            JumpW1[0] = Position(-1643.91f, 1482.96f, 63.22f, 3.95f);
            JumpW1[1] = Position(-1638.83f, 1478.07f, 65.36f, 3.84f);
            JumpW1[2] = Position(-1631.49f, 1475.02f, 65.64f, 3.84f);

            LandingW1[0] = Position(-1677.39f, 1455.52f, 52.29f, 4.06f);
            LandingW1[1] = Position(-1671.89f, 1449.27f, 52.29f, 3.84f);
            LandingW1[2] = Position(-1664.82f, 1443.62f, 52.29f, 3.84f);

            LandingW2[0] = Position(-1704.93f, 1469.07f, 52.29f, 5.34f);
            LandingW2[1] = Position(-1698.90f, 1472.92f, 52.29f, 5.41f);
            LandingW2[2] = Position(-1703.37f, 1470.66f, 52.29f, 5.49f);
            LandingW2[3] = Position(-1700.37f, 1473.32f, 52.29f, 5.41f);
        }

        void Reset() override
        {
            Init();
        }

        void DoAction(int32 action) override
        {
            if (!m_playerGUID.IsEmpty())
            {
                m_events.ScheduleEvent(1, 500ms);
                m_phase = action;
                me->SetSpeedRate(MOVE_RUN, frand(1.0f, 1.2f));
            }
        }

        void SetGUID(ObjectGuid const& guid, int32 /*id*/) override
        {
            m_playerGUID = guid;
        }

        void MovementInform(uint32 type, uint32 pointId) override
        {
            if (type == POINT_MOTION_TYPE)
                m_phase = pointId;
            else if (type == EFFECT_MOTION_TYPE && pointId == EVENT_JUMP)
                m_phase = 25;
        }

        void UpdateAI(uint32 diff) override
        {
            m_events.Update(diff);

            uint32 eventId = m_events.ExecuteEvent();
            switch (eventId)
            {
                case 1:
                {
                    m_events.ScheduleEvent(1, 500ms);
                    DoWalk();
                    break;
                }
            }

            if (m_phase > 0 && m_phase < 7)
                return;
            if (!UpdateVictim())
                return;
        }

        void DoWalk()
        {
            switch (m_phase)
            {
                case 1:
                {
                    m_phase = 3;
                    uint8 rol = urand(0, 2);
                    jump = JumpW1[rol];
                    me->GetMotionMaster()->MovePoint(11 + rol, jump, true, {}, {}, MovementWalkRunSpeedSelectionMode::ForceRun);
                    break;
                }
                case 2:
                    m_phase = 3;
                    jump = Position(-1717.73f, 1486.27f, 57.23f, 5.45f);
                    me->GetMotionMaster()->MovePoint(21, jump, true, {}, {}, MovementWalkRunSpeedSelectionMode::ForceRun);
                    break;
                case 11:
                    m_phase = 4;
                    me->GetMotionMaster()->MoveJump(EVENT_JUMP, LandingW1[0], frand(20.0f, 25.0f), frand(15.0f, 20.0f), 25);
                    break;
                case 12:
                    m_phase = 4;
                    me->GetMotionMaster()->MoveJump(EVENT_JUMP, LandingW1[1], frand(20.0f, 25.0f), frand(15.0f, 20.0f), 25);
                    break;
                case 13:
                    m_phase = 4;
                    me->GetMotionMaster()->MoveJump(EVENT_JUMP, LandingW1[2], frand(20.0f, 25.0f), frand(15.0f, 20.0f), 25);
                    break;
                case 21:
                {
                    m_phase = 5;
                    uint8 rol = urand(0, 3);
                    jump = LandingW2[rol];
                    me->GetMotionMaster()->MoveJump(EVENT_JUMP, jump, frand(20.0f, 25.0f), frand(15.0f, 20.0f), 25);
                    break;
                }
                case 25:
                    if (!m_playerGUID.IsEmpty())
                        if (Player* player = ObjectAccessor::GetPlayer(*me, m_playerGUID))
                            if (player->IsInWorld())
                            {
                                m_phase = 6;
                                Position pos = player->GetNearPosition(frand(2.0f, 4.0f), frand(3.14f, 6.28f));
                                me->GetMotionMaster()->MovePoint(26, pos, true, {}, {}, MovementWalkRunSpeedSelectionMode::ForceRun);
                            }
                    break;
                case 26:
                {
                    m_phase = 7;
                    me->SetHomePosition(me->GetPosition());
                    m_events.CancelEvent(1);
                    if (!m_playerGUID.IsEmpty())
                        if (Player* player = ObjectAccessor::GetPlayer(*me, m_playerGUID))
                            if (player->IsInWorld())
                                me->Attack(player, true);
                    break;
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_worgen_runt_35188AI(creature);
    }
};

// 35456
class npc_worgen_runt_35456 : public CreatureScript
{
public:
    npc_worgen_runt_35456() : CreatureScript("npc_worgen_runt_35456") { }

    struct npc_worgen_runt_35456AI : public ScriptedAI
    {
        npc_worgen_runt_35456AI(Creature* creature) : ScriptedAI(creature) { Init(); }

        ObjectGuid m_playerGUID;
        EventMap m_events;
        uint32 m_phase;
        Position jump;
        Position JumpW1[3];
        Position LandingW1[3];
        Position LandingW2[4];

        void Init()
        {
            m_phase = 0;
            m_playerGUID = ObjectGuid();
            m_events.Reset();
            JumpW1[0] = Position(-1643.91f, 1482.96f, 63.22f, 3.95f);
            JumpW1[1] = Position(-1638.83f, 1478.07f, 65.36f, 3.84f);
            JumpW1[2] = Position(-1631.49f, 1475.02f, 65.64f, 3.84f);

            LandingW1[0] = Position(-1677.39f, 1455.52f, 52.29f, 4.06f);
            LandingW1[1] = Position(-1671.89f, 1449.27f, 52.29f, 3.84f);
            LandingW1[2] = Position(-1664.82f, 1443.62f, 52.29f, 3.84f);

            LandingW2[0] = Position(-1704.93f, 1469.07f, 52.29f, 5.34f);
            LandingW2[1] = Position(-1698.90f, 1472.92f, 52.29f, 5.41f);
            LandingW2[2] = Position(-1703.37f, 1470.66f, 52.29f, 5.49f);
            LandingW2[3] = Position(-1700.37f, 1473.32f, 52.29f, 5.41f);
        }

        void Reset() override
        {
            Init();
        }

        void DoAction(int32 action) override
        {
            if (!m_playerGUID.IsEmpty())
            {
                m_events.ScheduleEvent(1, 500ms);
                m_phase = action;
                me->SetSpeedRate(MOVE_RUN, frand(1.0f, 1.2f));
            }
        }

        void SetGUID(ObjectGuid const& guid, int32 /*id*/) override
        {
            m_playerGUID = guid;
        }

        void MovementInform(uint32 type, uint32 pointId) override
        {
            if (type == POINT_MOTION_TYPE)
                m_phase = pointId;
            else if (type == EFFECT_MOTION_TYPE && pointId == EVENT_JUMP)
                m_phase = 25;
        }

        void UpdateAI(uint32 diff) override
        {
            m_events.Update(diff);

            uint32 eventId = m_events.ExecuteEvent();
            switch (eventId)
            {
            case 1:
            {
                m_events.ScheduleEvent(1, 500ms);
                DoWalk();
                break;
            }
            }

            if (m_phase > 0 && m_phase < 7)
                return;
            if (!UpdateVictim())
                return;
        }

        void DoWalk()
        {
            switch (m_phase)
            {
                case 1:
                {
                    m_phase = 3;
                    uint8 rol = urand(0, 2);
                    jump = JumpW1[rol];
                    me->GetMotionMaster()->MovePoint(11 + rol, jump, true, {}, {}, MovementWalkRunSpeedSelectionMode::ForceRun);
                    break;
                }
                case 2:
                    m_phase = 3;
                    jump = Position(-1717.73f, 1486.27f, 57.23f, 5.45f);
                    me->GetMotionMaster()->MovePoint(21, jump, true, {}, {}, MovementWalkRunSpeedSelectionMode::ForceRun);
                    break;
                case 11:
                    m_phase = 4;
                    me->GetMotionMaster()->MoveJump(EVENT_JUMP, LandingW1[0], frand(20.0f, 25.0f), frand(15.0f, 20.0f), 25);
                    break;
                case 12:
                    m_phase = 4;
                    me->GetMotionMaster()->MoveJump(EVENT_JUMP, LandingW1[1], frand(20.0f, 25.0f), frand(15.0f, 20.0f), 25);
                    break;
                case 13:
                    m_phase = 4;
                    me->GetMotionMaster()->MoveJump(EVENT_JUMP, LandingW1[2], frand(20.0f, 25.0f), frand(15.0f, 20.0f), 25);
                    break;
                case 21:
                {
                    m_phase = 5;
                    uint8 rol = urand(0, 3);
                    jump = LandingW2[rol];
                    me->GetMotionMaster()->MoveJump(EVENT_JUMP, jump, frand(20.0f, 25.0f), frand(15.0f, 20.0f), 25);
                    break;
                }
                case 25:
                    if (!m_playerGUID.IsEmpty())
                        if (Player* player = ObjectAccessor::GetPlayer(*me, m_playerGUID))
                            if (player->IsInWorld())
                            {
                                m_phase = 6;
                                Position pos = player->GetNearPosition(frand(2.0f, 4.0f), frand(3.14f, 6.28f));
                                me->GetMotionMaster()->MovePoint(26, pos, true, {}, {}, MovementWalkRunSpeedSelectionMode::ForceRun);
                            }
                    break;
                case 26:
                {
                    m_phase = 7;
                    me->SetHomePosition(me->GetPosition());
                    m_events.CancelEvent(1);
                    if (!m_playerGUID.IsEmpty())
                        if (Player* player = ObjectAccessor::GetPlayer(*me, m_playerGUID))
                            if (player->IsInWorld())
                                me->Attack(player, true);
                    break;
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_worgen_runt_35456AI(creature);
    }
};

// 35170
class npc_worgen_alpha_35170 : public CreatureScript
{
public:
    npc_worgen_alpha_35170() : CreatureScript("npc_worgen_alpha_35170") {}

    struct npc_worgen_alpha_35170AI : public ScriptedAI
    {
        npc_worgen_alpha_35170AI(Creature* creature) : ScriptedAI(creature) { Init(); }

        ObjectGuid m_playerGUID;
        EventMap m_events;
        uint32 m_phase;
        Position jump;
        Position JumpW1[3];
        Position LandingW1[3];
        Position LandingW2[4];

        void Init()
        {
            m_phase = 0;
            m_playerGUID = ObjectGuid();
            m_events.Reset();
            JumpW1[0] = Position(-1643.91f, 1482.96f, 63.22f, 3.95f);
            JumpW1[1] = Position(-1638.83f, 1478.07f, 65.36f, 3.84f);
            JumpW1[2] = Position(-1631.49f, 1475.02f, 65.64f, 3.84f);

            LandingW1[0] = Position(-1677.39f, 1455.52f, 52.29f, 4.06f);
            LandingW1[1] = Position(-1671.89f, 1449.27f, 52.29f, 3.84f);
            LandingW1[2] = Position(-1664.82f, 1443.62f, 52.29f, 3.84f);

            LandingW2[0] = Position(-1704.93f, 1469.07f, 52.29f, 5.34f);
            LandingW2[1] = Position(-1698.90f, 1472.92f, 52.29f, 5.41f);
            LandingW2[2] = Position(-1703.37f, 1470.66f, 52.29f, 5.49f);
            LandingW2[3] = Position(-1700.37f, 1473.32f, 52.29f, 5.41f);
        }

        void Reset() override
        {
            Init();
        }

        void DoAction(int32 action) override
        {
            if (!m_playerGUID.IsEmpty())
            {
                m_events.ScheduleEvent(1, 500ms);
                m_phase = action;
                me->SetSpeedRate(MOVE_RUN, frand(1.0f, 1.2f));
            }
        }

        void SetGUID(ObjectGuid const& guid, int32 /*id*/) override
        {
            m_playerGUID = guid;
        }

        void MovementInform(uint32 type, uint32 pointId) override
        {
            if (type == POINT_MOTION_TYPE)
                m_phase = pointId;
            else if (type == EFFECT_MOTION_TYPE && pointId == EVENT_JUMP)
                m_phase = 25;
        }

        void UpdateAI(uint32 diff) override
        {
            m_events.Update(diff);

            uint32 eventId = m_events.ExecuteEvent();
            switch (eventId)
            {
                case 1:
                {
                    m_events.ScheduleEvent(1, 500ms);
                    DoWalk();
                    break;
                }
            }

            if (m_phase > 0 && m_phase < 7)
                return;
            if (!UpdateVictim())
                return;
        }

        void DoWalk()
        {
            switch (m_phase)
            {
                case 1:
                {
                    m_phase = 3;
                    uint8 rol = urand(0, 2);
                    jump = JumpW1[rol];
                    me->GetMotionMaster()->MovePoint(11 + rol, jump, true, {}, {}, MovementWalkRunSpeedSelectionMode::ForceRun);
                    break;
                }
                case 2:
                    m_phase = 3;
                    jump = Position(-1717.73f, 1486.27f, 57.23f, 5.45f);
                    me->GetMotionMaster()->MovePoint(21, jump, true, {}, {}, MovementWalkRunSpeedSelectionMode::ForceRun);
                    break;
                case 11:
                    m_phase = 4;
                    me->GetMotionMaster()->MoveJump(EVENT_JUMP, LandingW1[0], frand(20.0f, 25.0f), frand(15.0f, 20.0f), 25);
                    break;
                case 12:
                    m_phase = 4;
                    me->GetMotionMaster()->MoveJump(EVENT_JUMP, LandingW1[1], frand(20.0f, 25.0f), frand(15.0f, 20.0f), 25);
                    break;
                case 13:
                    m_phase = 4;
                    me->GetMotionMaster()->MoveJump(EVENT_JUMP, LandingW1[2], frand(20.0f, 25.0f), frand(15.0f, 20.0f), 25);
                    break;
                case 21:
                {
                    m_phase = 5;
                    uint8 rol = urand(0, 3);
                    jump = LandingW2[rol];
                    me->GetMotionMaster()->MoveJump(EVENT_JUMP, jump, frand(20.0f, 25.0f), frand(15.0f, 20.0f), 25);
                    break;
                }
                case 25:
                    if (!m_playerGUID.IsEmpty())
                        if (Player* player = ObjectAccessor::GetPlayer(*me, m_playerGUID))
                            if (player->IsInWorld())
                            {
                                m_phase = 6;
                                Position pos = player->GetNearPosition(frand(2.0f, 4.0f), frand(3.14f, 6.28f));
                                me->GetMotionMaster()->MovePoint(26, pos, true, {}, {}, MovementWalkRunSpeedSelectionMode::ForceRun);
                            }
                    break;
                case 26:
                {
                    m_phase = 7;
                    me->SetHomePosition(me->GetPosition());
                    m_events.CancelEvent(1);
                    if (!m_playerGUID.IsEmpty())
                        if (Player* player = ObjectAccessor::GetPlayer(*me, m_playerGUID))
                            if (player->IsInWorld())
                                me->Attack(player, true);
                    break;
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_worgen_alpha_35170AI(creature);
    }
};

// 35167
class npc_worgen_alpha_35167 : public CreatureScript
{
public:
    npc_worgen_alpha_35167() : CreatureScript("npc_worgen_alpha_35167") {}

    struct npc_worgen_alpha_35167AI : public ScriptedAI
    {
        npc_worgen_alpha_35167AI(Creature* creature) : ScriptedAI(creature) { Init(); }

        ObjectGuid m_playerGUID;
        EventMap m_events;
        uint32 m_phase;
        Position jump;
        Position JumpW1[3];
        Position LandingW1[3];
        Position LandingW2[4];

        void Init()
        {
            m_phase = 0;
            m_playerGUID = ObjectGuid();
            m_events.Reset();
            JumpW1[0] = Position(-1643.91f, 1482.96f, 63.22f, 3.95f);
            JumpW1[1] = Position(-1638.83f, 1478.07f, 65.36f, 3.84f);
            JumpW1[2] = Position(-1631.49f, 1475.02f, 65.64f, 3.84f);

            LandingW1[0] = Position(-1677.39f, 1455.52f, 52.29f, 4.06f);
            LandingW1[1] = Position(-1671.89f, 1449.27f, 52.29f, 3.84f);
            LandingW1[2] = Position(-1664.82f, 1443.62f, 52.29f, 3.84f);

            LandingW2[0] = Position(-1704.93f, 1469.07f, 52.29f, 5.34f);
            LandingW2[1] = Position(-1698.90f, 1472.92f, 52.29f, 5.41f);
            LandingW2[2] = Position(-1703.37f, 1470.66f, 52.29f, 5.49f);
            LandingW2[3] = Position(-1700.37f, 1473.32f, 52.29f, 5.41f);
        }

        void Reset() override
        {
            Init();
        }

        void DoAction(int32 action) override
        {
            if (!m_playerGUID.IsEmpty())
            {
                m_events.ScheduleEvent(1, 500ms);
                m_phase = action;
                me->SetSpeedRate(MOVE_RUN, frand(1.0f, 1.2f));
            }
        }

        void SetGUID(ObjectGuid const& guid, int32 /*id*/) override
        {
            m_playerGUID = guid;
        }

        void MovementInform(uint32 type, uint32 pointId) override
        {
            if (type == POINT_MOTION_TYPE)
                m_phase = pointId;
            else if (type == EFFECT_MOTION_TYPE && pointId == EVENT_JUMP)
                m_phase = 25;
        }

        void UpdateAI(uint32 diff) override
        {
            m_events.Update(diff);

            uint32 eventId = m_events.ExecuteEvent();
            switch (eventId)
            {
            case 1:
            {
                m_events.ScheduleEvent(1, 500ms);
                DoWalk();
                break;
            }
            }

            if (m_phase > 0 && m_phase < 7)
                return;
            if (!UpdateVictim())
                return;
        }

        void DoWalk()
        {
            switch (m_phase)
            {
                case 1:
                {
                    m_phase = 3;
                    uint8 rol = urand(0, 2);
                    jump = JumpW1[rol];
                    me->GetMotionMaster()->MovePoint(11 + rol, jump, true, {}, {}, MovementWalkRunSpeedSelectionMode::ForceRun);
                    break;
                }
                case 2:
                    m_phase = 3;
                    jump = Position(-1717.73f, 1486.27f, 57.23f, 5.45f);
                    me->GetMotionMaster()->MovePoint(21, jump, true, {}, {}, MovementWalkRunSpeedSelectionMode::ForceRun);
                    break;
                case 11:
                    m_phase = 4;
                    me->GetMotionMaster()->MoveJump(EVENT_JUMP, LandingW1[0], frand(20.0f, 25.0f), frand(15.0f, 20.0f), 25);
                    break;
                case 12:
                    m_phase = 4;
                    me->GetMotionMaster()->MoveJump(EVENT_JUMP, LandingW1[1], frand(20.0f, 25.0f), frand(15.0f, 20.0f), 25);
                    break;
                case 13:
                    m_phase = 4;
                    me->GetMotionMaster()->MoveJump(EVENT_JUMP, LandingW1[2], frand(20.0f, 25.0f), frand(15.0f, 20.0f), 25);
                    break;
                case 21:
                {
                    m_phase = 5;
                    uint8 rol = urand(0, 3);
                    jump = LandingW2[rol];
                    me->GetMotionMaster()->MoveJump(EVENT_JUMP, jump, frand(20.0f, 25.0f), frand(15.0f, 20.0f), 25);
                    break;
                }
                case 25:
                    if (!m_playerGUID.IsEmpty())
                        if (Player* player = ObjectAccessor::GetPlayer(*me, m_playerGUID))
                            if (player->IsInWorld())
                            {
                                m_phase = 6;
                                Position pos = player->GetNearPosition(frand(2.0f, 4.0f), frand(3.14f, 6.28f));
                                me->GetMotionMaster()->MovePoint(26, pos, true, {}, {}, MovementWalkRunSpeedSelectionMode::ForceRun);
                            }
                    break;
                case 26:
                {
                    m_phase = 7;
                    me->SetHomePosition(me->GetPosition());
                    m_events.CancelEvent(1);
                    if (!m_playerGUID.IsEmpty())
                        if (Player* player = ObjectAccessor::GetPlayer(*me, m_playerGUID))
                            if (player->IsInWorld())
                                me->Attack(player, true);
                    break;
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_worgen_alpha_35167AI(creature);
    }
};
/* QUEST - 14154 - By The Skin of His Teeth - END */

// 35112
class npc_king_greymane_35112 : public CreatureScript
{
public:
    npc_king_greymane_35112() : CreatureScript("npc_king_greymane_35112") {}

    struct npc_king_greymane_35112AI : public ScriptedAI
    {
        npc_king_greymane_35112AI(Creature* creature) : ScriptedAI(creature) {}

        EventMap m_events;
        ObjectGuid m_playerGUID;

        enum eEvents
        {
            EVENT_GODFREY_TALK = 101,
            EVENT_GREYMANE_TALK,
        };

        void OnQuestReward(Player* player, Quest const* quest, LootItemType /*type*/, uint32 /*opt*/) override
        {
            if (quest->GetQuestId() == QUEST_ESCAPE_BY_BLOOD)
            {
                m_playerGUID = player->GetGUID();
                if (Creature* godfrey = me->FindNearestCreature(NPC_LORD_GODFREY_35115, 10.0f))
                    godfrey->AI()->Talk(0, player);
                m_events.ScheduleEvent(EVENT_GREYMANE_TALK, 5s);
            }
        }

        void UpdateAI(uint32 diff) override
        {
            m_events.Update(diff);

            while (uint32 eventId = m_events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_GREYMANE_TALK:
                {
                    if (Player* player = ObjectAccessor::GetPlayer(*me, m_playerGUID))
                        Talk(0, player);
                    break;
                }
                }
            }

            if (!UpdateVictim())
                return;
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_king_greymane_35112AI(creature);
    }
};

/* QUEST - 14159 - The Rebel Lord's Arsenal - START */
// 35369
class npc_josiah_avery_35369 : public CreatureScript
{
public:
    npc_josiah_avery_35369() : CreatureScript("npc_josiah_avery_35369") {}

    enum eNpc
    {
        DELAY_SAY_JOSIAH_AVERY = 20000,
        EVENT_SAY_JOSIAH_AVERY = 101,
        ACTION_START_ANIM = 102,
    };

    struct npc_josiah_avery_35369AI : public ScriptedAI
    {
        npc_josiah_avery_35369AI(Creature* creature) : ScriptedAI(creature) {}

        EventMap m_events;
        uint32 m_currentSayCounter; // Current Say
        ObjectGuid m_playerGUID;
        ObjectGuid m_badAveryGUID;
        ObjectGuid m_triggerGUID;

        void OnQuestReward(Player* player, Quest const* quest, LootItemType /*type*/, uint32 /*opt*/) override
        {
            if (quest->GetQuestId() == QUEST_THE_REBEL_LORDS_ARSENAL)
            {
                me->AddAura(SPELL_WORGEN_BITE, player);
                me->AddAura(SPELL_PHASE_QUEST_ZONE_SPECIFIC_02, player);
                player->SaveToDB();
                SetGUID(player->GetGUID(), PLAYER_GUID);
                DoAction(ACTION_START_ANIM);
            }
        }

        // Evade or Respawn
        void Reset() override
        {
            m_playerGUID = ObjectGuid();
            m_badAveryGUID = ObjectGuid();
            m_triggerGUID = ObjectGuid();
            m_events.Reset();
            m_events.ScheduleEvent(EVENT_SAY_JOSIAH_AVERY, 20000ms);
            m_currentSayCounter = 0;
        }

        void SetGUID(ObjectGuid const& guid, int32 id) override
        {
            switch (id)
            {
                case PLAYER_GUID:
                {
                    m_playerGUID = guid;
                    break;
                }
            }
        }

        void DoAction(int32 param) override
        {
            switch (param)
            {
                case ACTION_START_ANIM:
                {
                    m_events.ScheduleEvent(EVENT_START_ANIM, 10ms);
                    break;
                }
            }
        }

        //Timed events
        void UpdateAI(uint32 diff) override
        {
            m_events.Update(diff);

            while (uint32 eventId = m_events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_SAY_JOSIAH_AVERY:
                {
                    m_currentSayCounter += 1;
                    if (m_currentSayCounter > 5)
                        m_currentSayCounter = 1;

                    std::list<Player*> pList = GetListOfPlayersNearAndIndoorsAndWithQuest();
                    TalkToGroup(pList, m_currentSayCounter);

                    m_events.ScheduleEvent(EVENT_SAY_JOSIAH_AVERY, 20000ms);
                    break;
                }
                case EVENT_START_ANIM:
                {
                    if (Player* player = ObjectAccessor::GetPlayer(*me, m_playerGUID))
                        me->CastSpell(player, SPELL_FORCE_CAST_SUMMON_JOSIAH, true);
                    break;
                }
                }
            }
        }

        std::list<Player*> GetListOfPlayersNearAndIndoorsAndWithQuest()
        {
            std::list<Player*> pList = me->SelectNearestPlayers(20.0f, true);
            pList.remove_if([](Player* player)
            {
                return player->GetQuestStatus(QUEST_THE_REBEL_LORDS_ARSENAL) != QUEST_STATUS_COMPLETE;
            });
            return pList;
        }

        void TalkToGroup(std::list<Player*> pList, uint8 groupId)
        {
            if (pList.empty())
                return;

            for (std::list<Player*>::const_iterator itr = pList.begin(); itr != pList.end(); itr++)
                Talk(groupId, (*itr));
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_josiah_avery_35369AI(creature);
    }
};

// 35370
class npc_josiah_avery_35370 : public CreatureScript
{
public:
    npc_josiah_avery_35370() : CreatureScript("npc_josiah_avery_35370") {}

    struct npc_josiah_avery_35370AI : public ScriptedAI
    {
        npc_josiah_avery_35370AI(Creature* creature) : ScriptedAI(creature) {}

        void IsSummonedBy(WorldObject* /*summoner*/) override
        {
            PhasingHandler::AddPhase(me, 171, true);
            me->SetReactState(REACT_PASSIVE);
            me->CombatStop();
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_josiah_avery_35370AI(creature);
    }
};

// 50415
class npc_josiah_avery_trigger_50415 : public CreatureScript
{
public:
    npc_josiah_avery_trigger_50415() : CreatureScript("npc_josiah_avery_trigger_50415") {}

    enum eNpc
    {
        SAY_JOSAIH_AVERY_TRIGGER = 0,
        EVENT_KNOCKBACK = 1,
        EVENT_SHOOT,
        EVENT_WHISPER,
    };

    struct npc_josiah_avery_trigger_50415AI : public ScriptedAI
    {
        npc_josiah_avery_trigger_50415AI(Creature* creature) : ScriptedAI(creature) {}

        EventMap m_events;
        ObjectGuid m_playerGUID;
        ObjectGuid m_badAveryGUID;

        void IsSummonedBy(WorldObject* summoner) override
        {
            PhasingHandler::AddPhase(me, 171, true);

            Player* player = nullptr;
            if (summoner->IsPlayer())
                player = summoner->ToPlayer();
            else
                player = me->SelectNearestPlayer(30.0f);

            if (player)
            {
                m_playerGUID = player->GetGUID();
                me->SetDisplayId(11686);
                m_events.ScheduleEvent(EVENT_KNOCKBACK, 500ms);
            }

            me->DespawnOrUnsummon(16000ms);
        }

        void UpdateAI(uint32 diff) override
        {
            m_events.Update(diff);

            while (uint32 eventId = m_events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_KNOCKBACK:
                {
                    if (Player* player = ObjectAccessor::GetPlayer(*me, m_playerGUID))
                    {
                        std::list<Creature*> allCreatures;
                        me->GetCreatureListWithEntryInGrid(allCreatures, NPC_JOSIAH_AVERY_35370, 100.0f);
                        if (!allCreatures.empty())
                        {
                            Creature* badAvery = allCreatures.front();
                            m_badAveryGUID = badAvery->GetGUID();
                            badAvery->SetReactState(REACT_PASSIVE);
                            badAvery->CombatStop();
                            badAvery->SetOrientation(badAvery->GetAbsoluteAngle(player));
                            badAvery->CastSpell(player, SPELL_COSMETIC_COMBAT_ATTACK, true);
                            player->GetMotionMaster()->MoveJump(EVENT_CHARGE, Position(-1791.94f, 1427.29f, 12.4584f), 22.0f, 2.0f);
                        }
                    }
                    m_events.ScheduleEvent(EVENT_SHOOT, 1200ms);
                    break;
                }
                case EVENT_SHOOT:
                {
                    if (Creature* badAvery = ObjectAccessor::GetCreature(*me, m_badAveryGUID))
                    {
                        std::list<Creature*> lornaList;
                        me->GetCreatureListWithEntryInGrid(lornaList, NPC_LORNA_CROWLEY_35378, 100.0f);
                        if (!lornaList.empty())
                        {
                            Creature* lorna = lornaList.front();
                            lorna->SetOrientation(lorna->GetAbsoluteAngle(badAvery));
                            lorna->CastSpell(badAvery, SPELL_SHOOT, CastSpellExtraArgs(TriggerCastFlags(TRIGGERED_FULL_MASK | TRIGGERED_IGNORE_TARGET_CHECK)));
                        }
                        badAvery->CastSpell(badAvery, SPELL_GET_SHOT, true);
                        me->Kill(me, badAvery);
                        badAvery->DespawnOrUnsummon(1000ms);
                    }
                    m_events.ScheduleEvent(EVENT_WHISPER, 200ms);
                    break;
                }
                case EVENT_WHISPER:
                {
                    if (Player* player = ObjectAccessor::GetPlayer(*me, m_playerGUID))
                        Talk(SAY_JOSAIH_AVERY_TRIGGER, player);
                    me->DespawnOrUnsummon(1000ms);
                    break;
                }
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_josiah_avery_trigger_50415AI(creature);
    }
};
/* QUEST - 14159 - The Rebel Lord's Arsenal - END */



/* QUEST - 14204 - From the Shadows - START */
// 35378
class npc_lorna_crowley_35378 : public CreatureScript
{
public:
    npc_lorna_crowley_35378() : CreatureScript("npc_lorna_crowley_35378") {}

    bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest) override
    {
        if (quest->GetQuestId() == QUEST_FROM_THE_SHADOWS)
        {
            if (Pet* pet = player->GetPet())
                player->RemovePet(pet, PET_SAVE_NOT_IN_SLOT);
            player->CastSpell(player, SPELL_SUMMON_GILNEAN_MASTIFF);
            creature->AI()->Talk(0);
        }
        return true;
    }

    struct npc_lorna_crowley_35378AI : public ScriptedAI
    {
        npc_lorna_crowley_35378AI(Creature* creature) : ScriptedAI(creature) { }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_lorna_crowley_35378AI(creature);
    }
};

// 35631
class npc_gilnean_mastiff_35631 : public CreatureScript
{
public:
    npc_gilnean_mastiff_35631() : CreatureScript("npc_gilnean_mastiff_35631") { }

    enum eNpc
    {
        EVENT_CHECK_QUEST_REWARDED = 101,
    };

    struct npc_gilnean_mastiff_35631AI : public ScriptedAI
    {
        npc_gilnean_mastiff_35631AI(Creature* creature) : ScriptedAI(creature) { }

        EventMap m_events;
        ObjectGuid m_playerGUID;
        ObjectGuid m_lurkerGUID;

        void Reset() override
        {
            me->InitCharmInfo();
            me->GetCharmInfo()->InitEmptyActionBar(false);
            me->GetCharmInfo()->SetActionBar(0, SPELL_ATTACK_LURKER, ACT_PASSIVE);
            me->SetReactState(REACT_DEFENSIVE);
            me->GetCharmInfo()->SetIsFollowing(true);

            m_events.Reset();
            m_events.ScheduleEvent(EVENT_CHECK_QUEST_REWARDED, 10s);
        }

        void IsSummonedBy(WorldObject* summoner) override
        {
            if (Player* player = summoner->ToPlayer())
                m_playerGUID = player->GetGUID();
        }

        void JustDied(Unit* /*killer*/) override
        {
            me->DespawnOrUnsummon(1s);
        }

        void UpdateAI(uint32 diff) override
        {
            m_events.Update(diff);

            while (uint32 eventId = m_events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_CHECK_QUEST_REWARDED:
                {
                    if (Player* player = ObjectAccessor::GetPlayer(*me, m_playerGUID))
                    {
                        QuestStatus status = player->GetQuestStatus(QUEST_FROM_THE_SHADOWS);
                        if (status == QUEST_STATUS_REWARDED || status == QUEST_STATUS_NONE)
                            me->DespawnOrUnsummon(1s);
                    }
                    else
                        me->DespawnOrUnsummon(1s);

                    m_events.ScheduleEvent(EVENT_CHECK_QUEST_REWARDED, 10s);
                    break;
                }
                }
            }

            if (!UpdateVictim())
                return;
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_gilnean_mastiff_35631AI(creature);
    }
};

// 35463
class npc_bloodfang_lurker_35463 : public CreatureScript
{
public:
    npc_bloodfang_lurker_35463() : CreatureScript("npc_bloodfang_lurker_35463") {}

    enum eNpc
    {
        COOLDOWN_ENRAGE = 121000,
    };

    struct npc_bloodfang_lurker_35463AI : public ScriptedAI
    {
        npc_bloodfang_lurker_35463AI(Creature* creature) : ScriptedAI(creature) {}

        uint32 tEnrage, tSeek;
        bool willCastEnrage;

        void Reset() override
        {
            tEnrage = 0;
            willCastEnrage = urand(0, 1);
            tSeek = urand(5000, 10000);
            DoCast(me, SPELL_SHADOWSTALKER_STEALTH);
        }

        void MoveInLineOfSight(Unit*) override {}

        void SpellHit(WorldObject* caster, SpellInfo const* spellInfo) override
        {
            if (spellInfo->Id == SPELL_ATTACK_LURKER)
            {
                me->RemoveAura(SPELL_SHADOWSTALKER_STEALTH);
                if (Unit* unit = caster->ToUnit())
                    AttackStart(unit);
            }
        }

        void UpdateAI(uint32 diff) override
        {
            if (tSeek <= diff)
            {
                if (me->IsAlive() && !me->IsInCombat() &&
                    me->GetDistance2d(me->GetHomePosition().GetPositionX(), me->GetHomePosition().GetPositionY()) <= 0.7f)
                {
                    if (Player* player = me->SelectNearestPlayer(0.5f))
                    {
                        if (!player->IsInCombat())
                        {
                            me->RemoveAura(SPELL_SHADOWSTALKER_STEALTH);
                            AttackStart(player);
                            tSeek = urand(5000, 10000);
                        }
                    }
                }
            }
            else
                tSeek -= diff;

            if (!UpdateVictim())
                return;

            if (tEnrage <= diff && willCastEnrage && me->GetHealthPct() <= 30)
            {
                me->TextEmote(-106, 0);
                DoCast(me, SPELL_ENRAGE_8599);
                tEnrage = COOLDOWN_ENRAGE;
            }
            else
                tEnrage -= diff;
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_bloodfang_lurker_35463AI(creature);
    }
};
/* QUEST - 14204 - From the Shadows - END */

// 35504
class npc_gilnean_city_guard_35504 : public CreatureScript
{
public:
    npc_gilnean_city_guard_35504() : CreatureScript("npc_gilnean_city_guard_35504") {}

    enum eNpc
    {
        EVENT_CHECK_SHOWFIGHT = 101,
        MOVE_TO_HOMEPOSITION = 101,
    };

    struct npc_gilnean_city_guard_35504AI : public ScriptedAI
    {
        npc_gilnean_city_guard_35504AI(Creature* creature) : ScriptedAI(creature) { Initialize(); }

        EventMap m_events;
        float    m_minHealthPct;
        std::list<uint32> m_worgenList;

        void Initialize()
        {
            m_worgenList.clear();
            m_worgenList.push_back(NPC_RAMPAGING_WORGEN_35660);
            m_worgenList.push_back(NPC_BLOODFANG_RIPPER_35505);
            m_minHealthPct = frand(30.0f, 85.0f);
        }

        void Reset() override
        {
            m_events.Reset();
            m_events.ScheduleEvent(EVENT_CHECK_SHOWFIGHT, 1000ms);
        }

        void DamageTaken(Unit* who, uint32 &Damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
        {
            if (who->ToCreature())
            {
                if (me->GetHealthPct() < m_minHealthPct)
                    Damage = 0;
                else
                    Damage = 1;
            }
        }

        void MovementInform(uint32 type, uint32 pointId) override
        {
            if (type == POINT_MOTION_TYPE && pointId == MOVE_TO_HOMEPOSITION)
                m_events.ScheduleEvent(EVENT_CHECK_SHOWFIGHT, 25ms);
        }

        void UpdateAI(uint32 diff) override
        {
            m_events.Update(diff);

            while (uint32 eventId = m_events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_CHECK_SHOWFIGHT:
                {
                    if (!me->IsAlive() || me->IsInCombat())
                    {
                        m_events.ScheduleEvent(EVENT_CHECK_SHOWFIGHT, 2500ms);
                        return;
                    }

                    if (me->GetDistance2d(me->GetHomePosition().GetPositionX(), me->GetHomePosition().GetPositionY()) > 10.0f)
                    {
                        me->GetMotionMaster()->MovePoint(MOVE_TO_HOMEPOSITION, me->GetHomePosition());
                        return;
                    }

                    for (uint32 worgenEntry : m_worgenList)
                    {
                        if (Creature* worgen = me->FindNearestCreature(worgenEntry, 10.0f))
                        {
                            me->Attack(worgen, true);
                            worgen->Attack(me, true);
                            m_events.ScheduleEvent(EVENT_CHECK_SHOWFIGHT, 2500ms);
                            return;
                        }
                    }

                    m_events.ScheduleEvent(EVENT_CHECK_SHOWFIGHT, 2500ms);
                    break;
                }
                }
            }

            if (!UpdateVictim())
                return;
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_gilnean_city_guard_35504AI(creature);
    }
};

// 35550
class npc_king_genn_greymane_35550 : public CreatureScript
{
public:
    npc_king_genn_greymane_35550() : CreatureScript("npc_king_genn_greymane_35550") {}

        enum eNpc
        {
            SAY_DONT_GIVE_UP = 1,
            SAY_PUSH_THEM_BACK = 2,
            MAX_SUMMONED_RIPPER = 20,
            TIMER_WAVE = 60000,
        };

    bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest) override
    {
        if (quest->GetQuestId() == QUEST_SAVE_KRENNAN_ARANAS)
        {
            creature->GetAI()->SetGUID(player->GetGUID(), PLAYER_GUID);
            player->AddAura(SPELL_GENERIC_QUEST_INVISIBILITY_DETECTION_1, player);
            creature->CastSpell(player, SPELL_FORCECAST_SUMMON_GRAYMANE_HORSE, true);
        }
        return true;
    }

    struct npc_king_genn_greymane_35550AI : public ScriptedAI
    {
        npc_king_genn_greymane_35550AI(Creature* creature) : ScriptedAI(creature) {}

        uint32 tSummon;
        uint32 m_counter;
        uint8 m_sayGroup;
        ObjectGuid m_playerGUID;

        void Reset() override
        {
            tSummon = TIMER_WAVE;
            m_counter = 0;
            m_sayGroup = SAY_DONT_GIVE_UP;
        }

        void JustSummoned(Creature* summoned) override
        {
            if (summoned->GetEntry() == NPC_BLOODFANG_RIPPER_35505)
            {
                m_counter += 1;
                summoned->SetReactState(REACT_AGGRESSIVE);
            }
        }

        void SummonedCreatureDespawn(Creature* summon) override
        {
            if (summon->GetEntry() == NPC_BLOODFANG_RIPPER_35505 && m_counter)
                m_counter -= 1;
        }

        void SetGUID(ObjectGuid const& guid, int32 id) override
        {
            if (id == PLAYER_GUID)
                m_playerGUID = guid;
        }

        void UpdateAI(uint32 diff) override
        {
            if (tSummon <= diff)
            {
                Talk(m_sayGroup);
                m_sayGroup = (m_sayGroup == SAY_DONT_GIVE_UP) ? SAY_PUSH_THEM_BACK : SAY_DONT_GIVE_UP;
                SummonWave();
                tSummon = TIMER_WAVE;
            }
            else tSummon -= diff;
        }

        void SummonWave()
        {
            if (m_counter >= MAX_SUMMONED_RIPPER)
                return;

            for (int i = 0; i < 5; i++)
                me->SummonCreature(NPC_BLOODFANG_RIPPER_35505, -1781.17f + irand(-15, 15), 1372.90f + irand(-15, 15), 19.78f, urand(0, 6), TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 15000ms);
            for (int i = 0; i < 5; i++)
                me->SummonCreature(NPC_BLOODFANG_RIPPER_35505, -1756.30f + irand(-15, 15), 1380.61f + irand(-15, 15), 19.76f, urand(0, 6), TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 15000ms);
            for (int i = 0; i < 5; i++)
                me->SummonCreature(NPC_BLOODFANG_RIPPER_35505, -1739.84f + irand(-15, 15), 1384.87f + irand(-15, 15), 19.84f, urand(0, 6), TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 15000ms);
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_king_genn_greymane_35550AI(creature);
    }
};

// 68219 - DBC handles: trigger 68228 (summon 35907 + kill credit), remove aura 49416
// Script handles: range check (must be near 35753), despawn 35753
class spell_rescue_krennan_68219 : public SpellScript
{
    void HandleCast()
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        Player* player = nullptr;
        if (caster->IsPlayer())
            player = caster->ToPlayer();
        else if (caster->IsVehicle())
            if (Unit* passenger = caster->GetVehicleKit()->GetPassenger(0))
                player = passenger->ToPlayer();

        if (!player)
            return;

        player->AreaExploredOrEventHappens(QUEST_SAVE_KRENNAN_ARANAS);

        if (Creature* krennanTree = player->FindNearestCreature(NPC_KRENNAN_ARANAS_TREE, 50.0f, true))
            krennanTree->DespawnOrUnsummon();
    }

    void Register() override
    {
        OnCast += SpellCastFn(spell_rescue_krennan_68219::HandleCast);
    }
};

// 35905
class npc_king_greymanes_horse_35905 : public CreatureScript
{
    public:
        npc_king_greymanes_horse_35905() : CreatureScript("npc_king_greymanes_horse_35905") {}

        enum eHorse
        {
            EVENT_JUMP_TO_KRENNAN = 102,
            EVENT_RESUME_PATH = 103,
            EVENT_START_PATH = 104,
            EVENT_FORCE_WORGEN_ATTACK = 106,
            PATH_ID = 3590500,
            PATH_ID_PART2 = 3590501,
        };

        static WaypointPath const& GetPathPart1()
        {
            static WaypointPath path(PATH_ID, {
                { 0,  -1800.89f, 1407.40f, 19.8611f },
                { 1,  -1797.47f, 1396.97f, 20.0717f },
                { 2,  -1788.72f, 1378.22f, 20.0717f },
                { 3,  -1780.72f, 1368.97f, 20.0717f },
                { 4,  -1767.47f, 1358.22f, 19.8217f },
                { 5,  -1746.47f, 1358.72f, 20.0717f },
                { 6,  -1726.22f, 1354.22f, 19.8217f },
                { 7,  -1709.06f, 1348.53f, 19.7823f }
            }, WaypointMoveType::Run);
            return path;
        }

        static WaypointPath const& GetPathPart2()
        {
            static WaypointPath path(PATH_ID_PART2, {
                { 8,  -1676.16f, 1346.19f, 15.1349f },
                { 9,  -1664.81f, 1345.01f, 15.4850f },
                { 10, -1662.81f, 1354.51f, 15.4850f },
                { 11, -1667.31f, 1362.51f, 15.4850f },
                { 12, -1674.31f, 1363.76f, 15.4850f },
                { 13, -1686.06f, 1355.01f, 15.4850f },
                { 14, -1691.06f, 1347.26f, 15.4850f },
                { 15, -1705.81f, 1350.01f, 19.9850f },
                { 16, -1731.31f, 1360.01f, 19.9850f },
                { 17, -1744.81f, 1370.51f, 20.2350f },
                { 18, -1758.81f, 1389.51f, 19.9850f },
                { 19, -1768.31f, 1410.01f, 19.9850f },
                { 20, -1771.56f, 1423.01f, 19.9850f },
                { 21, -1770.95f, 1430.33f, 19.8351f }
            }, WaypointMoveType::Run);
            return path;
        }

        struct npc_king_greymanes_horse_35905AI : public NullCreatureAI
        {
            npc_king_greymanes_horse_35905AI(Creature* creature) : NullCreatureAI(creature) { }

            EventMap m_events;
            ObjectGuid m_playerGUID;
            ObjectGuid m_krennanHorseGUID;

            void Reset() override
            {
                m_events.Reset();
                m_playerGUID = ObjectGuid();
                m_krennanHorseGUID = ObjectGuid();
            }

            void WaypointReached(uint32 nodeId, uint32 /*pathId*/) override
            {
                switch (nodeId)
                {
                case 6:
                {
                    if (!m_krennanHorseGUID)
                        if (Creature* krennan = me->FindNearestCreature(NPC_KRENNAN_ARANAS_TREE, 100.0f, true))
                            if (Player* player = ObjectAccessor::GetPlayer(*me, m_playerGUID))
                                krennan->AI()->Talk(0, player);
                    break;
                }
                case 7:
                {
                    m_events.ScheduleEvent(EVENT_JUMP_TO_KRENNAN, 1ms);
                    break;
                }
                case 21:
                {
                    m_events.CancelEvent(EVENT_FORCE_WORGEN_ATTACK);
                    if (Player* player = ObjectAccessor::GetPlayer(*me, m_playerGUID))
                    {
                        if (Creature* krennan = ObjectAccessor::GetCreature(*me, m_krennanHorseGUID))
                        {
                            krennan->AI()->Talk(0, player);
                            krennan->SetDisableGravity(false);
                            krennan->SetHover(false);
                            krennan->DespawnOrUnsummon(5s);
                        }
                        player->RemoveUnitFlag(UNIT_FLAG_IMMUNE_TO_NPC);

                        player->ExitVehicle();
                    }
                    me->DespawnOrUnsummon(5s);
                    break;
                }
                }
            }

            void PassengerBoarded(Unit* who, int8 /*seatId*/, bool apply) override
            {
                if (apply)
                {
                    if (Player* player = who->ToPlayer())
                    {
                        if (!m_playerGUID)
                        {
                            m_playerGUID = player->GetGUID();
                            player->SetClientControl(me, false);
                            player->SetUnitFlag(UNIT_FLAG_IMMUNE_TO_NPC);
                            me->SetFaction(player->GetFaction());
                            me->SetReactState(REACT_PASSIVE);
                            player->AddAura(SPELL_GENERIC_QUEST_INVISIBILITY_DETECTION_1, player);
                            m_events.ScheduleEvent(EVENT_START_PATH, 500ms);
                        }
                    }
                    else if (who->GetEntry() == NPC_KRENNAN_ARANAS && !m_krennanHorseGUID)
                    {
                        m_krennanHorseGUID = who->GetGUID();
                        who->SetUnitFlag(UNIT_FLAG_IMMUNE_TO_NPC);
                        m_events.ScheduleEvent(EVENT_RESUME_PATH, 1ms);
                    }
                }
            }

            void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damagetype*/, SpellInfo const* /*spellInfo*/) override
            {
                damage = 0;
            }

            void JustDied(Unit* /*killer*/) override
            {
                if (Player* player = ObjectAccessor::GetPlayer(*me, m_playerGUID))
                {
                    player->FailQuest(QUEST_SAVE_KRENNAN_ARANAS);
                    player->RemoveAura(SPELL_GENERIC_QUEST_INVISIBILITY_DETECTION_1);
                    player->RemoveUnitFlag(UNIT_FLAG_IMMUNE_TO_NPC);
                }
            }

            void UpdateAI(uint32 diff) override
            {
                m_events.Update(diff);

                while (uint32 eventId = m_events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case EVENT_JUMP_TO_KRENNAN:
                    {
                        me->GetMotionMaster()->MoveJump(EVENT_JUMP_TO_KRENNAN, Position(-1676.16f, 1346.19f, 15.1349f), 25.0f, 10.0f);
                        break;
                    }
                    case EVENT_RESUME_PATH:
                    {
                        me->GetMotionMaster()->MovePath(GetPathPart2(), false);
                        break;
                    }
                    case EVENT_START_PATH:
                    {
                        me->ClearUnitState(UNIT_STATE_ROOT);
                        me->RemoveUnitMovementFlag(MOVEMENTFLAG_ROOT);
                        me->GetMotionMaster()->MovePath(GetPathPart1(), false);
                        m_events.ScheduleEvent(EVENT_FORCE_WORGEN_ATTACK, 2s);
                        break;
                    }
                    case EVENT_FORCE_WORGEN_ATTACK:
                    {
                        std::list<Creature*> worgens;
                        me->GetCreatureListWithEntryInGrid(worgens, NPC_BLOODFANG_RIPPER_35505, 30.0f);
                        for (Creature* worgen : worgens)
                        {
                            if (!worgen->IsInCombat() && worgen->IsAlive())
                                worgen->AI()->AttackStart(me);
                        }
                        m_events.Repeat(2s);
                        break;
                    }
                    }
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const override
        {
            return new npc_king_greymanes_horse_35905AI(creature);
        }
};

// 35907
class npc_krennan_aranas_35907 : public CreatureScript
{
public:
    npc_krennan_aranas_35907() : CreatureScript("npc_krennan_aranas_35907") { }

    struct npc_krennan_aranas_35907AI : public ScriptedAI
    {
        npc_krennan_aranas_35907AI(Creature* creature) : ScriptedAI(creature) { }

        void IsSummonedBy(WorldObject* /*summoner*/) override
        {
            if (Creature* horse = me->FindNearestCreature(NPC_GRAYMANE_HORSE_35905, 20.0f))
                DoCast(horse, SPELL_RIDE_BUNNY_SEAT2, true);
        }

        void DoAction(int32 /*param*/) override
        {
            me->DespawnOrUnsummon(8000ms);
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_krennan_aranas_35907AI(creature);
    }
};

// 35914
class npc_commandeered_cannon_35914 : public CreatureScript
{
public:
    npc_commandeered_cannon_35914() : CreatureScript("npc_commandeered_cannon_35914") { }

    enum eNpc
    {
        ACTION_STARTING_EVENT = 101,
        EVENT_STARTING_EVENT,
        EVENT_STARTING_DO_FIRE,
    };

    struct npc_commandeered_cannon_35914AI : public ScriptedAI
    {
        npc_commandeered_cannon_35914AI(Creature* creature) : ScriptedAI(creature) {}

        EventMap m_events;

        void Reset() override
        {
            m_events.Reset();
        }

        void JustSummoned(Creature* summon) override
        {
            summon->SetUnitFlag(UNIT_FLAG_REMOVE_CLIENT_CONTROL);
        }

        void DoAction(int32 param) override
        {
            switch (param)
            {
                case ACTION_STARTING_EVENT:
                {
                    m_events.ScheduleEvent(EVENT_STARTING_EVENT, 25ms);
                }
            }
        }

        void UpdateAI(uint32 diff) override
        {
            m_events.Update(diff);

            while (uint32 eventId = m_events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_STARTING_EVENT:
                {
                    for (int i = 0; i < 12; i++)
                    {
                        me->SummonCreature(NPC_BLOODFANG_WORGEN_35118, -1757.65f + irand(-6, 6), 1384.01f + irand(-6, 6), 19.872f, urand(0, 6), TEMPSUMMON_TIMED_DESPAWN, 5000ms);
                    }
                    m_events.ScheduleEvent(EVENT_STARTING_DO_FIRE, 400ms);
                    break;
                }
                case EVENT_STARTING_DO_FIRE:
                {
                    if (Creature* Worgen = me->FindNearestCreature(NPC_BLOODFANG_WORGEN_35118, 50.0f, true))
                        me->CastSpell(Worgen, SPELL_CANNON_FIRE, true);
                    break;
                }
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_commandeered_cannon_35914AI(creature);
    }
};

// 35906
class npc_lord_godfrey_35906 : public CreatureScript
{
public:
    npc_lord_godfrey_35906() : CreatureScript("npc_lord_godfrey_35906") { }

    enum eNpc
    {
        SAY_LORD_GODFREY_P4 = 0,
        ACTION_STARTING_EVENT = 101,
    };

    struct npc_lord_godfrey_35906AI : public ScriptedAI
    {
        npc_lord_godfrey_35906AI(Creature* creature) : ScriptedAI(creature) {}

        void OnQuestReward(Player* player, Quest const* quest, LootItemType /*type*/, uint32 /*opt*/) override
        {
            if (quest->GetQuestId() == QUEST_SAVE_KRENNAN_ARANAS)
            {
                Talk(SAY_LORD_GODFREY_P4);
                player->RemoveAura(SPELL_WORGEN_BITE);
                me->AddAura(SPELL_INFECTED_BITE, player);
                me->AddAura(SPELL_PHASE_QUEST_ZONE_SPECIFIC_03, player);
                me->CastSpell(player, SPELL_GILNEAS_CANNON_CAMERA);
                player->SaveToDB();
                if (Creature* cannon = GetClosestCreatureWithEntry(me, NPC_COMMANDEERED_CANNON, 50.0f))
                    cannon->GetAI()->DoAction(ACTION_STARTING_EVENT);
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_lord_godfrey_35906AI(creature);
    }
};

/* Phase 8 - START */

// 35552
class npc_lord_darius_crowley_35552 : public CreatureScript
{
public:
    npc_lord_darius_crowley_35552() : CreatureScript("npc_lord_darius_crowley_35552") { }

    bool OnQuestAccept(Player* player, Creature* /*creature*/, Quest const* quest) override
    {
        if (quest->GetQuestId() == QUEST_SACRIFICES)
        {
            player->CastSpell(player, SPELL_SUMMON_CROWLEY_HORSE);
            return true;
        }
        return false;
    }

    struct npc_lord_darius_crowley_35552AI : public ScriptedAI
    {
        npc_lord_darius_crowley_35552AI(Creature* creature) : ScriptedAI(creature) { Initialize(); }

        EventMap m_events;
        ObjectGuid   m_kingGUID;
        ObjectGuid   m_princeGUID;
        ObjectGuid   m_playerGUID;
        bool     m_AnimRunning;
        std::set<ObjectGuid> pList;

        void Initialize()
        {
            m_kingGUID = ObjectGuid();
            m_princeGUID = ObjectGuid();
            m_playerGUID = ObjectGuid();
            m_AnimRunning = false;
        }

        void MoveInLineOfSight(Unit* who) override
        {
            if (!m_AnimRunning)
                if (Player* player = who->ToPlayer())
                    if (me->GetDistance(player) < 30.0f)
                    if (!HasPlayer(player->GetGUID()))
                    if (player->GetQuestStatus(QUEST_TIME_TO_REGROUP) != QUEST_STATUS_COMPLETE)
                    {
                        m_AnimRunning = true;
                        m_playerGUID = player->GetGUID();
                        m_events.RescheduleEvent(EVENT_START_TALK, 1000ms);
                    }
        }

        void UpdateAI(uint32 diff) override
        {
            m_events.Update(diff);

            while (uint32 eventId = m_events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_START_TALK:
                    {
                        GetTalkMember();
                        if (ObjectAccessor::GetPlayer(*me, m_playerGUID))
                            if (ObjectAccessor::GetCreature(*me, m_kingGUID))
                                if (ObjectAccessor::GetCreature(*me, m_princeGUID))
                                {
                                    m_events.ScheduleEvent(EVENT_TALK_PART_00, 1000ms);
                                    break;
                                }

                        m_AnimRunning = false;
                        break;
                    }
                    case EVENT_TALK_PART_00:
                    {
                        if (Player* player = ObjectAccessor::GetPlayer(*me, m_playerGUID))
                            if (Creature* king = ObjectAccessor::GetCreature(*me, m_kingGUID))
                                king->AI()->Talk(0, player);
                        m_events.ScheduleEvent(EVENT_TALK_PART_01, 9000ms);
                        break;
                    }
                    case EVENT_TALK_PART_01:
                    {
                        if (Player* player = ObjectAccessor::GetPlayer(*me, m_playerGUID))
                            Talk(0, player);
                        m_events.ScheduleEvent(EVENT_TALK_PART_02, 9000ms);
                        break;
                    }
                    case EVENT_TALK_PART_02:
                    {
                        if (Player* player = ObjectAccessor::GetPlayer(*me, m_playerGUID))
                            if (Creature* prince = ObjectAccessor::GetCreature(*me, m_princeGUID))
                                prince->AI()->Talk(0, player);
                        m_events.ScheduleEvent(EVENT_TALK_PART_03, 6000ms);
                        break;
                    }
                    case EVENT_TALK_PART_03:
                    {
                        if (Player* player = ObjectAccessor::GetPlayer(*me, m_playerGUID))
                            Talk(1, player);
                        m_events.ScheduleEvent(EVENT_TALK_PART_04, 9000ms);
                        break;
                    }
                    case EVENT_TALK_PART_04:
                    {
                        if (Player* player = ObjectAccessor::GetPlayer(*me, m_playerGUID))
                            Talk(2, player);
                        m_events.ScheduleEvent(EVENT_TALK_PART_05, 8000ms);
                        break;
                    }
                    case EVENT_TALK_PART_05:
                    {
                        if (Player* player = ObjectAccessor::GetPlayer(*me, m_playerGUID))
                            if (Creature* king = ObjectAccessor::GetCreature(*me, m_kingGUID))
                                king->AI()->Talk(2, player);
                        m_events.RescheduleEvent(EVENT_TALK_PART_06, 30000ms, 45000ms);
                        break;
                    }
                    case EVENT_TALK_PART_06:
                    {
                        AddPlayer();
                        m_AnimRunning = false;
                        m_playerGUID = ObjectGuid();
                        break;
                    }
                }
            }

            if (!UpdateVictim())
                return;
        }

        void AddPlayer()
        {
            if (!HasPlayer(m_playerGUID))
                pList.insert(m_playerGUID);
        }

        bool HasPlayer(ObjectGuid guid)
        {
            return (pList.find(guid) != pList.end());
        }

        void GetTalkMember()
        {
            if (!m_kingGUID)
                if (Creature* king = me->FindNearestCreature(NPC_KING_GREYMANE, 15.0f))
                    m_kingGUID = king->GetGUID();
            if (!m_princeGUID)
                if (Creature* prince = me->FindNearestCreature(NPC_PRINCE_GREYMANE, 15.0f))
                    m_princeGUID = prince->GetGUID();
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_lord_darius_crowley_35552AI(creature);
    }
};

/* start quest Sacrifices */

// 35231
// Vehicle seat has VEHICLE_SEAT_FLAG_CAN_CONTROL, so VehicleJoinEvent::Execute calls
// SetCharmedBy (CHARM_TYPE_VEHICLE), which normally replaces our AI with PossessedAI.
// We override OnCharmed to keep our AI active, so MovementInform/WaypointReached work.
// The charm is kept alive so the vehicle action bar remains visible for the player.
// When the horse despawns, Vehicle::RemovePassenger detects VEHICLE_SEAT_FLAG_CAN_CONTROL
// and calls RemoveCharmedBy, which properly cleans up m_playerMovingMe (avoids crash).
class npc_crowley_horse_35231 : public CreatureScript
{
public:
    npc_crowley_horse_35231() : CreatureScript("npc_crowley_horse_35231") {}

    enum eHorse
    {
        MOVE_TO_START = 1031,
        MOVE_TO_JUMP2 = 1032,
    };

    static WaypointPath const& GetPathRun1()
    {
        static WaypointPath path(352310, {
            { 0,  -1686.12f, 1655.64f, 20.6106f },
            { 1,  -1668.16f, 1639.43f, 20.6106f },
            { 2,  -1666.63f, 1628.86f, 20.5327f },
            { 3,  -1670.76f, 1617.43f, 20.6106f },
            { 4,  -1693.65f, 1592.64f, 20.6106f },
            { 5,  -1708.02f, 1590.55f, 20.6104f },
            { 6,  -1719.37f, 1595.65f, 20.6104f },
            { 7,  -1720.57f, 1610.14f, 20.6074f },
            { 8,  -1711.90f, 1629.70f, 20.6074f },
            { 9,  -1698.15f, 1656.48f, 20.6106f },
            { 10, -1696.42f, 1682.01f, 20.5262f },
            { 11, -1684.68f, 1700.12f, 20.4648f },
            { 12, -1658.31f, 1710.31f, 20.5898f },
            { 13, -1625.48f, 1712.30f, 21.8051f },
        }, WaypointMoveType::Run);
        return path;
    }

    static WaypointPath const& GetPathRun2()
    {
        static WaypointPath path(352311, {
            { 0,  -1545.08f, 1684.23f, 20.6099f },
            { 1,  -1542.79f, 1656.08f, 20.6099f },
            { 2,  -1543.00f, 1640.54f, 20.6099f },
            { 3,  -1520.73f, 1618.10f, 20.6105f },
            { 4,  -1498.79f, 1621.74f, 20.6105f },
            { 5,  -1468.36f, 1630.78f, 20.6105f },
            { 6,  -1441.65f, 1627.67f, 20.6105f },
            { 7,  -1429.84f, 1616.63f, 20.5898f },
            { 8,  -1417.99f, 1586.55f, 20.5898f },
            { 9,  -1429.18f, 1548.78f, 20.8485f },
            { 10, -1447.42f, 1532.79f, 20.6010f },
            { 11, -1463.10f, 1535.03f, 20.6105f },
            { 12, -1483.04f, 1544.15f, 20.6105f },
            { 13, -1505.16f, 1568.61f, 20.6105f },
            { 14, -1531.60f, 1581.87f, 26.5387f },
            { 15, -1540.59f, 1574.43f, 29.2067f },
        }, WaypointMoveType::Run);
        return path;
    }

    struct npc_crowley_horse_35231AI : public NullCreatureAI
    {
        npc_crowley_horse_35231AI(Creature* creature) : NullCreatureAI(creature) { }

        ObjectGuid m_playerGUID;
        ObjectGuid m_dariusGUID;
        uint8 m_phase;

        void Reset() override
        {
            m_playerGUID = ObjectGuid();
            m_dariusGUID = ObjectGuid();
            m_phase = 0;
            me->SetUnitFlag(UNIT_FLAG_IMMUNE_TO_NPC);
        }

        void OnCharmed(bool /*apply*/) override
        {
        }

        void IsSummonedBy(WorldObject* summoner) override
        {
            if (Player* player = summoner->ToPlayer())
                m_playerGUID = player->GetGUID();

            me->m_Events.AddEventAtOffset([this]() {
                if (Vehicle* vehicle = me->GetVehicleKit())
                    for (auto const& [seatId, seat] : vehicle->Seats)
                        if (Unit* passenger = ObjectAccessor::GetUnit(*me, seat.Passenger.Guid))
                            if (passenger->GetEntry() == NPC_DARIUS_CROWLEY)
                            {
                                m_dariusGUID = passenger->GetGUID();
                                passenger->ToCreature()->SetUnitFlag(UNIT_FLAG_IMMUNE_TO_NPC);
                            }

                me->m_Events.AddEventAtOffset([this]() {
                    m_phase = 1;
                    me->ClearUnitState(UNIT_STATE_ROOT);
                    me->RemoveUnitMovementFlag(MOVEMENTFLAG_ROOT);
                    me->GetMotionMaster()->MovePoint(MOVE_TO_START, -1735.01f, 1653.01f, 20.49f);
                }, 1s);
            }, 1s);
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type == POINT_MOTION_TYPE)
            {
                switch (id)
                {
                case MOVE_TO_START:
                    m_phase = 2;
                    me->GetMotionMaster()->MoveJump(EVENT_JUMP, Position(-1714.02f, 1666.37f, 20.57f), 25.0f, 15.0f);
                    break;
                case MOVE_TO_JUMP2:
                    m_phase = 5;
                    me->GetMotionMaster()->MoveJump(EVENT_JUMP, Position(-1571.1566f, 1709.6495f, 21.8051f), 25.0f, 15.0f);
                    break;
                }
            }
            else if (type == EFFECT_MOTION_TYPE && id == EVENT_JUMP)
            {
                switch (m_phase)
                {
                case 2:
                    m_phase = 3;
                    me->GetMotionMaster()->MovePath(GetPathRun1(), false);
                    me->m_Events.AddEventAtOffset([this]() {
                        if (Creature* darius = ObjectAccessor::GetCreature(*me, m_dariusGUID))
                            if (Player* player = ObjectAccessor::GetPlayer(*me, m_playerGUID))
                                darius->AI()->Talk(0, player);
                        SchedulePeriodicTalk(10s);
                    }, 5s);
                    break;
                case 5:
                    m_phase = 6;
                    me->GetMotionMaster()->MovePath(GetPathRun2(), false);
                    break;
                }
            }
        }

        void WaypointReached(uint32 nodeId, uint32 /*pathId*/) override
        {
            if (m_phase == 3 && nodeId == 13)
            {
                m_phase = 4;
                me->GetMotionMaster()->MovePoint(MOVE_TO_JUMP2, -1592.09f, 1710.71f, 20.5899f);
            }
            else if (m_phase == 6 && nodeId == 15)
            {
                me->m_Events.AddEventAtOffset([this]() {
                    if (Player* player = ObjectAccessor::GetPlayer(*me, m_playerGUID))
                        if (player->GetQuestStatus(QUEST_SACRIFICES) != QUEST_STATUS_COMPLETE)
                        {
                            player->FailQuest(QUEST_SACRIFICES);
                            player->NearTeleportTo(-1739.2f, 1657.9f, 20.48f, 0.5225f);
                        }
                    me->DespawnOrUnsummon(10s);
                }, 100ms);
            }
        }

        void JustDied(Unit* /*killer*/) override
        {
            if (Player* player = ObjectAccessor::GetPlayer(*me, m_playerGUID))
                if (player->GetQuestStatus(QUEST_SACRIFICES) == QUEST_STATUS_INCOMPLETE)
                {
                    player->FailQuest(QUEST_SACRIFICES);
                    player->NearTeleportTo(-1739.2f, 1657.9f, 20.48f, 0.5225f);
                }
        }

        void SchedulePeriodicTalk(Milliseconds delay)
        {
            me->m_Events.AddEventAtOffset([this]() {
                if (m_phase <= 6 && me->IsAlive())
                {
                    if (Creature* darius = ObjectAccessor::GetCreature(*me, m_dariusGUID))
                        if (Player* player = ObjectAccessor::GetPlayer(*me, m_playerGUID))
                            darius->AI()->Talk(urand(1,3), player);
                    SchedulePeriodicTalk(randtime(15s, 30s));
                }
            }, delay);
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_crowley_horse_35231AI(creature);
    }
};

// 67063
class spell_throw_torch_67063 : public SpellScriptLoader
{
public:
    spell_throw_torch_67063() : SpellScriptLoader("spell_throw_torch_67063") { }

    class FriendlyCheck
    {
    public:
        FriendlyCheck(Unit* caster) : _caster(caster) { }

        bool operator()(WorldObject* object) const
        {
            if (Unit* unit = object->ToUnit())
                return !unit->IsHostileTo(_caster);
            return true;
        }

    private:
        Unit* _caster;
    };

    class IsNotStalker
    {
    public:
        IsNotStalker() {}

        bool operator()(WorldObject* object) const
        {
            if (Unit* unit = object->ToUnit())
                return unit->GetEntry() != NPC_BLOODFANG_STALKER;
            return true;
        }
    };

    class spell_throw_torch_67063_SpellScript : public SpellScript
    {

        void FilterTargets(std::list<WorldObject*>& targets)
        {
            targets.remove_if(IsNotStalker()); // (FriendlyCheck(GetCaster()));
            uint8 rol = urand(3, 8);
            while (targets.size() > rol)
                targets.erase(targets.begin());
        }

        void Register() override
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_throw_torch_67063_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_DEST_AREA_ENTRY);
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_throw_torch_67063_SpellScript::FilterTargets, EFFECT_1, TARGET_UNIT_DEST_AREA_ENTRY);
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_throw_torch_67063_SpellScript::FilterTargets, EFFECT_2, TARGET_UNIT_DEST_AREA_ENTRY);
        }
    };

    SpellScript* GetSpellScript() const override
    {
        return new spell_throw_torch_67063_SpellScript();
    }
};

// 35229
class npc_bloodfang_stalker_35229 : public CreatureScript
{
public:
    npc_bloodfang_stalker_35229() : CreatureScript("npc_bloodfang_stalker_35229") {}

    enum eNpc
    {
        COOLDOWN_ENRAGE = 30000,
        SOUND_WORGEN_ATTACK = 558,
        EVENT_ENRAGE_COOLDOWN = 101,
        EVENT_CHECK_CROWLEY,
    };

    struct npc_bloodfang_stalker_35229AI : public ScriptedAI
    {
        npc_bloodfang_stalker_35229AI(Creature* creature) : ScriptedAI(creature) {}

        EventMap m_events;
        bool m_enrage;
        bool m_isFollowing;
        bool m_isTalking;

        void Reset() override
        {
            me->SetUnitFlag(UNIT_FLAG_IMMUNE_TO_NPC);
            m_enrage = false;
            m_isFollowing = false;
            m_isTalking = false;
        }

        void MoveInLineOfSight(Unit* who) override
        {
            if (!m_isFollowing)
                if (Creature* horse = who->ToCreature())
                    if (horse->GetEntry() == NPC_CROWLEYS_HORSE)
                        if (horse->GetDistance(me) < 15.0f)
                            if (urand(0, 100) < 30)
                            {
                                me->SetWalk(true);
                                me->GetMotionMaster()->MoveFollow(horse, frand(8.0f, 15.0f), frand(2.8f, 3.3f));
                                m_isFollowing = true;
                                m_events.ScheduleEvent(EVENT_GIVE_UP, 4000ms, 10000ms);
                            }
        }

        void DamageTaken(Unit* /*who*/, uint32& /*damage*/, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
        {
            if (!m_enrage && me->GetHealthPct() < 90.0f)
            {
                me->CastSpell(me, SPELL_ENRAGE_8599);
                m_enrage = true;
                m_events.ScheduleEvent(EVENT_ENRAGE_COOLDOWN, 60000ms);
            }
            if (!m_isTalking)
                if (urand(0, 100) < 30)
                {
                    Talk(1);
                    m_isTalking = true;
                }
        }

        void SpellHit(WorldObject* caster, const SpellInfo* spell) override
        {
            if (spell->Id == SPELL_THROW_TORCH)
                if (Player* player = caster->ToPlayer())
                    if (player->GetQuestStatus(QUEST_SACRIFICES) == QUEST_STATUS_INCOMPLETE)
                    {
                        caster->ToPlayer()->KilledMonsterCredit(NPC_BLOODFANG_STALKER_CREDIT);
                        me->DespawnOrUnsummon(3000ms);
                    }
        }

        void UpdateAI(uint32 diff) override
        {
            m_events.Update(diff);

            while (uint32 eventId = m_events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_ENRAGE_COOLDOWN:
                {
                    m_enrage = false;
                    break;
                }
                case EVENT_GIVE_UP:
                {
                    me->GetMotionMaster()->MoveTargetedHome();
                    m_isFollowing = false;
                    break;
                }
                }
            }

            if (!UpdateVictim())
                return;
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_bloodfang_stalker_35229AI(creature);
    }
};

/* end quest Sacrifices */

// 35317
class npc_rebel_cannon_35317 : public CreatureScript
{
public:
    npc_rebel_cannon_35317() : CreatureScript("npc_rebel_cannon_35317") { }

    enum eNPC
    {
        EVENT_CHECK_STALKER = 102,
    };

    struct npc_rebel_cannon_35317AI : public ScriptedAI
    {
        npc_rebel_cannon_35317AI(Creature* creature) : ScriptedAI(creature) {}

        EventMap m_events;

        void Reset() override
        {
            m_events.ScheduleEvent(EVENT_CHECK_STALKER, 1000ms);
        }

        void UpdateAI(uint32 diff) override
        {
            m_events.Update(diff);

            while (uint32 eventId = m_events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_CHECK_STALKER:
                {
                    std::list<Creature*> cList = me->FindNearestCreatures(NPC_BLOODFANG_STALKER, 4.0f);
                    for (std::list<Creature*>::const_iterator itr = cList.begin(); itr != cList.end(); ++itr)
                    {
                        if (Creature* stalker = *itr)
                            stalker->DespawnOrUnsummon(25ms);
                    }
                    m_events.ScheduleEvent(EVENT_CHECK_STALKER, 1000ms);
                    break;
                }
                }
            }

            if (!UpdateVictim())
                return;
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_rebel_cannon_35317AI(creature);
    }
};

// 36057 Northgate Rebel
class npc_northgate_rebel_36057 : public CreatureScript
{
public:
    npc_northgate_rebel_36057() : CreatureScript("npc_northgate_rebel_36057") { }

    enum eNPC
    {
        EVENT_CHECK_SHOWFIGHT = 101,
        MOVE_TO_HOMEPOSITION,
    };

    struct npc_northgate_rebel_36057AI : public ScriptedAI
    {
        npc_northgate_rebel_36057AI(Creature* creature) : ScriptedAI(creature) { Initialize(); }

        EventMap m_events;
        float    m_minHealthPct;
        bool     m_isShowFight;
        uint32    m_minDamage;
        uint32    m_maxDamage;
        std::list<uint32> m_targetList;

        void Initialize()
        {
            m_targetList.clear();
            m_targetList.push_back(NPC_BLOODFANG_STALKER);
            m_minHealthPct = frand(30.0f, 85.0f);
            m_minDamage = 1;
            m_maxDamage = 1;
        }

        void Reset() override
        {
            m_events.Reset();
            m_isShowFight = true;
            m_events.ScheduleEvent(EVENT_CHECK_SHOWFIGHT, 1000ms);
        }

        void MovementInform(uint32 type, uint32 pointId) override
        {
            if (type == POINT_MOTION_TYPE && pointId == MOVE_TO_HOMEPOSITION)
                m_events.ScheduleEvent(EVENT_CHECK_SHOWFIGHT, 25ms);
        }

        uint32 FindTargetEntry(uint32 entry)
        {
            if (m_targetList.empty())
                return 0;

            for (std::list<uint32>::const_iterator itr = m_targetList.begin(); itr != m_targetList.end(); ++itr)
                if ((*itr) == entry)
                    return entry;

            return 0;
        }

        void DamageTaken(Unit* who, uint32 &damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
        {
            if (m_isShowFight && who->ToCreature())
                if (FindTargetEntry(who->GetEntry()))
                {
                    if (damage > m_maxDamage)
                        damage = m_maxDamage;

                    if (me->GetHealthPct() < m_minHealthPct)
                        damage = 0;
                }

        }

        void DamageDealt(Unit* victim, uint32& damage, DamageEffectType /*damageType*/) override
        {
            if (m_isShowFight && victim->ToCreature())
                if (FindTargetEntry(victim->GetEntry()))
                {
                    if (damage > m_maxDamage)
                        damage = m_maxDamage;

                    if (victim->GetHealthPct() < m_minHealthPct)
                        damage = m_minDamage;
                }
        }

        void UpdateAI(uint32 diff) override
        {
            m_events.Update(diff);

            while (uint32 eventId = m_events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_CHECK_SHOWFIGHT:
                {
                    if (me->IsInCombat() || !me->IsAlive())
                    {
                        m_events.ScheduleEvent(EVENT_CHECK_SHOWFIGHT, 2500ms);
                        return;
                    }

                    if (me->GetDistance2d(me->GetHomePosition().GetPositionX(), me->GetHomePosition().GetPositionY()) > 10.0f)
                    {
                        me->GetMotionMaster()->MovePoint(MOVE_TO_HOMEPOSITION, me->GetHomePosition());
                        return;
                    }

                    for (uint32 TargetEntry : m_targetList)
                    {
                        if (Creature* worgen = me->FindNearestCreature(TargetEntry, 5.0f))
                        {
                            me->Attack(worgen, true);
                            worgen->Attack(me, true);
                            m_events.ScheduleEvent(EVENT_CHECK_SHOWFIGHT, 2500ms);
                            return;
                        }
                    }

                    m_events.ScheduleEvent(EVENT_CHECK_SHOWFIGHT, 2500ms);
                    break;
                }
                }
            }

            if (!UpdateVictim())
                return;
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_northgate_rebel_36057AI(creature);
    }
};

// 35566
class npc_lord_darius_crowley_35566 : public CreatureScript
{
public:
    npc_lord_darius_crowley_35566() : CreatureScript("npc_lord_darius_crowley_35566") {}

    bool OnQuestAccept(Player* player, Creature* /*creature*/, Quest const* quest) override
    {
        if (quest->GetQuestId() == QUEST_LAST_STAND)
        {
            if (player->HasAura(SPELL_INFECTED_BITE))
                player->RemoveAura(SPELL_INFECTED_BITE);
            player->AddAura(SPELL_HIDEOUS_BITE_WOUND, player);
            return true;
        }
        return false;
    }

    struct npc_lord_darius_crowley_35566AI : public ScriptedAI
    {
        npc_lord_darius_crowley_35566AI(Creature* creature) : ScriptedAI(creature) {  }

        EventMap m_events;
        ObjectGuid   m_playerGUID;

        void OnQuestReward(Player* player, Quest const* quest, LootItemType /*type*/, uint32 /*opt*/) override
        {
            if (quest->GetQuestId() == QUEST_LAST_STAND)
            {
                player->RemoveAura(SPELL_HIDEOUS_BITE_WOUND);
                player->CastSpell(player, SPELL_ALTERED_FORM, true);
                player->CastSpell(player, SPELL_FORCE_WORGEN_ALTERED_FORM, true);
                player->CastSpell(player, SPELL_ALTERED_FORM2, true);
                SetGUID(player->GetGUID(), PLAYER_GUID);
                DoAction(EVENT_START_MOVIE);
            }
        }

        void Reset() override
        {
            m_playerGUID = ObjectGuid();
        }

        void SetGUID(ObjectGuid const& guid, int32 id) override
        {
            switch (id)
            {
                case PLAYER_GUID:
                {
                    m_playerGUID = guid;
                    break;
                }
            }
        }

        void DoAction(int32 param) override
        {
            switch (param)
            {
                case EVENT_START_MOVIE:
                {
                    if (Player* player = ObjectAccessor::GetPlayer(*me, m_playerGUID))
                        player->CastSpell(player, SPELL_CURSE_OF_THE_WORGEN, true);
                    m_events.RescheduleEvent(EVENT_START_MOVIE, 2000ms);
                    break;
                }
            }
        }

        void UpdateAI(uint32 diff) override
        {
            m_events.Update(diff);

            while (uint32 eventId = m_events.ExecuteEvent())
            {
                switch (eventId)
                {

                case EVENT_START_MOVIE:
                {
                    if (Player* player = ObjectAccessor::GetPlayer(*me, m_playerGUID))
                    {
                        player->CastSpell(player, SPELL_LAST_STAND_COMPLETE, true);
                        player->AddAura(SPELL_PHASE_QUEST_ZONE_SPECIFIC_06, player);
                        player->RemoveAura(SPELL_PHASE_QUEST_ZONE_SPECIFIC_03);
                        player->RemoveAura(SPELL_PHASE_QUEST_ZONE_SPECIFIC_02);
                        player->SaveToDB();

                    }
                    break;
                }
                }
            }

            if (!UpdateVictim())
                return;
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_lord_darius_crowley_35566AI(creature);
    }
};

class player_gilneas_city1_reapply_auras : public PlayerScript
{
public:
    player_gilneas_city1_reapply_auras() : PlayerScript("player_gilneas_city1_reapply_auras") {}

    void OnLogin(Player* player, bool /*loginFirst*/) override
    {
        if (player->GetQuestStatus(QUEST_THE_REBEL_LORDS_ARSENAL) == QUEST_STATUS_REWARDED)
        {
            if (player->GetQuestStatus(QUEST_SAVE_KRENNAN_ARANAS) == QUEST_STATUS_REWARDED)
            {
                if (player->GetQuestStatus(QUEST_LAST_STAND) == QUEST_STATUS_REWARDED)
                    return;

                if (player->IsActiveQuest(QUEST_LAST_STAND))
                    player->AddAura(SPELL_HIDEOUS_BITE_WOUND, player);
                else
                {
                    player->AddAura(SPELL_INFECTED_BITE, player);
                    player->AddAura(SPELL_PHASE_QUEST_ZONE_SPECIFIC_03, player);
                }
            }
            else
            {
                player->AddAura(SPELL_WORGEN_BITE, player);
                player->AddAura(SPELL_PHASE_QUEST_ZONE_SPECIFIC_02, player);
            }
        }
    }
};

// next part in script zone_duskhaven

void AddSC_zone_gilneas_chapter_1()
{
    new npc_gilnean_crow();
    new npc_gwen_armstead_35840();

    //QUEST: 14154
    new npc_lord_darius_crowley_35077();
    new npc_tobias_mistmantle_35124();
    new npc_worgen_runt_35188();
    new npc_worgen_alpha_35170();
    new npc_worgen_runt_35456();
    new npc_worgen_alpha_35167();

    new npc_king_greymane_35112();

    //QUEST: 14159
    new npc_josiah_avery_35369();
    new npc_josiah_avery_35370();
    new npc_josiah_avery_trigger_50415();

    //QUEST: 14204
    new npc_lorna_crowley_35378();
    new npc_bloodfang_lurker_35463();
    new npc_gilnean_mastiff_35631();

    //QUEST: 14293
    new npc_lord_godfrey_35906();
    new npc_gilnean_city_guard_35504();
    new npc_king_genn_greymane_35550();
    new npc_king_greymanes_horse_35905();
    new npc_krennan_aranas_35907();
    new spell_rescue_krennan_68219();
    new npc_commandeered_cannon_35914();
    new npc_bloodfang_stalker_35229();

    //QUEST: 14212
    new npc_lord_darius_crowley_35552();
    new npc_crowley_horse_35231();
    new spell_throw_torch_67063();

    //QUEST: 14218
    new npc_rebel_cannon_35317();
    new npc_northgate_rebel_36057();

    //QUEST
    new npc_lord_darius_crowley_35566();
    new player_gilneas_city1_reapply_auras();
};

