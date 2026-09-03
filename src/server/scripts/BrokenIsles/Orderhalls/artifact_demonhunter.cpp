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

// Demon Hunter Legion artifact acquisitions. Per-class Broken Isles content (cf. zone_orderhall_hunter.cpp); the
// generic class-hall framework lives in orderhall_legion.cpp, the shared scenario helpers in
// orderhall_artifact_common.h.
//
// Both DH acquisitions are authored as InstanceScenarios in retail, but on this server the acquisition quests carry
// ONLY plain kill-credit objectives (verified in quest_objectives) - there is no Type-14 scenario objective on either
// quest - so the quest completes on the flight credit + the boss-death credit alone. The scenario itself is not present
// in the world `scenarios` table for either map, so it will not auto-start; we still keep a director that advances the
// on-screen scenario steps by hand IF one is running (harmless no-op otherwise), mirroring the Hunter's Titanstrike.
//
//   Havoc - "The Hunt" (39247) -> Twinblades of the Deceiver (item 127829)
//        Fel Hammer (map 1220) --(Illidari Fel Bat 94321, obj 0 flight credit)--> Felsoul Hold (map 1498, scenario 900)
//        -> slay Varedis Felsoul (94836) -> wield-credit 114515 (obj 1) -> return to the Fel Hammer to hand in.
//
//   Vengeance - "Vengeance Will Be Ours" (41863; phase duplicate 40249) -> Aldrachi Warblades (item 128832)
//        Fel Hammer (map 1220) --(Illidari Fel Bat 99250, obj 0 flight credit)--> Broken Shore (map 1500, scenario 961)
//        -> slay Caria Felsoul (99184) -> warblades-credit 114514 (obj 1) -> ride back to Dalaran, credit 97377 (obj 2)
//        -> hand in to Kor'vas Bloodthorn (102799).

#include "orderhall_artifact_common.h"
#include "Creature.h"
#include "EventProcessor.h"
#include "Log.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "QuestDef.h"
#include "Scenario.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"
#include "SharedDefines.h"
#include "Unit.h"

enum DemonHunterArtifactData
{
    // --- Havoc: "The Hunt" (39247) -> Twinblades of the Deceiver ---
    QUEST_THE_HUNT               = 39247,
    MAP_FELSOUL_HOLD             = 1498,   // scenario 900 instance ("AcquisitionHavoc")
    NPC_VAREDIS_FELSOUL          = 94836,  // final boss (faction 16 hostile)
    NPC_HAVOC_DIRECTOR           = 94902,  // Kayn Sunfury (faction 2838) - spawned on 1498 as the scenario director
    CREDIT_FLY_FELSOUL_HOLD      = 94321,  // obj 0 "Fly to Felsoul Hold"
    CREDIT_TWINBLADES            = 114515, // obj 1 "Twinblades of the Deceiver" (invisible kill-credit)

    // --- Vengeance: "Vengeance Will Be Ours" (41863 / phase-dup 40249) -> Aldrachi Warblades ---
    QUEST_VENGEANCE_WILL_BE_OURS = 41863,
    MAP_BROKEN_SHORE_SCENARIO    = 1500,   // scenario 961 instance ("ArtifactPaladinRetAcquisition", shared map)
    NPC_ALLARI_SOULEATER         = 98882,  // ally/director (faction 2846, already spawned)
    NPC_CARIA_FELSOUL            = 99184,  // final boss (faction 16 hostile)
    CREDIT_FLY_BROKEN_SHORE      = 99250,  // obj 0 "Fly to the Broken Shore"
    CREDIT_ALDRACHI_WARBLADES    = 114514, // obj 1 "Aldrachi Warblades" (invisible kill-credit)
    CREDIT_RETURN_DALARAN        = 97377,  // obj 2 "Return to Dalaran"

    MAP_FEL_HAMMER               = 1220
};

// Landing on Felsoul Hold (map 1498), slightly back from Fel Commander Igrius (95285 @ 1201,4994) and facing the run
// down toward Varedis (94836 @ 968,4828). The Havoc director (Kayn) is spawned at this same point.
static constexpr Position FelsoulHoldLanding = { 1230.0f, 5010.0f, 58.0f, 3.90f };

// Landing on the Broken Shore scenario map (map 1500), beside Allari the Souleater (98882 @ -2507.5,115.5), the
// ally/director; Caria (99184) waits deep in the pit at (-2791,-264).
static constexpr Position BrokenShoreLanding = { -2505.0f, 118.0f, 9.4f, 6.00f };

// The Fel Hammer hand-in point, beside Kor'vas Bloodthorn (102799), the ender for both acquisition quests.
static constexpr Position FelHammerReturn = { -832.70f, 4257.74f, 746.25f, 4.76f };

// =====================================================================================================================
// Shared scenario director. Bound to Kayn (94902) on Felsoul Hold and to Allari (98882) on the Broken Shore. It sets
// itself active on its scenario map and advances the on-screen InstanceScenario one step for each distance gate the
// party clears from the landing. If no scenario is running (the world `scenarios` row is absent), every advance is a
// harmless no-op - the quest completes on the flight + boss-death kill-credits, never on scenario steps.
struct npc_dh_artifact_scenario_director : public ScriptedAI
{
    npc_dh_artifact_scenario_director(Creature* creature) : ScriptedAI(creature) { }

    uint32   _pollTimer = 0;
    uint8    _advanced  = 0;
    bool     _active    = false;
    Position _origin;

    void Reset() override
    {
        uint32 const mapId = me->GetMap()->GetId();
        if (mapId == MAP_FELSOUL_HOLD || mapId == MAP_BROKEN_SHORE_SCENARIO)
        {
            _active = true;
            _origin = me->GetPosition();
            me->setActive(true);
        }
    }

    void UpdateAI(uint32 diff) override
    {
        if (!_active)
            return;

        _pollTimer += diff;
        if (_pollTimer < 1000)
            return;
        _pollTimer = 0;

        // One authored step per gate the party pushes past the landing (monotonic, so rushing ahead still counts).
        static constexpr float gates[6] = { 50.0f, 110.0f, 170.0f, 230.0f, 290.0f, 350.0f };
        if (_advanced < 6 && ArtifactScenarioOrder(me) == _advanced)
        {
            if (ArtifactPlayerBeyond(me, _origin, gates[_advanced]))
            {
                ArtifactAdvanceScenario(me);
                ++_advanced;
            }
        }
    }
};

// =====================================================================================================================
// Havoc - "The Hunt" (39247).
//
// On accept, fly to Felsoul Hold: grant the flight credit (obj 0) and drop the player at the scenario landing beside
// the Kayn director. (The retail on-rails fel-bat flight is polish; this transfer is the functional stand-in.)
class HavocHuntFlightEvent : public BasicEvent
{
public:
    explicit HavocHuntFlightEvent(Player* player) : _player(player) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (_player->IsInWorld())
        {
            _player->KilledMonsterCredit(CREDIT_FLY_FELSOUL_HOLD);                                        // obj 0
            _player->TeleportTo(MAP_FELSOUL_HOLD, FelsoulHoldLanding.GetPositionX(), FelsoulHoldLanding.GetPositionY(),
                FelsoulHoldLanding.GetPositionZ(), FelsoulHoldLanding.GetOrientation());
        }
        return true;
    }

private:
    Player* _player;
};

// After Varedis falls and the Twinblades are claimed, ride back to the Fel Hammer so "The Hunt" can be handed in to
// Kor'vas Bloodthorn (102799) - Havoc has no Dalaran leg, but its ender sits on the Fel Hammer (map 1220).
class HavocReturnEvent : public BasicEvent
{
public:
    explicit HavocReturnEvent(Player* player) : _player(player) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (_player->IsInWorld() && _player->GetMapId() == MAP_FELSOUL_HOLD)
            _player->TeleportTo(MAP_FEL_HAMMER, FelHammerReturn.GetPositionX(), FelHammerReturn.GetPositionY(),
                FelHammerReturn.GetPositionZ(), FelHammerReturn.GetOrientation());
        return true;
    }

private:
    Player* _player;
};

struct quest_dh_the_hunt : QuestScript
{
    quest_dh_the_hunt() : QuestScript("quest_dh_the_hunt") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus == QUEST_STATUS_INCOMPLETE) // just accepted -> fly to Felsoul Hold
            player->m_Events.AddEventAtOffset(new HavocHuntFlightEvent(player), 1500ms);
    }
};

// Varedis Felsoul (94836) - Havoc final boss. Already faction 16 in the template; we re-assert it on Reset for safety.
// His death grants the Twinblades wield-credit (obj 1), completes the quest, advances the on-screen scenario's final
// step, and rides the player home to the Fel Hammer a few seconds later. Bound to 94836 via creature_template.
struct npc_varedis_felsoul : public ScriptedAI
{
    npc_varedis_felsoul(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        if (me->GetMap()->GetId() == MAP_FELSOUL_HOLD)
            me->SetFaction(FACTION_MONSTER_2); // faction 16 - hostile Havoc boss
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        if (me->GetMap()->GetId() == MAP_FELSOUL_HOLD)
        {
            me->Yell("So, Illidari, you seek my head, that you might take the Twinblades as your own? Allow me to introduce you to their power... personally.", LANG_UNIVERSAL);
            ArtifactPlayScene(me->GetMap(), 1054); // DH Havoc - Varedis reveal scene
        }
    }

    void JustDied(Unit* /*killer*/) override
    {
        if (me->GetMap()->GetId() != MAP_FELSOUL_HOLD)
            return;

        me->Yell("I have no need for mortal armaments! This is not over... Illidari.", LANG_UNIVERSAL);
        ArtifactAdvanceScenario(me); // on-screen scenario final beat (no-op if no scenario running)
        ArtifactPlayScene(me->GetMap(), 1063); // DH Havoc - Twinblades looted

        for (auto const& ref : me->GetMap()->GetPlayers())
            if (Player* p = ref.GetSource())
                if (p->IsInWorld())
                {
                    p->KilledMonsterCredit(CREDIT_TWINBLADES);                              // obj 1
                    if (p->GetQuestStatus(QUEST_THE_HUNT) == QUEST_STATUS_INCOMPLETE)
                        p->CompleteQuest(QUEST_THE_HUNT);                                    // safety net
                    p->m_Events.AddEventAtOffset(new HavocReturnEvent(p), 6s);              // ride home to hand in
                }
    }
};

// =====================================================================================================================
// Vengeance - "Vengeance Will Be Ours" (41863; phase duplicate 40249).
//
// On accept, fly to the Broken Shore: grant the flight credit (obj 0) and drop the player at the scenario landing
// beside Allari the Souleater (98882), the ally/director.
class VengeanceFlightEvent : public BasicEvent
{
public:
    explicit VengeanceFlightEvent(Player* player) : _player(player) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (_player->IsInWorld())
        {
            _player->KilledMonsterCredit(CREDIT_FLY_BROKEN_SHORE);                                        // obj 0
            _player->TeleportTo(MAP_BROKEN_SHORE_SCENARIO, BrokenShoreLanding.GetPositionX(), BrokenShoreLanding.GetPositionY(),
                BrokenShoreLanding.GetPositionZ(), BrokenShoreLanding.GetOrientation());
        }
        return true;
    }

private:
    Player* _player;
};

// After Caria falls and the warblades are claimed, ride the fel bat back to Dalaran: grant the return credit (obj 2),
// complete the quest, and drop the player at the Fel Hammer beside Kor'vas Bloodthorn (102799), the ender.
class VengeanceReturnEvent : public BasicEvent
{
public:
    explicit VengeanceReturnEvent(Player* player) : _player(player) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (_player->IsInWorld() && _player->GetMapId() == MAP_BROKEN_SHORE_SCENARIO)
        {
            _player->KilledMonsterCredit(CREDIT_RETURN_DALARAN);                            // obj 2
            if (_player->GetQuestStatus(QUEST_VENGEANCE_WILL_BE_OURS) == QUEST_STATUS_INCOMPLETE)
                _player->CompleteQuest(QUEST_VENGEANCE_WILL_BE_OURS);                        // safety net
            _player->TeleportTo(MAP_FEL_HAMMER, FelHammerReturn.GetPositionX(), FelHammerReturn.GetPositionY(),
                FelHammerReturn.GetPositionZ(), FelHammerReturn.GetOrientation());
        }
        return true;
    }

private:
    Player* _player;
};

struct quest_dh_vengeance_will_be_ours : QuestScript
{
    quest_dh_vengeance_will_be_ours() : QuestScript("quest_dh_vengeance_will_be_ours") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus == QUEST_STATUS_INCOMPLETE) // just accepted -> fly to the Broken Shore
            player->m_Events.AddEventAtOffset(new VengeanceFlightEvent(player), 1500ms);
    }
};

// Caria Felsoul (99184) - Vengeance final boss. Already faction 16 in the template; re-asserted on Reset for safety.
// Her death grants the Aldrachi Warblades credit (obj 1), advances the on-screen scenario's final step, and schedules
// the ride home (which grants obj 2 + completes the quest). Bound to 99184 via creature_template.
struct npc_caria_felsoul : public ScriptedAI
{
    npc_caria_felsoul(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        if (me->GetMap()->GetId() == MAP_BROKEN_SHORE_SCENARIO)
            me->SetFaction(FACTION_MONSTER_2); // faction 16 - hostile Vengeance boss
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        if (me->GetMap()->GetId() == MAP_BROKEN_SHORE_SCENARIO)
            me->Yell("WITNESS THE MIGHT OF THE ALDRACHI!", LANG_UNIVERSAL);
    }

    void JustDied(Unit* /*killer*/) override
    {
        if (me->GetMap()->GetId() != MAP_BROKEN_SHORE_SCENARIO)
            return;

        me->Yell("I will... be... reborn...", LANG_UNIVERSAL);
        ArtifactAdvanceScenario(me); // on-screen scenario final beat (no-op if no scenario running)
        ArtifactPlayScene(me->GetMap(), 1245); // DH Vengeance - Warblades looted

        for (auto const& ref : me->GetMap()->GetPlayers())
            if (Player* p = ref.GetSource())
                if (p->IsInWorld())
                {
                    p->KilledMonsterCredit(CREDIT_ALDRACHI_WARBLADES);                      // obj 1
                    p->m_Events.AddEventAtOffset(new VengeanceReturnEvent(p), 6s);          // ride home -> obj 2 + complete
                }
    }
};

void AddSC_artifact_demonhunter()
{
    // Quest
    new quest_dh_the_hunt();
    new quest_dh_vengeance_will_be_ours();

    // Creature
    RegisterCreatureAI(npc_dh_artifact_scenario_director);
    RegisterCreatureAI(npc_varedis_felsoul);
    RegisterCreatureAI(npc_caria_felsoul);
}
