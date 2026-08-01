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

// Beast Mastery Hunter artifact quest "Stolen Thunder" (41574) -> recovering Titanstrike from Warlord Volund's tomb on
// Shield's Rest, the Prustaga betrayal, and the Mimiron transfer to the Creator's Workshop. Per-class Broken Isles
// content (cf. zone_orderhall_warrior.cpp). The generic class-hall framework lives in orderhall_legion.cpp.
#include "Creature.h"
#include "EventProcessor.h"
#include "GameObject.h"
#include "GameObjectAI.h"
#include "Player.h"
#include "QuestDef.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "ScriptMgr.h"
#include "SharedDefines.h"
#include "Unit.h"


// ---------------------------------------------------------------------------------------------------------------------
// Hunter Beast Mastery artifact on-ramp: the flight to Shield's Rest.
//
// Quest "Stolen Thunder" (41574) sends the Hunter to recover Titanstrike from Warlord Volund's tomb on the isle of
// Shield's Rest - a fully populated sub-map (1495: Grif, Prustaga, Warlord Volund are all spawned there). In retail
// you talk to Grif Wildheart and "Huey" flies you across (quest objective 1 = kill credit 104993). That scripted
// flight is absent from our world DB, so the quest strands the player in Dalaran with no transport. Talking to Grif
// while the flight leg is outstanding credits it and drops the player at the Shield's Rest landing beside Grif /
// Prustaga so the scenario (tomb -> Titanstrike) can continue. Bound to Grif 106879 via creature_template.ScriptName.
enum StolenThunder
{
    QUEST_STOLEN_THUNDER            = 41574,
    NPC_CREDIT_FLY_TO_SHIELDS_REST  = 104993,
    NPC_PRUSTAGA                    = 104949,   // the vrykul "ally" who betrays you and snatches Titanstrike
    MAP_STORMHEIM_SHIELDS_REST      = 1495
};

struct npc_grif_wildheart_flight : public ScriptedAI
{
    npc_grif_wildheart_flight(Creature* creature) : ScriptedAI(creature) { }

    bool OnGossipHello(Player* player) override
    {
        // Only intercept while the "fly to Shield's Rest" leg of Stolen Thunder is still outstanding; otherwise fall
        // through to Grif's normal quest gossip (offering/other states).
        if (player->GetQuestStatus(QUEST_STOLEN_THUNDER) == QUEST_STATUS_INCOMPLETE)
        {
            CloseGossipMenuFor(player);
            player->KilledMonsterCredit(NPC_CREDIT_FLY_TO_SHIELDS_REST);           // completes objective "Fly with Grif to Shield's Rest"
            player->TeleportTo(MAP_STORMHEIM_SHIELDS_REST, 4803.4f, 78.0f, -2.5f, 1.38f);
            return true;
        }

        return false;
    }
};

// Robust, phase-independent flight trigger for "Stolen Thunder" (41574). Talking to Grif is unreliable at this step -
// there are several Grif spawns with mixed phasing plus a SmartAI one, so the gossip interaction doesn't consistently
// reach the bound creature. Also fly the player when they ACCEPT the quest: credit the flight objective (104993) and
// drop them at the Shield's Rest landing. Deferred ~1.5s so the accept flow settles before the map change. Bound via
// quest_template_addon.ScriptName; the npc_grif_wildheart_flight gossip above still works whenever the player does
// reach an interactable bound Grif.
class StolenThunderFlightEvent : public BasicEvent
{
public:
    explicit StolenThunderFlightEvent(Player* player) : _player(player) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (_player->IsInWorld())
        {
            _player->KilledMonsterCredit(NPC_CREDIT_FLY_TO_SHIELDS_REST);
            _player->TeleportTo(MAP_STORMHEIM_SHIELDS_REST, 4803.4f, 78.0f, -2.5f, 1.38f);
        }
        return true;
    }

private:
    Player* _player;
};

struct quest_stolen_thunder : QuestScript
{
    quest_stolen_thunder() : QuestScript("quest_stolen_thunder") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus == QUEST_STATUS_INCOMPLETE)   // just accepted -> take the flight to Shield's Rest
            player->m_Events.AddEventAtOffset(new StolenThunderFlightEvent(player), 1500ms);
    }
};

// Deferred cross-map transfer to the Creator's Workshop (map 1579), where "Stolen Thunder" turns in to Mimiron and
// the Hati questline continues. The Shield's Rest tomb (1495) and the workshop (1579) are separate maps, so the
// questline's progression between them is inherently a map transfer (retail plays a Huey cinematic across it).
class TitanWorkshopTransferEvent : public BasicEvent
{
public:
    explicit TitanWorkshopTransferEvent(Player* player) : _player(player) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (_player->IsInWorld() && _player->GetMapId() == 1495)   // still at Volund's tomb
            _player->TeleportTo(1579, 2782.0f, 2546.0f, 364.0f, 3.5f);   // Creator's Workshop, by Mimiron
        return true;
    }

private:
    Player* _player;
};

// Titanstrike hand-off + betrayal. Reaching Titanstrike at the end of the tomb and claiming it (clicking the Titan
// Chest) is the retail trigger: it completes "Stolen Thunder" (objective 2 "Track down Titanstrike", a CriteriaTree
// the incomplete import can't otherwise satisfy), then Prustaga - the vrykul "ally" - betrays the group and snatches
// the weapon, and moments later Mimiron teleports the player to the Creator's Workshop (map 1579) to continue the
// chain (it turns in to Mimiron there). Bound to the Titan Chest GO (249718) via gameobject_template.ScriptName.
// Warlord Volund stays a plain hostile boss fought on the way in - he is not the completion trigger.
struct go_titanstrike : public GameObjectAI
{
    go_titanstrike(GameObject* go) : GameObjectAI(go) { }

    bool OnGossipHello(Player* player) override
    {
        if (player->GetQuestStatus(QUEST_STOLEN_THUNDER) != QUEST_STATUS_INCOMPLETE)
            return false;   // not on this step -> default behaviour

        player->CompleteQuest(QUEST_STOLEN_THUNDER);   // claim Titanstrike -> objective 2 "Track down Titanstrike"

        // The betrayal: Prustaga snatches Titanstrike and gloats.
        if (Creature* prustaga = me->FindNearestCreature(NPC_PRUSTAGA, 100.0f))
            prustaga->HandleEmoteCommand(EMOTE_ONESHOT_LAUGH);

        // ...then Mimiron teleports the player onward to the Creator's Workshop.
        player->m_Events.AddEventAtOffset(new TitanWorkshopTransferEvent(player), 5s);
        return true;   // handled -> suppress the default (empty) chest loot
    }
};
void AddSC_orderhall_hunter()
{
    // Quest
    new quest_stolen_thunder();

    // Creature
    RegisterCreatureAI(npc_grif_wildheart_flight);

    // GameObject
    RegisterGameObjectAI(go_titanstrike);
}
