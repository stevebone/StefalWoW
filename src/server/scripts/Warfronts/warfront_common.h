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

#ifndef TRINITYCORE_WARFRONT_SCRIPT_COMMON_H
#define TRINITYCORE_WARFRONT_SCRIPT_COMMON_H

#include "ChatPackets.h"
#include "Config.h"
#include "Creature.h"
#include "DB2Structure.h"
#include "Group.h"             // PingSubjectType
#include "InstanceScenario.h"
#include "InstanceScript.h"
#include "Log.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "ObjectDefines.h"     // VisibilityDistanceType
#include "PartyPackets.h"
#include "Player.h"
#include "Scenario.h"
#include "SharedDefines.h"
#include "StringFormat.h"
#include "TemporarySummon.h"
#include "Timer.h"
#include "UnitDefines.h"
#include "Warfront.h"          // WarfrontId / TeamId
#include "World.h"
#include <algorithm>
#include <array>
#include <string>

// Shared building blocks for the two warfront battle-map controllers (instance_warfront_arathi/darkshore.cpp) and the
// final-boss AI (warfront_bosses.cpp). See WARFRONTS_DESIGN.md §4.
namespace WarfrontBattle
{
    // Faction templates every warfront combatant on these four maps uses (verified against creature_template on maps
    // 1876/1943/2105/2111 and FactionTemplate.db2 for build 12.0.7):
    //   2958 -> Faction 66  (Horde),    FactionGroup 4, FriendGroup 4 (Horde),    EnemyGroup 10 (Alliance|Monster)
    //   2959 -> Faction 189 (Alliance), FactionGroup 2, FriendGroup 2 (Alliance), EnemyGroup 12 (Horde|Monster)
    // The mustered final boss is forced into the DEFENDER's template so it is guaranteed hostile to the assaulting
    // raid. This is not cosmetic - see MusterFinalBoss() for the two shipped templates that are unattackable without
    // it (146628 Sira Moonwarden ships as FactionTemplate 2854 = The Wardens, FriendGroup 0 / EnemyGroup 0, i.e.
    // hostile to nobody; 82877 High Warlord Volrath ships as 877, FriendGroup 4 = friendly to its own Horde
    // attackers on map 1876).
    inline constexpr uint32 WARFRONT_FACTION_HORDE    = 2958;
    inline constexpr uint32 WARFRONT_FACTION_ALLIANCE = 2959;

    // Per battle-map configuration. FinalBossEntry is mustered by the instance script when the scenario reaches its
    // last step; that boss's death (warfront_bosses.cpp) completes the scenario and flips zone control.
    struct MapInfo
    {
        uint32 MapId;              // battle map id (1876/1943 Stromgarde, 2105/2111 Darkshore)
        uint32 WarfrontId;
        TeamId AttackingTeam;      // the only faction that assaults this map (== the challenger)
        uint32 ScenarioId;         // faction scenario bound via the `scenarios` table (reference / logging)
        uint32 FinalBossEntry;     // High Warlord Volrath / Maiev Shadowsong / Sira Moonwarden
        float  BossX, BossY, BossZ, BossO;   // where the final boss musters - MUST be deep in defender-held ground
        // The party's landing spot, mirrored from game/Warfronts/WarfrontQueue.cpp GetBattleEntry(). Kept here purely
        // so the "the boss must NOT spawn on top of the raid's landing zone" invariant is checkable - both at compile
        // time (static_assert below) and at runtime (MusterFinalBoss). Keep the two tables in sync.
        float  LandingX, LandingY, LandingZ;
        uint32 StepCount;          // scenario step count (Stromgarde 3, Darkshore 11) - logging only
        char const* BossLandmark;  // human-readable "where is it" hint used in the sighting announcement
        char const* ObjectiveHint; // fallback objective text when a scenario step carries no DB2 Description/Title
    };

    // ---------------------------------------------------------------------------------------------------------------
    // Final-boss muster positions
    //
    // Every one of these must sit in ground the DEFENDER holds, far from the attacker's landing zone. Until 12.0.7 the
    // two Darkshore rows carried the *landing* coordinates verbatim, so the boss materialised silently behind the raid
    // at the spot they had just spawned on and the assault was unfindable. That is the bug this table fixes.
    //
    // (Coordinates below use WoW's convention: +X is NORTH, +Y is WEST, +Z is up.)
    //
    // 2105 (Alliance assault on Darkshore) - Sira Moonwarden 146628 has NO spawn of her own on this map (integ_world
    //   only spawns her on 2111), so there is no datamined retail location to reuse. This is therefore a SERVER-AUTHORED
    //   placement, not a retail one. Chosen: the Horde foundry camp at the far northern end of the battlefield, the
    //   deepest enemy-held position on the map, ~1320 yd from the landing at (6268.43, 558.82). It is the natural end
    //   of the raid's push up the shore and it is defended by real content: Deathguard Champion 145129 @
    //   7405.44,-119.39 z 3.20, Magmaster Blastcrank 146125 @ 7421.64,-122.38 z 1.87, Forsaken Alchemist 144974 @
    //   7413.10,-124.84 z 2.18 and 7414.70,-114.29 z 2.13, plus six Forsaken Deathguard 144976 @ 7398-7401 /
    //   -107..-130, z 3.85..4.46 (21 spawns inside an 80 yd radius in total).
    //   Z sanity: every neighbour within ~15 yd of (7400, -119) sits between z 1.87 and z 4.46, so z 4.30 is on the
    //   camp floor - deliberately a hair above it so the summon settles down onto the ground rather than clipping
    //   through it. Facing o 2.60 rad = atan2(558.82 - -119.00, 6268.43 - 7400.00), i.e. turned back down the approach
    //   lane at the incoming raid.
    // 2111 (Horde assault on Darkshore) - Maiev Shadowsong 149098 IS spawned in-DB on this map, so her real position
    //   is used verbatim: 7652.68, 1.32, -1.55 (o 0.09), the northern-most of her ten spawns. Z is corroborated by a
    //   different entry standing beside her, Forsaken Lancer 144800 @ 7640.46,-10.90 z -1.55. ~913 yd from the landing
    //   at (6740.36, 35.31).
    // 1876/1943 (Stromgarde) - these maps carry no DB spawns at all; the Stromgarde keep ruins were already used and
    //   sit ~405 yd from the Arathi landing, so they are kept unchanged.
    // ---------------------------------------------------------------------------------------------------------------
    inline constexpr std::array<MapInfo, 4> BattleMaps =
    {{
        // Arathi (Stromgarde) - boss musters in the keep ruins, the raid lands on the open ground north-west of it.
        { 1876, WARFRONT_STROMGARDE, TEAM_HORDE,    1346, 82877,  -1418.72f, -2538.15f,  74.12f, 3.40f,
                 -1050.00f, -2370.00f, 53.00f,  3, "the Stromgarde keep ruins",
                 "Push into Stromgarde Keep and break the defenders." },
        { 1943, WARFRONT_STROMGARDE, TEAM_ALLIANCE, 1568, 82877,  -1418.72f, -2538.15f,  74.12f, 3.40f,
                 -1050.00f, -2370.00f, 53.00f,  3, "the Stromgarde keep ruins",
                 "Push into Stromgarde Keep and break the defenders." },
        // Darkshore.
        { 2105, WARFRONT_DARKSHORE,  TEAM_ALLIANCE, 1697, 146628,  7400.00f,  -119.00f,   4.30f, 2.60f,
                  6268.43f,   558.82f,  3.18f, 11, "the Horde foundry camp at the far north end of the battlefield",
                  "Push north along the shore and break the Horde foundry camp." },   // Sira Moonwarden
        { 2111, WARFRONT_DARKSHORE,  TEAM_HORDE,    1701, 149098,  7652.68f,     1.32f,  -1.55f, 0.09f,
                  6740.36f,    35.31f, 46.65f, 11, "the Kaldorei redoubt at the far north end of the battlefield",
                  "Push north through the ruins and break the Kaldorei redoubt." },   // Maiev Shadowsong
    }};

    // The boss must never muster on the party's landing spot - that is exactly the "where do I go?" bug. 100 yd is a
    // deliberately loose floor: it only catches a coordinate block copy-pasted from WarfrontQueue's landing table.
    constexpr bool BossIsAwayFromLanding(MapInfo const& info)
    {
        float const dx = info.BossX - info.LandingX;
        float const dy = info.BossY - info.LandingY;
        return (dx * dx + dy * dy) > (100.0f * 100.0f);
    }

    constexpr bool AllBossesAreAwayFromLanding()
    {
        for (MapInfo const& info : BattleMaps)
            if (!BossIsAwayFromLanding(info))
                return false;
        return true;
    }

    static_assert(AllBossesAreAwayFromLanding(),
        "A warfront final boss is mustered on (or within 100 yd of) the party's landing position. The boss would "
        "materialise behind the raid at the spot they just spawned on and the assault would have no destination. "
        "Move the Boss* coordinates deep into defender-held ground.");

    inline MapInfo const* GetMapInfo(uint32 mapId)
    {
        for (MapInfo const& info : BattleMaps)
            if (info.MapId == mapId)
                return &info;
        return nullptr;
    }

    // The scripted assault controller shared by both zones. It drives the server-authored scenario progression:
    // the warfront scenario steps have no DB2 CriteriaTree (progression is server-authored), so we advance them on a
    // short timer until the final step, at which point we muster the boss. The boss's death completes the last step.
    //
    // TODO(RTS): steps 1..N-1 model the retail resource/base/troop economy (WARFRONTS_DESIGN.md §4.2A) for which no
    // server data exists. They are advanced here as a clearly-scripted timed progression so the assault reaches its
    // boss and grants identical credit/reward. Real economy is a content wall - see WARFRONTS_STATUS.md.
    struct BattleInstanceScript : public InstanceScript
    {
        explicit BattleInstanceScript(InstanceMap* map) : InstanceScript(map),
            _info(GetMapInfo(map->GetId())), _advanceTimer(0), _beaconTimer(0), _started(false), _bossSummoned(false)
        {
            SetHeaders("WFB");
        }

        // Begin the scripted assault timeline once the first challenger lands in the instance.
        void OnPlayerEnter(Player* player) override
        {
            _started = true;
            if (!player)
                return;

            // Somebody joining (or reconnecting) after the muster would otherwise have no idea a boss is up or where.
            if (_bossSummoned)
            {
                SendAnnouncement(player, BuildSightingText(), true);
                SendBossPing(player);
                return;
            }

            // Otherwise tell them what the assault is currently trying to do, so nobody ever lands with no objective.
            InstanceScenario* scenario = instance->GetInstanceScenario();
            if (std::string const text = BuildObjectiveText(scenario ? scenario->GetStep() : nullptr); !text.empty())
                SendAnnouncement(player, text, false);
        }

        void Update(uint32 diff) override
        {
            if (!_started || !_info)
                return;

            if (_bossSummoned)
            {
                // The client's world pin is a timed pin - it fades. Re-ping on a slow loop (silently, no chat) so the
                // marker stays on screen and on the minimap for as long as the fight is unfinished.
                _beaconTimer += diff;
                if (_beaconTimer >= BeaconIntervalMs)
                {
                    _beaconTimer = 0;
                    instance->DoOnPlayers([this](Player* player) { SendBossPing(player); });
                }
                return;
            }

            // Examination mode: with Warfront.Battle.AutoAdvance = 0 the scenario holds its state so a tester can walk
            // the map indefinitely, then drive it with `.warfront step` / `.warfront boss`.
            if (!sConfigMgr->GetBoolDefault("Warfront.Battle.AutoAdvance", true))
                return;

            _advanceTimer += diff;
            if (_advanceTimer < StepIntervalMs())
                return;
            _advanceTimer = 0;

            AdvanceOneStep();
        }

        // GM/testing hook (driven by the .warfront step/boss commands via InstanceScript::SetData).
        void SetData(uint32 type, uint32 /*data*/) override
        {
            if (type == WARFRONT_CMD_ADVANCE_STEP)
                AdvanceOneStep();
            else if (type == WARFRONT_CMD_MUSTER_BOSS)
                MusterFinalBoss(nullptr);
        }

        // `.warfront boss` passes the commanding GM's guid so the fight can be dropped on top of the tester instead of
        // at the scripted muster point (see cs_warfront.cpp).
        void SetGuidData(uint32 type, ObjectGuid value) override
        {
            if (type != WARFRONT_CMD_MUSTER_BOSS_AT_PLAYER)
                return;

            Player* at = ObjectAccessor::FindConnectedPlayer(value);
            MusterFinalBoss(at && at->GetMap() == instance ? at : nullptr);
        }

        // `.warfront goboss` reads the live boss guid back out so it can teleport the caller to the fight.
        ObjectGuid GetGuidData(uint32 type) const override
        {
            if (type == WARFRONT_DATA_FINAL_BOSS_GUID)
                return _bossGuid;
            return InstanceScript::GetGuidData(type);
        }

    private:
        static constexpr uint32 BeaconIntervalMs     = 60 * IN_MILLISECONDS;   // world-pin refresh cadence
        static constexpr int32  BeaconPingDurationMs = 75 * IN_MILLISECONDS;   // slightly longer, so pins never gap

        static uint32 StepIntervalMs()
        {
            // Admin-tunable so the whole assault can be compressed for testing.
            uint32 secs = sConfigMgr->GetIntDefault("Warfront.Battle.StepSeconds", 30);
            return std::max<uint32>(1u, secs) * IN_MILLISECONDS;
        }

        // ---- player-facing messaging -------------------------------------------------------------------------------

        // A raid warning is the loudest thing an InstanceScript can reach: Map::SendToPlayers broadcasts to every
        // player on this instanced copy, and CHAT_MSG_RAID_WARNING renders as the big centre-screen banner *and* a
        // chat line. The same text is mirrored as CHAT_MSG_SYSTEM so it survives in the chat log after the banner
        // fades. There is no InstanceScript-level "SendWorldText" - World::SendWorldText is realm-wide and would leak
        // the message to everyone online, so it is deliberately not used here.
        void BroadcastAnnouncement(std::string const& text, bool raidWarning) const
        {
            WorldPackets::Chat::Chat packet;
            if (raidWarning)
            {
                packet.Initialize(CHAT_MSG_RAID_WARNING, LANG_UNIVERSAL, nullptr, nullptr, text);
                instance->SendToPlayers(packet.Write());
            }

            WorldPackets::Chat::Chat sysPacket;
            sysPacket.Initialize(CHAT_MSG_SYSTEM, LANG_UNIVERSAL, nullptr, nullptr, text);
            instance->SendToPlayers(sysPacket.Write());
        }

        static void SendAnnouncement(Player* player, std::string const& text, bool raidWarning)
        {
            if (!player)
                return;

            WorldPackets::Chat::Chat packet;
            if (raidWarning)
            {
                packet.Initialize(CHAT_MSG_RAID_WARNING, LANG_UNIVERSAL, nullptr, nullptr, text);
                player->SendDirectMessage(packet.Write());
            }

            WorldPackets::Chat::Chat sysPacket;
            sysPacket.Initialize(CHAT_MSG_SYSTEM, LANG_UNIVERSAL, nullptr, nullptr, text);
            player->SendDirectMessage(sysPacket.Write());
        }

        // World-visible marker. SMSG_RECEIVE_PING_WORLD_POINT is the retail group-ping pin: the client draws a pin in
        // the 3D world at the given point *and* on the minimap/world map edge, with a direction arrow, for the given
        // duration. It is the only marker primitive TC exposes that can be aimed at an arbitrary coordinate from an
        // InstanceScript.
        //   * SenderGUID is deliberately set to the RECEIVING player's own guid: the ping frame is attributed to the
        //     sender, and a guid the client cannot resolve to a party member is the one case that could be dropped.
        //     "You pinged it" always resolves.
        //   * Scenario POIs (SMSG_SCENARIO_POIS) are NOT usable here: ScenarioMgr keys POI blobs by CriteriaTreeID and
        //     the warfront scenario steps carry no CriteriaTree (progression is server-authored), so there is nothing
        //     to attach a blob to. See ScenarioMgr::GetScenarioPOIs.
        void SendBossPing(Player* player) const
        {
            if (!player || !_info)
                return;

            WorldPackets::Party::ReceivePingWorldPoint ping;
            ping.SenderGUID = player->GetGUID();
            ping.MapID = instance->GetId();
            ping.Point = _bossPos;
            ping.Type = PingSubjectType::Attack;
            ping.PinFrameID = WarfrontBossPinFrameId;
            ping.Transport = ObjectGuid::Empty;
            ping.PingDuration = Milliseconds(BeaconPingDurationMs);
            player->SendDirectMessage(ping.Write());
        }

        std::string BuildSightingText() const
        {
            return Trinity::StringFormat("{} has been sighted at {:.0f}, {:.0f} - {}! Slay the enemy commander to win "
                "the battle. (A world marker has been placed; `.warfront goboss` teleports you there.)",
                _bossName, _bossPos.GetPositionX(), _bossPos.GetPositionY(),
                _info->BossLandmark ? _info->BossLandmark : "deep in enemy ground");
        }

        // Name the objective the raid is on right now. Prefer the scenario step's own DB2 text (Description, then
        // Title); the warfront steps are frequently text-less, so fall back to the per-map scripted objective.
        // Returns an empty string when there is nothing meaningful to say.
        std::string BuildObjectiveText(ScenarioStepEntry const* step) const
        {
            char const* text = nullptr;
            if (step)
            {
                LocaleConstant const locale = sWorld->GetDefaultDbcLocale();
                if (step->Description[locale] && *step->Description[locale])
                    text = step->Description[locale];
                else if (step->Title[locale] && *step->Title[locale])
                    text = step->Title[locale];
            }

            if (!text && _info)
                text = _info->ObjectiveHint;
            if (!text)
                return std::string();

            if (step)
                return Trinity::StringFormat("Objective {}/{}: {}",
                    uint32(step->OrderIndex) + 1, _info ? _info->StepCount : uint32(0), text);
            return Trinity::StringFormat("Objective: {}", text);
        }

        // ---- scenario progression ----------------------------------------------------------------------------------

        void AdvanceOneStep()
        {
            InstanceScenario* scenario = instance->GetInstanceScenario();
            if (!scenario)
            {
                // Scenario failed to attach (e.g. no `scenarios` row matched this difficulty). The battle must still
                // be winnable, so muster the boss immediately: its death flips control via OnScenarioComplete.
                MusterFinalBoss(nullptr);
                return;
            }

            ScenarioStepEntry const* step = scenario->GetStep();
            if (!step || step == scenario->GetLastStep())
            {
                // Reached the final step (destroy the gate / defeat the commander) - stop the scripted advance and
                // muster the boss. Its death completes this last step (see warfront_bosses.cpp).
                MusterFinalBoss(nullptr);
                return;
            }

            // Scripted RTS placeholder: mark the current economy/objective step done and advance to the next.
            // TODO(RTS): real resource/base/troop economy - content wall, see WARFRONTS_STATUS.md
            scenario->SetStepState(step, SCENARIO_STEP_DONE);
            scenario->CompleteStep(step);

            // Tell the raid what they are now supposed to be doing. Without this the scenario tracker silently ticks
            // over and the player has no idea anything happened.
            if (std::string const text = BuildObjectiveText(scenario->GetStep()); !text.empty())
                BroadcastAnnouncement(text, false);
        }

        // `at` (optional) overrides the scripted muster point - used by `.warfront boss` so a tester always gets the
        // fight where they are standing.
        void MusterFinalBoss(Player* at)
        {
            if (_bossSummoned || !_info)
                return;
            _bossSummoned = true;
            _beaconTimer = 0;

            if (at)
            {
                // Land the fight on the tester: 5 yd directly in front of them, turned around to face them.
                Position pos = at->GetPosition();
                at->MovePosition(pos, 5.0f, 0.0f);
                pos.SetOrientation(Position::NormalizeOrientation(at->GetOrientation() + float(M_PI)));
                _bossPos = pos;
            }
            else
            {
                _bossPos = Position(_info->BossX, _info->BossY, _info->BossZ, _info->BossO);

                // Runtime twin of the static_assert above. It cannot fire for the shipped table, but it guards the
                // next person who edits either coordinate block: a boss on the landing zone is invisible content.
                if (!BossIsAwayFromLanding(*_info))
                    TC_LOG_ERROR("warfront", "Warfront battle map {}: final boss {} musters on the party's landing "
                        "position ({:.2f}, {:.2f}) - players will never find the fight. Fix WarfrontBattle::BattleMaps.",
                        instance->GetId(), _info->FinalBossEntry, _info->LandingX, _info->LandingY);
            }

            TempSummon* boss = instance->SummonCreature(_info->FinalBossEntry, _bossPos);
            if (!boss)
            {
                TC_LOG_ERROR("warfront", "Warfront battle map {}: failed to summon final boss {} at {:.2f}, {:.2f}, {:.2f}.",
                    instance->GetId(), _info->FinalBossEntry, _bossPos.GetPositionX(), _bossPos.GetPositionY(),
                    _bossPos.GetPositionZ());
                return;
            }

            PrepareBossForBattle(boss);

            _bossGuid = boss->GetGUID();
            _bossName = boss->GetName();
            if (_bossName.empty())
                _bossName = "The enemy commander";

            TC_LOG_INFO("warfront", "Warfront battle map {}: final boss {} ({}) mustered for the final step at "
                "{:.2f}, {:.2f}, {:.2f} (faction {}, {}){}.",
                instance->GetId(), _bossName, _info->FinalBossEntry, _bossPos.GetPositionX(), _bossPos.GetPositionY(),
                _bossPos.GetPositionZ(), boss->GetFaction(),
                _info->BossLandmark ? _info->BossLandmark : "scripted muster point",
                at ? " [forced to the commanding GM's position]" : "");

            std::string const text = BuildSightingText();
            BroadcastAnnouncement(text, true);
            instance->DoOnPlayers([this](Player* player) { SendBossPing(player); });
        }

        // Guarantee the mustered boss actually fights the raid that came to kill it.
        //
        // FINDING (12.0.7 integ_world + FactionTemplate.db2): two of the three final-boss templates ship with a
        // faction that cannot fight their own attacker.
        //   * 146628 Sira Moonwarden (map 2105, ALLIANCE assault) is FactionTemplate 2854 -> Faction 1894 "The
        //     Wardens", FactionGroup 0, FriendGroup 0, EnemyGroup 0. Hostile to nobody: she could never be attacked,
        //     so the win condition was unreachable.
        //   * 82877 High Warlord Volrath (maps 1876 + 1943) is FactionTemplate 877 -> FriendGroup 4 (Horde),
        //     EnemyGroup 10 (Alliance|Monster). On 1876 the attacker IS the Horde, so he was friendly to his own
        //     assault raid.
        //   * 149098 Maiev Shadowsong (map 2111, HORDE assault) is 2959 (Alliance) and was already correct.
        // Forcing the DEFENDER's warfront faction template fixes all three uniformly and is what every other combatant
        // on these maps already uses.
        void PrepareBossForBattle(Creature* boss) const
        {
            uint32 const defenderFaction = (_info->AttackingTeam == TEAM_ALLIANCE)
                ? WARFRONT_FACTION_HORDE : WARFRONT_FACTION_ALLIANCE;
            boss->SetFaction(defenderFaction);

            // Strip anything on the template that would make it unattackable/passive (146628 ships with
            // UNIT_FLAG_IMMUNE_TO_NPC set, and the ambient Darkshore copies of the same models add IMMUNE_TO_PC).
            boss->RemoveUnitFlag(UnitFlags(UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_ATTACKABLE_1 | UNIT_FLAG_IMMUNE_TO_PC
                | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_NON_ATTACKABLE_2 | UNIT_FLAG_PACIFIED | UNIT_FLAG_UNINTERACTIBLE));
            boss->SetImmuneToAll(false);
            boss->SetReactState(REACT_AGGRESSIVE);
            boss->SetHomePosition(_bossPos);

            // Make it findable in the world, not just in chat: force the nameplate on, keep the boss's grid loaded and
            // override its visibility distance so the raid can see and target it from anywhere on the battle map.
            boss->SetUnitFlag(UNIT_FLAG_FORCE_NAMEPLATE);
            boss->setActive(true);
            boss->SetFarVisible(true);
            boss->SetVisibilityDistanceOverride(VisibilityDistanceType::Infinite);
        }

        MapInfo const* _info;
        uint32 _advanceTimer;
        uint32 _beaconTimer;
        bool _started;
        bool _bossSummoned;
        ObjectGuid _bossGuid;
        Position _bossPos;
        std::string _bossName;
    };
}

#endif // TRINITYCORE_WARFRONT_SCRIPT_COMMON_H
