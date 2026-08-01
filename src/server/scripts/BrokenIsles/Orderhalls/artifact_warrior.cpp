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

// Warrior Legion artifact-acquisition scripts (all three specs). Mirrors the proven Beast Mastery Hunter file
// (zone_orderhall_hunter.cpp) and reuses the shared helpers in orderhall_artifact_common.h.
//
// All three acquisition quests start AND end at Odyn (96469) in Skyhold (map 1479). Two of the three are pure
// kill-credit quests; the Arms quest is a scenario quest with a dead game-event criterion we force with CompleteQuest.
//
//   Arms  "The Sword of Kings"        (41105) -> Strom'kar, the Warbreaker (128910). Scenario 1037, map 1539
//         (Tomb of Tyr). Objectives: [0] speak Aerylia (credit 160047), [1] portal (102394)+reached Tirisfal (113105),
//         [2] Type-14 CriteriaTree 217561 "Strom'kar claimed" (dead game-event -> forced via CompleteQuest),
//         [3] won scenario (103114), [4] returned to Valhallas (103739). Director on Thoradin (103144) drives the
//         on-screen scenario steps; Zakajz (104276) death runs the finale.
//
//   Fury  "The Hunter of Heroes"      (40043) -> Warswords of the Valarjar (128908). Map 1511 (Tideskorn Harbor).
//         Pure credits: [0] 98032 (entry), [1] 98033 (claim), [2] 103739 (return). Finale on Vigfus (98602) death.
//
//   Prot  "Legacy of the Icebreaker"  (39191) -> Scale of the Earth-Warder (128289). Map 1495 (Shield's Rest).
//         Pure credits: [0] 96508 (met Hruthnir), [1] 94851 (claim), [2] 103739 (return). Finale on Magnar (96034)
//         death.
//
// The kill-credit specs complete purely on their three Type-0 credits (no scenario criteria needed), so a QuestScript
// entry hook (grants the entry credit + teleports into the map) plus a boss ScriptedAI (hostile in Reset, grants the
// claim credit + weapon and schedules the return in JustDied) is enough to make the quest turn-in-able.

#include "Creature.h"
#include "EventProcessor.h"
#include "GameObject.h"
#include "GameObjectAI.h"
#include "Log.h"
#include "Map.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "QuestDef.h"
#include "Scenario.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"
#include "SharedDefines.h"
#include "TemporarySummon.h"
#include "Unit.h"
#include "orderhall_artifact_common.h"

// Skyhold (the class hall) return point + Odyn's questgiver spot; shared by all three specs.
enum WarriorArtifactShared
{
    MAP_SKYHOLD                 = 1479,
    NPC_ODYN                    = 96469,
    CREDIT_RETURNED_VALHALLAS   = 103739, // "Returned to Valhallas" - the final credit on every spec's quest
    SPELL_ARTIFACT_RETURN       = 192085  // the generic "return home" cinematic spell used by Fury/Prot at claim
};

static constexpr Position SkyholdReturn = { 1071.81f, 7224.32f, 97.891f, 3.90f }; // beside Odyn in Skyhold

// ---------------------------------------------------------------------------------------------------------------------
// Shared return-home event. Casts an optional cinematic spell, grants the "Returned to Valhallas" credit (which is the
// last objective on every spec's quest), optionally force-completes a scenario quest whose criteria cannot fire, and
// teleports the player back to Skyhold to hand in to Odyn.
class ReturnToSkyholdEvent : public BasicEvent
{
public:
    ReturnToSkyholdEvent(Player* player, uint32 returnSpell, uint32 completeQuestId)
        : _player(player), _returnSpell(returnSpell), _completeQuestId(completeQuestId) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (_player->IsInWorld())
        {
            if (_returnSpell)
                _player->CastSpell(_player, _returnSpell, true);
            _player->KilledMonsterCredit(CREDIT_RETURNED_VALHALLAS);
            // Force the whole quest complete for scenario quests carrying a dead Type-14 criterion; harmless for the
            // credit-only specs (they will already be at COMPLETE from their three credits).
            if (_completeQuestId && _player->GetQuestStatus(_completeQuestId) == QUEST_STATUS_INCOMPLETE)
                _player->CompleteQuest(_completeQuestId);
            _player->TeleportTo(MAP_SKYHOLD, SkyholdReturn.GetPositionX(), SkyholdReturn.GetPositionY(),
                SkyholdReturn.GetPositionZ(), SkyholdReturn.GetOrientation());
        }
        return true;
    }

private:
    Player* _player;
    uint32  _returnSpell;
    uint32  _completeQuestId;
};

// =====================================================================================================================
// ARMS: "The Sword of Kings" (41105) -> Strom'kar, the Warbreaker (128910). Scenario 1037 on map 1539 (Tomb of Tyr).
// =====================================================================================================================
enum ArmsArtifact
{
    QUEST_THE_SWORD_OF_KINGS    = 41105,
    MAP_TOMB_OF_TYR             = 1539,
    NPC_THORADIN                = 103144, // ghost-king ally + scenario director
    NPC_ZAKAJZ_CORRUPTOR        = 104276, // C'Thraxxi final boss (template faction 14)
    NPC_SOTHOZZ_GUARDIAN        = 104591, // mini-boss - NOT statically spawned, director summons it hostile
    CREDIT_SPOKE_AERYLIA        = 160047, // objective 0 ObjectID (granted at accept)
    CREDIT_TOOK_PORTAL          = 102394, // objective 1 ObjectID
    CREDIT_REACHED_TIRISFAL     = 113105, // objective 1 ObjectID
    CREDIT_WON_SCENARIO         = 103114, // objective 3 ObjectID
    ITEM_STROMKAR               = 128910
};

static constexpr Position TombLanding   = { 2131.32f, 2423.56f, 122.278f, 3.90f }; // beside Thoradin (entry hub)
static constexpr Position SothozzSpawn  = { 2000.00f, 2290.00f,  80.000f, 2.30f }; // mid-tomb, on the path to Zakajz

// Teleport the player into the Tomb of Tyr and grant the portal/arrival credits (objective 1).
class SwordOfKingsPortalEvent : public BasicEvent
{
public:
    explicit SwordOfKingsPortalEvent(Player* player) : _player(player) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (_player->IsInWorld())
        {
            _player->TeleportTo(MAP_TOMB_OF_TYR, TombLanding.GetPositionX(), TombLanding.GetPositionY(),
                TombLanding.GetPositionZ(), TombLanding.GetOrientation());
            ArtifactGrantCredits(_player, { CREDIT_TOOK_PORTAL, CREDIT_REACHED_TIRISFAL }); // objective 1
        }
        return true;
    }

private:
    Player* _player;
};

struct quest_the_sword_of_kings : QuestScript
{
    quest_the_sword_of_kings() : QuestScript("quest_the_sword_of_kings") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus == QUEST_STATUS_INCOMPLETE) // accepted from Odyn after speaking with Aerylia
        {
            player->KilledMonsterCredit(CREDIT_SPOKE_AERYLIA);                            // objective 0
            player->m_Events.AddEventAtOffset(new SwordOfKingsPortalEvent(player), 1500ms);
        }
    }
};

// Scenario 1037 director, bound to Thoradin (103144). Set active in Reset() so his AI runs wherever the party is. He
// speaks an intro, then advances the on-screen scenario step (via ArtifactAdvanceScenario - the game-event criteria do
// not fire on this server) as the player pushes deeper into the tomb, and summons Soth'ozz hostile mid-run. The finale
// (Zakajz death) is handled authoritatively by npc_zakajz_corruptor, so a scenario hiccup can never strand the run.
struct npc_thoradin_scenario_director : public ScriptedAI
{
    npc_thoradin_scenario_director(Creature* creature) : ScriptedAI(creature) { }

    uint32 _pollTimer = 0;
    uint32 _introTimer = 0;
    uint8  _introLine = 0;
    bool   _introDone = false;
    uint8  _gate = 0;
    bool   _sothSummoned = false;

    void Reset() override
    {
        if (me->GetMap()->GetId() == MAP_TOMB_OF_TYR)
            me->setActive(true);
    }

    void SaySelf(char const* text) { me->Say(text, LANG_UNIVERSAL); }

    void UpdateAI(uint32 diff) override
    {
        if (me->GetMap()->GetId() != MAP_TOMB_OF_TYR)
            return;

        _pollTimer += diff;
        if (_pollTimer < 1000)
            return;
        _pollTimer = 0;

        Player* anyP = ArtifactPlayerBeyond(me, TombLanding, 0.0f); // any player in the tomb
        if (!anyP)
            return;

        // --- Intro exchange at the landing, then mark scenario step 0 done. ---
        if (!_introDone)
        {
            _introTimer += 1000;
            switch (_introLine)
            {
                case 0: SaySelf("So... a mortal comes to claim Strom'kar. The blade of the King of Stormheim will test you."); ++_introLine; break;
                case 1: if (_introTimer >= 4000) { SaySelf("The corruptor Zakajz festers within this tomb. Free my blade, and it is yours."); ++_introLine; } break;
                case 2: if (_introTimer >= 8000) { SaySelf("Follow me into the depths. Steel yourself."); ++_introLine; } break;
                default:
                    _introDone = true;
                    ArtifactAdvanceScenario(me); // scenario step 0 complete
                    break;
            }
            return;
        }

        // --- Advance the scenario one step each time the player pushes past the next distance gate. ---
        static constexpr float gates[] = { 60.0f, 140.0f, 220.0f, 300.0f };
        if (_gate < 4 && ArtifactPlayerBeyond(me, TombLanding, gates[_gate]))
        {
            ++_gate;
            ArtifactAdvanceScenario(me);

            if (_gate == 2)
                SaySelf("The air grows thick with the whispers of the Old Gods. Stay close.");

            // Step "The Root of the Corruption": summon Soth'ozz the Guardian hostile (it is not statically spawned).
            if (_gate == 3 && !_sothSummoned)
            {
                _sothSummoned = true;
                SaySelf("Beware! Soth'ozz, the root of this corruption, stirs!");
                if (Creature* soth = me->SummonCreature(NPC_SOTHOZZ_GUARDIAN, SothozzSpawn, TEMPSUMMON_MANUAL_DESPAWN))
                {
                    soth->SetFaction(FACTION_MONSTER_2); // 16 - hostile
                    if (Player* target = ArtifactPlayerBeyond(me, TombLanding, gates[2]))
                        soth->AI()->AttackStart(target);
                }
            }
        }
    }
};

// Zakajz the Corruptor (104276) - the Arms final boss. Hostile in Reset(); his death grants "won scenario" + Strom'kar
// and schedules the return to Skyhold (which force-completes the quest's dead Type-14 criterion via CompleteQuest).
struct npc_zakajz_corruptor : public ScriptedAI
{
    npc_zakajz_corruptor(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        if (me->GetMap()->GetId() == MAP_TOMB_OF_TYR)
            me->SetFaction(FACTION_MONSTER_2); // 16 - hostile (template is faction 14; enforce here regardless)
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        if (me->GetMap()->GetId() == MAP_TOMB_OF_TYR)
            me->Yell("Flesh-thing! The blade of Tyr will not save you from the void!", LANG_UNIVERSAL);
    }

    void JustDied(Unit* /*killer*/) override
    {
        if (me->GetMap()->GetId() != MAP_TOMB_OF_TYR)
            return;

        me->Yell("The whispers... they fade...", LANG_UNIVERSAL);

        // Advance the scenario onto its final beats (the on-screen presentation) - safe no-ops if already complete.
        ArtifactAdvanceScenario(me);
        ArtifactAdvanceScenario(me);

        for (auto const& ref : me->GetMap()->GetPlayers())
            if (Player* p = ref.GetSource())
                if (p->IsInWorld())
                {
                    if (p->GetQuestStatus(QUEST_THE_SWORD_OF_KINGS) == QUEST_STATUS_INCOMPLETE)
                    {
                        p->KilledMonsterCredit(CREDIT_WON_SCENARIO);   // objective 3
                        p->AddItem(ITEM_STROMKAR, 1);                  // Strom'kar, the Warbreaker
                        // Return after the finale beats; CompleteQuest there force-satisfies the Type-14 objective 2.
                        p->m_Events.AddEventAtOffset(new ReturnToSkyholdEvent(p, 0, QUEST_THE_SWORD_OF_KINGS), 10s);
                    }
                    // Priest Shadow "Blade in Twilight" (40710) shares this boss + map (1539, Tomb of Tyr / Tirisfal).
                    if (p->GetQuestStatus(40710) == QUEST_STATUS_INCOMPLETE)
                    {
                        p->CompleteQuest(40710);
                        p->TeleportTo(1512, 1333.9f, 1335.6f, 177.2f, 3.1f); // back to Netherlight Temple (Priest hall)
                    }
                }
    }
};

// =====================================================================================================================
// FURY: "The Hunter of Heroes" (40043) -> Warswords of the Valarjar (128908). Map 1511 (Tideskorn Harbor).
// =====================================================================================================================
enum FuryArtifact
{
    QUEST_THE_HUNTER_OF_HEROES  = 40043,
    MAP_TIDESKORN_HARBOR        = 1511,
    NPC_VIGFUS_FINAL            = 98602,  // Vigfus Bladewind - final form in the shallows (template faction 16)
    CREDIT_FURY_ENTRY           = 98032,  // objective 0
    CREDIT_FURY_CLAIM           = 98033,  // objective 1 (Warswords claimed)
    ITEM_WARSWORDS              = 128908,
    SPELL_FURY_ARTIFACT_GRANT   = 205443
};

static constexpr Position TideskornLanding = { 3386.46f, 1406.94f, 68.94f, 3.90f }; // start ledge (LegionCore MoveJump)

class HunterOfHeroesEntryEvent : public BasicEvent
{
public:
    explicit HunterOfHeroesEntryEvent(Player* player) : _player(player) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (_player->IsInWorld())
        {
            _player->TeleportTo(MAP_TIDESKORN_HARBOR, TideskornLanding.GetPositionX(), TideskornLanding.GetPositionY(),
                TideskornLanding.GetPositionZ(), TideskornLanding.GetOrientation());
            _player->KilledMonsterCredit(CREDIT_FURY_ENTRY); // objective 0
        }
        return true;
    }

private:
    Player* _player;
};

struct quest_the_hunter_of_heroes : QuestScript
{
    quest_the_hunter_of_heroes() : QuestScript("quest_the_hunter_of_heroes") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus == QUEST_STATUS_INCOMPLETE)
            player->m_Events.AddEventAtOffset(new HunterOfHeroesEntryEvent(player), 1500ms);
    }
};

// Vigfus Bladewind's final form (98602). His death grants the Warswords claim credit + weapon and schedules the return.
struct npc_vigfus_bladewind_final : public ScriptedAI
{
    npc_vigfus_bladewind_final(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        if (me->GetMap()->GetId() == MAP_TIDESKORN_HARBOR)
            me->SetFaction(FACTION_MONSTER_2); // 16 - hostile
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        if (me->GetMap()->GetId() == MAP_TIDESKORN_HARBOR)
            me->Yell("The Warswords are mine! Helya promised me power beyond death!", LANG_UNIVERSAL);
    }

    void JustDied(Unit* /*killer*/) override
    {
        if (me->GetMap()->GetId() != MAP_TIDESKORN_HARBOR)
            return;

        for (auto const& ref : me->GetMap()->GetPlayers())
            if (Player* p = ref.GetSource())
                if (p->IsInWorld() && p->GetQuestStatus(QUEST_THE_HUNTER_OF_HEROES) == QUEST_STATUS_INCOMPLETE)
                {
                    p->KilledMonsterCredit(CREDIT_FURY_CLAIM);              // objective 1 (Warswords claimed)
                    p->AddItem(ITEM_WARSWORDS, 1);
                    p->CastSpell(p, SPELL_FURY_ARTIFACT_GRANT, true);
                    p->m_Events.AddEventAtOffset(new ReturnToSkyholdEvent(p, SPELL_ARTIFACT_RETURN, 0), 8s);
                }
    }
};

// =====================================================================================================================
// PROTECTION: "Legacy of the Icebreaker" (39191) -> Scale of the Earth-Warder (128289). Map 1495 (Shield's Rest).
// =====================================================================================================================
enum ProtArtifact
{
    QUEST_LEGACY_OF_ICEBREAKER  = 39191,
    MAP_SHIELDS_REST            = 1495,
    NPC_MAGNAR_ICEBREAKER       = 96034,  // boss (template faction 35 -> made hostile here)
    NPC_HRUTHNIR                = 96468,  // ally escort (template faction 16 -> kept friendly here)
    CREDIT_PROT_MET_HRUTHNIR    = 96508,  // objective 0
    CREDIT_PROT_CLAIM           = 94851,  // objective 1 (Scale claimed)
    ITEM_SCALE_EARTHWARDER      = 128289,
    SPELL_PROT_ARTIFACT_GRANT   = 205340
};

static constexpr Position ShieldsRestLanding = { 4700.21f, 352.016f, -37.49f, 3.90f }; // Magnar's arena / Scale GO area

class LegacyIcebreakerEntryEvent : public BasicEvent
{
public:
    explicit LegacyIcebreakerEntryEvent(Player* player) : _player(player) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (_player->IsInWorld())
        {
            _player->TeleportTo(MAP_SHIELDS_REST, ShieldsRestLanding.GetPositionX(), ShieldsRestLanding.GetPositionY(),
                ShieldsRestLanding.GetPositionZ(), ShieldsRestLanding.GetOrientation());
            _player->KilledMonsterCredit(CREDIT_PROT_MET_HRUTHNIR); // objective 0 (met Hruthnir at the tomb)
        }
        return true;
    }

private:
    Player* _player;
};

struct quest_legacy_of_the_icebreaker : QuestScript
{
    quest_legacy_of_the_icebreaker() : QuestScript("quest_legacy_of_the_icebreaker") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus == QUEST_STATUS_INCOMPLETE)
            player->m_Events.AddEventAtOffset(new LegacyIcebreakerEntryEvent(player), 1500ms);
    }
};

// Magnar Icebreaker (96034) - the Prot boss. Template faction 35 placeholder; made hostile in Reset(). His death grants
// the Scale claim credit + weapon and schedules the return.
struct npc_magnar_icebreaker : public ScriptedAI
{
    npc_magnar_icebreaker(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        if (me->GetMap()->GetId() == MAP_SHIELDS_REST)
            me->SetFaction(FACTION_MONSTER_2); // 16 - hostile
    }

    void JustEngagedWith(Unit* /*who*/) override
    {
        if (me->GetMap()->GetId() == MAP_SHIELDS_REST)
            me->Yell("The Scale of the Earth-Warder is not for the likes of you! I am the greatest warrior of the Valarjar!", LANG_UNIVERSAL);
    }

    void JustDied(Unit* /*killer*/) override
    {
        if (me->GetMap()->GetId() != MAP_SHIELDS_REST)
            return;

        for (auto const& ref : me->GetMap()->GetPlayers())
            if (Player* p = ref.GetSource())
                if (p->IsInWorld() && p->GetQuestStatus(QUEST_LEGACY_OF_ICEBREAKER) == QUEST_STATUS_INCOMPLETE)
                {
                    p->KilledMonsterCredit(CREDIT_PROT_CLAIM);             // objective 1 (Scale claimed)
                    p->AddItem(ITEM_SCALE_EARTHWARDER, 1);
                    p->CastSpell(p, SPELL_PROT_ARTIFACT_GRANT, true);
                    p->m_Events.AddEventAtOffset(new ReturnToSkyholdEvent(p, SPELL_ARTIFACT_RETURN, 0), 8s);
                }
    }
};

// Hruthnir (96468) - the Prot escort ally. Template faction 16 (hostile) is a data bug for an ally; keep him friendly so
// he does not attack the player. (We do NOT edit creature_template.faction; we correct it at runtime here.)
struct npc_hruthnir_escort : public ScriptedAI
{
    npc_hruthnir_escort(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override
    {
        if (me->GetMap()->GetId() == MAP_SHIELDS_REST)
            me->SetFaction(35); // neutral/friendly - an ally must not be hostile faction 16
    }
};

void AddSC_artifact_warrior()
{
    // Quests
    new quest_the_sword_of_kings();
    new quest_the_hunter_of_heroes();
    new quest_legacy_of_the_icebreaker();

    // Creatures
    RegisterCreatureAI(npc_thoradin_scenario_director);
    RegisterCreatureAI(npc_zakajz_corruptor);
    RegisterCreatureAI(npc_vigfus_bladewind_final);
    RegisterCreatureAI(npc_magnar_icebreaker);
    RegisterCreatureAI(npc_hruthnir_escort);
}
