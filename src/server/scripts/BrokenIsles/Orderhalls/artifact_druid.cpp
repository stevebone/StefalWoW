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

// Druid Legion artifact acquisition. Per-class Broken Isles content (cf. zone_orderhall_hunter.cpp); the generic
// class-hall framework lives in orderhall_legion.cpp.
//
// None of the four druid artifacts use a Type-14 InstanceScenario - all are open-world kill-credit / travel questlines
// (verified against the class-hall data in integ_world), so every spec here follows the simple Hunter-MM/SV pattern:
// a QuestScript bound to the acquisition quest that, on accept (QUEST_STATUS_INCOMPLETE), schedules a BasicEvent which
// grants the flight / travel / gossip kill-credits and teleports the player to the encounter, plus a small ScriptedAI
// on any placeholder-faction boss to make it hostile and grant its "weapon claimed" follow-on credit on death.
//
//   Balance  - "The Scythe of Elune" (40783) -> "Its Rightful Place" (40784) -> Scythe of Elune. Travel to Duskwood,
//              meet Valorn (101656), reclaim the scythe at Raven Hill (credit 101702). No combat.
//   Feral    - "The Shrine of Ashamane" (42428) / "Aid for the Ashen" (42439) / "The Shrine in Peril" (42440) ->
//              "The Fangs of Ashamane" (42430) -> Fangs of Ashamane. Talk Danise, slay Verstok, cleanse the shrine,
//              then defeat Ebonfang (107729) to claim the fangs (credit 107750). Placeholder mobs made hostile here.
//   Restoration - "Cleansing the Mother Tree" (41689) -> G'Hanir. A single standalone quest driven by three travel /
//              event credits (enter Dreamway 104608 -> travel to Hyjal 104613 -> cleanse 114511). No combat.
//   Guardian - "Claws of Ursoc": the quest chain is ABSENT from integ_world (no quest_template / objectives / NPCs).
//              It cannot be scripted without first authoring the full quest data, which is outside a script file's
//              scope. See the risks note in the delivery.
//
// FACTION_MONSTER_2 (16) is applied in Reset() to the Feral bosses/adds, whose creature_template faction is the
// friendly placeholder 35; we never edit creature_template.faction (a fresh spawn re-reads it), we override in AI.

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
#include "SpellScript.h"
#include "orderhall_artifact_common.h"

// =====================================================================================================================
// Balance artifact: Scythe of Elune.
//   40783 "The Scythe of Elune"  (Naralex 103778 -> Valorn Stillbough 101656)
//        obj0 103585 "Travel through the Dreamway to Duskwood"   (travel credit)
//        obj1 101701 "Credit - Valorn Met"                       (gossip credit)
//   40784 "Its Rightful Place"   (Valorn Stillbough 101656 -> Belysra Starbreeze 101651)
//        obj0 101702 "Credit - Scythe Taken"                     (reclaim the scythe at Raven Hill; weapon awarded on
//                                                                  turn-in to Belysra)
// All world NPCs already exist on maps 1220/0/1540; only the travel/gossip credits + the cross-map hop are scripted.
// =====================================================================================================================
enum ScytheOfEluneData
{
    QUEST_SCYTHE_OF_ELUNE       = 40783,
    QUEST_ITS_RIGHTFUL_PLACE    = 40784,
    NPC_CREDIT_DREAM_TRAVEL     = 103585, // 40783 obj0
    NPC_CREDIT_VALORN_MET       = 101701, // 40783 obj1
    NPC_CREDIT_SCYTHE_TAKEN     = 101702, // 40784 obj0
    MAP_EASTERN_KINGDOMS        = 0
};

// Duskwood / Raven Hill - beside Valorn Stillbough (101656) and Belysra Starbreeze (101651), who stand together.
static constexpr Position DuskwoodValorn  = { -10330.3f, -489.528f, 50.2545f, 2.82315f };
static constexpr Position DuskwoodBelysra = { -10330.9f, -485.015f, 49.7964f, 3.51351f };

// 40783: accept from Naralex in the Dreamgrove -> travel through the Emerald Dreamway to Duskwood (obj0), where meeting
// Valorn (obj1) closes the quest. We credit both and drop the player beside Valorn so 40783 is ready to hand in.
class ScytheDreamwayTravelEvent : public BasicEvent
{
public:
    explicit ScytheDreamwayTravelEvent(Player* player) : _player(player) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (_player->IsInWorld())
        {
            ArtifactGrantCredits(_player, { NPC_CREDIT_DREAM_TRAVEL, NPC_CREDIT_VALORN_MET });
            _player->TeleportTo(MAP_EASTERN_KINGDOMS, DuskwoodValorn.GetPositionX(), DuskwoodValorn.GetPositionY(),
                DuskwoodValorn.GetPositionZ(), DuskwoodValorn.GetOrientation());
        }
        return true;
    }

private:
    Player* _player;
};

struct quest_scythe_of_elune : QuestScript
{
    quest_scythe_of_elune() : QuestScript("quest_scythe_of_elune") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus == QUEST_STATUS_INCOMPLETE) // just accepted -> travel to Duskwood
            player->m_Events.AddEventAtOffset(new ScytheDreamwayTravelEvent(player), 1500ms);
    }
};

// 40784: Valorn sends the druid to reclaim the Scythe of Elune at Raven Hill Cemetery. Interacting with the scythe
// grants "Credit - Scythe Taken" (101702) and closes the quest; the Scythe of Elune is awarded on turn-in to Belysra.
// We grant the credit and step the player over to Belysra (a few yards away) so the quest can be handed in.
class ScytheReclaimEvent : public BasicEvent
{
public:
    explicit ScytheReclaimEvent(Player* player) : _player(player) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (_player->IsInWorld())
        {
            _player->KilledMonsterCredit(NPC_CREDIT_SCYTHE_TAKEN); // 40784 obj0
            _player->TeleportTo(MAP_EASTERN_KINGDOMS, DuskwoodBelysra.GetPositionX(), DuskwoodBelysra.GetPositionY(),
                DuskwoodBelysra.GetPositionZ(), DuskwoodBelysra.GetOrientation());
        }
        return true;
    }

private:
    Player* _player;
};

struct quest_its_rightful_place : QuestScript
{
    quest_its_rightful_place() : QuestScript("quest_its_rightful_place") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus == QUEST_STATUS_INCOMPLETE) // just accepted -> reclaim the scythe
            player->m_Events.AddEventAtOffset(new ScytheReclaimEvent(player), 1500ms);
    }
};

// =====================================================================================================================
// Feral artifact: Fangs of Ashamane. The whole chain is phased open-world in Val'sharah (map 1220), around the Shrine
// of Ashamane just south-west of the Dreamgrove.
//
//   42428 "The Shrine of Ashamane"  (Rensar 101195 -> Delandros Shimmermoon 107392)
//        obj0 107457 "Danise Stargazer"      (talk/rescue credit; she is spawned)
//        obj1 107520 "Verstok Darkbough"     (KILL; faction 2850, already hostile)
//   42439 "Aid for the Ashen"        (Delandros 107392, both ends)
//        obj0 107535 "Eredar Soul Lasher" x4 (KILL; faction-35 placeholder -> made hostile)
//   42440 "The Shrine in Peril"      (Delandros 107392, both ends)
//        obj0 107607 "Investigate Shrine Kill Credit", obj1 114768 "Algromon", obj2 107708 "Little Bunny" (bridge)
//   42430 "The Fangs of Ashamane"    (Delandros 107392 -> Rensar 101195)
//        obj0 107729 "Ebonfang"                       (KILL boss; faction-35 placeholder -> made hostile)
//        obj1 107750 "Kill Credit: Fangs of Ashamane" (claim the fangs; granted on Ebonfang's death)
//
// Delandros (107392, the chain's quest-giver), Verstok, Ebonfang and the Eredar Soul Lashers are NOT spawned in
// integ_world - the SQL adds them at the shrine. Verstok's faction (2850) is already hostile; Ebonfang and the lashers
// are placeholder faction 35 and are made hostile in AI below.
// =====================================================================================================================
enum FangsOfAshamaneData
{
    QUEST_SHRINE_OF_ASHAMANE    = 42428,
    QUEST_SHRINE_IN_PERIL       = 42440,
    QUEST_FANGS_OF_ASHAMANE     = 42430,
    NPC_CREDIT_DANISE           = 107457, // 42428 obj0
    NPC_EREDAR_SOUL_LASHER      = 107535, // 42439 obj0 (x4)
    NPC_EBONFANG                = 107729, // 42430 obj0 (boss)
    NPC_CREDIT_FANGS            = 107750, // 42430 obj1
    NPC_CREDIT_INVESTIGATE      = 107607, // 42440 obj0
    NPC_CREDIT_ALGROMON         = 114768, // 42440 obj1
    NPC_CREDIT_LITTLE_BUNNY     = 107708, // 42440 obj2
    MAP_VALSHARAH               = 1220
};

// Shrine of Ashamane, Val'sharah (map 1220) - anchored on Danise Stargazer's spawn (4075.33, 7243.44, 52.23). The SQL
// places Delandros/Verstok/Ebonfang/Lashers around this point.
static constexpr Position ShrineOfAshamane = { 4078.0f, 7245.0f, 52.2f, 4.20f };
static constexpr Position EbonfangArena    = { 4090.0f, 7228.0f, 52.0f, 3.10f };

// 42428: accept from Rensar in the Dreamgrove -> travel to the Shrine of Ashamane. Danise's "met" credit (obj0) is a
// talk credit we grant on arrival; Verstok (obj1) is a real kill of the spawned NPC (auto-credited on his death).
class ShrineOfAshamaneTravelEvent : public BasicEvent
{
public:
    explicit ShrineOfAshamaneTravelEvent(Player* player) : _player(player) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (_player->IsInWorld())
        {
            _player->KilledMonsterCredit(NPC_CREDIT_DANISE); // 42428 obj0
            _player->TeleportTo(MAP_VALSHARAH, ShrineOfAshamane.GetPositionX(), ShrineOfAshamane.GetPositionY(),
                ShrineOfAshamane.GetPositionZ(), ShrineOfAshamane.GetOrientation());
        }
        return true;
    }

private:
    Player* _player;
};

struct quest_shrine_of_ashamane : QuestScript
{
    quest_shrine_of_ashamane() : QuestScript("quest_shrine_of_ashamane") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus == QUEST_STATUS_INCOMPLETE) // just accepted -> travel to the shrine
            player->m_Events.AddEventAtOffset(new ShrineOfAshamaneTravelEvent(player), 1500ms);
    }
};

// 42440 "The Shrine in Peril": a bridge quest whose three objectives are an investigate credit (107607), Algromon
// (114768) and a Little Bunny travel credit (107708) - none are combat and none are spawned. We grant all three shortly
// after accept so the quest hands back to Delandros, keeping the Feral chain moving toward 42430.
class ShrineInPerilEvent : public BasicEvent
{
public:
    explicit ShrineInPerilEvent(Player* player) : _player(player) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (_player->IsInWorld())
            ArtifactGrantCredits(_player, { NPC_CREDIT_INVESTIGATE, NPC_CREDIT_ALGROMON, NPC_CREDIT_LITTLE_BUNNY });
        return true;
    }

private:
    Player* _player;
};

struct quest_shrine_in_peril : QuestScript
{
    quest_shrine_in_peril() : QuestScript("quest_shrine_in_peril") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus == QUEST_STATUS_INCOMPLETE)
            player->m_Events.AddEventAtOffset(new ShrineInPerilEvent(player), 2s);
    }
};

// 42430: accept from Delandros -> step into Ebonfang's arena. Killing Ebonfang (obj0) is auto-credited; his death also
// grants "Kill Credit: Fangs of Ashamane" (obj1). The Fangs of Ashamane are awarded on turn-in to Rensar.
class FangsArenaEvent : public BasicEvent
{
public:
    explicit FangsArenaEvent(Player* player) : _player(player) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (_player->IsInWorld())
            _player->TeleportTo(MAP_VALSHARAH, EbonfangArena.GetPositionX(), EbonfangArena.GetPositionY(),
                EbonfangArena.GetPositionZ(), EbonfangArena.GetOrientation());
        return true;
    }

private:
    Player* _player;
};

struct quest_fangs_of_ashamane : QuestScript
{
    quest_fangs_of_ashamane() : QuestScript("quest_fangs_of_ashamane") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus == QUEST_STATUS_INCOMPLETE) // just accepted -> move to the arena
            player->m_Events.AddEventAtOffset(new FangsArenaEvent(player), 1500ms);
    }
};

// Ebonfang (107729) - the Fangs of Ashamane boss. creature_template faction is the friendly placeholder 35, so we make
// him hostile in Reset(); his death grants "Kill Credit: Fangs of Ashamane" (42430 obj1) - the kill of Ebonfang himself
// auto-credits obj0. Bound to 107729 via creature_template.ScriptName.
struct npc_ebonfang : public ScriptedAI
{
    npc_ebonfang(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override { me->SetFaction(FACTION_MONSTER_2); } // faction 16 - attackable Feral boss

    void JustEngagedWith(Unit* /*who*/) override
    {
        me->Yell("The fangs are mine, whelp! Ashamane's power dies with me!", LANG_UNIVERSAL);
    }

    void JustDied(Unit* killer) override
    {
        Player* player = killer ? killer->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr;
        if (!player) // pet/environmental kill - fall back to any player nearby
            for (auto const& ref : me->GetMap()->GetPlayers())
                if (Player* p = ref.GetSource()) { player = p; break; }

        if (player)
            player->KilledMonsterCredit(NPC_CREDIT_FANGS); // 42430 obj1 (obj0 is the kill of Ebonfang itself)
    }
};

// Eredar Soul Lasher (107535) - "Aid for the Ashen" (42439) adds, kill x4. Placeholder faction 35 -> made hostile so
// they can be killed and auto-credit the objective. Bound to 107535 via creature_template.ScriptName.
struct npc_eredar_soul_lasher : public ScriptedAI
{
    npc_eredar_soul_lasher(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override { me->SetFaction(FACTION_MONSTER_2); } // faction 16 - attackable add
};

// =====================================================================================================================
// Restoration artifact: G'Hanir, the Mother Tree.
//   41689 "Cleansing the Mother Tree" (Lyessa Bloomwatcher 104577, both ends) - a single standalone artifact quest.
//        obj0 104608 "Kill Credit: Enter the Dreamway"   (travel credit)
//        obj1 104613 "Kill Credit: Travel to Mount Hyjal" (travel credit)
//        obj2 114511 "Kill Credit"                        (cleanse the Mother Tree; G'Hanir awarded on turn-in)
// All three objectives are invisible travel/event credits (no combat). Lyessa is NOT spawned in integ_world; the SQL
// places her in the Dreamgrove beside Rensar so the quest can be both accepted and turned in locally. On accept we play
// out the travel beats as credits, leaving 41689 ready to hand back to her.
// =====================================================================================================================
enum GHanirData
{
    QUEST_CLEANSING_MOTHER_TREE = 41689,
    NPC_CREDIT_ENTER_DREAMWAY   = 104608, // obj0
    NPC_CREDIT_TRAVEL_HYJAL     = 104613, // obj1
    NPC_CREDIT_CLEANSE          = 114511  // obj2
};

// Grant the three travel/event credits in sequence so the Dreamway -> Hyjal -> cleanse progression reads correctly, then
// leaves the quest complete for turn-in to Lyessa (who is spawned at the Dreamgrove by the SQL).
class MotherTreeCleanseEvent : public BasicEvent
{
public:
    explicit MotherTreeCleanseEvent(Player* player) : _player(player) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (_player->IsInWorld())
            ArtifactGrantCredits(_player, { NPC_CREDIT_ENTER_DREAMWAY, NPC_CREDIT_TRAVEL_HYJAL, NPC_CREDIT_CLEANSE });
        return true;
    }

private:
    Player* _player;
};

struct quest_cleansing_the_mother_tree : QuestScript
{
    quest_cleansing_the_mother_tree() : QuestScript("quest_cleansing_the_mother_tree") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus == QUEST_STATUS_INCOMPLETE) // just accepted -> cleanse the Mother Tree
            player->m_Events.AddEventAtOffset(new MotherTreeCleanseEvent(player), 2s);
    }
};

// =====================================================================================================================
// Guardian artifact: Claws of Ursoc.
//   40647 "When Dreams Become Nightmares" (Lea Stonepaw 104535 -> ender 105354) - the Ursoc's Lair scenario (Emerald
//        Dream/Nightmare). obj 101390 "Arch-Desecrator Malithar" (kill the boss - auto-credited by his death);
//        obj 104543 "Dreaming Bunny" (a stage credit); obj 101334 "Complete the Ursoc's Lair Scenario".
// Ursoc's Lair (map 1536) is already populated in integ_world (Spirit of Ursoc 101362, the Claws of Ursoc display
// 105331, the in-lair Lea Stonepaw 105243, Rothoof trash) but had no boss and no quest-giver, so 40647 was unobtainable
// and had no encounter. The SQL spawns the giver/ender Lea in the Dreamgrove (beside the other Druid artifact givers)
// and spawns Malithar AT the Claws (the retail fight spot - he claims the Claws and fights there). On accept we teleport
// the player into the lair; Malithar's death grants the two credit objectives, completes 40647, and returns the player
// to the Dreamgrove to hand in.
//
// LIMITATION: the mid-scenario Xavius betrayal cinematic (Xavius appears, kidnaps Ursoc, leaves Malithar to seize the
// Claws) is not reproduced - Malithar stands at the Claws from the start and delivers the transform line on engage.
// There is no verified acquisition Scene for Guardian (Conversation-driven; sceneId 0), so dialogue is creature Say/Yell
// (canon lines, correctly attributed to the in-lair Ursoc/Lea where they are present).
// =====================================================================================================================
enum ClawsOfUrsocData
{
    QUEST_WHEN_DREAMS_BECOME_NIGHTMARES = 40647,
    MAP_URSOCS_LAIR                     = 1536,
    MAP_DRUID_DREAMGROVE                = 1220,
    NPC_ARCH_DESECRATOR_MALITHAR        = 101390,
    NPC_SPIRIT_OF_URSOC                 = 101362,
    NPC_LEA_STONEPAW_LAIR               = 105243,
    CREDIT_DREAMING_BUNNY               = 104543, // stage credit (obj)
    CREDIT_URSOCS_LAIR_COMPLETE         = 101334  // "Complete the Ursoc's Lair Scenario" (obj)
};

// Enter beside the in-lair Lea; return beside the Dreamgrove givers to hand in.
static constexpr Position UrsocsLairEntrance = { -12412.00f, -12977.40f, 318.16f, 1.57f };
static constexpr Position DreamgroveReturn   = { 3970.50f, 7395.00f, 24.00f, 5.35f };

// Simple teleport helper (this file has no shared teleport event).
class GuardianTeleportEvent : public BasicEvent
{
public:
    GuardianTeleportEvent(Player* player, uint32 map, Position pos) : _player(player), _map(map), _pos(pos) { }
    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (_player->IsInWorld())
            _player->TeleportTo(_map, _pos.GetPositionX(), _pos.GetPositionY(), _pos.GetPositionZ(), _pos.GetOrientation());
        return true;
    }
private:
    Player* _player;
    uint32 _map;
    Position _pos;
};

struct quest_when_dreams_become_nightmares : QuestScript
{
    quest_when_dreams_become_nightmares() : QuestScript("quest_when_dreams_become_nightmares") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus == QUEST_STATUS_INCOMPLETE) // just accepted -> enter Ursoc's Lair
            player->m_Events.AddEventAtOffset(new GuardianTeleportEvent(player, MAP_URSOCS_LAIR, UrsocsLairEntrance), 1500ms);
    }
};

// Arch-Desecrator Malithar (101390) - the Guardian boss, wielding the corrupted Claws of Ursoc. Placeholder faction 35
// -> hostile in Reset(). Death grants the two credit objectives (the kill of Malithar auto-credits obj 101390),
// completes 40647, and returns the player to the Dreamgrove. Bound to 101390 via creature_template.ScriptName.
struct npc_malithar : public ScriptedAI
{
    npc_malithar(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override { me->SetFaction(FACTION_MONSTER_2); } // faction 16 - attackable

    void JustEngagedWith(Unit* /*who*/) override
    {
        if (Creature* ursoc = me->FindNearestCreature(NPC_SPIRIT_OF_URSOC, 200.0f))
            ursoc->Yell("Vile beasts of the Nightmare! You will never take my claws!", LANG_UNIVERSAL);
        me->Yell("AAARRRRGGGHHHH!!! THE CLAWS! THEY THIRST FOR BLOOD!!", LANG_UNIVERSAL);
    }

    void JustDied(Unit* /*killer*/) override
    {
        me->Yell("The claws... slip from my grasp...", LANG_UNIVERSAL);
        if (Creature* lea = me->FindNearestCreature(NPC_LEA_STONEPAW_LAIR, 200.0f))
            lea->Say("You did it! You got him! The claws are reclaimed!", LANG_UNIVERSAL);

        for (auto const& ref : me->GetMap()->GetPlayers())
            if (Player* p = ref.GetSource())
                if (p->IsInWorld() && p->GetQuestStatus(QUEST_WHEN_DREAMS_BECOME_NIGHTMARES) == QUEST_STATUS_INCOMPLETE)
                {
                    p->KilledMonsterCredit(CREDIT_DREAMING_BUNNY);       // obj 104543
                    p->KilledMonsterCredit(CREDIT_URSOCS_LAIR_COMPLETE); // obj 101334 (obj 101390 = Malithar's own death)
                    p->m_Events.AddEventAtOffset(new GuardianTeleportEvent(p, MAP_DRUID_DREAMGROVE, DreamgroveReturn), 5s);
                }
    }
};

// =====================================================================================================================
// The Dreamgrove's guardian ejects intruders exactly like the hunter hall's Eagle Sentinel: spell_area 203810
// ("Drowsy", Dreamgrove area 7846) auto-applies to everyone who enters and, via a 6s periodic trigger, casts 203822 -
// a teleport-with-loading-screen (Effect 252, TARGET_DEST_DB) to 3689,7096,25, just outside the grove. Retail only
// ejects NON-druids, but TC has no class check, so it teleports druids out of their own hall too. Skip the ejection
// teleport for DRUID players; keep booting everyone else. (Verified against SpellEffect.db2 / spell_target_position.)
class spell_druid_dreamgrove_eject : public SpellScript
{
    void SkipDruids(SpellEffIndex effIndex)
    {
        if (Player* target = GetHitPlayer())
            if (target->GetClass() == CLASS_DRUID)
                PreventHitDefaultEffect(effIndex);
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_druid_dreamgrove_eject::SkipDruids, EFFECT_0, SPELL_EFFECT_ANY);
    }
};

// =====================================================================================================================
void AddSC_artifact_druid()
{
    // Quest
    new quest_scythe_of_elune();            // 40783 Balance
    new quest_its_rightful_place();         // 40784 Balance
    new quest_shrine_of_ashamane();         // 42428 Feral
    new quest_shrine_in_peril();            // 42440 Feral
    new quest_fangs_of_ashamane();          // 42430 Feral
    new quest_cleansing_the_mother_tree();  // 41689 Restoration
    new quest_when_dreams_become_nightmares(); // 40647 Guardian

    // Creature
    RegisterCreatureAI(npc_ebonfang);           // 107729 Feral boss
    RegisterCreatureAI(npc_eredar_soul_lasher); // 107535 Feral adds
    RegisterCreatureAI(npc_malithar);           // 101390 Guardian boss

    // Spell
    RegisterSpellScript(spell_druid_dreamgrove_eject); // 203822 Dreamgrove guardian: don't eject druids from their own hall
}