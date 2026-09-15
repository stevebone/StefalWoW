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

// Mage Legion artifact acquisition. Per-class Broken Isles content (cf. zone_orderhall_hunter.cpp); the generic
// class-hall framework lives in orderhall_legion.cpp, and the shared scenario/credit helpers in
// orderhall_artifact_common.h.
//
// The two implementable Mage artifacts are both "scenario" acquisitions that end on an instanced map. As with the
// Hunter Titanstrike scenario, the Legion artifact InstanceScenarios ship as empty placeholder content in our world DB
// (map 1583 and map 1616 have NO row in the `scenarios` table), so:
//   * the quest completion is driven authoritatively by the kill-credit dummies the quest_objectives reference
//     (granted from the boss' death), so the acquisition always completes; and
//   * the on-screen scenario-step presentation is advanced best-effort via ArtifactAdvanceScenario() IF a scenario
//     happens to be attached - a scenario hiccup can never strand the questline.
//
//   Arcane  - "The Nexus Vault" (42011) -> Aluneth. Scenario 1101 on map 1583. Bilaal 104502 is the spawned boss; the
//             quest's obj0 references credit 106708 (not spawned) + obj1 105109, both granted on Bilaal's death.
//             Ends at Archmage Kalec (105081) back in Dalaran (1220).
//   Frost   - "The Mage Hunter" (42479) -> Ebonchill. Scenario 1122 on map 1616. Balaadur 108168 is the boss; the
//             quest's obj0 is credit 108025 (flight in, granted on accept) + obj1 108026 (won, granted on Balaadur's
//             death). Ends at Meryl Felstorm (102700) back in Dalaran (1220).
//
// (Fire / Felo'melorn has NO acquisition quest present in this world DB - the entire chain is missing - so it is not
// wired here. See the file footer note.)

#include "Creature.h"
#include "EventProcessor.h"
#include "Log.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "QuestDef.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"
#include "SharedDefines.h"
#include "orderhall_artifact_common.h"

// A generic "ride/portal home" transfer used at the end of each Mage scenario: teleport a player from the scenario map
// back to a destination (Dalaran, beside the quest ender), a few seconds after the finale beats.
class MageReturnHomeEvent : public BasicEvent
{
public:
    MageReturnHomeEvent(Player* player, uint32 fromMap, uint32 toMap, Position const& dst)
        : _player(player), _fromMap(fromMap), _toMap(toMap), _dst(dst) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (_player->IsInWorld() && _player->GetMapId() == _fromMap)
            _player->TeleportTo(_toMap, _dst.GetPositionX(), _dst.GetPositionY(), _dst.GetPositionZ(), _dst.GetOrientation());
        return true;
    }

private:
    Player* _player;
    uint32 _fromMap;
    uint32 _toMap;
    Position _dst;
};

// =====================================================================================================================
// Arcane artifact: "The Nexus Vault" (42011) -> Aluneth, Greatstaff of the Magna. Scenario 1101, map 1583.
// =====================================================================================================================
enum NexusVaultData
{
    QUEST_THE_NEXUS_VAULT      = 42011,
    MAP_NEXUS_VAULT            = 1583,
    NPC_AZUREGOS_NARRATOR      = 106699, // friendly ally on map 1583 - the scenario director / narrator
    NPC_NEXUS_PRINCE_BILAAL    = 104502, // the spawned scenario boss (faction-35 placeholder -> made hostile here)
    CREDIT_BILAAL_TARGET       = 106708, // 42011 obj0 "Defeat Nexus-Prince Bilaal" (quest-target entry, not spawned)
    CREDIT_NEXUS_SCENARIO_DONE = 105109, // 42011 obj1 (scenario-complete / Aluneth claimed)
    NPC_ARCHMAGE_KALEC         = 105081, // 42011 ender, in Dalaran (map 1220)
    MAP_DALARAN_BROKEN_ISLE    = 1220
};

// Player landing on map 1583, beside the Azure Prisoner / Azuregos cluster where the scenario begins (step 0). Boss
// Bilaal is deeper in, at ~(4211, 7106, 268).
static constexpr Position NexusVaultLanding = { 4199.0f, 7405.0f, 263.0f, 4.71f };
// Where the party is returned in Dalaran (beside Archmage Kalec 105081, the quest ender).
static constexpr Position NexusVaultHome    = { -855.0f, 4637.0f, 749.4f, 5.34f };

// Accepting "The Nexus Vault" opens a portal in the Hall to the Nexus Vault instance; that scripted portal is absent
// from our world DB, so we transfer the player to the scenario landing when the quest is accepted.
class NexusVaultPortalEvent : public BasicEvent
{
public:
    explicit NexusVaultPortalEvent(Player* player) : _player(player) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (_player->IsInWorld())
            _player->TeleportTo(MAP_NEXUS_VAULT, NexusVaultLanding.GetPositionX(), NexusVaultLanding.GetPositionY(),
                NexusVaultLanding.GetPositionZ(), NexusVaultLanding.GetOrientation());
        return true;
    }

private:
    Player* _player;
};

struct quest_the_nexus_vault : QuestScript
{
    quest_the_nexus_vault() : QuestScript("quest_the_nexus_vault") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus == QUEST_STATUS_INCOMPLETE) // just accepted -> portal into the Nexus Vault
            player->m_Events.AddEventAtOffset(new NexusVaultPortalEvent(player), 1500ms);
    }
};

// Nexus-Prince Bilaal (104502): the spawned Arcane boss. A faction-35 placeholder, so we make him hostile in Reset().
// His death is the authoritative completion point - it grants BOTH quest-objective credits (obj0 106708 + obj1 105109)
// and completes 42011 - so the acquisition finishes whether or not scenario 1101 is attached.
struct npc_nexus_prince_bilaal : public ScriptedAI
{
    npc_nexus_prince_bilaal(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        if (me->GetMap()->GetId() == MAP_NEXUS_VAULT)
            me->SetFaction(FACTION_MONSTER_2); // faction 16 - attackable
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        if (me->GetMap()->GetId() == MAP_NEXUS_VAULT)
            me->Yell("Fools! You may have destroyed the Surge Needles, but you are too late to stop us! The power of Aluneth will rip open the breach!", LANG_UNIVERSAL);
    }

    void JustDied(Unit* /*killer*/) override
    {
        if (me->GetMap()->GetId() != MAP_NEXUS_VAULT)
            return;

        me->Yell("What!? The void cannot be defeated! Noooo!", LANG_UNIVERSAL);
        ArtifactAdvanceScenario(me); // advance whatever scenario is running here (Mage or Priest Discipline)

        for (auto const& ref : me->GetMap()->GetPlayers())
            if (Player* p = ref.GetSource())
                if (p->IsInWorld())
                {
                    ArtifactGrantCredits(p, { CREDIT_BILAAL_TARGET, CREDIT_NEXUS_SCENARIO_DONE }); // obj0 + obj1
                    if (p->GetQuestStatus(QUEST_THE_NEXUS_VAULT) == QUEST_STATUS_INCOMPLETE)
                        p->CompleteQuest(QUEST_THE_NEXUS_VAULT); // safety net: ready to hand in to Kalec
                    // Priest Discipline "The Light's Wrath" (41625) shares this boss + the Nexus Vault (map 1583).
                    if (p->GetQuestStatus(41625) == QUEST_STATUS_INCOMPLETE)
                    {
                        p->CompleteQuest(41625);
                        p->TeleportTo(1220, -857.2f, 4637.6f, 749.4f, 0.0f); // back beside Archmage Kalec
                    }
                }
    }
};

// Scenario 1101 director + narrator, bound to Azuregos (106699). Set active so its AI runs wherever the party is in the
// instance. Best-effort: it advances the on-screen scenario steps (0 free Azuregos -> 6 claim Aluneth) as the player
// pushes in and once Bilaal falls, but the actual quest completion is owned by npc_nexus_prince_bilaal::JustDied, so a
// missing / unattached scenario can never block the run. On Bilaal's death it plays the finale and rides the party home.
struct npc_azuregos_nexus_director : public ScriptedAI
{
    npc_azuregos_nexus_director(Creature* creature) : ScriptedAI(creature) { }

    uint32 _pollTimer = 0;
    bool   _introSaid = false;
    bool   _bossSeen  = false;
    bool   _bossDead  = false;
    bool   _finaleScheduled = false;

    void Reset() override
    {
        if (me->GetMap()->GetId() == MAP_NEXUS_VAULT)
            me->setActive(true);
    }

    void UpdateAI(uint32 diff) override
    {
        if (me->GetMap()->GetId() != MAP_NEXUS_VAULT)
            return;

        _pollTimer += diff;
        if (_pollTimer < 1000)
            return;
        _pollTimer = 0;

        Player* anyP = ArtifactPlayerBeyond(me, NexusVaultLanding, 0.0f); // any player on the instance
        if (!anyP)
            return;

        if (!_introSaid)
        {
            _introSaid = true;
            me->Say("Free at last! The Nexus-Prince guards the staff in the vault below. Fight your way to him, mage!", LANG_UNIVERSAL);
        }

        // Track Bilaal's death (his own AI owns the credits/completion; the director only reacts for presentation).
        if (me->FindNearestCreature(NPC_NEXUS_PRINCE_BILAAL, 500.0f, true))
            _bossSeen = true;
        else if (_bossSeen)
            _bossDead = true;

        // Best-effort on-screen scenario-step advance (no-ops if no scenario is attached to this map).
        switch (ArtifactScenarioOrder(me))
        {
            case 0: ArtifactAdvanceScenario(me); break;                                    // The Azure Prisoner
            case 1: if (ArtifactPlayerBeyond(me, NexusVaultLanding, 60.0f))  ArtifactAdvanceScenario(me); break; // Seeking Answers
            case 2: if (ArtifactPlayerBeyond(me, NexusVaultLanding, 120.0f)) ArtifactAdvanceScenario(me); break; // Echoes of Ancient Power
            case 3: if (ArtifactPlayerBeyond(me, NexusVaultLanding, 200.0f)) ArtifactAdvanceScenario(me); break; // The Way Out is Through
            case 4: if (_bossDead) ArtifactAdvanceScenario(me); break;                     // Consumed by Void (Bilaal)
            case 5: if (_bossDead) ArtifactAdvanceScenario(me); break;                     // Breaking and Binding
            case 6: if (_bossDead) ArtifactAdvanceScenario(me); break;                     // The Power of Aegwyn
            default: break;
        }

        // Authoritative finale: once Bilaal is down, seize Aluneth and ride home a few seconds later.
        if (_bossDead && !_finaleScheduled)
        {
            _finaleScheduled = true;
            me->Say("The staff is yours, mage. Aluneth, the Greatstaff of the Magna - wield it well. I shall return you to the Hall.", LANG_UNIVERSAL);
            for (auto const& ref : me->GetMap()->GetPlayers())
                if (Player* p = ref.GetSource())
                    if (p->IsInWorld())
                        p->m_Events.AddEventAtOffset(new MageReturnHomeEvent(p, MAP_NEXUS_VAULT, MAP_DALARAN_BROKEN_ISLE, NexusVaultHome), 6s);
            TC_LOG_INFO("scripts", "[Aluneth 1101] finale: Bilaal down -> credits granted, return to Dalaran scheduled");
        }
    }
};

// =====================================================================================================================
// Frost artifact: "The Mage Hunter" (42479) -> Ebonchill, Greatstaff of Alodi. Scenario 1122, map 1616.
// =====================================================================================================================
enum MageHunterData
{
    QUEST_THE_MAGE_HUNTER   = 42479,
    MAP_MAGE_HUNTER         = 1616,
    NPC_MERYL_NARRATOR      = 108097, // friendly Meryl Felstorm on map 1616 - the scenario director / narrator
    NPC_BALAADUR           = 108168, // the boss "Balaadur, the Hunter of Mages" (already faction 16 - hostile)
    CREDIT_FLIGHT_TO_SCENARIO = 108025, // 42479 obj0 "Kill Credit: Flight to Scenario" (granted on accept)
    CREDIT_WON_SCENARIO       = 108026, // 42479 obj1 "Kill Credit: Won Scenario" (granted on Balaadur's death)
    NPC_MERYL_ENDER         = 102700 // 42479 ender, in Dalaran (map 1220)
};

// Player landing on map 1616, near Balaadur at ~(-4404, 477, 434).
static constexpr Position MageHunterLanding = { -4360.0f, 477.0f, 434.0f, 3.14f };
// Where the party is returned in Dalaran (beside Meryl Felstorm 102700, the quest ender).
static constexpr Position MageHunterHome     = { -843.0f, 4432.0f, 742.5f, 2.60f };

// Accepting "The Mage Hunter" flies the player to the scenario ("Flight to Scenario", obj0 credit 108025); that
// scripted flight is absent from our world DB, so we grant the flight credit and transfer the player on accept.
class MageHunterFlightEvent : public BasicEvent
{
public:
    explicit MageHunterFlightEvent(Player* player) : _player(player) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (_player->IsInWorld())
        {
            _player->KilledMonsterCredit(CREDIT_FLIGHT_TO_SCENARIO); // obj0
            _player->TeleportTo(MAP_MAGE_HUNTER, MageHunterLanding.GetPositionX(), MageHunterLanding.GetPositionY(),
                MageHunterLanding.GetPositionZ(), MageHunterLanding.GetOrientation());
        }
        return true;
    }

private:
    Player* _player;
};

struct quest_the_mage_hunter : QuestScript
{
    quest_the_mage_hunter() : QuestScript("quest_the_mage_hunter") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus == QUEST_STATUS_INCOMPLETE) // just accepted -> fly to the scenario
            player->m_Events.AddEventAtOffset(new MageHunterFlightEvent(player), 1500ms);
    }
};

// Balaadur (108168): the Frost boss. Already faction 16 in the DB, but we (re)assert it in Reset() to be safe. His
// death grants obj1 (credit 108026 "Won Scenario") and completes 42479 - the authoritative completion point.
struct npc_balaadur : public ScriptedAI
{
    npc_balaadur(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        if (me->GetMap()->GetId() == MAP_MAGE_HUNTER)
            me->SetFaction(FACTION_MONSTER_2); // faction 16 - attackable
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        if (me->GetMap()->GetId() == MAP_MAGE_HUNTER)
            me->Yell("For your insolence, your end will be painful and slow!", LANG_UNIVERSAL);
    }

    void JustDied(Unit* /*killer*/) override
    {
        if (me->GetMap()->GetId() != MAP_MAGE_HUNTER)
            return;

        me->Yell("Your world... will... fall...", LANG_UNIVERSAL);

        for (auto const& ref : me->GetMap()->GetPlayers())
            if (Player* p = ref.GetSource())
                if (p->IsInWorld())
                {
                    p->KilledMonsterCredit(CREDIT_WON_SCENARIO); // obj1
                    if (p->GetQuestStatus(QUEST_THE_MAGE_HUNTER) == QUEST_STATUS_INCOMPLETE)
                        p->CompleteQuest(QUEST_THE_MAGE_HUNTER); // safety net: ready to hand in to Meryl
                }
    }
};

// Scenario 1122 director + narrator, bound to Meryl Felstorm (108097). Same best-effort pattern as Azuregos above:
// advances the on-screen steps (0 Preparations, 3 The Great Ritual, 4 Showdown, 5 Ebonchill) as the player pushes in
// and once Balaadur falls, then rides the party home. Completion is owned by npc_balaadur::JustDied.
struct npc_meryl_mage_hunter_director : public ScriptedAI
{
    npc_meryl_mage_hunter_director(Creature* creature) : ScriptedAI(creature) { }

    uint32 _pollTimer = 0;
    bool   _introSaid = false;
    bool   _bossSeen  = false;
    bool   _bossDead  = false;
    bool   _finaleScheduled = false;

    void Reset() override
    {
        if (me->GetMap()->GetId() == MAP_MAGE_HUNTER)
            me->setActive(true);
    }

    void UpdateAI(uint32 diff) override
    {
        if (me->GetMap()->GetId() != MAP_MAGE_HUNTER)
            return;

        _pollTimer += diff;
        if (_pollTimer < 1000)
            return;
        _pollTimer = 0;

        Player* anyP = ArtifactPlayerBeyond(me, MageHunterLanding, 0.0f);
        if (!anyP)
            return;

        if (!_introSaid)
        {
            _introSaid = true;
            me->Say("This is the Realm of Madness. Balaadur, the Hunter of Mages, holds Ebonchill. Strike him down and take it back!", LANG_UNIVERSAL);
        }

        if (me->FindNearestCreature(NPC_BALAADUR, 800.0f, true))
            _bossSeen = true;
        else if (_bossSeen)
            _bossDead = true;

        // Best-effort on-screen scenario-step advance (no-ops if no scenario is attached). Scenario 1122 authored
        // OrderIndexes are 0/3/4/5; we simply advance whatever the current step is toward the boss beat.
        switch (ArtifactScenarioOrder(me))
        {
            case 0: ArtifactAdvanceScenario(me); break;                                     // Preparations
            case 3: if (ArtifactPlayerBeyond(me, MageHunterLanding, 40.0f)) ArtifactAdvanceScenario(me); break; // The Great Ritual
            case 4: if (_bossDead) ArtifactAdvanceScenario(me); break;                       // Showdown (Balaadur)
            case 5: if (_bossDead) ArtifactAdvanceScenario(me); break;                       // Ebonchill
            default: if (_bossDead) ArtifactAdvanceScenario(me); break;
        }

        if (_bossDead && !_finaleScheduled)
        {
            _finaleScheduled = true;
            me->Say("Ebonchill is yours, Archmage. The Greatstaff of Alodi answers to you now. Let us return to the Hall.", LANG_UNIVERSAL);
            for (auto const& ref : me->GetMap()->GetPlayers())
                if (Player* p = ref.GetSource())
                    if (p->IsInWorld())
                        p->m_Events.AddEventAtOffset(new MageReturnHomeEvent(p, MAP_MAGE_HUNTER, MAP_DALARAN_BROKEN_ISLE, MageHunterHome), 6s);
            TC_LOG_INFO("scripts", "[Ebonchill 1122] finale: Balaadur down -> credit granted, return to Dalaran scheduled");
        }
    }
};

// =====================================================================================================================
// Fire artifact: "The Frozen Flame" (11997) -> Felo'melorn, Flamestrike of the Phoenix. Icecrown, map 1480 (shared with
// the DK Frost scenario area). Quest 11997 IS in the world DB with two objectives - obj0 credit 99418 "Aethas's Portal"
// (Mage Portal Taken, granted on accept) + obj1 credit 100290 "Flame Bunny" (Obtain Felo'melorn, granted on Lyandra's
// death) - but it had NO quest-giver and no boss AI, so it was unobtainable. The SQL adds Meryl Felstorm (102700 /
// 109222, already spawned in the Hall of the Guardian) as giver+ender - mirroring Frost's "The Mage Hunter" - and binds
// the Lyandra boss AI. On accept we grant the portal credit and drop the player beside Lyandra in Icecrown; her death
// grants the Felo'melorn credit + completes 11997, then returns the player to Meryl.
//
// LIMITATION: map 1480 is shared with the DK Frost scenario; Lyandra sits ~200y from the DK content so they don't
// normally interfere, but Fire + DK-Frost run simultaneously on 1480 could see each other's actors. The Aethas Spellbind
// / Gorewing-frees-Lyandra mid-fight beats are creature dialogue here, not a scripted phase (no Scene for Fire).
// =====================================================================================================================
enum FrozenFlameData
{
    QUEST_THE_FROZEN_FLAME   = 11997,
    MAP_ICECROWN_FIRE        = 1480,
    NPC_LYANDRA_SUNSTRIDER   = 99615,  // the boss (already faction 16 - hostile)
    CREDIT_MAGE_PORTAL_TAKEN = 99418,  // 11997 obj0 (granted on accept)
    CREDIT_FELOMELORN_LOOTED = 100290  // 11997 obj1 (granted on Lyandra's death)
};

// Player landing on map 1480 beside Lyandra Sunstrider (~4573, 2769, 361).
static constexpr Position FrozenFlameLanding = { 4558.0f, 2769.0f, 361.3f, 0.0f };
// Return beside Meryl Felstorm (102700) in Dalaran (the quest ender) - same spot as the Frost return.
static constexpr Position FrozenFlameHome    = { -843.0f, 4432.0f, 742.5f, 2.60f };

// Accepting "The Frozen Flame" opens Aethas's portal to Icecrown (obj0 credit 99418) and drops the player beside Lyandra.
class FrozenFlamePortalEvent : public BasicEvent
{
public:
    explicit FrozenFlamePortalEvent(Player* player) : _player(player) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (_player->IsInWorld())
        {
            _player->KilledMonsterCredit(CREDIT_MAGE_PORTAL_TAKEN); // obj0
            _player->TeleportTo(MAP_ICECROWN_FIRE, FrozenFlameLanding.GetPositionX(), FrozenFlameLanding.GetPositionY(),
                FrozenFlameLanding.GetPositionZ(), FrozenFlameLanding.GetOrientation());
        }
        return true;
    }

private:
    Player* _player;
};

struct quest_the_frozen_flame : QuestScript
{
    quest_the_frozen_flame() : QuestScript("quest_the_frozen_flame") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus == QUEST_STATUS_INCOMPLETE) // just accepted -> portal to Icecrown
            player->m_Events.AddEventAtOffset(new FrozenFlamePortalEvent(player), 1500ms);
    }
};

// Lyandra Sunstrider (99615): the Fire boss in Icecrown. Already faction 16; (re)asserted in Reset(). Her death grants
// obj1 (credit 100290 "Obtain Felo'melorn") + completes 11997 (the authoritative completion point) and rides the player
// home to Meryl. Bound to 99615 via creature_template.ScriptName.
struct npc_lyandra_sunstrider : public ScriptedAI
{
    npc_lyandra_sunstrider(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        if (me->GetMap()->GetId() == MAP_ICECROWN_FIRE)
            me->SetFaction(FACTION_MONSTER_2); // faction 16 - attackable
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        if (me->GetMap()->GetId() == MAP_ICECROWN_FIRE)
            me->Yell("This ends now!", LANG_UNIVERSAL);
    }

    void JustDied(Unit* /*killer*/) override
    {
        if (me->GetMap()->GetId() != MAP_ICECROWN_FIRE)
            return;

        me->Yell("So... cold... Felo'melorn is yours. Take it.", LANG_UNIVERSAL);

        for (auto const& ref : me->GetMap()->GetPlayers())
            if (Player* p = ref.GetSource())
                if (p->IsInWorld())
                {
                    p->KilledMonsterCredit(CREDIT_FELOMELORN_LOOTED); // obj1
                    if (p->GetQuestStatus(QUEST_THE_FROZEN_FLAME) == QUEST_STATUS_INCOMPLETE)
                    {
                        p->CompleteQuest(QUEST_THE_FROZEN_FLAME); // safety net: ready to hand in to Meryl
                        p->m_Events.AddEventAtOffset(new MageReturnHomeEvent(p, MAP_ICECROWN_FIRE, MAP_DALARAN_BROKEN_ISLE, FrozenFlameHome), 6s);
                    }
                }
    }
};

void AddSC_artifact_mage()
{
    // Quest
    new quest_the_nexus_vault();
    new quest_the_mage_hunter();
    new quest_the_frozen_flame();               // 11997 Fire

    // Creature
    RegisterCreatureAI(npc_nexus_prince_bilaal);
    RegisterCreatureAI(npc_azuregos_nexus_director);
    RegisterCreatureAI(npc_balaadur);
    RegisterCreatureAI(npc_meryl_mage_hunter_director);
    RegisterCreatureAI(npc_lyandra_sunstrider); // 99615 Fire boss
}