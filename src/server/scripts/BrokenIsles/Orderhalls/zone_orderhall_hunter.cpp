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
#include "Map.h"
#include "MotionMaster.h"
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

// ---------------------------------------------------------------------------------------------------------------------
// Leg 0: the flight to Shield's Rest.
//
// "Stolen Thunder" (41574) opens with Grif Wildheart flying the Hunter from Dalaran to the isle of Shield's Rest
// (objective 0 = kill-credit 104993). That scripted flight is absent from our world DB, so we credit the flight leg
// and drop the player at the tomb landing beside Grif and Prustaga when the quest is accepted, so the scenario can
// begin. (A real on-rails Huey flight is the intended presentation; this transfer is the functional stand-in.)
// Bound to Grif 106879 via creature_template.ScriptName, and to 41574 via quest_template_addon.ScriptName.
class ShieldsRestFlightEvent : public BasicEvent
{
public:
    explicit ShieldsRestFlightEvent(Player* player) : _player(player) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (_player->IsInWorld())
        {
            _player->KilledMonsterCredit(NPC_CREDIT_FLY_SHIELDS_REST);                 // objective 0
            _player->TeleportTo(MAP_SHIELDS_REST, 4803.4f, 78.0f, -2.5f, 1.38f);       // landing beside Grif/Prustaga
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
// Scenario 1068 director.
//
// Bound to Prustaga (104949), the tomb ally, via creature_template.ScriptName. Prustaga is set active so her AI keeps
// updating while the party roams the far end of the tomb. Each poll she reads the running InstanceScenario's current
// step and, once a player has reached that step's landmark, fires the step's game event to advance it. Firing is
// idempotent and gated inside the scenario on step==currentStep, so re-firing (or firing while a later step is
// current) is a no-op; a player who runs straight to the pad simply advances one soft step per poll until they reach
// the Volund gate. Volund's death (step 4) is driven by npc_warlord_volund below.
struct npc_prustaga_scenario_director : public ScriptedAI
{
    npc_prustaga_scenario_director(Creature* creature) : ScriptedAI(creature), _pollTimer(0) { }

    uint32 _pollTimer;

    void Reset() override
    {
        if (me->GetMap()->GetId() == MAP_SHIELDS_REST)
            me->setActive(true);
    }

    // Return the first live player on the map for whom pred() holds (a landmark-reached test), else nullptr.
    template <typename Pred>
    Player* FindReachedPlayer(Pred pred) const
    {
        for (auto const& ref : me->GetMap()->GetPlayers())
            if (Player* p = ref.GetSource())
                if (p->IsInWorld() && p->IsAlive() && pred(p))
                    return p;
        return nullptr;
    }

    void UpdateAI(uint32 diff) override
    {
        if (me->GetMap()->GetId() != MAP_SHIELDS_REST)
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
            case 0: // Making Introductions - anyone present at the landing meets Prustaga with Grif
                if (Player* p = FindReachedPlayer([](Player*) { return true; }))
                    GameEvents::Trigger(GE_MEET_PRUSTAGA, p, p);
                break;
            case 1: // Tomb Raider - reached the tomb entrance (Tombs Door 243522 @ y~169)
                if (Player* p = FindReachedPlayer([](Player* pl) { return pl->GetPositionY() >= 140.0f; }))
                    GameEvents::Trigger(GE_FIND_TOMB, p, p);
                break;
            case 2: // Volund's Hoard - Prustaga opens the door; the player has pushed into the tomb (passage @ y~283)
                if (Player* p = FindReachedPlayer([](Player* pl) { return pl->GetPositionY() >= 230.0f; }))
                    GameEvents::Trigger(GE_PROTECT_PRUSTAGA, p, p);
                break;
            case 3: // Every Nook and Cranny - reached the inner chamber (chest/pad/Volund @ x~4970-5010)
                if (Player* p = FindReachedPlayer([](Player* pl) { return pl->GetPositionX() >= 4900.0f; }))
                    GameEvents::Trigger(GE_SEARCH_TITAN, p, p);
                break;
            case 4: // Volund's Last Stand - completed when Warlord Volund dies (npc_warlord_volund)
                break;
            case 5: // Answering the Call - step onto the Relay Device pad to join Mimiron in Ulduar
                if (Player* p = FindReachedPlayer([](Player* pl) { return pl->GetDistance(RelayDevicePad) <= 9.0f; }))
                {
                    GameEvents::Trigger(GE_JOIN_MIMIRON, p, p); // completes the scenario -> 41574 objective 2
                    // Mimiron pulls everyone in the instance onward to the Creator's Workshop.
                    for (auto const& ref : me->GetMap()->GetPlayers())
                        if (Player* mp = ref.GetSource())
                            if (mp->IsInWorld())
                                mp->m_Events.AddEventAtOffset(new TitanWorkshopTransferEvent(mp), 4s);
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

    void JustDied(Unit* killer) override
    {
        if (me->GetMap()->GetId() != MAP_SHIELDS_REST)
            return;

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
    MAP_DALARAN_BROKEN_ISLE  = 1220,
    FACTION_MONSTER          = 16       // generic hostile Stormheim faction
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

// Scenario 1099 director, bound to the temple's Grif (106715). Mirrors the Shield's Rest director: it advances the
// scenario off the real beats - conversing with Thorim, clearing the vrykul, Prustaga's defeat (fired from her AI
// below), then the finale (bind Hati, wield Titanstrike, ride home).
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

    // Give every player in the instance the wolf Hati as a companion that fights at their side (the artifact grants
    // it permanently in retail; here it joins as a guardian for the finale and beyond).
    void GrantHati()
    {
        for (auto const& ref : me->GetMap()->GetPlayers())
            if (Player* p = ref.GetSource())
                if (p->IsInWorld())
                    if (Creature* hati = p->SummonCreature(NPC_HATI, *p, TEMPSUMMON_MANUAL_DESPAWN))
                        hati->GetMotionMaster()->MoveFollow(p, 2.0f, static_cast<float>(M_PI));
    }

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
            case 3: // Heart of Thunder - bind Hati's spirit to your own
                if (Player* p = AnyPlayer())
                {
                    GrantHati();
                    GameEvents::Trigger(GE_BIND_HATI, p, p);
                }
                break;
            case 4: // The Power of the Titans - wield Titanstrike (recovers the artifact -> objective 2)
                if (Player* p = AnyPlayer())
                {
                    for (auto const& ref : me->GetMap()->GetPlayers())
                        if (Player* mp = ref.GetSource())
                            if (mp->IsInWorld())
                                mp->KilledMonsterCredit(NPC_CREDIT_TITANSTRIKE);   // objective 2 for the whole party
                    GameEvents::Trigger(GE_WIELD_TITANSTRIKE, p, p);
                }
                break;
            case 5: // Odyssey's End - ride Huey back to Dalaran
                if (Player* p = AnyPlayer())
                {
                    GameEvents::Trigger(GE_RIDE_HUEY_HOME, p, p);
                    for (auto const& ref : me->GetMap()->GetPlayers())
                        if (Player* mp = ref.GetSource())
                            if (mp->IsInWorld())
                                mp->m_Events.AddEventAtOffset(new DalaranReturnEvent(mp), 4s);
                }
                break;
            default:
                break;
        }
    }
};

// Prustaga at the Temple of Storms (106744): scripted hostile, and her death completes scenario step 2 and grants
// the "Titanstrike recovered" credit as a safety net (the director also grants it at step 4). Bound to 106744 via
// creature_template.ScriptName.
struct npc_prustaga_temple : public ScriptedAI
{
    npc_prustaga_temple(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        if (me->GetMap()->GetId() == MAP_TEMPLE_OF_STORMS)
            me->SetFaction(FACTION_MONSTER); // this entry is friendly by default; here she is the antagonist
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
            GameEvents::Trigger(GE_DEFEAT_PRUSTAGA, player, player);
            GameEvents::Trigger(GE_DEFEAT_PRUSTAGA_SUB, player, player);
        }
    }
};

void AddSC_orderhall_hunter()
{
    // Quest
    new quest_stolen_thunder();
    new quest_creators_workshop();
    new quest_never_hunt_alone();

    // Creature
    RegisterCreatureAI(npc_grif_wildheart_flight);
    RegisterCreatureAI(npc_prustaga_scenario_director);
    RegisterCreatureAI(npc_warlord_volund);
    RegisterCreatureAI(npc_grif_temple_director);
    RegisterCreatureAI(npc_prustaga_temple);
}
