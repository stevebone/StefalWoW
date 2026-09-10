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

#include "AbominationFactory.h"
#include "DB2Stores.h"
#include "DB2Structure.h"
#include "DatabaseEnv.h"
#include "GameTime.h"
#include "Garrison.h"
#include "GarrisonMgr.h"
#include "Log.h"
#include "Player.h"
#include "SharedDefines.h"
#include "SpellMgr.h"
#include "SpellInfo.h"
#include <algorithm>

AbominationFactory::AbominationFactory(Player* owner) : _owner(owner)
{
}

uint32 AbominationFactory::GetFactoryTreeId() const
{
    if (!_owner || _owner->GetActiveCovenant() != COVENANT_ID_NECROLORD)
        return 0;

    if (std::vector<GarrTalentTreeEntry const*> const* trees = sGarrisonMgr.GetTalentTreesForGarrType(GARRISON_TYPE_COVENANT))
        for (GarrTalentTreeEntry const* tree : *trees)
            if (tree->FeatureTypeIndex == GARR_TALENT_FEATURE_UNIQUE && tree->FeatureSubtypeIndex == COVENANT_ID_NECROLORD)
                return tree->ID;

    return 0;
}

uint32 AbominationFactory::GetRank() const
{
    uint32 const treeId = GetFactoryTreeId();
    if (!treeId)
        return 0;

    Garrison const* garrison = _owner->GetGarrison(GARRISON_TYPE_COVENANT);
    if (!garrison)
        return 0;

    std::vector<GarrTalentEntry const*> const* talents = sGarrisonMgr.GetTalentsForTree(treeId);
    if (!talents)
        return 0;

    // One rank of Abominable Stitching per completed tier - CriteriaTree 87448 "Abominable Stitching to Rank 1"
    // resolves to Criteria 49374 (learn garrison talent, asset 1096 = the tier-0 talent of this tree). A talent
    // counts as completed at Rank >= 1, the same test the rest of the sanctum uses.
    uint32 unlocked = 0;
    for (GarrTalentEntry const* talent : *talents)
        if (Garrison::Talent const* owned = garrison->GetTalent(talent->ID))
            if (owned->Rank >= 1)
                ++unlocked;

    return std::min<uint32>(unlocked, ABOMINATION_FACTORY_MAX_RANK);
}

bool AbominationFactory::IsAccessible() const
{
    return GetRank() > 0;
}

bool AbominationFactory::HasConstruct(uint32 recipeSpellId) const
{
    return _constructs.find(recipeSpellId) != _constructs.end();
}

std::vector<AbominationConstruct const*> AbominationFactory::GetConstructs() const
{
    std::vector<AbominationConstruct const*> constructs;
    constructs.reserve(_constructs.size());
    for (auto const& [spellId, construct] : _constructs)
        constructs.push_back(&construct);

    std::sort(constructs.begin(), constructs.end(),
        [](AbominationConstruct const* l, AbominationConstruct const* r) { return l->BuiltTime < r->BuiltTime; });
    return constructs;
}

void AbominationFactory::RefreshSkillAndRecipes()
{
    if (!_owner)
        return;

    uint32 const rank = GetRank();
    bool const hasSkill = _owner->HasSkill(SKILL_ABOMINABLE_STITCHING);

    // Nothing moved since the last pass - skip the whole thing so the 60s garrison tick does not walk the skill
    // line for every character that will never be a Necrolord.
    if (rank == _appliedRank && (rank ? hasSkill : !hasSkill))
        return;

    std::vector<SkillLineAbilityEntry const*> const* abilities = sDB2Manager.GetSkillLineAbilitiesBySkill(SKILL_ABOMINABLE_STITCHING);

    if (!rank)
    {
        // Tree 321 is unresearched, or the character left the Necrolords: take the skill line and every recipe
        // this feature taught back off them. Only authored recipes are touched - anything the character got by
        // another route is left alone.
        if (abilities)
        {
            for (SkillLineAbilityEntry const* ability : *abilities)
                if (sGarrisonMgr.GetAbominationRecipeRank(ability->Spell) && _owner->HasSpell(ability->Spell))
                    _owner->RemoveSpell(ability->Spell);
        }

        if (hasSkill)
            _owner->SetSkill(SKILL_ABOMINABLE_STITCHING, 0, 0, 0);

        _appliedRank = 0;
        return;
    }

    // SkillLine 2787 "Abominable Stitching". Step and value are both the researched tier count; the ceiling is
    // GarrTalentTree 321's MaxTiers.
    _owner->SetSkill(SKILL_ABOMINABLE_STITCHING, uint16(rank), uint16(rank), uint16(ABOMINATION_FACTORY_MAX_RANK));

    // SkillLine 2787 is CategoryID 11 (profession), so Player::SetSkill's generic path will hand it one of the
    // two primary profession tool slots if one is free. Abominable Stitching is a covenant feature with no
    // profession tools or accessories of its own, and taking a slot would cost the character a real profession's
    // tool slot - so hand it straight back. The skill itself stays in the skill list, which is what the
    // profession spellbook and the trade-skill window read.
    int32 const professionSlot = _owner->GetProfessionSlotFor(SKILL_ABOMINABLE_STITCHING);
    if (professionSlot != -1)
        _owner->SetProfessionSkillLine(uint32(professionSlot), 0);

    if (!abilities)
    {
        TC_LOG_ERROR("garrison", "AbominationFactory: SkillLine {} has no SkillLineAbility rows in this build; "
            "no stitching recipe can be taught.", uint32(SKILL_ABOMINABLE_STITCHING));
        _appliedRank = rank;
        return;
    }

    // Teach what this rank has earned and take back anything above it (a talent reset can lower the rank). The
    // recipe SET is client data; only the rank that unlocks each recipe is authored - see AbominationFactory.h.
    uint32 taught = 0;
    for (SkillLineAbilityEntry const* ability : *abilities)
    {
        uint8 const requiredRank = sGarrisonMgr.GetAbominationRecipeRank(ability->Spell);
        if (!requiredRank)
            continue;   // unauthored: never taught, never removed

        if (requiredRank <= rank)
        {
            if (!_owner->HasSpell(ability->Spell))
            {
                _owner->LearnSpell(ability->Spell, false, SKILL_ABOMINABLE_STITCHING);
                ++taught;
            }
        }
        else if (_owner->HasSpell(ability->Spell))
            _owner->RemoveSpell(ability->Spell);
    }

    TC_LOG_DEBUG("garrison", "AbominationFactory: player {} is at Abominable Stitching rank {} ({} recipe(s) newly taught).",
        _owner->GetGUID().ToString(), rank, taught);

    _appliedRank = rank;
}

void AbominationFactory::Update()
{
    RefreshSkillAndRecipes();
}

AbominationFactoryError AbominationFactory::BuildConstruct(uint32 recipeSpellId)
{
    if (!_owner || _owner->GetActiveCovenant() != COVENANT_ID_NECROLORD)
        return ABOMINATION_FACTORY_ERROR_NOT_NECROLORD;

    uint32 const rank = GetRank();
    if (!rank)
        return ABOMINATION_FACTORY_ERROR_NOT_UNLOCKED;

    if (!sGarrisonMgr.IsAbominationStitchingRecipe(recipeSpellId))
        return ABOMINATION_FACTORY_ERROR_UNKNOWN_RECIPE;

    if (!sGarrisonMgr.IsAbominationConstructRecipe(recipeSpellId))
        return ABOMINATION_FACTORY_ERROR_NOT_A_CONSTRUCT;

    uint8 const requiredRank = sGarrisonMgr.GetAbominationRecipeRank(recipeSpellId);
    if (!requiredRank)
        return ABOMINATION_FACTORY_ERROR_NO_RECIPE_DATA;

    if (rank < requiredRank)
        return ABOMINATION_FACTORY_ERROR_RANK_TOO_LOW;

    if (HasConstruct(recipeSpellId))
        return ABOMINATION_FACTORY_ERROR_ALREADY_BUILT;

    AbominationConstruct& construct = _constructs[recipeSpellId];
    construct.RecipeSpellId = recipeSpellId;
    construct.BuiltTime = GameTime::GetGameTime();
    MarkChanged();

    TC_LOG_DEBUG("garrison", "AbominationFactory: player {} added construct {} to the stable ({} total).",
        _owner->GetGUID().ToString(), recipeSpellId, uint32(_constructs.size()));

    return ABOMINATION_FACTORY_OK;
}

void AbominationFactory::LoadFromDB(PreparedQueryResult result)
{
    _constructs.clear();
    if (!result)
        return;

    do
    {
        Field* fields = result->Fetch();
        uint32 const recipeSpellId = fields[0].GetUInt32();
        if (!sGarrisonMgr.IsAbominationConstructRecipe(recipeSpellId))
        {
            TC_LOG_ERROR("garrison", "AbominationFactory: dropping stored construct {} for player {} - it is not a "
                "construct-body recipe of SkillLine {}.", recipeSpellId, _owner->GetGUID().ToString(),
                uint32(SKILL_ABOMINABLE_STITCHING));
            continue;
        }

        AbominationConstruct& construct = _constructs[recipeSpellId];
        construct.RecipeSpellId = recipeSpellId;
        construct.BuiltTime = fields[1].GetInt64();
    } while (result->NextRow());
}

void AbominationFactory::SaveToDB(CharacterDatabaseTransaction trans) const
{
    // Nothing was ever built and nothing changed - skip the delete/insert churn.
    if (!_needsSave && _constructs.empty())
        return;

    CharacterDatabasePreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHARACTER_GARRISON_ABOMINATION);
    stmt->setUInt64(0, _owner->GetGUID().GetCounter());
    trans->Append(stmt);

    for (auto const& [recipeSpellId, construct] : _constructs)
    {
        stmt = CharacterDatabase.GetPreparedStatement(CHAR_INS_CHARACTER_GARRISON_ABOMINATION);
        stmt->setUInt64(0, _owner->GetGUID().GetCounter());
        stmt->setUInt32(1, construct.RecipeSpellId);
        stmt->setInt64(2, construct.BuiltTime);
        trans->Append(stmt);
    }
}
