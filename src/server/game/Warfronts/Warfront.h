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

#ifndef TRINITYCORE_WARFRONT_H
#define TRINITYCORE_WARFRONT_H

#include "Define.h"
#include "ObjectGuid.h"
#include "SharedDefines.h"      // TeamId
#include <ctime>

// The macro-state of a single warfront's challenger. The public "named" phase a player sees is the pair
// {ControllingTeam} x {State}; the mirror CONTRIBUTION with teams swapped is the resting state after a flip.
// See WARFRONTS_DESIGN.md §2.
enum WarfrontState : uint8
{
    WF_CONTRIBUTION = 0,    // challenger fills the region-wide contribution bar; owner Patrols the outdoor zone
    WF_SIEGE        = 1,    // bar reached target; the challenger's assault scenario is queueable for 7 days
    WF_FLIP         = 2,    // transient: scenario done / siege expired -> control flips, loser's world boss spawns
};

// Our own warfront ids (NOT client ids); one per contested zone. See WARFRONTS_DESIGN.md §1.1.
enum WarfrontId : uint32
{
    WARFRONT_STROMGARDE = 1,    // Battle for Stromgarde (Arathi Highlands)
    WARFRONT_DARKSHORE  = 2,    // Battle for Darkshore
};

// --- LFG wiring (the native "Join Battle" button) ---------------------------------------------------------------
// Warfronts queue through the LFG system, NOT BattlemasterList: the client's war-table "Join Battle" button calls
// JoinSingleLFG(LE_LFG_CATEGORY_SCENARIO, lfgDungeonID), which puts CMSG_DF_JOIN (0x400037) on the wire with
// slot = dungeonID | (TypeID << 24). Warfront LFGDungeons rows are TypeID 1 / Subtype 3, so slot = id | 0x01000000 -
// exactly TrinityCore's existing LFG_DUNGEON_ENTRY convention, which WorldSession::HandleLfgJoinOpcode already
// decodes. That handler intercepts these ids and routes them to WarfrontMgr's single-team queue instead of running
// the normal group-forming/proposal flow (a warfront is one faction versus NPCs - there is nobody to match against).
//
// One row per warfront LFGDungeons.db2 entry, VERIFIED against LFGDungeons.db2 build 68275 (TypeID=1, Subtype=3).
// BattleMapId is the map that dungeon enters, and matches Warfront::BattleMap_Alliance/_Horde exactly.
struct WarfrontLfgDungeonEntry
{
    uint32      LfgDungeonId;   // LFGDungeons.db2 ID sent in the CMSG_DF_JOIN slot (low 24 bits)
    uint32      WarfrontId;     // our WarfrontId
    uint32      BattleMapId;    // Map.db2 id this dungeon enters (== the challenging faction's battle map)
    bool        Heroic;         // DifficultyID 149 (Heroic) vs 147 (Normal)
    char const* Name;           // for logging / GM output
};

// TODO: the Heroic rows resolve to the same warfront and the same battle map as their Normal twins; per-difficulty
// scaling, the 10-player floor and separate lockouts are a later phase. Heroic is treated as Normal for now.
inline constexpr WarfrontLfgDungeonEntry WarfrontLfgDungeons[] =
{
    { 1760, WARFRONT_STROMGARDE, 1943, false, "Battle for Stromgarde (Alliance)"        },
    { 1615, WARFRONT_STROMGARDE, 1876, false, "Battle for Stromgarde (Horde)"           },
    { 1901, WARFRONT_DARKSHORE,  2105, false, "The Battle for Darkshore (Alliance)"     },
    { 1902, WARFRONT_DARKSHORE,  2111, false, "The Battle for Darkshore (Horde)"        },
    { 2007, WARFRONT_STROMGARDE, 1943, true,  "Battle for Stromgarde (Alliance, Heroic)"},
    { 1982, WARFRONT_STROMGARDE, 1876, true,  "Battle for Stromgarde (Horde, Heroic)"   },
    { 2032, WARFRONT_DARKSHORE,  2105, true,  "The Battle for Darkshore (Alliance, Heroic)" },
    { 2031, WARFRONT_DARKSHORE,  2111, true,  "The Battle for Darkshore (Horde, Heroic)"    },
};

// InstanceScript::SetData commands the warfront battle controller accepts (used by the .warfront GM commands to
// drive a battle instance manually for examination/testing). See scripts/Warfronts/warfront_common.h.
enum WarfrontInstanceCommand : uint32
{
    WARFRONT_CMD_ADVANCE_STEP = 0xF00001,   // SetData:     advance the scenario one step
    WARFRONT_CMD_MUSTER_BOSS  = 0xF00002,   // SetData:     muster the final boss at its scripted muster point

    // SetGuidData: muster the final boss right next to the given player instead of at the scripted muster point, so a
    // tester using `.warfront boss` always gets the fight immediately wherever they happen to be standing.
    WARFRONT_CMD_MUSTER_BOSS_AT_PLAYER = 0xF00003,

    // GetGuidData: the live final boss creature guid (empty until mustered). Used by `.warfront goboss`.
    WARFRONT_DATA_FINAL_BOSS_GUID      = 0xF00010,
};

// Client-side pin frame slot the warfront battle controller re-uses for its "the boss is here" world marker
// (SMSG_RECEIVE_PING_WORLD_POINT). A fixed id means each refresh replaces the previous pin instead of stacking them.
inline constexpr uint32 WarfrontBossPinFrameId = 0xF0B055;

// --- Blizzlike unlock chain (BfA war campaign) -------------------------------------------------------------------
// Retail gates warfront queueing behind war-campaign progression, not behind a GM command. Two hurdles per faction:
//
//   1. the campaign hurdle - the quest that opens World Quests, which is what actually makes the warfront intro
//      appear at all ("Uniting Kul Tiras" 51918 Alliance / "Uniting Zandalar" 51916 Horde). The three war-campaign
//      Footholds are criteria of the *Ready for War* ACHIEVEMENT (12510 A / 12509 H), not direct quest prerequisites,
//      so they are deliberately not gated on here.
//
//   2. the warfront intro chain, which ends at a quest whose completion is what opens the assault:
//        Stromgarde (8.0) A: The Warfront Looms 53175 -> To the Front 53194 -> Touring the Front 53197
//                            -> Back to Boralus 53198   [UNLOCK]   -> Warfront: The Battle for Stromgarde 53414
//        Stromgarde (8.0) H: The Warfront Looms 53207 -> To the Front 53208 -> Touring the Front 53210
//                            -> Back to Zuldazar 53212  [UNLOCK]   -> Warfront: The Battle for Stromgarde 53416
//        Darkshore  (8.1) A: On Whispered Winds 53847 -> ... -> In Darkest Night 53990
//                            -> We Are Coming 54871      [UNLOCK]  -> Warfront: The Battle for Darkshore 53992
//        Darkshore  (8.1) H: Trouble in Darkshore 54042 -> ... -> Aftermath 54050
//                            -> Warfront Preparations 54416 [UNLOCK] -> Warfront: The Battle for Darkshore 53955
//
// Darkshore is an independent 8.1 chain - it does NOT require the Stromgarde chain.
//
// Every id below was verified to exist in the realm's quest_template. Holding (or having completed) the terminal
// "Warfront: The Battle for X" quest also counts as unlocked, because it can only be picked up after the intro.
enum WarfrontUnlockQuests : uint32
{
    // campaign hurdle (World Quest unlock)
    QUEST_UNITING_KUL_TIRAS                 = 51918,    // Alliance
    QUEST_UNITING_ZANDALAR                  = 51916,    // Horde

    // Stromgarde intro terminal + warfront quest
    QUEST_BACK_TO_BORALUS                   = 53198,    // Alliance unlock
    QUEST_WARFRONT_STROMGARDE_ALLIANCE      = 53414,
    QUEST_BACK_TO_ZULDAZAR                  = 53212,    // Horde unlock
    QUEST_WARFRONT_STROMGARDE_HORDE         = 53416,

    // Darkshore intro terminal + warfront quest
    QUEST_WE_ARE_COMING                     = 54871,    // Alliance unlock
    QUEST_WARFRONT_DARKSHORE_ALLIANCE       = 53992,
    QUEST_WARFRONT_PREPARATIONS             = 54416,    // Horde unlock
    QUEST_WARFRONT_DARKSHORE_HORDE          = 53955,
};

// Retail kill-credit creature awarded for a single war-effort donation. It is the objective of the weekly
// "Warfront Contribution" quests (53185 Alliance / 53209 Horde, objective type 0, ObjectID 143337, "Make a donation
// to the war effort"), so handing it out on a successful contribution is what closes that loop.
inline constexpr uint32 WarfrontDonationCreditCreature = 143337;

// One independent state machine per contested zone. WarfrontMgr owns the cycle; it does NOT own the fight (that is
// the instance) nor the contribution bar counter (that is ManagedWorldStateMgr). It observes the bar, drives the
// phase transitions, flips zone control and (in a later phase) spawns/despawns the world boss.
struct Warfront
{
    uint32 Id = 0;                          // WarfrontId
    WarfrontState State = WF_CONTRIBUTION;   // current challenger phase
    TeamId ControllingTeam = TEAM_ALLIANCE;  // who Patrols the outdoor zone now
    TeamId ChallengingTeam = TEAM_HORDE;     // = the other team; the one currently contributing
    time_t PhaseEndTime = 0;                 // unix; fixed 7-day siege expiry, then ~5-day world-boss despawn (0 = none)
    // When the current phase began. In-memory only (not persisted): a restart re-stamps it to the boot time, which is
    // the best available lower bound. Feeds the native Contribution UI's `startTime` return of C_ContributionCollector
    // .GetState(). See WARFRONT_OPCODE_SPEC.md §C.
    time_t PhaseStartTime = 0;

    // ManagedWorldState ids feeding the contribution bar for each faction's assault (authored in P1; 0 for now).
    uint32 ContributionMWS_Alliance = 0;
    uint32 ContributionMWS_Horde = 0;

    // Spawned during a flip, for the faction that just lost control (P4; unset for now).
    ObjectGuid WorldBossGuid;

    // --- static config (defaults from research; §2.2) -----------------------------------------------------------
    char const* Name = "";                   // for logging / GM output
    uint32 SiegeDurationSecs = 7 * 24 * 60 * 60;   // the one hard timer: 7 days
    uint32 BossWindowSecs    = 5 * 24 * 60 * 60;   // world-boss up-time: ~5 days

    // The instanced battle map the *attacking* faction of that name zones into (WARFRONTS_DESIGN.md §3.1). The
    // challenger's team picks the map: Arathi 1943 (A) / 1876 (H); Darkshore 2105 (A) / 2111 (H).
    uint32 BattleMap_Alliance = 0;
    uint32 BattleMap_Horde    = 0;

    // Minimum enrolled players before the queue can form a battle group (5 Normal; §3.1). Heroic (10) is P6.
    uint32 QueueMinPlayers = 5;

    // --- P4: outdoor zone control + world boss (WARFRONTS_DESIGN.md §6.2) ----------------------------------------
    uint32 OutdoorMapId = 0;                  // continent the outdoor Patrol zone lives on (0 = EK / 1 = Kalimdor)
    // The world boss spawned on a flip is the *losing* faction's abandoned super-weapon; retail keys which weapon
    // is up off the *new controlling* team. 0 = none identified for that side.
    uint32 WorldBossWhenAllianceControls = 0;
    uint32 WorldBossWhenHordeControls = 0;
    float  WorldBossX = 0.0f, WorldBossY = 0.0f, WorldBossZ = 0.0f, WorldBossO = 0.0f;

    // Zone-scoped world states pushed to the outdoor map so it reflects the new owner. Real client WorldState.db2
    // ids for the warfront frame are unrecovered (§2.3), so these are server-authored custom ids (see status doc).
    int32  ControlWorldStateId = 0;           // value = controlling TeamId (0 Alliance / 1 Horde)
    int32  SiegeOpenWorldStateId = 0;         // value = 1 while the assault (SIEGE) window is open, else 0

    // Which world-boss entry to spawn given who now controls the zone.
    uint32 GetWorldBossForController() const
    {
        return ControllingTeam == TEAM_ALLIANCE ? WorldBossWhenAllianceControls : WorldBossWhenHordeControls;
    }

    // --- Blizzlike unlock gate (see WarfrontUnlockQuests above) --------------------------------------------------
    uint32 CampaignQuest_Alliance = 0;   // "Uniting Kul Tiras" - opens World Quests, i.e. the war campaign hurdle
    uint32 CampaignQuest_Horde    = 0;   // "Uniting Zandalar"
    uint32 UnlockQuest_Alliance   = 0;   // terminal quest of the warfront intro chain; completing it opens the assault
    uint32 UnlockQuest_Horde      = 0;
    uint32 WarfrontQuest_Alliance = 0;   // "Warfront: The Battle for X" - only obtainable after the intro chain
    uint32 WarfrontQuest_Horde    = 0;

    uint32 GetCampaignQuest(TeamId team) const { return team == TEAM_ALLIANCE ? CampaignQuest_Alliance : CampaignQuest_Horde; }
    uint32 GetUnlockQuest(TeamId team) const   { return team == TEAM_ALLIANCE ? UnlockQuest_Alliance   : UnlockQuest_Horde;   }
    uint32 GetWarfrontQuest(TeamId team) const { return team == TEAM_ALLIANCE ? WarfrontQuest_Alliance : WarfrontQuest_Horde; }

    // Returns the ManagedWorldState id that drives the given team's contribution bar (0 when unwired).
    uint32 GetContributionMWS(TeamId team) const
    {
        return team == TEAM_ALLIANCE ? ContributionMWS_Alliance : ContributionMWS_Horde;
    }

    // Returns the ManagedWorldState id that drives the *challenger's* bar (the one WarfrontMgr watches).
    uint32 GetChallengerContributionMWS() const
    {
        return ChallengingTeam == TEAM_ALLIANCE ? ContributionMWS_Alliance : ContributionMWS_Horde;
    }

    // Returns the battle map id the current challenger assaults into (drives WarfrontQueue routing).
    uint32 GetChallengerBattleMap() const
    {
        return ChallengingTeam == TEAM_ALLIANCE ? BattleMap_Alliance : BattleMap_Horde;
    }
};

#endif // TRINITYCORE_WARFRONT_H
