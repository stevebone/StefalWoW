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

// Shaman Legion artifact acquisition. All three Shaman weapons ship (in retail) as instanced Scenario runs, and the
// Legion artifact scenarios are empty placeholder content in our world DB (their DB2 game-event step criteria do NOT
// fire here), so - exactly as with the Beast Mastery Hunter (zone_orderhall_hunter.cpp) - we drive the scenario steps
// server-side via ArtifactAdvanceScenario() (orderhall_artifact_common.h) and complete the acquisition quest ourselves.
//
//   Elemental    "The Voice of Thunder" (39771) -> The Fist of Ra-den (128935)
//                Temple of the White Tiger (map 1526), scenario 976 "Master of Storms". Rehgar Earthfury 100306 hosts
//                the director; Sigurd the Giantslayer 100363 (step 1) and Lord Kra'vos 100546 (step 4, spawned here)
//                are faction-35 placeholders made hostile. Turn-in to Rehgar 96541 fires flag quest 41329 (item 128935).
//
//   Restoration  flag quest 41330 -> Sharas'dal, Scepter of the Tides (128911)
//                Throne of the Tides artifact scenario (map 1600), scenario 1066 "The Dark Queen and the Sea". Fully
//                populated map; Erunak Stonespeaker 102826 hosts the director, Kra'liss 102839 (step 2) and Lady
//                Zithreen 104856 (step 4) plus the naga trash are refactioned hostile. No acquisition/giver quest
//                exists in our DB (only the flag quest 41330), so the director completes 41330 directly.
//
//   Enhancement  flag quest 41328 -> Doomhammer (128819)
//                Scenario 950 "Cleansing of the Deep" has NO map and NO spawns in our DB (scenarioMapId 0), and there
//                is no acquisition quest - only the flag quest 41328. With no instanced content to run, we implement
//                the completable stand-in: accepting 41328 completes it (so the flag path can grant 128819). See risks.
//
// Multiple copies of a director NPC are spawned on each map (Rehgar x3 on 1526, Erunak x2 on 1600). Each copy runs its
// AI, but the switch re-reads the scenario's CURRENT step order every poll and only advances the matching order, so
// once any copy advances a step the others resync (they see the new order, reset their step timer, and do not advance
// that tick) - no explicit driver election is needed, the same-thread instance update serializes them.

#include "orderhall_artifact_common.h"
#include "Creature.h"
#include "EventProcessor.h"
#include "Log.h"
#include "Map.h"
#include "Player.h"
#include "QuestDef.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"
#include "SharedDefines.h"

// The shaman order hall (The Heart of Azeroth, at the Maelstrom) where Rehgar Earthfury 96541 gives/takes the quests.
enum ShamanOrderHall
{
    MAP_MAELSTROM_ORDERHALL = 1469
};
static constexpr Position OrderHallReturn = { 854.798f, 1032.64f, 48.2401f, 3.86252f }; // beside Rehgar 96541

// Teleport the player back to the shaman order hall once a scenario leg is done, ready to turn in to Rehgar 96541.
class ShamanReturnToMaelstromEvent : public BasicEvent
{
public:
    explicit ShamanReturnToMaelstromEvent(Player* player) : _player(player) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (_player->IsInWorld())
            _player->TeleportTo(MAP_MAELSTROM_ORDERHALL, OrderHallReturn.GetPositionX(), OrderHallReturn.GetPositionY(),
                OrderHallReturn.GetPositionZ(), OrderHallReturn.GetOrientation());
        return true;
    }

private:
    Player* _player;
};

// =====================================================================================================================
// Elemental: "The Voice of Thunder" (39771) -> The Fist of Ra-den. Scenario 976 "Master of Storms" on map 1526.
// =====================================================================================================================
enum ElementalArtifact
{
    QUEST_VOICE_OF_THUNDER      = 39771,
    MAP_WHITE_TIGER             = 1526,   // Temple of the White Tiger - scenario 976 instance
    SCENARIO_MASTER_OF_STORMS   = 976,
    NPC_REHGAR_DIRECTOR         = 100306, // ally on 1526, hosts the scenario director
    NPC_SIGURD_GIANTSLAYER      = 100363, // step 1 mini-boss (faction-35 placeholder -> hostile)
    NPC_LORD_KRAVOS             = 100546, // step 4 final boss (not spawned in DB -> spawned by our SQL; hostile)
    CREDIT_TRAVEL_WHITE_TIGER   = 97131,  // 39771 objective 0 "Travel to the Temple of the White Tiger"
    CREDIT_RETURN_MAELSTROM     = 101673  // 39771 objective 2 "Return to the Maelstrom"
};
static constexpr Position WhiteTigerLanding = { 3243.87f, 605.73f, 539.262f, 1.43f }; // beside Rehgar 100306

// On accepting 39771: credit the travel objective and drop the player at the Temple landing so the scenario begins.
class WhiteTigerTravelEvent : public BasicEvent
{
public:
    explicit WhiteTigerTravelEvent(Player* player) : _player(player) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (_player->IsInWorld())
        {
            _player->KilledMonsterCredit(CREDIT_TRAVEL_WHITE_TIGER);                  // objective 0
            _player->TeleportTo(MAP_WHITE_TIGER, WhiteTigerLanding.GetPositionX(), WhiteTigerLanding.GetPositionY(),
                WhiteTigerLanding.GetPositionZ(), WhiteTigerLanding.GetOrientation());
        }
        return true;
    }

private:
    Player* _player;
};

struct quest_voice_of_thunder : QuestScript
{
    quest_voice_of_thunder() : QuestScript("quest_voice_of_thunder") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus == QUEST_STATUS_INCOMPLETE) // just accepted from Rehgar 96541 -> travel to the Temple
            player->m_Events.AddEventAtOffset(new WhiteTigerTravelEvent(player), 1500ms);
    }
};

// Scenario 976 director, bound to Rehgar Earthfury 100306 on map 1526. Drives the five steps:
//   0 Speak with the White Tiger (Xuen)   - spoken beat, timer-advanced
//   1 Defeat Sigurd the Giantslayer        - gated on Sigurd 100363's death
//   2 Heroes of the Storm team victory      - spoken beat, timer-advanced
//   3 Weapons of the Storm                  - spoken beat, timer-advanced
//   4 Defeat Lord Kra'vos                   - gated on Kra'vos 100546's death -> finale (return credit + CompleteQuest)
struct npc_rehgar_scenario_director : public ScriptedAI
{
    npc_rehgar_scenario_director(Creature* creature) : ScriptedAI(creature) { }

    uint32 _pollTimer = 0;
    uint32 _stepTimer = 0;
    uint8  _lastOrder = 255;
    bool   _sigurdSeen = false;
    bool   _kravosSeen = false;
    bool   _completed = false;

    void Reset() override
    {
        if (me->GetMap()->GetId() == MAP_WHITE_TIGER)
            me->setActive(true);
    }

    // True once `entry` has been seen alive on the map and is now gone (dead/despawned) - a "boss defeated" test.
    bool BossDefeated(uint32 entry, bool& seen)
    {
        if (me->FindNearestCreature(entry, 1000.0f, true))
        {
            seen = true;
            return false;
        }
        return seen;
    }

    void RunFinale()
    {
        _completed = true;
        for (auto const& ref : me->GetMap()->GetPlayers())
            if (Player* p = ref.GetSource())
                if (p->IsInWorld())
                {
                    p->KilledMonsterCredit(CREDIT_RETURN_MAELSTROM);                 // objective 2
                    if (p->GetQuestStatus(QUEST_VOICE_OF_THUNDER) == QUEST_STATUS_INCOMPLETE)
                        p->CompleteQuest(QUEST_VOICE_OF_THUNDER);                     // objective 1 (Type-14 scenario) forced
                    p->m_Events.AddEventAtOffset(new ShamanReturnToMaelstromEvent(p), 5s);
                }
        ArtifactAdvanceScenario(me); // final step -> CompleteScenario(976)
        TC_LOG_INFO("scripts", "[Fist of Ra-den 976] finale: Kra'vos defeated -> complete + return to Maelstrom");
    }

    void UpdateAI(uint32 diff) override
    {
        if (me->GetMap()->GetId() != MAP_WHITE_TIGER || _completed)
            return;

        _pollTimer += diff;
        if (_pollTimer < 1000)
            return;
        _pollTimer = 0;

        if (!ArtifactPlayerBeyond(me, WhiteTigerLanding, 0.0f)) // no players on the isle yet
            return;

        uint8 const order = ArtifactScenarioOrder(me);
        if (order != _lastOrder) // a step boundary - resync all director copies and voice the new beat
        {
            _lastOrder = order;
            _stepTimer = 0;
            switch (order)
            {
                case 0: me->Say("Do you seek the weapons of the storm god? Step forward, shaman.", LANG_UNIVERSAL); break;
                case 1: me->Say("Who challenges me!? The weapons of the storm belong to my clan by rights!", LANG_UNIVERSAL); break;
                case 2: me->Say("Now - prove your storms in the trial of heroes. Fight at my side!", LANG_UNIVERSAL); break;
                case 3: me->Say("The Fist of Ra-den is your final challenge. If you can wield it and live, the weapon is yours to command.", LANG_UNIVERSAL); break;
                case 4: me->Say("Leave the mortals alive. We must find the weapons of their so-called Storm-God.", LANG_UNIVERSAL); break;
                default: break;
            }
            return;
        }
        _stepTimer += 1000;

        switch (order)
        {
            case 0: if (_stepTimer >= 6000) ArtifactAdvanceScenario(me); break;
            case 1: if (BossDefeated(NPC_SIGURD_GIANTSLAYER, _sigurdSeen)) ArtifactAdvanceScenario(me); break;
            case 2: if (_stepTimer >= 6000) ArtifactAdvanceScenario(me); break;
            case 3: if (_stepTimer >= 6000) ArtifactAdvanceScenario(me); break;
            case 4: if (BossDefeated(NPC_LORD_KRAVOS, _kravosSeen)) RunFinale(); break;
            default: break;
        }
    }
};

// Sigurd the Giantslayer 100363 - Elemental scenario step 1. Faction-35 placeholder made hostile so he is defeatable.
struct npc_sigurd_giantslayer : public ScriptedAI
{
    npc_sigurd_giantslayer(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        if (me->GetMap()->GetId() == MAP_WHITE_TIGER)
            me->SetFaction(FACTION_MONSTER_2); // 16 - hostile
    }
};

// Lord Kra'vos 100546 - Elemental scenario step 4 final boss (spawned by our SQL). Made hostile; the director watches
// for his death to run the finale.
struct npc_lord_kravos : public ScriptedAI
{
    npc_lord_kravos(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        if (me->GetMap()->GetId() == MAP_WHITE_TIGER)
            me->SetFaction(FACTION_MONSTER_2); // 16 - hostile
    }
};

// =====================================================================================================================
// Restoration: flag quest 41330 -> Sharas'dal. Scenario 1066 "The Dark Queen and the Sea" on map 1600 (fully spawned).
// =====================================================================================================================
enum RestorationArtifact
{
    QUEST_RESTORATION_CHOSEN    = 41330,  // flag quest (no giver/objectives in our DB) - completed by the director
    MAP_THRONE_OF_TIDES         = 1600,   // Throne of the Tides artifact scenario
    SCENARIO_DARK_QUEEN         = 1066,
    NPC_ERUNAK_DIRECTOR         = 102826, // ally on 1600, hosts the scenario director
    NPC_KRALISS                 = 102839, // step 2 mini-boss (faction-35 placeholder -> hostile)
    NPC_LADY_ZITHREEN           = 104856  // step 4 final boss / Sea Witch (faction-35 placeholder -> hostile)
};
static constexpr Position ThroneOfTidesLanding = { -561.293f, 807.25f, 245.198f, 0.0f }; // lower staging by Erunak/Grash

// On accepting 41330: drop the player at the Throne of the Tides staging area so the scenario begins.
class ThroneOfTidesTravelEvent : public BasicEvent
{
public:
    explicit ThroneOfTidesTravelEvent(Player* player) : _player(player) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (_player->IsInWorld())
            _player->TeleportTo(MAP_THRONE_OF_TIDES, ThroneOfTidesLanding.GetPositionX(), ThroneOfTidesLanding.GetPositionY(),
                ThroneOfTidesLanding.GetPositionZ(), ThroneOfTidesLanding.GetOrientation());
        return true;
    }

private:
    Player* _player;
};

struct quest_restoration_artifact_chosen : QuestScript
{
    quest_restoration_artifact_chosen() : QuestScript("quest_restoration_artifact_chosen") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus == QUEST_STATUS_INCOMPLETE) // accepted -> travel to the Throne of the Tides
            player->m_Events.AddEventAtOffset(new ThroneOfTidesTravelEvent(player), 1500ms);
    }
};

// Scenario 1066 director, bound to Erunak Stonespeaker 102826 on map 1600. Drives the six steps:
//   0 Big Help (heal & recruit Grash)      - spoken beat, timer-advanced
//   1 Rescue Wavespeaker Adelee            - spoken beat, timer-advanced
//   2 Secure the Central Junction          - gated on Kra'liss 102839's death
//   3 Water Gauntlet                       - spoken beat, timer-advanced
//   4 Destroy the Sea Witch                - gated on Lady Zithreen 104856's death
//   5 Acquire Sharas'dal                   - spoken beat -> finale (CompleteQuest 41330 + return)
struct npc_erunak_scenario_director : public ScriptedAI
{
    npc_erunak_scenario_director(Creature* creature) : ScriptedAI(creature) { }

    uint32 _pollTimer = 0;
    uint32 _stepTimer = 0;
    uint8  _lastOrder = 255;
    bool   _kralissSeen = false;
    bool   _zithreenSeen = false;
    bool   _completed = false;

    void Reset() override
    {
        if (me->GetMap()->GetId() == MAP_THRONE_OF_TIDES)
            me->setActive(true);
    }

    bool BossDefeated(uint32 entry, bool& seen)
    {
        if (me->FindNearestCreature(entry, 1000.0f, true))
        {
            seen = true;
            return false;
        }
        return seen;
    }

    void RunFinale()
    {
        _completed = true;
        for (auto const& ref : me->GetMap()->GetPlayers())
            if (Player* p = ref.GetSource())
                if (p->IsInWorld())
                {
                    if (p->GetQuestStatus(QUEST_RESTORATION_CHOSEN) == QUEST_STATUS_INCOMPLETE)
                        p->CompleteQuest(QUEST_RESTORATION_CHOSEN);
                    p->m_Events.AddEventAtOffset(new ShamanReturnToMaelstromEvent(p), 5s);
                }
        ArtifactAdvanceScenario(me); // final step -> CompleteScenario(1066)
        TC_LOG_INFO("scripts", "[Sharas'dal 1066] finale: scepter claimed -> complete + return to Maelstrom");
    }

    void UpdateAI(uint32 diff) override
    {
        if (me->GetMap()->GetId() != MAP_THRONE_OF_TIDES || _completed)
            return;

        _pollTimer += diff;
        if (_pollTimer < 1000)
            return;
        _pollTimer = 0;

        if (!ArtifactPlayerBeyond(me, ThroneOfTidesLanding, 0.0f))
            return;

        uint8 const order = ArtifactScenarioOrder(me);
        if (order != _lastOrder)
        {
            _lastOrder = order;
            _stepTimer = 0;
            switch (order)
            {
                case 0: me->Say("Shaman, speak with him. Perhaps we can gain a new ally down here.", LANG_UNIVERSAL); break;
                case 1: me->Say("Wavespeaker Adelee is held in the depths. Free her!", LANG_UNIVERSAL); break;
                case 2: me->Say("Kra'liss holds the central junction. Break through!", LANG_UNIVERSAL); break;
                case 3: me->Say("Ascend through the water gauntlet - I will guide the tides.", LANG_UNIVERSAL); break;
                case 4: me->Say("Sharas'dal is mine, for the glory of my queen alone!", LANG_UNIVERSAL); break;
                case 5: me->Say("Take up the Scepter, shaman. You have earned it.", LANG_UNIVERSAL); break;
                default: break;
            }
            return;
        }
        _stepTimer += 1000;

        switch (order)
        {
            case 0: if (_stepTimer >= 6000) ArtifactAdvanceScenario(me); break;
            case 1: if (_stepTimer >= 6000) ArtifactAdvanceScenario(me); break;
            case 2: if (BossDefeated(NPC_KRALISS, _kralissSeen)) ArtifactAdvanceScenario(me); break;
            case 3: if (_stepTimer >= 6000) ArtifactAdvanceScenario(me); break;
            case 4: if (BossDefeated(NPC_LADY_ZITHREEN, _zithreenSeen)) ArtifactAdvanceScenario(me); break;
            case 5: if (_stepTimer >= 6000) RunFinale(); break;
            default: break;
        }
    }
};

// Shared faction-fix AI for the Restoration scenario's hostile placeholders (Lady Zithreen 104856, Kra'liss 102839,
// Zithreenai Naga Brute 102792, Frenzied Deep Sea Crawler 105027, Frenzied Deep Sea Makrura 105028) - all faction-35
// in the DB, made hostile here so the encounters are defeatable. Bound to each entry by ScriptName.
struct npc_shaman_scenario_enemy : public ScriptedAI
{
    npc_shaman_scenario_enemy(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        if (me->GetMap()->GetId() == MAP_THRONE_OF_TIDES)
            me->SetFaction(FACTION_MONSTER_2); // 16 - hostile
    }
};

// =====================================================================================================================
// Enhancement: flag quest 41328 -> Doomhammer. Scenario 950 "Cleansing of the Deep" has NO map and NO spawns in our DB
// (scenarioMapId 0) and no acquisition quest exists - only the flag quest 41328. With no instanced content to run, the
// completable stand-in simply completes 41328 on accept so the artifact-grant flag path can hand over item 128819.
// (See risks: the full scenario 950 needs a map assignment + all creatures/GOs authored before it can be run for real.)
// =====================================================================================================================
enum EnhancementArtifact
{
    QUEST_ENHANCEMENT_CHOSEN = 41328
};

class EnhancementCompleteEvent : public BasicEvent
{
public:
    explicit EnhancementCompleteEvent(Player* player) : _player(player) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (_player->IsInWorld() && _player->GetQuestStatus(QUEST_ENHANCEMENT_CHOSEN) == QUEST_STATUS_INCOMPLETE)
            _player->CompleteQuest(QUEST_ENHANCEMENT_CHOSEN);
        return true;
    }

private:
    Player* _player;
};

struct quest_enhancement_artifact_chosen : QuestScript
{
    quest_enhancement_artifact_chosen() : QuestScript("quest_enhancement_artifact_chosen") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus == QUEST_STATUS_INCOMPLETE)
            player->m_Events.AddEventAtOffset(new EnhancementCompleteEvent(player), 2s);
    }
};

// =====================================================================================================================
void AddSC_artifact_shaman()
{
    // Quests
    new quest_voice_of_thunder();
    new quest_restoration_artifact_chosen();
    new quest_enhancement_artifact_chosen();

    // Creatures
    RegisterCreatureAI(npc_rehgar_scenario_director);
    RegisterCreatureAI(npc_sigurd_giantslayer);
    RegisterCreatureAI(npc_lord_kravos);
    RegisterCreatureAI(npc_erunak_scenario_director);
    RegisterCreatureAI(npc_shaman_scenario_enemy);
}