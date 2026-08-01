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

// Rogue artifact acquisitions. Three specs, each a short instanced scenario reached from the Hall of Shadows (map 1220):
//
//   Assassination - The Kingslayers      : quest 42504/42627 "The Unseen Blade"   -> scenario map 1620 (Stormwind)
//   Outlaw        - The Dreadblades       : quest 40849 "The Dreadblades"          -> scenario map 1545 (Crimson Veil)
//   Subtlety      - Fangs of the Devourer : quest 41924 "Fangs of the Devourer"    -> scenario map 1607 (the Citadel)
//
// The Legion artifact InstanceScenarios ship as empty placeholder content in our world DB, so - exactly as in the proven
// Beast Mastery Hunter file (zone_orderhall_hunter.cpp) - the questline is driven authoritatively by kill-credits +
// CompleteQuest, and the on-screen scenario steps are nudged forward best-effort via ArtifactAdvanceScenario (the
// game-event step criteria never fire here). The acquisition quest is therefore always completable, scenario or not.
//
// Data verified against integ_world (2026-08): quest_objectives, creature_questender and the current spawn state of maps
// 1620/1545/1607. The Kingslayers actors are entirely absent from map 1620 (that map otherwise holds a different class's
// scourge scenario), so this file's SQL spawns the two Kingslayers bosses; the Dreadblades and Devourer actors are
// already spawned and only need their faction-35 placeholders made hostile + the completion wiring.

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

namespace
{
// Hall of Shadows (rogue class hall) return point - beside Valeera/Ravenholdt on map 1220.
constexpr uint32 MAP_HALL_OF_SHADOWS = 1220;
constexpr Position HallOfShadowsReturn = { -951.6f, 4550.2f, 698.1f, 1.6f };

// Shared: a return event that (optionally) grants a return kill-credit then ports the player back to the hall.
class ReturnToHallEvent : public BasicEvent
{
public:
    ReturnToHallEvent(Player* player, uint32 returnCredit) : _player(player), _returnCredit(returnCredit) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (_player->IsInWorld())
        {
            if (_returnCredit)
                _player->KilledMonsterCredit(_returnCredit);
            _player->TeleportTo(MAP_HALL_OF_SHADOWS, HallOfShadowsReturn.GetPositionX(), HallOfShadowsReturn.GetPositionY(),
                HallOfShadowsReturn.GetPositionZ(), HallOfShadowsReturn.GetOrientation());
        }
        return true;
    }

private:
    Player* _player;
    uint32 _returnCredit;
};
}

// =====================================================================================================================
// Assassination - "The Unseen Blade" (42504 Alliance / 42627 Horde) -> The Kingslayers, scenario 1123 on map 1620.
//
// Objectives (both variants): obj0 Type0 credit 114508 "Obtain the Kingslayers" (retail: loot GO 251107 at the end of
// the escort), obj1 Type14 CriteriaTree "Return to the Hall of Shadows". Nothing Kingslayers-specific is spawned on map
// 1620, so the SQL freshly spawns final boss Melris Malagan (107831) and mini-boss Sister Althea Ebonlocke (108218);
// both are faction-35 placeholders made hostile here. Killing Melris grants credit 114508 and completes the quest.
// =====================================================================================================================
namespace
{
constexpr uint32 QUEST_UNSEEN_BLADE_A   = 42504;
constexpr uint32 QUEST_UNSEEN_BLADE_H   = 42627;
constexpr uint32 MAP_KINGSLAYERS        = 1620;
constexpr uint32 NPC_KINGSLAYERS_CREDIT = 114508; // obj0 "Obtain the Kingslayers"
constexpr uint8  KINGSLAYERS_STEP_STOP  = 9;       // scenario 1123 has 11 steps (0..10); the final beat is Melris' death
constexpr Position KingslayersLanding   = { 1420.0f, -1344.0f, 60.0f, 3.9f };

// On quest accept, port the rogue into the Stormwind scenario to begin the hunt for the Kingslayers.
class KingslayersEntryEvent : public BasicEvent
{
public:
    explicit KingslayersEntryEvent(Player* player) : _player(player) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (_player->IsInWorld())
            _player->TeleportTo(MAP_KINGSLAYERS, KingslayersLanding.GetPositionX(), KingslayersLanding.GetPositionY(),
                KingslayersLanding.GetPositionZ(), KingslayersLanding.GetOrientation());
        return true;
    }

private:
    Player* _player;
};
}

// Bound to both 42504 and 42627 (they share this ScriptName): on accept, fly the player into scenario map 1620.
struct quest_kingslayers : QuestScript
{
    quest_kingslayers() : QuestScript("quest_kingslayers") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus == QUEST_STATUS_INCOMPLETE)
            player->m_Events.AddEventAtOffset(new KingslayersEntryEvent(player), 1500ms);
    }
};

// Sister Althea Ebonlocke (108218): the mini-boss encountered first, doubling as the scenario director. Faction-35
// placeholder made hostile; while active she nudges the on-screen scenario forward (stopping short of the final step,
// which Melris' death completes).
struct npc_kingslayers_director : public ScriptedAI
{
    npc_kingslayers_director(Creature* creature) : ScriptedAI(creature), _pollTimer(0) { }

    uint32 _pollTimer;

    void Reset() override
    {
        me->SetFaction(FACTION_MONSTER_2); // 16 - attackable mini-boss
        if (me->GetMap()->GetId() == MAP_KINGSLAYERS)
            me->setActive(true);
    }

    void UpdateAI(uint32 diff) override
    {
        if (me->GetMap()->GetId() != MAP_KINGSLAYERS)
            return;

        _pollTimer += diff;
        if (_pollTimer < 8000)
            return;
        _pollTimer = 0;

        if (ArtifactScenarioOrder(me) < KINGSLAYERS_STEP_STOP)
            ArtifactAdvanceScenario(me);
    }
};

// Melris Malagan (107831): the Kingslayers final boss. Faction-35 placeholder made hostile; his death grants the
// "Obtain the Kingslayers" credit (obj0), force-completes whichever variant is in progress (obj1 is a return criteria
// tree that does not fire here), and ports the rogue back to the Hall of Shadows.
struct npc_kingslayers_melris : public ScriptedAI
{
    npc_kingslayers_melris(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override { me->SetFaction(FACTION_MONSTER_2); } // 16 - hostile final boss

    void JustDied(Unit* /*killer*/) override
    {
        for (auto const& ref : me->GetMap()->GetPlayers())
            if (Player* p = ref.GetSource())
                if (p->IsInWorld())
                {
                    p->KilledMonsterCredit(NPC_KINGSLAYERS_CREDIT);                 // obj0
                    if (p->GetQuestStatus(QUEST_UNSEEN_BLADE_A) == QUEST_STATUS_INCOMPLETE)
                        p->CompleteQuest(QUEST_UNSEEN_BLADE_A);                      // obj1 return criteria tree
                    if (p->GetQuestStatus(QUEST_UNSEEN_BLADE_H) == QUEST_STATUS_INCOMPLETE)
                        p->CompleteQuest(QUEST_UNSEEN_BLADE_H);
                    p->m_Events.AddEventAtOffset(new ReturnToHallEvent(p, 0), 5s);
                }

        ArtifactAdvanceScenario(me); // best-effort finale on the on-screen scenario
    }
};

// =====================================================================================================================
// Outlaw - "The Dreadblades" (40849) -> scenario 1012 on map 1545. Actors already spawned.
//
// Objectives (all Type0): obj0 credit 102120 (gossip Fleet Admiral Tethys to start), obj1 credit 114513 (retail: loot
// GO 254087 after defeating Dread Admiral Eliza), obj2 credit 97377 "Portal to Dalaran Taken" (the return flight).
// The enemy trash/bosses are faction-35 placeholders (except Eliza, already faction 14): we make DeGauza (102185) and
// Brinebeard (102239) hostile; Eliza's (102293) death grants obj1 + completes the quest, then the return grants obj2.
// =====================================================================================================================
namespace
{
constexpr uint32 QUEST_DREADBLADES        = 40849;
constexpr uint32 MAP_DREADBLADES          = 1545;
constexpr uint32 NPC_DREADBLADES_START    = 102120; // obj0 "gossip Tethys"
constexpr uint32 NPC_DREADBLADES_LOOT     = 114513; // obj1 loot credit
constexpr uint32 NPC_DREADBLADES_RETURN   = 97377;  // obj2 "Portal to Dalaran Taken"
constexpr uint8  DREADBLADES_STEP_STOP    = 3;       // scenario 1012 has 5 steps (0..4); Eliza's death is the last beat
constexpr Position DreadbladesLanding     = { -1390.0f, 5880.0f, 1.0f, 6.27f }; // beside Tethys-actor 102179

class DreadbladesEntryEvent : public BasicEvent
{
public:
    explicit DreadbladesEntryEvent(Player* player) : _player(player) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (_player->IsInWorld())
        {
            _player->KilledMonsterCredit(NPC_DREADBLADES_START);                    // obj0 (commandeer the ship)
            _player->TeleportTo(MAP_DREADBLADES, DreadbladesLanding.GetPositionX(), DreadbladesLanding.GetPositionY(),
                DreadbladesLanding.GetPositionZ(), DreadbladesLanding.GetOrientation());
        }
        return true;
    }

private:
    Player* _player;
};
}

struct quest_dreadblades : QuestScript
{
    quest_dreadblades() : QuestScript("quest_dreadblades") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus == QUEST_STATUS_INCOMPLETE)
            player->m_Events.AddEventAtOffset(new DreadbladesEntryEvent(player), 1500ms);
    }
};

// Fleet Admiral Tethys scenario-actor (102179): a friendly guide, repurposed as the scenario director. Left friendly;
// simply nudges the on-screen scenario forward (stopping short of the final step, completed by Eliza's death).
struct npc_dreadblades_director : public ScriptedAI
{
    npc_dreadblades_director(Creature* creature) : ScriptedAI(creature), _pollTimer(0) { }

    uint32 _pollTimer;

    void Reset() override
    {
        if (me->GetMap()->GetId() == MAP_DREADBLADES)
            me->setActive(true);
    }

    void UpdateAI(uint32 diff) override
    {
        if (me->GetMap()->GetId() != MAP_DREADBLADES)
            return;

        _pollTimer += diff;
        if (_pollTimer < 8000)
            return;
        _pollTimer = 0;

        if (ArtifactScenarioOrder(me) < DREADBLADES_STEP_STOP)
            ArtifactAdvanceScenario(me);
    }
};

// First Mate DeGauza (102185) and Lord Brinebeard (102239): faction-35 placeholder enemies made hostile so they can be
// fought on the way to Eliza. No completion logic - the quest is gated on Eliza's death.
struct npc_dreadblades_enemy : public ScriptedAI
{
    npc_dreadblades_enemy(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override { me->SetFaction(FACTION_MONSTER_2); } // 16 - hostile
};

// Dread Admiral Eliza (102293): the Dreadblades final boss (already faction 14). Her death grants the loot credit
// (obj1), force-completes the quest, and schedules the return flight that grants obj2.
struct npc_dreadblades_eliza : public ScriptedAI
{
    npc_dreadblades_eliza(Creature* creature) : ScriptedAI(creature) { }

    void JustDied(Unit* /*killer*/) override
    {
        for (auto const& ref : me->GetMap()->GetPlayers())
            if (Player* p = ref.GetSource())
                if (p->IsInWorld())
                {
                    p->KilledMonsterCredit(NPC_DREADBLADES_LOOT);                   // obj1
                    if (p->GetQuestStatus(QUEST_DREADBLADES) == QUEST_STATUS_INCOMPLETE)
                        p->CompleteQuest(QUEST_DREADBLADES);
                    p->m_Events.AddEventAtOffset(new ReturnToHallEvent(p, NPC_DREADBLADES_RETURN), 6s); // obj2 on arrival
                }

        ArtifactAdvanceScenario(me);
    }
};

// =====================================================================================================================
// Subtlety - "Fangs of the Devourer" (41924) -> scenario 1078 on map 1607. Actors already spawned.
//
// Objectives (all Type0): obj0 credit 105435 "Portal to Citadel Opened", obj1 credit 116918 "Rune of Portals
// Delivered", obj2 credit 114510 (retail: loot GO 249347 after the final Akaari). Unlike the other specs there is no
// plain teleport-on-accept in retail (entry is gated on an intro capture-fight); we reproduce it as a scripted capture
// teleport into the citadel. Intro Akaari (105536) and Xirus (105542) are faction-35 placeholders made hostile; the
// final Akaari (105660, faction 14) death grants obj2 and completes the quest.
// =====================================================================================================================
namespace
{
constexpr uint32 QUEST_DEVOURER        = 41924;
constexpr uint32 MAP_DEVOURER          = 1607;
constexpr uint32 NPC_DEVOURER_PORTAL   = 105435; // obj0 "Portal to Citadel Opened"
constexpr uint32 NPC_DEVOURER_RUNE     = 116918; // obj1 "Rune of Portals Delivered"
constexpr uint32 NPC_DEVOURER_LOOT     = 114510; // obj2 loot credit
constexpr uint8  DEVOURER_STEP_STOP    = 5;       // scenario 1078 has 7 steps (0..6); final Akaari's death is the last
constexpr Position DevourerCapture     = { 1847.77f, 1259.92f, 57.09f, 1.56f }; // captured/caged inside the citadel

class DevourerEntryEvent : public BasicEvent
{
public:
    explicit DevourerEntryEvent(Player* player) : _player(player) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (_player->IsInWorld())
        {
            _player->KilledMonsterCredit(NPC_DEVOURER_PORTAL);                      // obj0
            _player->KilledMonsterCredit(NPC_DEVOURER_RUNE);                        // obj1
            _player->TeleportTo(MAP_DEVOURER, DevourerCapture.GetPositionX(), DevourerCapture.GetPositionY(),
                DevourerCapture.GetPositionZ(), DevourerCapture.GetOrientation());
        }
        return true;
    }

private:
    Player* _player;
};
}

struct quest_fangs_devourer : QuestScript
{
    quest_fangs_devourer() : QuestScript("quest_fangs_devourer") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus == QUEST_STATUS_INCOMPLETE)
            player->m_Events.AddEventAtOffset(new DevourerEntryEvent(player), 1500ms);
    }
};

// Fangs of the Devourer decorative actor (105843): friendly, repurposed as the scenario director.
struct npc_devourer_director : public ScriptedAI
{
    npc_devourer_director(Creature* creature) : ScriptedAI(creature), _pollTimer(0) { }

    uint32 _pollTimer;

    void Reset() override
    {
        if (me->GetMap()->GetId() == MAP_DEVOURER)
            me->setActive(true);
    }

    void UpdateAI(uint32 diff) override
    {
        if (me->GetMap()->GetId() != MAP_DEVOURER)
            return;

        _pollTimer += diff;
        if (_pollTimer < 8000)
            return;
        _pollTimer = 0;

        if (ArtifactScenarioOrder(me) < DEVOURER_STEP_STOP)
            ArtifactAdvanceScenario(me);
    }
};

// Intro Akaari Shadowgore (105536) and Inquisitor Xirus (105542): faction-35 placeholder enemies made hostile.
struct npc_devourer_enemy : public ScriptedAI
{
    npc_devourer_enemy(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override { me->SetFaction(FACTION_MONSTER_2); } // 16 - hostile
};

// Final Akaari Shadowgore (105660): the Devourer final boss (already faction 14). Her death grants the loot credit
// (obj2), force-completes the quest, and ports the rogue back to the Hall of Shadows.
struct npc_devourer_akaari_final : public ScriptedAI
{
    npc_devourer_akaari_final(Creature* creature) : ScriptedAI(creature) { }

    void JustDied(Unit* /*killer*/) override
    {
        for (auto const& ref : me->GetMap()->GetPlayers())
            if (Player* p = ref.GetSource())
                if (p->IsInWorld())
                {
                    p->KilledMonsterCredit(NPC_DEVOURER_LOOT);                      // obj2
                    if (p->GetQuestStatus(QUEST_DEVOURER) == QUEST_STATUS_INCOMPLETE)
                        p->CompleteQuest(QUEST_DEVOURER);
                    p->m_Events.AddEventAtOffset(new ReturnToHallEvent(p, 0), 5s);
                }

        ArtifactAdvanceScenario(me);
    }
};

void AddSC_artifact_rogue()
{
    // Quest
    new quest_kingslayers();
    new quest_dreadblades();
    new quest_fangs_devourer();

    // Creature
    RegisterCreatureAI(npc_kingslayers_director);
    RegisterCreatureAI(npc_kingslayers_melris);
    RegisterCreatureAI(npc_dreadblades_director);
    RegisterCreatureAI(npc_dreadblades_enemy);
    RegisterCreatureAI(npc_dreadblades_eliza);
    RegisterCreatureAI(npc_devourer_director);
    RegisterCreatureAI(npc_devourer_enemy);
    RegisterCreatureAI(npc_devourer_akaari_final);
}
