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
#include "Creature.h"
#include "DB2Structure.h"
#include "EventProcessor.h"
#include "GameObject.h"
#include "GameObjectAI.h"
#include "Garrison.h"
#include "GarrisonMap.h"
#include "Map.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "QuestDef.h"
#include "ScriptedCreature.h"
#include "ScriptedGossip.h"
#include "ScriptMgr.h"
#include "TemporarySummon.h"
#include "Unit.h"

#include <cmath>
#include <iterator>
#include <algorithm>
#include <mutex>
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
    // The "Rise, Champions"-style unlock quest(s). Completing one creates the hall (if needed) and recruits the full
    // champion roster. Demon Hunter has two loyalty variants (Illidari vs Altruis).
    std::vector<uint32> Quests;
    // Champion GarrFollower ids (GarrType 3, this class per GarrFollower.ChrClassID), leader first.
    std::vector<uint32> Champions;
};

// Keyed by class id (Classes enum). Verified vs GarrFollower.db2 (ChrClassID = column 28) + the class order-hall
// questlines.
static std::unordered_map<uint8 /*Classes*/, ClassOrderHallInfo> const ClassOrderHalls =
{
    { CLASS_WARRIOR,      { { 42598 },        { 708, 709, 710, 711, 712, 713, 714, 715, 989 } } },  // Skyhold (Valarjar)
    { CLASS_PALADIN,      { { 39696 },        { 478, 479, 480, 755, 756, 757, 758, 759, 1000 } } },  // Sanctum of Light
    { CLASS_HUNTER,       { { 40954, 40955 }, { 593, 742, 743, 744, 745, 746, 747, 748, 996 } } },  // Trueshot Lodge (Unseen Path)
    { CLASS_ROGUE,        { { 42139 },        { 591, 778, 779, 780, 890, 891, 892, 893, 988 } } },  // Hall of Shadows (Uncrowned)
    { CLASS_PRIEST,       { { 43270 },        { 856, 857, 870, 871, 872, 873, 874, 875, 1002 } } },  // Netherlight Temple
    { CLASS_MONK,         { { 42187 },        { 596, 588, 602, 603, 604, 605, 606, 607, 998 } } },  // Temple of Five Dawns
    { CLASS_DRUID,        { { 42583 },        { 639, 640, 641, 642, 643, 644, 645, 646, 999 } } },  // The Dreamgrove
    { CLASS_DEMON_HUNTER, { { 42671, 42670 }, { 595, 498, 722, 721, 499, 594, 807, 718, 719, 720 } } },  // The Fel Hammer (loyalty)
    { CLASS_DEATH_KNIGHT, { { 43264 },        { 855, 584, 586, 838, 839, 599, 853, 854, 1003 } } },  // Acherus (Ebon Blade)
    { CLASS_SHAMAN,       { { 42383 },        { 611, 608, 609, 610, 612, 614, 613, 615, 992 } } },  // The Maelstrom (Earthen Ring)
    { CLASS_MAGE,         { { 42663 },        { 761, 716, 717, 725, 723, 726, 762, 724, 597, 994 } } },  // Hall of the Guardian (Tirisgarde)
    { CLASS_WARLOCK,      { { 40823, 42608 }, { 589, 619, 617, 618, 620, 621, 616, 590, 997 } } },  // Dreadscar Rift (Black Harvest)
};

// Bound to every class order hall's unlock quest(s) via quest_template_addon.ScriptName = 'quest_class_order_hall'.
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
        if (std::find(info.Quests.begin(), info.Quests.end(), quest->GetQuestId()) == info.Quests.end())
            return;

        // Establish the faction's class hall if the player has none, then recruit the class's champions. AddFollower
        // is idempotent (a duplicate is a no-op), so completing another of the class's unlock quests is harmless.
        if (!player->GetGarrison(GARRISON_TYPE_CLASS_ORDER))
            player->CreateGarrison(player->GetTeamId() == TEAM_ALLIANCE ? GARR_SITE_CLASS_HALL_ALLIANCE : GARR_SITE_CLASS_HALL_HORDE);

        Garrison* hall = player->GetGarrison(GARRISON_TYPE_CLASS_ORDER);
        if (!hall)
            return;

        for (uint32 champion : info.Champions)
            hall->AddFollower(champion);

        hall->GenerateAvailableMissions();
    }
};

// ---------------------------------------------------------------------------------------------------------------------
// Class order hall on-ramp - the "class messenger".
//
// In retail, once a character arrives in Legion Dalaran the class-hall intro chain's first quest is offered by a
// class-specific NPC who seeks the player out. In our world DB that NPC's static spawn sits in a scenario/staging area
// the arriving player never walks through (e.g. Hunter: Vereesa Windrunner 100190 is spawned far below floating
// Dalaran, in zone 7543 at z~26), so the otherwise-intact chain is effectively unreachable. We reproduce the retail
// behaviour by summoning a personal copy of the messenger next to an eligible player in Dalaran; the messenger walks
// up to the player and follows until the root quest is engaged, from which the class-hall chain (culminating in the
// hall-unlock quest handled by quest_class_order_hall above) plays out normally.
//
// Data-driven per class. Only classes whose intro chain is verified walkable through to the unlock quest are listed;
// add a class here as its chain links are repaired. (Hunter's chain 40400 -> 40419 -> 40952 -> 40953 -> 40954 -> 40955
// is intact today; the other classes' Prev/Next links dead-end mid-campaign and need repair before they belong here.)
enum { DALARAN_LEGION_ZONE = 7502 };

struct ClassHallMessengerInfo
{
    uint32 MessengerEntry;   // the class's intro quest giver (creature_template entry, kept as a personal summon)
    uint32 RootQuest;        // the first quest of that class's order-hall chain
};

static std::unordered_map<uint8 /*Classes*/, ClassHallMessengerInfo> const ClassHallMessengers =
{
    { CLASS_HUNTER, { 100190 /*Vereesa Windrunner*/, 40400 /*Clandestine Operation*/ } },
};

class class_hall_messenger : public PlayerScript
{
public:
    class_hall_messenger() : PlayerScript("class_hall_messenger") { }

    void OnUpdateZone(Player* player, uint32 newZone, uint32 /*newArea*/) override
    {
        if (newZone != DALARAN_LEGION_ZONE)
        {
            Dismiss(player);            // left Dalaran - remove any pending messenger
            return;
        }

        TrySummonMessenger(player);
    }

    void OnQuestStatusChange(Player* player, uint32 questId) override
    {
        // Once the player engages (accepts) the root quest, the messenger has done its job.
        auto itr = ClassHallMessengers.find(player->GetClass());
        if (itr != ClassHallMessengers.end() && itr->second.RootQuest == questId)
            Dismiss(player);
    }

    void OnLogout(Player* player) override
    {
        _messengers.erase(player->GetGUID());   // the personal summon despawns with the player / on its own timer
    }

private:
    // player GUID -> currently-summoned messenger GUID; prevents duplicates and allows an early despawn. A given
    // player is only ever touched from its own map-update thread, but different players run on different map threads,
    // so the shared container is guarded against concurrent structural modification.
    std::unordered_map<ObjectGuid, ObjectGuid> _messengers;
    std::mutex _messengersLock;

    void TrySummonMessenger(Player* player)
    {
        auto itr = ClassHallMessengers.find(player->GetClass());
        if (itr == ClassHallMessengers.end())
            return;

        ClassHallMessengerInfo const& info = itr->second;

        // Already handled: player has the class hall, or is already on/past the intro chain.
        if (player->GetGarrison(GARRISON_TYPE_CLASS_ORDER))
            return;
        if (player->GetQuestStatus(info.RootQuest) != QUEST_STATUS_NONE)
            return;

        // Only seek the player out if they can actually accept the quest (level/prerequisites/faction).
        Quest const* root = sObjectMgr->GetQuestTemplate(info.RootQuest);
        if (!root || !player->CanTakeQuest(root, false))
            return;

        // One messenger at a time.
        {
            std::lock_guard<std::mutex> guard(_messengersLock);
            if (_messengers.count(player->GetGUID()))
                return;
        }

        // Summon a personal copy a few yards behind the player and have it walk up and follow until spoken to. The
        // summon keeps the template's quest-giver flag + creature_queststarter, so the player can accept the root quest
        // from it directly. Private to the summoner so other players don't see a stray Vereesa in Dalaran.
        Position pos = player->GetFirstCollisionPosition(10.0f, float(M_PI));
        TempSummon* messenger = player->SummonCreature(info.MessengerEntry, pos, TEMPSUMMON_TIMED_DESPAWN, Minutes(5), 0, 0, player->GetGUID());
        if (!messenger)
            return;

        messenger->GetMotionMaster()->MoveFollow(player, 2.0f);

        std::lock_guard<std::mutex> guard(_messengersLock);
        _messengers[player->GetGUID()] = messenger->GetGUID();
    }

    void Dismiss(Player* player)
    {
        ObjectGuid summonGuid;
        {
            std::lock_guard<std::mutex> guard(_messengersLock);
            auto itr = _messengers.find(player->GetGUID());
            if (itr == _messengers.end())
                return;
            summonGuid = itr->second;
            _messengers.erase(itr);
        }

        if (Creature* messenger = ObjectAccessor::GetCreature(*player, summonGuid))
            messenger->DespawnOrUnsummon();
    }
};

// ---------------------------------------------------------------------------------------------------------------------
// Garrison render-on-entry fix.
//
// The WoD garrison lives on its own instanced map, entered via a seamless (no-loading-screen) transfer. On a normal
// login the client runs the garrison handshake (CMSG_GET_GARRISON_INFO + CMSG_GARRISON_GET_MAP_DATA) and renders the
// plot buildings; on a seamless map transfer it does NOT, so although the building GameObjects are spawned the client
// never receives the garrison state that drives the plot-building WMOs - the plots render empty while only each
// building's interior/work-order spawns show. Relogging fixes it (full handshake); ".reload" cannot (it is server-side
// only). We push the same responses the client would have requested. The push is deferred a moment because the map
// change fires OnMapChanged from within Map::AddPlayerToMap, before the client has finished loading the new map -
// sending immediately would arrive too early to stick (matching the observed "buildings flash then vanish" on
// re-entry).
class GarrisonRenderEvent : public BasicEvent
{
public:
    explicit GarrisonRenderEvent(Player* player) : _player(player) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (_player->IsInWorld() && _player->GetMap()->IsGarrison())
        {
            for (auto const& [type, garrison] : _player->GetGarrisons())
            {
                GarrSiteLevelEntry const* site = garrison->GetSiteLevel();
                if (site && site->MapID == _player->GetMapId())
                {
                    garrison->SendInfo();           // GetGarrisonInfoResult (+ mission/troop refresh) - the login snapshot
                    garrison->SendMapData(_player);  // GarrisonMapDataResponse - drives the plot-building WMO rendering
                    break;
                }
            }
        }
        return true;
    }

private:
    Player* _player;
};

class garrison_render_on_enter : public PlayerScript
{
public:
    garrison_render_on_enter() : PlayerScript("garrison_render_on_enter") { }

    void OnMapChanged(Player* player) override
    {
        if (!player->GetMap()->IsGarrison())
            return;

        // Defer ~1.5s so the client has finished loading the garrison map before we push its render state.
        player->m_Events.AddEventAtOffset(new GarrisonRenderEvent(player), 1500ms);
    }
};

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

// Titanstrike hand-off: defeating Warlord Volund (104956) at the end of the tomb completes "Stolen Thunder" for the
// killer - objective 2 "Track down Titanstrike" (a CriteriaTree the incomplete import can't otherwise satisfy: the
// tomb's Titan Chest has no loot table and there is no scenario framework here). Completing the quest and carrying the
// player on to the Creator's Workshop (map 1579) lets the chain continue (it turns in to Mimiron there). Bound to
// Warlord Volund via creature_template.ScriptName.
struct npc_warlord_volund : public ScriptedAI
{
    npc_warlord_volund(Creature* creature) : ScriptedAI(creature) { }

    void JustDied(Unit* killer) override
    {
        Player* player = killer ? killer->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr;
        if (player && player->GetQuestStatus(QUEST_STOLEN_THUNDER) == QUEST_STATUS_INCOMPLETE)
        {
            player->CompleteQuest(QUEST_STOLEN_THUNDER);
            // Give the player a few seconds to loot, then carry them on to Mimiron's workshop to turn in / continue.
            player->m_Events.AddEventAtOffset(new TitanWorkshopTransferEvent(player), 6s);
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

    // Creature
    RegisterCreatureAI(npc_grif_wildheart_flight);
    RegisterCreatureAI(npc_warlord_volund);

    // Quest
    new quest_garrison_shipyard_intro();
    new quest_class_order_hall();
    new quest_stolen_thunder();

    // Player
    new class_hall_messenger();
    new garrison_render_on_enter();
}
