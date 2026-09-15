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

#ifndef PathOfAscension_h__
#define PathOfAscension_h__

#include "DatabaseEnvFwd.h"
#include "Define.h"
#include <ctime>
#include <unordered_map>
#include <vector>

class Player;

/*
 * The Path of Ascension - the Kyrian covenant's unique sanctum feature (P6.3 of the covenant plan).
 *
 * WHAT THE DATA SAYS THIS IS. Every id below is read out of 12.0.7.68275 client DB2 or `integ_world`; the
 * "NOT DERIVABLE" block at the bottom lists exactly what deliberately is not, and why refusing beats guessing.
 *
 * ---------------------------------------------------------------------------------------------------------
 * THE CLIENT PROTOCOL: THERE ISN'T ONE. (established before a line of this was written)
 * ---------------------------------------------------------------------------------------------------------
 * The string "ascension" does not occur ANYWHERE in the 12.0.7.68275 client: not in the 113 MB image, not in
 * the 181,200-entry string-xref table, not in the UI source tree (317 addons), not in the 2,687-entry opcode
 * master. Searches were method-validated against known-present controls before their zeros were believed -
 * `C_ArdenwealdGardening` (the Night Fae feature's namespace), `C_CovenantSanctumUI`, `CovenantSanctum`,
 * `SanctumUnique`, `GarrTalentFeatureSubtype` and the 7 `COVENANT_*` opcodes all resolve by the same methods
 * that return nothing for Ascension. There is no `*Ascension*` addon, no Lua namespace, no tutorial-frame
 * constant, and the only `ASCEN` opcodes in the build are `CMSG_MOVE_START_ASCEND` / `_STOP_ASCEND`.
 *
 * The reason is visible in the client's own type system: `Enum.GarrTalentFeatureType.SanctumUnique = 5` is a
 * SINGLE shared slot for all four covenants' unique features, and the only thing identifying the Kyrian one is
 * the zone-named `Enum.GarrTalentFeatureSubtype.Bastion = 1`. The feature's display name comes from the
 * GlobalString `COVENANT_SANCTUM_UNIQUE_KYRIAN`, which is why "Ascension" is not in the executable at all.
 * `Blizzard_CovenantSanctum/Blizzard_CovenantSanctumUpgrades.lua` renders it through the shared
 * `UniqueUpgrade` frame and drives it with the ordinary garrison-talent events (`GARRISON_TALENT_UPDATE`,
 * `GARRISON_TALENT_COMPLETE`, `GARRISON_TALENT_RESEARCH_STARTED`).
 *
 * So this class sends no packet of its own and adds no opcode - exactly like the two finished siblings
 * (QueensConservatory, AbominationFactory). The research half of the feature already reaches the client over
 * `CMSG_GARRISON_RESEARCH_TALENT` / `SMSG_GARRISON_TALENT_*`, which the generic engine already speaks.
 *
 * ---------------------------------------------------------------------------------------------------------
 * THE UNLOCK LADDER (GarrTalentTree 320)
 * ---------------------------------------------------------------------------------------------------------
 *   GarrTalentTree 320
 *       GarrTypeID 111 (covenant sanctum), MaxTiers 5, Flags 16, CurrencyID 0, UiTextureKitID 5283,
 *       FeatureTypeIndex 5 (= GARR_TALENT_FEATURE_UNIQUE, client Enum.GarrTalentFeatureType.SanctumUnique),
 *       FeatureSubtypeIndex 1 (= CovenantID, client Enum.GarrTalentFeatureSubtype.Bastion).
 *
 *   Its five talents are an unlock ladder, not perks - every GarrTalentRank.PerkSpellID is 0 (ranks
 *   1394-1398), exactly like Night Fae tree 319 and Necrolord tree 321. CONFIRMED, not assumed: all five
 *   rows read PerkSpellID = 0 and PerkPlayerConditionID = 0.
 *
 *   The talents' own descriptions are a complete specification of the feature, and everything this class
 *   derives comes from them verbatim:
 *       1091 "First Steps"          Tier 0 (PlayerConditionID 84025)
 *            "Unlocks the sacred trial of the Path of Ascension, facilitating the capture of SIX Shadowlands
 *             memories for your soulbinds to train against for unique rewards."
 *       1092 "Sacred Trials"        Tier 1
 *            "Allows the capture of FOUR MORE Shadowlands memories for training in the Path of Ascension.
 *             Empowers SOME of your memories with accumulated anima, granting you access to their SECOND
 *             TRIAL. Unlocks access to weekly quests."
 *       1093 "Continued Training"   Tier 2
 *            "Empowers more of your captured memories, unlocking the REST OF THE TRIALS OF LOYALTY as well as
 *             the FIRST OF THE TRIALS OF WISDOM. The Brazier of Lessons Learned has been activated,
 *             INCREASING YOUR SOULBINDS' HEALTH AND DAMAGE in Ascension Coliseum."
 *       1094 "Teachings of Wisdom"  Tier 3
 *            "Your collected anima strengthens the remaining memories, granting access to the REMAINING
 *             TRIALS OF WISDOM. Unlocks access to a SECOND WEEKLY QUEST."
 *       1095 "Trials of Humility"   Tier 4
 *            "Unlocks the final trial for ALL of your captured memories, the TRIAL OF HUMILITY. The Brazier of
 *             Inward Reflection has been activated, increasing your soulbinds' health and damage in Ascension
 *             Coliseum."
 *   => memory capacity is 6 at one researched tier and 10 from two tiers upward (6 + "four more"); it does NOT
 *      grow per tier. The trial ceiling, the weekly-quest slots and the two braziers all move on the tiers
 *      quoted above. That is what the ladder tables in this file encode - nothing there is a round number
 *      someone liked.
 *
 *   Research costs (GarrTalentRank 1394-1398 + GarrTalentCost 4419-4423 / 6488-6506, which agree):
 *       T1 1500 x1813 + 6 x1810 @3600s, T2 5000 + 12 @43200s, T3 10000 + 22 @86400s,
 *       T4 12500 + 40 @86400s, T5 15000 + 70 @86400s.  (1813 Reservoir Anima, 1810 Redeemed Soul.)
 *   The generic Garrison::LearnTalent/ResearchTalent engine already charges and times these; this class does
 *   not re-implement any of it, it only reads the resulting talent ranks.
 *
 * ---------------------------------------------------------------------------------------------------------
 * THE ACTIVITY IS A SOLO SCENARIO ON ITS OWN MAP, RUN AT ONE OF FOUR NAMED DIFFICULTIES
 * ---------------------------------------------------------------------------------------------------------
 * This is the part the plan (§A22/§D.10) recorded as having "no DB2 representation". It does have one - it is
 * simply not in any Garr* table:
 *
 *   Map 2375      "9.0 Bastion Arena - Path of Ascension", InstanceType 5 (= MAP_SCENARIO), ExpansionID 8.
 *   AreaTable 13462 "Ascension Coliseum" (ZoneName "BastionAscensionColiseum", ContinentID 2375) - the arena.
 *   Scenario 1803 "Path of Ascension", Type 0, Flags 2, UiTextureKitID 5361; 1826 is its tutorial twin.
 *                 LFGDungeons 2084 "Path of Ascension" -> ScenarioID 1803, and 2089 "(Tutorial)" -> 1826.
 *   Difficulty 168 "Path of Ascension: Courage"   InstanceType 5, MinPlayers 1, MaxPlayers 1
 *   Difficulty 169 "Path of Ascension: Loyalty"   InstanceType 5, MinPlayers 1, MaxPlayers 1
 *   Difficulty 170 "Path of Ascension: Wisdom"    InstanceType 5, MinPlayers 1, MaxPlayers 1
 *   Difficulty 171 "Path of Ascension: Humility"  InstanceType 5, MinPlayers 1, MaxPlayers 1
 *
 *   The four difficulties ARE the four trials, and their id order IS the difficulty order. That is not an
 *   inference from lore: quest 63181-63191 log text says "in a Trial of LOYALTY OR HIGHER" while 63188/63189
 *   say "in a Trial of WISDOM", and talent 1093 unlocks "the rest of the Trials of Loyalty as well as the
 *   first of the Trials of Wisdom" - Courage < Loyalty < Wisdom < Humility, matching 168 < 169 < 170 < 171.
 *   MinPlayers = MaxPlayers = 1 is why this is modelled per character and not as a group activity.
 *
 *   The scenario's four steps are ScenarioStep 4521/4542/4543/4544 (OrderIndex 0-3) - "Preparation" /
 *   "Fight" / "Reward" / "Reward" - carrying CriteriaTrees 85338/85638/85650/85741, whose developer names are
 *   "9.0 Bastion - Path of Ascension - Scenario - Step 1 Preparation / Step 2 Fight / Step 3 Reward /
 *   Step 4 Reward (JLW)". The tutorial twin 1826 has steps 4581-4584 / trees 86532/86536/86538/86540.
 *   (The scenario id was cross-checked numerically through ScenarioStep.ScenarioID rather than taken from a
 *   string lookup - a naive WDC5 string reader mis-attributes it to the neighbouring scenario 1798, which is
 *   actually Halls of Atonement.)
 *
 *   MapDifficulty 4795/4796/4797/4798 bind difficulties 168/169/170/171 to map 2375 with MaxPlayers 1 -
 *   and all four carry ContentTuningID 0, i.e. THE BUILD PUBLISHES NO SCALING for the four trials. That is
 *   one of the reasons the encounter itself cannot be derived (see NOT DERIVABLE below).
 *
 *   Progress is counted by Achievement 14340/14342/14343/14344/14345/14346/14348/14349/14351 ("The Path
 *   Towards Ascension N", "Defeat N boss(es) in the Path of Ascension" for N = 1/3/5/7/12/16/20/24/39,
 *   rewarding "Character Unlock: Additional challenges within Path of Ascension").
 *
 * ---------------------------------------------------------------------------------------------------------
 * WHAT A "MEMORY" IS, AND WHY THE PLAYER IS NOT THE COMBATANT
 * ---------------------------------------------------------------------------------------------------------
 * The opponents are captured Shadowlands *memories*; the player is a tactician and the soulbinds fight. The
 * client's own quest text says so - quest 60496 "Into the Coliseum": "As you are not a being of Bastion, you
 * shall act as a TACTICIAN FOR YOUR SOULBINDS." Apolon's gossip menu 26145 is the feature's help text and uses
 * the same vocabulary throughout: "How do I capture new memories?", "How do I fight higher difficulty
 * memories?", "What are Medallions of Service...?", "What is equipment?", "What are charms?", "What do I do if
 * I'm struggling against a memory?". Creature 171873 is literally named "Athanos - Overpowered Combat Memory".
 *
 * All of it already exists in `integ_world` under QuestSortID -595 (SharedDefines.h
 * QUEST_SORT_PATH_OF_ASCENSION = 595) - 90 quests, with their questgivers already linked:
 *     Haephus  167745  -> 60489 "The Path of Ascension"  (the sanctum-upgrades NPC; feature intro)
 *     Artemede 168427  -> 60496/60497/60498 intro + the ten weekly "Artemede's Challenge" quests 63181-63191
 *                         and 63192 "Path of Ascension: Trial of Humility"
 *     Apolon   168485  -> the CHAMPION quests 62951 Kleia / 62952 Pelagos / 62953 Mikanikos ("Win as ...")
 *                         and the TEN MEMORY quests 62954, 63168-63176
 *     Dactylis 168430  "Path of Ascension Crafter" -> the "Blueprint: ..." equipment quests
 *
 * The ten memory quests are exactly the "six ... four more" the talents promise, and each one's objective is a
 * kill credit on the memory's own creature - which is why CompleteTrial below awards that credit and lets the
 * ordinary quest system pay the reward (item 184812 for a memory, 184811 for a weekly challenge) instead of
 * this class inventing a reward table:
 *     62954 Kalisthene            -> credit 170654      63173 Thran'tiok             -> credit 172411
 *     63168 Echthra               -> credit 172177      63174 Mad Mortimer            -> credit 172101
 *     63169 Alderyn and Myn'ir    -> credit 172408      63175 Athanos                 -> credit 171873
 *     63170 Nuuminuuru            -> credit 172410      63176 Azaruux                 -> credit 172333
 *     63171 Craven Corinth        -> credit 172412
 *     63172 Splinterbark Nightmare-> credit 172682
 *
 * ---------------------------------------------------------------------------------------------------------
 * NOT DERIVABLE OFFLINE - deliberately left as data, never guessed
 * ---------------------------------------------------------------------------------------------------------
 *   * WHICH six memories the first tier captures and which four the second adds. The talents state the COUNTS
 *     and never the names; no PlayerCondition in the build references talents 1091-1095 beyond 84025 on the
 *     tier-0 talent (which tree 321 shares), and GarrTalentRank.PerkSpellID is 0 throughout.
 *   * WHICH memories are "empowered" at tier 2 ("some ... their second trial") versus tier 3 ("the rest of the
 *     Trials of Loyalty as well as the first of the Trials of Wisdom") versus tier 4. Same reason.
 *   Those two are the whole content of the world table `garrison_ascension_memory`, one row per memory, and
 *   the per-trial columns encode exactly the "some / rest / first / remaining" wording. The table ships EMPTY.
 *
 *   * THE ARENA ITSELF. `integ_world.scenarios` has no row for map 2375 or scenario 1803 (317 rows present,
 *     so the table is populated and the absence is real), `scenario_poi` has no row for CriteriaTrees
 *     85338/85638/85650/85741, map 2375 has ZERO creature, ZERO gameobject and ZERO areatrigger rows, and
 *     `instance_template` has no row for it. Apolon 168485, Artemede 168427 and Dactylis 168430 are all
 *     unspawned, and every one of the boss creature_template rows has an empty ScriptName.
 *   * WHICH creature variant is which trial. Each boss ships as 2-4 creature_template entries (e.g. Echthra
 *     172177/172482/172515, Azaruux 172333/172504/172643) which looks like a per-difficulty set, but nothing
 *     maps them: `creature_template_difficulty` has no rows for them and MapDifficulty 4795-4798 name no
 *     encounter. Guessing that mapping would be inventing the encounter.
 *   * ANY SCALING OR REWARD. MapDifficulty 4795-4798 all carry ContentTuningID 0; no loot template references
 *     map 2375; the two braziers' "increased health and damage" magnitudes appear in no table.
 *   Because of that StartTrial REFUSES with ASCENSION_ERROR_NO_ARENA_CONTENT rather than starting a challenge
 *   that could never be completed. A trial that cannot be fought is not silently "started" and it is never
 *   auto-won: the only way a completion is recorded is a real one (or an explicit GM command, which is a GM
 *   tool, not a fallback). Authoring the scenario + spawns turns it on with no code change here.
 */

enum PathOfAscensionConstants : uint32
{
    // GarrTalentTree.FeatureSubtypeIndex of tree 320 / client Enum.GarrTalentFeatureSubtype.Bastion.
    COVENANT_ID_KYRIAN                  = 1,
    // MaxTiers of GarrTalentTree 320.
    ASCENSION_MAX_TIERS                 = 5,
    // Scenario 1803 "Path of Ascension" on Map 2375 "9.0 Bastion Arena - Path of Ascension"
    // (InstanceType 5 = MAP_SCENARIO), AreaTable 13462 "Ascension Coliseum" (ZoneName
    // "BastionAscensionColiseum", ContinentID 2375). 1826 is the parallel tutorial scenario.
    ASCENSION_SCENARIO_ID               = 1803,
    ASCENSION_SCENARIO_TUTORIAL_ID      = 1826,
    ASCENSION_MAP_ID                    = 2375,
    ASCENSION_AREA_ID                   = 13462,
    // Talent 1091 "First Steps" says SIX memories; 1092 "Sacred Trials" adds FOUR MORE. It does not grow again.
    ASCENSION_MEMORIES_AT_FIRST_TIER    = 6,
    ASCENSION_MEMORIES_AT_SECOND_TIER   = 10,
    // Talent 1092 "Unlocks access to weekly quests"; talent 1094 "Unlocks access to a second weekly quest".
    ASCENSION_WEEKLY_SLOTS_TIER_FIRST   = 2,
    ASCENSION_WEEKLY_SLOTS_TIER_SECOND  = 4,
    // Talent 1093 activates the Brazier of Lessons Learned, talent 1095 the Brazier of Inward Reflection.
    ASCENSION_BRAZIER_LESSONS_TIER      = 3,
    ASCENSION_BRAZIER_INWARD_TIER       = 5
};

// The four trials. These are Difficulty.db2 rows, not an invented scale - see the file header. The enum value
// is the ordering the client's own quest text asserts ("a Trial of Loyalty or higher"), and
// GetTrialDifficultyId() maps it back to the real Difficulty id for whoever ends up instantiating the map.
enum AscensionTrial : uint8
{
    ASCENSION_TRIAL_NONE        = 0,
    ASCENSION_TRIAL_COURAGE     = 1,    // Difficulty 168 "Path of Ascension: Courage"
    ASCENSION_TRIAL_LOYALTY     = 2,    // Difficulty 169 "Path of Ascension: Loyalty"
    ASCENSION_TRIAL_WISDOM      = 3,    // Difficulty 170 "Path of Ascension: Wisdom"
    ASCENSION_TRIAL_HUMILITY    = 4,    // Difficulty 171 "Path of Ascension: Humility"
    ASCENSION_TRIAL_MAX         = 4
};

enum PathOfAscensionError : uint32
{
    ASCENSION_OK = 0,
    ASCENSION_ERROR_NOT_KYRIAN,             // owner is not pledged to the Kyrian
    ASCENSION_ERROR_NOT_UNLOCKED,           // GarrTalentTree 320 tier 1 (talent 1091 "First Steps") not researched
    ASCENSION_ERROR_UNKNOWN_MEMORY,         // no `garrison_ascension_memory` row with that id
    ASCENSION_ERROR_NO_MEMORY_DATA,         // the table is empty - see the header comment
    ASCENSION_ERROR_MEMORY_TIER_TOO_LOW,    // the memory needs more researched tiers than are held
    ASCENSION_ERROR_MEMORY_CAPACITY,        // already holding as many memories as the researched tiers allow
    ASCENSION_ERROR_ALREADY_CAPTURED,
    ASCENSION_ERROR_NOT_CAPTURED,           // trial asked for a memory this character has not captured
    ASCENSION_ERROR_INVALID_TRIAL,          // trial outside 1-4
    ASCENSION_ERROR_TRIAL_LOCKED,           // that trial is not open for that memory at the researched tier
    ASCENSION_ERROR_NO_ARENA_CONTENT        // scenario 1803 / map 2375 is unauthored - refuse rather than fake it
};

// One authored memory. The COUNTS and the trial vocabulary are client data (see the header); what is content -
// and only this - is which memory occupies which slot of the ladder, so that is all a row carries.
struct AscensionMemoryTemplate
{
    uint32 MemoryId         = 0;    // author-chosen id, referenced by character_garrison_path_of_ascension
    uint32 CreatureId       = 0;    // the memory's creature (creature_template); kill credit on a completion
    uint32 CaptureQuestId   = 0;    // the "Path of Ascension: X" quest (QuestSortID -595), 0 = none
    uint8  RequiredTier     = 1;    // researched tiers of tree 320 needed before it can be captured (1-5)
    // Researched tiers of tree 320 at which each trial opens FOR THIS MEMORY. 0 = this memory never offers it.
    // This is exactly the talents' "some / the rest / the first / the remaining" wording, per memory.
    uint8  CourageTier      = 1;
    uint8  LoyaltyTier      = 0;
    uint8  WisdomTier       = 0;
    uint8  HumilityTier     = 0;

    // Researched tiers needed for `trial` on this memory; 0 when this memory does not offer it at all.
    uint8 GetTrialTier(AscensionTrial trial) const;
};

// One memory this character has captured, and how far it has been taken.
struct AscensionMemory
{
    uint32 MemoryId             = 0;
    time_t CapturedTime         = 0;
    uint8  HighestTrialWon      = ASCENSION_TRIAL_NONE;  // highest trial completed against this memory
    time_t LastCompletedTime    = 0;
};

class TC_GAME_API PathOfAscension
{
public:
    explicit PathOfAscension(Player* owner);

    // Difficulty.db2 id backing a trial (168-171), or 0 for ASCENSION_TRIAL_NONE / out of range.
    static uint32 GetTrialDifficultyId(AscensionTrial trial);
    static char const* GetTrialName(AscensionTrial trial);

    // --- lifecycle -------------------------------------------------------------------------------------
    void LoadFromDB(PreparedQueryResult result);
    void SaveToDB(CharacterDatabaseTransaction trans) const;
    // Driven from Garrison::Update's existing 60s tick. Nothing here is cast at the client (there is no client
    // protocol - see the header); it only trims state that a talent reset can invalidate.
    void Update();

    // --- state -----------------------------------------------------------------------------------------
    // Kyrian + at least tier 1 of tree 320 researched.
    bool IsAccessible() const;
    // Researched tiers of GarrTalentTree 320 (0-5).
    uint32 GetResearchedTiers() const;
    // Memories the sanctum can hold: 0 / 6 / 10 / 10 / 10 / 10 by researched tier (talents 1091 + 1092).
    uint32 GetMemoryCapacity() const;
    // Highest trial the sanctum can offer at all at the current research. Per-MEMORY availability is content
    // and lives in the world table; this is only the ceiling the talents state.
    AscensionTrial GetMaxTrial() const;
    // Weekly Path of Ascension quests unlocked: 0 below tier 2, 1 from tier 2, 2 from tier 4 (talents
    // 1092 + 1094). The quests themselves are ordinary quests already in `integ_world` (63181-63192).
    uint32 GetWeeklyQuestSlots() const;
    // Braziers lit in the Ascension Coliseum: Lessons Learned at tier 3, Inward Reflection at tier 5. Both
    // "increase your soulbinds' health and damage" - the MAGNITUDE is published nowhere and is not invented
    // here; this only reports how many are active so an encounter script can apply an authored value.
    uint32 GetActiveBraziers() const;

    bool HasMemory(uint32 memoryId) const;
    AscensionMemory const* GetMemory(uint32 memoryId) const;
    std::vector<AscensionMemory const*> GetMemories() const;
    // Highest trial won against a memory (ASCENSION_TRIAL_NONE when uncaptured or never beaten).
    AscensionTrial GetHighestTrialWon(uint32 memoryId) const;
    // Total trial wins across every memory - what the "Defeat N bosses in the Path of Ascension" achievements
    // count (Achievement.db2: 1, 3, 5, 7, 12, 16, 20, 24, 39).
    uint32 GetTotalTrialWins() const;

    // True when `trial` is open for `memoryId` right now: the memory is captured, the world row offers that
    // trial, and enough tiers are researched for it.
    bool IsTrialAvailable(uint32 memoryId, AscensionTrial trial) const;

    // --- actions ---------------------------------------------------------------------------------------
    // Capture a Shadowlands memory. Validates covenant, research tier, capacity and the authored row. Persists.
    PathOfAscensionError CaptureMemory(uint32 memoryId);

    // Check every gate for entering the Ascension Coliseum with this memory at this trial. Returns
    // ASCENSION_ERROR_NO_ARENA_CONTENT when scenario 1803 / map 2375 is unauthored, so nothing ever "starts"
    // a challenge that cannot be finished. Deliberately makes no state change of its own - a run is only ever
    // recorded by CompleteTrial, when it has actually been won.
    PathOfAscensionError StartTrial(uint32 memoryId, AscensionTrial trial) const;

    // Record a won trial: raises the memory's high-water mark and awards kill credit on the memory's creature
    // so the existing QuestSortID -595 quests pay their own rewards. Persists. Refuses everything StartTrial
    // refuses except the arena-content check, so a real scenario completion can call it directly.
    PathOfAscensionError CompleteTrial(uint32 memoryId, AscensionTrial trial);

private:
    // The unique-feature tree of the owner's covenant: GarrTypeID 111 + FeatureTypeIndex 5 (SanctumUnique) +
    // FeatureSubtypeIndex == CovenantID. For the Kyrian that resolves to tree 320. Resolved from the DB2
    // stores rather than hardcoded. Returns 0 when the owner is not Kyrian.
    uint32 GetAscensionTreeId() const;
    // Shared gate for StartTrial/CompleteTrial. Does not test arena content.
    PathOfAscensionError CheckTrial(uint32 memoryId, AscensionTrial trial) const;
    void MarkChanged() { _needsSave = true; }

    Player* _owner;
    std::unordered_map<uint32 /*memoryId*/, AscensionMemory> _memories;
    bool _needsSave = false;
};

#endif // PathOfAscension_h__
