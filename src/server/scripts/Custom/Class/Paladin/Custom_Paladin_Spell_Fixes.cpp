/*
 * This file is part of the Stefal WoW Project.
 * It is designed to work exclusively with the TrinityCore framework.
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
 * This code is provided for personal and educational use within the
 * Stefal WoW Project. It is not intended for commercial distribution,
 * resale, or any form of monetization.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "SpellHistory.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "SpellScript.h"
#include "Unit.h"

#include "Custom_Paladin_Defines.h"

namespace Scripts::Custom::Paladin
{
    // 406064 - Art of War
    // EFFECT_0: base proc chance (15%) on melee attacks to reset Blade of Justice.
    // EFFECT_1: extra chance (10%) added on critical strikes.
    // Blade of Justice is charge-based (ChargeCategory 2128) in 12.1, so RestoreCharge is used
    // instead of ResetCooldown.
    class spell_pal_art_of_war_custom : public AuraScript
    {
        bool Validate(SpellInfo const* spellInfo) override
        {
            return ValidateSpellInfo({ Spells::ArtOfWarTriggered, Spells::BladeOfJustice })
                && ValidateSpellEffect({ { spellInfo->Id, EFFECT_1 } });
        }

        bool CheckProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
        {
            int32 chance = aurEff->GetAmountAsInt();
            if (eventInfo.GetHitMask() & PROC_HIT_CRITICAL)
                if (AuraEffect const* critBonus = GetEffect(EFFECT_1))
                    chance += critBonus->GetAmountAsInt();

            return roll_chance(chance);
        }

        bool CheckCritBonus(AuraEffect const* /*aurEff*/, ProcEventInfo& /*eventInfo*/)
        {
            // EFFECT_1 is only the crit chance bonus; it must never count as the proccing effect.
            return false;
        }

        void HandleProc(AuraEffect* /*aurEff*/, ProcEventInfo& eventInfo)
        {
            PreventDefaultAction();

            Unit* target = GetTarget();
            target->GetSpellHistory()->RestoreCharge(
                sSpellMgr->AssertSpellInfo(Spells::BladeOfJustice, GetCastDifficulty())->ChargeCategoryId);

            target->CastSpell(target, Spells::ArtOfWarTriggered, CastSpellExtraArgsInit
            {
                .TriggerFlags = TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR,
                .TriggeringSpell = eventInfo.GetProcSpell()
            });
        }

        void Register() override
        {
            DoCheckEffectProc += AuraCheckEffectProcFn(spell_pal_art_of_war_custom::CheckProc, EFFECT_0, SPELL_AURA_DUMMY);
            DoCheckEffectProc += AuraCheckEffectProcFn(spell_pal_art_of_war_custom::CheckCritBonus, EFFECT_1, SPELL_AURA_DUMMY);
            OnEffectProc += AuraEffectProcFn(spell_pal_art_of_war_custom::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
        }
    };

    // 402912 - Righteous Cause
    // Each Holy Power spent has a 6% chance to reset Blade of Justice.
    // The core's auto-generated proc entry rolls flat 6% regardless of HP spent.
    // spell_proc Chance=100 ensures only the script's per-HP roll matters.
    class spell_pal_righteous_cause_custom : public AuraScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ Spells::ArtOfWarTriggered, Spells::BladeOfJustice, Spells::BladeOfJusticeAoE });
        }

        bool CheckProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
        {
            Spell const* procSpell = eventInfo.GetProcSpell();
            if (!procSpell || !procSpell->HasPowerTypeCost(POWER_HOLY_POWER))
                return false;

            Optional<int32> holyPowerCost = procSpell->GetPowerTypeCostAmount(POWER_HOLY_POWER);
            if (!holyPowerCost || *holyPowerCost <= 0)
                return false;

            return roll_chance(aurEff->GetAmountAsInt() * *holyPowerCost);
        }

        void HandleProc(AuraEffect* /*aurEff*/, ProcEventInfo& eventInfo)
        {
            PreventDefaultAction();

            Unit* target = GetTarget();
            SpellHistory* history = target->GetSpellHistory();

            auto restoreCharge = [history](uint32 spellId, Difficulty difficulty)
            {
                if (SpellInfo const* info = sSpellMgr->GetSpellInfo(spellId, difficulty))
                    if (info->ChargeCategoryId)
                        history->RestoreCharge(info->ChargeCategoryId);
            };

            restoreCharge(Spells::BladeOfJustice, GetCastDifficulty());
            restoreCharge(Spells::BladeOfJusticeAoE, GetCastDifficulty());

            target->CastSpell(target, Spells::ArtOfWarTriggered, CastSpellExtraArgsInit
            {
                .TriggerFlags = TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR,
                .TriggeringSpell = eventInfo.GetProcSpell()
            });
        }

        void Register() override
        {
            DoCheckEffectProc += AuraCheckEffectProcFn(spell_pal_righteous_cause_custom::CheckProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
            OnEffectProc += AuraEffectProcFn(spell_pal_righteous_cause_custom::HandleProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
            DoCheckEffectProc += AuraCheckEffectProcFn(spell_pal_righteous_cause_custom::CheckProc, EFFECT_0, SPELL_AURA_DUMMY);
            OnEffectProc += AuraEffectProcFn(spell_pal_righteous_cause_custom::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
        }
    };

    // Called by 184575 - Blade of Justice
    // 404834 - Consecrated Blade: cast Consecration at the target's location (ICD 407475, 10s)
    // 382275 - Consecrated Blade (Art of War variant): bypasses ICD when 231843 is active
    // Consecration creates its AreaTrigger via AuraEffect::HandleCreateAreaTrigger at the aura
    // bearer's position, so we temporarily relocate the caster to the target's position.
    class spell_pal_consecrated_blade_custom : public SpellScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ Spells::Consecration, Spells::ConsecratedBladeICD });
        }

        bool Load() override
        {
            Unit* caster = GetCaster();
            return caster->HasAura(Spells::ConsecratedBlade)
                || caster->HasAura(Spells::ConsecratedBladeOverride)
                || caster->HasAura(Spells::ConsecratedBladeArtOfWar);
        }

        void HandleAfterCast() const
        {
            Unit* caster = GetCaster();
            Unit* target = GetExplTargetUnit();
            if (!caster || !target)
                return;

            bool const hasTalent = caster->HasAura(Spells::ConsecratedBlade)
                || caster->HasAura(Spells::ConsecratedBladeOverride);
            bool const hasArtOfWarVariant = caster->HasAura(Spells::ConsecratedBladeArtOfWar);

            if (!hasTalent && !hasArtOfWarVariant)
                return;

            // Art of War variant: bypass ICD when the Art of War proc buff is active
            if (hasArtOfWarVariant && caster->HasAura(Spells::ArtOfWarTriggered))
            {
                CastConsecrationAtTarget(caster, target);
                return;
            }

            // Normal Consecrated Blade: respect 10s ICD
            if (hasTalent && !caster->HasAura(Spells::ConsecratedBladeICD))
            {
                CastConsecrationAtTarget(caster, target);

                caster->CastSpell(caster, Spells::ConsecratedBladeICD, CastSpellExtraArgsInit
                {
                    .TriggerFlags = TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR,
                    .TriggeringSpell = GetSpell()
                });
            }
        }

        void CastConsecrationAtTarget(Unit* caster, Unit* target) const
        {
            Position const home = caster->GetPosition();
            Position dest = target->GetPosition();
            caster->UpdateAllowedPositionZ(dest.m_positionX, dest.m_positionY, dest.m_positionZ);
            caster->Relocate(dest);

            caster->CastSpell(caster, Spells::Consecration, CastSpellExtraArgsInit
            {
                .TriggerFlags = TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_IGNORE_SPELL_AND_CATEGORY_CD
                    | TRIGGERED_IGNORE_GCD | TRIGGERED_DONT_REPORT_CAST_ERROR,
                .TriggeringSpell = GetSpell()
            });

            caster->Relocate(home);
        }

        void Register() override
        {
            AfterCast += SpellCastFn(spell_pal_consecrated_blade_custom::HandleAfterCast);
        }
    };
}

void AddSC_custom_paladin_spell_fixes()
{
    using namespace Scripts::Custom::Paladin;

    RegisterSpellScript(spell_pal_art_of_war_custom);
    RegisterSpellScript(spell_pal_righteous_cause_custom);
    RegisterSpellScript(spell_pal_consecrated_blade_custom);
}
