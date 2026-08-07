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

#ifndef AbominationFactory_h__
#define AbominationFactory_h__

#include "DatabaseEnvFwd.h"
#include "Define.h"
#include <ctime>
#include <unordered_map>
#include <vector>

class Player;

/*
 * The Abomination Factory - the Necrolord covenant's unique sanctum feature (P6.2 of the covenant plan).
 *
 * WHAT THE DATA SAYS THIS IS. Every id below is read out of 12.0.7.68275 client DB2 or `integ_world`; the
 * "NOT DERIVABLE" block at the bottom lists the single thing that deliberately is not.
 *
 *   GarrTalentTree 321 "Abomination Factory"
 *       GarrTypeID 111 (covenant sanctum), MaxTiers 5, Flags 16, CurrencyID 0,
 *       FeatureTypeIndex 5 (= GARR_TALENT_FEATURE_UNIQUE, client Enum.GarrTalentFeatureType.SanctumUnique),
 *       FeatureSubtypeIndex 4 (= CovenantID, client Enum.GarrTalentFeatureSubtype.Maldraxxus).
 *
 *   Its five talents are an unlock ladder, not perks - every GarrTalentRank.PerkSpellID is 0 (ranks 1399-1403),
 *   exactly like the Night Fae tree 319. Their own descriptions say what a tier does, and they say it in the
 *   feature's own vocabulary:
 *       1096 "Build a Buddy"         Tier 0 - "grants you the FIRST RANK OF ABOMINABLE STITCHING. Allowing you to
 *                                             build your own constructs! Welcome Chordy and a variety of other
 *                                             useful constructs..."           (PlayerConditionID 84025)
 *       1097 "Crafting Limbs"        Tier 1 - "the SECOND RANK of prowess in Abominable Stitching... build
 *                                             additional constructs... a new battle pet, a construct disguise
 *                                             and other useful consumables."
 *       1098 "Bring Them to Life"    Tier 2 - "the THIRD RANK... call in Emeni... additional constructs"
 *       1099 "Forged Friends"        Tier 3 - "the FOURTH RANK... two dangerous high-end constructs, as well as
 *                                             a bag of treats to lure Twigin"
 *       1100 "Best Fiends Forever"   Tier 4 - "the FINAL RANK... the Soulfused Construct... Bonesewn Fleshroc.
 *                                             Marshal your entire stable of constructs to earn the rarest prizes."
 *   Research costs (GarrTalentRank 1399-1403 + GarrTalentCost 4424-4428 / 6553-6568, which agree):
 *       T1 1500 x1813 + 6 x1810 @3600s, T2 5000 + 12 @43200s, T3 10000 + 22 @86400s,
 *       T4 12500 + 40 @86400s, T5 15000 + 70 @86400s.  (1813 Reservoir Anima, 1810 Redeemed Soul.)
 *   The generic Garrison::LearnTalent/ResearchTalent engine already charges and times these; this class does
 *   not re-implement any of it, it only reads the resulting talent ranks.
 *
 *   "RANK OF ABOMINABLE STITCHING" IS LITERAL, AND IT IS A REAL SKILL LINE.
 *       SkillLine 2787 "Abominable Stitching" (AlternateVerb "Stitcher", CategoryID 11,
 *       Description "Parts can be found on monsters in the Shadowlands, and from quests.") - already declared
 *       in this core as SKILL_ABOMINABLE_STITCHING (SharedDefines.h). Its rank IS the researched tier count,
 *       proven by client data rather than inferred: CriteriaTree 87447/87448 "Abominable Stitching to Rank 1"
 *       -> Criteria 49374, Type 198 (learn garrison talent), Asset **1096** = the tier-0 talent of tree 321.
 *
 *   THE ACTIVITY IS A TRADESKILL, NOT A BESPOKE UI.
 *       66 SkillLineAbility rows carry SkillLine 2787, grouped by TradeSkillCategory children of 1478
 *       "Abominable Stitching": 1477 Basic Constructs (12), 1479 Specialized Constructs (3), 1480 Consumables (9),
 *       1481 Components (2), 1524 Specialty Items (3), 1523/1534/1535/1533 Fashion Accessories (37).
 *       All 66 have MinSkillLineRank 1 and AcquireMethod 3 = NeverLearned (except spell 325453 "Flytrap",
 *       AcquireMethod 0 = Learned) - i.e. Player::LearnSkillRewardedSpells deliberately skips every one of them
 *       and THE SERVER IS EXPECTED TO TEACH THEM EXPLICITLY. That is what RefreshSkillAndRecipes below does.
 *       Each recipe's cost is real published data (SpellReagents), e.g.
 *           325284 Construct Body: "Chordy"        <- 1 x 183743 Malleable Flesh
 *           325451 Construct Body: "Roseboil"      <- 10 x 178061 Malleable Flesh + 1 x 183744 Superior Parts
 *           338037 Construct Body: "Iron Phillip"  <- 20 x 178061 + 20 x 171828 Laestrite Ore + 10 x 183744
 *                                                     + 1 x 183475 Indomitable Hide
 *           338043 Construct Body: "Naxx"          <- 20 x 178061 + 12 x 173202 Shrouded Cloth + 10 x 183744
 *                                                     + 1 x 183519 Necromantic Oil
 *       Those reagents are taken by the ordinary Spell::TakeReagents path; this class never re-charges them.
 *
 *   THE "STABLE" IS THE 15 CONSTRUCT BODIES. A construct-body recipe is identified from data, not from a
 *   hardcoded list: among the skill-2787 abilities it is exactly the set whose spell carries
 *   SPELL_EFFECT_KILL_CREDIT (creature credit 167076 "Chordy's Body" / 167581 "Build an additional Construct").
 *   That effect appears on 15 of the 66 and on nothing else in the skill. The client's own roster agrees -
 *   CriteriaTree 88195 "Abominable Stitching - Build Abom" lists Chordy, Flytrap, Marz, Atticus, Roseboil,
 *   Sabrina, Toothpick, The Professor, Gas Bag, Guillotine, Mama Tomalin, Naxx, Iron Phillip, Miru Soulblossom,
 *   Neena (+ "Unity", which is a campaign quest, not a recipe).
 *   Built constructs are persisted in `character_garrison_abomination_factory`, one row per construct, so the
 *   stable survives a restart and a covenant switch back to the Necrolords.
 *
 *   THE DAILIES ARE ORDINARY QUESTS GIVEN BY THE CONSTRUCTS THEMSELVES. CriteriaTree 87819 "Abominable
 *   Stitching - Things To Do While You're Dead" enumerates 31 of them, and every one already exists in
 *   `integ_world.quest_template` under QuestSortID -593 (QUEST_SORT_ABOMINABLE_STITCHING, SharedDefines.h)
 *   with its questgiver already set to the construct creature: Chordy 161270, Mama Tomalin 161678,
 *   Flytrap 158300, Marz 158301, Naxx 158298, The Professor 159198, Iron Phillip 159199, Toothpick 159212,
 *   Guillotine 159214, Sabrina 159226, Atticus 159238, Gas Bag 159240, Roseboil 159241, Miru Soulblossom 167877.
 *   So the daily layer needs NO engine at all - it needs those creatures spawned in the Stitchyard, which is
 *   world content (see the gap list below).
 *
 * NOT DERIVABLE OFFLINE - deliberately left as data, never guessed:
 *   * WHICH of the 66 recipes each of the five ranks unlocks. SkillLineAbility.MinSkillLineRank is 1 on all 66,
 *     TradeSkillCategory groups them by kind (heads/backs/consumables) and not by rank, no PlayerCondition in
 *     the build references talents 1096-1100, and GarrTalentRank.PerkSpellID is 0. The talent descriptions say
 *     how many arrive per tier in prose but never name them.
 *   Rather than invent that mapping it is a column of the world table `garrison_abomination_recipe`. The engine
 *   below is complete and runs off that table; with the table empty the skill line is still granted and ranked
 *   (that part is derived), but no recipe is taught and BuildConstruct answers
 *   ABOMINATION_FACTORY_ERROR_NO_RECIPE_DATA. Authoring rows turns it on with no code change.
 */

enum AbominationFactoryConstants : uint32
{
    // GarrTalentTree.FeatureSubtypeIndex of tree 321 / client Enum.GarrTalentFeatureSubtype.Maldraxxus.
    COVENANT_ID_NECROLORD                   = 4,
    // MaxTiers of GarrTalentTree 321 = the highest rank Abominable Stitching (SkillLine 2787) can reach.
    ABOMINATION_FACTORY_MAX_RANK            = 5
};

enum AbominationFactoryError : uint32
{
    ABOMINATION_FACTORY_OK = 0,
    ABOMINATION_FACTORY_ERROR_NOT_NECROLORD,    // owner is not pledged to the Necrolords
    ABOMINATION_FACTORY_ERROR_NOT_UNLOCKED,     // GarrTalentTree 321 tier 1 (talent 1096 "Build a Buddy") not researched
    ABOMINATION_FACTORY_ERROR_UNKNOWN_RECIPE,   // spell is not a SkillLineAbility of SkillLine 2787
    ABOMINATION_FACTORY_ERROR_NOT_A_CONSTRUCT,  // a stitching recipe, but not one of the 15 construct bodies
    ABOMINATION_FACTORY_ERROR_NO_RECIPE_DATA,   // world table `garrison_abomination_recipe` has no row for it
    ABOMINATION_FACTORY_ERROR_RANK_TOO_LOW,     // the recipe needs more researched tiers than are held
    ABOMINATION_FACTORY_ERROR_ALREADY_BUILT     // that construct is already in the stable
};

// One authored recipe gate. The recipe set itself comes from SkillLineAbility (client data); only the rank that
// unlocks it is content, so that is the only thing this row carries.
struct AbominationRecipeTemplate
{
    uint32 RecipeSpellId    = 0;    // SkillLineAbility.Spell of a SkillLine 2787 row
    uint8  RequiredRank     = 1;    // researched tiers of tree 321 needed before it is taught (1-5)
};

// One construct in the stable.
struct AbominationConstruct
{
    uint32 RecipeSpellId = 0;   // the "Construct Body: X" recipe that produced it
    time_t BuiltTime     = 0;
};

class TC_GAME_API AbominationFactory
{
public:
    explicit AbominationFactory(Player* owner);

    // --- lifecycle -------------------------------------------------------------------------------------
    void LoadFromDB(PreparedQueryResult result);
    void SaveToDB(CharacterDatabaseTransaction trans) const;
    // Re-syncs SkillLine 2787 and the taught recipe set to the current researched tier count. Driven from
    // Garrison::Update's existing 60s tick, so a tier that completed while offline lights up on the first tick.
    void Update();

    // --- state -----------------------------------------------------------------------------------------
    // Necrolord + at least tier 1 of tree 321 researched.
    bool IsAccessible() const;
    // Researched tiers of GarrTalentTree 321 = the rank of Abominable Stitching (see header comment).
    uint32 GetRank() const;
    bool HasConstruct(uint32 recipeSpellId) const;
    std::vector<AbominationConstruct const*> GetConstructs() const;

    // Grants / ranks / strips SkillLine 2787 and teaches or unteaches the authored recipes for the current rank.
    // Safe to call at any time; a no-op for anyone who is not a Necrolord with a researched Abomination Factory.
    void RefreshSkillAndRecipes();

    // --- actions ---------------------------------------------------------------------------------------
    // Records a construct body in the stable. Called from the spell path when the owner successfully casts a
    // construct-body recipe (the reagents have already been taken by Spell::TakeReagents at that point), and
    // from the GM command. Persists.
    AbominationFactoryError BuildConstruct(uint32 recipeSpellId);

private:
    // The unique-feature tree of the owner's covenant: GarrTypeID 111 + FeatureTypeIndex 5 (SanctumUnique) +
    // FeatureSubtypeIndex == CovenantID. For the Necrolords that resolves to tree 321. Resolved from the DB2
    // stores rather than hardcoded. Returns 0 when the owner is not a Necrolord.
    uint32 GetFactoryTreeId() const;
    void MarkChanged() { _needsSave = true; }

    Player* _owner;
    std::unordered_map<uint32 /*recipeSpellId*/, AbominationConstruct> _constructs;
    // Last rank pushed to SkillLine 2787, so the 60s tick only touches skills and spells when something moved.
    uint32 _appliedRank = 0;
    bool _needsSave = false;
};

#endif // AbominationFactory_h__
