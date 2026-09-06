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

#include "CellImpl.h"
#include "CommonPredicates.h"
#include "Containers.h"
#include "DB2Stores.h"
#include "GridNotifiers.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "Random.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "SpellHistory.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "SpellScript.h"
#include "Unit.h"

#include <any>

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
        }
    };

    // 114852 - Holy Prism (Damage)
    // 114871 - Holy Prism (Heal)
    // Fixed: EFFECT_2 target type must match DBC (ENTRY for 114852, ENEMY for 114871)
    class spell_pal_holy_prism_selector : public SpellScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ Spells::HolyPrismTargetAlly, Spells::HolyPrismBeamVisual });
        }

        void SaveTargetGuid(SpellEffIndex /*effIndex*/)
        {
            _targetGUID = GetHitUnit()->GetGUID();
        }

        void FilterTargets(std::list<WorldObject*>& targets)
        {
            uint8 const maxTargets = 5;

            if (targets.size() > maxTargets)
            {
                if (GetSpellInfo()->Id == Spells::HolyPrismTargetAlly)
                {
                    targets.sort(Trinity::Predicates::HealthPctOrderPred());
                    targets.resize(maxTargets);
                }
                else
                    Trinity::Containers::RandomResize(targets, maxTargets);
            }

            _sharedTargets = targets;
        }

        void ShareTargets(std::list<WorldObject*>& targets)
        {
            targets = _sharedTargets;
        }

        void HandleScript(SpellEffIndex /*effIndex*/)
        {
            if (Unit* initialTarget = ObjectAccessor::GetUnit(*GetCaster(), _targetGUID))
                initialTarget->CastSpell(GetHitUnit(), Spells::HolyPrismBeamVisual, true);
        }

        void Register() override
        {
            if (m_scriptSpellId == Spells::HolyPrismTargetEnemy)
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_pal_holy_prism_selector::FilterTargets, EFFECT_1, TARGET_UNIT_DEST_AREA_ALLY);
            else if (m_scriptSpellId == Spells::HolyPrismTargetAlly)
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_pal_holy_prism_selector::FilterTargets, EFFECT_1, TARGET_UNIT_DEST_AREA_ENEMY);

            if (m_scriptSpellId == Spells::HolyPrismTargetEnemy)
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_pal_holy_prism_selector::ShareTargets, EFFECT_2, TARGET_UNIT_DEST_AREA_ENTRY);
            else if (m_scriptSpellId == Spells::HolyPrismTargetAlly)
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_pal_holy_prism_selector::ShareTargets, EFFECT_2, TARGET_UNIT_DEST_AREA_ENEMY);

            OnEffectHitTarget += SpellEffectFn(spell_pal_holy_prism_selector::SaveTargetGuid, EFFECT_0, SPELL_EFFECT_ANY);
            OnEffectHitTarget += SpellEffectFn(spell_pal_holy_prism_selector::HandleScript, EFFECT_2, SPELL_EFFECT_SCRIPT_EFFECT);
        }

    private:
        std::list<WorldObject*> _sharedTargets;
        ObjectGuid _targetGUID;
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

    // =========================================================================
    // Divine Storm / Empyrean Power / Tempest of the Lightbringer
    // =========================================================================

    namespace PaladinTempest
    {
        struct WaveData
        {
            int32 EffectivenessPct = 20;
        };

        static void ApplyWaveEffectiveness(Spell const* spell, float& pctMod)
        {
            if (WaveData const* wave = std::any_cast<WaveData>(&spell->m_customArg))
                ApplyPct(pctMod, wave->EffectivenessPct);
        }
    }

    // 53385 - Divine Storm
    // Overrides native spell_pal_divine_storm to handle:
    // - Visual kit
    // - Empyrean Power buff consumption
    // - Tempest of the Lightbringer frontal cone wave
    // - Empyrean Legacy bonus damage
    class spell_pal_divine_storm_custom : public SpellScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return sSpellVisualKitStore.HasRecord(Spells::VisualKitDivineStorm)
                && ValidateSpellInfo(
                {
                    Spells::TempestOfTheLightbringer,
                    Spells::TempestOfTheLightbringerAT,
                    Spells::DivineStormDamage,
                    Spells::EmpyreanPowerBuff
                });
        }

        void HandleOnCast()
        {
            GetCaster()->SendPlaySpellVisualKit(Spells::VisualKitDivineStorm, 0, 0);
        }

        void HandleDamage(SpellEffectInfo const& /*spellEffectInfo*/, Unit const* /*victim*/, int32& /*damage*/, int32& /*flatMod*/, float& pctMod) const
        {
            if (int32 const* bonusPct = std::any_cast<int32>(&GetSpell()->m_customArg))
                AddPct(pctMod, *bonusPct);
        }

        void HandleAfterCast()
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            // Empyrean Power buff is consumed by the free Divine Storm.
            if (Aura* empyreanPower = caster->GetAura(Spells::EmpyreanPowerBuff))
                empyreanPower->Remove();

            AuraEffect const* tempestEff = caster->GetAuraEffect(Spells::TempestOfTheLightbringer, EFFECT_0);
            if (!tempestEff)
                return;

            int32 const rangeYards = std::max(tempestEff->GetAmountAsInt(), 1);
            int32 effectivenessPct = 20;
            if (AuraEffect const* pctEff = caster->GetAuraEffect(Spells::TempestOfTheLightbringer, EFFECT_1))
                effectivenessPct = std::max(pctEff->GetAmountAsInt(), 1);

            // Projectile / wave visual (AreaTrigger).
            caster->CastSpell(caster, Spells::TempestOfTheLightbringerAT, CastSpellExtraArgsInit{
                .TriggerFlags = TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR,
                .TriggeringSpell = GetSpell()
            });

            std::list<Unit*> targets;
            Trinity::AnyUnfriendlyUnitInObjectRangeCheck check(caster, caster, float(rangeYards));
            Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(caster, targets, check);
            Cell::VisitAllObjects(caster, searcher, float(rangeYards));

            for (Unit* target : targets)
            {
                if (!caster->IsValidAttackTarget(target) || !caster->isInFront(target))
                    continue;

                caster->CastSpell(target, Spells::DivineStormDamage, CastSpellExtraArgsInit{
                    .TriggerFlags = TRIGGERED_IGNORE_GCD | TRIGGERED_IGNORE_POWER_COST | TRIGGERED_IGNORE_CAST_IN_PROGRESS
                        | TRIGGERED_DONT_REPORT_CAST_ERROR | TRIGGERED_IGNORE_SPELL_AND_CATEGORY_CD,
                    .TriggeringSpell = GetSpell(),
                    .CustomArg = PaladinTempest::WaveData{ .EffectivenessPct = effectivenessPct }
                });
            }
        }

        void Register() override
        {
            OnCast += SpellCastFn(spell_pal_divine_storm_custom::HandleOnCast);
            AfterCast += SpellCastFn(spell_pal_divine_storm_custom::HandleAfterCast);
            CalcDamage += SpellCalcDamageFn(spell_pal_divine_storm_custom::HandleDamage);
        }
    };

    // 224239 - Divine Storm (damage)
    // Applies Empyrean Legacy bonus and Tempest wave effectiveness to damage.
    class spell_pal_divine_storm_damage_custom : public SpellScript
    {
        void HandleDamage(SpellEffectInfo const& /*spellEffectInfo*/, Unit const* /*victim*/, int32& /*damage*/, int32& /*flatMod*/, float& pctMod) const
        {
            if (int32 const* bonusPct = std::any_cast<int32>(&GetSpell()->m_customArg))
                AddPct(pctMod, *bonusPct);

            PaladinTempest::ApplyWaveEffectiveness(GetSpell(), pctMod);
        }

        void Register() override
        {
            CalcDamage += SpellCalcDamageFn(spell_pal_divine_storm_damage_custom::HandleDamage);
        }
    };

    // =========================================================================
    // Empyrean Power
    // =========================================================================

    // 326732 - Empyrean Power: CS / Templar Strike / Templar Slash / Crusading Strikes
    // can grant a free empowered Divine Storm.
    class spell_pal_empyrean_power_custom : public AuraScript
    {
        ObjectGuid _lastAttemptCastId;

        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo(
            {
                Spells::EmpyreanPowerBuff,
                Spells::CrusaderStrike,
                Spells::CrusadingStrikesTalent,
                Spells::CrusadingStrikesDamage,
                Spells::TemplarStrikesTalent,
                Spells::TemplarStrike,
                Spells::TemplarSlash
            });
        }

        static bool HasCrusadingStrikesTalent(Unit const* unit)
        {
            if (unit->HasAura(Spells::CrusadingStrikesTalent))
                return true;

            if (Player const* player = unit->ToPlayer())
                return player->HasSpell(Spells::CrusadingStrikesTalent);

            return false;
        }

        static bool IsCrusaderStrikeFamilySpell(uint32 spellId)
        {
            switch (spellId)
            {
                case Spells::CrusaderStrike:
                case Spells::TemplarStrike:
                case Spells::TemplarSlash:
                    return true;
                default:
                    return false;
            }
        }

        bool CheckProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
        {
            if (aurEff->GetEffIndex() != EFFECT_0 && aurEff->GetEffIndex() != EFFECT_1)
                return false;

            Spell const* procSpell = eventInfo.GetProcSpell();
            SpellInfo const* spellInfo = eventInfo.GetSpellInfo();
            Unit* caster = GetTarget();

            int32 chance = 0;
            if (spellInfo && IsCrusaderStrikeFamilySpell(spellInfo->Id))
            {
                if (aurEff->GetEffIndex() != EFFECT_0)
                    return false;
                chance = GetEffectInfo(EFFECT_0).CalcValueAsInt(caster);
            }
            else if (spellInfo && spellInfo->Id == Spells::CrusadingStrikesDamage
                && HasCrusadingStrikesTalent(caster))
            {
                if (aurEff->GetEffIndex() != EFFECT_1)
                    return false;
                chance = GetEffectInfo(EFFECT_0).CalcValueAsInt(caster);
            }
            else
                return false;

            if (chance <= 0)
                return false;

            // AoE / multi-hit: only one roll per cast. White swings have no cast id - roll each swing.
            if (procSpell)
            {
                if (procSpell->m_castId == _lastAttemptCastId)
                    return false;
                _lastAttemptCastId = procSpell->m_castId;
            }

            return roll_chance(chance);
        }

        void HandleProc(AuraEffect* /*aurEff*/, ProcEventInfo& eventInfo)
        {
            PreventDefaultAction();
            eventInfo.GetActor()->CastSpell(eventInfo.GetActor(), Spells::EmpyreanPowerBuff, CastSpellExtraArgsInit{
                .TriggerFlags = TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR,
                .TriggeringSpell = eventInfo.GetProcSpell()
            });
        }

        void Register() override
        {
            DoCheckEffectProc += AuraCheckEffectProcFn(spell_pal_empyrean_power_custom::CheckProc, EFFECT_0, SPELL_AURA_DUMMY);
            OnEffectProc += AuraEffectProcFn(spell_pal_empyrean_power_custom::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
            DoCheckEffectProc += AuraCheckEffectProcFn(spell_pal_empyrean_power_custom::CheckProc, EFFECT_1, SPELL_AURA_DUMMY);
            OnEffectProc += AuraEffectProcFn(spell_pal_empyrean_power_custom::HandleProc, EFFECT_1, SPELL_AURA_DUMMY);
        }
    };

    // =========================================================================
    // Empyrean Legacy
    // =========================================================================

    namespace PaladinEmpyreanLegacy
    {
        static bool IsAvengingWrathSpell(uint32 spellId)
        {
            switch (spellId)
            {
                case Spells::AvengingWrath:  // Avenging Wrath
                case Spells::AvengingWrathRet: // Avenging Wrath (Ret version)
                case Spells::AvengingWrath3:
                case Spells::AvengingWrath4:
                    return true;
                default:
                    return false;
            }
        }

        static bool IsSingleTargetHolyPowerSpender(uint32 spellId)
        {
            switch (spellId)
            {
                case Spells::TemplarsVerdict:
                case Spells::FinalVerdictCast:
                case Spells::JusticarsVengeance:
                    return true;
                default:
                    return false;
            }
        }

        static void TryTriggerDivineStorm(Unit* caster, int32 bonusPct)
        {
            if (!caster)
                return;

            caster->CastSpell(caster, Spells::DivineStorm, CastSpellExtraArgsInit{
                .TriggerFlags = TRIGGERED_IGNORE_GCD | TRIGGERED_IGNORE_POWER_COST | TRIGGERED_IGNORE_CAST_IN_PROGRESS
                    | TRIGGERED_DONT_REPORT_CAST_ERROR | TRIGGERED_IGNORE_SPELL_AND_CATEGORY_CD,
                .CustomArg = bonusPct
            });
        }
    }

    // 387170 - Empyrean Legacy: Avenging Wrath procs grant an empowered DS on next ST HP spender.
    class spell_pal_empyrean_legacy_custom : public AuraScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo(
            {
                Spells::EmpyreanLegacyBuff,
                Spells::EmpyreanLegacyICD
            });
        }

        bool CheckProc(ProcEventInfo& eventInfo)
        {
            SpellInfo const* spellInfo = eventInfo.GetSpellInfo();
            if (!spellInfo || !PaladinEmpyreanLegacy::IsAvengingWrathSpell(spellInfo->Id))
                return false;

            Unit* caster = eventInfo.GetActor();
            if (!caster || caster->HasAura(Spells::EmpyreanLegacyICD))
                return false;

            return true;
        }

        void HandleProc(AuraEffect* /*aurEff*/, ProcEventInfo& eventInfo)
        {
            PreventDefaultAction();

            Unit* caster = eventInfo.GetActor();
            if (!caster)
                return;

            caster->CastSpell(caster, Spells::EmpyreanLegacyBuff, CastSpellExtraArgsInit{
                .TriggerFlags = TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR,
                .TriggeringSpell = eventInfo.GetProcSpell()
            });

            caster->CastSpell(caster, Spells::EmpyreanLegacyICD, CastSpellExtraArgsInit{
                .TriggerFlags = TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR,
                .TriggeringSpell = eventInfo.GetProcSpell()
            });
        }

        void Register() override
        {
            DoCheckProc += AuraCheckProcFn(spell_pal_empyrean_legacy_custom::CheckProc);
            OnEffectProc += AuraEffectProcFn(spell_pal_empyrean_legacy_custom::HandleProc, EFFECT_0, SPELL_AURA_PROC_TRIGGER_SPELL);
        }
    };

    // 387178 - Empyrean Legacy (buff): consumed by next ST HP spender, triggers empowered DS.
    class spell_pal_empyrean_legacy_buff_custom : public AuraScript
    {
        bool Validate(SpellInfo const* spellInfo) override
        {
            return ValidateSpellInfo({ Spells::DivineStorm })
                && ValidateSpellEffect({ { spellInfo->Id, EFFECT_0 } });
        }

        bool CheckProc(AuraEffect const* /*aurEff*/, ProcEventInfo& eventInfo)
        {
            Spell const* procSpell = eventInfo.GetProcSpell();
            if (!procSpell || !procSpell->HasPowerTypeCost(POWER_HOLY_POWER))
                return false;

            SpellInfo const* spellInfo = eventInfo.GetSpellInfo();
            if (!spellInfo || !PaladinEmpyreanLegacy::IsSingleTargetHolyPowerSpender(spellInfo->Id))
                return false;

            return true;
        }

        void HandleProc(AuraEffect* aurEff, ProcEventInfo& eventInfo)
        {
            PreventDefaultAction();

            Unit* caster = eventInfo.GetActor();
            if (!caster)
                return;

            int32 bonusPct = aurEff->GetAmountAsInt();
            PaladinEmpyreanLegacy::TryTriggerDivineStorm(caster, bonusPct);

            Remove();
        }

        void Register() override
        {
            DoCheckEffectProc += AuraCheckEffectProcFn(spell_pal_empyrean_legacy_buff_custom::CheckProc, EFFECT_0, SPELL_AURA_DUMMY);
            OnEffectProc += AuraEffectProcFn(spell_pal_empyrean_legacy_buff_custom::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
        }
    };

    // Fallback: ST HP spenders consume Empyrean Legacy buff via AfterCast if proc system misses it.
    class spell_pal_empyrean_legacy_spender_custom : public SpellScript
    {
        bool Load() override
        {
            return GetCaster()->HasAura(Spells::EmpyreanLegacyBuff);
        }

        void HandleAfterCast()
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            if (Aura* legacyBuff = caster->GetAura(Spells::EmpyreanLegacyBuff))
            {
                int32 bonusPct = legacyBuff->GetEffect(EFFECT_0)->GetAmountAsInt();
                PaladinEmpyreanLegacy::TryTriggerDivineStorm(caster, bonusPct);
                legacyBuff->Remove();
            }
        }

        void Register() override
        {
            AfterCast += SpellCastFn(spell_pal_empyrean_legacy_spender_custom::HandleAfterCast);
        }
    };

    // =========================================================================
    // Second Sunrise
    // =========================================================================

    namespace PaladinSecondSunrise
    {
        struct EchoData
        {
            int32 EffectivenessPct = 100;
        };

        static bool IsEchoableSpell(uint32 spellId)
        {
            switch (spellId)
            {
                case Spells::DivineStorm:
                case Spells::HammerOfWrath:
                case Spells::HammerOfWrathLegacy:
                case Spells::HolyShock:
                case Spells::LightOfDawn:
                    return true;
                default:
                    return false;
            }
        }

        static void ApplyEffectiveness(Spell const* spell, float& pctMod)
        {
            if (EchoData const* echo = std::any_cast<EchoData>(&spell->m_customArg))
                ApplyPct(pctMod, echo->EffectivenessPct);
        }

        static void TryEcho(Unit* caster, Spell const* procSpell, AuraEffect const* aurEff)
        {
            if (!caster || !procSpell || !aurEff)
                return;

            Aura const* aura = aurEff->GetBase();
            if (!aura)
                return;

            int32 effectivenessPct = aura->GetSpellInfo()->GetEffect(EFFECT_1).CalcValueAsInt(caster);

            uint32 const spellId = procSpell->GetSpellInfo()->Id;
            ObjectGuid targetGuid;
            switch (spellId)
            {
                case Spells::DivineStorm:
                case Spells::LightOfDawn:
                    targetGuid = caster->GetGUID();
                    break;
                default:
                    if (Unit* target = procSpell->m_targets.GetUnitTarget())
                        targetGuid = target->GetGUID();
                    else
                        targetGuid = caster->GetGUID();
                    break;
            }

            caster->m_Events.AddEventAtOffset([caster, targetGuid, spellId, effectivenessPct]()
            {
                Unit* target = ObjectAccessor::GetUnit(*caster, targetGuid);
                if (!target)
                    return;

                caster->CastSpell(target, spellId, CastSpellExtraArgsInit{
                    .TriggerFlags = TRIGGERED_IGNORE_GCD | TRIGGERED_IGNORE_POWER_COST | TRIGGERED_IGNORE_CAST_IN_PROGRESS
                        | TRIGGERED_DONT_REPORT_CAST_ERROR | TRIGGERED_IGNORE_SPELL_AND_CATEGORY_CD,
                    .CustomArg = EchoData{ .EffectivenessPct = effectivenessPct }
                });
            }, 200ms);
        }
    }

    // 431474 - Second Sunrise: DS/HoW (Ret) and Holy Shock/Light of Dawn (Holy)
    // have a chance to cast again at reduced effectiveness.
    class spell_pal_second_sunrise_custom : public AuraScript
    {
        ObjectGuid _lastAttemptCastId;

        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo(
            {
                Spells::DivineStorm,
                Spells::HammerOfWrath,
                Spells::HammerOfWrathLegacy,
                Spells::HolyShock,
                Spells::LightOfDawn
            });
        }

        bool CheckProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
        {
            if (aurEff->GetEffIndex() != EFFECT_0)
                return false;

            Spell const* procSpell = eventInfo.GetProcSpell();
            if (!procSpell || !PaladinSecondSunrise::IsEchoableSpell(procSpell->GetSpellInfo()->Id))
                return false;

            // Echoes must not echo again.
            if (std::any_cast<PaladinSecondSunrise::EchoData>(&procSpell->m_customArg))
                return false;

            // Divine Storm / Light of Dawn hit multiple targets; only roll once per cast.
            if (procSpell->m_castId == _lastAttemptCastId)
                return false;
            _lastAttemptCastId = procSpell->m_castId;

            return roll_chance(GetEffectInfo(EFFECT_0).CalcValueAsInt(GetTarget()));
        }

        void HandleProc(AuraEffect* aurEff, ProcEventInfo& eventInfo)
        {
            PreventDefaultAction();
            PaladinSecondSunrise::TryEcho(GetTarget(), eventInfo.GetProcSpell(), aurEff);
        }

        void Register() override
        {
            DoCheckEffectProc += AuraCheckEffectProcFn(spell_pal_second_sunrise_custom::CheckProc, EFFECT_0, SPELL_AURA_DUMMY);
            OnEffectProc += AuraEffectProcFn(spell_pal_second_sunrise_custom::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
        }
    };

    // Applies Second Sunrise echo effectiveness to damage spells.
    class spell_pal_second_sunrise_effectiveness_damage : public SpellScript
    {
        void HandleDamage(SpellEffectInfo const& /*spellEffectInfo*/, Unit const* /*victim*/, int32& /*damage*/, int32& /*flatMod*/, float& pctMod) const
        {
            PaladinSecondSunrise::ApplyEffectiveness(GetSpell(), pctMod);
        }

        void Register() override
        {
            CalcDamage += SpellCalcDamageFn(spell_pal_second_sunrise_effectiveness_damage::HandleDamage);
        }
    };

    // Applies Second Sunrise echo effectiveness to healing spells.
    class spell_pal_second_sunrise_effectiveness_healing : public SpellScript
    {
        void HandleHealing(SpellEffectInfo const& /*spellEffectInfo*/, Unit const* /*victim*/, int32& /*healing*/, int32& /*flatMod*/, float& pctMod) const
        {
            PaladinSecondSunrise::ApplyEffectiveness(GetSpell(), pctMod);
        }

        void Register() override
        {
            CalcHealing += SpellCalcHealingFn(spell_pal_second_sunrise_effectiveness_healing::HandleHealing);
        }
    };

    // =========================================================================
    // Holy Shock (override for Second Sunrise CustomArg forwarding)
    // =========================================================================

    // 20473 - Holy Shock
    // Overrides native spell_pal_holy_shock to forward m_customArg to damage/heal spells
    // for Second Sunrise effectiveness.
    class spell_pal_holy_shock_custom : public SpellScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo
            ({
                Spells::HolyShock,
                Spells::HolyShockHealing,
                Spells::HolyShockDamage
            });
        }

        SpellCastResult CheckCast()
        {
            Unit* caster = GetCaster();

            if (Unit* target = GetExplTargetUnit())
            {
                if (!caster->IsFriendlyTo(target))
                {
                    if (!caster->IsValidAttackTarget(target))
                        return SPELL_FAILED_BAD_TARGETS;

                    if (!caster->isInFront(target))
                        return SPELL_FAILED_UNIT_NOT_INFRONT;
                }
            }
            else
                return SPELL_FAILED_BAD_TARGETS;

            return SPELL_CAST_OK;
        }

        void HandleDummy(SpellEffIndex /*effIndex*/)
        {
            Unit* caster = GetCaster();

            if (Unit* unitTarget = GetHitUnit())
            {
                uint32 const shockId = caster->IsFriendlyTo(unitTarget)
                    ? Spells::HolyShockHealing
                    : Spells::HolyShockDamage;

                // Forward CustomArg so Second Sunrise effectiveness reaches damage/heal spells.
                caster->CastSpell(unitTarget, shockId, CastSpellExtraArgsInit{
                    .TriggerFlags = TRIGGERED_FULL_MASK,
                    .TriggeringSpell = GetSpell(),
                    .CustomArg = GetSpell()->m_customArg
                });
            }
        }

        void Register() override
        {
            OnCheckCast += SpellCheckCastFn(spell_pal_holy_shock_custom::CheckCast);
            OnEffectHitTarget += SpellEffectFn(spell_pal_holy_shock_custom::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
        }
    };

    // =========================================================================
    // Dawnlight (Herald of the Sun hero talent)
    // =========================================================================

    namespace PaladinDawnlight
    {
        // Get radiation percentage based on spec: 8% Holy, 4% Retribution
        static int32 GetRadiationPct(Unit* caster)
        {
            if (caster && caster->IsPlayer())
            {
                auto spec = caster->ToPlayer()->GetPrimarySpecialization();
                if (spec == ChrSpecialization::PaladinRetribution)
                    return 4;
            }
            return 8;
        }

        // Get max targets before split reduction from 431581 EFFECT_1
        static int32 GetMaxTargets(Unit* caster)
        {
            if (AuraEffect const* eff = caster->GetAuraEffect(Spells::DawnlightRadiateMeta, EFFECT_1))
                return std::max(eff->GetAmountAsInt(), 1);
            return 5;
        }

        // Get Sun's Avatar soft cap: 5 Holy, 8 Retribution
        static int32 GetSoftCap(Unit* caster)
        {
            if (caster && caster->IsPlayer())
            {
                auto spec = caster->ToPlayer()->GetPrimarySpecialization();
                if (spec == ChrSpecialization::PaladinRetribution)
                    return 8;
            }
            return 5;
        }

        // Get Sun's Avatar link range from 431425 EFFECT_7 (default 30 yards)
        static float GetLinkRange(Unit* caster)
        {
            if (AuraEffect const* eff = caster->GetAuraEffect(Spells::SunsAvatar, EFFECT_7))
                return float(eff->GetAmountAsInt());
            return 30.0f;
        }
    }

    // 431522 - Dawnlight (charges aura)
    // Procs on Holy Power spending abilities at HIT phase. Applies Dawnlight DoT/HoT to target.
    // Also applies 431581 (radiate meta) when charges are gained, since nothing natively casts it.
    class spell_pal_dawnlight_charges : public AuraScript
    {
        bool CheckEffectProc(AuraEffect const* /*aurEff*/, ProcEventInfo& eventInfo)
        {
            Spell const* procSpell = eventInfo.GetProcSpell();
            if (!procSpell || !procSpell->HasPowerTypeCost(POWER_HOLY_POWER))
                return false;

            if (!eventInfo.GetActionTarget())
                return false;

            return true;
        }

        void HandleProc(AuraEffect* /*aurEff*/, ProcEventInfo& eventInfo)
        {
            Unit* caster = eventInfo.GetActor();
            Unit* target = eventInfo.GetActionTarget();
            if (!caster || !target)
                return;

            if (caster->IsValidAttackTarget(target))
                caster->CastSpell(target, Spells::DawnlightDamage, CastSpellExtraArgsInit{
                    .TriggerFlags = TRIGGERED_FULL_MASK,
                    .TriggeringSpell = eventInfo.GetProcSpell()
                });
            else
                caster->CastSpell(target, Spells::DawnlightHeal, CastSpellExtraArgsInit{
                    .TriggerFlags = TRIGGERED_FULL_MASK,
                    .TriggeringSpell = eventInfo.GetProcSpell()
                });

            GetAura()->ModStackAmount(-1);
        }

        void HandleApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            Unit* target = GetTarget();
            if (!target)
                return;

            target->CastSpell(target, Spells::DawnlightRadiateMeta, CastSpellExtraArgsInit{
                .TriggerFlags = TRIGGERED_FULL_MASK
            });
        }

        void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            Unit* target = GetTarget();
            if (!target)
                return;

            target->RemoveAura(Spells::DawnlightRadiateMeta);
        }

        void Register() override
        {
            DoCheckEffectProc += AuraCheckEffectProcFn(spell_pal_dawnlight_charges::CheckEffectProc, EFFECT_FIRST_FOUND, SPELL_AURA_DUMMY);
            OnEffectProc += AuraEffectProcFn(spell_pal_dawnlight_charges::HandleProc, EFFECT_FIRST_FOUND, SPELL_AURA_DUMMY);
            AfterEffectApply += AuraEffectApplyFn(spell_pal_dawnlight_charges::HandleApply, EFFECT_FIRST_FOUND, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
            AfterEffectRemove += AuraEffectRemoveFn(spell_pal_dawnlight_charges::HandleRemove, EFFECT_FIRST_FOUND, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
        }
    };

    // 431482 - Morning Star (Herald of the Sun)
    // Every 5s, stacks Morning Star buff (431539) on caster. 2x rate out of combat.
    class spell_pal_morning_star : public AuraScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ Spells::MorningStarBuff });
        }

        void HandlePeriodic(AuraEffect const* /*aurEff*/)
        {
            Unit* target = GetTarget();
            if (!target)
                return;

            target->CastSpell(target, Spells::MorningStarBuff, CastSpellExtraArgsInit{
                .TriggerFlags = TRIGGERED_FULL_MASK
            });

            if (!target->IsInCombat())
                target->CastSpell(target, Spells::MorningStarBuff, CastSpellExtraArgsInit{
                    .TriggerFlags = TRIGGERED_FULL_MASK
                });
        }

        void Register() override
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_pal_morning_star::HandlePeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
        }
    };

    // 431406 - Will of the Dawn (Herald of the Sun)
    // 5% movement speed above 80% HP, 40% burst for 5s when brought below 35% HP (1min ICD).
    // Native EFFECT_2/3 PROC_TRIGGER_SPELL fire from everything, so block them and drive from 1s tick.
    class spell_pal_will_of_the_dawn : public AuraScript
    {
        bool _wasAbovePanicThreshold = true;

        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({
                Spells::WillOfTheDawnSpeed,
                Spells::WillOfTheDawnBurst,
                Spells::WillOfTheDawnIcd
            });
        }

        void HandleApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            Unit* target = GetTarget();
            if (!target)
                return;

            int32 const abovePct = GetEffect(EFFECT_0) ? GetEffect(EFFECT_0)->GetAmountAsInt() : 80;
            if (target->HealthAbovePct(abovePct))
                target->CastSpell(target, Spells::WillOfTheDawnSpeed, CastSpellExtraArgsInit{
                    .TriggerFlags = TRIGGERED_FULL_MASK
                });

            int32 const panicPct = GetEffect(EFFECT_2) ? GetEffect(EFFECT_2)->GetAmountAsInt() : 35;
            _wasAbovePanicThreshold = !target->HealthBelowPct(panicPct);
        }

        void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            if (Unit* target = GetTarget())
                target->RemoveAurasDueToSpell(Spells::WillOfTheDawnSpeed);
        }

        bool CheckProc(ProcEventInfo& /*eventInfo*/)
        {
            return false;
        }

        void HandlePeriodic(AuraEffect const* /*aurEff*/)
        {
            Unit* target = GetTarget();
            if (!target)
                return;

            int32 const abovePct = GetEffect(EFFECT_0) ? GetEffect(EFFECT_0)->GetAmountAsInt() : 80;
            if (target->HealthAbovePct(abovePct))
            {
                if (!target->HasAura(Spells::WillOfTheDawnSpeed))
                    target->CastSpell(target, Spells::WillOfTheDawnSpeed, CastSpellExtraArgsInit{
                        .TriggerFlags = TRIGGERED_FULL_MASK
                    });
            }
            else if (target->HasAura(Spells::WillOfTheDawnSpeed))
                target->RemoveAurasDueToSpell(Spells::WillOfTheDawnSpeed);

            int32 const panicPct = GetEffect(EFFECT_2) ? GetEffect(EFFECT_2)->GetAmountAsInt() : 35;
            bool const nowBelow = target->HealthBelowPct(panicPct);
            if (_wasAbovePanicThreshold && nowBelow && !target->HasAura(Spells::WillOfTheDawnIcd))
            {
                target->CastSpell(target, Spells::WillOfTheDawnBurst, CastSpellExtraArgsInit{
                    .TriggerFlags = TRIGGERED_FULL_MASK
                });
                target->CastSpell(target, Spells::WillOfTheDawnIcd, CastSpellExtraArgsInit{
                    .TriggerFlags = TRIGGERED_FULL_MASK
                });
            }
            _wasAbovePanicThreshold = !nowBelow;
        }

        void Register() override
        {
            AfterEffectApply += AuraEffectApplyFn(spell_pal_will_of_the_dawn::HandleApply, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
            AfterEffectRemove += AuraEffectRemoveFn(spell_pal_will_of_the_dawn::HandleRemove, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_pal_will_of_the_dawn::HandlePeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
            DoCheckProc += AuraCheckProcFn(spell_pal_will_of_the_dawn::CheckProc);
        }
    };

    // 431380 - Dawnlight (damage DoT)
    // On periodic tick, radiates a percentage of damage to nearby enemies.
    // Morning Star buff is consumed on apply and increases damage by 5% per stack.
    class spell_pal_dawnlight_damage_custom : public AuraScript
    {
        int32 _morningStarStacks = 0;

        void HandleApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            Unit* caster = GetCaster();
            Unit* target = GetTarget();
            if (!caster || !target)
                return;

            if (Aura* morningStar = caster->GetAura(Spells::MorningStarBuff))
            {
                _morningStarStacks = morningStar->GetStackAmount();
                caster->RemoveAura(Spells::MorningStarBuff);
            }

            if (caster->HasAura(Spells::SunsAvatar))
                caster->CastSpell(target, Spells::SunsAvatarLink, CastSpellExtraArgsInit{
                    .TriggerFlags = TRIGGERED_FULL_MASK
                });
        }

        void CalcDamageAndHealing(AuraEffect const* /*aurEff*/, Unit const* /*victim*/, int32& /*damageOrHealing*/, int32& /*flatMod*/, float& pctMod)
        {
            pctMod *= (1.0f + _morningStarStacks * 0.05f);
        }

        void HandlePeriodic(AuraEffect const* aurEff)
        {
            Unit* caster = GetCaster();
            Unit* target = GetTarget();
            if (!caster || !target)
                return;

            int32 radiationPct = PaladinDawnlight::GetRadiationPct(caster);
            double tickAmount = aurEff->GetAmount();
            double morningStarMod = 1.0 + _morningStarStacks * 0.05;
            double radiationAmount = tickAmount * radiationPct / 100.0 * morningStarMod;

            caster->CastSpell(target, Spells::DawnlightRadiationDamage, CastSpellExtraArgsInit{
                .TriggerFlags = TRIGGERED_FULL_MASK,
                .TriggeringSpell = nullptr,
                .CustomArg = radiationAmount
            });
        }

        void Register() override
        {
            AfterEffectApply += AuraEffectApplyFn(spell_pal_dawnlight_damage_custom::HandleApply, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE, AURA_EFFECT_HANDLE_REAL);
            DoEffectCalcDamageAndHealing += AuraEffectCalcDamageFn(spell_pal_dawnlight_damage_custom::CalcDamageAndHealing, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_pal_dawnlight_damage_custom::HandlePeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
        }
    };

    // 431381 - Dawnlight (healing HoT)
    // On periodic tick, radiates a percentage of healing to nearby allies.
    // Morning Star buff is consumed on apply and increases healing by 5% per stack.
    class spell_pal_dawnlight_heal_custom : public AuraScript
    {
        int32 _morningStarStacks = 0;

        void HandleApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            Unit* caster = GetCaster();
            Unit* target = GetTarget();
            if (!caster || !target)
                return;

            if (Aura* morningStar = caster->GetAura(Spells::MorningStarBuff))
            {
                _morningStarStacks = morningStar->GetStackAmount();
                caster->RemoveAura(Spells::MorningStarBuff);
            }

            if (caster->HasAura(Spells::SunsAvatar))
            {
                if (target == caster)
                    caster->CastSpell(caster, Spells::SunsAvatarSelfLink, CastSpellExtraArgsInit{
                        .TriggerFlags = TRIGGERED_FULL_MASK
                    });
                else
                    caster->CastSpell(target, Spells::SunsAvatarLink, CastSpellExtraArgsInit{
                        .TriggerFlags = TRIGGERED_FULL_MASK
                    });
            }
        }

        void CalcDamageAndHealing(AuraEffect const* /*aurEff*/, Unit const* /*victim*/, int32& /*damageOrHealing*/, int32& /*flatMod*/, float& pctMod)
        {
            pctMod *= (1.0f + _morningStarStacks * 0.05f);
        }

        void HandlePeriodic(AuraEffect const* aurEff)
        {
            Unit* caster = GetCaster();
            Unit* target = GetTarget();
            if (!caster || !target)
                return;

            int32 radiationPct = PaladinDawnlight::GetRadiationPct(caster);
            double tickAmount = aurEff->GetAmount();
            double morningStarMod = 1.0 + _morningStarStacks * 0.05;
            double radiationAmount = tickAmount * radiationPct / 100.0 * morningStarMod;

            caster->CastSpell(target, Spells::DawnlightRadiationHeal, CastSpellExtraArgsInit{
                .TriggerFlags = TRIGGERED_FULL_MASK,
                .TriggeringSpell = nullptr,
                .CustomArg = radiationAmount
            });
        }

        void Register() override
        {
            AfterEffectApply += AuraEffectApplyFn(spell_pal_dawnlight_heal_custom::HandleApply, EFFECT_0, SPELL_AURA_PERIODIC_HEAL, AURA_EFFECT_HANDLE_REAL);
            DoEffectCalcDamageAndHealing += AuraEffectCalcHealingFn(spell_pal_dawnlight_heal_custom::CalcDamageAndHealing, EFFECT_0, SPELL_AURA_PERIODIC_HEAL);
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_pal_dawnlight_heal_custom::HandlePeriodic, EFFECT_0, SPELL_AURA_PERIODIC_HEAL);
        }
    };

    // 431399 - Dawnlight (damage radiation)
    // AoE damage to enemies in 12 yards. Amount from m_customArg, split beyond 5 targets.
    class spell_pal_dawnlight_radiation_damage_custom : public SpellScript
    {
        void FilterTargets(std::list<WorldObject*>& targets)
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            int32 maxTargets = PaladinDawnlight::GetMaxTargets(caster);
            if (int32(targets.size()) > maxTargets)
                targets.resize(maxTargets);
        }

        void HandleDamage(SpellEffectInfo const& /*spellEffectInfo*/, Unit const* /*victim*/, int32& damage, int32& /*flatMod*/, float& /*pctMod*/) const
        {
            double const* radiationAmount = std::any_cast<double>(&GetSpell()->m_customArg);
            if (!radiationAmount)
                return;

            damage = int32(*radiationAmount);
        }

        void Register() override
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_pal_dawnlight_radiation_damage_custom::FilterTargets, EFFECT_0, TARGET_UNIT_DEST_AREA_ENEMY);
            CalcDamage += SpellCalcDamageFn(spell_pal_dawnlight_radiation_damage_custom::HandleDamage);
        }
    };

    // 431382 - Dawnlight (heal radiation)
    // AoE heal to allies in 12 yards. Amount from m_customArg, split beyond 5 targets.
    // Excludes targets with Beacon of Light from the caster.
    class spell_pal_dawnlight_radiation_heal_custom : public SpellScript
    {
        void FilterTargets(std::list<WorldObject*>& targets)
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            ObjectGuid casterGuid = caster->GetGUID();
            targets.remove_if([&casterGuid](WorldObject* obj)
            {
                Unit* unit = obj->ToUnit();
                if (!unit)
                    return true;
                if (unit->HasAura(Spells::BeaconOfLight, casterGuid))
                    return true;
                return false;
            });

            int32 maxTargets = PaladinDawnlight::GetMaxTargets(caster);
            if (int32(targets.size()) > maxTargets)
                targets.resize(maxTargets);
        }

        void HandleHealing(SpellEffectInfo const& /*spellEffectInfo*/, Unit const* /*victim*/, int32& heal, int32& /*flatMod*/, float& /*pctMod*/) const
        {
            double const* radiationAmount = std::any_cast<double>(&GetSpell()->m_customArg);
            if (!radiationAmount)
                return;

            heal = int32(*radiationAmount);
        }

        void Register() override
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_pal_dawnlight_radiation_heal_custom::FilterTargets, EFFECT_0, TARGET_UNIT_DEST_AREA_ALLY);
            CalcHealing += SpellCalcHealingFn(spell_pal_dawnlight_radiation_heal_custom::HandleHealing);
        }
    };

    // =========================================================================
    // Sun's Avatar (Herald of the Sun hero talent)
    // =========================================================================

    // 431907 - Sun's Avatar Link (applied to each Dawnlight target)
    // On each 0.5s tick, casts beam damage/heal from caster to this target's position.
    // Self-removes when Dawnlight expires or caster moves out of range.
    class spell_pal_suns_avatar_link : public AuraScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({
                Spells::SunsAvatarDamage,
                Spells::SunsAvatarHeal,
                Spells::DawnlightDamage,
                Spells::DawnlightHeal
            });
        }

        void HandlePeriodic(AuraEffect const* /*aurEff*/)
        {
            Unit* target = GetTarget();
            Unit* caster = GetCaster();
            if (!target || !caster)
            {
                Remove();
                return;
            }

            float linkRange = PaladinDawnlight::GetLinkRange(caster);
            if (!caster->IsWithinDist(target, linkRange))
                return;

            ObjectGuid casterGuid = caster->GetGUID();
            if (!target->HasAura(Spells::DawnlightDamage, casterGuid) &&
                !target->HasAura(Spells::DawnlightHeal, casterGuid))
            {
                Remove();
                return;
            }

            caster->CastSpell({ target->GetPosition() }, Spells::SunsAvatarDamage, CastSpellExtraArgsInit{
                .TriggerFlags = TRIGGERED_FULL_MASK
            });
            caster->CastSpell({ target->GetPosition() }, Spells::SunsAvatarHeal, CastSpellExtraArgsInit{
                .TriggerFlags = TRIGGERED_FULL_MASK
            });
        }

        void Register() override
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_pal_suns_avatar_link::HandlePeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
        }
    };

    // 463073 - Sun's Avatar Self Link (applied to caster when Dawnlight is on self)
    // On each 0.5s tick, casts 5-yard AoE damage around caster.
    // Self-removes when self-cast Dawnlight expires.
    class spell_pal_suns_avatar_self_link : public AuraScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({
                Spells::SunsAvatarSelfDamage,
                Spells::DawnlightHeal
            });
        }

        void HandlePeriodic(AuraEffect const* /*aurEff*/)
        {
            Unit* target = GetTarget();
            Unit* caster = GetCaster();
            if (!target || !caster || target != caster)
            {
                Remove();
                return;
            }

            if (!caster->HasAura(Spells::DawnlightHeal, caster->GetGUID()))
            {
                Remove();
                return;
            }

            caster->CastSpell(caster, Spells::SunsAvatarSelfDamage, CastSpellExtraArgsInit{
                .TriggerFlags = TRIGGERED_FULL_MASK
            });
        }

        void Register() override
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_pal_suns_avatar_self_link::HandlePeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
        }
    };

    // 431911 - Sun's Avatar Beam Damage
    // Line damage to enemies in beam from caster to Dawnlight target. Soft cap 5/8 targets.
    class spell_pal_suns_avatar_damage : public SpellScript
    {
        void FilterTargets(std::list<WorldObject*>& targets)
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            int32 maxTargets = PaladinDawnlight::GetSoftCap(caster);
            if (int32(targets.size()) > maxTargets)
                targets.resize(maxTargets);
        }

        void Register() override
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_pal_suns_avatar_damage::FilterTargets, EFFECT_0, TARGET_UNIT_LINE_CASTER_TO_DEST_ENEMY);
        }
    };

    // 431939 - Sun's Avatar Beam Heal
    // Line healing to allies in beam from caster to Dawnlight target. Soft cap 5/8 targets.
    class spell_pal_suns_avatar_heal : public SpellScript
    {
        void FilterTargets(std::list<WorldObject*>& targets)
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            int32 maxTargets = PaladinDawnlight::GetSoftCap(caster);
            if (int32(targets.size()) > maxTargets)
                targets.resize(maxTargets);
        }

        void Register() override
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_pal_suns_avatar_heal::FilterTargets, EFFECT_0, TARGET_UNIT_LINE_CASTER_TO_DEST_ALLY);
        }
    };

    // 463075 - Sun's Avatar Self Radiate Damage
    // 5-yard AoE damage around caster. Soft cap 5/8 targets.
    class spell_pal_suns_avatar_self_damage : public SpellScript
    {
        void FilterTargets(std::list<WorldObject*>& targets)
        {
            Unit* caster = GetCaster();
            if (!caster)
                return;

            int32 maxTargets = PaladinDawnlight::GetSoftCap(caster);
            if (int32(targets.size()) > maxTargets)
                targets.resize(maxTargets);
        }

        void Register() override
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_pal_suns_avatar_self_damage::FilterTargets, EFFECT_0, TARGET_UNIT_DEST_AREA_ENEMY);
        }
    };

    // =========================================================================
    // Judge, Jury and Executioner (406157)
    // EFFECT_0 (+5% spender damage) is DBC spellmod (aura 108).
    // EFFECT_1 PROC_TRIGGER_SPELL -> 1253174 has unreliable ProcTypeMask on our core;
    // grant the buff from Execution Sentence cast, then refund on next HP spender.
    // =========================================================================

    // 406157 - talent: swallow DBC proc (we drive the buff from ES cast).
    class spell_pal_judge_jury_executioner : public AuraScript
    {
        bool CheckProc(ProcEventInfo& /*eventInfo*/)
        {
            return false;
        }

        void Register() override
        {
            DoCheckProc += AuraCheckProcFn(spell_pal_judge_jury_executioner::CheckProc);
        }
    };

    // 343527 - Execution Sentence: grant refund buff when JJ&E is talented.
    class spell_pal_judge_jury_execution_sentence : public SpellScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ Spells::JudgeJuryExecutioner, Spells::JudgeJuryExecutionerBuff });
        }

        void HandleAfterCast()
        {
            Unit* caster = GetCaster();
            if (!caster || !caster->HasAura(Spells::JudgeJuryExecutioner))
                return;

            caster->CastSpell(caster, Spells::JudgeJuryExecutionerBuff, CastSpellExtraArgsInit{
                .TriggerFlags = TRIGGERED_IGNORE_CAST_IN_PROGRESS | TRIGGERED_DONT_REPORT_CAST_ERROR,
                .TriggeringSpell = GetSpell()
            });
        }

        void Register() override
        {
            AfterCast += SpellCastFn(spell_pal_judge_jury_execution_sentence::HandleAfterCast);
        }
    };

    // 1253174 - JJ&E buff: next Holy Power ability refunds its cost (1 charge).
    class spell_pal_judge_jury_executioner_buff : public AuraScript
    {
        bool CheckProc(AuraEffect const* /*aurEff*/, ProcEventInfo& eventInfo)
        {
            Spell const* procSpell = eventInfo.GetProcSpell();
            if (!procSpell || !procSpell->HasPowerTypeCost(POWER_HOLY_POWER))
                return false;

            SpellInfo const* info = procSpell->GetSpellInfo();
            if (!info)
                return false;

            Optional<SpellPowerCost> cost = info->CalcPowerCost(POWER_HOLY_POWER, false, eventInfo.GetActor(), eventInfo.GetSchoolMask());
            return cost && cost->Amount > 0;
        }

        void HandleProc(AuraEffect* /*aurEff*/, ProcEventInfo& eventInfo)
        {
            PreventDefaultAction();

            Unit* actor = eventInfo.GetActor();
            Spell const* procSpell = eventInfo.GetProcSpell();
            if (!actor || !procSpell)
                return;

            SpellInfo const* info = procSpell->GetSpellInfo();
            if (!info)
                return;

            Optional<SpellPowerCost> cost = info->CalcPowerCost(POWER_HOLY_POWER, false, actor, eventInfo.GetSchoolMask());
            if (!cost || cost->Amount <= 0)
                return;

            actor->ModifyPower(POWER_HOLY_POWER, cost->Amount);
            Remove();
        }

        void Register() override
        {
            DoCheckEffectProc += AuraCheckEffectProcFn(spell_pal_judge_jury_executioner_buff::CheckProc, EFFECT_0, SPELL_AURA_DUMMY);
            OnEffectProc += AuraEffectProcFn(spell_pal_judge_jury_executioner_buff::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
        }
    };

    // 53651 - Light's Beacon (Beacon of Light) override
    // Prevents Dawnlight healing (431381, 431382) from transferring to Beacon of Light target.
    class spell_pal_light_s_beacon_custom : public AuraScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ Spells::BeaconOfLight, Spells::BeaconOfLightHeal });
        }

        bool CheckProc(ProcEventInfo& eventInfo)
        {
            if (!eventInfo.GetActionTarget())
                return false;

            if (eventInfo.GetActionTarget()->HasAura(Spells::BeaconOfLight, eventInfo.GetActor()->GetGUID()))
                return false;

            SpellInfo const* procSpell = eventInfo.GetSpellInfo();
            if (procSpell && (procSpell->Id == Spells::DawnlightHeal || procSpell->Id == Spells::DawnlightRadiationHeal))
                return false;

            HealInfo* healInfo = eventInfo.GetHealInfo();
            return healInfo && healInfo->GetHeal();
        }

        void HandleProc(AuraEffect* aurEff, ProcEventInfo& eventInfo)
        {
            PreventDefaultAction();

            SpellEffectValue heal = CalculatePct(eventInfo.GetHealInfo()->GetHeal(), aurEff->GetAmount());

            Unit::AuraList const& auras = GetCaster()->GetSingleCastAuras();
            for (Unit::AuraList::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
            {
                if ((*itr)->GetId() == Spells::BeaconOfLight)
                {
                    std::vector<AuraApplication*> applications;
                    (*itr)->GetApplicationVector(applications);
                    if (!applications.empty())
                    {
                        CastSpellExtraArgs args(aurEff);
                        args.AddSpellMod(SPELLVALUE_BASE_POINT0, heal);
                        eventInfo.GetActor()->CastSpell(applications.front()->GetTarget(), Spells::BeaconOfLightHeal, args);
                    }
                    return;
                }
            }
        }

        void Register() override
        {
            DoCheckProc += AuraCheckProcFn(spell_pal_light_s_beacon_custom::CheckProc);
            OnEffectProc += AuraEffectProcFn(spell_pal_light_s_beacon_custom::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
        }
    };

    // 383344 - Expurgation
    // Passive aura that should only proc from Blade of Justice (184575) and its AOE (404358).
    // DB2 EffectSpellClassMask is 0, so the auto-generated proc entry has no family filter.
    // spell_proc Chance=100 + CheckProc filters by exact spell ID.
    class spell_pal_expurgation_custom : public AuraScript
    {
        bool CheckProc(ProcEventInfo& eventInfo)
        {
            SpellInfo const* procSpell = eventInfo.GetSpellInfo();
            if (!procSpell)
                return false;

            return procSpell->Id == Spells::BladeOfJustice
                || procSpell->Id == Spells::BladeOfJusticeAoE;
        }

        void Register() override
        {
            DoCheckProc += AuraCheckProcFn(spell_pal_expurgation_custom::CheckProc);
        }
    };

    // =========================================================================
    // Hammer of Wrath Talent (1241288)
    // Passive talent: replaces Judgment with Hammer of Wrath during Avenging Wrath.
    // Spec -> Judgment -> HoW mapping (from DB2 override auras 1241410 + 1277026):
    //   Ret (20271)  -> 24275
    //   Prot (275779) -> 1241413
    //   Holy (275773) -> 1241413
    // =========================================================================

    namespace PaladinHammerOfWrath
    {
        static uint32 GetJudgmentSpellId(Player const* player)
        {
            switch (player->GetPrimarySpecialization())
            {
                case ChrSpecialization::PaladinRetribution: return Spells::Judgment;
                case ChrSpecialization::PaladinProtection:  return Spells::JudgmentProt;
                case ChrSpecialization::PaladinHoly:         return Spells::JudgmentHoly;
                default:                                     return 0;
            }
        }

        static uint32 GetHammerOfWrathSpellId(Player const* player)
        {
            if (player->GetPrimarySpecialization() == ChrSpecialization::PaladinRetribution)
                return Spells::HammerOfWrathLegacy;  // 24275
            return Spells::HammerOfWrath;            // 1241413
        }

        static bool HasAvengingWrathAura(Unit const* unit)
        {
            return unit->HasAura(Spells::AvengingWrath)
                || unit->HasAura(Spells::AvengingWrathRet)
                || unit->HasAura(Spells::AvengingWrath3)
                || unit->HasAura(Spells::AvengingWrath4);
        }

        static void ApplyOverride(Player* player)
        {
            uint32 judgmentSpell = GetJudgmentSpellId(player);
            uint32 howSpell = GetHammerOfWrathSpellId(player);
            if (!judgmentSpell || !howSpell)
                return;

            player->AddTemporarySpell(howSpell);
            player->AddOverrideSpell(judgmentSpell, howSpell);
            player->SendSupercededSpell(judgmentSpell, howSpell);

            SpellHistory* history = player->GetSpellHistory();
            history->ResetCooldown(howSpell, true);
            if (SpellInfo const* howInfo = sSpellMgr->GetSpellInfo(howSpell, DIFFICULTY_NONE))
                if (howInfo->ChargeCategoryId)
                    history->RestoreCharge(howInfo->ChargeCategoryId);
        }

        static void RemoveOverride(Player* player)
        {
            uint32 judgmentSpell = GetJudgmentSpellId(player);
            uint32 howSpell = GetHammerOfWrathSpellId(player);
            if (!judgmentSpell || !howSpell)
                return;

            player->RemoveOverrideSpell(judgmentSpell, howSpell);
            player->SendSupercededSpell(howSpell, judgmentSpell);
            player->RemoveTemporarySpell(howSpell);
        }
    }

    // Avenging Wrath aura hooks — apply/remove Judgment override when AW is cast/expired.
    class spell_pal_hammer_of_wrath_avenging_wrath : public AuraScript
    {
        void HandleApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            Player* player = GetTarget()->ToPlayer();
            if (!player)
                return;

            if (!player->HasAura(Spells::HammerOfWrathTalent))
                return;

            PaladinHammerOfWrath::ApplyOverride(player);
        }

        void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            Player* player = GetTarget()->ToPlayer();
            if (!player)
                return;

            if (!player->HasAura(Spells::HammerOfWrathTalent))
                return;

            PaladinHammerOfWrath::RemoveOverride(player);
        }

        void Register() override
        {
            AfterEffectApply += AuraEffectApplyFn(spell_pal_hammer_of_wrath_avenging_wrath::HandleApply, EFFECT_FIRST_FOUND, SPELL_AURA_ANY, AURA_EFFECT_HANDLE_REAL);
            AfterEffectRemove += AuraEffectRemoveFn(spell_pal_hammer_of_wrath_avenging_wrath::HandleRemove, EFFECT_FIRST_FOUND, SPELL_AURA_ANY, AURA_EFFECT_HANDLE_REAL);
        }
    };

    // Hammer of Wrath talent passive aura hooks — apply/remove override if AW is active
    // when the talent is learned or unlearned.
    class spell_pal_hammer_of_wrath_talent : public AuraScript
    {
        void HandleApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            Player* player = GetTarget()->ToPlayer();
            if (!player)
                return;

            if (!PaladinHammerOfWrath::HasAvengingWrathAura(player))
                return;

            PaladinHammerOfWrath::ApplyOverride(player);
        }

        void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            Player* player = GetTarget()->ToPlayer();
            if (!player)
                return;

            if (!PaladinHammerOfWrath::HasAvengingWrathAura(player))
                return;

            PaladinHammerOfWrath::RemoveOverride(player);
        }

        void Register() override
        {
            AfterEffectApply += AuraEffectApplyFn(spell_pal_hammer_of_wrath_talent::HandleApply, EFFECT_FIRST_FOUND, SPELL_AURA_ANY, AURA_EFFECT_HANDLE_REAL);
            AfterEffectRemove += AuraEffectRemoveFn(spell_pal_hammer_of_wrath_talent::HandleRemove, EFFECT_FIRST_FOUND, SPELL_AURA_ANY, AURA_EFFECT_HANDLE_REAL);
        }
    };

    // =========================================================================
    // Radiant Glory
    // =========================================================================

    // 458359 - Radiant Glory: casting Wake of Ashes activates Avenging Wrath (or Crusade).
    class spell_pal_radiant_glory : public AuraScript
    {
        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({
                Spells::AvengingWrath3,
                Spells::AvengingWrath4
            });
        }

        bool CheckProc(ProcEventInfo& eventInfo)
        {
            SpellInfo const* spellInfo = eventInfo.GetSpellInfo();
            if (!spellInfo || spellInfo->Id != Spells::WakeOfAshes)
                return false;

            return true;
        }

        void HandleProc(AuraEffect* /*aurEff*/, ProcEventInfo& eventInfo)
        {
            PreventDefaultAction();

            Unit* caster = eventInfo.GetActor();
            if (!caster)
                return;

            // Crusade (231895 Ret variant or 384392 non-Ret variant) -> cast 454373
            // Otherwise -> cast Avenging Wrath 454351
            uint32 awSpell = Spells::AvengingWrath3;
            if (caster->HasSpell(Spells::AvengingWrathRet) || caster->HasSpell(Spells::CrusadeVariant))
                awSpell = Spells::AvengingWrath4;

            caster->CastSpell(caster, awSpell, CastSpellExtraArgsInit{
                .TriggerFlags = TRIGGERED_IGNORE_GCD | TRIGGERED_IGNORE_POWER_COST | TRIGGERED_IGNORE_CAST_IN_PROGRESS
                    | TRIGGERED_DONT_REPORT_CAST_ERROR | TRIGGERED_IGNORE_SPELL_AND_CATEGORY_CD,
                .TriggeringSpell = eventInfo.GetProcSpell()
            });
        }

        void Register() override
        {
            DoCheckProc += AuraCheckProcFn(spell_pal_radiant_glory::CheckProc);
            OnEffectProc += AuraEffectProcFn(spell_pal_radiant_glory::HandleProc, EFFECT_FIRST_FOUND, SPELL_AURA_ANY);
        }
    };

    // =========================================================================
    // Grand Crusader
    // =========================================================================

    // 85043 - Grand Crusader
    // Procs on avoided melee attacks (dodge/parry/miss/block) or on
    // Crusader Strike / Hammer of the Righteous / Blessed Hammer cast.
    // 15% chance to reset Avenger's Shield cooldown.
    // Native script has wrong aura type (PROC_TRIGGER_SPELL vs DUMMY) and
    // no hit-mask / spell-family filtering, so it never fires correctly.
    class spell_pal_grand_crusader_custom : public AuraScript
    {
        static bool IsGrandCrusaderTriggerSpell(SpellInfo const* spellInfo)
        {
            if (!spellInfo || spellInfo->SpellFamilyName != SPELLFAMILY_PALADIN)
                return false;

            // Crusader Strike family (all variants): Mask_1=0x8000, Mask_2=0x2
            if ((spellInfo->SpellFamilyFlags[1] & 0x8000) && (spellInfo->SpellFamilyFlags[2] & 0x2))
                return true;

            // Hammer of the Righteous: Mask_1=0x40000, Mask_2=0x2
            if ((spellInfo->SpellFamilyFlags[1] & 0x40000) && (spellInfo->SpellFamilyFlags[2] & 0x2))
                return true;

            // Blessed Hammer damage: Mask_2=0x2, Mask_3=0x200000
            if ((spellInfo->SpellFamilyFlags[2] & 0x2) && (spellInfo->SpellFamilyFlags[3] & 0x200000))
                return true;

            // Templar Strike / Templar Slash (completely different mask)
            if (spellInfo->Id == Spells::TemplarStrike || spellInfo->Id == Spells::TemplarSlash)
                return true;

            return false;
        }

        bool Validate(SpellInfo const* /*spellInfo*/) override
        {
            return ValidateSpellInfo({ Spells::AvengersShield });
        }

        bool CheckProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
        {
            if (GetTarget()->GetTypeId() != TYPEID_PLAYER)
                return false;

            // Avoided melee attacks: only proc on dodge/parry/miss/block/full_block
            if (eventInfo.GetTypeMask() & TAKEN_HIT_PROC_FLAG_MASK)
            {
                uint32 hitMask = eventInfo.GetHitMask();
                if (!(hitMask & (PROC_HIT_DODGE | PROC_HIT_PARRY | PROC_HIT_MISS | PROC_HIT_BLOCK | PROC_HIT_FULL_BLOCK)))
                    return false;
            }

            // Done melee abilities: only proc on Crusader Strike / HoTR / Blessed Hammer / Templar Strike
            if (eventInfo.GetTypeMask() & DONE_HIT_PROC_FLAG_MASK)
            {
                if (eventInfo.GetSpellPhaseMask() != PROC_SPELL_PHASE_HIT)
                    return false;

                if (!IsGrandCrusaderTriggerSpell(eventInfo.GetSpellInfo()))
                    return false;
            }

            return roll_chance(aurEff->GetAmount());
        }

        void HandleEffectProc(AuraEffect* /*aurEff*/, ProcEventInfo& /*eventInfo*/)
        {
            PreventDefaultAction();
            GetTarget()->CastSpell(GetTarget(), Spells::GrandCrusaderBuff, true);
            GetTarget()->GetSpellHistory()->ResetCooldown(Spells::AvengersShield, true);
        }

        void Register() override
        {
            DoCheckEffectProc += AuraCheckEffectProcFn(spell_pal_grand_crusader_custom::CheckProc, EFFECT_0, SPELL_AURA_DUMMY);
            OnEffectProc += AuraEffectProcFn(spell_pal_grand_crusader_custom::HandleEffectProc, EFFECT_0, SPELL_AURA_DUMMY);
        }
    };
}

void AddSC_custom_paladin_spell_fixes()
{
    using namespace Scripts::Custom::Paladin;

    RegisterSpellScript(spell_pal_art_of_war_custom);
    RegisterSpellScript(spell_pal_righteous_cause_custom);
    RegisterSpellScript(spell_pal_consecrated_blade_custom);
    RegisterSpellScript(spell_pal_divine_storm_custom);
    RegisterSpellScript(spell_pal_divine_storm_damage_custom);
    RegisterSpellScript(spell_pal_empyrean_power_custom);
    RegisterSpellScript(spell_pal_empyrean_legacy_custom);
    RegisterSpellScript(spell_pal_empyrean_legacy_buff_custom);
    RegisterSpellScript(spell_pal_empyrean_legacy_spender_custom);
    RegisterSpellScript(spell_pal_second_sunrise_custom);
    RegisterSpellScript(spell_pal_second_sunrise_effectiveness_damage);
    RegisterSpellScript(spell_pal_second_sunrise_effectiveness_healing);
    RegisterSpellScript(spell_pal_holy_shock_custom);
    RegisterSpellScript(spell_pal_dawnlight_charges);
    RegisterSpellScript(spell_pal_morning_star);
    RegisterSpellScript(spell_pal_will_of_the_dawn);
    RegisterSpellScript(spell_pal_dawnlight_damage_custom);
    RegisterSpellScript(spell_pal_dawnlight_heal_custom);
    RegisterSpellScript(spell_pal_dawnlight_radiation_damage_custom);
    RegisterSpellScript(spell_pal_dawnlight_radiation_heal_custom);
    RegisterSpellScript(spell_pal_suns_avatar_link);
    RegisterSpellScript(spell_pal_suns_avatar_self_link);
    RegisterSpellScript(spell_pal_suns_avatar_damage);
    RegisterSpellScript(spell_pal_suns_avatar_heal);
    RegisterSpellScript(spell_pal_suns_avatar_self_damage);
    RegisterSpellScript(spell_pal_light_s_beacon_custom);
    RegisterSpellScript(spell_pal_expurgation_custom);
    RegisterSpellScript(spell_pal_hammer_of_wrath_avenging_wrath);
    RegisterSpellScript(spell_pal_hammer_of_wrath_talent);
    RegisterSpellScript(spell_pal_radiant_glory);
    RegisterSpellScript(spell_pal_grand_crusader_custom);
    RegisterSpellScript(spell_pal_holy_prism_selector);
    RegisterSpellScript(spell_pal_judge_jury_executioner);
    RegisterSpellScript(spell_pal_judge_jury_execution_sentence);
    RegisterSpellScript(spell_pal_judge_jury_executioner_buff);
}
