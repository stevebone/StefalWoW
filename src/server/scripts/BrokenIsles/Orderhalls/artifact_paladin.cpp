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

// Paladin Legion artifact acquisition - all three specs. Per-class Broken Isles content, mirroring the Beast Mastery
// Hunter Titanstrike scenario (zone_orderhall_hunter.cpp); shared helpers live in orderhall_artifact_common.h.
//
// All three acquisition quests use Type-0 (kill/credit) objectives - NOT DB2 CriteriaTree/CompleteScenario steps - so
// on this server each quest completes purely by KilledMonsterCredit of its credit-dummy entries (plus, for Retribution,
// the actual kill of Balnazzar). The authored InstanceScenarios (1092/1082/775) ship as empty placeholder content, so
// we drive their on-screen step presentation by hand via ArtifactAdvanceScenario() from an ally "director" AI, while
// the objective credits are granted authoritatively from the flight event + the boss deaths.
//
//   Holy        - 42120 "The Silver Hand"          -> scenario 1092, map 1539. Director Travard (106429); boss Horrific
//                 Aberration (106669). Credits: portal 102394 + fly 106416 (accept), won 106357 (boss death),
//                 return 105892 (teleport home).
//   Protection  - 42017 "Shrine of the Truthguard" -> scenario 1082, map 1495. Director Orik (105910); boss Yrgrim the
//                 Truthseeker (105695). Credits: fly 105889 (accept), won 105891 (boss death), return 105892 (home).
//   Retribution - 38376 "The Search for the Highlord" -> scenario 775, map 1500. Director Tirion (92676); demons
//                 Jailer Zerus (91672) + Dark Inquisitor (91697) re-factioned hostile; boss Balnazzar (90981, already
//                 faction 14). Credits: fly 90384 (accept), Balnazzar kill 90981 (auto), won 114505 (Balnazzar death).
//
// The faction-35 placeholder bosses are made hostile (FACTION_MONSTER_2 = 16) in the C++ Reset(); Balnazzar is left on
// its authored hostile faction 14.

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
#include "orderhall_artifact_common.h"
#include <utility>
#include <vector>

// Common return point: the Paladin order hall (Sanctum of Light, Dalaran/Broken Isle map 1220), beside the questgivers
// and the shared quest-ender Lord Maxwell Tyrosus.
namespace
{
constexpr uint32 MAP_ORDER_HALL = 1220;
constexpr Position OrderHallReturn = { -848.4f, 4257.1f, 746.28f, 0.87f };
}

// ---------------------------------------------------------------------------------------------------------------------
// Shared events.

// Flight/portal stand-in: grant the "fly to scenario" (and, for Holy, "take the portal") credits, then drop the player
// at the scenario landing so the encounter can begin. A real on-rails flight is the intended presentation; this
// transfer is the functional stand-in.
class ArtifactFlightEvent : public BasicEvent
{
public:
    ArtifactFlightEvent(Player* player, uint32 map, Position dest, std::vector<uint32> credits)
        : _player(player), _map(map), _dest(dest), _credits(std::move(credits)) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (_player->IsInWorld())
        {
            for (uint32 credit : _credits)
                _player->KilledMonsterCredit(credit);
            _player->TeleportTo(_map, _dest.GetPositionX(), _dest.GetPositionY(), _dest.GetPositionZ(), _dest.GetOrientation());
        }
        return true;
    }

private:
    Player* _player;
    uint32 _map;
    Position _dest;
    std::vector<uint32> _credits;
};

// Return the player to the order hall after the artifact is claimed, granting the "Return to Dalaran" credit if the
// spec has one (Holy/Protection = 105892; Retribution has no return objective, pass 0).
class ArtifactReturnEvent : public BasicEvent
{
public:
    ArtifactReturnEvent(Player* player, uint32 returnCredit) : _player(player), _returnCredit(returnCredit) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (_player->IsInWorld())
        {
            if (_returnCredit)
                _player->KilledMonsterCredit(_returnCredit);
            _player->TeleportTo(MAP_ORDER_HALL, OrderHallReturn.GetPositionX(), OrderHallReturn.GetPositionY(),
                OrderHallReturn.GetPositionZ(), OrderHallReturn.GetOrientation());
        }
        return true;
    }

private:
    Player* _player;
    uint32 _returnCredit;
};

// =====================================================================================================================
// HOLY - "The Silver Hand" (42120), scenario 1092, map 1539.
// =====================================================================================================================
namespace
{
constexpr uint32 QUEST_THE_SILVER_HAND   = 42120;
constexpr uint32 MAP_TIRISFAL_SCENARIO   = 1539;
constexpr uint32 NPC_HORRIFIC_ABERRATION = 106669;
constexpr uint32 CREDIT_HOLY_PORTAL      = 102394; // obj 0 "Take the Portal to Dalaran Crater"
constexpr uint32 CREDIT_HOLY_FLY         = 106416; // obj 1 "Fly to Scenario"
constexpr uint32 CREDIT_HOLY_WON         = 106357; // obj 2 "Won Scenario"
constexpr uint32 CREDIT_HOLY_RETURN      = 105892; // obj 3 "Go to Dalaran"
constexpr Position SilverHandLanding = { 2137.70f, 2397.60f, 118.54f, 4.80f };
}

struct quest_the_silver_hand : QuestScript
{
    quest_the_silver_hand() : QuestScript("quest_the_silver_hand") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus == QUEST_STATUS_INCOMPLETE)
            player->m_Events.AddEventAtOffset(new ArtifactFlightEvent(player, MAP_TIRISFAL_SCENARIO, SilverHandLanding,
                { CREDIT_HOLY_PORTAL, CREDIT_HOLY_FLY }), 1500ms);
    }
};

// Scenario 1092 director, bound to the Tirisfal Travard copy (106429). Set active so the AI runs wherever the party
// is; advances the on-screen scenario steps by the player's own progress from the landing and speaks the beats. The
// quest itself is completed from the boss death + the flight/return events, so a scenario hiccup never strands the run.
struct npc_travard_scenario_director : public ScriptedAI
{
    npc_travard_scenario_director(Creature* creature) : ScriptedAI(creature) { }

    uint32 _pollTimer = 0;
    uint8  _beat = 0;

    void Reset() override
    {
        if (me->GetMap()->GetId() == MAP_TIRISFAL_SCENARIO)
            me->setActive(true);
    }

    void UpdateAI(uint32 diff) override
    {
        if (me->GetMap()->GetId() != MAP_TIRISFAL_SCENARIO)
            return;

        _pollTimer += diff;
        if (_pollTimer < 1000)
            return;
        _pollTimer = 0;

        if (!ArtifactPlayerBeyond(me, SilverHandLanding, 0.0f))
            return; // no player on the isle yet

        switch (_beat)
        {
            case 0:
                me->Say("Ah, there you are. The area was secured at great cost. This cannot continue - we must retrieve the Silver Hand and destroy the entrance to the tomb.", LANG_UNIVERSAL);
                ArtifactAdvanceScenario(me);
                ++_beat;
                break;
            case 1:
                if (ArtifactPlayerBeyond(me, SilverHandLanding, 45.0f))
                {
                    me->Say("Behold the Tomb of Tyr... wait! These monsters dare befoul the tomb?! This cannot stand - destroy these abominations!", LANG_UNIVERSAL);
                    ArtifactAdvanceScenario(me);
                    ++_beat;
                }
                break;
            case 2:
                if (ArtifactPlayerBeyond(me, SilverHandLanding, 110.0f))
                {
                    ArtifactAdvanceScenario(me);
                    ++_beat;
                }
                break;
            default:
                break;
        }
    }
};

// Horrific Aberration (106669) - Holy scenario boss. Placeholder faction 35 -> made hostile. Its death credits "Won
// Scenario" (106357) and sends the party home (granting the "Go to Dalaran" credit) after the finale beats.
struct npc_horrific_aberration : public ScriptedAI
{
    npc_horrific_aberration(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        if (me->GetMap()->GetId() == MAP_TIRISFAL_SCENARIO)
            me->SetFaction(FACTION_MONSTER_2); // faction 16 - attackable
    }

    void JustDied(Unit* /*killer*/) override
    {
        if (me->GetMap()->GetId() != MAP_TIRISFAL_SCENARIO)
            return;

        me->Say("It is done. Quickly, take up the hammer! In so doing you will ensure Tyr's remains are protected for all time!", LANG_UNIVERSAL);
        ArtifactAdvanceScenario(me); // "Won Scenario" beat on-screen

        for (auto const& ref : me->GetMap()->GetPlayers())
            if (Player* p = ref.GetSource())
                if (p->IsInWorld())
                {
                    p->KilledMonsterCredit(CREDIT_HOLY_WON);                                  // obj 2
                    p->m_Events.AddEventAtOffset(new ArtifactReturnEvent(p, CREDIT_HOLY_RETURN), 8s); // obj 3 + home
                }
    }
};

// =====================================================================================================================
// PROTECTION - "Shrine of the Truthguard" (42017), scenario 1082, map 1495.
// =====================================================================================================================
namespace
{
constexpr uint32 QUEST_SHRINE_TRUTHGUARD = 42017;
constexpr uint32 MAP_SHIELDS_REST        = 1495;
constexpr uint32 NPC_YRGRIM_TRUTHSEEKER  = 105695;
constexpr uint32 CREDIT_PROT_FLY         = 105889; // obj 0 "Fly to Scenario"
constexpr uint32 CREDIT_PROT_WON         = 105891; // obj 1 "Claim the Truthguard"
constexpr uint32 CREDIT_PROT_RETURN      = 105892; // obj 2 "Return to Dalaran"
constexpr Position TruthguardLanding = { 4804.64f, 148.32f, 22.38f, 1.10f };
}

struct quest_shrine_of_the_truthguard : QuestScript
{
    quest_shrine_of_the_truthguard() : QuestScript("quest_shrine_of_the_truthguard") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus == QUEST_STATUS_INCOMPLETE)
            player->m_Events.AddEventAtOffset(new ArtifactFlightEvent(player, MAP_SHIELDS_REST, TruthguardLanding,
                { CREDIT_PROT_FLY }), 1500ms);
    }
};

// Scenario 1082 director, bound to Orik Trueheart (105910) on Shield's Rest. Advances the on-screen steps and speaks
// the trial-by-combat beats as the player pushes toward Yrgrim.
struct npc_orik_scenario_director : public ScriptedAI
{
    npc_orik_scenario_director(Creature* creature) : ScriptedAI(creature) { }

    uint32 _pollTimer = 0;
    uint8  _beat = 0;

    void Reset() override
    {
        if (me->GetMap()->GetId() == MAP_SHIELDS_REST)
            me->setActive(true);
    }

    void UpdateAI(uint32 diff) override
    {
        if (me->GetMap()->GetId() != MAP_SHIELDS_REST)
            return;

        _pollTimer += diff;
        if (_pollTimer < 1000)
            return;
        _pollTimer = 0;

        if (!ArtifactPlayerBeyond(me, TruthguardLanding, 0.0f))
            return;

        switch (_beat)
        {
            case 0:
                me->Say("Cato! I'm glad ye could make it. The shield is close... I can feel it!", LANG_UNIVERSAL);
                ArtifactAdvanceScenario(me);
                ++_beat;
                break;
            case 1:
                if (ArtifactPlayerBeyond(me, TruthguardLanding, 60.0f))
                {
                    me->Say("Lookee! Seems Yrgrim was the champion, and he came here after Tyr's fall. Tomb's up ahead - we'll follow yer lead, champion.", LANG_UNIVERSAL);
                    ArtifactAdvanceScenario(me);
                    ++_beat;
                }
                break;
            case 2:
                if (ArtifactPlayerBeyond(me, TruthguardLanding, 160.0f))
                {
                    ArtifactAdvanceScenario(me);
                    ++_beat;
                }
                break;
            default:
                break;
        }
    }
};

// Yrgrim the Truthseeker (105695) - Protection scenario boss. Placeholder faction 35 -> made hostile. His death
// credits "Claim the Truthguard" (105891) and sends the party home (granting "Return to Dalaran" 105892).
struct npc_yrgrim_truthseeker : public ScriptedAI
{
    npc_yrgrim_truthseeker(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        if (me->GetMap()->GetId() == MAP_SHIELDS_REST)
            me->SetFaction(FACTION_MONSTER_2); // faction 16
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        if (me->GetMap()->GetId() == MAP_SHIELDS_REST)
            me->Say("I bid you greetings, Opener of Doors. You are the first. Long have I waited for one who proves worthy. Face me in battle that we might test your heart!", LANG_UNIVERSAL);
    }

    void JustDied(Unit* /*killer*/) override
    {
        if (me->GetMap()->GetId() != MAP_SHIELDS_REST)
            return;

        me->Say("I am beaten! You are indeed worthy. Were Tyr alive, he would make you his champion. I hereby bestow Truthguard upon you.", LANG_UNIVERSAL);
        ArtifactAdvanceScenario(me);

        for (auto const& ref : me->GetMap()->GetPlayers())
            if (Player* p = ref.GetSource())
                if (p->IsInWorld())
                {
                    p->KilledMonsterCredit(CREDIT_PROT_WON);                                  // obj 1
                    p->m_Events.AddEventAtOffset(new ArtifactReturnEvent(p, CREDIT_PROT_RETURN), 8s); // obj 2 + home
                }
    }
};

// =====================================================================================================================
// RETRIBUTION - "The Search for the Highlord" (38376), scenario 775, map 1500.
// =====================================================================================================================
namespace
{
constexpr uint32 QUEST_SEARCH_HIGHLORD   = 38376;
constexpr uint32 MAP_BROKEN_SHORE        = 1500;
constexpr uint32 NPC_BALNAZZAR           = 90981;  // obj 2 kill target; authored hostile faction 14
constexpr uint32 CREDIT_RET_FLY          = 90384;  // obj 0 "Argent Hippogryph"
constexpr uint32 CREDIT_RET_WON          = 114505; // obj 1 "Kill Credit"
constexpr Position BrokenShoreLanding = { -2436.54f, 138.51f, 8.09f, 3.77f };
}

struct quest_search_for_the_highlord : QuestScript
{
    quest_search_for_the_highlord() : QuestScript("quest_search_for_the_highlord") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus == QUEST_STATUS_INCOMPLETE)
            player->m_Events.AddEventAtOffset(new ArtifactFlightEvent(player, MAP_BROKEN_SHORE, BrokenShoreLanding,
                { CREDIT_RET_FLY }), 1500ms);
    }
};

// Scenario 775 director, bound to Highlord Tirion Fordring (92676) on the Broken Shore. Advances the on-screen assault
// steps and speaks the Argent Crusade beats as the player pushes toward Balnazzar's arena.
struct npc_tirion_scenario_director : public ScriptedAI
{
    npc_tirion_scenario_director(Creature* creature) : ScriptedAI(creature) { }

    uint32 _pollTimer = 0;
    uint8  _beat = 0;

    void Reset() override
    {
        if (me->GetMap()->GetId() == MAP_BROKEN_SHORE)
            me->setActive(true);
    }

    void UpdateAI(uint32 diff) override
    {
        if (me->GetMap()->GetId() != MAP_BROKEN_SHORE)
            return;

        _pollTimer += diff;
        if (_pollTimer < 1000)
            return;
        _pollTimer = 0;

        if (!ArtifactPlayerBeyond(me, BrokenShoreLanding, 0.0f))
            return;

        switch (_beat)
        {
            case 0:
                me->Say("Sound the charge! The Argent Crusade marches on the Legion - reclaim the Ashbringer, champion!", LANG_UNIVERSAL);
                ArtifactAdvanceScenario(me);
                ++_beat;
                break;
            case 1:
                if (ArtifactPlayerBeyond(me, BrokenShoreLanding, 120.0f))
                {
                    me->Say("The Lost Temple lies ahead. Cut through the demons and take back Tirion's blade!", LANG_UNIVERSAL);
                    ArtifactAdvanceScenario(me);
                    ++_beat;
                }
                break;
            case 2:
                if (ArtifactPlayerBeyond(me, BrokenShoreLanding, 300.0f))
                {
                    me->Say("Balnazzar the Risen commands this assault. End him, and the Highlord's charge is won!", LANG_UNIVERSAL);
                    ArtifactAdvanceScenario(me);
                    ++_beat;
                }
                break;
            default:
                break;
        }
    }
};

// The Broken Shore demon placeholders - Jailer Zerus (91672) and Dark Inquisitor (91697). Both faction-35 placeholders,
// made hostile so the player can cut through them on the way to Balnazzar. Bound to both entries via the same
// ScriptName.
struct npc_broken_shore_demon : public ScriptedAI
{
    npc_broken_shore_demon(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        if (me->GetMap()->GetId() == MAP_BROKEN_SHORE)
            me->SetFaction(FACTION_MONSTER_2); // faction 16
    }
};

// Balnazzar the Risen (90981) - the Retribution objective kill (obj 2 tracks entry 90981 directly, credited by the
// kill). Already authored on hostile faction 14, so it is NOT re-factioned. Its death additionally grants the "Won"
// credit (114505) and sends the party home. No "Return to Dalaran" objective exists for this quest.
struct npc_balnazzar_risen : public ScriptedAI
{
    npc_balnazzar_risen(Creature* creature) : ScriptedAI(creature) { }

    void JustDied(Unit* /*killer*/) override
    {
        if (me->GetMap()->GetId() != MAP_BROKEN_SHORE)
            return;

        me->Say("You must... wield the blade... you must... stop the Legion... You must become... the Ashbringer...", LANG_UNIVERSAL);
        ArtifactAdvanceScenario(me);

        for (auto const& ref : me->GetMap()->GetPlayers())
            if (Player* p = ref.GetSource())
                if (p->IsInWorld())
                {
                    p->KilledMonsterCredit(CREDIT_RET_WON);                          // obj 1 (obj 2 = the kill itself)
                    p->m_Events.AddEventAtOffset(new ArtifactReturnEvent(p, 0), 10s); // home; no return objective
                }
    }
};

// =====================================================================================================================
void AddSC_artifact_paladin()
{
    // Quests
    new quest_the_silver_hand();
    new quest_shrine_of_the_truthguard();
    new quest_search_for_the_highlord();

    // Creatures
    RegisterCreatureAI(npc_travard_scenario_director);
    RegisterCreatureAI(npc_horrific_aberration);
    RegisterCreatureAI(npc_orik_scenario_director);
    RegisterCreatureAI(npc_yrgrim_truthseeker);
    RegisterCreatureAI(npc_tirion_scenario_director);
    RegisterCreatureAI(npc_broken_shore_demon);
    RegisterCreatureAI(npc_balnazzar_risen);
}
