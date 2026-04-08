/*
 * Advanced Flying / Skyriding spell scripts
 * Based on hwis/DragonCore + TrinityCore PR #30199 (Traesh)
 *
 * Key insight: Active abilities (374763, 372608, 361584) require
 * CasterAuraSpell = 372773 (Vigor) in DB2 SpellAuraRestrictions.
 * Without Vigor aura, client AND server block the cast.
 */

#include "ScriptMgr.h"
#include "Player.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "Unit.h"
#include <G3D/g3dmath.h>

static constexpr float THRILL_OF_THE_SKIES_MIN_VELOCITY = 40.f;

enum AdvancedFlyingSpells
{
    SPELL_STEADY_FLIGHT         = 404468,
    SPELL_SKYRIDING             = 404464,

    SPELL_SKYRIDING_BASICS      = 376777,
    SPELL_SWITCH_FLIGHT_STYLE   = 436854,
    SPELL_VIGOR                 = 372773,
    SPELL_LIFT_OFF_1            = 374763,
    SPELL_LIFT_OFF_2            = 372610,
    SPELL_LIFT_OFF_3            = 386451,
    SPELL_THRILL_OF_THE_SKIES   = 377234,
    SPELL_THRILL_OF_THE_SKIES_2 = 383366,
    SPELL_SURGE_FORWARD         = 372608,
    SPELL_WHIRLING_SURGE        = 361584,

    SPELL_ENERGY_WIDGET         = 423624,
    SPELL_VIGOR_CACHE           = 433547,
    SPELL_DRAGONRIDER_ENERGIZE = 372606,

    SPELL_EVOKER_SOAR_RACIAL    = 369536,
};

static void SendFacingImpulse(Unit* caster, float speed)
{
    float orientation = caster->GetOrientation();
    float pitch = caster->m_movementInfo.pitch;
    float cosPitch = std::cos(pitch);
    Position direction(
        std::cos(orientation) * cosPitch * speed,
        std::sin(orientation) * cosPitch * speed,
        std::sin(pitch) * speed
    );
    caster->SendAddImpulse(direction);
}

static SpellCastResult CheckSkyriding(SpellScript* script)
{
    Unit* caster = script->GetCaster();
    if (!caster->HasExtraUnitMovementFlag2(MOVEMENTFLAG3_CAN_ADV_FLY))
    {
        script->SetCustomCastResultMessage(SPELL_CUSTOM_ERROR_REQUIRES_SKYRIDING);
        return SPELL_FAILED_CUSTOM_ERROR;
    }
    return SPELL_CAST_OK;
}

// Spell 436854 - Switch Flight Style
class spell_switch_flight : public SpellScript
{
    bool Validate(SpellInfo const* /*spellInfo*/) override
    {
        return ValidateSpellInfo({ SPELL_SKYRIDING, SPELL_STEADY_FLIGHT, SPELL_VIGOR });
    }

    void HandleDummy(SpellEffIndex /*effIndex*/)
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        Player* player = caster->ToPlayer();
        if (!player)
            return;

        bool hasSkyriding = caster->HasAura(SPELL_SKYRIDING);
        //bool hasSteadyFlight = caster->HasAura(SPELL_STEADY_FLIGHT);

        caster->RemoveAura(SPELL_SKYRIDING);
        caster->RemoveAura(SPELL_STEADY_FLIGHT);

        if (hasSkyriding)
        {
            caster->CastSpell(caster, SPELL_STEADY_FLIGHT, true);
            caster->RemoveAura(SPELL_VIGOR);
            if (caster->IsMounted())
            {
                caster->SetCanAdvFly(false);
                caster->SetCanDoubleJump(false);
            }
        }
        else
        {
            caster->CastSpell(caster, SPELL_SKYRIDING, true);
            if (!caster->HasAura(SPELL_VIGOR))
                caster->CastSpell(caster, SPELL_VIGOR, true);
            if (caster->IsMounted())
            {
                caster->SetCanAdvFly(true);
                caster->SetCanDoubleJump(true);
                caster->SetFlightCapabilityID(1, true);
            }
        }
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_switch_flight::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// Skyward Ascent (Abheben) - Spells 374763, 372610, 386451
class spell_af_skyward_ascent : public SpellScript
{
    SpellCastResult CheckCast()
    {
        return CheckSkyriding(this);
    }

    void HandleHit(SpellEffIndex /*effIndex*/)
    {
        if (Unit* caster = GetCaster())
        {
            float orientation = caster->GetOrientation();
            float horizontalSpeed = 12.25f;
            Position direction(
                std::cos(orientation) * horizontalSpeed,
                std::sin(orientation) * horizontalSpeed,
                49.0f
            );
            caster->SendAddImpulse(direction);
        }
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_af_skyward_ascent::CheckCast);
        OnEffectHitTarget += SpellEffectFn(spell_af_skyward_ascent::HandleHit, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// Surge Forward - Spell 372608
class spell_af_surge_forward : public SpellScript
{
    SpellCastResult CheckCast()
    {
        return CheckSkyriding(this);
    }

    void HandleHit(SpellEffIndex /*effIndex*/)
    {
        if (Unit* caster = GetCaster())
            SendFacingImpulse(caster, 30.0f);
    }

    void Register() override
    {
        OnCheckCast         += SpellCheckCastFn(spell_af_surge_forward::CheckCast);
        OnEffectHitTarget   += SpellEffectFn(spell_af_surge_forward::HandleHit, EFFECT_0, SPELL_EFFECT_DUMMY);
    }
};

// Whirling Surge - Spell 361584
class spell_af_whirling_surge : public SpellScript
{
    void HandleDummy()
    {
        Player* caster = GetCaster()->ToPlayer();
        if (!caster)
            return;

        float speed = 60.0f;
        float orientation = caster->GetOrientation();
        float pitch = caster->m_movementInfo.pitch;

        float deltaX = speed * std::cos(orientation) * std::cos(pitch);
        float deltaY = speed * std::sin(orientation) * std::cos(pitch);
        float deltaZ = speed * std::sin(pitch);

        caster->AddMoveImpulse(Position(deltaX, deltaY, deltaZ));
    }

    void Register() override
    {
        AfterCast += SpellCastFn(spell_af_whirling_surge::HandleDummy);
    }
};

// 361584 - Whirling Surge (AuraScript for periodic impulse ticks)
// Wowhead: Apply Aura: Dummy, 3s duration. Sniff: 5-6 impulse ticks at magnitude 5.0, facing+pitch oriented.
class spell_dragonriding_whirling_surge_aura : public AuraScript
{
    void HandlePeriodicDummy(AuraEffect const* /*aurEff*/)
    {
        if (Unit* target = GetTarget())
            SendFacingImpulse(target, 5.0f);
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_dragonriding_whirling_surge_aura::HandlePeriodicDummy, EFFECT_0, SPELL_AURA_DUMMY);
    }
};

// 392752 - Launch Boost (SpellScript for initial upward impulse on spell hit)
// Wowhead: Periodic Dummy, period 100ms, duration 2s. Sniff: Z=45 impulse on first hit.
class spell_dragonriding_launch_boost : public SpellScript
{
    void HandleHit(SpellEffIndex /*effIndex*/)
    {
        if (Unit* caster = GetCaster())
        {
            Position direction(0.0f, 0.0f, 45.0f);
            caster->SendAddImpulse(direction);
        }
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_dragonriding_launch_boost::HandleHit, EFFECT_0, SPELL_EFFECT_APPLY_AURA);
    }
};

// 392752 - Launch Boost (AuraScript for periodic forward impulse ticks after initial launch)
class spell_dragonriding_launch_boost_aura : public AuraScript
{
    void HandlePeriodicDummy(AuraEffect const* /*aurEff*/)
    {
        if (Unit* target = GetTarget())
        {
            if (!target->HasExtraUnitMovementFlag2(MOVEMENTFLAG3_CAN_ADV_FLY))
                return;

            SendFacingImpulse(target, 5.0f);
        }
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_dragonriding_launch_boost_aura::HandlePeriodicDummy, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
    }
};

// 406095 - Dynamic Flight
class spell_dragonriding : public AuraScript
{
    void OnPeriodic(AuraEffect const* /*aurEff*/)
    {
        float advFlyingVelocity = GetTarget()->GetAdvFlyingVelocity();
        bool advFylingEnabled = GetTarget()->HasAuraType(SPELL_AURA_ADV_FLYING);

        float ground = GetTarget()->GetFloorZ();
        bool isInAir = (G3D::fuzzyGt(GetTarget()->GetPositionZ(), ground + GROUND_HEIGHT_TOLERANCE) || G3D::fuzzyLt(GetTarget()->GetPositionZ(), ground - GROUND_HEIGHT_TOLERANCE));

        if (isInAir && advFlyingVelocity && advFylingEnabled)
        {
            if (advFlyingVelocity > THRILL_OF_THE_SKIES_MIN_VELOCITY)
            {
                if (!GetTarget()->HasAura(SPELL_THRILL_OF_THE_SKIES))
                    GetTarget()->CastSpell(GetTarget(), SPELL_THRILL_OF_THE_SKIES, TRIGGERED_FULL_MASK);
                GetTarget()->CastSpell(GetTarget(), SPELL_THRILL_OF_THE_SKIES_2, TRIGGERED_FULL_MASK);
            }
            else
                GetTarget()->RemoveAurasDueToSpell(SPELL_THRILL_OF_THE_SKIES);
            GetTarget()->RemoveAurasDueToSpell(SPELL_THRILL_OF_THE_SKIES_2);
        }
        else
        {
            GetTarget()->RemoveAurasDueToSpell(SPELL_THRILL_OF_THE_SKIES);
            GetTarget()->RemoveAurasDueToSpell(SPELL_THRILL_OF_THE_SKIES_2);
        }

        if (advFlyingVelocity <= 8.5f)
        {
        }

        // Soar Evoker
        if (!isInAir && GetCaster()->HasAura(SPELL_EVOKER_SOAR_RACIAL))
            GetCaster()->RemoveAura(SPELL_EVOKER_SOAR_RACIAL);
    }

    void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        if (Unit* caster = GetCaster())
            caster->AddAura(SPELL_DRAGONRIDER_ENERGY, caster);
        GetTarget()->CastSpell(GetTarget(), SPELL_ENERGY_WIDGET, true);
        GetTarget()->SetPower(POWER_ALTERNATE_MOUNT, GetTarget()->GetPower(POWER_ALTERNATE_MOUNT), true);
    }

    void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        if (Unit* caster = GetCaster())
        {
            caster->RemoveAura(SPELL_DRAGONRIDER_ENERGY);
            caster->RemoveAura(SPELL_VIGOR_CACHE);
        }
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_dragonriding::OnPeriodic, EFFECT_2, SPELL_AURA_PERIODIC_DUMMY);
        OnEffectApply += AuraEffectApplyFn(spell_dragonriding::OnApply, EFFECT_2, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
        OnEffectRemove += AuraEffectRemoveFn(spell_dragonriding::OnRemove, EFFECT_2, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

// 372771 - Dragonrider Energy
class spell_dragonrider_energy : public AuraScript
{
    void OnPeriodic(AuraEffect* /*aurEff*/)
    {
        Unit* caster = GetCaster();
        if (!caster)
            return;

        caster->ToPlayer()->AddAura(433547, caster);

        int newMaxPower = 3;

        if (caster->HasAura(377920) && !caster->HasAura(377921) && !caster->HasAura(377922))
        {
            newMaxPower = 4;
        }
        else if (caster->HasAura(377921) && caster->HasAura(377920) && !caster->HasAura(377922))
        {
            newMaxPower = 5;
        }
        else if (caster->HasAura(377922) && caster->HasAura(377921) && caster->HasAura(377920))
        {
            newMaxPower = 6;
        }

        caster->SetMaxPower(POWER_ALTERNATE_MOUNT, newMaxPower);
    }

    void Register() override
    {
        OnEffectUpdatePeriodic += AuraEffectUpdatePeriodicFn(spell_dragonrider_energy::OnPeriodic, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY);
    }
};

// 372773 - Dragonrider Energy
class spell_af_energy : public AuraScript
{
    void OnPeriodic(AuraEffect* /*aurEff*/)
    {
        if (Unit* caster = GetCaster())
        {
            if (ShouldRegenEnergy(caster))
            {
                if (AuraEffect* subAmountAurEff = caster->GetAuraEffect(SPELL_VIGOR_CACHE, EFFECT_1))
                {
                    int32 baseRegen = 20; // Todo : Calculate this based on talents & if we are thrilled/grounded

                    int32 newAmount = subAmountAurEff->GetAmount() + baseRegen;

                    if (newAmount >= 100)
                    {
                        newAmount -= 100;

                        caster->CastSpell(caster, SPELL_DRAGONRIDER_ENERGIZE, TRIGGERED_FULL_MASK);

                        if (AuraEffect* amountAurEff = caster->GetAuraEffect(SPELL_VIGOR_CACHE, EFFECT_0))
                            amountAurEff->SetAmount(caster->GetPower(POWER_ALTERNATE_MOUNT));
                    }

                    subAmountAurEff->SetAmount(newAmount);
                    subAmountAurEff->GetBase()->SetNeedClientUpdateForTargets();
                }
            }
        }
    }

    bool ShouldRegenEnergy(Unit const* caster) const
    {
        if (caster->GetPower(POWER_ALTERNATE_MOUNT) == caster->GetMaxPower(POWER_ALTERNATE_MOUNT))
            return false;

        FlightCapabilityEntry const* flightCapabilityEntry = sFlightCapabilityStore.LookupEntry(caster->GetFlightCapabilityID());
        if (!flightCapabilityEntry)
            return false;

        float velocityRegenThreshold = flightCapabilityEntry->MaxVel * flightCapabilityEntry->VigorRegenMaxVelCoefficient;
        if (caster->GetAdvFlyingVelocity() >= velocityRegenThreshold)
            return true;

        return !caster->IsInAir() || caster->IsInWater();
    }

    void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        GetTarget()->RemoveAurasDueToSpell(SPELL_VIGOR_CACHE);
    }

    void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Unit* target = GetTarget();
        if (!target->HasAura(SPELL_VIGOR_CACHE))
        {
            CastSpellExtraArgs extraArgs(TRIGGERED_FULL_MASK);
            extraArgs.AddSpellMod(SPELLVALUE_BASE_POINT0, target->GetPower(POWER_ALTERNATE_MOUNT));
            target->CastSpell(target, SPELL_VIGOR_CACHE, extraArgs);
        }
    }

    void Register() override
    {
        OnEffectApply += AuraEffectApplyFn(spell_af_energy::OnApply, EFFECT_0, SPELL_AURA_ENABLE_ALT_POWER, AURA_EFFECT_HANDLE_REAL);
        OnEffectUpdatePeriodic += AuraEffectUpdatePeriodicFn(spell_af_energy::OnPeriodic, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY);
        OnEffectRemove += AuraEffectRemoveFn(spell_af_energy::OnRemove, EFFECT_0, SPELL_AURA_ENABLE_ALT_POWER, AURA_EFFECT_HANDLE_REAL);
    }
};

// PlayerScript: Auto-teach Skyriding abilities + apply Vigor
class adv_flying_check : public PlayerScript
{
public:
    adv_flying_check() : PlayerScript("adv_flying_check") { }

    void CheckFlying(Player* player) const
    {
        if (!player || !player->IsInWorld())
            return;

        uint8 level = player->GetLevel();

        if (level >= 10)
        {
            auto learn = [&](uint32 spellId) {
                if (!player->HasSpell(spellId))
                    player->LearnSpell(spellId, false);
            };

            learn(SPELL_SKYRIDING_BASICS);
            learn(SPELL_LIFT_OFF_1);
            learn(SPELL_SURGE_FORWARD);
            learn(SPELL_WHIRLING_SURGE);

            if (level >= 20)
                learn(SPELL_SWITCH_FLIGHT_STYLE);

            bool hasSkyriding = player->HasAura(SPELL_SKYRIDING);
            bool hasSteady = player->HasAura(SPELL_STEADY_FLIGHT);

            if (hasSkyriding && hasSteady)
            {
                player->RemoveAura(SPELL_STEADY_FLIGHT);
            }
            else if (!hasSkyriding && !hasSteady)
            {
                player->CastSpell(player, SPELL_SKYRIDING, true);
            }

            // Vigor aura: CRITICAL for active abilities (CasterAuraSpell=372773 in DB2)
            if (player->HasAura(SPELL_SKYRIDING) && !player->HasAura(SPELL_VIGOR))
                player->CastSpell(player, SPELL_VIGOR, true);
            if (player->HasAura(SPELL_STEADY_FLIGHT) && player->HasAura(SPELL_VIGOR))
                player->RemoveAura(SPELL_VIGOR);
        }
    }

    void OnLogin(Player* player, bool /*firstLogin*/) override
    {
        CheckFlying(player);
    }

    void OnLevelChanged(Player* player, uint8 /*oldLevel*/) override
    {
        CheckFlying(player);
    }
};

void AddSC_advanced_flying_spell_scripts()
{
    RegisterSpellScript(spell_switch_flight);
    RegisterSpellScript(spell_af_skyward_ascent);
    RegisterSpellScript(spell_af_surge_forward);
    //RegisterSpellScript(spell_af_whirling_surge);
    RegisterSpellAndAuraScriptPair(spell_af_whirling_surge, spell_dragonriding_whirling_surge_aura);
    RegisterSpellAndAuraScriptPair(spell_dragonriding_launch_boost, spell_dragonriding_launch_boost_aura);
    RegisterSpellScript(spell_dragonriding);
    RegisterSpellScript(spell_dragonrider_energy);
    RegisterSpellScript(spell_af_energy);
    new adv_flying_check();
}
