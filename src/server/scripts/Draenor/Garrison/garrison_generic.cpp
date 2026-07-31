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

#include "AreaTrigger.h"
#include "AreaTriggerAI.h"
#include "Chat.h"
#include "GameObject.h"
#include "GameObjectAI.h"
#include "Garrison.h"
#include "GarrisonMap.h"
#include "Map.h"
#include "Player.h"
#include "QuestDef.h"
#include "ScriptMgr.h"
#include "Unit.h"

#include <iterator>
#include <vector>
#include <unordered_map>

// XX - Garrison enter AreaTrigger
struct at_garrison_enter : AreaTriggerAI
{
    at_garrison_enter(AreaTrigger* areatrigger) : AreaTriggerAI(areatrigger) { }

    void OnInitialize() override
    {
        at->setActive(true); // has to be active, otherwise the at is no longer updated before we are able to leave it
    }

    void OnUnitEnter(Unit* unit) override
    {
        Player* player = unit->ToPlayer();
        if (!player)
            return;

        Garrison* garrison = player->GetGarrison();
        if (!garrison)
            return;

        garrison->Enter();
    }
};

// XX - Garrison exit AreaTrigger
struct at_garrison_exit : AreaTriggerAI
{
    at_garrison_exit(AreaTrigger* areatrigger) : AreaTriggerAI(areatrigger) { }

    void OnInitialize() override
    {
        at->setActive(true); // has to be active, otherwise the at is no longer updated before we are able to leave it
    }

    void OnUnitExit(Unit* unit, AreaTriggerExitReason /*reason*/) override
    {
        Player* player = unit->ToPlayer();
        if (!player)
            return;

        Garrison* garrison = player->GetGarrison();
        if (!garrison)
            return;

        garrison->Leave();
    }
};

// Garrison resource cache: the WoD cache GameObject (types "Garrison Cache" / "Hefty" / "Full") accrues
// Garrison Resources over time. Clicking it collects whatever has banked (Garrison::CollectGarrisonCache).
// Retail gives only the currency-gain toast as confirmation, so we don't emit a system chat message.
struct go_garrison_cache : GameObjectAI
{
    go_garrison_cache(GameObject* go) : GameObjectAI(go) { }

    uint32 _displayTimer = 0;

    Garrison* GetOwnerGarrison() const
    {
        if (Map* map = me->GetMap())
            if (map->IsGarrison())
                return static_cast<GarrisonMap*>(map)->GetGarrison();
        return nullptr;
    }

    // The resource cache swaps its model as Garrison Resources bank up: Normal (< 200), Hefty (200-499),
    // Full (>= 500, capped). DisplayInfoIDs are the per-faction Garrison Cache / Hefty / Full GO templates
    // (Alliance 23775/23773/23777, Horde 23774/23772/23776). Collecting empties it back to the Normal model.
    void RefreshDisplay()
    {
        Garrison* garrison = GetOwnerGarrison();
        if (!garrison || garrison->GetType() != GARRISON_TYPE_GARRISON)
            return;

        uint32 const banked = garrison->GetPendingCacheResources();
        bool const alliance = garrison->GetFaction() == GARRISON_FACTION_INDEX_ALLIANCE;

        uint32 displayId;
        if (banked >= 500)
            displayId = alliance ? 23777 : 23776; // Full
        else if (banked >= 200)
            displayId = alliance ? 23773 : 23772; // Hefty
        else
            displayId = alliance ? 23775 : 23774; // Normal

        if (me->GetDisplayId() != displayId)
            me->SetDisplayId(displayId);
    }

    void UpdateAI(uint32 diff) override
    {
        _displayTimer += diff;
        if (_displayTimer < 5000)
            return;
        _displayTimer = 0;
        RefreshDisplay();
    }

    bool OnGossipHello(Player* player) override
    {
        Garrison* garrison = player->GetGarrison();
        if (!garrison || garrison->GetType() != GARRISON_TYPE_GARRISON)
            return false;

        garrison->CollectGarrisonCache(); // grants the currency (client shows the standard gain toast)
        me->SendCustomAnim(0);            // play the cache's use animation for loot feedback
        RefreshDisplay();                 // banked resources reset to 0 -> revert to the empty (Normal) model
        return true; // the cache is fully handled here — suppress the default goober behaviour
    }
};

// NOTE: the building work-order crate (GAMEOBJECT_TYPE_GARRISON_SHIPMENT) is handled entirely in core
// (GameObject::Use -> Garrison::SendOpenShipmentUI); it needs no GameObject script here.

// WoD Shipyard unlock. The Blizzlike path: at garrison Tier 3 the "Garrison Campaign: War Council" pop-up chain
// starts from the faction leader, runs out to the Iron Docks (We Need a Shipwright -> Derailment -> The Train Gang
// -> Hook, Line, and... Sink Him! -> Nothing Remains) and ends with "All Hands on Deck" back at the garrison,
// whose completion builds the shipyard (retail casts reward spell 186007 Alliance / 185915 Horde). We hook the
// terminal quest's REWARDED status directly (verified quest ids) rather than the reward spell, so the trigger does
// not depend on that spell's effect layout. CreateShipyard() itself re-checks the Tier-3 prerequisite.
enum ShipyardIntroQuests
{
    QUEST_ALL_HANDS_ON_DECK_ALLIANCE = 38259,
    QUEST_ALL_HANDS_ON_DECK_HORDE    = 38574
};

// Bound to quests 38259 / 38574 via quest_template.ScriptName = 'quest_garrison_shipyard_intro'.
struct quest_garrison_shipyard_intro : QuestScript
{
    quest_garrison_shipyard_intro() : QuestScript("quest_garrison_shipyard_intro") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus != QUEST_STATUS_REWARDED)
            return;

        if (Garrison* garrison = player->GetGarrison())
            garrison->CreateShipyard(); // no-op unless it is a Tier-3 garrison with no shipyard yet
    }
};

// ============================================================
// Legion Class Order Halls (GarrType 3)
// ============================================================
// A class order hall is a GarrType-3 garrison. The GarrSite is per FACTION, not per class (matches the client /
// AshamaneCore): Alliance = 161, Horde = 163. The class-specific content (physical hall, champions) is keyed by the
// player's class. Each class has a QuestInfoID-107 intro chain given by its hall leader; retail establishes the hall
// and recruits champions via SPELL_EFFECT_CREATE_GARRISON / follower-grant spells. We hook the two key quests'
// REWARDED status per class so the trigger is independent of that (offline-unavailable) spell-effect layout.
enum ClassOrderHallSites
{
    GARR_SITE_CLASS_HALL_ALLIANCE = 161,
    GARR_SITE_CLASS_HALL_HORDE    = 163
};

// Per-class order-hall data. EstablishQuest creates the hall + recruits the leader (Champions[0]); ChampionQuest
// recruits the remaining champions + seeds the mission board. Champions are GarrFollower ids (GarrType 3), leader
// first. Add one row per class as its data is verified, and bind BOTH of that class's quests to ScriptName
// 'quest_class_order_hall' in quest_template_addon.
struct ClassOrderHallInfo
{
    uint32 EstablishQuest;
    uint32 ChampionQuest;
    std::vector<uint32> Champions;
};

static std::unordered_map<uint8 /*Classes*/, ClassOrderHallInfo> const ClassOrderHalls =
{
    // Hunter - "Unseen Path" (Trueshot Lodge). Leader: Emmarel Shadewarden (593).
    { CLASS_HUNTER, { 40954 /*The Unseen Path*/, 40955 /*Oath of Service*/, { 593, 642, 742, 743, 744, 745, 746, 747, 748 } } },
    // TODO remaining 11 classes: each class's QuestInfoID-107 establish + champion quests + its champion GarrFollower ids.
};

// Bound to every class order hall's establish + champion quests via quest_template_addon.ScriptName.
struct quest_class_order_hall : QuestScript
{
    quest_class_order_hall() : QuestScript("quest_class_order_hall") { }

    void OnQuestStatusChange(Player* player, Quest const* quest, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus != QUEST_STATUS_REWARDED)
            return;

        auto itr = ClassOrderHalls.find(player->GetClass());
        if (itr == ClassOrderHalls.end())
            return;

        ClassOrderHallInfo const& info = itr->second;
        uint32 const questId = quest->GetQuestId();

        if (questId == info.EstablishQuest)
        {
            // Establish the faction's class hall if the player has none, then recruit its leader.
            if (!player->GetGarrison(GARRISON_TYPE_CLASS_ORDER))
                player->CreateGarrison(player->GetTeamId() == TEAM_ALLIANCE ? GARR_SITE_CLASS_HALL_ALLIANCE : GARR_SITE_CLASS_HALL_HORDE);
            if (Garrison* hall = player->GetGarrison(GARRISON_TYPE_CLASS_ORDER))
                if (!info.Champions.empty())
                    hall->AddFollower(info.Champions.front());
        }
        else if (questId == info.ChampionQuest)
        {
            // The remaining champions swear service; seed the initial mission board.
            if (Garrison* hall = player->GetGarrison(GARRISON_TYPE_CLASS_ORDER))
            {
                for (std::size_t i = 1; i < info.Champions.size(); ++i)
                    hall->AddFollower(info.Champions[i]);
                hall->GenerateAvailableMissions();
            }
        }
    }
};

void AddSC_garrison_generic()
{
    // AreaTrigger
    RegisterAreaTriggerAI(at_garrison_enter);
    RegisterAreaTriggerAI(at_garrison_exit);

    // GameObject
    RegisterGameObjectAI(go_garrison_cache);

    // Quest
    new quest_garrison_shipyard_intro();
    new quest_class_order_hall();
}
