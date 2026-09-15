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

// Death Knight Legion artifact acquisition. Per-class Broken Isles content (cf. zone_orderhall_hunter.cpp); the generic
// class-hall framework lives in orderhall_legion.cpp. Three specs, three very different states of the world DB:
//
//   Unholy  - "Apocalypse" (40930 -> 40935). The FULL kill-credit questline exists (both maps: Deadwind Pass surface
//             map 0 + the Karazhan Catacombs scenario map 1533 with Ariden already spawned). We wire each quest's
//             scripted/flight/gossip/escort objectives as kill-credits + scripted teleports, and drive the artifact
//             claim off Ariden's death. This is the completable acquisition.
//
//   Frost   - "Blades of the Fallen Prince". SCENARIO 901 + map 1480 ("DeathKnightArtifactArea") are fully spawned
//             (Highlord Darion, The Lich King, the Blades, Frostmourne fragments, the Scourge Teleporter, hostile ICC
//             trash), but there is NO acquisition quest_template in the world DB and no `scenarios` row. We add the
//             `scenarios` row (SQL) so scenario 901 runs on map 1480, then bind a scenario-director AI to Darion that
//             advances the nine steps by the player's progress (the DB2 step criteria do not fire here - same wall as
//             the Hunter BM Titanstrike scenario). See risks: without an acquisition quest the run has no quest turn-in
//             and no scripted entry trigger; the director drives the on-screen scenario + the Acherus return.
//
//   Blood   - "Maw of the Damned". Genuinely greenfield: no quest, no objectives, and the two "Maw of the Damned"
//             display creatures (103795 / 101478) are UNSPAWNED. It cannot be made completable without authoring a
//             full quest_template + quest_objectives (outside the ScriptName/spawn SQL template used here). Documented
//             in risks; no non-functional code is emitted for it.

#include "Creature.h"
#include "EventProcessor.h"
#include "Log.h"
#include "Map.h"
#include "Player.h"
#include "QuestDef.h"
#include "Scenario.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"
#include "SharedDefines.h"
#include "orderhall_artifact_common.h"
#include <utility>
#include <vector>

// =====================================================================================================================
// Unholy artifact: "Apocalypse" chain (40930 -> 40931 -> 40932 -> 40933 -> 40934 -> 40935).
//
// Confirmed against integ_world: the six quests + their quest_objectives + creature_queststarter/questender all exist,
// as do both encounter maps. Most objectives are scripted/flight/gossip/escort kill-credit dummies; one (40932) is a
// Type-2 gameobject-use objective (GO 245789), and 40934's artifact claim is a kill-credit granted on Ariden's death.
// =====================================================================================================================
enum ApocalypseData
{
    // Chain
    QUEST_APOCALYPSE              = 40930,
    QUEST_FOLLOWING_THE_CURSE     = 40931,
    QUEST_DISTURBING_THE_PAST     = 40932,
    QUEST_A_GRISLY_TASK           = 40933,
    QUEST_THE_DARK_RIDERS         = 40934, // artifact claimed here
    QUEST_CALL_OF_VENGEANCE       = 40935,

    // 40930 "Apocalypse" objectives (Type-0 credits)
    CREDIT_PORTAL_KARAZHAN        = 102580, // "Take the Dalaran portal to Karazhan"
    CREDIT_INVESTIGATE_MISTMANTLE = 100821, // "Investigate Manor Mistmantle in Duskwood"
    CREDIT_CONVINCE_REVIL         = 100368, // "Convince Revil to help" (gossip)

    // 40931 "Following the Curse" objectives (escort)
    CREDIT_FOLLOW_REVIL           = 100655, // "Follow Revil to Ariden's Camp"
    CREDIT_REACHED_ARIDEN_CAMP    = 100716, // "Reached Outside Ariden's Camp"

    // 40932 "Disturbing the Past" - Type-2 gameobject-use objective
    GO_ARIDENS_CAMP               = 245789, // "Ariden's Camp investigated"

    // 40933 "A Grisly Task" objectives
    CREDIT_LEARN_DARK_RIDERS      = 102411, // "Learn the location of the Dark Riders"
    NPC_LAITH_SHAOL               = 102459, // kill objective x99

    // 40934 "The Dark Riders" objectives
    CREDIT_DEFEAT_DARK_RIDERS     = 100813, // "Defeat the Dark Riders" (Sewer Scenario Complete)
    CREDIT_APOCALYPSE_LOOTED      = 102571, // "Apocalypse claimed" (artifact)

    // 40935 "The Call of Vengeance" objectives
    CREDIT_DEATHGATE_THRONE       = 111886, // "Take the Death Gate to the Frozen Throne"
    CREDIT_MARK_OF_LICH_KING      = 102598, // "Obtain the Mark of the Lich King"
    CREDIT_DEATHGATE_ACHERUS      = 96125,  // "Take the Death Gate to Acherus"

    // Actors
    NPC_ARIDEN_HOSTILE            = 102532, // real hostile Ariden (faction 2102)
    NPC_ARIDEN_PLACEHOLDER        = 102200, // faction-35 duplicate at the same spot - made hostile here

    // Maps
    MAP_DEADWIND_PASS             = 0,      // Deadwind Pass surface (Revil, escort, Ariden's Camp GO)
    MAP_KARAZHAN_CATACOMBS        = 1533,   // scenario map "Karazhan Catacombs" - the Dark Riders / Ariden fight
    MAP_ACHERUS                   = 1220    // Broken Isles / Acherus / Dalaran
};

// Approach beside Revil Kost (100323) on the Deadwind Pass surface (map 0).
static constexpr Position DeadwindRevilPos   = { -10360.0f, -1253.0f, 35.9f, 3.10f };
// Drop point beside Ariden inside the catacombs (map 1533) - a few yards short of the boss.
static constexpr Position AridenCatacombsPos = { -10882.0f, -1961.0f, -41.0f, 0.00f };
// Return beside Revil (101282) after Ariden dies, so 40934 can be handed in (questender is on map 0).
static constexpr Position AridenReturnPos    = { -10850.0f, -1961.0f, -41.0f, 3.10f };
// Acherus / Duke Lankral, map 1220 - the Death Gate return for 40935 (and the Frost scenario finale below).
static constexpr Position AcherusReturnPos   = { -841.858f, 4271.92f, 746.263f, 0.00f };

// A single reusable deferred step: grant a set of monster kill-credits and/or gameobject-use credits, then optionally
// teleport the player. Legion artifact quests use kill-credit dummies for their flight / gossip / scene / "reached X"
// objectives, so nearly every chain beat reduces to this. Mirrors the per-leg BasicEvents in the Hunter file.
class DkChainStepEvent : public BasicEvent
{
public:
    DkChainStepEvent(Player* player, std::vector<uint32> monsterCredits, std::vector<uint32> goCredits,
                     int32 teleMap, Position telePos)
        : _player(player), _monsterCredits(std::move(monsterCredits)), _goCredits(std::move(goCredits)),
          _teleMap(teleMap), _telePos(telePos) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (_player->IsInWorld())
        {
            for (uint32 entry : _monsterCredits)
                _player->KilledMonsterCredit(entry);
            for (uint32 goEntry : _goCredits)
                _player->KillCreditGO(goEntry);
            if (_teleMap >= 0)
                _player->TeleportTo(uint32(_teleMap), _telePos.GetPositionX(), _telePos.GetPositionY(),
                    _telePos.GetPositionZ(), _telePos.GetOrientation());
        }
        return true;
    }

private:
    Player* _player;
    std::vector<uint32> _monsterCredits;
    std::vector<uint32> _goCredits;
    int32 _teleMap;
    Position _telePos;
};

// 40930 "Apocalypse": the opening beats are a scripted Dalaran->Karazhan portal, a Duskwood investigation, and a
// gossip on Revil - all dummy credits. Grant the three on accept and drop the player beside Revil Kost (100323) in
// Deadwind Pass, ready to hand 40930 back to him. Bound to 40930 via quest_template_addon.ScriptName.
struct quest_dk_apocalypse : QuestScript
{
    quest_dk_apocalypse() : QuestScript("quest_dk_apocalypse") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus == QUEST_STATUS_INCOMPLETE)
            player->m_Events.AddEventAtOffset(new DkChainStepEvent(player,
                { CREDIT_PORTAL_KARAZHAN, CREDIT_INVESTIGATE_MISTMANTLE, CREDIT_CONVINCE_REVIL },
                {}, MAP_DEADWIND_PASS, DeadwindRevilPos), 1500ms);
    }
};

// 40931 "Following the Curse": the escort of Revil to Ariden's Camp. Both credits granted on accept (the escort is the
// show; the objectives are the gate). Bound to 40931.
struct quest_dk_following_the_curse : QuestScript
{
    quest_dk_following_the_curse() : QuestScript("quest_dk_following_the_curse") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus == QUEST_STATUS_INCOMPLETE)
            player->m_Events.AddEventAtOffset(new DkChainStepEvent(player,
                { CREDIT_FOLLOW_REVIL, CREDIT_REACHED_ARIDEN_CAMP }, {}, -1, {}), 1500ms);
    }
};

// 40932 "Disturbing the Past": a single Type-2 gameobject-use objective (Ariden's Camp, GO 245789). Credited via
// Player::KillCreditGO on accept. Bound to 40932.
struct quest_dk_disturbing_the_past : QuestScript
{
    quest_dk_disturbing_the_past() : QuestScript("quest_dk_disturbing_the_past") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus == QUEST_STATUS_INCOMPLETE)
            player->m_Events.AddEventAtOffset(new DkChainStepEvent(player, {}, { GO_ARIDENS_CAMP }, -1, {}), 1500ms);
    }
};

// 40933 "A Grisly Task": learn the Dark Riders' location (1 credit) + slay 99 of Laith Sha'ol (102459). We grant the
// learn credit and the 99 kills on accept. Bound to 40933.
struct quest_dk_a_grisly_task : QuestScript
{
    quest_dk_a_grisly_task() : QuestScript("quest_dk_a_grisly_task") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus == QUEST_STATUS_INCOMPLETE)
        {
            std::vector<uint32> credits = { CREDIT_LEARN_DARK_RIDERS };
            for (int i = 0; i < 99; ++i) // objective is 99x kill of 102459
                credits.push_back(NPC_LAITH_SHAOL);
            player->m_Events.AddEventAtOffset(new DkChainStepEvent(player, std::move(credits), {}, -1, {}), 1500ms);
        }
    }
};

// 40934 "The Dark Riders": the artifact fight. On accept we transfer the player into the Karazhan Catacombs (map 1533)
// beside Ariden; the artifact claim itself is granted from Ariden's death (npc_dk_ariden). Bound to 40934.
struct quest_dk_the_dark_riders : QuestScript
{
    quest_dk_the_dark_riders() : QuestScript("quest_dk_the_dark_riders") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus == QUEST_STATUS_INCOMPLETE)
            player->m_Events.AddEventAtOffset(new DkChainStepEvent(player, {}, {}, MAP_KARAZHAN_CATACOMBS, AridenCatacombsPos), 1500ms);
    }
};

// 40935 "The Call of Vengeance": Death Gate to the Frozen Throne, obtain the Mark of the Lich King, Death Gate back to
// Acherus - three scripted credits. Grant them and return the player to Acherus. Bound to 40935.
struct quest_dk_call_of_vengeance : QuestScript
{
    quest_dk_call_of_vengeance() : QuestScript("quest_dk_call_of_vengeance") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus == QUEST_STATUS_INCOMPLETE)
            player->m_Events.AddEventAtOffset(new DkChainStepEvent(player,
                { CREDIT_DEATHGATE_THRONE, CREDIT_MARK_OF_LICH_KING, CREDIT_DEATHGATE_ACHERUS },
                {}, MAP_ACHERUS, AcherusReturnPos), 1500ms);
    }
};

// Ariden - the Dark Riders' leader, final boss of 40934. Bound to BOTH the real hostile entry (102532) and the
// faction-35 duplicate (102200) spawned at the same spot; Reset() forces faction 16 so whichever the player engages is
// attackable. His death grants "Defeat the Dark Riders" (100813) and "Apocalypse claimed" (102571), then returns the
// player to the surface beside Revil (101282) to hand 40934 in.
struct npc_dk_ariden : public ScriptedAI
{
    npc_dk_ariden(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        if (me->GetMap()->GetId() == MAP_KARAZHAN_CATACOMBS)
            me->SetFaction(FACTION_MONSTER_2); // faction 16 - fixes the f35 duplicate and keeps the real one hostile
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        me->Yell("So this is what you're after? The artifacts belong here! Your Light does not reach here -- come and claim it!", LANG_UNIVERSAL);
    }

    void JustDied(Unit* /*killer*/) override
    {
        if (me->GetMap()->GetId() != MAP_KARAZHAN_CATACOMBS)
            return;

        me->Yell("S... souls of the dead! Aid your master!", LANG_UNIVERSAL);

        for (auto const& ref : me->GetMap()->GetPlayers())
            if (Player* p = ref.GetSource())
                if (p->IsInWorld() && p->GetQuestStatus(QUEST_THE_DARK_RIDERS) == QUEST_STATUS_INCOMPLETE)
                {
                    p->KilledMonsterCredit(CREDIT_DEFEAT_DARK_RIDERS); // 100813
                    p->KilledMonsterCredit(CREDIT_APOCALYPSE_LOOTED);  // 102571 - Apocalypse claimed
                    ArtifactPlayScene(me->GetMap(), 1603); // DK Unholy - Apocalypse loot scene
                    p->m_Events.AddEventAtOffset(new DkChainStepEvent(p, {}, {}, MAP_DEADWIND_PASS, AridenReturnPos), 5s);
                }
    }
};

// =====================================================================================================================
// Frost artifact: "Blades of the Fallen Prince" - InstanceScenario 901 on map 1480 ("DeathKnightArtifactArea").
//
// The scenario and all its actors are spawned, but there is NO acquisition quest in the world DB. We add a `scenarios`
// row (SQL) so scenario 901 instantiates on map 1480, then drive its nine steps from a director bound to Highlord
// Darion Mograine (95193) - exactly as the Hunter BM director drives scenario 1068, because the DB2 step criteria do
// not fire on this server. Steps advance on the player's progress across the two sub-areas (the ICC citadel band where
// the Frostmourne fragments lie, and the Frozen Throne band where the Blades are reforged and claimed). The Scourge
// Teleporter (95416, step O3) bridges the two bands, so we teleport the player between them there. The finale (O8)
// completes the scenario and returns the player to Acherus.
//
// LIMITATION: with no acquisition quest_template there is no scripted way IN to map 1480 and no quest turn-in; the
// director assumes the player is already on the map (e.g. via a future quest or a GM port). See risks.
// =====================================================================================================================
enum BladesFallenPrinceData
{
    SCENARIO_BLADES_FALLEN_PRINCE = 901,
    MAP_DK_ARTIFACT_AREA          = 1480,
    NPC_DARION_DIRECTOR           = 95193,  // Highlord Darion Mograine - scenario director
    NPC_LICH_KING_FROST           = 103996, // The Lich King at the Frozen Throne (grantor)
    NPC_BLADES_ARTIFACT           = 100577, // Blades of the Fallen Prince (display)
    NPC_SCOURGE_TELEPORTER        = 95416   // links the citadel band to the Frozen Throne band (step O3)
};

// The two sub-area bands on map 1480 (verified from the spawns), plus the teleporter bridge between them.
static constexpr Position IccCitadelBand    = { 4364.10f, 2748.30f, 353.20f, 0.00f }; // Frostmourne fragment area
static constexpr Position ScourgeTeleporter = { 4357.63f, 2770.24f, 356.14f, 0.00f }; // step O3 bridge
static constexpr Position FrozenThroneBand  = { 504.07f, -2124.24f, 840.95f, 3.00f }; // reforge / claim the Blades

// Scenario director bound to Highlord Darion (95193). setActive so his AI runs wherever the party is on the map.
struct npc_dk_blades_scenario_director : public ScriptedAI
{
    npc_dk_blades_scenario_director(Creature* creature) : ScriptedAI(creature) { }

    uint32 _pollTimer = 0;
    uint32 _orderTimer = 0;       // ms spent on the current scenario step (paces the non-positional beats)
    uint8  _lastOrder = 255;
    bool   _teleportedToThrone = false;
    bool   _completed = false;

    void Reset() override
    {
        if (me->GetMap()->GetId() == MAP_DK_ARTIFACT_AREA)
            me->setActive(true);
    }

    Player* AnyPlayer() const
    {
        for (auto const& ref : me->GetMap()->GetPlayers())
            if (Player* p = ref.GetSource())
                if (p->IsInWorld() && p->IsAlive())
                    return p;
        return nullptr;
    }

    void UpdateAI(uint32 diff) override
    {
        if (me->GetMap()->GetId() != MAP_DK_ARTIFACT_AREA || _completed)
            return;

        _pollTimer += diff;
        if (_pollTimer < 1000)
            return;
        _pollTimer = 0;

        if (!me->GetScenario()) // scenario 901 not running (needs the `scenarios` row for map 1480)
            return;

        uint8 const order = ArtifactScenarioOrder(me);
        if (order != _lastOrder)
        {
            _lastOrder = order;
            _orderTimer = 0;
            TC_LOG_INFO("scripts", "[Blades 901] scenario step is now order {}", order);
        }
        else
            _orderTimer += 1000;

        if (!AnyPlayer())
            return;

        switch (order)
        {
            case 0: // The Call of the North
                if (_orderTimer == 0) me->Say("The Lich King calls, death knight. Enter Icecrown Citadel and reclaim what is his.", LANG_UNIVERSAL);
                if (_orderTimer >= 4000) ArtifactAdvanceScenario(me);
                break;
            case 1: // The Gates Are Open - enter the citadel
                if (ArtifactPlayerNear(me, IccCitadelBand, 120.0f))
                {
                    ArtifactPlayScene(me->GetMap(), 1467); // DK Frost - Icecrown entrance scene
                    ArtifactAdvanceScenario(me);
                }
                break;
            case 2: // Seek the Fragments - the Frostmourne shards
                if (ArtifactPlayerNear(me, IccCitadelBand, 60.0f))
                {
                    me->Say("Gather the shards of Frostmourne scattered through these halls.", LANG_UNIVERSAL);
                    ArtifactAdvanceScenario(me);
                }
                break;
            case 3: // Travel to the Frozen Throne - use the Scourge Teleporter
                if (ArtifactPlayerNear(me, ScourgeTeleporter, 20.0f))
                {
                    if (!_teleportedToThrone)
                    {
                        _teleportedToThrone = true;
                        for (auto const& ref : me->GetMap()->GetPlayers())
                            if (Player* p = ref.GetSource())
                                if (p->IsInWorld())
                                    p->TeleportTo(MAP_DK_ARTIFACT_AREA, FrozenThroneBand.GetPositionX(),
                                        FrozenThroneBand.GetPositionY() + 12.0f, FrozenThroneBand.GetPositionZ(), 3.0f);
                    }
                    ArtifactAdvanceScenario(me);
                }
                break;
            case 4: // Power Overwhelming - reforge the fragments at the throne
                if (ArtifactPlayerNear(me, FrozenThroneBand, 40.0f)) ArtifactAdvanceScenario(me);
                break;
            case 5: // The Purge - purge the blades of malevolent souls
                if (_orderTimer == 0) me->Say("Purge the blades of the souls they have devoured. Let nothing linger within.", LANG_UNIVERSAL);
                if (_orderTimer >= 4000) ArtifactAdvanceScenario(me);
                break;
            case 6: // The Hungering Cold - claim the Blades
                if (_orderTimer == 0)
                {
                    me->Say("The Blades of the Fallen Prince are yours now. Take them, and wield them well.", LANG_UNIVERSAL);
                    ArtifactPlayScene(me->GetMap(), 1466); // DK Frost - Blades loot scene
                }
                if (_orderTimer >= 4000) ArtifactAdvanceScenario(me);
                break;
            case 7: // Death's March - the Lich King's Blessing
                if (_orderTimer == 0) me->Say("You bear the Lich King's blessing. Do not squander it.", LANG_UNIVERSAL);
                if (_orderTimer >= 4000) ArtifactAdvanceScenario(me);
                break;
            case 8: // You Have Your Orders - depart via the Acherus Waygate
                if (_orderTimer == 0) me->Say("Return to Acherus. The Scourge does not wait, and neither do your Blades.", LANG_UNIVERSAL);
                if (_orderTimer >= 5000)
                {
                    _completed = true;
                    ArtifactAdvanceScenario(me); // final step -> completes scenario 901
                    TC_LOG_INFO("scripts", "[Blades 901] finale reached -> complete + return to Acherus");
                    for (auto const& ref : me->GetMap()->GetPlayers())
                        if (Player* p = ref.GetSource())
                            if (p->IsInWorld())
                                p->TeleportTo(MAP_ACHERUS, AcherusReturnPos.GetPositionX(), AcherusReturnPos.GetPositionY(),
                                    AcherusReturnPos.GetPositionZ(), AcherusReturnPos.GetOrientation());
                }
                break;
            default:
                break;
        }
    }
};

// =====================================================================================================================
// Blood artifact: "Maw of the Damned" - quest 40740 "The Dead and the Damned".
//
// The quest IS in the world DB (given + turned in by Highlord Darion Mograine, 93437/113695, in Dalaran map 1220), but
// shipped with NO objectives and NO encounter, so it auto-completed on accept and had no cutscene. We add a single
// kill-Gorelix objective (SQL) and script the acquisition: on accept we fly the player to the Broken Shore (the retail
// finale leg) and TRANSIENTLY summon Gorelix the Fleshripper (101778) - a TempSummon rather than a permanent spawn, so
// he never bleeds into the OTHER classes' runs that share the Broken Shore scenario map (1500). Gorelix's reveal (scene
// 1561) plays on engage and his death plays the Maw-of-the-Damned loot scene (1532), grants the kill objective, and
// returns the player to Darion to hand 40740 in.
//
// LIMITATION: the exact retail encounter spot (Dalaran -> Darkstone Isle -> a Demon Portal on the Broken Shore) is not
// in our DB; we reuse the verified Broken Shore scenario landing as a best-effort location. The Darkstone Isle leg and
// the Lich King closing charge (Icecrown) are not reproduced. See the gap list.
// =====================================================================================================================
enum MawOfTheDamnedData
{
    QUEST_THE_DEAD_AND_THE_DAMNED = 40740,
    NPC_GORELIX                   = 101778, // Gorelix the Fleshripper (already faction 16 / hostile in the DB)
    MAP_BLOOD_BROKEN_SHORE        = 1500,   // shared Broken Shore scenario map - we only TempSummon here
    MAP_DALARAN_BROKEN_ISLES      = 1220    // Darion Mograine (turn-in) is here
};

// Best-effort Broken Shore encounter spot (reuses the verified scenario landing) + Gorelix a few yards ahead, and the
// return point beside Darion in Dalaran (the questender's spawn).
static constexpr Position BloodBrokenShoreLanding = { -2505.00f, 118.00f, 9.40f, 6.00f };
static constexpr Position GorelixSummonPos        = { -2492.00f, 118.00f, 9.40f, 3.14f };
static constexpr Position DarionReturnPos         = { -1438.80f, 1160.70f, 319.10f, 3.90f };

// Summons Gorelix beside the player AFTER the flight has landed them on the Broken Shore (a separate delayed event so
// the summon lands on the destination map, not the origin). TempSummon => transient, no permanent shared-map spawn.
class BloodGorelixSummonEvent : public BasicEvent
{
public:
    explicit BloodGorelixSummonEvent(Player* player) : _player(player) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (_player->IsInWorld() && _player->GetMap()->GetId() == MAP_BLOOD_BROKEN_SHORE)
            _player->SummonCreature(NPC_GORELIX, GorelixSummonPos, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 60s);
        return true;
    }

private:
    Player* _player;
};

// On accept (the quest has a kill objective now, so it enters INCOMPLETE): fly to the Broken Shore, then summon Gorelix.
class quest_dk_the_dead_and_the_damned : public QuestScript
{
public:
    quest_dk_the_dead_and_the_damned() : QuestScript("quest_dk_the_dead_and_the_damned") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus != QUEST_STATUS_INCOMPLETE)
            return;

        player->m_Events.AddEventAtOffset(new DkChainStepEvent(player, {}, {}, MAP_BLOOD_BROKEN_SHORE, BloodBrokenShoreLanding), 1500ms);
        player->m_Events.AddEventAtOffset(new BloodGorelixSummonEvent(player), 4s);
    }
};

// Gorelix the Fleshripper - the Blood acquisition boss. Reveal scene on engage, Maw-of-the-Damned loot scene + kill
// credit + return-to-Darion on death.
struct npc_dk_gorelix : public ScriptedAI
{
    npc_dk_gorelix(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        me->SetFaction(FACTION_MONSTER_2); // faction 16 - hostile
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        me->Yell("What is this? More foolish mortals infesting my realm? You've come a long way just to die.", LANG_UNIVERSAL);
        ArtifactPlayScene(me->GetMap(), 1561); // DK Blood - Gorelix reveal scene
    }

    void JustDied(Unit* /*killer*/) override
    {
        me->Yell("Netrezaar... take... you...", LANG_UNIVERSAL);
        ArtifactPlayScene(me->GetMap(), 1532); // DK Blood - Maw of the Damned loot scene

        for (auto const& ref : me->GetMap()->GetPlayers())
            if (Player* p = ref.GetSource())
                if (p->IsInWorld() && p->GetQuestStatus(QUEST_THE_DEAD_AND_THE_DAMNED) == QUEST_STATUS_INCOMPLETE)
                {
                    p->KilledMonsterCredit(NPC_GORELIX); // completes the kill objective (also auto-credited on death)
                    p->m_Events.AddEventAtOffset(new DkChainStepEvent(p, {}, {}, MAP_DALARAN_BROKEN_ISLES, DarionReturnPos), 5s);
                }
    }
};

void AddSC_artifact_deathknight()
{
    // Unholy - "Apocalypse" chain (40930 -> 40935)
    new quest_dk_apocalypse();
    new quest_dk_following_the_curse();
    new quest_dk_disturbing_the_past();
    new quest_dk_a_grisly_task();
    new quest_dk_the_dark_riders();
    new quest_dk_call_of_vengeance();
    RegisterCreatureAI(npc_dk_ariden);

    // Frost - "Blades of the Fallen Prince" scenario 901
    RegisterCreatureAI(npc_dk_blades_scenario_director);

    // Blood - "Maw of the Damned" (quest 40740)
    new quest_dk_the_dead_and_the_damned();
    RegisterCreatureAI(npc_dk_gorelix);
}