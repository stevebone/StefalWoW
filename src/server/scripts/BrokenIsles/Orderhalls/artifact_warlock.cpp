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

// Warlock Legion artifact acquisition. Three specs, all acquisitionType "mixed": an open-world lead-in plus an
// authored InstanceScenario in which the weapon is looted. The Legion artifact scenarios ship as empty placeholder
// content in our world DB, so (as with the Beast Mastery Hunter Titanstrike scenario, zone_orderhall_hunter.cpp) we
// wire the scenario-step progression and the quest completion here, driving them off the player's own progress and
// the encounter bosses' deaths rather than the (non-firing) DB2 game-event criteria.
//
//   Affliction  - Ulthalesh, the Deadwind Harvester : open-world 40495 -> Karazhan Catacombs SCENARIO 988 (map 1533,
//                 quest 40623 "The Dark Riders"). Defeat The Conservator (101257) then Ariden (102532), loot Ulthalesh.
//   Demonology  - Skull of the Man'ari             : reagent gather 42128 -> Felsoul Hold SCENARIO 1097 (map 1498,
//                 quests 42168/42125). Defeat Felborn Overfiend (106644), pursue Mephistroth (106811), take the Skull.
//   Destruction - Scepter of Sargeras              : open-world 43100 -> Tol Barad SCENARIO 1155 (map 1630, quest
//                 43153 "An Eye for a Scepter") -> finale 43254. Slay the void terror Eye of the Beast (106757).
//
// The instanced scenario maps are populated in our DB except for the scenario directors (an ally/quest-ender NPC) and
// two encounter bosses, which the accompanying SQL spawns. The scenario UI is advanced cosmetically via
// ArtifactAdvanceScenario; the quests complete authoritatively from the boss deaths / granted kill-credits, so a
// scenario hiccup can never strand the questline.

#include "orderhall_artifact_common.h"
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
#include <vector>

// ---------------------------------------------------------------------------------------------------------------------
// Shared data
// ---------------------------------------------------------------------------------------------------------------------
enum WarlockArtifactData
{
    // Maps
    MAP_DREADSCAR_RIFT          = 1107,
    MAP_KARAZHAN_CATACOMBS      = 1533, // Affliction scenario 988
    MAP_FELSOUL_HOLD            = 1498, // Demonology scenario 1097
    MAP_TOL_BARAD_SCENARIO      = 1630, // Destruction scenario 1155

    // Affliction quests / credits
    QUEST_ULTHALESH             = 40495, // open-world lead-in (ender Revil Kost 100323, map 0)
    QUEST_DARK_RIDERS           = 40623, // Karazhan Catacombs scenario quest (ender Revil Kost 100812)
    CREDIT_KARAZHAN_PORTAL      = 102580,
    CREDIT_MISTMANTLE_SEARCHED  = 100821,
    CREDIT_REVIL_CONVINCED      = 100368,
    CREDIT_SEWER_SCENARIO       = 100813, // "scenario 988 complete"
    CREDIT_ULTHALESH_LOOTED     = 103473,
    NPC_ARIDEN                  = 102532, // scenario 988 final boss (hostile)
    NPC_CONSERVATOR             = 101257, // scenario 988 vault guardian

    // Demonology quests / credits
    QUEST_RITUAL_REAGENTS       = 42128, // reagent gather (ender Calydus 106610)
    QUEST_LOOKING_DARKNESS      = 42168, // Felsoul Hold scenario quest (giver/ender Calydus 106610)
    QUEST_DARK_WHISPERS         = 42125, // skull obtained (ender Calydus 101097, Dreadscar)
    CREDIT_RITUAL_BUNNY         = 106462,
    CREDIT_SKULL_LOCATION       = 106746,
    CREDIT_NISKARA_BUNNY        = 106033,
    CREDIT_KILL_CREDIT          = 106973,
    NPC_FELBORN_OVERFIEND       = 106644, // scenario 1097 step-1 boss
    NPC_MEPHISTROTH             = 106811, // scenario 1097 fleeing antagonist

    // Destruction quests / credits
    QUEST_FINDING_SCEPTER       = 43100, // open-world Dalaran Crater / Caer Darrow (ender Calydus 109698)
    QUEST_EYE_FOR_A_SCEPTER     = 43153, // Tol Barad scenario quest (ender Calydus 109838)
    QUEST_RITUAL_RUINATION      = 43254, // finale (ender Calydus 101097, Dreadscar)
    CREDIT_DALARAN_CRATER       = 102394,
    CREDIT_CAER_DARROW          = 109659,
    CREDIT_INFO_COUNCIL         = 109693,
    CREDIT_SPOKE_CALYDUS        = 109696,
    CREDIT_TOL_BARAD_PORTAL     = 109835,
    CREDIT_WON_SCENARIO         = 109836, // "scenario 1155 complete"
    CREDIT_GO_TO_CALYDUS        = 109837,
    CREDIT_FEL_BAT_RIDE         = 110523,
    CREDIT_GULDAN_LISTENED      = 110581,
    CREDIT_KILLED_ALLARIS       = 114337,
    CREDIT_ARTIFACT_LOOTED      = 110603,
    CREDIT_RITUAL_RUINED        = 110606,
    CREDIT_ESCAPE_DALARAN       = 110607,
    NPC_EYE_OF_THE_BEAST        = 106757  // scenario 1155 finale boss (void terror)
};

// Landing points inside each scenario map (from live spawns of the encounter bosses), and the Dreadscar return.
static constexpr Position AfflicLanding   = { -10865.3f, -1961.7f, -41.0f, 3.29f };
static constexpr Position DemoLanding      = {    999.0f,  4920.0f,  36.0f, 2.20f };
static constexpr Position DestroLanding    = {  -1038.6f,  1151.6f,  99.6f, 3.87f };
static constexpr Position DreadscarReturn = {   3121.3f,  1106.3f, 286.6f, 4.65f };

// ---------------------------------------------------------------------------------------------------------------------
// One scripted deferred action, used by every quest-accept hook and by the directors' return-home schedule:
// grant a set of kill-credits, optionally force the quest to COMPLETE (so it can be handed in), and optionally
// teleport the player. A map of -1 means "no teleport".
// ---------------------------------------------------------------------------------------------------------------------
class WarlockActionEvent : public BasicEvent
{
public:
    WarlockActionEvent(Player* player, std::vector<uint32> credits, uint32 completeQuest, int32 map, Position pos)
        : _player(player), _credits(std::move(credits)), _completeQuest(completeQuest), _map(map), _pos(pos) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (!_player->IsInWorld())
            return true;

        for (uint32 credit : _credits)
            _player->KilledMonsterCredit(credit);

        if (_completeQuest && _player->GetQuestStatus(_completeQuest) == QUEST_STATUS_INCOMPLETE)
            _player->CompleteQuest(_completeQuest);

        if (_map >= 0)
            _player->TeleportTo(uint32(_map), _pos.GetPositionX(), _pos.GetPositionY(), _pos.GetPositionZ(), _pos.GetOrientation());

        return true;
    }

private:
    Player* _player;
    std::vector<uint32> _credits;
    uint32 _completeQuest;
    int32 _map;
    Position _pos;
};

// Convenience: schedule a WarlockActionEvent on a single player.
static void ScheduleAction(Player* player, std::vector<uint32> credits, uint32 completeQuest, int32 map, Position pos, Milliseconds delay)
{
    player->m_Events.AddEventAtOffset(new WarlockActionEvent(player, std::move(credits), completeQuest, map, pos), delay);
}

// ---------------------------------------------------------------------------------------------------------------------
// Generic hostile-boss AI: the encounter bosses spawned by our SQL (and the faction-35 placeholders) are made
// attackable here rather than by editing creature_template.faction. Bound to Ariden placeholder 102200, Felborn
// Overfiend 106644 and Eye of the Beast 106757.
// ---------------------------------------------------------------------------------------------------------------------
struct npc_warlock_artifact_hostile_boss : public ScriptedAI
{
    npc_warlock_artifact_hostile_boss(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override { me->SetFaction(FACTION_MONSTER_2); } // faction 16 - attackable
};

// ---------------------------------------------------------------------------------------------------------------------
// Affliction: Karazhan Catacombs scenario 988 (map 1533). Director bound to Revil Kost (100323); he is also spawned on
// map 0 as the 40495 ender, so all logic is guarded on MAP_KARAZHAN_CATACOMBS. He advances scenario 988 cosmetically
// and, authoritatively, completes "The Dark Riders" (40623) when Ariden (102532) falls: grant the scenario-complete
// and Ulthalesh-looted credits, then return the party to the Dreadscar for the auto-reward (40689) hand-off.
// ---------------------------------------------------------------------------------------------------------------------
struct npc_warlock_afflic_director : public ScriptedAI
{
    npc_warlock_afflic_director(Creature* creature) : ScriptedAI(creature) { }

    uint32 _pollTimer = 0;
    uint32 _advTimer = 0;
    bool _conservatorSeen = false;
    bool _conservatorDead = false;
    bool _aridenSeen = false;
    bool _aridenDead = false;
    bool _completed = false;
    bool _spokeStart = false;

    void Reset() override
    {
        if (me->GetMap()->GetId() == MAP_KARAZHAN_CATACOMBS)
            me->setActive(true);
    }

    void UpdateAI(uint32 diff) override
    {
        if (me->GetMap()->GetId() != MAP_KARAZHAN_CATACOMBS)
            return;

        _pollTimer += diff;
        if (_pollTimer < 1000)
            return;
        _pollTimer = 0;

        if (!ArtifactPlayerBeyond(me, AfflicLanding, 0.0f)) // nobody here yet
            return;

        if (!_spokeStart)
        {
            _spokeStart = true;
            me->Say("The catacombs run deep. Stay close - the Dark Riders guard the Harvester with the dead.", LANG_UNIVERSAL);
        }

        // Track the guardian and the boss so we know when each has fallen.
        if (me->FindNearestCreature(NPC_CONSERVATOR, 500.0f, true))
            _conservatorSeen = true;
        else if (_conservatorSeen && !_conservatorDead)
        {
            _conservatorDead = true;
            me->Say("The Conservator is undone. The vault is ours to search.", LANG_UNIVERSAL);
        }

        if (me->FindNearestCreature(NPC_ARIDEN, 500.0f, true))
            _aridenSeen = true;
        else if (_aridenSeen && !_aridenDead)
            _aridenDead = true;

        // Cosmetic scenario advance, paced so it does not race to completion before Ariden is dealt with.
        if (!_completed)
        {
            _advTimer += 1000;
            if (_advTimer >= 7000)
            {
                _advTimer = 0;
                if (!_aridenDead && ArtifactScenarioOrder(me) < 5)
                    ArtifactAdvanceScenario(me);
            }
        }

        // Authoritative completion: Ariden defeated -> Ulthalesh claimed.
        if (_aridenDead && !_completed)
        {
            _completed = true;
            me->Say("Ulthalesh feasts once more. Take it, and let us be gone from this place.", LANG_UNIVERSAL);

            for (int i = 0; i < 8; ++i) // finish the on-screen scenario steps
                ArtifactAdvanceScenario(me);

            ArtifactPlayScene(me->GetMap(), 1573); // Warlock Affliction - Ulthalesh loot scene

            for (auto const& ref : me->GetMap()->GetPlayers())
                if (Player* p = ref.GetSource())
                    if (p->IsInWorld())
                    {
                        p->KilledMonsterCredit(CREDIT_SEWER_SCENARIO);   // 40623 obj 0
                        p->KilledMonsterCredit(CREDIT_ULTHALESH_LOOTED); // 40623 obj 1
                        if (p->GetQuestStatus(QUEST_DARK_RIDERS) == QUEST_STATUS_INCOMPLETE)
                            p->CompleteQuest(QUEST_DARK_RIDERS);
                        ScheduleAction(p, {}, 0, MAP_DREADSCAR_RIFT, DreadscarReturn, 12s);
                    }

            TC_LOG_INFO("scripts", "[Warlock/Ulthalesh 988] Ariden down - 40623 completed, returning party to Dreadscar");
        }
    }
};

struct quest_warlock_ulthalesh : QuestScript
{
    quest_warlock_ulthalesh() : QuestScript("quest_warlock_ulthalesh") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus == QUEST_STATUS_INCOMPLETE) // open-world lead-in: fire the portal / search / gossip credits
            ScheduleAction(player, { CREDIT_KARAZHAN_PORTAL, CREDIT_MISTMANTLE_SEARCHED, CREDIT_REVIL_CONVINCED }, 0, -1, {}, 1500ms);
    }
};

struct quest_warlock_dark_riders : QuestScript
{
    quest_warlock_dark_riders() : QuestScript("quest_warlock_dark_riders") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus == QUEST_STATUS_INCOMPLETE) // enter the Karazhan Catacombs scenario; the director drives it
            ScheduleAction(player, {}, 0, MAP_KARAZHAN_CATACOMBS, AfflicLanding, 1500ms);
    }
};

// ---------------------------------------------------------------------------------------------------------------------
// Demonology: Felsoul Hold scenario 1097 (map 1498). Director bound to Calydus (106610, spawned by our SQL as the
// Felsoul quest-giver/ender). The quests complete from their own accept hooks (the Skull is taken via granted
// credits), so this director is the on-screen scenario presenter: it advances scenario 1097 cosmetically and voices
// the beats as the Felborn Overfiend (106644) falls and Mephistroth (106811) flees.
// ---------------------------------------------------------------------------------------------------------------------
struct npc_warlock_demo_director : public ScriptedAI
{
    npc_warlock_demo_director(Creature* creature) : ScriptedAI(creature) { }

    uint32 _pollTimer = 0;
    uint32 _advTimer = 0;
    bool _spokeStart = false;
    bool _overfiendSeen = false;
    bool _overfiendDead = false;
    bool _done = false;

    void Reset() override
    {
        if (me->GetMap()->GetId() == MAP_FELSOUL_HOLD)
            me->setActive(true);
    }

    void UpdateAI(uint32 diff) override
    {
        if (me->GetMap()->GetId() != MAP_FELSOUL_HOLD)
            return;

        _pollTimer += diff;
        if (_pollTimer < 1000)
            return;
        _pollTimer = 0;

        if (!ArtifactPlayerBeyond(me, DemoLanding, 0.0f))
            return;

        if (!_spokeStart)
        {
            _spokeStart = true;
            me->Say("Thal'kiel's skull lies below, in Mephistroth's grasp. We must take it before he shatters it.", LANG_UNIVERSAL);
        }

        if (me->FindNearestCreature(NPC_FELBORN_OVERFIEND, 500.0f, true))
            _overfiendSeen = true;
        else if (_overfiendSeen && !_overfiendDead)
        {
            _overfiendDead = true;
            me->Say("The Overfiend is slain! Mephistroth flees deeper - after him!", LANG_UNIVERSAL);
        }

        if (!_done)
        {
            _advTimer += 1000;
            if (_advTimer >= 7000)
            {
                _advTimer = 0;
                if (ArtifactScenarioOrder(me) != 0 || _overfiendDead) // let step 0 hold until the fight begins
                    ArtifactAdvanceScenario(me);
                if (!me->GetScenario() || !me->GetScenario()->GetStep())
                    _done = true;
            }
        }
    }
};

struct quest_warlock_ritual_reagents : QuestScript
{
    quest_warlock_ritual_reagents() : QuestScript("quest_warlock_ritual_reagents") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus == QUEST_STATUS_INCOMPLETE) // gather the reagents (stand-in), then meet Calydus at Felsoul Hold
            ScheduleAction(player, {}, QUEST_RITUAL_REAGENTS, MAP_FELSOUL_HOLD, DemoLanding, 1500ms);
    }
};

struct quest_warlock_looking_darkness : QuestScript
{
    quest_warlock_looking_darkness() : QuestScript("quest_warlock_looking_darkness") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus == QUEST_STATUS_INCOMPLETE) // the Felsoul Hold search scenario; credits fire the two objectives
            ScheduleAction(player, { CREDIT_RITUAL_BUNNY, CREDIT_SKULL_LOCATION }, QUEST_LOOKING_DARKNESS, -1, {}, 1500ms);
    }
};

struct quest_warlock_dark_whispers : QuestScript
{
    quest_warlock_dark_whispers() : QuestScript("quest_warlock_dark_whispers") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus == QUEST_STATUS_INCOMPLETE) // take the Skull, then return to Calydus at the Dreadscar for turn-in
        {
            ArtifactPlayScene(player->GetMap(), 1617); // Warlock Demonology - Skull of the Man'ari loot scene
            ScheduleAction(player, { CREDIT_NISKARA_BUNNY, CREDIT_KILL_CREDIT }, QUEST_DARK_WHISPERS, MAP_DREADSCAR_RIFT, DreadscarReturn, 1500ms);
        }
    }
};

// ---------------------------------------------------------------------------------------------------------------------
// Destruction: Tol Barad scenario 1155 (map 1630). Director bound to Calydus (109838, spawned by our SQL as the
// scenario quest-ender). Advances scenario 1155 cosmetically and completes "An Eye for a Scepter" (43153) when the
// void terror Eye of the Beast (106757) is slain: grant the "won scenario" + "go to Calydus" credits, then return the
// party to the Dreadscar for 43254 and the auto-reward (40690).
// ---------------------------------------------------------------------------------------------------------------------
struct npc_warlock_destro_director : public ScriptedAI
{
    npc_warlock_destro_director(Creature* creature) : ScriptedAI(creature) { }

    uint32 _pollTimer = 0;
    uint32 _advTimer = 0;
    bool _spokeStart = false;
    bool _eyeSeen = false;
    bool _eyeDead = false;
    bool _completed = false;

    void Reset() override
    {
        if (me->GetMap()->GetId() == MAP_TOL_BARAD_SCENARIO)
            me->setActive(true);
    }

    void UpdateAI(uint32 diff) override
    {
        if (me->GetMap()->GetId() != MAP_TOL_BARAD_SCENARIO)
            return;

        _pollTimer += diff;
        if (_pollTimer < 1000)
            return;
        _pollTimer = 0;

        if (!ArtifactPlayerBeyond(me, DestroLanding, 0.0f))
            return;

        if (!_spokeStart)
        {
            _spokeStart = true;
            me->Say("The Eye of Sargeras is held in the deepest cell. Free the prisoners, and beware the beast that guards it.", LANG_UNIVERSAL);
        }

        if (me->FindNearestCreature(NPC_EYE_OF_THE_BEAST, 600.0f, true))
            _eyeSeen = true;
        else if (_eyeSeen && !_eyeDead)
            _eyeDead = true;

        if (!_completed)
        {
            _advTimer += 1000;
            if (_advTimer >= 7000)
            {
                _advTimer = 0;
                if (!_eyeDead && ArtifactScenarioOrder(me) < 5)
                    ArtifactAdvanceScenario(me);
            }
        }

        if (_eyeDead && !_completed)
        {
            _completed = true;
            me->Say("The beast is dead and the Eye is ours. Back to the Dreadscar - the Scepter is nearly whole.", LANG_UNIVERSAL);

            for (int i = 0; i < 10; ++i)
                ArtifactAdvanceScenario(me);

            for (auto const& ref : me->GetMap()->GetPlayers())
                if (Player* p = ref.GetSource())
                    if (p->IsInWorld())
                    {
                        p->KilledMonsterCredit(CREDIT_WON_SCENARIO); // 43153 obj 1
                        p->KilledMonsterCredit(CREDIT_GO_TO_CALYDUS); // 43153 obj 2
                        if (p->GetQuestStatus(QUEST_EYE_FOR_A_SCEPTER) == QUEST_STATUS_INCOMPLETE)
                            p->CompleteQuest(QUEST_EYE_FOR_A_SCEPTER);
                        ScheduleAction(p, {}, 0, MAP_DREADSCAR_RIFT, DreadscarReturn, 12s);
                    }

            TC_LOG_INFO("scripts", "[Warlock/Scepter 1155] Eye of the Beast down - 43153 completed, returning party to Dreadscar");
        }
    }
};

struct quest_warlock_finding_scepter : QuestScript
{
    quest_warlock_finding_scepter() : QuestScript("quest_warlock_finding_scepter") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus == QUEST_STATUS_INCOMPLETE) // open-world Dalaran Crater / Caer Darrow: fire the trail credits
            ScheduleAction(player, { CREDIT_DALARAN_CRATER, CREDIT_CAER_DARROW, CREDIT_INFO_COUNCIL, CREDIT_SPOKE_CALYDUS },
                QUEST_FINDING_SCEPTER, -1, {}, 1500ms);
    }
};

struct quest_warlock_eye_for_a_scepter : QuestScript
{
    quest_warlock_eye_for_a_scepter() : QuestScript("quest_warlock_eye_for_a_scepter") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus == QUEST_STATUS_INCOMPLETE) // took the Tol Barad portal (obj 0), then enter scenario 1155
            ScheduleAction(player, { CREDIT_TOL_BARAD_PORTAL }, 0, MAP_TOL_BARAD_SCENARIO, DestroLanding, 1500ms);
    }
};

struct quest_warlock_ritual_ruination : QuestScript
{
    quest_warlock_ritual_ruination() : QuestScript("quest_warlock_ritual_ruination") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus == QUEST_STATUS_INCOMPLETE) // scripted finale: fel-bat ride, Gul'dan, kill Allaris, loot, escape
        {
            ArtifactPlayScene(player->GetMap(), 1718); // Warlock Destruction - Broken Shore ritual/betrayal set-piece
            ScheduleAction(player, { CREDIT_FEL_BAT_RIDE, CREDIT_GULDAN_LISTENED, CREDIT_KILLED_ALLARIS,
                CREDIT_ARTIFACT_LOOTED, CREDIT_RITUAL_RUINED, CREDIT_ESCAPE_DALARAN }, QUEST_RITUAL_RUINATION, -1, {}, 1500ms);
            ArtifactPlayScene(player->GetMap(), 1681); // Warlock Destruction - Scepter of Sargeras loot scene
        }
    }
};

// ---------------------------------------------------------------------------------------------------------------------
void AddSC_artifact_warlock()
{
    // Quests
    new quest_warlock_ulthalesh();
    new quest_warlock_dark_riders();
    new quest_warlock_ritual_reagents();
    new quest_warlock_looking_darkness();
    new quest_warlock_dark_whispers();
    new quest_warlock_finding_scepter();
    new quest_warlock_eye_for_a_scepter();
    new quest_warlock_ritual_ruination();

    // Creatures
    RegisterCreatureAI(npc_warlock_artifact_hostile_boss);
    RegisterCreatureAI(npc_warlock_afflic_director);
    RegisterCreatureAI(npc_warlock_demo_director);
    RegisterCreatureAI(npc_warlock_destro_director);
}