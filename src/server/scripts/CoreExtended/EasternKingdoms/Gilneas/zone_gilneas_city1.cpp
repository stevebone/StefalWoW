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
#include "PhasingHandler.h"
#include "zone_gilneas.h"

enum eZoneGilneas
{
    GO_MERCHANT_DOOR                             = 195327,

    NPC_PANICKED_CITIZEN                         = 34851,
    NPC_RAMPAGING_WORGEN_34884                   = 34884,
    NPC_PRINCE_LIAM                              = 34913,
    NPC_FRIGHTENED_CITIZEN_34981                 = 34981,
    NPC_KING_GREYMANE_35112                      = 35112,
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
    NPC_BLOODFANG_LURKER                         = 35463,
    NPC_BLOODFANG_RIPPER_35505                   = 35505,
    NPC_PRINCE_GREYMANE                          = 35551,
    NPC_LORD_DARIUS_CROWLEY                      = 35552,
    NPC_BLOODFANG_STALKER_CREDIT                 = 35582,
    NPC_TOBIAS_MISTMANTLE                        = 35618,
    NPC_FRENZIED_STALKER                         = 35627,
    NPC_GILNEAN_MASTIFF                          = 35631,
    NPC_RAMPAGING_WORGEN_35660                   = 35660,
    NPC_KRENNAN_ARANAS_TREE                      = 35753,
    NPC_GILNEAS_EVACUATION_FACING_MARKER_35830   = 35830,
    NPC_FRIGHTENED_CITIZEN_WORGEN                = 35836,
    NPC_GRAYMANE_HORSE_35905                     = 35905,
    NPC_KRENNAN_ARANAS                           = 35907,
    NPC_KING_GREYMANE                            = 35911,
    NPC_COMMANDEERED_CANNON                      = 35914,
    NPC_NORTHGATE_REBEL_1                        = 36057,
    NPC_PANICKED_CITIZEN_GATE                    = 44086,
    NPC_HEALING_CREDIT                           = 47091,
    NPC_JOSAIH_AVERY_TRIGGER                     = 50415,

    QUEST_LOCKDOWN                               = 14078,
    QUEST_THE_REBEL_LORDS_ARSENAL                = 14159,
    QUEST_FROM_THE_SHADOWS                       = 14204,
    QUEST_SACRIFICES                             = 14212,
    QUEST_LAST_STAND                             = 14222,
    QUEST_FLASH_HEAL                             = 14279,
    QUEST_A_REJUVENATING_TOUCH                   = 14283,
    QUEST_SAVE_KRENNAN_ARANAS                    = 14293,

    SPELL_RENEW                                  = 139,
    SPELL_SHADOWSTALKER_STEALTH                  = 5916,
    SPELL_SHOOT                                  = 6660,
    SPELL_ENRAGE_8599                            = 8599,
    SPELL_ENRAGE_56646                           = 56646,
    SPELL_BY_THE_SKIN                            = 66914,
    SPELL_THROW_TORCH                            = 67063,
    SPELL_GET_SHOT                               = 67349,
    SPELL_SUMMON_JOSIAH_AVERY                    = 67350,
    SPELL_FORCE_CAST_SUMMON_JOSIAH               = 67352,
    SPELL_WORGEN_ATTACK_KNOCKBACK                = 42880,
    SPELL_INSTAKILL_LORNA                        = 67593,
    SPELL_ATTACK_LURKER                          = 67805,
    SPELL_SUMMON_GILNEAN_MASTIFF                 = 67807,
    SPELL_RESCUE_KRENNAN                         = 68219,
    SPELL_FORCECAST_SUMMON_GRAYMANE_HORSE        = 68232,
    SPELL_CANNON_FIRE                            = 68235,
    SPELL_CURSE_OF_THE_WORGEN                    = 68630,
    SPELL_TWO_FORMS                              = 68996,
    SPELL_IN_STOCKS                              = 69196,
    SPELL_COSMETIC_COMBAT_ATTACK                 = 69873,
    SPELL_TRANSFORMING                           = 72794,
    SPELL_WORGEN_BITE                            = 72870,
    SPELL_INFECTED_BITE                          = 72872,
    SPELL_FROSTBOLT_VISUAL_ONLY                  = 74277,
    SPELL_HIDEOUS_BITE_WOUND                     = 76642,
    SPELL_RIDE_BUNNY_SEAT2                       = 84275,
    SPELL_GILNEAS_CANNON_CAMERA                  = 93555,
    SPELL_FADE_OF_BLACK                          = 94053,
    SPELL_ALTERED_FORM                           = 94293,
    SPELL_ALTERED_FORM2                          = 97709,
    SPELL_FORCE_WORGEN_ALTERED_FORM              = 98274,
    SPELL_SUMMON_CROWLEY_HORSE                   = 67001,
    ZONE_DUSKHAVEN                               = 4786,

    SPELL_GENERIC_QUEST_INVISIBILITY_DETECTION_1 = 49416,
    SPELL_PHASE_QUEST_ZONE_SPECIFIC_02           = 59074, // phase 171

};

// player
class player_zone_gilneas_city1 : public PlayerScript
{
public:
    player_zone_gilneas_city1() : PlayerScript("player_zone_gilneas_city1") { }

    void OnLogin(Player* player, bool /*firstLogin*/) override
    {
        if (player->GetQuestStatus(QUEST_THE_REBEL_LORDS_ARSENAL) == QUEST_STATUS_REWARDED)
        {
            if (!player->HasAura(SPELL_PHASE_QUEST_ZONE_SPECIFIC_02))
            {
                player->AddAura(SPELL_PHASE_QUEST_ZONE_SPECIFIC_02, player);
                player->AddAura(SPELL_WORGEN_BITE, player);
            }
        }
    }

    void OnQuestStatusChange(Player* player, uint32 questId) override
    {
        if (player->GetQuestStatus(questId) == QUEST_STATUS_INCOMPLETE ||
            player->GetQuestStatus(questId) == QUEST_STATUS_COMPLETE)
            return;

        switch (questId)
        {
            case QUEST_THE_REBEL_LORDS_ARSENAL:
                player->AddAura(SPELL_PHASE_QUEST_ZONE_SPECIFIC_02, player);
                break;
        }
    }
};

//gilnean crow 50260
class npc_gilnean_crow : public CreatureScript
{
public:
    npc_gilnean_crow() : CreatureScript("npc_gilnean_crow") { }

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
        npc_gilnean_crow50260AI(Creature* creature) : ScriptedAI(creature) { }

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

// 34864
class npc_gilneas_city_guard_gate_34864 : public CreatureScript
{
public:
    npc_gilneas_city_guard_gate_34864() : CreatureScript("npc_gilneas_city_guard_gate_34864") { }

    enum eNpc
    {
        EVENT_START_TALK_WITH_CITIZEN = 101,
        EVENT_TALK_WITH_CITIZEN_1,
        EVENT_TALK_WITH_CITIZEN_2,
        EVENT_TALK_WITH_CITIZEN_3,
    };

    struct npc_gilneas_city_guard_gate_34864AI : public ScriptedAI
    {
        npc_gilneas_city_guard_gate_34864AI(Creature* creature) : ScriptedAI(creature) { }

        EventMap m_events;
        uint8    m_say;
        Emote    m_emote;
        ObjectGuid     m_citicenGUID;

        void Reset() override
        {
            if (me->GetDistance2d(-1430.47f, 1345.55f) < 10.0f)
                m_events.ScheduleEvent(EVENT_START_TALK_WITH_CITIZEN, 10s, 30s);
        }

        void UpdateAI(uint32 diff) override
        {
            m_events.Update(diff);

            while (uint32 eventId = m_events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_START_TALK_WITH_CITIZEN:
                    {
                        m_citicenGUID = GetRandomCitizen();
                        m_emote = RAND(EMOTE_STATE_COWER, EMOTE_STATE_TALK, EMOTE_ONESHOT_CRY, EMOTE_STATE_SPELL_PRECAST, EMOTE_STATE_EXCLAIM);
                        m_say = urand(0, 2);
                        if (Creature* npc = ObjectAccessor::GetCreature(*me, m_citicenGUID))
                            npc->HandleEmoteCommand(m_emote);
                        m_events.ScheduleEvent(EVENT_TALK_WITH_CITIZEN_1, 1200ms, 2s);
                        break;
                    }
                    case EVENT_TALK_WITH_CITIZEN_1:
                    {
                        if (Creature* npc = ObjectAccessor::GetCreature(*me, m_citicenGUID))
                            npc->AI()->Talk(m_say);
                        m_events.ScheduleEvent(EVENT_TALK_WITH_CITIZEN_2, 5s);
                        break;
                    }
                    case EVENT_TALK_WITH_CITIZEN_2:
                    {
                        Talk(m_say);
                        m_events.ScheduleEvent(EVENT_TALK_WITH_CITIZEN_3, 5s);
                        break;
                    }
                    case EVENT_TALK_WITH_CITIZEN_3:
                    {
                        if (Creature* npc = ObjectAccessor::GetCreature(*me, m_citicenGUID))
                            npc->HandleEmoteCommand(EMOTE_STATE_NONE);
                        m_events.ScheduleEvent(EVENT_START_TALK_WITH_CITIZEN, 5s, 30s);
                        break;
                    }
                }
            }

            UpdateVictim();
        }

        void FillCitizenList()
        {
            listOfCitizenGUID.clear();
            std::list<Creature*> listOfCitizen;
            me->GetCreatureListWithEntryInGrid(listOfCitizen, NPC_PANICKED_CITIZEN_GATE, 35.0f);
            for (std::list<Creature*>::iterator itr = listOfCitizen.begin(); itr != listOfCitizen.end(); ++itr)
                listOfCitizenGUID.push_back((*itr)->GetGUID());
        }

        ObjectGuid GetRandomCitizen()
        {
            if (listOfCitizenGUID.empty())
                FillCitizenList();
            uint8 rol = urand(0, listOfCitizenGUID.size() - 1);
            std::list<ObjectGuid>::iterator itr = listOfCitizenGUID.begin();
            std::advance(itr, rol);
            return (*itr);
        }

    private:
        std::list<ObjectGuid> listOfCitizenGUID;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_gilneas_city_guard_gate_34864AI(creature);
    }
};

/* 35660 // part showfight worgen <> liam */   /* Quest 14098 */
class npc_rampaging_worgen_35660 : public CreatureScript
{
public:
    npc_rampaging_worgen_35660() : CreatureScript("npc_rampaging_worgen_35660") { }

    enum eNpc
    {
        MOVE_TO_START_POSITION = 101,
        MOVE_TO_PRINCE_LIAM,
        MOVE_TO_DOOR,
        EVENT_MOVE_TO_LIAM = 101,
        EVENT_ATTACK_LIAM,
        EVENT_ENRAGE_COOLDOWN,
        EVENT_MOVE_TO_DOOR,
        EVENT_FOLLOW_CITIZEN1,
        EVENT_FOLLOW_CITIZEN2,
        ACTION_START_ANIM_MERCANT = 101,
        ACTION_START_ANIM_LIAM = 102,
    };

    struct npc_rampaging_worgen_35660AI : public ScriptedAI
    {
        npc_rampaging_worgen_35660AI(Creature* creature) : ScriptedAI(creature) { }

        EventMap m_events;
        bool m_enrage;
        ObjectGuid m_liamGUID;
        ObjectGuid m_citizenGUID;
        ObjectGuid m_doorGUID;

        void Reset() override
        {
            m_enrage = false;
            m_doorGUID = ObjectGuid::Empty;
            m_liamGUID = ObjectGuid::Empty;
            m_citizenGUID = ObjectGuid::Empty;
            me->SetUnitFlag(UNIT_FLAG_IMMUNE_TO_NPC);
        }

        void JustDied(Unit* /*killer*/) override
        {
            me->DespawnOrUnsummon(1s);
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type == POINT_MOTION_TYPE)
                switch (id)
                {
                    case MOVE_TO_START_POSITION:
                    {
                        m_events.ScheduleEvent(EVENT_MOVE_TO_LIAM, 500ms);
                        break;
                    }
                    case MOVE_TO_PRINCE_LIAM:
                    {
                        m_events.ScheduleEvent(EVENT_ATTACK_LIAM, 100ms);
                        break;
                    }
                    case MOVE_TO_DOOR:
                    {
                        m_events.ScheduleEvent(EVENT_FOLLOW_CITIZEN1, 3s);
                        break;
                    }
                }
        }

        void DamageTaken(Unit* /*attacker*/, uint32& /*damage*/, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
        {
            if (!m_enrage && me->GetHealthPct() < 90.0f)
            {
                me->CastSpell(me, SPELL_ENRAGE_56646);
                m_enrage = true;
                m_events.ScheduleEvent(EVENT_ENRAGE_COOLDOWN, 121s, 150s);
            }
        }

        void AttackStart(Unit* who) override
        {
            if (who->GetEntry() != NPC_PRINCE_LIAM)
                return;
            ScriptedAI::AttackStart(who);
        }

        void SetGUID(ObjectGuid const& guid, int32 id) override
        {
            switch (id)
            {
                case NPC_PRINCE_LIAM:
                {
                    m_liamGUID = guid;
                    break;
                }
                case NPC_FRIGHTENED_CITIZEN_WORGEN:
                {
                    m_citizenGUID = guid;
                    break;
                }
                case GO_MERCHANT_DOOR:
                {
                    m_doorGUID = guid;
                    break;
                }
            }
        }

        void DoAction(int32 param) override
        {
            switch (param)
            {
                case ACTION_START_ANIM_MERCANT:
                {
                    m_events.ScheduleEvent(EVENT_MOVE_TO_DOOR, 1s);
                    break;
                }
                case ACTION_START_ANIM_LIAM:
                {
                    me->SetSpeed(MOVE_RUN, 1.8f);
                    me->GetMotionMaster()->MovePoint(MOVE_TO_START_POSITION, -1482.9f, 1394.6f, 35.55f);
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
                    case EVENT_ENRAGE_COOLDOWN:
                    {
                        m_enrage = false;
                        break;
                    }
                    case EVENT_MOVE_TO_LIAM:
                    {
                        if (Creature* liam = ObjectAccessor::GetCreature(*me, m_liamGUID))
                        {
                            Position pos = liam->GetPosition();
                            float angle = liam->GetAbsoluteAngle(pos.GetPositionX(), pos.GetPositionY());
                            Position pos2 = liam->GetNearPosition(2.0f, angle);
                            me->GetMotionMaster()->MovePoint(MOVE_TO_PRINCE_LIAM, pos2, true);
                        }
                        break;
                    }
                    case EVENT_ATTACK_LIAM:
                    {
                        if (Creature* liam = ObjectAccessor::GetCreature(*me, m_liamGUID))
                        {
                            me->SetReactState(REACT_AGGRESSIVE);
                            me->Attack(liam, true);
                            liam->Attack(me, true);
                        }
                        break;
                    }
                    case EVENT_MOVE_TO_DOOR:
                    {
                        if (GameObject* go = ObjectAccessor::GetGameObject(*me, m_doorGUID))
                            me->GetMotionMaster()->MovePoint(MOVE_TO_DOOR, go->GetPosition());
                        break;
                    }
                    case EVENT_FOLLOW_CITIZEN1:
                    {
                        if (Creature* citizen = ObjectAccessor::GetCreature(*me, m_citizenGUID))
                            me->GetMotionMaster()->MoveFollow(citizen, 1.0f, 0.0f);
                        m_events.ScheduleEvent(EVENT_FOLLOW_CITIZEN2, 5s);
                        break;
                    }
                    case EVENT_FOLLOW_CITIZEN2:
                    {
                        me->DespawnOrUnsummon();
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
        return new npc_rampaging_worgen_35660AI(creature);
    }
};

// 34913 // part showfight liam <> worgen
class npc_prince_liam_greymane_34913 : public CreatureScript
{
public:
    npc_prince_liam_greymane_34913() : CreatureScript("npc_prince_liam_greymane_34913") { }

    enum eNpc
    {
        EVENT_START_NEXT_SHOWFIGHT  = 101,
        ACTION_START_ATTACK_LIAM    = 102,
    };

    struct npc_prince_liam_greymane_34913AI : public ScriptedAI
    {
        npc_prince_liam_greymane_34913AI(Creature* c) : ScriptedAI(c) { }

        EventMap m_events;
        ObjectGuid m_worgenGUID;

        void Reset() override
        {
            m_events.ScheduleEvent(EVENT_START_NEXT_SHOWFIGHT, 60s);
            me->SetUnitFlag(UNIT_FLAG_REMOVE_CLIENT_CONTROL);
        }

        void DamageTaken(Unit* /*attacker*/, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
        {
            damage /= 3;
        }

        void JustSummoned(Creature* summon) override
        {
            if (summon->GetEntry() == NPC_RAMPAGING_WORGEN_35660)
            {
                m_worgenGUID = summon->GetGUID();
                summon->GetAI()->SetGUID(me->GetGUID(), me->GetEntry());
                summon->GetAI()->DoAction(ACTION_START_ATTACK_LIAM);
            }
        }

        void SummonedCreatureDies(Creature* summon, Unit* /*killer*/) override
        {
            m_worgenGUID = ObjectGuid::Empty;
            if (summon->GetEntry() == NPC_RAMPAGING_WORGEN_35660)
                m_events.ScheduleEvent(EVENT_START_NEXT_SHOWFIGHT, 20s);
        }

        void UpdateAI(uint32 diff) override
        {
            m_events.Update(diff);

            while (uint32 eventId = m_events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_START_NEXT_SHOWFIGHT:
                    {
                        me->SummonCreature(NPC_RAMPAGING_WORGEN_35660, (-1491.9f, 1413.1f, 35.56f, 5.5f), TEMPSUMMON_TIMED_DESPAWN, 120000);
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
        return new npc_prince_liam_greymane_34913AI(creature);
    }
};

// 34916 // part showfight guard <> worgen..
class npc_gilneas_city_guard_34916 : public CreatureScript
{
public:
    npc_gilneas_city_guard_34916() : CreatureScript("npc_gilneas_city_guard_34916") { }

    enum eNpc
    {
        EVENT_CHECK_SHOWFIGHT = 101,
        MOVE_TO_HOMEPOSITION = 101,
        NPC_RAMPAGING_WORGEN_34884 = 34884,
        NPC_BLOODFANG_WORGEN_35118 = 35118,
        NPC_RAMPAGING_WORGEN_35660 = 35660
    };

    struct npc_gilneas_city_guard_34916AI : public ScriptedAI
    {
        npc_gilneas_city_guard_34916AI(Creature* creature) : ScriptedAI(creature) { Initialize(); }

        EventMap m_events;
        float    m_minHealthPct;
        std::list<uint32> m_worgenList;

        void Initialize()
        {
            m_worgenList.clear();
            m_worgenList.push_back(NPC_RAMPAGING_WORGEN_34884);
            m_worgenList.push_back(NPC_BLOODFANG_WORGEN_35118);
            m_worgenList.push_back(NPC_RAMPAGING_WORGEN_35660);
            m_minHealthPct = frand(30.0f, 85.0f);
        }

        void Reset() override
        {
            m_events.Reset();
            m_events.ScheduleEvent(EVENT_CHECK_SHOWFIGHT, 900ms, 1200ms);
            me->SetUnitFlag(UNIT_FLAG_IMMUNE_TO_NPC);
        }

        void DamageTaken(Unit* who, uint32& Damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
        {
            if (who->ToCreature())
            {
                if (me->GetHealthPct() < m_minHealthPct)
                    Damage = 0;
                else
                    Damage = 1;
            }
        }

        void DamageDealt(Unit* victim, uint32& damage, DamageEffectType /*damageType*/) override
        {
            if (victim->ToCreature())
            {
                if (victim->GetHealthPct() < m_minHealthPct)
                    damage = 0;
                else
                    damage = 1;
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

                    Creature* worgen = nullptr;
                    float nearestDist = 5.0f;

                    for (uint32 worgenEntry : m_worgenList)
                    {
                        if (Creature* tempWorgen = me->FindNearestCreature(worgenEntry, nearestDist))
                        {
                            if (!tempWorgen->IsInCombat())
                            {
                                worgen = tempWorgen;
                                break;
                            }
                        }
                    }

                    if (worgen)
                    {
                        me->Attack(worgen, true);
                        worgen->Attack(me, true);
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
        return new npc_gilneas_city_guard_34916AI(creature);
    }
};

// 34884 // part showfight worgen <> guard..
class npc_rampaging_worgen_34884 : public CreatureScript
{
public:
    npc_rampaging_worgen_34884() : CreatureScript("npc_rampaging_worgen_34884") {}

    enum eNpc
    {
        EVENT_ENRAGE_COOLDOWN,
    };

    struct npc_rampaging_worgen_34884AI : public ScriptedAI
    {
        npc_rampaging_worgen_34884AI(Creature* creature) : ScriptedAI(creature) {}

        EventMap m_events;
        bool m_enrage;

        void Reset() override
        {
            m_enrage = false;
            me->SetUnitFlag(UNIT_FLAG_IMMUNE_TO_NPC);
        }

        void DamageTaken(Unit* /*who*/, uint32& /*damage*/, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
        {
            if (!m_enrage && me->GetHealthPct() < 90.0f)
            {
                me->CastSpell(me, SPELL_ENRAGE_8599);
                m_enrage = true;
                m_events.ScheduleEvent(EVENT_ENRAGE_COOLDOWN, 121s, 150s);
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
                }
            }

            if (!UpdateVictim())
                return;
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_rampaging_worgen_34884AI(creature);
    }
};

// 34981 // alone
class npc_frightened_citizen_34981 : public CreatureScript
{
public:
    npc_frightened_citizen_34981() : CreatureScript("npc_frightened_citizen_34981") {}

    enum eNpc
    {
        ACTION_START_ANIM_CITIZEN = 103,
        EVENT_MOVE_TO_DOOR = 101,
        EVENT_MOVE_TO_PLAYER,
        EVENT_MOVE_TO_MARKER,
        EVENT_MOVE_TO_END,
        MOVE_TO_DOOR = 101,
        MOVE_TO_PLAYER,
        MOVE_TO_MARKER,
        MOVE_TO_END,
    };

    struct npc_frightened_citizen_34981AI : public ScriptedAI
    {
        npc_frightened_citizen_34981AI(Creature* creature) : ScriptedAI(creature) {}

        EventMap m_events;
        ObjectGuid m_playerGUID;
        ObjectGuid m_doorGUID;

        void Reset() override
        {
            m_playerGUID = ObjectGuid();
            m_doorGUID = ObjectGuid();
            me->SetUnitFlag(UNIT_FLAG_IMMUNE_TO_NPC);
        }

        void MovementInform(uint32 type, uint32 point) override
        {
            if (type == POINT_MOTION_TYPE)
                switch (point)
                {
                    case MOVE_TO_DOOR:
                    {
                        m_events.ScheduleEvent(EVENT_MOVE_TO_PLAYER, 25ms);
                        break;
                    }
                    case MOVE_TO_PLAYER:
                    {
                        Talk(1);
                        m_events.ScheduleEvent(EVENT_MOVE_TO_MARKER, 4s);
                        break;
                    }
                    case MOVE_TO_MARKER:
                    {
                        m_events.ScheduleEvent(EVENT_MOVE_TO_END, 25ms);
                        break;
                    }
                    case MOVE_TO_END:
                    {
                        me->DespawnOrUnsummon();
                        break;
                    }
                }
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
                case GO_MERCHANT_DOOR:
                {
                    m_doorGUID = guid;
                    break;
                }
            }
        }

        void DoAction(int32 param) override
        {
            switch (param)
            {
                case ACTION_START_ANIM_CITIZEN:
                {
                    m_events.ScheduleEvent(EVENT_MOVE_TO_DOOR, 2s);
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
                case EVENT_MOVE_TO_DOOR:
                {
                    if (GameObject* go = ObjectAccessor::GetGameObject(*me, m_doorGUID))
                        me->GetMotionMaster()->MovePoint(MOVE_TO_DOOR, go->GetPosition());
                    break;
                }
                case EVENT_MOVE_TO_PLAYER:
                {
                    if (Player* player = ObjectAccessor::GetPlayer(*me, m_playerGUID))
                        me->GetMotionMaster()->MovePoint(MOVE_TO_PLAYER, player->GetNearPosition(2.0, player->GetOrientation()));
                    break;
                }
                case EVENT_MOVE_TO_MARKER:
                {
                    if (Creature* marker = me->FindNearestCreature(NPC_GILNEAS_EVACUATION_FACING_MARKER_35830, 100.0f))
                        me->GetMotionMaster()->MovePoint(MOVE_TO_MARKER, marker->GetPosition());
                    break;
                }
                case EVENT_MOVE_TO_END:
                {
                    me->GetMotionMaster()->MovePoint(MOVE_TO_END, me->GetNearPosition(25.0f, 0.0f));
                    break;
                }
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_frightened_citizen_34981AI(creature);
    }
};

// 35836 // with worgen
class npc_frightened_citizen_35836 : public CreatureScript
{
public:
    npc_frightened_citizen_35836() : CreatureScript("npc_frightened_citizen_35836") {}

    enum eNpc
    {
        ACTION_START_ANIM_CITIZEN = 103,
        EVENT_MOVE_TO_DOOR = 101,
        EVENT_MOVE_TO_PLAYER,
        EVENT_MOVE_TO_MARKER,
        EVENT_MOVE_TO_END,
        MOVE_TO_DOOR = 101,
        MOVE_TO_PLAYER,
        MOVE_TO_MARKER,
        MOVE_TO_END,
    };

    struct npc_frightened_citizen_35836AI : public ScriptedAI
    {
        npc_frightened_citizen_35836AI(Creature* creature) : ScriptedAI(creature) {}

        EventMap m_events;
        ObjectGuid m_playerGUID;
        ObjectGuid m_doorGUID;

        void Reset() override
        {
            m_playerGUID = ObjectGuid();
            m_doorGUID = ObjectGuid();
            me->SetUnitFlag(UNIT_FLAG_IMMUNE_TO_NPC);
        }

        void MovementInform(uint32 type, uint32 point) override
        {
            if (type == POINT_MOTION_TYPE)
                switch (point)
                {
                    case MOVE_TO_DOOR:
                    {
                        m_events.ScheduleEvent(EVENT_MOVE_TO_PLAYER, 25ms);
                        break;
                    }
                    case MOVE_TO_PLAYER:
                    {
                        Talk(0);
                        m_events.ScheduleEvent(EVENT_MOVE_TO_MARKER, 2s);
                        break;
                    }
                    case MOVE_TO_MARKER:
                    {
                        m_events.ScheduleEvent(EVENT_MOVE_TO_END, 25ms);
                        break;
                    }
                    case MOVE_TO_END:
                    {
                        me->DespawnOrUnsummon();
                        break;
                    }
                }
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
                case GO_MERCHANT_DOOR:
                {
                    m_doorGUID = guid;
                    break;
                }
            }
        }

        void DoAction(int32 param) override
        {
            switch (param)
            {
            case ACTION_START_ANIM_CITIZEN:
            {
                m_events.ScheduleEvent(EVENT_MOVE_TO_DOOR, 1s);
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
                    case EVENT_MOVE_TO_DOOR:
                    {
                        if (GameObject* go = ObjectAccessor::GetGameObject(*me, m_doorGUID))
                            me->GetMotionMaster()->MovePoint(MOVE_TO_DOOR, go->GetPosition());
                        break;
                    }
                    case EVENT_MOVE_TO_PLAYER:
                    {
                        if (Player* player = ObjectAccessor::GetPlayer(*me, m_playerGUID))
                        {
                            me->GetMotionMaster()->MovePoint(MOVE_TO_PLAYER, player->GetNearPosition(2.0, player->GetOrientation()));
                        }
                        break;
                    }
                    case EVENT_MOVE_TO_MARKER:
                    {
                        if (Creature* marker = me->FindNearestCreature(NPC_GILNEAS_EVACUATION_FACING_MARKER_35830, 100.0f))
                            me->GetMotionMaster()->MovePoint(MOVE_TO_MARKER, marker->GetPosition());
                        break;
                    }
                    case EVENT_MOVE_TO_END:
                    {
                        me->GetMotionMaster()->MovePoint(MOVE_TO_END, me->GetNearPosition(10.0f, 0.0f));
                        break;
                    }
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_frightened_citizen_35836AI(creature);
    }
};

// 195327
class go_merchant_square_door_195327 : public GameObjectScript
{
public:
    go_merchant_square_door_195327() : GameObjectScript("go_merchant_square_door_195327") { }

    enum eDoor
    {
        ACTION_START_ANIM_MERCANT = 101,
        ACTION_START_ANIM_CITIZEN = 103,
    };

    struct go_merchant_square_door_195327AI : public GameObjectAI
    {
        go_merchant_square_door_195327AI(GameObject* go) : GameObjectAI(go) { }

        ObjectGuid m_playerGUID;
        ObjectGuid m_doorGUID;
        ObjectGuid m_citizenGUID;
        ObjectGuid m_worgenGUID;

        bool OnGossipHello(Player* player, GameObject* go) override
        {
            if (!player || !go)
                return false;

            switch (urand(0, 1))
            {
            case 0:
            {
                SummonOnlyCitizen(player, go);
                player->KilledMonsterCredit(NPC_GILNEAS_EVACUATION_FACING_MARKER_35830);
                break;
            }
            case 1:
            {
                SummonCitizenAndWorgen(player, go);
                player->KilledMonsterCredit(NPC_GILNEAS_EVACUATION_FACING_MARKER_35830);
                break;
            }
            }

            return false;
        }

    private:
        void SummonOnlyCitizen(Player* player, GameObject* go)
        {
            Position posC = GetCitizenPosition(go);
            if (Creature* citizen = player->SummonCreature(NPC_FRIGHTENED_CITIZEN_34981, posC, TEMPSUMMON_TIMED_DESPAWN, 30000ms))
            {
                citizen->GetAI()->SetGUID(go->GetGUID(), go->GetEntry());
                citizen->GetAI()->SetGUID(player->GetGUID(), PLAYER_GUID);
                citizen->GetAI()->DoAction(ACTION_START_ANIM_CITIZEN);
            }
        }

        void SummonCitizenAndWorgen(Player* player, GameObject* go)
        {
            Position posC = GetCitizenPosition(go);
            Position posW = GetWorgenPosition(go);
            if (Creature* citizen = player->SummonCreature(NPC_FRIGHTENED_CITIZEN_WORGEN, posC, TEMPSUMMON_TIMED_DESPAWN, 30000ms))
                if (Creature* worgen = player->SummonCreature(NPC_RAMPAGING_WORGEN_35660, posW, TEMPSUMMON_TIMED_DESPAWN, 30000ms))
                {
                    citizen->GetAI()->SetGUID(go->GetGUID(), go->GetEntry());
                    citizen->GetAI()->SetGUID(player->GetGUID(), PLAYER_GUID);
                    citizen->GetAI()->SetGUID(worgen->GetGUID(), worgen->GetEntry());
                    citizen->GetAI()->DoAction(ACTION_START_ANIM_CITIZEN);
                    worgen->GetAI()->SetGUID(go->GetGUID(), go->GetEntry());
                    worgen->GetAI()->SetGUID(player->GetGUID(), PLAYER_GUID);
                    worgen->GetAI()->SetGUID(citizen->GetGUID(), citizen->GetEntry());
                    worgen->GetAI()->DoAction(ACTION_START_ANIM_MERCANT);
                }
        }

        Position GetCitizenPosition(GameObject* go)
        {
            float x, y;
            go->GetNearPoint2D(go, x, y, 1.0f, go->GetOrientation() + M_PI);
            return Position(x, y, go->GetPositionZ());
        }

        Position GetWorgenPosition(GameObject* go)
        {
            float x, y;
            go->GetNearPoint2D(go, x, y, 3.0f, go->GetOrientation() + M_PI * 0.75f);
            return Position(x, y, go->GetPositionZ());
        }
    };

    GameObjectAI* GetAI(GameObject* go) const override
    {
        return new go_merchant_square_door_195327AI(go);
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

// 47091
class npc_wounded_guard_47091 : public CreatureScript
{
public:
    npc_wounded_guard_47091() : CreatureScript("npc_wounded_guard_47091") {}

    struct npc_wounded_guard_47091AI : public ScriptedAI
    {
        npc_wounded_guard_47091AI(Creature* creature) : ScriptedAI(creature) {}

        void SpellHit(WorldObject* caster, SpellInfo const* /*spellInfo*/) override
        {
            if (Player* player = caster->ToPlayer())
            {
                if (player->GetQuestStatus(QUEST_A_REJUVENATING_TOUCH) == QUEST_STATUS_INCOMPLETE)
                    player->KilledMonsterCredit(NPC_HEALING_CREDIT);
                else if (player->GetQuestStatus(QUEST_FLASH_HEAL) == QUEST_STATUS_INCOMPLETE)
                    player->KilledMonsterCredit(NPC_HEALING_CREDIT);
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_wounded_guard_47091AI(creature);
    }
};

// 35872
class npc_mariam_spellwalker_35872 : public CreatureScript
{
public:
    npc_mariam_spellwalker_35872() : CreatureScript("npc_mariam_spellwalker_35872") {}

    enum eNpc
    {
        EVENT_CHECK_SPELL = 101,
        AI_MIN_HP = 85,
    };

    struct npc_mariam_spellwalker_35872AI : public ScriptedAI
    {
        npc_mariam_spellwalker_35872AI(Creature* creature) : ScriptedAI(creature) {}

        EventMap m_events;

        void Reset() override
        {
            m_events.ScheduleEvent(EVENT_CHECK_SPELL, 1s, 2s);
        }

        void DamageTaken(Unit* who, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
        {
            if (me->HealthBelowPct(AI_MIN_HP) && who->ToCreature())
                damage = 0;
        }

        void UpdateAI(uint32 diff) override
        {
            m_events.Update(diff);

            while (uint32 eventId = m_events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_CHECK_SPELL:
                {
                    if ((me->IsAlive()) && (!me->IsInCombat() && (me->GetDistance2d(me->GetHomePosition().GetPositionX(), me->GetHomePosition().GetPositionY()) <= 1.0f)))
                        if (Creature* enemy = me->FindNearestCreature(NPC_BLOODFANG_WORGEN_35118, 5.0f, true))
                            me->AI()->AttackStart(enemy); // She should really only grab agro when npc Cleese is not there, so we will keep this range small

                    m_events.ScheduleEvent(EVENT_CHECK_SPELL, 1000ms, 2000ms);
                    break;
                }
                }
            }

            if (!UpdateVictim())
                return;

            if (me->GetVictim()->GetEntry() == NPC_BLOODFANG_WORGEN_35118)
                DoSpellAttackIfReady(SPELL_FROSTBOLT_VISUAL_ONLY); //Dummy spell, visual only to prevent getting agro (Blizz-like)
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_mariam_spellwalker_35872AI(creature);
    }
};

// 35232
class npc_gilnean_royal_guard_35232 : public CreatureScript
{
public:
    npc_gilnean_royal_guard_35232() : CreatureScript("npc_gilnean_royal_guard_35232") {}

    enum eNpc
    {
        EVENT_CHECK_SHOWFIGHT = 101,
        EVENT_CHECK_KING_NEAR,
        MOVE_TO_HOMEPOSITION = 101,
        NPC_RAMPAGING_WORGEN_34884 = 34884,
        NPC_BLOODFANG_WORGEN_35118 = 35118,
        NPC_RAMPAGING_WORGEN_35660 = 35660,
        NPC_KING_GREYMANE_35112 = 35112
    };

    struct npc_gilnean_royal_guard_35232AI : public ScriptedAI
    {
        npc_gilnean_royal_guard_35232AI(Creature* creature) : ScriptedAI(creature) { Initialize(); }

        EventMap m_events;
        float    m_minHealthPct;
        bool     m_isShowFight;
        bool     m_theKingIsNear;
        ObjectGuid   m_kingGUID;
        std::list<uint32> m_worgenList;

        void Initialize()
        {
            m_worgenList.clear();
            m_worgenList.push_back(NPC_RAMPAGING_WORGEN_34884);
            m_worgenList.push_back(NPC_BLOODFANG_WORGEN_35118);
            m_worgenList.push_back(NPC_RAMPAGING_WORGEN_35660);
            m_theKingIsNear = false;
            m_isShowFight = true;
            m_minHealthPct = frand(30.0f, 85.0f);
        }

        void Reset() override
        {
            m_events.Reset();
            m_events.ScheduleEvent(EVENT_CHECK_SHOWFIGHT, 1000ms);
            m_events.ScheduleEvent(EVENT_CHECK_KING_NEAR, 1000ms);
        }

        void DamageTaken(Unit* who, uint32& Damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
        {
            if (!m_theKingIsNear)
                if (m_isShowFight && who->ToCreature())
                {
                    if (me->GetHealthPct() < m_minHealthPct)
                        Damage = 0;
                    else
                        Damage = 1;
                }
        }

        void DamageDealt(Unit* victim, uint32& damage, DamageEffectType /*damageType*/) override
        {
            if (!m_theKingIsNear)
                if (m_isShowFight && victim->ToCreature())
                {
                    if (victim->GetHealthPct() < m_minHealthPct)
                        damage = 0;
                    else
                        damage = 1;
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

                    Creature* worgen = nullptr;
                    float nearestDist = 5.0f;

                    for (uint32 worgenEntry : m_worgenList)
                    {
                        if (Creature* tempWorgen = me->FindNearestCreature(worgenEntry, nearestDist))
                        {
                            worgen = tempWorgen;
                            break;
                        }
                    }

                    if (worgen)
                    {
                        me->Attack(worgen, true);
                        worgen->Attack(me, true);
                    }

                    m_events.ScheduleEvent(EVENT_CHECK_SHOWFIGHT, 2500ms);
                    break;
                }
                case EVENT_CHECK_KING_NEAR:
                {
                    if (Creature* king = me->FindNearestCreature(NPC_KING_GREYMANE_35112, 15.0f))
                    {
                        m_kingGUID = king->GetGUID();
                        m_theKingIsNear = true;
                        m_isShowFight = false;
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
        return new npc_gilnean_royal_guard_35232AI(creature);
    }
};

// 35839
class npc_sergeant_cleese_35839 : public CreatureScript
{
public:
    npc_sergeant_cleese_35839() : CreatureScript("npc_sergeant_cleese_35839") {}

    enum eNpc
    {
        AI_MIN_HP = 85,
    };

    struct npc_sergeant_cleese_35839AI : public ScriptedAI
    {
        npc_sergeant_cleese_35839AI(Creature* creature) : ScriptedAI(creature) {}

        uint32 tSeek;

        void Reset() override
        {
            tSeek = urand(1000, 2000);
        }

        void DamageTaken(Unit* who, uint32& damage, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
        {
            if (who->GetTypeId() == TYPEID_PLAYER)
            {
                me->GetThreatManager().ResetAllThreat();
                AddThreat(me, 1.0f);
                AddThreat(who, 1.0f);
                me->AI()->AttackStart(who);
            }
            else if (who->IsPet())
            {
                me->GetThreatManager().ResetAllThreat();
                AddThreat(who, 1.0f);
                me->AI()->AttackStart(who);
            }
            else if (me->HealthBelowPct(AI_MIN_HP) && who->GetEntry() == NPC_BLOODFANG_WORGEN_35118)
                damage = 0;
        }

        void UpdateAI(uint32 diff) override
        {
            if (tSeek <= diff)
            {
                if ((me->IsAlive()) && (!me->IsInCombat() && (me->GetDistance2d(me->GetHomePosition().GetPositionX(), me->GetHomePosition().GetPositionY()) <= 1.0f)))
                    if (Creature* enemy = me->FindNearestCreature(NPC_BLOODFANG_WORGEN_35118, 10.0f, true))
                        me->AI()->AttackStart(enemy);
                tSeek = urand(1000, 2000); // optimize cpu load, seeking only sometime between 1 and 2 seconds
            }
            else tSeek -= diff;

            if (!UpdateVictim())
                return;
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_sergeant_cleese_35839AI(creature);
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
        SAY_KING_GENN_GREYMANE = 0,
        MAX_SUMMONED_RIPPER = 20,
    };

    bool OnQuestAccept(Player* player, Creature* creature, Quest const* quest) override
    {
        if (quest->GetQuestId() == QUEST_SAVE_KRENNAN_ARANAS)
        {
            creature->GetAI()->SetGUID(player->GetGUID(), PLAYER_GUID);
            player->CastSpell(player, SPELL_GENERIC_QUEST_INVISIBILITY_DETECTION_1);
            creature->CastSpell(player, SPELL_FORCECAST_SUMMON_GRAYMANE_HORSE, true);
        }
        return true;
    }

    struct npc_king_genn_greymane_35550AI : public ScriptedAI
    {
        npc_king_genn_greymane_35550AI(Creature* creature) : ScriptedAI(creature) { Initialize(); }

        uint32 tSummon, tSay;
        bool EventActive, RunOnce;
        uint32 m_counter;
        ObjectGuid m_playerGUID;

        void Initialize()
        {
            m_counter = 0;
        }

        void Reset() override
        {
            tSay = urand(10000, 20000);
            tSummon = urand(3000, 5000); // How often we spawn
        }

        void JustSummoned(Creature* summoned) override
        {
            switch (summoned->GetEntry())
            {
                case NPC_BLOODFANG_RIPPER_35505:
                {
                    m_counter += 1;
                    summoned->GetDefaultMovementType();
                    summoned->SetReactState(REACT_AGGRESSIVE);
                    break;
                }
            }
        }

        void SummonedCreatureDespawn(Creature* summon) override
        {
            if (summon->GetEntry() == NPC_BLOODFANG_RIPPER_35505 && m_counter)
                m_counter -= 1;
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

        void UpdateAI(uint32 diff) override
        {
            if (tSay <= diff) // Time for next spawn wave
            {
                Talk(SAY_KING_GENN_GREYMANE);
                tSay = urand(10000, 20000);
            }
            else tSay -= diff;

            if (tSummon <= diff) // Time for next spawn wave
            {
                SummonNextWave(); // Activate next spawn wave
                tSummon = urand(3000, 5000); // Reset our spawn timer
            }
            else tSummon -= diff;
        }

        void SummonNextWave()
        {
            switch (urand(1, 4))
            {
                case 1:
                {
                    for (int i = 0; i < 5; i++)
                        if (m_counter < MAX_SUMMONED_RIPPER)
                            me->SummonCreature(NPC_BLOODFANG_RIPPER_35505, -1781.173f + irand(-15, 15), 1372.90f + irand(-15, 15), 19.7803f, urand(0, 6), TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 15000ms);
                    break;
                }
                case 2:
                {
                    for (int i = 0; i < 5; i++)
                        if (m_counter < MAX_SUMMONED_RIPPER)
                            me->SummonCreature(NPC_BLOODFANG_RIPPER_35505, -1756.30f + irand(-15, 15), 1380.61f + irand(-15, 15), 19.7652f, urand(0, 6), TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 15000ms);
                    break;
                }
                case 3:
                {
                    for (int i = 0; i < 5; i++)
                        if (m_counter < MAX_SUMMONED_RIPPER)
                            me->SummonCreature(NPC_BLOODFANG_RIPPER_35505, -1739.84f + irand(-15, 15), 1384.87f + irand(-15, 15), 19.841f, urand(0, 6), TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 15000ms);
                    break;
                }
                case 4:
                {
                    for (int i = 0; i < 5; i++)
                        if (m_counter < MAX_SUMMONED_RIPPER)
                            me->SummonCreature(NPC_BLOODFANG_RIPPER_35505, -1781.173f + irand(-15, 15), 1372.90f + irand(-15, 15), 19.7803f, urand(0, 6), TEMPSUMMON_TIMED_OR_DEAD_DESPAWN, 15000ms);
                    break;
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_king_genn_greymane_35550AI(creature);
    }
};

// 35905
class npc_king_greymanes_horse_35905 : public CreatureScript
{
    public:
        npc_king_greymanes_horse_35905() : CreatureScript("npc_king_greymanes_horse_35905") { }

        enum eHorse
        {
            SAY_KRENNAN_TREE_HELP = 0,
            SAY_HORSE_HOW_DO_HELP = 1,
            SAY_KRENNAN_HORSE_THANKS = 2,
            EVENT_SAY_KRENNAN_HELP = 101,   // krennan 35753 in tree
            EVENT_STARTING_RESCUE_PART2,
            EVENT_SAY_KRENNAN_THANKS,
            EVENT_START_WAYPOINT_MOVEMENT,
            EVENT_MOVE_TO_NEXT_POINT,
            WAYPOINT_COMPLETE
        };

        struct WaypointData
        {
            float x, y, z, o;
        };

        static const std::vector<WaypointData> waypoints;

        struct npc_king_greymanes_horse_35905AI : public ScriptedAI
        {
            npc_king_greymanes_horse_35905AI(Creature* creature) : ScriptedAI(creature)
            {
                _currentWaypoint = 0;
                _isPaused = false;
            }

            TaskScheduler _scheduler;
            EventMap m_events;
            ObjectGuid m_playerGUID;
            ObjectGuid m_krennanHorseGUID;
            ObjectGuid m_krennanTreeGUID;
            uint32 _currentWaypoint;
            bool _isPaused;

            void Reset() override
            {
                m_events.Reset();
                _scheduler.CancelAll();
                m_playerGUID = ObjectGuid();
                m_krennanHorseGUID = ObjectGuid();
                m_krennanTreeGUID = ObjectGuid();
                _currentWaypoint = 0;
                _isPaused = false;
                me->GetMotionMaster()->MoveIdle();
                m_events.ScheduleEvent(EVENT_SAY_KRENNAN_HELP, 500ms);
            }

            void StartWaypointMovement(ObjectGuid playerGUID)
            {
                m_playerGUID = playerGUID;
                _currentWaypoint = 0;
                _isPaused = false;

                _scheduler.Schedule(1s, [this](TaskContext& /*context*/)
                    {
                        MoveToNextPoint();
                    });
            }

            void SetWaypointPaused(bool paused)
            {
                _isPaused = paused;
                if (!paused)
                    MoveToNextPoint();
            }

            Player* GetPlayerForWaypoint()
            {
                return ObjectAccessor::GetPlayer(*me, m_playerGUID);
            }

            // ????? ??? ??????????? ? ????????? ?????
            void MoveToNextPoint()
            {
                if (_isPaused || _currentWaypoint >= waypoints.size())
                    return;

                const WaypointData& wp = waypoints[_currentWaypoint];
                me->GetMotionMaster()->MovePoint(WAYPOINT_COMPLETE, wp.x, wp.y, wp.z, wp.o);
            }

            void MovementInform(uint32 type, uint32 id) override
            {
                if (type == POINT_MOTION_TYPE && id == WAYPOINT_COMPLETE)
                {
                    // ???????????? ???????? ? ?????
                    HandleWaypointReached(_currentWaypoint);

                    // ???? ?? ?? ?????, ????????? ???????? ? ????????? ?????
                    if (!_isPaused)
                    {
                        _currentWaypoint++;
                        _scheduler.Schedule(1s, [this](TaskContext& /*context*/)
                            {
                                MoveToNextPoint();
                            });
                    }
                }
                else if (type == EFFECT_MOTION_TYPE && _currentWaypoint == 5)
                {
                    // ????????? ?????????? ??????
                    _currentWaypoint++;
                    _isPaused = false;
                    _scheduler.Schedule(1s, [this](TaskContext& /*context*/)
                        {
                            MoveToNextPoint();
                        });
                }
            }

            // ?????? WaypointReached ?? EscortAI
            void HandleWaypointReached(uint32 waypointId)
            {
                switch (waypointId)
                {
                case 5:
                {
                    if (me->GetVehicleKit()->HasEmptySeat(1))
                    {
                        _isPaused = true;
                        me->GetMotionMaster()->MoveJump(EVENT_JUMP, Position(-1679.089f, 1348.42f, 15.31f), 25.0f, 15.0f);
                        if (Player* player = GetPlayerForWaypoint())
                        {
                            Talk(SAY_HORSE_HOW_DO_HELP, player);
                            player->SetClientControl(me, true);
                        }
                    }
                    break;
                }
                case 6:
                {
                    if (Player* player = GetPlayerForWaypoint())
                        player->SetClientControl(me, false);
                    break;
                }
                case 12:
                {
                    if (Player* player = GetPlayerForWaypoint())
                        player->ExitVehicle();
                    break;
                }
                }
            }

            void PassengerBoarded(Unit* who, int8 seatId, bool apply) override
            {
                if (apply)
                {
                    if (Player* player = who->ToPlayer())
                    {
                        if (!m_playerGUID)
                        {
                            m_playerGUID = player->GetGUID();
                            StartWaypointMovement(m_playerGUID);
                            player->SetClientControl(me, false);
                        }
                    }
                    else if (who->GetEntry() == NPC_KRENNAN_ARANAS && !m_krennanHorseGUID)
                    {
                        m_krennanHorseGUID = who->GetGUID();
                        if (Player* player = ObjectAccessor::GetPlayer(*me, m_playerGUID))
                            player->RemoveAura(SPELL_GENERIC_QUEST_INVISIBILITY_DETECTION_1);
                        m_events.ScheduleEvent(EVENT_STARTING_RESCUE_PART2, 400ms);
                    }
                }
                else if (seatId == 1)
                {
                    m_events.ScheduleEvent(EVENT_SAY_KRENNAN_THANKS, 25ms);
                }
            }

            void JustDied(Unit* /*killer*/) override
            {
                if (Player* player = GetPlayerForWaypoint())
                {
                    player->FailQuest(QUEST_SAVE_KRENNAN_ARANAS);
                    player->RemoveAura(SPELL_GENERIC_QUEST_INVISIBILITY_DETECTION_1);
                }
            }

            void UpdateAI(uint32 diff) override
            {
                _scheduler.Update(diff);
                m_events.Update(diff);

                while (uint32 eventId = m_events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case EVENT_SAY_KRENNAN_HELP:
                    {
                        if (!m_krennanTreeGUID)
                            if (Creature* krennan = me->FindNearestCreature(NPC_KRENNAN_ARANAS_TREE, 100.0f, true))
                                m_krennanTreeGUID = krennan->GetGUID();

                        if (!m_krennanHorseGUID)
                        {
                            if (Creature* krennan = ObjectAccessor::GetCreature(*me, m_krennanTreeGUID))
                                if (Player* player = ObjectAccessor::GetPlayer(*me, m_playerGUID))
                                {
                                    krennan->AI()->Talk(SAY_KRENNAN_TREE_HELP, player);
                                    m_events.ScheduleEvent(EVENT_SAY_KRENNAN_HELP, 6s, 9s);
                                    break;
                                }
                            m_events.ScheduleEvent(EVENT_SAY_KRENNAN_HELP, 500ms);
                        }
                        break;
                    }
                    case EVENT_STARTING_RESCUE_PART2:
                    {
                        SetWaypointPaused(false);
                        break;
                    }
                    case EVENT_SAY_KRENNAN_THANKS:
                    {
                        if (Creature* krennan = ObjectAccessor::GetCreature(*me, m_krennanHorseGUID))
                        {
                            krennan->AI()->Talk(0);
                            krennan->NearTeleportTo(-1771.03f, 1433.41f, 19.85f, 3.598f);
                            krennan->GetAI()->DoAction(0);
                        }
                        me->DespawnOrUnsummon(25ms);
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

const std::vector<npc_king_greymanes_horse_35905::WaypointData> npc_king_greymanes_horse_35905::waypoints = {
    { -1800.63f, 1404.35f, 19.77f, 0.0f },   // 0
    { -1798.42f, 1409.55f, 19.79f, 0.0f },   // 1
    { -1790.40f, 1419.52f, 19.53f, 0.0f },   // 2
    { -1779.77f, 1430.24f, 19.79f, 0.0f },   // 3
    { -1767.85f, 1430.73f, 19.68f, 0.0f },   // 4
    { -1745.50f, 1393.67f, 19.78f, 0.0f },   // 5 - jump here
    { -1720.64f, 1367.78f, 19.68f, 0.0f },   // 6
    { -1705.76f, 1356.66f, 19.83f, 0.0f },   // 7
    { -1687.94f, 1353.74f, 19.73f, 0.0f },   // 8
    { -1685.28f, 1351.58f, 19.73f, 0.0f },   // 9
    { -1681.12f, 1348.92f, 19.73f, 0.0f },   // 10
    { -1682.22f, 1347.67f, 19.73f, 0.0f },   // 11
    { -1771.03f, 1433.41f, 19.85f, 0.0f }    // 12 - last
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
                player->CastSpell(player, SPELL_GILNEAS_CANNON_CAMERA);
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
                    if (me->GetDistance(player) < 20.0f)
                    if (!HasPlayer(player->GetGUID()))
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
class npc_crowley_horse_35231 : public CreatureScript
{
public:
    npc_crowley_horse_35231() : CreatureScript("npc_crowley_horse_35231") {}

    enum eNpc
    {
        EVENT_JUMP_ON_WALL = 950,
        EVENT_JUMP_ON_BRIDGE,
        EVENT_CAST_TORCH,
    };

    struct npc_crowley_horse_35231AI : public NullCreatureAI
    {
        npc_crowley_horse_35231AI(Creature *creature) : NullCreatureAI(creature) { }

        EventMap    m_events;
        ObjectGuid      m_playerGUID;
        ObjectGuid      m_dariusGUID;
        uint32      m_movePart;
        bool        m_IsActiveAI;

        void Reset() override
        {
            m_playerGUID = ObjectGuid();
            m_dariusGUID = ObjectGuid();
            m_movePart = 0;
            me->SetUnitFlag(UNIT_FLAG_IMMUNE_TO_NPC);
        }

        void IsSummonedBy(WorldObject* summoner) override
        {
            if (Player* player = summoner->ToPlayer())
                m_playerGUID = player->GetGUID();
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

        void PassengerBoarded(Unit* passenger, int8 /*seatId*/, bool apply) override
        {
            if (passenger->IsPlayer())
            {
                if (apply)
                    m_events.ScheduleEvent(EVENT_START_WALK, 1s);
                else
                    me->DespawnOrUnsummon(10s);
            }
            if (Creature* npc = passenger->ToCreature())
            {
                if (npc->GetEntry() == NPC_DARIUS_CROWLEY)
                {
                    m_dariusGUID = npc->GetGUID();
                    npc->SetUnitFlag(UNIT_FLAG_IMMUNE_TO_NPC);
                }
            }
        }

        void MovementInform(uint32 type, uint32 id) override
        {
            if (type == WAYPOINT_MOTION_TYPE)
                switch (id)
                {
                case 3:
                {
                    if (m_movePart == 3)
                        if (Player* player = ObjectAccessor::GetPlayer(*me, m_playerGUID))
                            if (Creature* darius = ObjectAccessor::GetCreature(*me, m_dariusGUID))
                            {
                                darius->AI()->Talk(0, player);
                                m_events.ScheduleEvent(EVENT_TALK_PERIODIC, 6000ms);
                            }
                    break;
                }
                case 14:
                {
                    if (m_movePart == 5) // endpoint run 2
                        m_events.ScheduleEvent(EVENT_MOVE_PART5, 10s);
                    break;
                }
                case 16:
                {
                    if (m_movePart == 3) // endpoint run 1
                        m_events.ScheduleEvent(EVENT_MOVE_PART3, 10s);
                    break;
                }
                }
            else if (type == POINT_MOTION_TYPE)
            {
                switch (id)
                {
                case 1031: // ride to first wall
                {
                    m_events.ScheduleEvent(EVENT_MOVE_PART1, 10s);
                    break;
                }
                }
            }
            else if (type == EFFECT_MOTION_TYPE)
            {
                switch (m_movePart)
                {
                    case 2: // jump over wall
                    {
                        m_events.ScheduleEvent(EVENT_MOVE_PART2, 10s);
                        break;
                    }
                    case 4: // jump over bridge
                    {
                        m_events.ScheduleEvent(EVENT_MOVE_PART4, 10s);
                        break;
                    }
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
                case EVENT_START_WALK: // walk to the wall
                {
                    m_movePart = 1;
                    me->GetMotionMaster()->MovePoint(1031, -1735.01f, 1653.01f, 20.49f);
                    break;
                }
                case EVENT_MOVE_PART1: // jump over wall
                {
                    m_movePart = 2;
                    me->GetMotionMaster()->MoveJump(EVENT_JUMP, Position(-1714.02f, 1666.37f, 20.57f), 0.0f, 25.0f, 15.0f);
                    break;
                }
                case EVENT_MOVE_PART2:
                {
                    m_movePart = 3; // waypoint
                    me->GetMotionMaster()->MovePath(352311, false); // 0-16
                    break;
                }
                case EVENT_MOVE_PART3:
                {
                    m_movePart = 4; // jump over brige
                    me->GetMotionMaster()->MoveJump(EVENT_JUMP, Position(-1571.23f, 1710.034f, 20.485f), 0.0f, 25.0f, 15.0f);
                    break;
                }
                case EVENT_MOVE_PART4:
                {
                    m_movePart = 5; // to cathedrale
                    me->GetMotionMaster()->MovePath(352312, false); // 0-14
                    break;
                }
                case EVENT_MOVE_PART5:  // despawn
                {
                    if (Player* player = ObjectAccessor::GetPlayer(*me, m_playerGUID))
                        if (player->GetQuestStatus(QUEST_SACRIFICES) != QUEST_STATUS_COMPLETE)
                        {
                            player->FailQuest(QUEST_SACRIFICES);
                            player->NearTeleportTo(-1739.2f, 1657.9f, 20.48f, 0.5225f);
                        }
                    me->DespawnOrUnsummon(10s);
                    break;
                }
                case EVENT_TALK_PERIODIC:
                {
                    if (Player* player = ObjectAccessor::GetPlayer(*me, m_playerGUID))
                        if (Creature* darius = ObjectAccessor::GetCreature(*me, m_dariusGUID))
                            darius->AI()->Talk(1, player);
                    if (m_movePart < 5)
                        m_events.ScheduleEvent(EVENT_TALK_PERIODIC, 5000ms, 8000ms);
                    break;
                }
                }
            }

            UpdateVictim();
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

// 44468
class npc_sister_almyra_44468 : public CreatureScript
{
public:
    npc_sister_almyra_44468() : CreatureScript("npc_sister_almyra_44468") { }

    enum eNPC
    {
        EVENT_CAST_RENEW = 101,
    };

    struct npc_sister_almyra_44468AI : public ScriptedAI
    {
        npc_sister_almyra_44468AI(Creature* creature) : ScriptedAI(creature) { }

        EventMap m_events;

        void Reset() override
        {
            m_events.Reset();
            m_events.ScheduleEvent(EVENT_CAST_RENEW, 1000ms);
        }

        void UpdateAI(uint32 diff) override
        {
            m_events.Update(diff);

            while (uint32 eventId = m_events.ExecuteEvent())
            {
                switch (eventId)
                {
                    case EVENT_CAST_RENEW:
                    {
                        if (Player* player = me->SelectRandomPlayerInRange(40.0f, true))
                            if (!player->HasAura(SPELL_RENEW))
                                me->CastSpell(player, SPELL_RENEW);

                        m_events.ScheduleEvent(EVENT_CAST_RENEW, 5000ms);
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
        return new npc_sister_almyra_44468AI(creature);
    }
};

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

/* Phase 1024 - START */

// 35566
class npc_lord_darius_crowley_35566 : public CreatureScript
{
public:
    npc_lord_darius_crowley_35566() : CreatureScript("npc_lord_darius_crowley_35566") {}

    // ID - 72799 Last Stand Complete (bind 4786) 72788 95758 95759 72792 68992 68975 68978 68976

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
                        player->CastSpell(player, 72799, true);
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

// 35627
class npc_frenzied_stalker_35627 : public CreatureScript
{
public:
    npc_frenzied_stalker_35627() : CreatureScript("npc_frenzied_stalker_35627") {}

    enum eNpc
    {
        EVENT_CHECK_SHOWFIGHT = 101,
        EVENT_ENRAGE_COOLDOWN,
        MOVE_TO_HOMEPOSITION,
    };

    struct npc_frenzied_stalker_35627AI : public ScriptedAI
    {
        npc_frenzied_stalker_35627AI(Creature* creature) : ScriptedAI(creature) {}

        EventMap m_events;
        bool m_enrage;

        void Reset() override
        {
            me->SetUnitFlag(UNIT_FLAG_IMMUNE_TO_NPC);
            m_enrage = false;
            m_events.Reset();
            m_events.ScheduleEvent(EVENT_CHECK_SHOWFIGHT, 1000ms);
        }

        void DamageTaken(Unit* /*who*/, uint32& /*damage*/, DamageEffectType /*damageType*/, SpellInfo const* /*spellInfo = nullptr*/) override
        {
            if (!m_enrage && me->GetHealthPct() < 30.0f)
            {
                me->CastSpell(me, SPELL_ENRAGE_8599);
                Talk(1);
                m_enrage = true;
                m_events.ScheduleEvent(EVENT_ENRAGE_COOLDOWN, 121000ms, 150000ms);
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
                }
            }

            if (!UpdateVictim())
                return;
        }
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return new npc_frenzied_stalker_35627AI(creature);
    }
};

// 41015
class npc_northgate_rebel_41015 : public CreatureScript
{
public:
    npc_northgate_rebel_41015() : CreatureScript("npc_northgate_rebel_41015") { }

    enum eNPC
    {
        EVENT_CHECK_SHOWFIGHT = 101,
        MOVE_TO_HOMEPOSITION,
    };

    struct npc_northgate_rebel_41015AI : public ScriptedAI
    {
        npc_northgate_rebel_41015AI(Creature* creature) : ScriptedAI(creature) { Initialize(); }

        EventMap m_events;
        float    m_minHealthPct;
        bool     m_isShowFight;
        uint32    m_minDamage;
        uint32    m_maxDamage;
        std::list<uint32> m_targetList;

        void Initialize()
        {
            m_targetList.clear();
            m_targetList.push_back(NPC_FRENZIED_STALKER);
            m_minHealthPct = frand(55.0f, 85.0f);
            m_minDamage = 1;
            m_maxDamage = 1;
        }

        void Reset() override
        {
            me->SetUnitFlag(UNIT_FLAG_IMMUNE_TO_NPC);
            m_isShowFight = true;
            m_events.Reset();
            m_events.ScheduleEvent(EVENT_CHECK_SHOWFIGHT, 1000ms);
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
                        damage = 0;
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
                    m_events.ScheduleEvent(EVENT_CHECK_SHOWFIGHT, 2500ms);
                    if (me->IsInCombat() || !me->IsAlive())
                        return;

                    if (me->GetDistance2d(me->GetHomePosition().GetPositionX(), me->GetHomePosition().GetPositionY()) > 8.0f)
                    {
                        me->GetMotionMaster()->MovePoint(MOVE_TO_HOMEPOSITION, me->GetHomePosition());
                        return;
                    }

                    for (uint32 targetEntry : m_targetList)
                    {
                        if (Creature* worgen = me->FindNearestCreature(targetEntry, 10.0f))
                        {
                            me->Attack(worgen, true);
                            worgen->Attack(me, true);
                            return;
                        }
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
        return new npc_northgate_rebel_41015AI(creature);
    }
};

// next part in script zone_duskhaven

void AddSC_zone_gilneas_city1()
{
    new player_zone_gilneas_city1();
    new npc_gilneas_city_guard_gate_34864();
    new npc_rampaging_worgen_35660();
    new npc_gilneas_city_guard_34916();
    new npc_prince_liam_greymane_34913();
    new go_merchant_square_door_195327();
    new npc_rampaging_worgen_34884();
    new npc_frightened_citizen_34981();
    new npc_frightened_citizen_35836();
    new npc_gwen_armstead_35840();
    new npc_sergeant_cleese_35839();
    new npc_gilnean_royal_guard_35232();
    new npc_wounded_guard_47091();
    new npc_mariam_spellwalker_35872();

    //QUEST: 14154
    new npc_lord_darius_crowley_35077();
    new npc_tobias_mistmantle_35124();
    new npc_worgen_runt_35188();
    new npc_worgen_alpha_35170();
    new npc_worgen_runt_35456();
    new npc_worgen_alpha_35167();

    //QUEST: 14159
    new npc_josiah_avery_35369();
    new npc_josiah_avery_35370();
    new npc_josiah_avery_trigger_50415();


    new npc_lorna_crowley_35378();
    new npc_bloodfang_lurker_35463();
    new npc_gilnean_mastiff_35631();
    new npc_lord_godfrey_35906();
    new npc_gilnean_city_guard_35504();
    new npc_king_genn_greymane_35550();
    new npc_king_greymanes_horse_35905();
    new npc_krennan_aranas_35907();
    new npc_commandeered_cannon_35914();
    new npc_bloodfang_stalker_35229();
    new npc_lord_darius_crowley_35552();
    new npc_sister_almyra_44468();
    new npc_crowley_horse_35231();
    new spell_throw_torch_67063();
    new npc_rebel_cannon_35317();
    new npc_northgate_rebel_36057();
    new npc_lord_darius_crowley_35566();
    new npc_northgate_rebel_41015();
    new npc_frenzied_stalker_35627();
    new npc_gilnean_crow();
};

