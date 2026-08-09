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

#include "WarfrontMgr.h"
#include "Config.h"
#include "Creature.h"
#include "DatabaseEnv.h"
#include "GameTime.h"
#include "Log.h"
#include "ManagedWorldStateMgr.h"
#include "Map.h"
#include "MapManager.h"
#include "LFGPackets.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Position.h"
#include "QuestDef.h"
#include "StringFormat.h"
#include "TemporarySummon.h"
#include "WarfrontPackets.h"
#include "World.h"
#include "WorldSession.h"
#include <algorithm>

namespace
{
    // Only re-evaluate the (day-scale) cycle timers once per second; there is no need to touch them every world tick.
    constexpr uint32 WARFRONT_UPDATE_INTERVAL_MS = 1 * IN_MILLISECONDS;

    // Minimum character level to enroll for a warfront assault (BfA gates warfronts behind max-level; §3.1 prereqs).
    constexpr uint8 WARFRONT_MIN_LEVEL = 50;

}

// Master kill-switch for every warfront/contribution SMSG whose body is INFERRED rather than byte-recovered
// (SMSG_CONTRIBUTION_LAST_UPDATE_RESPONSE, SMSG_WARFRONT_COMPLETE, SMSG_OPEN_LFG_DUNGEON_FINDER). A malformed
// reflection-read message can disconnect or crash a live client, so it stays OFF until a sniff confirms the layout.
// See WARFRONT_OPCODE_SPEC.md §C / §E.
bool WarfrontMgr::IsNativeUiEnabled()
{
    return sConfigMgr->GetBoolDefault("Warfront.NativeUI.Enable", false);
}

// Blizzlike default: warfronts are war-campaign content and stay locked until that campaign has been progressed.
// A test realm can drop the requirement, but the normal path is the quest chain.
bool WarfrontMgr::IsUnlockGateEnabled()
{
    return sConfigMgr->GetBoolDefault("Warfront.RequireUnlockQuest", true);
}

WarfrontMgr::WarfrontMgr() : _updateTimer(0)
{
}

WarfrontMgr::~WarfrontMgr() = default;

WarfrontMgr* WarfrontMgr::instance()
{
    static WarfrontMgr instance;
    return &instance;
}

float WarfrontMgr::GetTimeScale() const
{
    // A value <= 0 would freeze/invert the cycle; clamp to a small positive floor so timers always progress.
    return std::max(0.0001f, sConfigMgr->GetFloatDefault("Warfront.TimeScale", 1.0f));
}

void WarfrontMgr::SeedDefaults()
{
    // Default cycle: Alliance holds each zone, Horde is the challenger contributing toward its assault. The
    // ContributionMWS ids are wired in P1 (contribution phase); they stay 0 here, so OnContributionTargetReached
    // matches nothing yet and transitions are driven only by the GM ".warfront advance" command.
    Warfront stromgarde;
    stromgarde.Id = WARFRONT_STROMGARDE;
    stromgarde.Name = "Battle for Stromgarde";
    stromgarde.State = WF_CONTRIBUTION;
    stromgarde.ControllingTeam = TEAM_ALLIANCE;
    stromgarde.ChallengingTeam = TEAM_HORDE;
    stromgarde.PhaseEndTime = 0;
    // Real client ManagedWorldState ids for the Arathi (Stromgarde) contribution bars (build 68275), traced
    // CreatureXContribution -> Contribution -> ManagedWorldStateInput -> ManagedWorldState:
    //   Alliance donates to "Warfront Alliance Contribution Dummy" 143709 -> Contribution 116 -> Input 115 -> MWS 113
    //   Horde    donates to "Warfront Horde Contribution Dummy"    143707 -> Contribution 11  -> Input 10  -> MWS 12
    stromgarde.ContributionMWS_Alliance = 113;
    stromgarde.ContributionMWS_Horde    = 12;
    // Battle instance maps the attacking faction assaults into (Map.db2): 1943 WarfrontsArathi-Alliance / 1876 Horde.
    stromgarde.BattleMap_Alliance = 1943;
    stromgarde.BattleMap_Horde    = 1876;
    // P4 outdoor control: Arathi Highlands is on Eastern Kingdoms (map 0). The world boss is the losing faction's
    // super-weapon, keyed off the new controller (retail): Alliance holds -> Doom's Howl 138122 (Horde weapon) is up;
    // Horde holds -> The Lion's Roar 137374 (Alliance weapon). Spawn point is Doom's Howl's real Arathi ground spawn.
    stromgarde.OutdoorMapId = 0;
    stromgarde.WorldBossWhenAllianceControls = 138122;  // Doom's Howl
    stromgarde.WorldBossWhenHordeControls    = 137374;  // The Lion's Roar
    stromgarde.WorldBossX = -1071.33f; stromgarde.WorldBossY = -2423.93f; stromgarde.WorldBossZ = 54.36f; stromgarde.WorldBossO = 1.5708f;
    // Blizzlike unlock: war campaign (World Quests) + the 8.0 warfront intro chain. See Warfront.h.
    stromgarde.CampaignQuest_Alliance = QUEST_UNITING_KUL_TIRAS;
    stromgarde.CampaignQuest_Horde    = QUEST_UNITING_ZANDALAR;
    stromgarde.UnlockQuest_Alliance   = QUEST_BACK_TO_BORALUS;
    stromgarde.UnlockQuest_Horde      = QUEST_BACK_TO_ZULDAZAR;
    stromgarde.WarfrontQuest_Alliance = QUEST_WARFRONT_STROMGARDE_ALLIANCE;
    stromgarde.WarfrontQuest_Horde    = QUEST_WARFRONT_STROMGARDE_HORDE;
    // No classic world states: the retail warfront outdoor UI is driven by the ContributionCollector protocol
    // (Enum.ContributionState Building/Active/UnderAttack/Destroyed + %), fed by the ManagedWorldState bar above,
    // and the in-battle counters by the Scenario/UIWidgetSet stack - confirmed from the client UI source
    // (Blizzard_Contribution.lua / Blizzard_ScenarioObjectiveTracker.lua). See WARFRONTS_STATUS.md.
    _warfronts[WARFRONT_STROMGARDE] = stromgarde;

    Warfront darkshore;
    darkshore.Id = WARFRONT_DARKSHORE;
    darkshore.Name = "Battle for Darkshore";
    darkshore.State = WF_CONTRIBUTION;
    darkshore.ControllingTeam = TEAM_ALLIANCE;
    darkshore.ChallengingTeam = TEAM_HORDE;
    darkshore.PhaseEndTime = 0;
    // Real client ManagedWorldState ids for the Darkshore contribution bars (build 68275):
    //   Alliance -> Contribution 117 -> Input 116 -> MWS 114 ;  Horde -> Contribution 118 -> Input 117 -> MWS 115
    darkshore.ContributionMWS_Alliance = 114;
    darkshore.ContributionMWS_Horde    = 115;
    // Darkshore battle instance maps (Map.db2): 2105 WarfrontsDarkshoreAlliance / 2111 WarfrontsDarkshoreHorde.
    darkshore.BattleMap_Alliance = 2105;
    darkshore.BattleMap_Horde    = 2111;
    // P4 outdoor control: Darkshore is on Kalimdor (map 1). World boss = Ivus variants keyed off the new controller:
    // Alliance holds -> Ivus the Forest Lord 144946; Horde holds -> Ivus the Decayed 148295. The spawn coordinate is
    // an approximate Darkshore ground point (no exact retail spawn is present in any local DB - see status doc).
    darkshore.OutdoorMapId = 1;
    darkshore.WorldBossWhenAllianceControls = 144946;   // Ivus the Forest Lord
    darkshore.WorldBossWhenHordeControls    = 148295;   // Ivus the Decayed
    darkshore.WorldBossX = 4506.22f; darkshore.WorldBossY = 407.36f; darkshore.WorldBossZ = 31.73f; darkshore.WorldBossO = 3.5f;
    // Blizzlike unlock: the 8.1 Darkshore chain is INDEPENDENT of the Stromgarde chain, but shares the same
    // war-campaign (World Quest) hurdle. See Warfront.h.
    darkshore.CampaignQuest_Alliance = QUEST_UNITING_KUL_TIRAS;
    darkshore.CampaignQuest_Horde    = QUEST_UNITING_ZANDALAR;
    darkshore.UnlockQuest_Alliance   = QUEST_WE_ARE_COMING;
    darkshore.UnlockQuest_Horde      = QUEST_WARFRONT_PREPARATIONS;
    darkshore.WarfrontQuest_Alliance = QUEST_WARFRONT_DARKSHORE_ALLIANCE;
    darkshore.WarfrontQuest_Horde    = QUEST_WARFRONT_DARKSHORE_HORDE;
    // (world states left 0 - see the ContributionCollector note under Stromgarde above)
    _warfronts[WARFRONT_DARKSHORE] = darkshore;
}

void WarfrontMgr::LoadFromDB()
{
    // Overlay the persisted coarse cycle state onto the seeded defaults. Any warfront without a row keeps its seed
    // and is written back so the row exists next boot. The bar counter itself restores through ManagedWorldStateMgr.
    std::vector<uint32> persisted;
    if (QueryResult result = CharacterDatabase.Query("SELECT WarfrontId, State, Controlling, PhaseEndTime FROM warfront_state"))
    {
        do
        {
            Field* fields = result->Fetch();
            uint32 warfrontId = fields[0].GetUInt8();

            auto itr = _warfronts.find(warfrontId);
            if (itr == _warfronts.end())
            {
                TC_LOG_ERROR("server.loading", "warfront_state has a row for unknown WarfrontId {} - ignored.", warfrontId);
                continue;
            }

            Warfront& wf = itr->second;
            wf.State = WarfrontState(fields[1].GetUInt8());
            wf.ControllingTeam = TeamId(fields[2].GetUInt8());
            wf.ChallengingTeam = GetOtherTeam(wf.ControllingTeam);
            wf.PhaseEndTime = time_t(fields[3].GetUInt32());
            persisted.push_back(warfrontId);
        }
        while (result->NextRow());
    }

    for (auto const& [id, wf] : _warfronts)
        if (std::find(persisted.begin(), persisted.end(), id) == persisted.end())
            SaveWarfront(wf);
}

void WarfrontMgr::Initialize()
{
    uint32 const oldMSTime = getMSTime();

    _warfronts.clear();
    _queues.clear();
    SeedDefaults();
    LoadFromDB();

    // One in-memory queue per warfront. Any warfront restored in SIEGE re-opens its queue so enrollment survives a
    // restart mid-siege (the enrolled player set itself is transient and legitimately starts empty).
    for (auto& [id, wf] : _warfronts)
    {
        _queues.try_emplace(id, id);

        // PhaseStartTime is not persisted; boot time is the best available lower bound for the current phase.
        wf.PhaseStartTime = GameTime::GetGameTime();

        if (wf.State == WF_SIEGE)
            OpenQueue(wf);

        // Reflect the restored owner/siege flags on the outdoor zone (no-op until the continent is loaded).
        PushZoneWorldStates(wf);
    }

    TC_LOG_INFO("server.loading", ">> Initialized {} warfronts in {} ms", _warfronts.size(), GetMSTimeDiffToNow(oldMSTime));
}

void WarfrontMgr::SaveWarfront(Warfront const& wf) const
{
    CharacterDatabase.PExecute("REPLACE INTO warfront_state (WarfrontId, State, Controlling, PhaseEndTime) VALUES ({}, {}, {}, {})",
        wf.Id, uint32(wf.State), uint32(wf.ControllingTeam), uint32(wf.PhaseEndTime));
}

void WarfrontMgr::Update(uint32 diff)
{
    _updateTimer += diff;
    if (_updateTimer < WARFRONT_UPDATE_INTERVAL_MS)
        return;
    _updateTimer = 0;

    time_t const now = GameTime::GetGameTime();

    for (auto& [id, wf] : _warfronts)
    {
        if (!wf.PhaseEndTime || now < wf.PhaseEndTime)
            continue;

        switch (wf.State)
        {
            case WF_SIEGE:
                // The 7-day siege window expired without the scenario being completed -> control flips anyway.
                FlipControl(wf);
                break;
            case WF_CONTRIBUTION:
                // A boss window was stamped by the last flip; it has now elapsed -> despawn the world boss.
                DespawnWorldBoss(wf);
                wf.PhaseEndTime = 0;
                SaveWarfront(wf);
                TC_LOG_INFO("warfront", "Warfront '{}' world-boss window elapsed.", wf.Name);
                break;
            default:
                break;
        }
    }
}

void WarfrontMgr::TransitionToSiege(Warfront& wf)
{
    wf.State = WF_SIEGE;
    wf.PhaseStartTime = GameTime::GetGameTime();
    wf.PhaseEndTime = wf.PhaseStartTime + time_t(wf.SiegeDurationSecs * GetTimeScale());
    SaveWarfront(wf);

    // Open the enrollment queue for the challenger (P2) and push WS_SIEGE_OPEN to the outdoor zone (§2.3).
    OpenQueue(wf);
    PushZoneWorldStates(wf);
    TC_LOG_INFO("warfront", "Warfront '{}' entered SIEGE (challenger team {}); siege ends at {}.",
        wf.Name, uint32(wf.ChallengingTeam), uint64(wf.PhaseEndTime));
}

void WarfrontMgr::FlipControl(Warfront& wf)
{
    TeamId const loser = wf.ControllingTeam;    // the faction that just lost control spawns its world boss

    wf.ControllingTeam = wf.ChallengingTeam;
    wf.ChallengingTeam = loser;
    wf.State = WF_CONTRIBUTION;
    wf.PhaseStartTime = GameTime::GetGameTime();
    wf.PhaseEndTime = wf.PhaseStartTime + time_t(wf.BossWindowSecs * GetTimeScale());
    wf.WorldBossGuid.Clear();
    SaveWarfront(wf);

    // The assault window is over -> close the queue and drop anyone still waiting (WARFRONTS_DESIGN.md §2.2).
    CloseQueue(wf.Id);

    // Reset the new challenger's contribution bar so the next cycle starts from empty (WARFRONTS_DESIGN.md §5).
    // GetChallengerContributionMWS() now reflects the swapped ChallengingTeam. No-ops if the id is 0/unknown.
    if (uint32 const challengerMWS = wf.GetChallengerContributionMWS())
        sManagedWorldStateMgr->ResetProgress(challengerMWS);

    // Spawn the losing faction's world boss for the new controller (Doom's Howl / Lion's Roar / Ivus), and push the
    // zone-scoped control + siege-open world states so the outdoor zone flips to the new owner.
    SpawnWorldBoss(wf);
    PushZoneWorldStates(wf);

    TC_LOG_INFO("warfront", "Warfront '{}' FLIPPED: control -> team {}, new challenger team {}. World-boss window ends at {}.",
        wf.Name, uint32(wf.ControllingTeam), uint32(wf.ChallengingTeam), uint64(wf.PhaseEndTime));
}

void WarfrontMgr::PushZoneWorldStates(Warfront const& wf) const
{
    // Zone-scoped: set on the outdoor continent map so only that zone's players see the owner/siege flags. The
    // continent (map 0/1) is always loaded once the world is up; before then this is a harmless no-op.
    Map* map = sMapMgr->FindMap(wf.OutdoorMapId, 0);
    if (!map)
        return;

    if (wf.ControlWorldStateId)
        map->SetWorldStateValue(wf.ControlWorldStateId, int32(wf.ControllingTeam), false);
    if (wf.SiegeOpenWorldStateId)
        map->SetWorldStateValue(wf.SiegeOpenWorldStateId, wf.State == WF_SIEGE ? 1 : 0, false);
}

void WarfrontMgr::SpawnWorldBoss(Warfront& wf)
{
    uint32 const entry = wf.GetWorldBossForController();
    if (!entry)
    {
        TC_LOG_INFO("warfront", "Warfront '{}': no world-boss id configured for controlling team {} - spawn skipped.",
            wf.Name, uint32(wf.ControllingTeam));
        return;
    }

    Map* map = sMapMgr->FindMap(wf.OutdoorMapId, 0);
    if (!map)
    {
        TC_LOG_WARN("warfront", "Warfront '{}': outdoor map {} not loaded - world boss {} not spawned.",
            wf.Name, wf.OutdoorMapId, entry);
        return;
    }

    // Replace any lingering boss from a previous cycle first.
    DespawnWorldBoss(wf);

    Position const pos(wf.WorldBossX, wf.WorldBossY, wf.WorldBossZ, wf.WorldBossO);
    if (TempSummon* boss = map->SummonCreature(entry, pos, nullptr, Milliseconds(0)))  // duration 0 = persists until we despawn it
    {
        wf.WorldBossGuid = boss->GetGUID();
        TC_LOG_INFO("warfront", "Warfront '{}': world boss {} spawned on map {} for controlling team {}.",
            wf.Name, entry, wf.OutdoorMapId, uint32(wf.ControllingTeam));
    }
}

void WarfrontMgr::DespawnWorldBoss(Warfront& wf)
{
    if (wf.WorldBossGuid.IsEmpty())
        return;

    if (Map* map = sMapMgr->FindMap(wf.OutdoorMapId, 0))
        if (Creature* boss = map->GetCreature(wf.WorldBossGuid))
            boss->DespawnOrUnsummon();

    wf.WorldBossGuid.Clear();
}

Warfront* WarfrontMgr::GetWarfront(uint32 warfrontId)
{
    auto itr = _warfronts.find(warfrontId);
    return itr != _warfronts.end() ? &itr->second : nullptr;
}

Warfront const* WarfrontMgr::GetWarfront(uint32 warfrontId) const
{
    auto itr = _warfronts.find(warfrontId);
    return itr != _warfronts.end() ? &itr->second : nullptr;
}

WarfrontState WarfrontMgr::GetState(uint32 warfrontId) const
{
    Warfront const* wf = GetWarfront(warfrontId);
    return wf ? wf->State : WF_CONTRIBUTION;
}

TeamId WarfrontMgr::GetControllingTeam(uint32 warfrontId) const
{
    Warfront const* wf = GetWarfront(warfrontId);
    return wf ? wf->ControllingTeam : TEAM_ALLIANCE;
}

Warfront const* WarfrontMgr::GetWarfrontByContributionMWS(uint32 managedWorldStateId) const
{
    if (!managedWorldStateId)
        return nullptr;

    for (auto const& [id, wf] : _warfronts)
        if (wf.ContributionMWS_Alliance == managedWorldStateId || wf.ContributionMWS_Horde == managedWorldStateId)
            return &wf;

    return nullptr;
}

void WarfrontMgr::SendWarfrontComplete(Warfront const& wf, TeamId winner) const
{
    // The SMSG_WARFRONT_COMPLETE body is byte-recovered now (see WarfrontPackets.h), but the flag stays the
    // operator's call - it also gates the contribution round-trip.
    if (!IsNativeUiEnabled())
        return;

    WorldPackets::Warfront::WarfrontComplete complete;
    // The client looks this up with C_PartyPose.GetPartyPoseInfoByMapID, so it must be the battle map the
    // winners just finished, not the warfront id.
    complete.MapID = int32(wf.GetChallengerBattleMap());
    // Faction group, which is the inverse of TeamId: PLAYER_FACTION_GROUP = { Horde = 0, Alliance = 1 }.
    complete.Winner = winner == TEAM_ALLIANCE ? 1 : 0;
    complete.Write();

    for (auto const& [accountId, session] : sWorld->GetAllSessions())
    {
        Player* player = session->GetPlayer();
        if (!player || !player->IsInWorld())
            continue;

        if (player->GetTeamId() != winner)
            continue;

        session->SendPacket(complete.GetRawPacket());
    }

    TC_LOG_INFO("warfront", "Warfront '{}': sent SMSG_WARFRONT_COMPLETE (mapID {}, winner factionGroup {}) to team {}.",
        wf.Name, complete.MapID, complete.Winner, uint32(winner));
}

void WarfrontMgr::OnContributionTargetReached(uint32 managedWorldStateId)
{
    // The bar that filled belongs to the challenger of exactly one warfront currently in CONTRIBUTION.
    for (auto& [id, wf] : _warfronts)
    {
        if (wf.State != WF_CONTRIBUTION)
            continue;

        if (managedWorldStateId && wf.GetChallengerContributionMWS() == managedWorldStateId)
        {
            TransitionToSiege(wf);
            return;
        }
    }
}

void WarfrontMgr::OnScenarioComplete(uint32 warfrontId, TeamId team)
{
    Warfront* wf = GetWarfront(warfrontId);
    if (!wf || wf->State != WF_SIEGE)
        return;

    // Only the challenger completing its assault scenario flips control.
    if (team != wf->ChallengingTeam)
        return;

    // The assault succeeded -> tell the winners' clients so Blizzard_WarfrontsPartyPoseUI shows the victory screen.
    // Fired BEFORE FlipControl because the flip swaps ChallengingTeam/ControllingTeam out from under `team`.
    SendWarfrontComplete(*wf, team);

    FlipControl(*wf);
}

bool WarfrontMgr::AdvanceWarfront(uint32 warfrontId)
{
    Warfront* wf = GetWarfront(warfrontId);
    if (!wf)
        return false;

    switch (wf->State)
    {
        case WF_CONTRIBUTION:
            TransitionToSiege(*wf);
            break;
        case WF_SIEGE:
            FlipControl(*wf);
            break;
        default:
            break;
    }
    return true;
}

WarfrontQueue* WarfrontMgr::GetQueue(uint32 warfrontId)
{
    auto itr = _queues.find(warfrontId);
    return itr != _queues.end() ? &itr->second : nullptr;
}

WarfrontQueue const* WarfrontMgr::GetQueue(uint32 warfrontId) const
{
    auto itr = _queues.find(warfrontId);
    return itr != _queues.end() ? &itr->second : nullptr;
}

void WarfrontMgr::OpenQueue(Warfront const& wf)
{
    if (WarfrontQueue* queue = GetQueue(wf.Id))
    {
        // Retail warfronts are 20-player; on a test realm the min is tunable (default 1) so a solo dev can launch
        // the assault. Set Warfront.QueueMinPlayers = 20 in worldserver.conf for the Blizzlike group requirement.
        uint32 const minPlayers = std::max<uint32>(1u, sConfigMgr->GetIntDefault("Warfront.QueueMinPlayers", 1));
        queue->Open(wf.ChallengingTeam, wf.GetChallengerBattleMap(), minPlayers);
    }
}

void WarfrontMgr::CloseQueue(uint32 warfrontId)
{
    if (WarfrontQueue* queue = GetQueue(warfrontId))
        queue->Close();
}

bool WarfrontMgr::HasUnlockedWarfront(Player const* player, uint32 warfrontId, std::string* reason /*= nullptr*/) const
{
    auto fail = [reason](std::string msg) { if (reason) *reason = std::move(msg); return false; };

    if (!player)
        return false;

    Warfront const* wf = GetWarfront(warfrontId);
    if (!wf)
        return fail("That warfront does not exist.");

    if (!IsUnlockGateEnabled())
        return true;

    TeamId const team = player->GetTeamId();

    // A character that already holds (or has completed) the terminal "Warfront: The Battle for X" quest has, by
    // definition, finished the whole intro chain - accept it without re-checking the earlier steps.
    if (uint32 const warfrontQuest = wf->GetWarfrontQuest(team))
        if (player->GetQuestStatus(warfrontQuest) != QUEST_STATUS_NONE || player->IsQuestRewarded(warfrontQuest))
            return true;

    auto questName = [](uint32 questId) -> std::string
    {
        if (Quest const* quest = sObjectMgr->GetQuestTemplate(questId))
            if (!quest->GetLogTitle().empty())
                return quest->GetLogTitle();

        return Trinity::StringFormat("quest {}", questId);
    };

    // 1) the war-campaign hurdle: the quest that opens World Quests. Without it the warfront intro never appears.
    if (uint32 const campaignQuest = wf->GetCampaignQuest(team))
    {
        if (!player->IsQuestRewarded(campaignQuest))
            return fail(Trinity::StringFormat("You have not yet earned your commander's trust. Complete \"{}\" first.",
                questName(campaignQuest)));
    }

    // 2) the terminal quest of this warfront's own intro chain.
    if (uint32 const unlockQuest = wf->GetUnlockQuest(team))
    {
        if (!player->IsQuestRewarded(unlockQuest))
            return fail(Trinity::StringFormat("The front is not yours to join yet. Finish \"{}\" to be cleared for the assault.",
                questName(unlockQuest)));
    }

    return true;
}

bool WarfrontMgr::GetContributionProgress(uint32 warfrontId, TeamId team, float& outFraction, int32& outProgress, int32& outTarget) const
{
    outFraction = 0.0f;
    outProgress = 0;
    outTarget = 0;

    Warfront const* wf = GetWarfront(warfrontId);
    if (!wf)
        return false;

    uint32 const managedWorldStateId = wf->GetContributionMWS(team);
    if (!managedWorldStateId)
        return false;

    ManagedWorldStateSnapshot snapshot;
    if (!sManagedWorldStateMgr->GetSnapshot(managedWorldStateId, snapshot))
        return false;

    outProgress = snapshot.Progress;
    outTarget = snapshot.Target;

    // The bar's empty position is the depletion floor, not zero - normalise against the usable span.
    int32 const span = snapshot.Target - snapshot.Floor;
    if (span > 0)
        outFraction = std::clamp(float(snapshot.Progress - snapshot.Floor) / float(span), 0.0f, 1.0f);

    return true;
}

bool WarfrontMgr::CanQueue(Player* player, uint32 warfrontId, std::string* reason /*= nullptr*/, bool ignoreUnlockGate /*= false*/) const
{
    auto fail = [reason](char const* msg) { if (reason) *reason = msg; return false; };

    if (!player)
        return false;

    Warfront const* wf = GetWarfront(warfrontId);
    if (!wf)
        return fail("That warfront does not exist.");

    // Blizzlike gate first, so the player is told about the war campaign rather than about the cycle phase.
    if (!ignoreUnlockGate && !HasUnlockedWarfront(player, warfrontId, reason))
        return false;

    if (wf->State != WF_SIEGE)
        return fail("The assault is not available yet - the war effort is still gathering strength.");

    // Only the challenging faction assaults; the controlling faction is Patrolling the outdoor zone.
    if (player->GetTeamId() != wf->ChallengingTeam)
        return fail("Your faction currently controls this zone - there is no assault for you to join.");

    if (player->GetLevel() < WARFRONT_MIN_LEVEL)
        return fail("You are not yet experienced enough to join the assault.");

    WarfrontQueue const* queue = GetQueue(warfrontId);
    if (!queue || !queue->IsOpen())
        return fail("The assault is not accepting recruits right now.");

    if (queue->IsEnrolled(player->GetGUID()))
        return fail("You are already enrolled for the assault.");

    return true;
}

bool WarfrontMgr::EnqueuePlayer(Player* player, uint32 warfrontId, std::string* reason /*= nullptr*/, bool ignoreUnlockGate /*= false*/)
{
    if (!CanQueue(player, warfrontId, reason, ignoreUnlockGate))
        return false;

    WarfrontQueue* queue = GetQueue(warfrontId);
    if (!queue)
        return false;

    return queue->Enqueue(player);
}

uint32 WarfrontMgr::GetWarfrontForLfgDungeon(uint32 lfgDungeonId, uint32* outMapId /*= nullptr*/)
{
    if (outMapId)
        *outMapId = 0;

    if (!lfgDungeonId)
        return 0;

    for (WarfrontLfgDungeonEntry const& row : WarfrontLfgDungeons)
    {
        if (row.LfgDungeonId != lfgDungeonId)
            continue;

        if (outMapId)
            *outMapId = row.BattleMapId;
        return row.WarfrontId;
    }

    return 0;
}

uint32 WarfrontMgr::GetLfgDungeonForWarfront(uint32 warfrontId, uint32 battleMapId)
{
    for (WarfrontLfgDungeonEntry const& row : WarfrontLfgDungeons)
        if (row.WarfrontId == warfrontId && row.BattleMapId == battleMapId && !row.Heroic)
            return row.LfgDungeonId;

    return 0;
}

bool WarfrontMgr::SendOpenLfgDungeonFinder(Player* player, uint32 warfrontId) const
{
    // The SMSG_OPEN_LFG_DUNGEON_FINDER body is INFERRED (a single uint32 dungeon id) - never put it on the wire
    // unless the operator has deliberately opted in.
    if (!player || !IsNativeUiEnabled())
        return false;

    Warfront const* wf = GetWarfront(warfrontId);
    if (!wf)
        return false;

    // Preselect the dungeon that matches the *current challenger's* battle map, i.e. the assault that is actually
    // runnable right now.
    uint32 const dungeonId = GetLfgDungeonForWarfront(warfrontId, wf->GetChallengerBattleMap());
    if (!dungeonId)
        return false;

    WorldPackets::LFG::OpenLfgDungeonFinder packet(dungeonId);
    player->SendDirectMessage(packet.Write());

    TC_LOG_INFO("warfront", "Warfront '{}': sent SMSG_OPEN_LFG_DUNGEON_FINDER (LFGDungeon {}) to player {}.",
        wf->Name, dungeonId, player->GetName());
    return true;
}

bool WarfrontMgr::DevJoinAssault(Player* player, uint32 warfrontId, std::string* reason /*= nullptr*/)
{
    if (!player)
        return false;

    Warfront* wf = GetWarfront(warfrontId);
    if (!wf)
    {
        if (reason)
            *reason = "Unknown warfront id.";
        return false;
    }

    // Make the caller's faction the attacker so a solo dev on either side can launch, regardless of who currently
    // owns the zone: the opposite team becomes the (NPC) controller.
    TeamId const myTeam = player->GetTeamId();
    wf->ControllingTeam = (myTeam == TEAM_ALLIANCE) ? TEAM_HORDE : TEAM_ALLIANCE;
    wf->ChallengingTeam = myTeam;

    // Force the zone into SIEGE (opens the queue routed to the challenger's battle map). If already sieged, re-open
    // for the new challenger so the queue/battle-map match the caller's faction.
    if (wf->State != WF_SIEGE)
    {
        wf->State = WF_CONTRIBUTION;    // guarantee a valid CONTRIBUTION -> SIEGE transition
        TransitionToSiege(*wf);
    }
    else
    {
        SaveWarfront(*wf);
        OpenQueue(*wf);
    }

    // Enroll the caller; at the test min-player floor Enqueue immediately forms the battle group and teleports the
    // player into the assault instance (WarfrontQueue::FormBattleGroup), where the scripted scenario musters the boss.
    // This is the GM/testing override, so it deliberately bypasses the war-campaign unlock gate.
    return EnqueuePlayer(player, warfrontId, reason, true);
}
