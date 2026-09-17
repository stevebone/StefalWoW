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

#include "ArchaeologyMgr.h"
#include "GameObject.h"
#include "GameObjectAI.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellPackets.h"
#include "SpellScript.h"
#include <any>
#include <optional>
#include <vector>

struct go_archaeology_find : public GameObjectAI
{
    go_archaeology_find(GameObject* gameObject) : GameObjectAI(gameObject) { }

    bool OnGossipHello(Player* player) override
    {
        // Private-object visibility is the first boundary; retain an authoritative owner/token check
        // in case a stale or forced-visible object is used.
        return !player->CanUseArchaeologyFind(me);
    }

    void OnLootStateChanged(uint32 state, Unit* /*unit*/) override
    {
        if (state != GO_JUST_DEACTIVATED)
            return;

        if (Player* owner = ObjectAccessor::GetPlayer(*me, me->GetOwnerGUID()))
            owner->OnArchaeologyFindLooted(me);
    }
};

// 80451 - Survey
// Archaeology survey: the profession loop lives in Player::HandleArchaeologySurvey; this hook just
// runs it after the survey cast completes for a player caster.
class spell_archaeology_survey : public SpellScript
{
    void HandleAfterCast()
    {
        if (Player* player = GetCaster()->ToPlayer())
            player->HandleArchaeologySurvey();
    }

    void Register() override
    {
        AfterCast += SpellCastFn(spell_archaeology_survey::HandleAfterCast);
    }
};

// Research project solve spells (one per ResearchProject.db2 entry, bound via spell_script_names)
// Solving = casting the project's own SpellID. The spell's own effect creates the reward item; this
// script validates one DB2-backed resource plan, commits exactly that plan before effects, then
// records the completion and rolls the branch's next project once.
class spell_archaeology_solve : public SpellScript
{
    std::optional<ArchaeologySolvePlan> _solvePlan;
    bool _resourcesConsumed = false;
    bool _completed = false;

    SpellCastResult CheckCast()
    {
        Player* player = GetCaster()->ToPlayer();
        if (!player)
            return SPELL_FAILED_DONT_REPORT;

        if (!_solvePlan)
        {
            std::vector<WorldPackets::Spells::SpellWeight> const* weights =
                std::any_cast<std::vector<WorldPackets::Spells::SpellWeight>>(&GetSpell()->m_customArg);
            if (!weights)
                return SPELL_FAILED_DONT_REPORT;

            _solvePlan = sArchaeologyMgr->BuildSolvePlan(GetSpellInfo()->Id, *weights);
            if (!_solvePlan)
                return SPELL_FAILED_DONT_REPORT;
        }

        return player->CanSolveResearchProject(*_solvePlan) ? SPELL_CAST_OK : SPELL_FAILED_DONT_REPORT;
    }

    void HandleOnCast()
    {
        if (_solvePlan)
            if (Player* player = GetCaster()->ToPlayer())
                _resourcesConsumed = player->ConsumeResearchProjectSolveResources(*_solvePlan);
    }

    void GuardEffects(SpellEffIndex effectIndex)
    {
        if (!_resourcesConsumed)
            PreventHitDefaultEffect(effectIndex);
    }

    void HandleAfterCast()
    {
        if (!_resourcesConsumed || _completed || !_solvePlan)
            return;

        if (Player* player = GetCaster()->ToPlayer())
        {
            player->CompleteResearchProjectSolve(*_solvePlan);
            _completed = true;
        }
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_archaeology_solve::CheckCast);
        OnCast += SpellCastFn(spell_archaeology_solve::HandleOnCast);
        OnEffectHitTarget += SpellEffectFn(spell_archaeology_solve::GuardEffects, EFFECT_ALL, SPELL_EFFECT_ANY);
        AfterCast += SpellCastFn(spell_archaeology_solve::HandleAfterCast);
    }
};

void AddSC_archaeology_spell_scripts()
{
    RegisterGameObjectAI(go_archaeology_find);
    RegisterSpellScript(spell_archaeology_survey);
    RegisterSpellScript(spell_archaeology_solve);
}
