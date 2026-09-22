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

#include <unordered_map>

static constexpr float THRILL_OF_THE_SKIES_MIN_VELOCITY = 40.f;

static constexpr float LIGHTNING_SPEED_IMPULSE_PER_TICK = 15.f; // forward impulse per 1s tick of the 5s burst
static constexpr float NEAR_WALL_RANGE = 24.f;                  // wall skim detection distance
static constexpr float NEAR_GROUND_RANGE = 18.f;                // low altitude above terrain counts as skimming too
static constexpr float MIN_SKIM_ALTITUDE = 2.f;                 // standing on the ground is not skimming
static constexpr uint32 STATIC_CHARGE_TICK = 1000;              // 1 stack per second near a surface
static constexpr uint32 STATIC_CHARGE_MAX_STACKS = 10;          // spell 418590 "Stackable up to 10"
static constexpr uint32 STATIC_CHARGE_DURATION_MS = 5000;       // spell 418590 duration, forced onto the aura

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
    SPELL_AIR_STOP              = 403092,
    SPELL_SECOND_WIND           = 425782,

    SPELL_WHIRLING_SURGE_2      = 376359,
    SPELL_LIGHTNING_RUSH        = 418592,
    SPELL_STATIC_CHARGE_STACKS  = 418590,
    SPELL_STATIC_CHARGE_READY   = 419252,

    SPELL_REPLACE_WITH_WHIRLING_SURGE  = 447981,
    SPELL_REPLACE_WITH_LIGHTNING_RUSH  = 447982,

    ACHIEVEMENT_DYNAMIC_FLIGHT_SECOND_WIND = 61553,
    ACHIEVEMENT_DYNAMIC_FLIGHT_AIR_STOP    = 61554,

    SPELL_ENERGY_WIDGET         = 423624,
    SPELL_VIGOR_CACHE           = 433547,
    SPELL_DRAGONRIDER_ENERGIZE  = 372606,

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
    if (!caster->HasUnitMovementFlag(MOVEMENTFLAG_CAN_ADV_FLY))
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

        bool hadSkyriding = caster->HasAura(SPELL_SKYRIDING);

        caster->RemoveAura(SPELL_SKYRIDING);
        caster->RemoveAura(SPELL_STEADY_FLIGHT);

        if (hadSkyriding)
        {
            caster->CastSpell(caster, SPELL_STEADY_FLIGHT, true);
            caster->RemoveAura(SPELL_VIGOR);
            if (caster->IsMounted())
            {
                caster->SetCanAdvFly(false);
                caster->SetCanDoubleJump(false);
            }

            // Exactly one style spell must stay learned: the outgoing one is removed
            // unconditionally, otherwise both accumulate in character_spell and the
            // login order decides which aura wins instead of the player's choice.
            player->RemoveSpell(SPELL_SKYRIDING);
            if (!player->HasSpell(SPELL_STEADY_FLIGHT))
                player->AddSpell(SPELL_STEADY_FLIGHT, false, true, false, false);
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

            player->RemoveSpell(SPELL_STEADY_FLIGHT);
            if (!player->HasSpell(SPELL_SKYRIDING))
                player->AddSpell(SPELL_SKYRIDING, false, true, false, false);
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
        OnCheckCast += SpellCheckCastFn(spell_af_surge_forward::CheckCast);
        OnEffectHitTarget += SpellEffectFn(spell_af_surge_forward::HandleHit, EFFECT_0, SPELL_EFFECT_DUMMY);
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
            if (!target->HasUnitMovementFlag(MOVEMENTFLAG_CAN_ADV_FLY))
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

        GetTarget()->CastSpell(GetTarget(), SPELL_ENERGY_WIDGET, true);
        GetTarget()->SetPower(POWER_ALTERNATE_MOUNT, GetTarget()->GetPower(POWER_ALTERNATE_MOUNT), true);
    }

    void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        if (Unit* caster = GetCaster())
        {
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
    adv_flying_check() : PlayerScript("adv_flying_check") {}

    void CheckFlying(Player* player) const
    {
        if (!player || !player->IsInWorld())
            return;

        uint8 level = player->GetLevel();

        if (level >= 20)
        {
            auto learn = [&](uint32 spellId) {
                if (!player->HasSpell(spellId))
                    player->LearnSpell(spellId, false);
            };

            learn(SPELL_SKYRIDING_BASICS);
            learn(SPELL_SURGE_FORWARD);
            learn(SPELL_SWITCH_FLIGHT_STYLE);
            learn(SPELL_SECOND_WIND);

            bool lightningSpeedSelected = player->HasSpell(SPELL_REPLACE_WITH_LIGHTNING_RUSH)
                || player->HasSpell(SPELL_LIGHTNING_RUSH);
            if (!lightningSpeedSelected)
                learn(SPELL_WHIRLING_SURGE);

            if (lightningSpeedSelected)
            {
                player->RemoveSpell(SPELL_WHIRLING_SURGE_2);
                player->RemoveSpell(SPELL_WHIRLING_SURGE);
            }
            else if (player->HasSpell(SPELL_REPLACE_WITH_WHIRLING_SURGE))
                player->RemoveSpell(SPELL_LIGHTNING_RUSH);

            auto completeAchievement = [&](uint32 achievementId)
            {
                if (AchievementEntry const* achievementEntry = sAchievementStore.LookupEntry(achievementId))
                    if (!player->HasAchieved(achievementId))
                        player->CompletedAchievement(achievementEntry);
            };
            completeAchievement(ACHIEVEMENT_DYNAMIC_FLIGHT_SECOND_WIND);

            if (level >= 30)
            {
                learn(SPELL_AIR_STOP);
                completeAchievement(ACHIEVEMENT_DYNAMIC_FLIGHT_AIR_STOP);
            }

            bool hasSkyriding = player->HasAura(SPELL_SKYRIDING);
            bool hasSteady = player->HasAura(SPELL_STEADY_FLIGHT);

            if (hasSkyriding && hasSteady)
            {
                player->RemoveAura(SPELL_STEADY_FLIGHT);
            }
            else if (!hasSkyriding && !hasSteady)
            {
                // The spell is learned with active=false so it stays out of the spell book.
                if (player->HasSpell(SPELL_STEADY_FLIGHT))
                    player->CastSpell(player, SPELL_STEADY_FLIGHT, true);
                else if (player->HasSpell(SPELL_SKYRIDING))
                    player->CastSpell(player, SPELL_SKYRIDING, true);
                else
                {
                    player->AddSpell(SPELL_SKYRIDING, false, true, false, false);
                    if (!player->HasAura(SPELL_SKYRIDING))
                        player->CastSpell(player, SPELL_SKYRIDING, true);
                }
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

// 418592 - Lightning Rush
class spell_lightning_rush : public SpellScript
{
    SpellCastResult CheckCast()
    {
        Unit* caster = GetCaster();
        if (!caster->HasUnitMovementFlag(MOVEMENTFLAG_CAN_ADV_FLY))
        {
            SetCustomCastResultMessage(SPELL_CUSTOM_ERROR_REQUIRES_SKYRIDING);
            return SPELL_FAILED_CUSTOM_ERROR;
        }

        return SPELL_CAST_OK;
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_lightning_rush::CheckCast);
    }
};

// 418592 - Lightning Rush aura
class spell_lightning_rush_aura : public AuraScript
{
    void OnPeriodicTick(AuraEffect const* /*aurEff*/)
    {
        if (Unit* target = GetTarget())
            SendFacingImpulse(target, LIGHTNING_SPEED_IMPULSE_PER_TICK);
    }

    void Register() override
    {
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_lightning_rush_aura::OnPeriodicTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
    }
};

static bool IsNearSurface(Unit* unit)
{
    float const x = unit->GetPositionX();
    float const y = unit->GetPositionY();
    float const z = unit->GetPositionZ();

    Map* map = unit->GetMap();
    float const groundBelow = map->GetHeight(unit->GetPhaseShift(), x, y, z + 1.f);
    if (groundBelow == INVALID_HEIGHT)
        return false;

    float const altitude = z - groundBelow;
    if (altitude < MIN_SKIM_ALTITUDE) // standing or walking on the ground is not skimming
        return false;

    // model surfaces (trees, buildings, cliff walls) through line of sight rays
    for (int i = 0; i < 4; ++i)
    {
        float const angle = unit->GetOrientation() + i * (float(M_PI) / 2.f);
        if (!unit->IsWithinLOS(x + std::cos(angle) * NEAR_WALL_RANGE, y + std::sin(angle) * NEAR_WALL_RANGE, z + 1.f))
            return true;
    }

    if (altitude < NEAR_GROUND_RANGE) // low pass over terrain
        return true;

    for (int i = 0; i < 4; ++i)
    {
        float const angle = unit->GetOrientation() + i * (float(M_PI) / 2.f);
        float const sample = map->GetHeight(unit->GetPhaseShift(),
            x + std::cos(angle) * NEAR_WALL_RANGE, y + std::sin(angle) * NEAR_WALL_RANGE, z + 1.f);
        if (sample != INVALID_HEIGHT && sample > z) // terrain rises above the flight level
            return true;
    }

    return false;
}

class adv_flying_static_charge : public PlayerScript
{
public:
    adv_flying_static_charge() : PlayerScript("adv_flying_static_charge") {}

    void OnUpdate(Player* player, uint32 diff) override
    {
        auto itr = _nextStackTick.find(player->GetGUID());
        if (itr == _nextStackTick.end())
        {
            _nextStackTick.emplace(player->GetGUID(), STATIC_CHARGE_TICK);
            return;
        }

        if (itr->second > diff)
        {
            itr->second -= diff;
            return;
        }
        itr->second = STATIC_CHARGE_TICK;

        if (!player->HasSpell(SPELL_LIGHTNING_RUSH) || !player->HasUnitMovementFlag(MOVEMENTFLAG_CAN_ADV_FLY))
            return;

        Aura* stacks = player->GetAura(SPELL_STATIC_CHARGE_STACKS);

        // the charge expired unused - the button enabler must not outlive it
        if (!stacks)
            player->RemoveAura(SPELL_STATIC_CHARGE_READY);

        if (stacks && stacks->GetStackAmount() >= STATIC_CHARGE_MAX_STACKS)
        {
            if (!player->HasAura(SPELL_STATIC_CHARGE_READY))
                player->CastSpell(player, SPELL_STATIC_CHARGE_READY, true);
            return;
        }

        if (IsNearSurface(player))
        {
            // direct aura application - spell 418590 carries CasterAuraSpell 417888 (storm rider
            // form) which normal players do not have, so a triggered cast would be rejected
            if (!stacks)
                stacks = player->AddAura(SPELL_STATIC_CHARGE_STACKS, player);

            if (stacks)
            {
                if (stacks->GetStackAmount() < STATIC_CHARGE_MAX_STACKS)
                    stacks->SetStackAmount(stacks->GetStackAmount() + 1);

                // force the intended duration - some server data builds carry a truncated one,
                // which made the aura expire between ticks
                stacks->SetMaxDuration(STATIC_CHARGE_DURATION_MS);
                stacks->SetDuration(STATIC_CHARGE_DURATION_MS);
            }
        }
    }

    void OnLogout(Player* player) override
    {
        _nextStackTick.erase(player->GetGUID());
    }

private:
    std::unordered_map<ObjectGuid, uint32> _nextStackTick;
};

// 447982 - Lightning Rush (Talent)
class spell_af_swap_lightning_rush : public AuraScript
{
    void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Player* player = GetTarget()->ToPlayer();
        if (!player)
            return;

        if (player->HasSpell(SPELL_WHIRLING_SURGE_2))
            player->RemoveSpell(SPELL_WHIRLING_SURGE_2);
        if (player->HasSpell(SPELL_WHIRLING_SURGE))
            player->RemoveSpell(SPELL_WHIRLING_SURGE);
        if (!player->HasSpell(SPELL_LIGHTNING_RUSH))
            player->LearnSpell(SPELL_LIGHTNING_RUSH, false);
    }

    void Register() override
    {
        OnEffectApply += AuraEffectApplyFn(spell_af_swap_lightning_rush::OnApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

// 447981 - Whirling Surge (Talent)
class spell_af_swap_whirling_surge : public AuraScript
{
    void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Player* player = GetTarget()->ToPlayer();
        if (!player)
            return;

        if (player->HasSpell(SPELL_LIGHTNING_RUSH))
            player->RemoveSpell(SPELL_LIGHTNING_RUSH);
        if (!player->HasSpell(SPELL_WHIRLING_SURGE_2))
            player->LearnSpell(SPELL_WHIRLING_SURGE_2, false);
        if (!player->HasSpell(SPELL_WHIRLING_SURGE))
            player->LearnSpell(SPELL_WHIRLING_SURGE, false);
    }

    void Register() override
    {
        OnEffectApply += AuraEffectApplyFn(spell_af_swap_whirling_surge::OnApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
    }
};

void AddSC_advanced_flying_spell_scripts()
{
    RegisterSpellScript(spell_switch_flight);
    RegisterSpellScript(spell_af_skyward_ascent);
    RegisterSpellScript(spell_af_surge_forward);
    RegisterSpellAndAuraScriptPair(spell_af_whirling_surge, spell_dragonriding_whirling_surge_aura);
    RegisterSpellAndAuraScriptPair(spell_dragonriding_launch_boost, spell_dragonriding_launch_boost_aura);
    RegisterSpellScript(spell_dragonriding);
    RegisterSpellScript(spell_af_energy);
    RegisterSpellAndAuraScriptPair(spell_lightning_rush, spell_lightning_rush_aura);
    RegisterAuraScript(spell_af_swap_lightning_rush);
    RegisterAuraScript(spell_af_swap_whirling_surge);
    new adv_flying_check();
    new adv_flying_static_charge();
}
