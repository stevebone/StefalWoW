/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
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

// Beast Mastery Hunter artifact acquisition ("Stolen Thunder" 41574 -> Titanstrike). Per-class Broken Isles content
// (cf. zone_orderhall_warrior.cpp); the generic class-hall framework lives in orderhall_legion.cpp.
//
// The acquisition is a three-map questline driven by two authored TrinityCore InstanceScenarios (the Legion artifact
// scenarios ship as empty placeholder content in our world DB, so we wire the scenario-step progression here):
//
//   Dalaran --(Grif's flight)--> Shield's Rest (map 1495, SCENARIO 1068 "Thunder of the Titans")
//        41574 "Stolen Thunder"  -> recover Titanstrike from Warlord Volund's tomb, then the Relay Device pad pulls
//                                   the party to Keeper Mimiron in Ulduar.
//   Ulduar / Creator's Workshop (map 1579, no scenario)
//        42158 "The Creator's Workshop"  -> a scripted assist (kill-credit 106559), handed back to Mimiron who then
//                                           offers "Never Hunt Alone" (42185, the Temple of Storms leg 1099).
//
// TrinityCore scenario mechanics used below: each ScenarioStep's CriteriaTree resolves to a single Type-92
// (AnyoneTriggerGameEventScenario) criterion whose Asset is a GameEvent id. Firing that game event with a player in
// the scenario as the source (GameEvents::Trigger) routes through Player::UpdateCriteria -> Scenario::UpdateCriteria,
// completing the current step and advancing to the next. Completing the final step completes the scenario, which
// fires CriteriaType::CompleteScenario(1068) and thereby satisfies 41574's objective 2 (CriteriaTree 47548 ->
// Criteria 29856, Type 152 CompleteScenario, asset 1068). We drive those game events off the real in-tomb triggers
// (reaching landmarks, Warlord Volund's death, stepping onto the teleport pad).

#include "Creature.h"
#include "DB2Structure.h"
#include "EventProcessor.h"
#include "GameEventSender.h"
#include "GameObject.h"
#include "GameObjectAI.h"
#include "Log.h"
#include "Map.h"
#include "MotionMaster.h"
#include "MovementDefines.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "QuestDef.h"
#include "Scenario.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "ScriptMgr.h"
#include "SharedDefines.h"
#include "TemporarySummon.h"
#include "Unit.h"

enum StolenThunderData
{
    QUEST_STOLEN_THUNDER        = 41574,
    MAP_SHIELDS_REST            = 1495,   // Warlord Volund's tomb - scenario 1068 instance
    SCENARIO_THUNDER_TITANS     = 1068,
    NPC_PRUSTAGA_ALLY           = 104949, // the vrykul ally who guides you through the tomb (scenario director)
    NPC_WARLORD_VOLUND          = 104956, // the tomb's final boss (scenario step 4)
    NPC_CREDIT_FLY_SHIELDS_REST = 104993, // 41574 objective 0 "Fly with Grif to Shield's Rest"

    MAP_CREATORS_WORKSHOP       = 1579,   // Ulduar - where the Relay Device pad drops the party
    QUEST_CREATORS_WORKSHOP     = 42158,
    NPC_CREDIT_MIMIRON_ASSIST   = 106559  // 42158's single objective "Kill Credit: Mimiron Assisted"
};

// Scenario 1068 step game-event assets (CriteriaType 92). One per authored ScenarioStep, in OrderIndex order.
enum Scenario1068GameEvents
{
    GE_MEET_PRUSTAGA    = 49879, // step 0 "Making Introductions"     - meet Prustaga with Grif at the landing
    GE_FIND_TOMB        = 50630, // step 1 "Tomb Raider"              - find Warlord Volund's tomb entrance
    GE_PROTECT_PRUSTAGA = 49992, // step 2 "Volund's Hoard"           - Prustaga opens the tomb door
    GE_SEARCH_TITAN     = 49889, // step 3 "Every Nook and Cranny"    - search the tomb for Titanstrike
    GE_DEFEAT_VOLUND    = 50631, // step 4 "Volund's Last Stand"      - defeat Warlord Volund
    GE_JOIN_MIMIRON     = 50632  // step 5 "Answering the Call"       - use the Relay Device pad to join Mimiron
};

// Relay Device pad (GO 249717) at the back of the tomb - a scenery (type-5) teleporter enabled by the scenario at
// step 5. It is not client-clickable, so we complete step 5 by proximity when the player stands on it.
static constexpr Position RelayDevicePad = { 4970.3f, 297.5f, -37.5f, 0.0f };

// The escort: Prustaga leads the Hunter from the landing into the tomb, Grif (104904) at her side, both speaking the
// scenario's lines. She pauses at each beat for the player to catch up. Progression is gated on how far the PLAYER has
// pushed from the landing (below), NOT on the escort reaching a spot - so a mispathed escort can never block the run.
enum { NPC_GRIF_LANDING = 104904, NPC_IMAGE_OF_MIMIRON = 106557 };
static constexpr Position TombLanding = { 4803.4f, 78.0f, -2.5f, 1.38f };
static constexpr Position TombBeat1   = { 4810.0f, 165.0f, -11.0f, 1.40f }; // tomb entrance   (step 1 Tomb Raider)
static constexpr Position TombBeat2   = { 4844.0f, 285.0f, -29.0f, 1.40f }; // rune door       (step 2 Volund's Hoard)
static constexpr Position TombBeat3   = { 4945.0f, 297.0f, -37.0f, 3.10f }; // the hoard room  (step 3 Every Nook)
// Player distance-from-landing thresholds that fire steps 1/2/3 (monotonic, so rushing ahead still counts).
static constexpr float StepDist1 = 65.0f;
static constexpr float StepDist2 = 165.0f;
static constexpr float StepDist3 = 235.0f;

// ---------------------------------------------------------------------------------------------------------------------
// Leg 0: the real "Huey" flight to Shield's Rest (matches the retail sequence in the reference video).
//
// "Stolen Thunder" (41574) opens with Grif Wildheart flying the Hunter on his gyrocopter "Huey" from Dalaran to the
// isle of Shield's Rest (objective 0 = kill-credit 104993). We reproduce the RIDDEN flight with the taxi system:
// ActivateTaxiPathTo routes the player from Dalaran (node 1774) to Shield's Rest (node 1855), whose flight mount IS
// Huey (creature 109682), so the approach is ridden on Huey as in the video. When the flight lands, the tomb scenario
// instance (map 1495) is entered ("Dungeon Difficulty set to Normal Scenario"). If the taxi graph has no route, it
// falls back to a direct teleport so the questline can never stall.
// Bound to Grif 106879 via creature_template.ScriptName, and to 41574 via quest_template_addon.ScriptName.
enum HueyFlightData
{
    NPC_GRIF_DALARAN  = 106879,
    NPC_HUEY_MOUNT    = 109682,
    TAXI_DALARAN      = 1774,
    TAXI_SHIELDS_REST = 1855
};

// Waits for the Huey taxi flight to land, then enters the Shield's Rest scenario. Self-reschedules while the player is
// still airborne (so it works for any flight length); a generous try-cap force-enters if the taxi never reports landing.
class ShieldsRestArrivalEvent : public BasicEvent
{
public:
    ShieldsRestArrivalEvent(Player* player, uint8 tries) : _player(player), _tries(tries) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (!_player->IsInWorld())
            return true;

        if (_player->IsInFlight() && _tries < 60) // still riding Huey - check again shortly
        {
            _player->m_Events.AddEventAtOffset(new ShieldsRestArrivalEvent(_player, _tries + 1), 3s);
            return true;
        }

        if (_player->GetQuestStatus(QUEST_STOLEN_THUNDER) == QUEST_STATUS_INCOMPLETE)
        {
            _player->KilledMonsterCredit(NPC_CREDIT_FLY_SHIELDS_REST);            // objective 0
            _player->TeleportTo(MAP_SHIELDS_REST, 4803.4f, 78.0f, -2.5f, 1.38f);  // tomb landing beside Prustaga
        }
        return true;
    }

private:
    Player* _player;
    uint8   _tries;
};

class ShieldsRestFlightEvent : public BasicEvent
{
public:
    explicit ShieldsRestFlightEvent(Player* player) : _player(player) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (!_player->IsInWorld())
            return true;

        // Grif's flight lines (from the reference video).
        if (Creature* grif = _player->FindNearestCreature(NPC_GRIF_DALARAN, 80.0f))
        {
            grif->Say("Hop on and let's get moving - ol' Huey don't bite much!", LANG_UNIVERSAL);
            grif->Say("To Shield's Rest!", LANG_UNIVERSAL);
            grif->Say("Make yourself comfortable. It's a bit of a ride to Stormheim.", LANG_UNIVERSAL);
        }

        // Real ridden flight to Shield's Rest; the final leg into node 1855 is mounted on Huey (109682). Fall back to a
        // direct teleport if the taxi graph cannot route it.
        if (_player->ActivateTaxiPathTo({ TAXI_DALARAN, TAXI_SHIELDS_REST }, nullptr, 0, 0))
            _player->m_Events.AddEventAtOffset(new ShieldsRestArrivalEvent(_player, 0), 4s);
        else
        {
            _player->KilledMonsterCredit(NPC_CREDIT_FLY_SHIELDS_REST);
            _player->TeleportTo(MAP_SHIELDS_REST, 4803.4f, 78.0f, -2.5f, 1.38f);
        }
        return true;
    }

private:
    Player* _player;
};

struct npc_grif_wildheart_flight : public ScriptedAI
{
    npc_grif_wildheart_flight(Creature* creature) : ScriptedAI(creature) { }

    bool OnGossipHello(Player* player) override
    {
        // Only intercept while "Stolen Thunder" is still in progress; otherwise fall through to Grif's normal quest
        // gossip. Re-crediting the flight objective / re-porting is idempotent, so a status check is sufficient.
        if (player->GetQuestStatus(QUEST_STOLEN_THUNDER) == QUEST_STATUS_INCOMPLETE)
        {
            CloseGossipMenuFor(player);
            player->m_Events.AddEventAtOffset(new ShieldsRestFlightEvent(player), 500ms);
            return true;
        }
        return false;
    }
};

struct quest_stolen_thunder : QuestScript
{
    quest_stolen_thunder() : QuestScript("quest_stolen_thunder") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus == QUEST_STATUS_INCOMPLETE) // just accepted -> fly to Shield's Rest
            player->m_Events.AddEventAtOffset(new ShieldsRestFlightEvent(player), 1500ms);
    }
};

// ---------------------------------------------------------------------------------------------------------------------
// Leg 1 -> Leg 2 transfer: the Relay Device pad pulls the party from Volund's tomb (1495) to Keeper Mimiron in the
// Creator's Workshop (1579). Scheduled by the scenario director when scenario 1068's final step completes.
class TitanWorkshopTransferEvent : public BasicEvent
{
public:
    explicit TitanWorkshopTransferEvent(Player* player) : _player(player) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (_player->IsInWorld() && _player->GetMapId() == MAP_SHIELDS_REST)
            _player->TeleportTo(MAP_CREATORS_WORKSHOP, 2782.0f, 2546.0f, 364.0f, 3.5f); // beside Mimiron (106558)
        return true;
    }

private:
    Player* _player;
};

// ---------------------------------------------------------------------------------------------------------------------
// Scenario 1068 director + escort. Bound to Prustaga (104949) via creature_template.ScriptName; she is set active so
// her AI runs wherever the party is.
//
// She plays the retail beats rather than ticking steps off in place: a short spoken introduction at the landing (with
// Grif 104904), then she runs the Hunter into the tomb - Grif at her side, both speaking the scenario's lines - and
// pauses at each beat (tomb entrance -> rune door she "dispels" -> the hoard room) for the player to keep up. Warlord
// Volund's death (npc_warlord_volund) drives stage 4; at stage 5 she voices the betrayal and the Relay Device pad
// sends the party to Mimiron.
//
// Crucially the scenario STEPS advance on how far the PLAYER has pushed from the landing (StepDist1/2/3) and on the
// player reaching the pad - never on the escort reaching a spot - so if her path ever snags on the tomb geometry the
// run still completes. The escort is the show; the player's own progress is the gate.
struct npc_prustaga_scenario_director : public ScriptedAI
{
    npc_prustaga_scenario_director(Creature* creature) : ScriptedAI(creature) { }

    uint32 _pollTimer = 0;
    uint32 _introTimer = 0;
    uint8  _introLine = 0;
    bool   _escortStarted = false;
    uint8  _leadBeat = 0;       // beat Prustaga is currently walking to (1..3), 0 = not moving yet
    bool   _leadArrived = false;
    bool   _betrayed = false;
    bool   _volundSeen = false;
    bool   _volundDead = false;
    bool   _completed = false;
    uint8  _lastOrder = 255;
    uint32 _hbTimer = 0;
    ObjectGuid _grifGuid;

    void Reset() override
    {
        if (me->GetMap()->GetId() == MAP_SHIELDS_REST)
            me->setActive(true);
    }

    // First live player within `within` yards of `pos` (a "player has reached here" test), else nullptr.
    Player* PlayerNear(Position const& pos, float within) const
    {
        for (auto const& ref : me->GetMap()->GetPlayers())
            if (Player* p = ref.GetSource())
                if (p->IsInWorld() && p->IsAlive() && p->GetDistance(pos) <= within)
                    return p;
        return nullptr;
    }

    // First live player who has pushed at least `dist` yards from the landing (monotonic progress into the tomb).
    Player* PlayerBeyond(float dist) const
    {
        for (auto const& ref : me->GetMap()->GetPlayers())
            if (Player* p = ref.GetSource())
                if (p->IsInWorld() && p->IsAlive() && p->GetDistance(TombLanding) >= dist)
                    return p;
        return nullptr;
    }

    Creature* Grif() const { return ObjectAccessor::GetCreature(*me, _grifGuid); }
    void SaySelf(char const* text) { me->Say(text, LANG_UNIVERSAL); }
    void SayGrif(char const* text) { if (Creature* g = Grif()) g->Say(text, LANG_UNIVERSAL); }

    // Advance the scenario UI one step by hand. The scenario's game-event criteria do not fire on this server's data,
    // but Scenario::SetStepState/CompleteStep are public, so we mark the current step done and let CompleteStep move to
    // the next (and, on the final step, complete the scenario) - exactly what the criteria path would have done.
    void AdvanceScenario()
    {
        if (Scenario* s = me->GetScenario())
            if (ScenarioStepEntry const* cur = s->GetStep())
            {
                s->SetStepState(cur, SCENARIO_STEP_DONE);
                s->CompleteStep(cur);
                TC_LOG_INFO("scripts", "[Titanstrike 1068] advanced scenario past step order {}", cur->OrderIndex);
            }
    }

    void UpdateAI(uint32 diff) override
    {
        if (me->GetMap()->GetId() != MAP_SHIELDS_REST)
            return;

        if (_completed)
            return; // leg finished (scenario complete + transfer scheduled) - go quiet

        _pollTimer += diff;
        if (_pollTimer < 1000)
            return;
        _pollTimer = 0;

        Scenario* scenario = me->GetScenario();                            // only used for the diagnostic logs below
        ScenarioStepEntry const* step = scenario ? scenario->GetStep() : nullptr;
        uint8 const order = step ? step->OrderIndex : 0;
        if (order != _lastOrder)
        {
            TC_LOG_INFO("scripts", "[Titanstrike 1068] scenario step is now order {}", order);
            _lastOrder = order;
        }
        _hbTimer += 1000;
        if (_hbTimer >= 10000)
        {
            _hbTimer = 0;
            TC_LOG_INFO("scripts", "[Titanstrike 1068] director heartbeat: order={} introLine={} escortStarted={} leadBeat={} Prustaga({},{},{})",
                order, _introLine, _escortStarted, _leadBeat, int(me->GetPositionX()), int(me->GetPositionY()), int(me->GetPositionZ()));
        }

        Player* anyP = PlayerBeyond(0.0f); // any player on the isle
        if (!anyP)
            return;
        if (_grifGuid.IsEmpty())
            if (Creature* g = me->FindNearestCreature(NPC_GRIF_LANDING, 80.0f))
                _grifGuid = g->GetGUID();

        // --- Stage 0 "Making Introductions": a spoken exchange at the landing, then the escort sets off. NOTE: this
        //     server's scenario-step criteria do not advance on the game events, so the escort and the quest are
        //     driven from here directly (not off the scenario step); we still fire the game event in case the UI
        //     listens. The escort begins when the intro dialogue finishes, independent of the scenario step. ---
        if (!_escortStarted)
        {
            _introTimer += 1000;
            switch (_introLine)
            {
                case 0: SayGrif("Prustaga! Brought a friend to join us, as promised."); ++_introLine; break;
                case 1: if (_introTimer >= 4000) { SaySelf("I care nothing for friendship, soft-earthen - so long as this one helps us enter the tomb."); ++_introLine; } break;
                case 2: if (_introTimer >= 8000) { SayGrif("Pipe down, ghostie! We're after yer gun for a good cause."); ++_introLine; } break;
                case 3: if (_introTimer >= 11000) { SaySelf("Enough chatter. Move!"); GameEvents::Trigger(GE_MEET_PRUSTAGA, anyP, anyP); ++_introLine; } break;
                default: // speeches done: set the escort in motion
                {
                    _escortStarted = true;
                    _leadBeat = 1;
                    _leadArrived = false;
                    me->SetWalk(false);
                    me->GetMotionMaster()->Clear();
                    me->GetMotionMaster()->MovePoint(1, TombBeat1, true);
                    TC_LOG_INFO("scripts", "[Titanstrike 1068] escort START: Prustaga ({},{},{}) -> MovePoint beat1; movegen {}",
                        int(me->GetPositionX()), int(me->GetPositionY()), int(me->GetPositionZ()),
                        uint32(me->GetMotionMaster()->GetCurrentMovementGeneratorType()));
                    if (Creature* g = Grif())
                    {
                        g->SetWalk(false);
                        g->GetMotionMaster()->Clear(); // Grif is MovementType=2 (waypoint); drop it so he can follow
                        g->GetMotionMaster()->MovePoint(1, TombBeat1.GetPositionX() - 3.0f, TombBeat1.GetPositionY(), TombBeat1.GetPositionZ(), true);
                    }
                    AdvanceScenario(); // stage 0 "Making Introductions" complete
                    break;
                }
            }
            return;
        }

        // --- Escort pacing: Prustaga leads beat to beat, pausing for the player, speaking as she arrives. ---
        if (_leadBeat >= 1 && _leadBeat <= 3)
        {
            Position const& target = (_leadBeat == 1) ? TombBeat1 : (_leadBeat == 2 ? TombBeat2 : TombBeat3);
            // Self-heal: if her move to the beat was ever cancelled (she is idle short of it), re-issue it.
            if (!_leadArrived && me->GetDistance(target) > 5.0f
                && me->GetMotionMaster()->GetCurrentMovementGeneratorType() == IDLE_MOTION_TYPE)
            {
                me->GetMotionMaster()->MovePoint(_leadBeat, target, false); // straight-line fallback (no navmesh)
                if (Creature* g = Grif())
                    if (g->GetMotionMaster()->GetCurrentMovementGeneratorType() == IDLE_MOTION_TYPE)
                        g->GetMotionMaster()->MovePoint(_leadBeat, target.GetPositionX() - 3.0f, target.GetPositionY(), target.GetPositionZ(), false);
            }
            if (!_leadArrived && me->GetDistance(target) <= 5.0f)
            {
                _leadArrived = true;
                if (_leadBeat == 1)      SayGrif("There, up ahead - the tomb of Warlord Volund. Watch yerself.");
                else if (_leadBeat == 2) SaySelf("I will dispel the runes on this door. Protect me!");
                else
                {
                    SaySelf("Titanstrike is close.");
                    SayGrif("By Magni's beard! Volund had a taste for titan relics!");
                    // The Image of Mimiron flickers to life at the Titan Chest and reads out the acquisition beat.
                    if (Creature* mimiron = me->FindNearestCreature(NPC_IMAGE_OF_MIMIRON, 140.0f))
                        mimiron->Say("Oh my stars! Yes, this is it! Titanstrike - forged to focus the fury of the storms!", LANG_UNIVERSAL);
                    SayGrif("There it is! Mission accomplished!");
                }
            }
            // Move on to the next beat only once the player has pushed far enough in behind us.
            if (_leadArrived && _leadBeat < 3)
            {
                float const gate = (_leadBeat == 1) ? StepDist1 : StepDist2;
                if (PlayerBeyond(gate))
                {
                    ++_leadBeat;
                    _leadArrived = false;
                    Position const& next = (_leadBeat == 2) ? TombBeat2 : TombBeat3;
                    me->GetMotionMaster()->MovePoint(_leadBeat, next, true);
                }
            }
        }

        // Watch for Warlord Volund's death once the party is deep enough that his chamber is loaded around us.
        if (_leadBeat >= 3 || PlayerBeyond(StepDist3 - 30.0f))
        {
            if (me->FindNearestCreature(NPC_WARLORD_VOLUND, 300.0f, true))
                _volundSeen = true;
            else if (_volundSeen)
                _volundDead = true;
        }

        // --- Drive the scenario stages by the player's own progress, advancing each via the public step API. ---
        switch (order)
        {
            case 1: // Tomb Raider -> reached the tomb entrance
                if (PlayerBeyond(StepDist1))
                    AdvanceScenario();
                break;
            case 2: // Volund's Hoard -> Prustaga dispelled the runes, the door is open
                if (PlayerBeyond(StepDist2))
                {
                    SayGrif("Look! The door's openin'!");
                    AdvanceScenario();
                }
                break;
            case 3: // Every Nook and Cranny -> reached the hoard room
                if (PlayerBeyond(StepDist3))
                    AdvanceScenario();
                break;
            case 4: // Volund's Last Stand -> he is defeated; the betrayal plays, then the stage advances
                if (_volundDead)
                {
                    if (!_betrayed)
                    {
                        _betrayed = true;
                        SaySelf("You've played your part. Now I shall play mine! The heart of this weapon will make me a titan!");
                        SayGrif("Prustaga, no! ...We can't stop now - to the pad, quickly!");
                    }
                    AdvanceScenario();
                }
                break;
            case 5: // Answering the Call -> step onto the Relay Device pad to join Mimiron in Ulduar
                if (!_completed && PlayerNear(RelayDevicePad, 15.0f))
                {
                    _completed = true;
                    AdvanceScenario(); // final step -> CompleteScenario (fires the CompleteScenario criteria)
                    TC_LOG_INFO("scripts", "[Titanstrike 1068] finale: pad reached -> complete + transfer to Ulduar");
                    for (auto const& ref : me->GetMap()->GetPlayers())
                        if (Player* mp = ref.GetSource())
                            if (mp->IsInWorld())
                            {
                                if (mp->GetQuestStatus(QUEST_STOLEN_THUNDER) == QUEST_STATUS_INCOMPLETE)
                                    mp->CompleteQuest(QUEST_STOLEN_THUNDER); // safety net: ready to hand in to Mimiron
                                mp->m_Events.AddEventAtOffset(new TitanWorkshopTransferEvent(mp), 4s);
                            }
                }
                break;
            default:
                break;
        }
    }
};

// Warlord Volund - scenario 1068 step 4. His death fires the "Defeat Warlord Volund" game event, advancing the
// scenario to the final step (the Relay Device pad). Bound to 104956 via creature_template.ScriptName.
struct npc_warlord_volund : public ScriptedAI
{
    npc_warlord_volund(Creature* creature) : ScriptedAI(creature) { }

    void JustEngagedWith(Unit* /*who*/) override
    {
        if (me->GetMap()->GetId() == MAP_SHIELDS_REST)
            me->Yell("Thieves! Your heads will decorate my throne! You of flesh are not worthy to wield the storms!", LANG_UNIVERSAL);
    }

    void JustDied(Unit* killer) override
    {
        if (me->GetMap()->GetId() != MAP_SHIELDS_REST)
            return;

        me->Yell("You... cannot... control...", LANG_UNIVERSAL);

        Player* player = killer ? killer->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr;
        if (!player) // pet/environmental kill - fall back to any player in the instance
            for (auto const& ref : me->GetMap()->GetPlayers())
                if (Player* p = ref.GetSource())
                {
                    player = p;
                    break;
                }

        if (player)
            GameEvents::Trigger(GE_DEFEAT_VOLUND, player, player);
    }
};

// ---------------------------------------------------------------------------------------------------------------------
// Leg 2: "The Creator's Workshop" (42158), Keeper Mimiron in Ulduar (map 1579).
//
// The party arrives via the Relay Device pad already holding a completed "Stolen Thunder" to turn in to Mimiron, who
// then offers 42158. Its single objective is a scripted assist (kill-credit 106559 "Mimiron Assisted"), not combat;
// we grant it shortly after the quest is accepted so it can be handed straight back to Mimiron, who in turn offers
// "Never Hunt Alone" (42185). Bound to 42158 via quest_template_addon.ScriptName.
class CreatorsWorkshopAssistEvent : public BasicEvent
{
public:
    explicit CreatorsWorkshopAssistEvent(Player* player) : _player(player) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (_player->IsInWorld())
            _player->KilledMonsterCredit(NPC_CREDIT_MIMIRON_ASSIST);
        return true;
    }

private:
    Player* _player;
};

struct quest_creators_workshop : QuestScript
{
    quest_creators_workshop() : QuestScript("quest_creators_workshop") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus == QUEST_STATUS_INCOMPLETE) // just accepted -> assist Mimiron
            player->m_Events.AddEventAtOffset(new CreatorsWorkshopAssistEvent(player), 2s);
    }
};

// =====================================================================================================================
// Leg 3: "Never Hunt Alone" (42185) - the Temple of Storms (map 1609), scenario 1099.
//
// Mimiron sends the Hunter after Prustaga, who fled to Thorim's Temple of Storms with Titanstrike. This leg is
// driven by InstanceScenario 1099 "Never Hunt Alone" (linked to map 1609 via the `scenarios` table). The temple in
// our world DB has only the allies spawned (Grif 106715, Thorim 106714, Hati 103154) - the antagonists are missing,
// so we spawn Prustaga (106744, repurposed - it spawns nowhere else) and a small vrykul horde (106302) and script
// them hostile. The quest itself completes on three kill-credits (106671 on accept, 106672 on arrival, 114509 on
// Prustaga's defeat); the scenario runs alongside for the on-screen step presentation, so a scenario hiccup can
// never strand the questline.
enum NeverHuntAloneData
{
    QUEST_NEVER_HUNT_ALONE   = 42185,
    MAP_TEMPLE_OF_STORMS     = 1609,
    NPC_GRIF_TEMPLE          = 106715, // temple ally + scenario 1099 director
    NPC_THORIM               = 106714, // Lord of Thunder - converse (step 0)
    NPC_HATI                 = 103154, // the wolf companion bound to Titanstrike (step 3)
    NPC_PRUSTAGA_TEMPLE      = 106744, // the traitor, defeated here (step 2); repurposed spawn
    NPC_VRYKUL_HORDE         = 106302, // Restless Tombguard - the vrykul adds (step 1)
    NPC_CREDIT_MIMIRON_HEAD  = 106671, // 42185 objective 0
    NPC_CREDIT_FLY_TEMPLE    = 106672, // 42185 objective 1 "Fly to the Temple of Storms"
    NPC_CREDIT_TITANSTRIKE   = 114509, // 42185 objective 2 "Titanstrike recovered"
    MAP_DALARAN_BROKEN_ISLE  = 1220
};

// Scenario 1099 step game-event assets (CriteriaType 92), in OrderIndex order.
enum Scenario1099GameEvents
{
    GE_CONVERSE_THORIM    = 50895, // step 0 "Thorim, Lord of Thunder"
    GE_FEND_VRYKUL        = 50910, // step 1 "Battle is Joined"
    GE_DEFEAT_PRUSTAGA    = 50921, // step 2 "Madness of the Usurper"
    GE_DEFEAT_PRUSTAGA_SUB= 50997, // step 2 child criterion
    GE_BIND_HATI          = 50922, // step 3 "Heart of Thunder"
    GE_WIELD_TITANSTRIKE  = 50923, // step 4 "The Power of the Titans"
    GE_RIDE_HUEY_HOME     = 50924  // step 5 "Odyssey's End"
};

static constexpr Position ThorimThrone      = { 7450.0f, -535.3f, 1896.9f, 0.0f };
static constexpr Position TempleLanding     = { 7420.0f, -540.0f, 1897.0f, 3.0f };  // facing the throne
static constexpr Position DalaranReturn     = { -819.0f, 4300.0f, 746.0f, 4.6f };   // beside Grif 106879 in Dalaran

// Transfer from the Creator's Workshop (1579) to the Temple of Storms (1609) - the "Fly to the Temple of Storms" leg.
class TempleTransferEvent : public BasicEvent
{
public:
    explicit TempleTransferEvent(Player* player) : _player(player) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (_player->IsInWorld())
        {
            _player->KilledMonsterCredit(NPC_CREDIT_FLY_TEMPLE);                    // objective 1
            _player->TeleportTo(MAP_TEMPLE_OF_STORMS, TempleLanding.GetPositionX(), TempleLanding.GetPositionY(),
                TempleLanding.GetPositionZ(), TempleLanding.GetOrientation());
        }
        return true;
    }

private:
    Player* _player;
};

struct quest_never_hunt_alone : QuestScript
{
    quest_never_hunt_alone() : QuestScript("quest_never_hunt_alone") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus == QUEST_STATUS_INCOMPLETE) // accepted from Mimiron in Ulduar
        {
            player->KilledMonsterCredit(NPC_CREDIT_MIMIRON_HEAD);                   // objective 0 (carry Mimiron's head)
            player->m_Events.AddEventAtOffset(new TempleTransferEvent(player), 1500ms);
        }
    }
};

// Return to Dalaran once the temple scenario ends (step 5 "Ride Huey to return to Dalaran"), where "Never Hunt Alone"
// turns in to Grif Wildheart (106879), leading into the class-hall chain (41009 -> 40953 -> 40954/40955).
class DalaranReturnEvent : public BasicEvent
{
public:
    explicit DalaranReturnEvent(Player* player) : _player(player) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (_player->IsInWorld() && _player->GetMapId() == MAP_TEMPLE_OF_STORMS)
            _player->TeleportTo(MAP_DALARAN_BROKEN_ISLE, DalaranReturn.GetPositionX(), DalaranReturn.GetPositionY(),
                DalaranReturn.GetPositionZ(), DalaranReturn.GetOrientation());
        return true;
    }

private:
    Player* _player;
};

// Give every player at the temple the wolf Hati as a companion at their side (the artifact grants it permanently in
// retail; here it joins as a guardian for the finale and beyond).
static void GrantHatiCompanion(Map* map)
{
    for (auto const& ref : map->GetPlayers())
        if (Player* p = ref.GetSource())
            if (p->IsInWorld())
                if (Creature* hati = p->SummonCreature(NPC_HATI, *p, TEMPSUMMON_MANUAL_DESPAWN))
                    hati->GetMotionMaster()->MoveFollow(p, 2.0f, static_cast<float>(M_PI));
}

// Scenario 1099 director, bound to the temple's Grif (106715). Mirrors the Shield's Rest director, but only fires the
// on-screen scenario step game events - the gameplay outcomes (recovering Titanstrike, gaining Hati, riding home) are
// driven authoritatively from Prustaga's death below, so the questline completes even if the scenario is unavailable.
struct npc_grif_temple_director : public ScriptedAI
{
    npc_grif_temple_director(Creature* creature) : ScriptedAI(creature), _pollTimer(0) { }

    uint32 _pollTimer;

    void Reset() override
    {
        if (me->GetMap()->GetId() == MAP_TEMPLE_OF_STORMS)
            me->setActive(true);
    }

    template <typename Pred>
    Player* FindReachedPlayer(Pred pred) const
    {
        for (auto const& ref : me->GetMap()->GetPlayers())
            if (Player* p = ref.GetSource())
                if (p->IsInWorld() && p->IsAlive() && pred(p))
                    return p;
        return nullptr;
    }

    Player* AnyPlayer() const { return FindReachedPlayer([](Player*) { return true; }); }

    void UpdateAI(uint32 diff) override
    {
        if (me->GetMap()->GetId() != MAP_TEMPLE_OF_STORMS)
            return;

        _pollTimer += diff;
        if (_pollTimer < 1000)
            return;
        _pollTimer = 0;

        Scenario* scenario = me->GetScenario();
        if (!scenario)
            return;

        ScenarioStepEntry const* step = scenario->GetStep();
        if (!step)
            return;

        switch (step->OrderIndex)
        {
            case 0: // Thorim, Lord of Thunder - converse with Thorim at the throne
                if (Player* p = FindReachedPlayer([](Player* pl) { return pl->GetDistance(ThorimThrone) <= 25.0f; }))
                    GameEvents::Trigger(GE_CONVERSE_THORIM, p, p);
                break;
            case 1: // Battle is Joined - fend off the vrykul horde (advances once every add is down)
                if (!me->FindNearestCreature(NPC_VRYKUL_HORDE, 300.0f, true))
                    if (Player* p = AnyPlayer())
                        GameEvents::Trigger(GE_FEND_VRYKUL, p, p);
                break;
            case 2: // Madness of the Usurper - defeat Prustaga (fired from npc_prustaga_temple::JustDied)
                break;
            case 3: // Heart of Thunder - bind Hati's spirit (the companion is granted from Prustaga's death)
                if (Player* p = AnyPlayer())
                    GameEvents::Trigger(GE_BIND_HATI, p, p);
                break;
            case 4: // The Power of the Titans - wield Titanstrike (the credit is granted from Prustaga's death)
                if (Player* p = AnyPlayer())
                    GameEvents::Trigger(GE_WIELD_TITANSTRIKE, p, p);
                break;
            case 5: // Odyssey's End - ride Huey back to Dalaran (the return is scheduled from Prustaga's death)
                if (Player* p = AnyPlayer())
                    GameEvents::Trigger(GE_RIDE_HUEY_HOME, p, p);
                break;
            default:
                break;
        }
    }
};

// Prustaga at the Temple of Storms (106744): scripted hostile, and the authoritative completion point for leg 3. Her
// death completes scenario step 2 (on-screen) and, independently of the scenario, recovers Titanstrike (objective 2),
// grants the Hati companion, and rides the party home to Dalaran a few seconds later (after the finale beats). Bound
// to 106744 via creature_template.ScriptName.
struct npc_prustaga_temple : public ScriptedAI
{
    npc_prustaga_temple(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        if (me->GetMap()->GetId() == MAP_TEMPLE_OF_STORMS)
            me->SetFaction(FACTION_MONSTER_2); // faction 16; this entry is friendly by default, here the antagonist
    }

    void JustDied(Unit* killer) override
    {
        if (me->GetMap()->GetId() != MAP_TEMPLE_OF_STORMS)
            return;

        Player* player = killer ? killer->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr;
        if (!player)
            for (auto const& ref : me->GetMap()->GetPlayers())
                if (Player* p = ref.GetSource())
                {
                    player = p;
                    break;
                }

        if (player)
        {
            GameEvents::Trigger(GE_DEFEAT_PRUSTAGA, player, player);      // scenario step 2 "Madness of the Usurper"
            GameEvents::Trigger(GE_DEFEAT_PRUSTAGA_SUB, player, player);
        }

        // Authoritative outcome: recover Titanstrike (objective 2) and return home a few seconds later, so the leg
        // completes whether or not scenario 1099 is running its on-screen steps.
        for (auto const& ref : me->GetMap()->GetPlayers())
            if (Player* p = ref.GetSource())
                if (p->IsInWorld())
                {
                    p->KilledMonsterCredit(NPC_CREDIT_TITANSTRIKE);                        // objective 2
                    p->m_Events.AddEventAtOffset(new DalaranReturnEvent(p), 12s);          // after the finale beats
                }

        GrantHatiCompanion(me->GetMap());
    }
};

// =====================================================================================================================
// Marksmanship artifact: "Call of the Marksman" (40392) -> Thas'dorah, Legacy of the Windrunners.
//
// A kill-credit / flight quest (NOT a scenario): fly from Trueshot Lodge to the Broken Shore (obj 0, credit 102173),
// then speak to Vereesa Windrunner (obj 1, credit 103602) who hands over the bow. All the real NPCs are already
// spawned (Vereesa 100190 at the Broken Shore landing), so we just script the flight + credits. The flight is a
// teleport stand-in (a real gyrocopter path is polish); on arrival beside Vereesa both the flight and "spoke to
// Vereesa" credits are granted, leaving the quest ready to hand in to her. Bound to 40392 via addon ScriptName.
enum MarksmanArtifact
{
    QUEST_CALL_OF_THE_MARKSMAN = 40392,
    CREDIT_FLY_BROKEN_SHORE    = 102173,
    CREDIT_TALK_VEREESA        = 103602
};
static constexpr Position BrokenShoreLanding = { -835.0f, 3679.0f, 26.3f, 4.24f }; // beside Vereesa (100190)

class MarksmanFlightEvent : public BasicEvent
{
public:
    explicit MarksmanFlightEvent(Player* player) : _player(player) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (_player->IsInWorld())
        {
            _player->KilledMonsterCredit(CREDIT_FLY_BROKEN_SHORE);   // obj 0
            _player->TeleportTo(1220, BrokenShoreLanding.GetPositionX(), BrokenShoreLanding.GetPositionY(),
                BrokenShoreLanding.GetPositionZ(), BrokenShoreLanding.GetOrientation());
            _player->KilledMonsterCredit(CREDIT_TALK_VEREESA);       // obj 1 (arrives beside Vereesa)
        }
        return true;
    }

private:
    Player* _player;
};

struct quest_call_of_the_marksman : QuestScript
{
    quest_call_of_the_marksman() : QuestScript("quest_call_of_the_marksman") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus == QUEST_STATUS_INCOMPLETE)
            player->m_Events.AddEventAtOffset(new MarksmanFlightEvent(player), 1500ms);
    }
};

// =====================================================================================================================
// Survival artifact: "The Eagle Spirit's Blessing" (39427) -> Talonclaw.
//
// Fly to Spiritwatch Point in Highmountain (obj 0/1, credits 110936/110937), slay Degar Bloodtotem (obj 2, kill 110685),
// and receive Ohn'ahra's blessing (obj 3, credit 110938), then hand in to Apata Highmountain (110821). Nothing on the
// Spiritwatch path is spawned, so we place Degar + Apata near Skyhorn (the nearest populated part of northern
// Highmountain - exact Spiritwatch coords aren't derivable offline). Degar is a faction-35 placeholder, so the script
// makes him hostile, and his death both credits obj 2 and grants Ohn'ahra's blessing (obj 3). Bound to 39427 + Degar.
enum SurvivalArtifact
{
    QUEST_EAGLE_SPIRITS_BLESSING = 39427,
    NPC_DEGAR_BLOODTOTEM         = 110685,
    CREDIT_FLY_SPIRITWATCH       = 110936,
    CREDIT_ENTER_SPIRITWATCH     = 110937,
    CREDIT_EAGLE_BLESSING        = 110938
};
static constexpr Position SpiritwatchPoint = { 4757.0f, 3928.0f, 809.0f, 3.0f }; // near Skyhorn, Highmountain (map 1220)

class SpiritwatchFlightEvent : public BasicEvent
{
public:
    explicit SpiritwatchFlightEvent(Player* player) : _player(player) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (_player->IsInWorld())
        {
            _player->KilledMonsterCredit(CREDIT_FLY_SPIRITWATCH);    // obj 0
            _player->TeleportTo(1220, SpiritwatchPoint.GetPositionX(), SpiritwatchPoint.GetPositionY(),
                SpiritwatchPoint.GetPositionZ(), SpiritwatchPoint.GetOrientation());
            _player->KilledMonsterCredit(CREDIT_ENTER_SPIRITWATCH);  // obj 1 (arrived at Spiritwatch Point)
        }
        return true;
    }

private:
    Player* _player;
};

struct quest_eagle_spirits_blessing : QuestScript
{
    quest_eagle_spirits_blessing() : QuestScript("quest_eagle_spirits_blessing") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus == QUEST_STATUS_INCOMPLETE)
            player->m_Events.AddEventAtOffset(new SpiritwatchFlightEvent(player), 1500ms);
    }
};

// Degar Bloodtotem (110685): the Survival obj-2 boss. Placeholder faction 35 -> made hostile; his death credits the
// kill and grants Ohn'ahra's blessing (obj 3), leaving 39427 ready to hand in to Apata (110821).
struct npc_degar_bloodtotem : public ScriptedAI
{
    npc_degar_bloodtotem(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override { me->SetFaction(FACTION_MONSTER_2); } // faction 16 - attackable Survival boss

    void JustDied(Unit* killer) override
    {
        Player* player = killer ? killer->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr;
        if (!player)
            for (auto const& ref : me->GetMap()->GetPlayers())
                if (Player* p = ref.GetSource()) { player = p; break; }
        if (player)
            player->KilledMonsterCredit(CREDIT_EAGLE_BLESSING); // obj 3 (kill of 110685 itself credits obj 2)
    }
};

// Stormweaver Ingrida (105122) - the Stage-2 "Tomb Raider" mini-boss of Volund's tomb. Placeholder faction 35 -> made
// hostile, with her engage/defeat lines from the reference video. Bound to 105122 via creature_template.ScriptName.
struct npc_stormweaver_ingrida : public ScriptedAI
{
    npc_stormweaver_ingrida(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        if (me->GetMap()->GetId() == MAP_SHIELDS_REST)
            me->SetFaction(FACTION_MONSTER_2); // faction 16 - attackable mini-boss
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        if (me->GetMap()->GetId() == MAP_SHIELDS_REST)
            me->Yell("You will find only death here! Flee now, and you may yet leave with your fragile lives.", LANG_UNIVERSAL);
    }

    void JustDied(Unit* /*killer*/) override
    {
        if (me->GetMap()->GetId() == MAP_SHIELDS_REST)
            me->Yell("My warlord... we have... guests...", LANG_UNIVERSAL);
    }
};

void AddSC_orderhall_hunter()
{
    // Quest
    new quest_stolen_thunder();
    new quest_creators_workshop();
    new quest_never_hunt_alone();
    new quest_call_of_the_marksman();
    new quest_eagle_spirits_blessing();

    // Creature
    RegisterCreatureAI(npc_grif_wildheart_flight);
    RegisterCreatureAI(npc_prustaga_scenario_director);
    RegisterCreatureAI(npc_stormweaver_ingrida);
    RegisterCreatureAI(npc_warlord_volund);
    RegisterCreatureAI(npc_grif_temple_director);
    RegisterCreatureAI(npc_prustaga_temple);
    RegisterCreatureAI(npc_degar_bloodtotem);
}
