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

// Monk Legion artifact acquisition (all three specs). Per-class Broken Isles content, mirroring the proven Beast
// Mastery Hunter director/escort pattern in zone_orderhall_hunter.cpp; shared helpers live in
// orderhall_artifact_common.h. Each acquisition is a Pandaria-themed InstanceScenario, but the Legion artifact
// scenarios ship as empty placeholder content in our world DB (no game-event step criteria fire, and every enemy is a
// faction-35 "universal friendly" placeholder), so - exactly as for the Hunter - we:
//   * teleport the player into the scenario map when the acquisition quest is accepted (the on-rails flight/escort is
//     absent from our world tables; the teleport is the functional stand-in),
//   * drive the on-screen scenario tracker by hand via ArtifactAdvanceScenario() as the player pushes forward,
//   * make the placeholder bosses/adds hostile from script (faction 16) so the encounter is actually fightable, and
//   * complete the acquisition quest authoritatively from the FINAL boss's death (kill-credit + CompleteQuest), so a
//     scenario hiccup can never strand the run, then ride the player home to the Monk order hall (map 1514).
//
//   Brewmaster  : "The Wanderer's Companion" (42762) -> Fu Zan.            SCENARIO 1137, map 1625 (Trial of the
//                 Serpent). Closer: Eredar Lord Korithis (109821) dies -> credit 101880 -> 42762 complete.
//   Mistweaver  : "The Emperor's Gift"      (41003) -> Sheilun.           SCENARIO 1007, map 1541 (Terrace of Endless
//                 Spring). Closer: Aspersius (101887) dies -> credits {102479,101880} -> 41003 complete.
//   Windwalker  : "The Thundering Heavens"  (42790, CREATED here - no     SCENARIO 983,  map 1528 (Skywall). No
//                 acquisition quest exists in our DB) -> Fists of the      acquisition quest existed in the DB, so a
//                 Heavens.                                                 minimal one is authored in the SQL. Closer:
//                 Typhinius (100760) dies -> quest objective (kill 100760) auto-credits -> 42790 complete.

#include "Creature.h"
#include "DB2Structure.h"
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

enum MonkArtifactData
{
    // Acquisition quests
    QUEST_WANDERERS_COMPANION = 42762, // Brewmaster -> Fu Zan
    QUEST_EMPERORS_GIFT       = 41003, // Mistweaver -> Sheilun
    QUEST_THUNDERING_HEAVENS  = 42790, // Windwalker -> Fists of the Heavens (authored in artifact_monk.sql)

    // Scenario maps
    MAP_TRIAL_OF_SERPENT      = 1625,  // Brewmaster scenario 1137
    MAP_TERRACE_ENDLESS       = 1541,  // Mistweaver scenario 1007
    MAP_SKYWALL               = 1528,  // Windwalker scenario 983
    MAP_MONK_ORDER_HALL       = 1514,  // The Wandering Isle - return point for all three

    // Directors (allies whose AI walks the scenario)
    NPC_YULON                 = 109391, // Brewmaster director (map 1625)
    NPC_TARAN_ZHU             = 101881, // Mistweaver director (map 1541)
    NPC_LILI_STORMSTOUT       = 100740, // Windwalker director (map 1528)

    // Final bosses (death completes the acquisition quest)
    NPC_LORD_KORITHIS         = 109821, // Brewmaster boss
    NPC_ASPERSIUS             = 101887, // Mistweaver boss
    NPC_TYPHINIUS             = 100760, // Windwalker boss

    // Kill-credit dummy NPCs
    NPC_CREDIT_TAK_TAK        = 101880, // 42762 obj0; 41003 obj0/obj2
    NPC_CREDIT_SHEILUN        = 102479  // 41003 obj1 (touch Sheilun)
};

// Scenario landing origins (player is dropped here on quest accept; scenario steps advance by distance from here).
static constexpr Position BrewmasterLanding = { 826.79f, -2549.49f, 183.12f, 1.27f };  // beside The Monkey King
static constexpr Position MistweaverLanding = { -1047.99f, -3144.39f, 28.32f, 3.02f };  // beside Taran Zhu
static constexpr Position WindwalkerLanding = { -735.29f, 436.08f, 644.55f, 1.47f };    // beside Li Li Stormstout
static constexpr Position OrderHallReturn   = { 883.41f, 3609.41f, 192.42f, 3.53f };    // Monk order hall (map 1514)

// ---------------------------------------------------------------------------------------------------------------------
// Shared helpers.

// Resolve the player who should receive credit for a boss kill: the killer (or its owner), else any player on the map.
static Player* ResolveCreditPlayer(Creature* me, Unit* killer)
{
    if (killer)
        if (Player* p = killer->GetCharmerOrOwnerPlayerOrPlayerItself())
            return p;
    for (auto const& ref : me->GetMap()->GetPlayers())
        if (Player* p = ref.GetSource())
            return p;
    return nullptr;
}

// Ride the party back to the Monk order hall (map 1514) once the artifact is claimed.
class MonkReturnHomeEvent : public BasicEvent
{
public:
    explicit MonkReturnHomeEvent(Player* player) : _player(player) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (_player->IsInWorld() && _player->GetMapId() != MAP_MONK_ORDER_HALL)
            _player->TeleportTo(MAP_MONK_ORDER_HALL, OrderHallReturn.GetPositionX(), OrderHallReturn.GetPositionY(),
                OrderHallReturn.GetPositionZ(), OrderHallReturn.GetOrientation());
        return true;
    }

private:
    Player* _player;
};

// Teleport the player into a scenario map when the acquisition quest is accepted (flight/escort stand-in).
class MonkScenarioEntryEvent : public BasicEvent
{
public:
    MonkScenarioEntryEvent(Player* player, uint32 mapId, Position dest) : _player(player), _mapId(mapId), _dest(dest) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (_player->IsInWorld())
            _player->TeleportTo(_mapId, _dest.GetPositionX(), _dest.GetPositionY(), _dest.GetPositionZ(),
                _dest.GetOrientation());
        return true;
    }

private:
    Player* _player;
    uint32 _mapId;
    Position _dest;
};

// A faction-35 placeholder made hostile so it can be fought. Used for the mid bosses / Skywall adds. The final boss of
// each spec has its own AI (below) that also drives quest completion, so this is only the "attackable filler" case.
struct npc_monk_artifact_hostile : public ScriptedAI
{
    npc_monk_artifact_hostile(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override { me->SetFaction(FACTION_MONSTER_2); } // faction 16 - hostile
};

// ---------------------------------------------------------------------------------------------------------------------
// Brewmaster: "The Wanderer's Companion" (42762), scenario 1137 on map 1625 (Trial of the Serpent).

struct quest_the_wanderers_companion : QuestScript
{
    quest_the_wanderers_companion() : QuestScript("quest_the_wanderers_companion") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus == QUEST_STATUS_INCOMPLETE) // accepted -> travel to the Trial of the Serpent
            player->m_Events.AddEventAtOffset(new MonkScenarioEntryEvent(player, MAP_TRIAL_OF_SERPENT, BrewmasterLanding), 1500ms);
    }
};

// Yu'lon (109391): scenario 1137 director. Set active so her AI runs wherever the party is; she voices the beats and
// advances the on-screen tracker as the player pushes south toward Korithis. Authoritative completion is Korithis'
// death (below), so the tracker is presentation only.
struct npc_yulon_brm_director : public ScriptedAI
{
    npc_yulon_brm_director(Creature* creature) : ScriptedAI(creature) { }

    uint32 _poll = 0;
    uint8  _step = 0;

    void Reset() override
    {
        if (me->GetMap()->GetId() == MAP_TRIAL_OF_SERPENT)
            me->setActive(true);
    }

    void UpdateAI(uint32 diff) override
    {
        if (me->GetMap()->GetId() != MAP_TRIAL_OF_SERPENT)
            return;

        _poll += diff;
        if (_poll < 1000)
            return;
        _poll = 0;

        if (!ArtifactPlayerBeyond(me, BrewmasterLanding, 0.0f)) // no player on the map yet
            return;

        static float const gates[] = { 60.0f, 110.0f, 160.0f };
        while (_step < 3 && ArtifactPlayerBeyond(me, BrewmasterLanding, gates[_step]))
        {
            ArtifactAdvanceScenario(me);
            switch (_step)
            {
                case 0: me->Say("The Monkey King guards Fu Zan. Press on, champion - the temple is under siege!", LANG_UNIVERSAL); break;
                case 1: me->Say("The Legion's fel wings darken the sky. Avenge the fallen scribes!", LANG_UNIVERSAL); break;
                default: me->Say("Korithis commands them. Cut him down and claim the Wanderer's Companion!", LANG_UNIVERSAL); break;
            }
            ++_step;
        }
    }
};

// Eredar Lord Korithis (109821): Brewmaster final boss. Made hostile; his death grants the Tak-Tak credit (42762's
// single objective) and completes the quest, then rides the party home.
struct npc_lord_korithis : public ScriptedAI
{
    npc_lord_korithis(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override { me->SetFaction(FACTION_MONSTER_2); } // faction 16 - attackable Brewmaster boss

    void JustEngagedWith(Unit* /*who*/) override
    {
        me->Yell("Yu'lon's brood will burn, and the Legion will feast on your ashes!", LANG_UNIVERSAL);
    }

    void JustDied(Unit* killer) override
    {
        if (me->GetMap()->GetId() != MAP_TRIAL_OF_SERPENT)
            return;

        me->Yell("Impossible... the flight... endures...", LANG_UNIVERSAL);

        if (Scenario* s = me->GetScenario())
            if (s->GetStep())
                ArtifactAdvanceScenario(me); // step "Fu Zan, the Wanderer's Companion"

        Player* credited = ResolveCreditPlayer(me, killer);
        for (auto const& ref : me->GetMap()->GetPlayers())
            if (Player* p = ref.GetSource())
                if (p->IsInWorld())
                {
                    ArtifactGrantCredits(p, { NPC_CREDIT_TAK_TAK });          // 42762 objective 0
                    if (p->GetQuestStatus(QUEST_WANDERERS_COMPANION) == QUEST_STATUS_INCOMPLETE)
                        p->CompleteQuest(QUEST_WANDERERS_COMPANION);
                    p->m_Events.AddEventAtOffset(new MonkReturnHomeEvent(p), 6s); // ride Yu'lon home
                }

        if (credited)
            TC_LOG_INFO("scripts", "[Fu Zan 1137] Korithis down -> 42762 completed for {}", credited->GetName());
    }
};

// ---------------------------------------------------------------------------------------------------------------------
// Mistweaver: "The Emperor's Gift" (41003), scenario 1007 on map 1541 (Terrace of Endless Spring).

struct quest_the_emperors_gift : QuestScript
{
    quest_the_emperors_gift() : QuestScript("quest_the_emperors_gift") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus == QUEST_STATUS_INCOMPLETE) // accepted -> travel to the Terrace of Endless Spring
            player->m_Events.AddEventAtOffset(new MonkScenarioEntryEvent(player, MAP_TERRACE_ENDLESS, MistweaverLanding), 1500ms);
    }
};

// Taran Zhu (101881): scenario 1007 director/escort. Set active; voices the beats and advances the tracker as the
// party moves north toward Aspersius. Completion is authoritatively Aspersius' death (below).
struct npc_taran_zhu_mw_director : public ScriptedAI
{
    npc_taran_zhu_mw_director(Creature* creature) : ScriptedAI(creature) { }

    uint32 _poll = 0;
    uint8  _step = 0;

    void Reset() override
    {
        if (me->GetMap()->GetId() == MAP_TERRACE_ENDLESS)
            me->setActive(true);
    }

    void UpdateAI(uint32 diff) override
    {
        if (me->GetMap()->GetId() != MAP_TERRACE_ENDLESS)
            return;

        _poll += diff;
        if (_poll < 1000)
            return;
        _poll = 0;

        if (!ArtifactPlayerBeyond(me, MistweaverLanding, 0.0f))
            return;

        static float const gates[] = { 90.0f, 180.0f, 280.0f };
        while (_step < 3 && ArtifactPlayerBeyond(me, MistweaverLanding, gates[_step]))
        {
            ArtifactAdvanceScenario(me);
            switch (_step)
            {
                case 0: me->Say("Free the Shado-Pan from their cages - Xaphan will pay for this!", LANG_UNIVERSAL); break;
                case 1: me->Say("The waters are poisoned. Aspersius lies ahead.", LANG_UNIVERSAL); break;
                default: me->Say("Strike him down and the Emperor's Gift, Sheilun, is yours.", LANG_UNIVERSAL); break;
            }
            ++_step;
        }
    }
};

// Aspersius (101887): Mistweaver final boss. Made hostile; his death grants the Sheilun (touch) + Tak-Tak credits and
// completes the quest, then rides the party home. CompleteQuest is the safety net for 41003's two Tak-Tak objectives
// (orders 0 and 2) plus the Sheilun objective (order 1).
struct npc_aspersius : public ScriptedAI
{
    npc_aspersius(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override { me->SetFaction(FACTION_MONSTER_2); } // faction 16 - attackable Mistweaver boss

    void JustEngagedWith(Unit* /*who*/) override
    {
        me->Yell("Drink deep of the fel tides, mortal!", LANG_UNIVERSAL);
    }

    void JustDied(Unit* killer) override
    {
        if (me->GetMap()->GetId() != MAP_TERRACE_ENDLESS)
            return;

        me->Yell("The mists... reclaim me...", LANG_UNIVERSAL);

        if (Scenario* s = me->GetScenario())
            if (s->GetStep())
                ArtifactAdvanceScenario(me); // step "The Emperor's Final Gift"

        Player* credited = ResolveCreditPlayer(me, killer);
        for (auto const& ref : me->GetMap()->GetPlayers())
            if (Player* p = ref.GetSource())
                if (p->IsInWorld())
                {
                    ArtifactGrantCredits(p, { NPC_CREDIT_SHEILUN, NPC_CREDIT_TAK_TAK }); // 41003 obj1 + obj0/obj2
                    if (p->GetQuestStatus(QUEST_EMPERORS_GIFT) == QUEST_STATUS_INCOMPLETE)
                        p->CompleteQuest(QUEST_EMPERORS_GIFT);
                    p->m_Events.AddEventAtOffset(new MonkReturnHomeEvent(p), 6s);
                }

        if (credited)
            TC_LOG_INFO("scripts", "[Sheilun 1007] Aspersius down -> 41003 completed for {}", credited->GetName());
    }
};

// ---------------------------------------------------------------------------------------------------------------------
// Windwalker: "The Thundering Heavens" (42790, authored in artifact_monk.sql), scenario 983 on map 1528 (Skywall).
// The quest's single objective is a kill of Typhinius (100760), so his (scripted-hostile) death both auto-credits the
// objective and, via CompleteQuest, guarantees closure.

struct quest_the_thundering_heavens : QuestScript
{
    quest_the_thundering_heavens() : QuestScript("quest_the_thundering_heavens") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus == QUEST_STATUS_INCOMPLETE) // accepted -> fly to Skywall
            player->m_Events.AddEventAtOffset(new MonkScenarioEntryEvent(player, MAP_SKYWALL, WindwalkerLanding), 1500ms);
    }
};

// Li Li Stormstout (100740): scenario 983 director/escort. Set active; voices the beats and advances the tracker as
// the party crosses Skywall toward Typhinius. Completion is authoritatively Typhinius' death (below).
struct npc_lili_ww_director : public ScriptedAI
{
    npc_lili_ww_director(Creature* creature) : ScriptedAI(creature) { }

    uint32 _poll = 0;
    uint8  _step = 0;

    void Reset() override
    {
        if (me->GetMap()->GetId() == MAP_SKYWALL)
            me->setActive(true);
    }

    void UpdateAI(uint32 diff) override
    {
        if (me->GetMap()->GetId() != MAP_SKYWALL)
            return;

        _poll += diff;
        if (_poll < 1000)
            return;
        _poll = 0;

        if (!ArtifactPlayerBeyond(me, WindwalkerLanding, 0.0f))
            return;

        static float const gates[] = { 120.0f, 300.0f, 450.0f };
        while (_step < 3 && ArtifactPlayerBeyond(me, WindwalkerLanding, gates[_step]))
        {
            ArtifactAdvanceScenario(me);
            switch (_step)
            {
                case 0: me->Say("Keep up! Shatter the Stormtouched Orbs before they overload!", LANG_UNIVERSAL); break;
                case 1: me->Say("Ride the storm dragon - it'll carry you to the Tyrant!", LANG_UNIVERSAL); break;
                default: me->Say("There he is - Typhinius! The Fists of the Heavens await, champion!", LANG_UNIVERSAL); break;
            }
            ++_step;
        }
    }
};

// Typhinius (100760): Windwalker final boss. Made hostile; killing him auto-credits 42790's objective (ObjectID
// 100760). We additionally CompleteQuest as a safety net and ride the party home.
struct npc_typhinius : public ScriptedAI
{
    npc_typhinius(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override { me->SetFaction(FACTION_MONSTER_2); } // faction 16 - attackable Windwalker boss

    void JustEngagedWith(Unit* /*who*/) override
    {
        me->Yell("The heavens answer to me alone! You will be scattered on the wind!", LANG_UNIVERSAL);
    }

    void JustDied(Unit* killer) override
    {
        if (me->GetMap()->GetId() != MAP_SKYWALL)
            return;

        me->Yell("The... skies... fall silent...", LANG_UNIVERSAL);

        if (Scenario* s = me->GetScenario())
            if (s->GetStep())
                ArtifactAdvanceScenario(me); // step "Fists of the Heavens"

        Player* credited = ResolveCreditPlayer(me, killer);
        for (auto const& ref : me->GetMap()->GetPlayers())
            if (Player* p = ref.GetSource())
                if (p->IsInWorld())
                {
                    if (p->GetQuestStatus(QUEST_THUNDERING_HEAVENS) == QUEST_STATUS_INCOMPLETE)
                        p->CompleteQuest(QUEST_THUNDERING_HEAVENS); // objective is the kill itself; this is the safety net
                    p->m_Events.AddEventAtOffset(new MonkReturnHomeEvent(p), 6s);
                }

        if (credited)
            TC_LOG_INFO("scripts", "[Fists of the Heavens 983] Typhinius down -> 42790 completed for {}", credited->GetName());
    }
};

void AddSC_artifact_monk()
{
    // Quests
    new quest_the_wanderers_companion();
    new quest_the_emperors_gift();
    new quest_the_thundering_heavens();

    // Creatures
    RegisterCreatureAI(npc_monk_artifact_hostile);
    RegisterCreatureAI(npc_yulon_brm_director);
    RegisterCreatureAI(npc_lord_korithis);
    RegisterCreatureAI(npc_taran_zhu_mw_director);
    RegisterCreatureAI(npc_aspersius);
    RegisterCreatureAI(npc_lili_ww_director);
    RegisterCreatureAI(npc_typhinius);
}
