/*
 * DoomCore Scripts - Magister's Terrace 12.0.1 Revamp (Map 2811)
 * Magister Umbric (Entry 234148) — Escort/Narrative NPC
 *
 * Umbric guides the party through the dungeon with timed dialogue and scripted
 * reactions to boss encounters. Progresses via DoAction calls from instance script
 * and boss scripts.
 *
 * Waypoints:
 * 1. Entrance: intro dialogue
 * 2. Pre-Boss 1: Rommath hologram confrontation, Umbric detained
 * 3. Post-Boss 1: freed, moves toward library
 * 4. Library: commentary + librarian warning
 * 5. Pre-Boss 2: RP exchange with Seranel
 * 6. Post-Boss 2: ward removal, Void breach
 * 7. Pre-Boss 4: Cynosure attunement
 * 8. Post-dungeon: wrap-up
 */

#include "ScriptMgr.h"
#include "InstanceScript.h"
#include "magisters_terrace_12.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "ScriptedCreature.h"

enum UmbricEvents
{
    // Intro sequence
    EVENT_UMBRIC_INTRO_1        = 1,
    EVENT_UMBRIC_INTRO_2,

    // Pre-Boss 1 (Rommath hologram)
    EVENT_UMBRIC_ROMMATH_REACT,
    EVENT_UMBRIC_DETAINED,
    EVENT_UMBRIC_INCAPACITATE,

    // Post-Boss 1
    EVENT_UMBRIC_POST_BOSS1_1,
    EVENT_UMBRIC_POST_BOSS1_MOVE,

    // Library
    EVENT_UMBRIC_LIBRARY_1,
    EVENT_UMBRIC_LIBRARY_2,

    // Pre-Boss 2 (Seranel RP)
    EVENT_UMBRIC_SERANEL_1,
    EVENT_UMBRIC_SERANEL_TRIGGER,

    // Post-Boss 2
    EVENT_UMBRIC_POST_BOSS2_1,
    EVENT_UMBRIC_POST_BOSS2_2,
    EVENT_UMBRIC_POST_BOSS2_3,

    // Post-Boss 3
    EVENT_UMBRIC_POST_BOSS3,

    // Pre-Boss 4
    EVENT_UMBRIC_PRE_BOSS4,

    // Post-Boss 4 / Dungeon complete
    EVENT_UMBRIC_POST_BOSS4_1,
    EVENT_UMBRIC_POST_BOSS4_2,
    EVENT_UMBRIC_POST_BOSS4_3,
};

enum UmbricMovePoints
{
    POINT_ENTRANCE              = 1,
    POINT_PRE_BOSS1,
    POINT_POST_BOSS1,
    POINT_LIBRARY,
    POINT_PRE_BOSS2,
    POINT_POST_BOSS2,
    POINT_PRE_BOSS4,
    POINT_POST_BOSS4,
};

// Umbric waypoint positions (approximate dungeon layout)
static constexpr Position UmbricWaypoints[] =
{
    { 170.0f, -10.0f, 2.0f, 0.0f },   // POINT_ENTRANCE
    { 190.0f, 0.0f, 2.0f, 0.0f },     // POINT_PRE_BOSS1
    { 200.0f, 10.0f, 2.0f, 0.0f },    // POINT_POST_BOSS1
    { 220.0f, 20.0f, 4.0f, 0.0f },    // POINT_LIBRARY
    { 240.0f, 30.0f, 5.0f, 0.0f },    // POINT_PRE_BOSS2
    { 260.0f, 20.0f, 6.0f, 0.0f },    // POINT_POST_BOSS2
    { 290.0f, 10.0f, 8.0f, 0.0f },    // POINT_PRE_BOSS4
    { 310.0f, 5.0f, 8.0f, 0.0f },     // POINT_POST_BOSS4
};

// ============================================================================
// NPC AI: Magister Umbric
// ============================================================================
struct npc_magister_umbric_mt : public ScriptedAI
{
    npc_magister_umbric_mt(Creature* creature) : ScriptedAI(creature),
        _currentPhase(0) { }

    void Reset() override
    {
        me->SetReactState(REACT_PASSIVE);
        me->SetUnitFlag(UNIT_FLAG_IMMUNE_TO_NPC);
        _currentPhase = 0;
    }

    void DoAction(int32 action) override
    {
        switch (action)
        {
            case ACTION_UMBRIC_INTRO:
                _events.ScheduleEvent(EVENT_UMBRIC_INTRO_1, 2s);
                break;
            case ACTION_UMBRIC_BOSS1_INTRO:
                me->GetMotionMaster()->MovePoint(POINT_PRE_BOSS1, UmbricWaypoints[1]);
                _events.ScheduleEvent(EVENT_UMBRIC_ROMMATH_REACT, 8s);
                break;
            case ACTION_UMBRIC_BOSS1_DETAINED:
                _events.ScheduleEvent(EVENT_UMBRIC_DETAINED, 1s);
                break;
            case ACTION_UMBRIC_BOSS1_DONE:
                _events.ScheduleEvent(EVENT_UMBRIC_POST_BOSS1_1, 3s);
                break;
            case ACTION_UMBRIC_LIBRARY:
                me->GetMotionMaster()->MovePoint(POINT_LIBRARY, UmbricWaypoints[3]);
                _events.ScheduleEvent(EVENT_UMBRIC_LIBRARY_1, 10s);
                break;
            case ACTION_UMBRIC_BOSS2_INTRO:
                me->GetMotionMaster()->MovePoint(POINT_PRE_BOSS2, UmbricWaypoints[4]);
                _events.ScheduleEvent(EVENT_UMBRIC_SERANEL_1, 8s);
                break;
            case ACTION_UMBRIC_BOSS2_DONE:
                _events.ScheduleEvent(EVENT_UMBRIC_POST_BOSS2_1, 3s);
                break;
            case ACTION_UMBRIC_BOSS3_DONE:
                _events.ScheduleEvent(EVENT_UMBRIC_POST_BOSS3, 3s);
                break;
            case ACTION_UMBRIC_BOSS4_INTRO:
                me->GetMotionMaster()->MovePoint(POINT_PRE_BOSS4, UmbricWaypoints[6]);
                _events.ScheduleEvent(EVENT_UMBRIC_PRE_BOSS4, 6s);
                break;
            case ACTION_UMBRIC_BOSS4_DONE:
                _events.ScheduleEvent(EVENT_UMBRIC_POST_BOSS4_1, 3s);
                break;
            default:
                break;
        }
    }

    void MovementInform(uint32 type, uint32 id) override
    {
        if (type != POINT_MOTION_TYPE)
            return;

        switch (id)
        {
            case POINT_POST_BOSS1:
                DoAction(ACTION_UMBRIC_LIBRARY);
                break;
            case POINT_POST_BOSS2:
                DoAction(ACTION_UMBRIC_BOSS4_INTRO);
                break;
            default:
                break;
        }
    }

    void UpdateAI(uint32 diff) override
    {
        _events.Update(diff);

        while (uint32 eventId = _events.ExecuteEvent())
        {
            switch (eventId)
            {
                // === INTRO ===
                case EVENT_UMBRIC_INTRO_1:
                    Talk(SAY_UMBRIC_INTRO);
                    _events.ScheduleEvent(EVENT_UMBRIC_INTRO_2, 6s);
                    break;
                case EVENT_UMBRIC_INTRO_2:
                    Talk(SAY_UMBRIC_ITEMS);
                    me->GetMotionMaster()->MovePoint(POINT_ENTRANCE, UmbricWaypoints[0]);
                    break;

                // === PRE-BOSS 1 ===
                case EVENT_UMBRIC_ROMMATH_REACT:
                    Talk(SAY_UMBRIC_SENTRY_REACT);
                    _events.ScheduleEvent(EVENT_UMBRIC_INCAPACITATE, 5s);
                    break;
                case EVENT_UMBRIC_INCAPACITATE:
                    Talk(SAY_UMBRIC_INCAPACITATE);
                    break;
                case EVENT_UMBRIC_DETAINED:
                    Talk(SAY_UMBRIC_DETAINED);
                    break;

                // === POST-BOSS 1 ===
                case EVENT_UMBRIC_POST_BOSS1_1:
                    Talk(SAY_UMBRIC_POST_BOSS1);
                    _events.ScheduleEvent(EVENT_UMBRIC_POST_BOSS1_MOVE, 4s);
                    break;
                case EVENT_UMBRIC_POST_BOSS1_MOVE:
                    me->GetMotionMaster()->MovePoint(POINT_POST_BOSS1, UmbricWaypoints[2]);
                    break;

                // === LIBRARY ===
                case EVENT_UMBRIC_LIBRARY_1:
                    Talk(SAY_UMBRIC_LIBRARY);
                    _events.ScheduleEvent(EVENT_UMBRIC_LIBRARY_2, 5s);
                    break;
                case EVENT_UMBRIC_LIBRARY_2:
                    Talk(SAY_UMBRIC_BEWARE_LIBRARIANS);
                    break;

                // === PRE-BOSS 2 (Seranel RP) ===
                case EVENT_UMBRIC_SERANEL_1:
                    Talk(SAY_UMBRIC_SERANEL_CONFRONT);
                    _events.ScheduleEvent(EVENT_UMBRIC_SERANEL_TRIGGER, 3s);
                    break;
                case EVENT_UMBRIC_SERANEL_TRIGGER:
                    if (InstanceScript* instance = me->GetInstanceScript())
                        if (Creature* seranel = instance->GetCreature(DATA_SERANEL_SUNLASH))
                            seranel->AI()->DoAction(ACTION_SERANEL_START_RP);
                    break;

                // === POST-BOSS 2 ===
                case EVENT_UMBRIC_POST_BOSS2_1:
                    Talk(SAY_UMBRIC_POST_BOSS2_1);
                    _events.ScheduleEvent(EVENT_UMBRIC_POST_BOSS2_2, 6s);
                    break;
                case EVENT_UMBRIC_POST_BOSS2_2:
                    Talk(SAY_UMBRIC_POST_BOSS2_2);
                    _events.ScheduleEvent(EVENT_UMBRIC_POST_BOSS2_3, 5s);
                    break;
                case EVENT_UMBRIC_POST_BOSS2_3:
                    Talk(SAY_UMBRIC_POST_BOSS2_3);
                    me->GetMotionMaster()->MovePoint(POINT_POST_BOSS2, UmbricWaypoints[5]);
                    break;

                // === POST-BOSS 3 ===
                case EVENT_UMBRIC_POST_BOSS3:
                    // Umbric reacts to Gemellus defeat, continues toward Degentrius
                    me->GetMotionMaster()->MovePoint(POINT_PRE_BOSS4, UmbricWaypoints[6]);
                    break;

                // === PRE-BOSS 4 ===
                case EVENT_UMBRIC_PRE_BOSS4:
                    Talk(SAY_UMBRIC_PRE_BOSS4);
                    break;

                // === POST-BOSS 4 / DUNGEON COMPLETE ===
                case EVENT_UMBRIC_POST_BOSS4_1:
                    Talk(SAY_UMBRIC_POST_BOSS4_1);
                    _events.ScheduleEvent(EVENT_UMBRIC_POST_BOSS4_2, 6s);
                    break;
                case EVENT_UMBRIC_POST_BOSS4_2:
                    Talk(SAY_UMBRIC_POST_BOSS4_2);
                    _events.ScheduleEvent(EVENT_UMBRIC_POST_BOSS4_3, 5s);
                    break;
                case EVENT_UMBRIC_POST_BOSS4_3:
                    Talk(SAY_UMBRIC_READY_TO_LEAVE);
                    me->GetMotionMaster()->MovePoint(POINT_POST_BOSS4, UmbricWaypoints[7]);
                    break;
                default:
                    break;
            }
        }
    }

private:
    EventMap _events;
    uint8 _currentPhase;
};

void AddSC_npc_magister_umbric_mt()
{
    RegisterMagistersTerrace12CreatureAI(npc_magister_umbric_mt);
}
