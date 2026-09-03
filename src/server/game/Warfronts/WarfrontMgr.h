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

#ifndef TRINITYCORE_WARFRONT_MGR_H
#define TRINITYCORE_WARFRONT_MGR_H

#include "Warfront.h"
#include "WarfrontQueue.h"
#include <string>
#include <unordered_map>

class Player;

// Realm-global cycle owner for BfA Warfronts (Battle for Stromgarde, Battle for Darkshore). Modeled on
// BattlefieldMgr: a singleton that owns one independent state machine per warfront, ticked from World::Update.
// It observes the contribution bar (ManagedWorldStateMgr), drives phase transitions, flips zone control and
// persists coarse cycle state to the characters DB (warfront_state). See WARFRONTS_DESIGN.md §1 / §2 / §P0.
class TC_GAME_API WarfrontMgr
{
    WarfrontMgr();
    ~WarfrontMgr();

public:
    WarfrontMgr(WarfrontMgr const&) = delete;
    WarfrontMgr(WarfrontMgr&&) = delete;
    WarfrontMgr& operator=(WarfrontMgr const&) = delete;
    WarfrontMgr& operator=(WarfrontMgr&&) = delete;

    static WarfrontMgr* instance();

    // Loads the warfront_state rows (or seeds the defaults: Stromgarde + Darkshore, Alliance controls / Horde
    // contributing) and builds the per-zone Warfront objects. Called once at server startup.
    void Initialize();

    // Ticked from World::Update; drives the PhaseEndTime timers (siege expiry -> flip, boss-window expiry -> despawn).
    void Update(uint32 diff);

    Warfront* GetWarfront(uint32 warfrontId);
    Warfront const* GetWarfront(uint32 warfrontId) const;
    WarfrontState GetState(uint32 warfrontId) const;
    TeamId GetControllingTeam(uint32 warfrontId) const;

    // Reverse lookup used by the Contribution native-UI display path: which warfront (if any) owns the contribution
    // bar driven by this ManagedWorldState id (either faction's bar). Returns nullptr when the id is not a warfront bar.
    Warfront const* GetWarfrontByContributionMWS(uint32 managedWorldStateId) const;

    // --- integration seams (see §Key integration seams) --------------------------------------------------------
    // Called when a challenger's contribution ManagedWorldState reaches its target: CONTRIBUTION -> SIEGE.
    void OnContributionTargetReached(uint32 managedWorldStateId);
    // Called from InstanceScenario completion for a warfront scenario: SIEGE -> flip control.
    void OnScenarioComplete(uint32 warfrontId, TeamId team);

    // GM/testing: force the given warfront into its next transition (CONTRIBUTION->SIEGE, or SIEGE->flip).
    // Returns false if the warfront id is unknown.
    bool AdvanceWarfront(uint32 warfrontId);

    // --- queue (Phase B / §3) ----------------------------------------------------------------------------------
    // The per-warfront single-team enrollment queue (opened while the zone is in SIEGE).
    WarfrontQueue* GetQueue(uint32 warfrontId);
    WarfrontQueue const* GetQueue(uint32 warfrontId) const;

    // Gate for enrolling: warfront unlocked by the war campaign, warfront in SIEGE, player is on the challenging
    // team, meets the level prereq, not already enrolled. `reason` (optional) receives a plain string suitable for
    // player feedback when false. `ignoreUnlockGate` is only used by the GM testing override.
    bool CanQueue(Player* player, uint32 warfrontId, std::string* reason = nullptr, bool ignoreUnlockGate = false) const;

    // Enrolls the player into the warfront queue if CanQueue passes. Returns true on a fresh enrollment.
    bool EnqueuePlayer(Player* player, uint32 warfrontId, std::string* reason = nullptr, bool ignoreUnlockGate = false);

    // --- Blizzlike unlock gate (Warfront.h §"Blizzlike unlock chain") -------------------------------------------
    // True when this character has progressed the war campaign far enough to assault this warfront: the World-Quest
    // campaign quest AND the terminal quest of the warfront intro chain (or the "Warfront: The Battle for X" quest
    // itself, which is only obtainable after that chain). `reason` receives the exact missing step when false.
    bool HasUnlockedWarfront(Player const* player, uint32 warfrontId, std::string* reason = nullptr) const;

    // worldserver.conf Warfront.RequireUnlockQuest - lets a test realm drop the war-campaign requirement.
    static bool IsUnlockGateEnabled();

    // Live contribution-bar readout for the given team's bar of this warfront, as a 0..1 fraction of the way from
    // the empty floor to the accumulation target. Returns false when the warfront/bar is unknown.
    bool GetContributionProgress(uint32 warfrontId, TeamId team, float& outFraction, int32& outProgress, int32& outTarget) const;

    // --- LFG wiring: the native war-table "Join Battle" button (see Warfront.h §"LFG wiring") -------------------
    // Resolves an LFGDungeons.db2 id carried by CMSG_DF_JOIN to the warfront it belongs to. Returns 0 when the id
    // is not one of the 8 warfront dungeons (i.e. a normal LFG join that must take the usual path). `outMapId`,
    // when given, receives the battle Map.db2 id that dungeon enters (0 on a miss).
    static uint32 GetWarfrontForLfgDungeon(uint32 lfgDungeonId, uint32* outMapId = nullptr);

    // Reverse lookup for the finder-opener: the Normal-difficulty LFGDungeons id for (warfront, battle map).
    // Returns 0 when the pair is unknown.
    static uint32 GetLfgDungeonForWarfront(uint32 warfrontId, uint32 battleMapId);

    // Sends SMSG_OPEN_LFG_DUNGEON_FINDER so the client pops its native finder preselected to this warfront's
    // current assault. The body is INFERRED, so this no-ops unless Warfront.NativeUI.Enable = 1. Returns true only
    // when a packet actually went out.
    bool SendOpenLfgDungeonFinder(Player* player, uint32 warfrontId) const;

    // worldserver.conf Warfront.NativeUI.Enable - master opt-in for every warfront SMSG whose body is INFERRED
    // rather than byte-recovered. Public so gossip/scripts can hide options that would send one.
    static bool IsNativeUiEnabled();

    // GM/testing one-shot: make the caller's faction the attacker, force the zone into SIEGE (opening the queue) and
    // enroll the caller - which, at the test min-player floor, immediately forms the battle group and teleports the
    // player into the assault instance to fight the boss. Returns false (with reason) if it can't launch.
    bool DevJoinAssault(Player* player, uint32 warfrontId, std::string* reason = nullptr);

    std::unordered_map<uint32, Warfront> const& GetWarfronts() const { return _warfronts; }

private:
    void SeedDefaults();
    void LoadFromDB();
    void SaveWarfront(Warfront const& wf) const;

    void TransitionToSiege(Warfront& wf);
    void FlipControl(Warfront& wf);

    // worldserver.conf Warfront.TimeScale: compresses every cycle timer for testing (default 1.0).
    float GetTimeScale() const;

    // Opens/closes the warfront's queue to mirror the SIEGE window (challenger team + routed battle map).
    void OpenQueue(Warfront const& wf);
    void CloseQueue(uint32 warfrontId);

    // --- P4: flip glue -----------------------------------------------------------------------------------------
    // Spawns / despawns the controlling faction's world boss on the outdoor continent map, and pushes the
    // zone-scoped control + siege world states so the outdoor zone reflects the current owner.
    void SpawnWorldBoss(Warfront& wf);
    void DespawnWorldBoss(Warfront& wf);
    void PushZoneWorldStates(Warfront const& wf) const;

    // Sends SMSG_WARFRONT_COMPLETE to every in-world player of the winning team. The packet body is INFERRED
    // (see WARFRONT_OPCODE_SPEC.md §C / §E), so this no-ops unless worldserver.conf Warfront.NativeUI.Enable = 1.
    void SendWarfrontComplete(Warfront const& wf, TeamId winner) const;

    std::unordered_map<uint32 /*warfrontId*/, Warfront> _warfronts;
    std::unordered_map<uint32 /*warfrontId*/, WarfrontQueue> _queues;
    uint32 _updateTimer;
};

#define sWarfrontMgr WarfrontMgr::instance()

#endif // TRINITYCORE_WARFRONT_MGR_H
