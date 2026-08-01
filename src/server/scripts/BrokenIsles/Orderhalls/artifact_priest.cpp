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

// Priest Legion artifact acquisition. Per-class Broken Isles content (cf. zone_orderhall_hunter.cpp); the generic
// class-hall framework lives in orderhall_legion.cpp. Shared scenario/credit helpers live in
// orderhall_artifact_common.h (ArtifactAdvanceScenario / ArtifactPlayerNear / ArtifactPlayerBeyond / ...).
//
// Three specs, three acquisition quests (verified against integ_world quest_objectives / creature_questender):
//
//   Discipline "The Light's Wrath" (41625)  - ZERO quest_objectives -> completes purely on the Nexus Vault run
//        (scenario 1065, map 1583). Alonsus Faol (110564) sends you in; ender is Archmage Kalec (105081, map 1220).
//        Azuregos (106699, ally) directs; the two bosses Judgment's Flame (104520) and Nexus-Prince Bilaal (104502)
//        ship as faction-35 placeholders, so the script makes them hostile and Bilaal's death completes the quest.
//
//   Holy "The Vindicator's Plea" (41957)     - a single Type-3 TalkTo objective (Brother Larry 105769); ender is
//        Vindicator Boros (105602). Neither Larry nor Boros were spawned, so the SQL spawns them at Netherlight
//        Temple and the QuestScript auto-grants the talk on accept (flight/talk stand-in, as the Hunter file does).
//        The full T'uure/Niskara scripted instance is out of scope here (see risks) - 41957 itself is a class-hall
//        talk step and is made completable + turn-in-able.
//
//   Shadow "Blade in Twilight" (40710)       - three kill-credit objectives: 101365 (fly to scenario), 101364 (won
//        scenario), 102285 (return to Faol). Scenario 991, map 1539. Shadowlord Slaghammer (101430, ally) directs;
//        Twilight Deacon Farthing (101148) and Zakajz the Corruptor (104276) are already hostile. 101365 is granted
//        on accept, 101364 on Zakajz's death, 102285 on the return teleport; enders Moira/Cho at Netherlight Temple.
//
// The Legion artifact scenarios ship as empty placeholder content in our world DB (their step CriteriaTrees resolve
// to Criteria.ID=0 / Type=None), so the on-screen scenario steps never advance on their own. As in the Hunter file we
// drive the step UI by hand with ArtifactAdvanceScenario() (Scenario::SetStepState+CompleteStep) and, crucially, we
// gate the actual QUEST completion on the player's own progress (boss deaths / credits) - never on the scenario - so
// a scenario hiccup can never strand the questline.

#include "orderhall_artifact_common.h"
#include "Creature.h"
#include "EventProcessor.h"
#include "Log.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "QuestDef.h"
#include "ScriptedCreature.h"
#include "ScriptMgr.h"
#include "SharedDefines.h"

enum PriestArtifactData
{
    // Discipline - "The Light's Wrath"
    QUEST_LIGHTS_WRATH          = 41625,
    MAP_NEXUS_VAULT             = 1583,   // scenario 1065 "The Nexus Vault"
    NPC_AZUREGOS                = 106699, // freed ally / scenario director
    NPC_JUDGMENTS_FLAME         = 104520, // boss step 2 (faction-35 placeholder -> hostile)
    NPC_NEXUS_PRINCE_BILAAL     = 104502, // boss step 4 (faction-35 placeholder -> hostile); death completes the quest
    NPC_ARCHMAGE_KALEC_MAP      = 1220,   // ender Archmage Kalec (105081) lives on map 1220

    // Holy - "The Vindicator's Plea"
    QUEST_VINDICATORS_PLEA      = 41957,
    NPC_BROTHER_LARRY           = 105769, // the quest's single TalkTo objective target

    // Shadow - "Blade in Twilight"
    QUEST_BLADE_IN_TWILIGHT     = 40710,
    MAP_TIRISFAL_SCENARIO       = 1539,   // scenario 991 "Blade in Twilight"
    NPC_SHADOWLORD_SLAGHAMMER    = 101430, // ally / scenario director
    NPC_TWILIGHT_DEACON_FARTHING = 101148, // boss step 7 (already hostile, faction 16)
    NPC_ZAKAJZ_CORRUPTOR         = 104276, // boss step 9 (already hostile, faction 14); death grants "won scenario"
    CREDIT_SHADOW_FLY            = 101365, // objective 0 "fly to scenario"
    CREDIT_SHADOW_WON            = 101364, // objective 1 "won scenario"
    CREDIT_SHADOW_RETURN         = 102285, // objective 2 "return to Faol"

    // Netherlight Temple (class hall) common return point beside Alonsus Faol (110564)
    MAP_NETHERLIGHT_TEMPLE       = 1512
};

// Landing / staging positions (from the research; teleport is the accepted flight stand-in).
static constexpr Position NexusVaultLanding = { 3864.7f, 7359.6f, 268.0f, 1.5f };   // beside Azuregos on map 1583
static constexpr Position KalecReturn       = { -857.2f, 4637.6f, 749.4f, 0.0f };   // beside Archmage Kalec, map 1220
static constexpr Position TirisfalCamp      = { 2044.9f, 2334.2f, 68.9f, 3.0f };    // Twilight Camp beside Slaghammer
static constexpr Position NetherlightReturn = { 1333.9f, 1335.6f, 177.2f, 3.1f };   // beside Alonsus Faol, map 1512

// =====================================================================================================================
// Discipline: "The Light's Wrath" (41625) -> Light's Wrath, in the Nexus Vault (map 1583, scenario 1065).
// =====================================================================================================================

// On accept, Alonsus Faol sends the priest into the Nexus Vault beside the freed Azuregos.
class NexusVaultEntryEvent : public BasicEvent
{
public:
    explicit NexusVaultEntryEvent(Player* player) : _player(player) { }

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

// After Bilaal falls and the artifact is claimed, return the priest to Archmage Kalec on map 1220 to hand in.
class NexusVaultReturnEvent : public BasicEvent
{
public:
    explicit NexusVaultReturnEvent(Player* player) : _player(player) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (_player->IsInWorld() && _player->GetMapId() == MAP_NEXUS_VAULT)
            _player->TeleportTo(NPC_ARCHMAGE_KALEC_MAP, KalecReturn.GetPositionX(), KalecReturn.GetPositionY(),
                KalecReturn.GetPositionZ(), KalecReturn.GetOrientation());
        return true;
    }

private:
    Player* _player;
};

struct quest_lights_wrath : QuestScript
{
    quest_lights_wrath() : QuestScript("quest_lights_wrath") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus == QUEST_STATUS_INCOMPLETE) // just accepted -> enter the Nexus Vault
            player->m_Events.AddEventAtOffset(new NexusVaultEntryEvent(player), 1500ms);
    }
};

// Azuregos (106699): freed ally + scenario 1065 director. He plays the between-fight beats and advances the on-screen
// scenario steps by hand (criteria are dead here). Quest completion itself is gated on Bilaal's death below, so a
// scenario hiccup can never block the run. Bound to 106699 via creature_template.ScriptName.
struct npc_azuregos_disc_director : public ScriptedAI
{
    npc_azuregos_disc_director(Creature* creature) : ScriptedAI(creature) { }

    uint32 _poll = 0;
    uint32 _timer = 0;
    uint8  _line = 0;
    uint8  _phase = 0;

    void Reset() override
    {
        if (me->GetMap()->GetId() == MAP_NEXUS_VAULT)
            me->setActive(true);
    }

    void UpdateAI(uint32 diff) override
    {
        if (me->GetMap()->GetId() != MAP_NEXUS_VAULT)
            return;

        _poll += diff;
        if (_poll < 1000)
            return;
        _poll = 0;
        _timer += 1000;

        if (!ArtifactPlayerBeyond(me, NexusVaultLanding, 0.0f)) // no player on the isle yet
            return;

        switch (_phase)
        {
            case 0: // The Azure Prisoner -> Seeking Answers: freed, then lead toward the Librarium
                switch (_line)
                {
                    case 0: me->Say("You are a capable healer, priest. I feel my strength returning.", LANG_UNIVERSAL); ++_line; break;
                    case 1: if (_timer >= 5000) { me->Say("But first, I think I'll slip into a more comfortable form. These halls were never quite spacious enough for my tastes.", LANG_UNIVERSAL); ++_line; } break;
                    default: ArtifactAdvanceScenario(me); _phase = 1; break; // step 0 "The Azure Prisoner" done
                }
                break;
            case 1: // Seeking Answers -> Cleansed by Holy Fire: reached the Librarium, Judgment's Flame bars the way
                if (ArtifactPlayerBeyond(me, NexusVaultLanding, 60.0f))
                {
                    me->Say("Priest, perhaps you can use your magic to influence his mind... Trick him into snuffing out his own flame!", LANG_UNIVERSAL);
                    ArtifactAdvanceScenario(me); // step 1 "Seeking Answers" done
                    _phase = 2;
                }
                break;
            case 2: // The Way Out is Through: the rift, and Nexus-Prince Bilaal beyond it
                if (ArtifactPlayerBeyond(me, NexusVaultLanding, 200.0f))
                {
                    me->Say("The rift ahead - Nexus-Prince Bilaal awaits. Steel yourself!", LANG_UNIVERSAL);
                    ArtifactAdvanceScenario(me); // step 3 "The Way Out is Through" done
                    _phase = 3;
                }
                break;
            default:
                break;
        }
    }
};

// Judgment's Flame (104520): scenario step 2 boss. Faction-35 placeholder -> made hostile; its death advances the
// scenario UI. Bound to 104520 via creature_template.ScriptName.
struct npc_judgments_flame : public ScriptedAI
{
    npc_judgments_flame(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override { me->SetFaction(FACTION_MONSTER_2); } // faction 16 - attackable

    void UpdateAI(uint32 /*diff*/) override
    {
        if (!UpdateVictim())
            return;
    }

    void JustDied(Unit* /*killer*/) override
    {
        if (me->GetMap()->GetId() != MAP_NEXUS_VAULT)
            return;
        me->Yell("The flame... is extinguished...", LANG_UNIVERSAL);
        ArtifactAdvanceScenario(me); // step 2 "Cleansed by Holy Fire" done
    }
};

// Nexus-Prince Bilaal (104502): scenario step 4 boss and the authoritative completion point for Discipline. Faction-35
// placeholder -> made hostile; his death advances the remaining steps (seize / claim Light's Wrath), completes the
// (objective-less) quest 41625, and returns the priest to Archmage Kalec. Bound to 104502 via ScriptName.
// NOTE: Nexus-Prince Bilaal (104502) is shared with the Mage artifact on the same map (1583), so a single script owns
// him in artifact_mage.cpp (npc_nexus_prince_bilaal) - it grants the Discipline completion (41625) too. A duplicate
// script here would collide on the script name, so the Priest Discipline completion lives in the Mage owner.

// =====================================================================================================================
// Holy: "The Vindicator's Plea" (41957) -> T'uure, Beacon of the Naaru.
//
// DB truth: 41957 has a single Type-3 TalkTo objective (Brother Larry 105769) and its ender is Vindicator Boros
// (105602); the full T'uure/Niskara scripted instance is not a DB2 InstanceScenario and its cast is unspawned (see
// risks). We keep 41957 completable + turn-in-able: the SQL spawns Larry and Boros at Netherlight Temple, and the
// QuestScript auto-grants the talk on accept (mirroring the Hunter file's "grant the intro credit on accept" pattern),
// so the priest can hand in to Boros immediately.
// =====================================================================================================================

struct quest_vindicators_plea : QuestScript
{
    quest_vindicators_plea() : QuestScript("quest_vindicators_plea") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus == QUEST_STATUS_INCOMPLETE) // accepted from Alonsus Faol -> intro talk with Brother Larry
            player->TalkedToCreature(NPC_BROTHER_LARRY, ObjectGuid::Empty); // satisfies the Type-3 TalkTo objective
    }
};

// =====================================================================================================================
// Shadow: "Blade in Twilight" (40710) -> Xal'atath, in Tirisfal Glades (map 1539, scenario 991).
// =====================================================================================================================

// On accept, credit the "fly to scenario" objective and drop the priest at the Twilight Camp beside Slaghammer.
class BladeTwilightEntryEvent : public BasicEvent
{
public:
    explicit BladeTwilightEntryEvent(Player* player) : _player(player) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (_player->IsInWorld())
            _player->TeleportTo(MAP_TIRISFAL_SCENARIO, TirisfalCamp.GetPositionX(), TirisfalCamp.GetPositionY(),
                TirisfalCamp.GetPositionZ(), TirisfalCamp.GetOrientation());
        return true;
    }

private:
    Player* _player;
};

// After Zakajz falls, credit the "return to Faol" objective and return the priest to Netherlight Temple (Moira / Cho).
class BladeTwilightReturnEvent : public BasicEvent
{
public:
    explicit BladeTwilightReturnEvent(Player* player) : _player(player) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (_player->IsInWorld() && _player->GetMapId() == MAP_TIRISFAL_SCENARIO)
        {
            _player->KilledMonsterCredit(CREDIT_SHADOW_RETURN); // objective 2 "Return to Alonsus and Moira"
            _player->TeleportTo(MAP_NETHERLIGHT_TEMPLE, NetherlightReturn.GetPositionX(), NetherlightReturn.GetPositionY(),
                NetherlightReturn.GetPositionZ(), NetherlightReturn.GetOrientation());
        }
        return true;
    }

private:
    Player* _player;
};

struct quest_blade_in_twilight : QuestScript
{
    quest_blade_in_twilight() : QuestScript("quest_blade_in_twilight") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus == QUEST_STATUS_INCOMPLETE) // just accepted -> fly to Tirisfal Glades
        {
            player->KilledMonsterCredit(CREDIT_SHADOW_FLY); // objective 0 "Go to the marked location"
            player->m_Events.AddEventAtOffset(new BladeTwilightEntryEvent(player), 1500ms);
        }
    }
};

// Shadowlord Slaghammer (101430): ally + scenario 991 director. He plays the escort beats and advances the on-screen
// scenario steps by hand; the objectives (credits) are driven off accept/Zakajz-death/return, so the scenario is only
// presentation. Bound to 101430 via creature_template.ScriptName.
struct npc_slaghammer_shadow_director : public ScriptedAI
{
    npc_slaghammer_shadow_director(Creature* creature) : ScriptedAI(creature) { }

    uint32 _poll = 0;
    uint32 _timer = 0;
    uint8  _line = 0;
    uint8  _phase = 0;

    void Reset() override
    {
        if (me->GetMap()->GetId() == MAP_TIRISFAL_SCENARIO)
            me->setActive(true);
    }

    void UpdateAI(uint32 diff) override
    {
        if (me->GetMap()->GetId() != MAP_TIRISFAL_SCENARIO)
            return;

        _poll += diff;
        if (_poll < 1000)
            return;
        _poll = 0;
        _timer += 1000;

        if (!ArtifactPlayerBeyond(me, TirisfalCamp, 0.0f))
            return;

        switch (_phase)
        {
            case 0: // The Twilight Camp -> Raiding the Tomb Raiders
                switch (_line)
                {
                    case 0: me->Say("Moira said ye'd be comin'. I'm Slaghammer, payin' me family debt by hangin' oot wi' these buggers. Lemme take down this barrier.", LANG_UNIVERSAL); ++_line; break;
                    case 1: if (_timer >= 5000) { me->Say("Cut through their camp - the desecrated tomb lies beyond. Xal'atath waits within.", LANG_UNIVERSAL); ++_line; } break;
                    default: ArtifactAdvanceScenario(me); _phase = 1; break;
                }
                break;
            case 1: // push into the tomb approach
                if (ArtifactPlayerBeyond(me, TirisfalCamp, 60.0f))
                {
                    me->Say("The Desecrated Tomb. Reconsecrate it as you pass - the reaper Farthing stands guard.", LANG_UNIVERSAL);
                    ArtifactAdvanceScenario(me);
                    _phase = 2;
                }
                break;
            case 2: // reach the inner sanctum where the Deacon and Zakajz wait
                if (ArtifactPlayerBeyond(me, TirisfalCamp, 140.0f))
                {
                    me->Say("There - Twilight Deacon Farthing! Strike him down and claim the Blade of the Black Empire!", LANG_UNIVERSAL);
                    ArtifactAdvanceScenario(me);
                    _phase = 3;
                }
                break;
            default:
                break;
        }
    }
};

// Twilight Deacon Farthing (101148): scenario step 7 boss (already hostile). His death advances the scenario UI toward
// the Blade. Bound to 101148 via ScriptName - faction left untouched (already 16).
struct npc_twilight_deacon_farthing : public ScriptedAI
{
    npc_twilight_deacon_farthing(Creature* creature) : ScriptedAI(creature) { }

    void UpdateAI(uint32 /*diff*/) override
    {
        if (!UpdateVictim())
            return;
    }

    void JustDied(Unit* /*killer*/) override
    {
        if (me->GetMap()->GetId() != MAP_TIRISFAL_SCENARIO)
            return;
        me->Yell("The Old Gods... will feast on your soul...", LANG_UNIVERSAL);
        ArtifactAdvanceScenario(me); // step 7 "Death to the Deacon" done -> the Blade is exposed
        ArtifactAdvanceScenario(me); // step 8 "The Blade of the Black Empire" (claim Xal'atath)
    }
};

// Zakajz the Corruptor (104276): scenario step 9 boss (already hostile, faction 14) and the authoritative "won the
// scenario" point. His death credits objective 1 and schedules the return that credits objective 2. Bound to 104276
// via ScriptName - faction left untouched (already 14).
// NOTE: Zakajz the Corruptor (104276) is shared with the Warrior Arms artifact on the same map (1539), so a single
// script owns him in artifact_warrior.cpp (npc_zakajz_corruptor) - it grants the Shadow completion (40710) too.

// =====================================================================================================================
// Holy (the REAL acquisition): "Return of the Light" (42074) -> T'uure, Beacon of the Naaru. The Niskara instance.
//
// "The Vindicator's Plea" (41957, above) is only the class-hall intro talk. The real T'uure acquisition is 42074 - given
// by Jace Darkweaver (106011) at Netherlight Temple, turned in to Prophet Velen (101313), BOTH already spawned. Its 3
// objectives are credit dummies: 106076 "Niskara Portal Key" (portal in), 106033 "Niskara Bunny" (stage), 106031 "Holy
// Bunny" (T'uure claimed). Map 1489 (Niskara) is heavily populated with the DEMON HUNTER Niskara scenario, so we do NOT
// permanently spawn our boss there - on accept we teleport the priest to the T'uure area (LegionCore's coords) and
// TRANSIENTLY summon Lady Calindris (106318); her death grants the claim credits, completes 42074, returns to Velen.
//
// LIMITATION: map 1489 hosts the DH scenario; ambient demons may aggro the priest (thematically fine on a demon world).
// LegionCore's full Boros heal-gate escort is simplified to the boss encounter. No verified Scene (Conversation-driven).
// =====================================================================================================================
enum ReturnOfLightData
{
    QUEST_RETURN_OF_THE_LIGHT = 42074,
    MAP_NISKARA               = 1489,
    MAP_NETHERLIGHT_TEMPLE    = 1220,
    NPC_LADY_CALINDRIS        = 106318,
    NPC_VINDICATOR_BOROS_NISK = 106134,
    CREDIT_NISKARA_PORTAL     = 106076, // obj0 (on accept)
    CREDIT_NISKARA_STAGE      = 106033, // obj1 (on Calindris death)
    CREDIT_TUURE_CLAIMED      = 106031  // obj2 (on Calindris death)
};

static constexpr Position NiskaraEntrance = { 12.00f, 1189.50f, -45.98f, 0.0f };
static constexpr Position CalindrisPos    = { 58.69f, 1212.91f, -39.64f, 3.14f };
static constexpr Position VelenReturn     = { -693.4f, 4492.8f, 728.5f, 0.0f }; // beside Prophet Velen (turn-in)

// Return the priest to Prophet Velen once T'uure is claimed.
class PriestNiskaraReturnEvent : public BasicEvent
{
public:
    explicit PriestNiskaraReturnEvent(Player* player) : _player(player) { }
    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (_player->IsInWorld() && _player->GetMapId() == MAP_NISKARA)
            _player->TeleportTo(MAP_NETHERLIGHT_TEMPLE, VelenReturn.GetPositionX(), VelenReturn.GetPositionY(),
                VelenReturn.GetPositionZ(), VelenReturn.GetOrientation());
        return true;
    }
private:
    Player* _player;
};

// Summon Lady Calindris (+ Boros for flavor) AFTER the portal teleport lands the priest on Niskara (transient - no
// permanent spawn on the DH-populated map).
class TuureSummonEvent : public BasicEvent
{
public:
    explicit TuureSummonEvent(Player* player) : _player(player) { }
    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        if (_player->IsInWorld() && _player->GetMap()->GetId() == MAP_NISKARA)
        {
            _player->SummonCreature(NPC_VINDICATOR_BOROS_NISK, NiskaraEntrance.GetPositionX() + 4.0f,
                NiskaraEntrance.GetPositionY(), NiskaraEntrance.GetPositionZ(), 0.0f, TEMPSUMMON_TIMED_DESPAWN, 5min);
            _player->SummonCreature(NPC_LADY_CALINDRIS, CalindrisPos, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 60s);
        }
        return true;
    }
private:
    Player* _player;
};

struct quest_return_of_the_light : QuestScript
{
    quest_return_of_the_light() : QuestScript("quest_return_of_the_light") { }

    void OnQuestStatusChange(Player* player, Quest const* /*quest*/, QuestStatus /*oldStatus*/, QuestStatus newStatus) override
    {
        if (newStatus != QUEST_STATUS_INCOMPLETE)
            return;
        player->KilledMonsterCredit(CREDIT_NISKARA_PORTAL); // obj0 "portal in"
        player->TeleportTo(MAP_NISKARA, NiskaraEntrance.GetPositionX(), NiskaraEntrance.GetPositionY(),
            NiskaraEntrance.GetPositionZ(), NiskaraEntrance.GetOrientation());
        player->m_Events.AddEventAtOffset(new TuureSummonEvent(player), 3s);
    }
};

// Lady Calindris (106318): the T'uure boss on Niskara. Placeholder faction 35 -> hostile in Reset(). Her death grants
// the stage + claim credits (T'uure absorbs the Light), completes 42074, and returns the priest to Velen. Bound via
// creature_template.ScriptName.
struct npc_lady_calindris : public ScriptedAI
{
    npc_lady_calindris(Creature* creature) : ScriptedAI(creature) { }

    void Reset() override { me->SetFaction(FACTION_MONSTER_2); } // faction 16 - attackable

    void JustEngagedWith(Unit* /*who*/) override
    {
        me->Yell("I grow tired of this! Be consumed by the darkness!", LANG_UNIVERSAL);
    }

    void JustDied(Unit* /*killer*/) override
    {
        me->Yell("My staff... it senses the Light within you! What is it... Nooooo!", LANG_UNIVERSAL);

        for (auto const& ref : me->GetMap()->GetPlayers())
            if (Player* p = ref.GetSource())
                if (p->IsInWorld() && p->GetQuestStatus(QUEST_RETURN_OF_THE_LIGHT) == QUEST_STATUS_INCOMPLETE)
                {
                    p->KilledMonsterCredit(CREDIT_NISKARA_STAGE); // obj1
                    p->KilledMonsterCredit(CREDIT_TUURE_CLAIMED); // obj2
                    p->CompleteQuest(QUEST_RETURN_OF_THE_LIGHT);  // safety net: ready to hand in to Velen
                    p->m_Events.AddEventAtOffset(new PriestNiskaraReturnEvent(p), 6s);
                }
    }
};

void AddSC_artifact_priest()
{
    // Quests
    new quest_lights_wrath();
    new quest_vindicators_plea();
    new quest_blade_in_twilight();
    new quest_return_of_the_light(); // 42074 Holy (T'uure)

    // Creatures
    RegisterCreatureAI(npc_azuregos_disc_director);
    RegisterCreatureAI(npc_judgments_flame);
    RegisterCreatureAI(npc_slaghammer_shadow_director);
    RegisterCreatureAI(npc_twilight_deacon_farthing);
    RegisterCreatureAI(npc_lady_calindris); // 106318 Holy T'uure boss
    // Nexus-Prince Bilaal (104502) + Zakajz (104276) are owned by the Mage/Warrior files (shared bosses/maps).
}