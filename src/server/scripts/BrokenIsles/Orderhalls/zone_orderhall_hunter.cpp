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
#include "InstanceScenario.h"
#include "Map.h"
#include "Player.h"
#include "QuestDef.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "ScriptMgr.h"
#include "SharedDefines.h"
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

        InstanceScenario* scenario = me->GetMap()->GetInstanceScenario();
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

void AddSC_orderhall_hunter()
{
    // Quest
    new quest_stolen_thunder();
    new quest_creators_workshop();

    // Creature
    RegisterCreatureAI(npc_grif_wildheart_flight);
    RegisterCreatureAI(npc_prustaga_scenario_director);
    RegisterCreatureAI(npc_warlord_volund);
}
