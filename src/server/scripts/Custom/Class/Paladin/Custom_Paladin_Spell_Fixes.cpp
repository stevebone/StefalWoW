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

            // AoE / multi-hit: only one roll per cast. White swings have no cast id — roll each swing.
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
                case 31884:  // Avenging Wrath
                case 389593: // Avenging Wrath (Ret version)
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
            OnEffectProc += AuraEffectProcFn(spell_pal_empyrean_legacy_custom::HandleProc, EFFECT_0, SPELL_AURA_DUMMY);
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

    // Applies Second Sunrise echo effectiveness to damage/healing spells.
    class spell_pal_second_sunrise_effectiveness_custom : public SpellScript
    {
        void HandleDamage(SpellEffectInfo const& /*spellEffectInfo*/, Unit const* /*victim*/, int32& /*damage*/, int32& /*flatMod*/, float& pctMod) const
        {
            PaladinSecondSunrise::ApplyEffectiveness(GetSpell(), pctMod);
        }

        void HandleHealing(SpellEffectInfo const& /*spellEffectInfo*/, Unit const* /*victim*/, int32& /*healing*/, int32& /*flatMod*/, float& pctMod) const
        {
            PaladinSecondSunrise::ApplyEffectiveness(GetSpell(), pctMod);
        }

        void Register() override
        {
            CalcDamage += SpellCalcDamageFn(spell_pal_second_sunrise_effectiveness_custom::HandleDamage);
            CalcHealing += SpellCalcHealingFn(spell_pal_second_sunrise_effectiveness_custom::HandleHealing);
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
    RegisterSpellScript(spell_pal_second_sunrise_effectiveness_custom);
    RegisterSpellScript(spell_pal_holy_shock_custom);
}
