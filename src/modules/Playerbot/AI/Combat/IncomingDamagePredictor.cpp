/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
 *
 * Incoming Damage Predictor implementation.
 * Analyzes enemy spell casts, melee attackers, and historical damage to
 * produce time-bucketed damage forecasts for proactive defensive CD usage.
 */

#include "IncomingDamagePredictor.h"
#include "InterruptAwareness.h"
#include "Player.h"
#include "Unit.h"
#include "Creature.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "ThreatManager.h"
#include "ObjectAccessor.h"
#include "GameTime.h"
#include "Log.h"
#include "../../Group/GroupRoleEnums.h"
#include <algorithm>
#include <cmath>

namespace Playerbot
{

// ============================================================================
// Constructor / Reset
// ============================================================================

IncomingDamagePredictor::IncomingDamagePredictor(Player* bot)
    : _bot(bot)
{
    if (bot)
    {
        // Auto-detect role
        GroupRole role = GetPlayerSpecRole(bot);
        _isTank = (role == GroupRole::TANK);
        _isHealer = (role == GroupRole::HEALER);
    }
}

void IncomingDamagePredictor::Reset()
{
    _forecast.Reset();
    _enemyHistory.clear();
    _totalIncomingDPS = 0.0f;
    _updateTimer = 0;
    _pruneTimer = 0;
    _dpsCalcTimer = 0;
}

// ============================================================================
// Core Update
// ============================================================================

void IncomingDamagePredictor::Update(uint32 diff, InterruptAwareness const* interruptAwareness)
{
    if (!_bot || !_bot->IsInWorld() || !_bot->IsAlive())
        return;

    _updateTimer += diff;
    _pruneTimer += diff;
    _dpsCalcTimer += diff;

    // Prune stale history periodically
    if (_pruneTimer >= HISTORY_PRUNE_INTERVAL_MS)
    {
        PruneHistory();
        _pruneTimer = 0;
    }

    // Recalculate total DPS periodically
    if (_dpsCalcTimer >= DPS_CALC_INTERVAL_MS)
    {
        UpdateHistoricalDPS(diff);
        _dpsCalcTimer = 0;
    }

    // Main prediction update at configured interval
    if (_updateTimer < UPDATE_INTERVAL_MS)
        return;

    _updateTimer = 0;

    // Reset forecast for this cycle
    _forecast.Reset();

    // Phase 1: Predict damage from detected spell casts
    PredictFromSpellCasts(interruptAwareness);

    // Phase 2: Predict damage from melee attackers
    PredictFromMeleeAttackers();

    // Phase 3: Add baseline from historical DPS patterns
    PredictFromHistory();

    // Sort events by time to impact
    std::sort(_forecast.events.begin(), _forecast.events.end(),
        [](PredictedDamageEvent const& a, PredictedDamageEvent const& b)
        {
            return a.timeToImpactMs < b.timeToImpactMs;
        });

    // Compute time-bucketed totals
    uint32 maxHealth = _bot->GetMaxHealth();
    for (auto const& event : _forecast.events)
    {
        if (event.timeToImpactMs <= 1000)
            _forecast.damageIn1Sec += event.estimatedDamage;
        if (event.timeToImpactMs <= 2000)
            _forecast.damageIn2Sec += event.estimatedDamage;
        if (event.timeToImpactMs <= 3000)
            _forecast.damageIn3Sec += event.estimatedDamage;
        if (event.timeToImpactMs <= 5000)
            _forecast.damageIn5Sec += event.estimatedDamage;

        if (event.estimatedDamage > _forecast.highestSingleHit)
            _forecast.highestSingleHit = event.estimatedDamage;

        if (event.spellId != 0)
            ++_forecast.totalSpellCasts;
        else
            ++_forecast.totalMeleeAttackers;
    }

    // Calculate health loss percentages
    if (maxHealth > 0)
    {
        float maxHf = static_cast<float>(maxHealth);
        _forecast.healthLossIn1Sec = (static_cast<float>(_forecast.damageIn1Sec) / maxHf) * 100.0f;
        _forecast.healthLossIn2Sec = (static_cast<float>(_forecast.damageIn2Sec) / maxHf) * 100.0f;
        _forecast.healthLossIn3Sec = (static_cast<float>(_forecast.damageIn3Sec) / maxHf) * 100.0f;
        _forecast.healthLossIn5Sec = (static_cast<float>(_forecast.damageIn5Sec) / maxHf) * 100.0f;
    }

    // Check for boss-level and lethal casts
    uint32 currentHealth = _bot->GetHealth();
    for (auto const& event : _forecast.events)
    {
        if (event.estimatedDamage >= currentHealth)
            _forecast.hasLethalCast = true;
    }

    // Classify severity
    _forecast.severity = ClassifySeverity();
}

// ============================================================================
// Phase 1: Spell Cast Prediction
// ============================================================================

void IncomingDamagePredictor::PredictFromSpellCasts(InterruptAwareness const* awareness)
{
    if (!awareness)
        return;

    auto activeCasts = awareness->GetHostileCasts();

    for (auto const& cast : activeCasts)
    {
        if (!cast.IsValid() || !cast.isHostile)
            continue;

        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(cast.spellId, DIFFICULTY_NONE);
        if (!spellInfo)
            continue;

        // Check if this spell deals damage
        bool dealsDamage = false;
        bool isAoE = false;
        for (auto const& effect : spellInfo->GetEffects())
        {
            if (effect.IsEffect(SPELL_EFFECT_SCHOOL_DAMAGE) ||
                effect.IsEffect(SPELL_EFFECT_WEAPON_DAMAGE) ||
                effect.IsEffect(SPELL_EFFECT_WEAPON_PERCENT_DAMAGE) ||
                effect.IsEffect(SPELL_EFFECT_NORMALIZED_WEAPON_DMG) ||
                effect.IsEffect(SPELL_EFFECT_HEALTH_LEECH) ||
                effect.IsEffect(SPELL_EFFECT_ENVIRONMENTAL_DAMAGE))
            {
                dealsDamage = true;
            }

            // Check for AoE targeting
            if (effect.TargetA.GetTarget() == TARGET_DEST_DEST ||
                effect.TargetA.GetTarget() == TARGET_UNIT_SRC_AREA_ENEMY ||
                effect.TargetA.GetTarget() == TARGET_UNIT_DEST_AREA_ENEMY ||
                effect.TargetA.GetTarget() == TARGET_UNIT_CONE_180_DEG_ENEMY ||
                effect.TargetA.GetTarget() == TARGET_UNIT_CONE_CASTER_TO_DEST_ENEMY)
            {
                isAoE = true;
            }
        }

        // Also check for DoT application which is indirect damage
        if (!dealsDamage)
        {
            for (auto const& effect : spellInfo->GetEffects())
            {
                if (effect.IsAura(SPELL_AURA_PERIODIC_DAMAGE) ||
                    effect.IsAura(SPELL_AURA_PERIODIC_DAMAGE_PERCENT) ||
                    effect.IsAura(SPELL_AURA_PERIODIC_LEECH))
                {
                    dealsDamage = true;
                    break;
                }
            }
        }

        if (!dealsDamage)
            continue;

        // Estimate the damage
        uint32 estimatedDamage = EstimateSpellDamage(cast);

        if (estimatedDamage == 0)
            continue;

        // Build prediction event
        PredictedDamageEvent event;
        event.sourceGuid = cast.casterGuid;
        event.spellId = cast.spellId;
        event.estimatedDamage = estimatedDamage;
        event.timeToImpactMs = cast.GetRemainingTime();
        event.schoolMask = cast.schoolMask;
        event.isAoE = isAoE;
        event.isAvoidable = isAoE; // AoE is typically avoidable by moving
        event.isInterruptible = cast.isInterruptible;
        event.confidence = 0.8f; // High confidence - we can see the cast happening

        // Check if caster is a boss
        if (Unit* caster = ObjectAccessor::GetUnit(*_bot, cast.casterGuid))
        {
            event.sourceName = caster->GetName();
            if (Creature* creature = caster->ToCreature())
            {
                if (creature->IsDungeonBoss() || creature->isWorldBoss())
                {
                    _forecast.hasBossCast = true;
                    event.confidence = 0.9f; // Boss casts are very predictable
                }
            }
        }

        _forecast.events.push_back(std::move(event));
    }
}

// ============================================================================
// Phase 2: Melee Attacker Prediction
// ============================================================================

void IncomingDamagePredictor::PredictFromMeleeAttackers()
{
    if (!_bot->IsInCombat())
        return;

    // Check who is currently attacking us via threat list
    ThreatManager& threatMgr = _bot->GetThreatManager();

    for (ThreatReference const* ref : threatMgr.GetUnsortedThreatList())
    {
        if (!ref || !ref->GetVictim())
            continue;

        Unit* attacker = ref->GetVictim();
        if (!attacker || attacker->isDead())
            continue;

        // Only predict melee damage from units actually targeting us
        if (attacker->GetVictim() != _bot)
            continue;

        if (!IsInMeleeRange(attacker))
            continue;

        uint32 meleeDamage = EstimateMeleeDamage(attacker);
        if (meleeDamage == 0)
            continue;

        // Melee swings happen approximately every 2 seconds
        // Predict 2-3 swings in our forecast window
        float swingTime = attacker->GetBaseAttackTime(BASE_ATTACK) > 0
            ? static_cast<float>(attacker->GetBaseAttackTime(BASE_ATTACK))
            : 2000.0f;

        for (float t = swingTime; t <= 5000.0f; t += swingTime)
        {
            PredictedDamageEvent event;
            event.sourceGuid = attacker->GetGUID();
            event.spellId = 0; // Melee
            event.estimatedDamage = meleeDamage;
            event.timeToImpactMs = static_cast<uint32>(t);
            event.schoolMask = SPELL_SCHOOL_MASK_NORMAL;
            event.isAoE = false;
            event.isAvoidable = false;
            event.isInterruptible = false;
            event.confidence = 0.5f; // Medium confidence - can miss/dodge/parry

            if (Unit* unit = attacker)
            {
                event.sourceName = unit->GetName();
                if (Creature* creature = unit->ToCreature())
                {
                    if (creature->IsDungeonBoss() || creature->isWorldBoss())
                    {
                        _forecast.hasBossCast = true;
                        event.confidence = 0.7f;
                    }
                }
            }

            _forecast.events.push_back(std::move(event));
        }
    }
}

// ============================================================================
// Phase 3: Historical Pattern Prediction
// ============================================================================

void IncomingDamagePredictor::PredictFromHistory()
{
    // Use historical DPS to provide a baseline damage prediction
    // This catches periodic effects, DoTs, and other sources not detected by spell scanning
    if (_totalIncomingDPS <= 0.0f)
        return;

    // Calculate how much "unaccounted" damage we expect
    // Subtract damage already predicted by spells and melee
    float spellAndMeleePredicted = 0.0f;
    for (auto const& event : _forecast.events)
    {
        if (event.timeToImpactMs <= 3000)
            spellAndMeleePredicted += static_cast<float>(event.estimatedDamage);
    }

    // Historical prediction for 3 seconds
    float historicalPrediction = _totalIncomingDPS * 3.0f;

    // Only add the difference if historical exceeds spell+melee predictions
    if (historicalPrediction > spellAndMeleePredicted * 1.2f)
    {
        float unaccounted = historicalPrediction - spellAndMeleePredicted;

        PredictedDamageEvent event;
        event.sourceGuid = ObjectGuid::Empty;
        event.spellId = 0;
        event.estimatedDamage = static_cast<uint32>(unaccounted);
        event.timeToImpactMs = 1500; // Spread over the window
        event.schoolMask = SPELL_SCHOOL_MASK_NORMAL;
        event.isAoE = false;
        event.isAvoidable = false;
        event.isInterruptible = false;
        event.confidence = 0.3f; // Low confidence - extrapolation
        event.sourceName = "Historical";

        _forecast.events.push_back(std::move(event));
    }
}

// ============================================================================
// Spell Damage Estimation
// ============================================================================

uint32 IncomingDamagePredictor::EstimateSpellDamage(DetectedSpellCast const& cast) const
{
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(cast.spellId, DIFFICULTY_NONE);
    if (!spellInfo)
        return 0;

    // Get caster level for scaling
    uint32 casterLevel = 80; // Default estimate
    if (Unit* caster = ObjectAccessor::GetUnit(*_bot, cast.casterGuid))
        casterLevel = caster->GetLevel();

    return EstimateSpellInfoDamage(spellInfo, casterLevel);
}

uint32 IncomingDamagePredictor::EstimateSpellInfoDamage(SpellInfo const* spellInfo, uint32 casterLevel) const
{
    if (!spellInfo)
        return 0;

    uint32 totalDamage = 0;

    for (auto const& effect : spellInfo->GetEffects())
    {
        // Direct damage effects
        if (effect.IsEffect(SPELL_EFFECT_SCHOOL_DAMAGE) ||
            effect.IsEffect(SPELL_EFFECT_HEALTH_LEECH) ||
            effect.IsEffect(SPELL_EFFECT_ENVIRONMENTAL_DAMAGE))
        {
            // Use base points from the effect
            int32 baseDamage = effect.CalcValue();

            if (baseDamage > 0)
            {
                totalDamage += static_cast<uint32>(baseDamage);
            }
            else
            {
                // Fallback: estimate based on caster level
                // Most spells at max level deal roughly level * 50-200 damage
                totalDamage += casterLevel * 100;
            }
        }
        // Weapon damage effects
        else if (effect.IsEffect(SPELL_EFFECT_WEAPON_DAMAGE) ||
                 effect.IsEffect(SPELL_EFFECT_NORMALIZED_WEAPON_DMG) ||
                 effect.IsEffect(SPELL_EFFECT_WEAPON_PERCENT_DAMAGE))
        {
            // Estimate weapon damage based on level
            totalDamage += casterLevel * 80;
        }
        // Periodic damage (DoT application)
        else if (effect.IsAura(SPELL_AURA_PERIODIC_DAMAGE))
        {
            int32 tickDamage = effect.CalcValue();
            uint32 amplitude = effect.ApplyAuraPeriod > 0 ? effect.ApplyAuraPeriod : 3000;
            uint32 duration = spellInfo->GetMaxDuration() > 0
                ? static_cast<uint32>(spellInfo->GetMaxDuration())
                : 15000;
            uint32 ticks = duration / amplitude;

            if (tickDamage > 0 && ticks > 0)
            {
                // Report total DoT damage over duration
                totalDamage += static_cast<uint32>(tickDamage) * ticks;
            }
            else
            {
                totalDamage += casterLevel * 50;
            }
        }
    }

    // Apply a rough mitigation estimate (armor/resistance reduces effective damage)
    // We estimate ~30% mitigation for physical, ~20% for magic at endgame
    uint32 schoolMask = spellInfo->SchoolMask;
    float mitigationFactor = 1.0f;
    if (schoolMask == SPELL_SCHOOL_MASK_NORMAL)
        mitigationFactor = 0.7f; // Physical - armor mitigation
    else if (schoolMask != 0)
        mitigationFactor = 0.8f; // Magic - resistance mitigation

    return static_cast<uint32>(totalDamage * mitigationFactor);
}

// ============================================================================
// Severity Classification
// ============================================================================

DamageSeverity IncomingDamagePredictor::ClassifySeverity() const
{
    if (!_bot)
        return DamageSeverity::NONE;

    uint32 maxHealth = _bot->GetMaxHealth();

    // Check if any single hit would kill us
    if (_forecast.hasLethalCast)
        return DamageSeverity::LETHAL;

    // Calculate predicted health after damage
    float predictedHealthPct = GetPredictedHealthPercent(3000);

    // Role-adjusted thresholds
    float criticalThreshold = _isTank ? 15.0f : 20.0f;
    float highThreshold = _isTank ? 30.0f : 40.0f;
    float moderateThreshold = _isTank ? 50.0f : 60.0f;

    if (predictedHealthPct <= 0.0f)
        return DamageSeverity::LETHAL;

    if (predictedHealthPct <= criticalThreshold)
        return DamageSeverity::CRITICAL;

    if (predictedHealthPct <= highThreshold)
        return DamageSeverity::HIGH;

    if (predictedHealthPct <= moderateThreshold)
        return DamageSeverity::MODERATE;

    // Check if damage rate is high relative to max health
    if (maxHealth > 0)
    {
        float dmg3sPct = (static_cast<float>(_forecast.damageIn3Sec) / static_cast<float>(maxHealth)) * 100.0f;

        if (dmg3sPct > 60.0f)
            return DamageSeverity::CRITICAL;
        if (dmg3sPct > 40.0f)
            return DamageSeverity::HIGH;
        if (dmg3sPct > 20.0f)
            return DamageSeverity::MODERATE;
        if (dmg3sPct > 5.0f)
            return DamageSeverity::LOW;
    }

    return DamageSeverity::NONE;
}

// ============================================================================
// Defensive Recommendation
// ============================================================================

DefensiveRecommendation IncomingDamagePredictor::GetDefensiveRecommendation() const
{
    DefensiveRecommendation rec;

    if (!_bot || _forecast.severity == DamageSeverity::NONE)
        return rec;

    rec.severity = _forecast.severity;
    rec.estimatedDamage = _forecast.damageIn3Sec;

    // Determine primary damage school
    uint32 physicalDamage = 0;
    uint32 magicDamage = 0;
    for (auto const& event : _forecast.events)
    {
        if (event.timeToImpactMs > 3000)
            continue;

        if (event.schoolMask == SPELL_SCHOOL_MASK_NORMAL || event.schoolMask == 0)
            physicalDamage += event.estimatedDamage;
        else
            magicDamage += event.estimatedDamage;
    }

    rec.schoolMask = (magicDamage > physicalDamage)
        ? SPELL_SCHOOL_MASK_MAGIC
        : SPELL_SCHOOL_MASK_NORMAL;

    rec.preferMagicDefense = (magicDamage > physicalDamage * 2);
    rec.preferPhysicalDefense = (physicalDamage > magicDamage * 2);

    // Find earliest dangerous event for timing
    for (auto const& event : _forecast.events)
    {
        if (event.estimatedDamage > _bot->GetMaxHealth() * 0.15f)
        {
            rec.timeWindowMs = event.timeToImpactMs;
            break;
        }
    }

    // Decision logic based on severity
    switch (_forecast.severity)
    {
        case DamageSeverity::LETHAL:
            rec.shouldUseDefensive = true;
            rec.preferImmunity = true;
            rec.reason = "Lethal damage incoming - use immunity";
            break;

        case DamageSeverity::CRITICAL:
            rec.shouldUseDefensive = true;
            rec.reason = "Critical damage predicted - use major defensive";
            break;

        case DamageSeverity::HIGH:
        {
            float currentHp = _bot->GetHealthPct();
            // Use defensive if already low or if predicted to go low
            if (currentHp < 60.0f || GetPredictedHealthPercent(3000) < 30.0f)
            {
                rec.shouldUseDefensive = true;
                rec.reason = "Heavy damage + low health - use defensive";
            }
            else if (_forecast.hasBossCast)
            {
                rec.shouldUseDefensive = true;
                rec.reason = "Boss cast detected - preemptive defensive";
            }
            break;
        }

        case DamageSeverity::MODERATE:
        {
            // Only recommend for tanks or if already low health
            if (_isTank && _bot->GetHealthPct() < 50.0f)
            {
                rec.shouldUseDefensive = true;
                rec.reason = "Tank taking sustained damage";
            }
            else if (_bot->GetHealthPct() < 40.0f)
            {
                rec.shouldUseDefensive = true;
                rec.reason = "Low health with moderate incoming damage";
            }
            break;
        }

        default:
            break;
    }

    return rec;
}

bool IncomingDamagePredictor::ShouldUsePreemptiveDefensive() const
{
    return GetDefensiveRecommendation().shouldUseDefensive;
}

PredictedDamageEvent const* IncomingDamagePredictor::GetMostDangerousEvent() const
{
    PredictedDamageEvent const* worst = nullptr;

    for (auto const& event : _forecast.events)
    {
        if (!worst || event.estimatedDamage > worst->estimatedDamage)
            worst = &event;
    }

    return worst;
}

float IncomingDamagePredictor::GetPredictedHealthPercent(uint32 windowMs) const
{
    if (!_bot || _bot->GetMaxHealth() == 0)
        return 100.0f;

    uint32 predictedDamage = GetPredictedDamage(windowMs);
    float currentHealth = static_cast<float>(_bot->GetHealth());
    float maxHealth = static_cast<float>(_bot->GetMaxHealth());

    float healthAfterDamage = currentHealth - static_cast<float>(predictedDamage);
    if (healthAfterDamage < 0.0f)
        healthAfterDamage = 0.0f;

    return (healthAfterDamage / maxHealth) * 100.0f;
}

uint32 IncomingDamagePredictor::GetPredictedDamage(uint32 windowMs) const
{
    uint32 total = 0;

    for (auto const& event : _forecast.events)
    {
        if (event.timeToImpactMs <= windowMs)
            total += event.estimatedDamage;
    }

    return total;
}

// ============================================================================
// Historical Damage Tracking
// ============================================================================

void IncomingDamagePredictor::RecordDamageTaken(ObjectGuid sourceGuid, uint32 damage, uint32 spellId)
{
    if (damage == 0)
        return;

    uint32 now = GameTime::GetGameTimeMS();

    auto& history = _enemyHistory[sourceGuid];
    history.guid = sourceGuid;

    if (history.firstDamageTime == 0)
        history.firstDamageTime = now;

    history.lastDamageTime = now;
    history.totalDamage += damage;
    history.hitCount++;
    history.lastSpellId = spellId;

    // Recalculate DPS for this enemy
    uint32 elapsed = now - history.firstDamageTime;
    if (elapsed > 0)
        history.dps = static_cast<float>(history.totalDamage) / (static_cast<float>(elapsed) / 1000.0f);
}

float IncomingDamagePredictor::GetEnemyDPS(ObjectGuid enemyGuid) const
{
    auto it = _enemyHistory.find(enemyGuid);
    if (it == _enemyHistory.end())
        return 0.0f;

    return it->second.dps;
}

float IncomingDamagePredictor::GetTotalIncomingDPS() const
{
    return _totalIncomingDPS;
}

void IncomingDamagePredictor::UpdateHistoricalDPS(uint32 /*diff*/)
{
    float totalDPS = 0.0f;
    uint32 now = GameTime::GetGameTimeMS();

    for (auto& [guid, history] : _enemyHistory)
    {
        // Only count active enemies (damaged us in last HISTORY_WINDOW)
        if (now - history.lastDamageTime > HISTORY_WINDOW_MS)
            continue;

        uint32 elapsed = history.lastDamageTime - history.firstDamageTime;
        if (elapsed > 100) // Avoid division by very small numbers
        {
            history.dps = static_cast<float>(history.totalDamage)
                / (static_cast<float>(elapsed) / 1000.0f);
        }

        totalDPS += history.dps;
    }

    _totalIncomingDPS = totalDPS;
}

void IncomingDamagePredictor::PruneHistory()
{
    uint32 now = GameTime::GetGameTimeMS();

    for (auto it = _enemyHistory.begin(); it != _enemyHistory.end(); )
    {
        // Remove entries that haven't dealt damage in twice the window
        if (now - it->second.lastDamageTime > HISTORY_WINDOW_MS * 2)
            it = _enemyHistory.erase(it);
        else
            ++it;
    }
}

// ============================================================================
// Helper Methods
// ============================================================================

bool IncomingDamagePredictor::IsSpellTargetingBot(Unit* caster, SpellInfo const* spellInfo) const
{
    if (!caster || !spellInfo || !_bot)
        return false;

    // If the caster's current target is the bot, spell is likely targeting us
    if (caster->GetVictim() == _bot)
        return true;

    // Check for AoE spells - if we're in range, we're a target
    for (auto const& effect : spellInfo->GetEffects())
    {
        if (effect.TargetA.GetTarget() == TARGET_UNIT_SRC_AREA_ENEMY ||
            effect.TargetA.GetTarget() == TARGET_UNIT_DEST_AREA_ENEMY)
        {
            float range = spellInfo->GetMaxRange(false);
            if (range <= 0.0f)
                range = 30.0f;

            if (_bot->GetDistance(caster) <= range)
                return true;
        }
    }

    return false;
}

bool IncomingDamagePredictor::IsInMeleeRange(Unit* enemy) const
{
    if (!enemy || !_bot)
        return false;

    return _bot->IsWithinMeleeRange(enemy);
}

float IncomingDamagePredictor::GetEffectiveHealth(uint32 schoolMask) const
{
    if (!_bot)
        return 0.0f;

    float health = static_cast<float>(_bot->GetHealth());

    // Rough mitigation estimate
    if (schoolMask == SPELL_SCHOOL_MASK_NORMAL || schoolMask == 0)
    {
        // Physical - use armor
        float armor = static_cast<float>(_bot->GetArmor());
        float level = static_cast<float>(_bot->GetLevel());
        // Simplified armor formula: DR = armor / (armor + level * 85 + 400)
        float dr = armor / (armor + level * 85.0f + 400.0f);
        health /= (1.0f - std::min(dr, 0.85f));
    }

    return health;
}

uint32 IncomingDamagePredictor::EstimateMeleeDamage(Unit* attacker) const
{
    if (!attacker)
        return 0;

    // Use the attacker's base damage range for estimation
    float minDmg = attacker->GetWeaponDamageRange(BASE_ATTACK, MINDAMAGE);
    float maxDmg = attacker->GetWeaponDamageRange(BASE_ATTACK, MAXDAMAGE);

    if (minDmg <= 0.0f && maxDmg <= 0.0f)
    {
        // Fallback: estimate based on level
        uint32 level = attacker->GetLevel();
        minDmg = static_cast<float>(level * 30);
        maxDmg = static_cast<float>(level * 60);
    }

    float avgDmg = (minDmg + maxDmg) / 2.0f;

    // Apply rough armor mitigation (30% at endgame)
    return static_cast<uint32>(avgDmg * 0.7f);
}

} // namespace Playerbot
