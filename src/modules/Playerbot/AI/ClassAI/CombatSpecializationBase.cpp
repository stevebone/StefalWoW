/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "CombatSpecializationBase.h"
#include "GameTime.h"
#include "Player.h"
#include "Unit.h"
#include "Spell.h"
#include "SpellMgr.h"
#include "SpellInfo.h"
#include "SpellAuras.h"
#include "SpellHistory.h"
#include "ObjectAccessor.h"
#include "Group.h"
#include "Map.h"
#include "MapInstanced.h"
#include "MotionMaster.h"
#include "WorldSession.h"
#include "Log.h"
#include <execution>
#include <algorithm>
#include <numeric>
#include "../../Spatial/SpatialGridManager.h"
#include "../../Spatial/SpatialGridQueryHelpers.h"  // PHASE 5F: Thread-safe queries
#include "Movement/UnifiedMovementCoordinator.h"
#include "../../Movement/Arbiter/MovementPriority.h"
#include "../BotAI.h"
#include "Core/PlayerBotHelpers.h"
#include "UnitAI.h"

namespace Playerbot
{

// Thread-local optimization for frequently accessed data
thread_local uint32 g_currentUpdateTime = 0;
thread_local ::std::array<float, 16> g_distanceCache;
thread_local uint32 g_distanceCacheTime = 0;

// Object pool for performance metrics (avoid allocations during combat)
static ObjectPool<PerformanceMetrics> s_metricsPool(100);

CombatSpecializationBase::CombatSpecializationBase(Player* bot, CombatRole role, ResourceType primaryResource)
    : _bot(bot)
    , _role(role)
    , _primaryResource(primaryResource)
    , _globalCooldownEnd(0)
    , _lastThreatUpdate(0)
    , _currentTarget(nullptr)
    , _inCombat(false)
    , _combatStartTime(0)
    , _lastPositionUpdate(0)
    , _lastBuffCheck(0)
    , _lastResourceRegen(0)
    , _lastEmergencyCheck(0)
    , _consecutiveFailedCasts(0)
    , _lastOptimalPositionCheck(0)
    , _lastGroupUpdate(0)
    , _cachedTank(nullptr)
    , _cachedHealer(nullptr)
{
    // Pre-allocate containers to avoid runtime allocations
    _cooldowns.reserve(32);
    _buffExpirationTimes.reserve(16);
    _procExpirationTimes.reserve(8);
    _threatTable.reserve(16);
    _cachedGroupMembers.reserve(40);

    // Initialize position with bot's current position
    _lastOptimalPosition = bot->GetPosition();
}

// Core buff management with batched updates for performance
void CombatSpecializationBase::UpdateBuffs()
{
    uint32 currentTime = GameTime::GetGameTimeMS();

    // Throttle buff checks to reduce CPU usage
    if (currentTime - _lastBuffCheck < 500) // 500ms minimum between checks
        return;

    _lastBuffCheck = currentTime;

    // Batch process all buffs in one pass
    RefreshExpiringBuffs();

    // Clean up expired buff tracking data
    ::std::erase_if(_buffExpirationTimes, [currentTime](const auto& pair) {
        return pair.second < currentTime;
    });
}

// Optimized cooldown management with lock-free updates
void CombatSpecializationBase::UpdateCooldowns(uint32 diff)
{
    uint32 currentTime = GameTime::GetGameTimeMS();

    // Update global cooldown
    UpdateGlobalCooldown(diff);

    // Batch update all cooldowns
    for (auto& [spellId, cooldownEnd] : _cooldowns)
    {
        if (cooldownEnd > currentTime)

            continue;

        // Mark as ready (0 means ready)
        cooldownEnd = 0;
    }

    // Periodic cleanup of ready cooldowns (every 5 seconds)
    // QW-4 FIX: Removed static - now uses instance member _lastCooldownCleanup
    if (currentTime - _lastCooldownCleanup > 5000)
    {
        ::std::erase_if(_cooldowns, [](const auto& pair) {

            return pair.second == 0;
        });
        _lastCooldownCleanup = currentTime;
    }
}

// High-performance ability validation with caching
bool CombatSpecializationBase::CanUseAbility(uint32 spellId)
{
    // Fast path checks first
    if (!_bot || !_bot->IsAlive())
        return false;

    // Check if we have the spell
    if (!HasSpell(spellId))
        return false;

    // Check cooldown
    if (!IsSpellReady(spellId))
        return false;

    // Check global cooldown
    if (HasGlobalCooldown())
        return false;

    // Check if already casting/channeling
    if (IsCasting() || IsChanneling())
        return false;

    // Check resource requirements
    if (!HasEnoughResource(spellId))
        return false;

    // Check if spell is usable
    SpellInfo const* spellInfo = GetSpellInfo(spellId);
    if (!spellInfo)
        return false;

    // Additional validation can be added by derived classes
    return true;
}

// Combat lifecycle management with metrics tracking
void CombatSpecializationBase::OnCombatStart(::Unit* target)
{
    _inCombat = true;
    _combatStartTime = GameTime::GetGameTimeMS();
    _currentTarget = target;
    _consecutiveFailedCasts = 0;

    // Start performance tracking
    _metrics.combatStartTime = ::std::chrono::steady_clock::now();

    // Pre-calculate frequently used values
    if (target)
    {        UpdateThreatTable();
        UpdateDoTTracking(target);
    }

    // Reset cooldowns if configured
    if (_bot->GetLevel() >= 60) // High level bots get cooldown reset    {
        ResetAllCooldowns();
    }

    TC_LOG_DEBUG("playerbot", "CombatSpecializationBase: {} entering combat with {}",
        _bot->GetName(), target ? target->GetName() : "unknown");
}

void CombatSpecializationBase::OnCombatEnd()
{
    _inCombat = false;
    _currentTarget = nullptr;

    // Update combat metrics
    auto combatDuration = ::std::chrono::steady_clock::now() - _metrics.combatStartTime;
    _metrics.totalCombatTime += ::std::chrono::duration_cast<::std::chrono::milliseconds>(combatDuration);

    // Clear combat-specific data
    _dotTracking.clear();
    _threatTable.clear();
    _procExpirationTimes.clear();

    // Log performance if significant combat
    if (combatDuration > ::std::chrono::seconds(10))
    {        LogPerformance();
    }
}
// Optimized resource management
bool CombatSpecializationBase::HasEnoughResource(uint32 spellId)
{
    SpellInfo const* spellInfo = GetSpellInfo(spellId);
    if (!spellInfo)        return false;

    // Check primary resource based on type
    switch (_primaryResource)
    {
        case ResourceType::MANA:

            return _bot->GetPower(POWER_MANA) >= GetSpellManaCost(spellId);
            case ResourceType::RAGE:

            return _bot->GetPower(POWER_RAGE) >= spellInfo->CalcPowerCost(_bot, spellInfo->GetSchoolMask());
            case ResourceType::ENERGY:

            return _bot->GetPower(POWER_ENERGY) >= spellInfo->CalcPowerCost(_bot, spellInfo->GetSchoolMask());
            case ResourceType::FOCUS:

            return _bot->GetPower(POWER_FOCUS) >= spellInfo->CalcPowerCost(_bot, spellInfo->GetSchoolMask());
            case ResourceType::RUNIC_POWER:

            return _bot->GetPower(POWER_RUNIC_POWER) >= spellInfo->CalcPowerCost(_bot, spellInfo->GetSchoolMask());
            case ResourceType::COMBO_POINTS:

            return _bot->GetPower(POWER_COMBO_POINTS) >= 1; // Minimum 1 combo point

        default:

            return true; // No resource requirement or unknown type
    }
}

// Optimized positioning with prediction and caching
Position CombatSpecializationBase::GetOptimalPosition(::Unit* target)
{
    if (!target)
        return _bot->GetPosition();

    uint32 currentTime = GameTime::GetGameTimeMS();

    // Use cached position if recent enough (100ms cache)
    if (currentTime - _lastOptimalPositionCheck < 100)
        return _lastOptimalPosition;

    _lastOptimalPositionCheck = currentTime;

    // Calculate based on role
    float optimalDistance = GetOptimalRange(target);
    float currentDistance = GetDistance(target);    // If already in optimal range, maintain position
    if (::std::abs(currentDistance - optimalDistance) < 2.0f)
    {
        _lastOptimalPosition = _bot->GetPosition();
        return _lastOptimalPosition;
    }

    // Calculate new position
    float angle = _bot->GetAngle(target);

    // Tanks stay in front, DPS prefer behind/side
    if (_role == CombatRole::MELEE_DPS)
    {
        // Try to get behind target
        angle = target->GetOrientation() + static_cast<float>(M_PI);
        
    }
    else if (_role == CombatRole::TANK)
    {
        // Face target head-on
        angle = target->GetAngle(_bot);
    }

    // Calculate position with terrain validation
    float x = target->GetPositionX() + cos(angle) * optimalDistance;
    float y = target->GetPositionY() + sin(angle) * optimalDistance;    float z = target->GetPositionZ();    // Ensure position is valid and reachable
    _bot->UpdateGroundPositionZ(x, y, z);

    _lastOptimalPosition = Position(x, y, z, angle);
    return _lastOptimalPosition;
}

float CombatSpecializationBase::GetOptimalRange(::Unit* target)
{
    if (!target)
        return 0.0f;

    switch (_role)
    {
        case CombatRole::TANK:
        case CombatRole::MELEE_DPS:

            return BOT_MELEE_RANGE;

        case CombatRole::RANGED_DPS:
        case CombatRole::HEALER:

            return RANGED_OPTIMAL_DISTANCE;

        default:

            return RANGED_MIN_DISTANCE;
    }
}

// High-performance interrupt handling with coordination
bool CombatSpecializationBase::ShouldInterrupt(::Unit* target)
{
    if (!target || !target->IsAlive())
        return false;

    // Check if target is casting an interruptible spell
    if (!target->HasUnitState(UNIT_STATE_CASTING))
        return false;

    Spell const* spell = target->GetCurrentSpell(CURRENT_GENERIC_SPELL);
    if (!spell)
        spell = target->GetCurrentSpell(CURRENT_CHANNELED_SPELL);

    if (!spell)
        return false;

    SpellInfo const* spellInfo = spell->GetSpellInfo();
    if (!spellInfo)
        return false;    // Check if spell is interruptible
    if (!spellInfo->HasAttribute(SPELL_ATTR4_CAN_BE_INTERRUPTED))
        return false;    // High priority interrupts (heals, crowd control)
    if (spellInfo->HasEffect(SPELL_EFFECT_HEAL) ||
        spellInfo->HasEffect(SPELL_EFFECT_HEAL_MAX_HEALTH) ||
        spellInfo->HasAura(SPELL_AURA_MOD_STUN) ||
        spellInfo->HasAura(SPELL_AURA_MOD_FEAR))
    {
        return true;
    }

    // Check remaining cast time (interrupt near end for efficiency)
    uint32 remainingTime = spell->GetCurrentCastTime();
    if (remainingTime > 0 && remainingTime < 1000) // Less than 1 second remaining
        return true;

    return false;
}

// Optimized target selection with threat consideration
::Unit* CombatSpecializationBase::SelectBestTarget()
{
    ::std::vector<::Unit*> enemies = GetNearbyEnemies();
    if (enemies.empty())
        return nullptr;

    // Lambda for scoring targets
    auto scoreTarget = [this](::Unit* target) -> float {
        float score = 100.0f;

        // Prefer current target (target switching penalty)
    if (target == _currentTarget)

            score += 20.0f;

        // Health percentage factor
        float healthPct = target->GetHealthPct();
        if (healthPct < 20.0f)

            score += 30.0f; // Execute range priority

        // Distance factor (closer is better for melee, optimal range for ranged)
        float distance = GetDistance(target);
        float optimalRange = GetOptimalRange(target);
        float distancePenalty = ::std::abs(distance - optimalRange);
        score -= distancePenalty;

        // Threat factor (tanks want high threat targets)
    if (_role == CombatRole::TANK)        {

            float threat = CalculateThreatLevel(target);
            score += threat * 0.5f;
        }

        // Debuff factor (prefer targets with our DoTs)
    if (_dotTracking.contains(target->GetGUID().GetRawValue()))
        score += 10.0f;

        return score;
    };

    // Find best target using parallel execution for large enemy counts
    if (enemies.size() > 10)
    {
        return *::std::max_element(::std::execution::par_unseq,

            enemies.begin(), enemies.end(),

            [&scoreTarget](::Unit* a, ::Unit* b) {

                return scoreTarget(a) < scoreTarget(b);

            });
    }
    else
    {
        return *::std::max_element(enemies.begin(), enemies.end(),

            [&scoreTarget](::Unit* a, ::Unit* b) {

                return scoreTarget(a) < scoreTarget(b);

            });
    }
}

// Efficient nearby unit detection with spatial indexing
::std::vector<ObjectGuid> GetNearbyEnemies(float range) const
{
    ::std::vector<ObjectGuid> guids;
    auto grid = sSpatialGridManager.GetGrid(_bot->GetMap());    if (!grid) return guids;

    float range = range;
    auto creatures = grid->QueryNearbyCreatures(_bot->GetPosition(), range);
        for (auto const& snapshot : creatures)
        {
        if (snapshot.isAlive && snapshot.isHostile)

            guids.push_back(snapshot.guid);
    }

    return guids;
}

    // Query nearby GUIDs (lock-free!)
    ::std::vector<ObjectGuid> nearbyGuids = spatialGrid->QueryNearbyCreatureGuids(
        _bot->GetPosition(), range);

    // Process results (replace old loop)
    for (ObjectGuid guid : nearbyGuids)
    {
        // PHASE 5F: Thread-safe spatial grid validation        auto snapshot_entity = SpatialGridQueryHelpers::FindCreatureByGuid(_bot, guid);
        Creature* entity = nullptr;
        if (snapshot_entity)
        {

        }
        if (!entity)

            continue;
        // Original filtering logic goes here
    }
    // End of spatial grid fix

    // Filter out invalid targets
    ::std::erase_if(enemies, [this](::Unit* unit) {
        return !IsValidTarget(unit);
    });

    return enemies;
}

std::vector<ObjectGuid> GetNearbyAllies(float range) const
{
    std::vector<ObjectGuid> guids;

    auto grid = sSpatialGridManager.GetGrid(_bot->GetMap());
    if (!grid) return guids;

    float range = range;
    auto creatures = grid->QueryNearbyCreatures(_bot->GetPosition(), range);
    for (auto const& snapshot : creatures) {
        if (snapshot.isAlive && snapshot.isHostile)

            guids.push_back(snapshot.guid);
    }

    return guids;
}

    // Query nearby GUIDs (lock-free!)
    std::vector<ObjectGuid> nearbyGuids = spatialGrid->QueryNearbyCreatureGuids(
        _bot->GetPosition(), range);

    // Process results (replace old loop)
    for (ObjectGuid guid : nearbyGuids)
    {
        // PHASE 5F: Thread-safe spatial grid validation
        auto snapshot_entity = SpatialGridQueryHelpers::FindCreatureByGuid(_bot, guid);
        Creature* entity = nullptr;
        if (snapshot_entity)
        {

        }
        if (!entity)

            continue;
        // Original filtering logic goes here
    }
    // End of spatial grid fix

    return allies;
}

// DoT tracking with efficient updates
void CombatSpecializationBase::UpdateDoTTracking(::Unit* target)
{
    if (!target)
        return;

    uint64 targetGuid = target->GetGUID().GetRawValue();
    uint32 currentTime = GameTime::GetGameTimeMS();

    // Check target's auras for our DoTs
    Unit::AuraApplicationMap const& auras = target->GetAppliedAuras();
    for (auto const& [auraId, auraApp] : auras)
    {
        Aura* aura = auraApp->GetBase();

        if (!aura || aura->GetCasterGUID() != _bot->GetGUID())
        continue;

        // Track DoT expiration
        uint32 duration = aura->GetDuration();
        if (duration > 0)
        {

            _dotTracking[targetGuid][aura->GetId()] = currentTime + duration;
            }
    }

    // Clean up expired DoTs
    if (_dotTracking.contains(targetGuid))
    {
        auto& dots = _dotTracking[targetGuid];
        std::erase_if(dots, [currentTime](const auto& pair) {

            return pair.second < currentTime;
            });

        // Remove target entry if no DoTs remain
    if (dots.empty())

            _dotTracking.erase(targetGuid);
    }
}

bool CombatSpecializationBase::ShouldRefreshDoT(::Unit* target, uint32 spellId, uint32 threshold) const
{
    if (!target)
        return true; // Apply if no target info

    uint64 targetGuid = target->GetGUID().GetRawValue();    // Check if DoT exists
    auto targetIt = _dotTracking.find(targetGuid);
    if (targetIt == _dotTracking.end())
        return true; // No DoTs on target, should apply

    auto dotIt = targetIt->second.find(spellId);
    if (dotIt == targetIt->second.end())
        return true; // This DoT not on target    // Check remaining time
    uint32 currentTime = GameTime::GetGameTimeMS();
    uint32 remaining = dotIt->second > currentTime ? dotIt->second - currentTime : 0;

    return remaining < threshold;
}

// Emergency response with intelligent cooldown usage
void CombatSpecializationBase::HandleEmergencySituation()
{    if (!_bot || !_bot->IsAlive())
        return;

    uint32 currentTime = GameTime::GetGameTimeMS();

    // Throttle emergency checks to avoid spam
    if (currentTime - _lastEmergencyCheck < 200)
        return;

    _lastEmergencyCheck = currentTime;
    _metrics.emergencyActions++;

    float healthPct = _bot->GetHealthPct();

    // Critical health - use all defensive cooldowns
    if (healthPct < EMERGENCY_HEALTH_PCT)
    {
        UseDefensiveCooldowns();

        // Try to use potions
    if (ShouldUsePotions())

            UsePotions();

        // Notify healer if in group
    if (IsInGroup())
        {

            Player* healer = GetGroupHealer();

            if (healer && healer != _bot)
            {
                // Healer notification would go here

                TC_LOG_DEBUG("playerbot", "{} requesting emergency healing at {}% health",

                    _bot->GetName(), healthPct);

            }
        }
    }
}

// Spell casting with comprehensive error handling
bool CombatSpecializationBase::CastSpell(uint32 spellId, ::Unit* target)
{
    if (!CanUseAbility(spellId))
    {
        _consecutiveFailedCasts++;
        _metrics.failedCasts++;
        return false;
    }

    SpellInfo const* spellInfo = GetSpellInfo(spellId);
    if (!spellInfo)
        return false;

    // Determine target
    ::Unit* actualTarget = target;
    if (!actualTarget)
    {
        if (spellInfo->IsPositive())

            actualTarget = _bot;
        else

            actualTarget = _currentTarget;
    }

    // Validate target
    if (actualTarget && !spellInfo->IsPositive())
    {
        if (!_bot->IsValidAttackTarget(actualTarget))

            return false;

        // Range check
    if (!IsInCastRange(actualTarget, spellId))

            return false;
    }

    // Cast the spell
    SpellCastTargets targets;
    if (actualTarget)
        targets.SetUnitTarget(actualTarget);

    Spell* spell = new Spell(_bot, spellInfo, TRIGGERED_NONE);
    ::SpellCastResult result = spell->prepare(targets);

    if (result == SPELL_CAST_OK)
    {
        _consecutiveFailedCasts = 0;
        _metrics.totalCasts++;

        // Set cooldown
        uint32 cooldown = spellInfo->RecoveryTime;
        if (cooldown > 0)

            SetSpellCooldown(spellId, cooldown);

        // Set global cooldown
    if (!spellInfo->HasAttribute(SPELL_ATTR0_NO_GCD))

            _globalCooldownEnd = GameTime::GetGameTimeMS() + GLOBAL_COOLDOWN_MS;

        // Consume resource
        ConsumeResource(spellId);

        return true;
    }
    else
    {
        delete spell;
        _consecutiveFailedCasts++;
        _metrics.failedCasts++;


        TC_LOG_DEBUG("playerbot", "{} failed to cast {} on {}: {}",
        _bot->GetName(), spellInfo->SpellName[0],

            actualTarget ? actualTarget->GetName() : "self",

            magic_enum::enum_name(result));
            return false;
    }
}
// Performance monitoring and metricsvoid CombatSpecializationBase::ResetMetrics()
{
    _metrics = PerformanceMetrics();
}

void CombatSpecializationBase::LogPerformance() const
{
    if (_metrics.totalCombatTime.count() == 0)
        return;

    float combatSeconds = _metrics.totalCombatTime.count() / 1000.0f;
    float dps = combatSeconds > 0 ? _metrics.totalDamageDealt / combatSeconds : 0;
    float hps = combatSeconds > 0 ? _metrics.totalHealingDone / combatSeconds : 0;
    float castSuccessRate = _metrics.totalCasts > 0 ?        100.0f * (_metrics.totalCasts - _metrics.failedCasts) / _metrics.totalCasts : 0;

    TC_LOG_INFO("playerbot.performance",
        "Bot {} Performance: DPS={:.1f} HPS={:.1f} CastSuccess={:.1f}% "
        "Interrupts={}/{} EmergencyActions={} CombatTime={:.1f}s",
        
        _bot->GetName(), dps, hps, castSuccessRate,
        _metrics.interruptsSuccessful,
        _metrics.interruptsSuccessful + _metrics.interruptsFailed,
        _metrics.emergencyActions,
        combatSeconds);
}

// Internal update methods
void CombatSpecializationBase::UpdateGlobalCooldown(uint32 diff)
{
    uint32 currentTime = GameTime::GetGameTimeMS();
    if (_globalCooldownEnd > currentTime)
    {
        // Still on GCD
        return;
    }

    _globalCooldownEnd = 0; // GCD expired
}

void CombatSpecializationBase::UpdateBuffTimers(uint32 diff)
{
    uint32 currentTime = GameTime::GetGameTimeMS();

    // Update buff expiration times
    for (auto it = _buffExpirationTimes.begin(); it != _buffExpirationTimes.end();)
    {
        if (it->second <= currentTime)

            it = _buffExpirationTimes.erase(it);
        else

            ++it;
            }
}

void CombatSpecializationBase::UpdateMetrics(uint32 diff)
{
    // Update performance metrics
    if (_inCombat)
    {
        _metrics.positioningUpdates++;
    }
}

// Helper methods implementation
bool CombatSpecializationBase::HasSpell(uint32 spellId) const
{    return _bot && _bot->HasSpell(spellId);
}

SpellInfo const* CombatSpecializationBase::GetSpellInfo(uint32 spellId) const
{
    return sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
}

uint32 CombatSpecializationBase::GetSpellCastTime(uint32 spellId) const
{
    SpellInfo const* spellInfo = GetSpellInfo(spellId);
    if (!spellInfo)
        return 0;

    return spellInfo->CalcCastTime(_bot);
}

uint32 CombatSpecializationBase::GetSpellManaCost(uint32 spellId) const
{
    SpellInfo const* spellInfo = GetSpellInfo(spellId);
    if (!spellInfo)
        return 0;

    return spellInfo->CalcPowerCost(_bot, spellInfo->GetSchoolMask());
}

bool CombatSpecializationBase::IsSpellReady(uint32 spellId) const
{
    auto it = _cooldowns.find(spellId);
    if (it == _cooldowns.end())
        return true; // No cooldown tracked

    return it->second <= GameTime::GetGameTimeMS();
}

void CombatSpecializationBase::SetSpellCooldown(uint32 spellId, uint32 cooldownMs)
{
    _cooldowns[spellId] = GameTime::GetGameTimeMS() + cooldownMs;
}

bool CombatSpecializationBase::IsCasting() const
{
    return _bot && _bot->HasUnitState(UNIT_STATE_CASTING);
}

bool CombatSpecializationBase::IsChanneling() const
{
    return _bot && _bot->GetCurrentSpell(CURRENT_CHANNELED_SPELL) != nullptr;
}

float CombatSpecializationBase::GetDistance(::Unit* target) const
{
    if (!_bot || !target)
        return 999.0f;

    // Use cached distance if available and recent
    uint32 currentTime = GameTime::GetGameTimeMS();
    if (g_distanceCacheTime == currentTime && target == _currentTarget)
    {
        return g_distanceCache[0];
    }

    float dist = std::sqrt(_bot->GetExactDistSq(target)); // Calculate once from squared distance    // Update cache
    g_distanceCache[0] = dist;
    g_distanceCacheTime = currentTime;

    return dist;
}

bool CombatSpecializationBase::IsInMeleeRange(::Unit* target) const
{
    return GetDistance(target) <= BOT_MELEE_RANGE;
}

bool CombatSpecializationBase::IsInCastRange(::Unit* target, uint32 spellId) const
{
    SpellInfo const* spellInfo = GetSpellInfo(spellId);
    if (!spellInfo)
        return false;

    float maxRange = spellInfo->GetMaxRange(spellInfo->IsPositive());
    return GetDistance(target) <= maxRange;
}

// Group coordination helpers
bool CombatSpecializationBase::IsInGroup() const
{
    return _bot && _bot->GetGroup() != nullptr;}

bool CombatSpecializationBase::IsInRaid() const
{
    Group* group = _bot ? _bot->GetGroup() : nullptr;
    return group && group->IsRaidGroup();
}

Player* CombatSpecializationBase::GetGroupTank() const
{
    // Use cached value if recent
    uint32 currentTime = GameTime::GetGameTimeMS();
    if (currentTime - _lastGroupUpdate < 5000) // 5 second cache
        return _cachedTank;

    // Find tank in group
    if (!IsInGroup())
        return nullptr;

    Group* group = _bot->GetGroup();
    for (GroupReference* itr : *group)
    {
        Player* member = itr->GetSource();
        if (!member || member == _bot)

            continue;

        // Simple tank detection based on spec/stance
        // This would need proper implementation based on your spec system
    if (member->GetClass() == CLASS_WARRIOR || member->GetClass() == CLASS_PALADIN)        {

            const_cast<CombatSpecializationBase*>(this)->_cachedTank = member;

            const_cast<CombatSpecializationBase*>(this)->_lastGroupUpdate = currentTime;

            return member;
        }
    }

    return nullptr;
}

std::vector<Player*> CombatSpecializationBase::GetGroupMembers() const
{
    // Use cached value if recent
    uint32 currentTime = GameTime::GetGameTimeMS();
    if (currentTime - _lastGroupUpdate < 2000 && !_cachedGroupMembers.empty())
        return _cachedGroupMembers;

    std::vector<Player*> members;

    if (!IsInGroup())
    {
        members.push_back(_bot);
        return members;
    }

    Group* group = _bot->GetGroup();
    members.reserve(group->GetMembersCount());

    for (GroupReference* itr : *group)
    {
        Player* member = itr->GetSource();
        if (member && member->IsAlive())

            members.push_back(member);
    }

    // Update cache
    const_cast<CombatSpecializationBase*>(this)->_cachedGroupMembers = members;
    const_cast<CombatSpecializationBase*>(this)->_lastGroupUpdate = currentTime;

    return members;
}

// Remaining implementations for completeness
void CombatSpecializationBase::OnTargetSwitch(::Unit* oldTarget, ::Unit* newTarget)
{
    _currentTarget = newTarget;

    if (newTarget)
    {
        UpdateDoTTracking(newTarget);
        _lastOptimalPositionCheck = 0; // Force position recalculation
    }
}

void CombatSpecializationBase::OnDamageTaken(::Unit* attacker, uint32 damage)
{
    _metrics.totalDamageTaken += damage;

    // Update threat for this attacker
    if (attacker)
    {
        uint64 guid = attacker->GetGUID().GetRawValue();
        _threatTable[guid] += damage * 1.1f; // Damage taken generates threat
    }
}

void CombatSpecializationBase::OnDamageDealt(::Unit* target, uint32 damage)
{
    _metrics.totalDamageDealt += damage;
}

void CombatSpecializationBase::OnHealingReceived(::Unit* healer, uint32 amount)
{
    // Track healing received for metrics
}

void CombatSpecializationBase::OnHealingDone(::Unit* target, uint32 amount)
{
    _metrics.totalHealingDone += amount;
}

void CombatSpecializationBase::ConsumeResource(uint32 spellId)
{
    SpellInfo const* spellInfo = GetSpellInfo(spellId);
    if (!spellInfo)
        return;

    uint32 cost = spellInfo->CalcPowerCost(_bot, spellInfo->GetSchoolMask());
    if (cost == 0)
        return;

    // Deduct resource based on type
    switch (_primaryResource)
    {
        case ResourceType::MANA:

            _bot->ModifyPower(POWER_MANA, -int32(cost));

            break;
        case ResourceType::RAGE:

            _bot->ModifyPower(POWER_RAGE, -int32(cost));

            break;
        case ResourceType::ENERGY:

            _bot->ModifyPower(POWER_ENERGY, -int32(cost));

            break;
        case ResourceType::FOCUS:

            _bot->ModifyPower(POWER_FOCUS, -int32(cost));

            break;
        case ResourceType::RUNIC_POWER:

            _bot->ModifyPower(POWER_RUNIC_POWER, -int32(cost));

            break;
        default:

            break;
    }
}

uint32 CombatSpecializationBase::GetCurrentResource() const
{
    if (!_bot)
        return 0;

    switch (_primaryResource)
    {
        case ResourceType::MANA:
            

            return _bot->GetPower(POWER_MANA);
        case ResourceType::RAGE:
            

            return _bot->GetPower(POWER_RAGE);
        case ResourceType::ENERGY:
            

            return _bot->GetPower(POWER_ENERGY);

        case ResourceType::FOCUS:
        return _bot->GetPower(POWER_FOCUS);

        case ResourceType::RUNIC_POWER:
        return _bot->GetPower(POWER_RUNIC_POWER);

        case ResourceType::COMBO_POINTS:
        return _bot->GetPower(POWER_COMBO_POINTS);
        default:

            return 0;
    }
}

uint32 CombatSpecializationBase::GetMaxResource() const
{
    if (!_bot)
        return 0;
        switch (_primaryResource)
    {
        case ResourceType::MANA:

            return _bot->GetMaxPower(POWER_MANA);
        case ResourceType::RAGE:

            return _bot->GetMaxPower(POWER_RAGE);

        case ResourceType::ENERGY:
        return _bot->GetMaxPower(POWER_ENERGY);
        case ResourceType::FOCUS:
        return _bot->GetMaxPower(POWER_FOCUS);
        case ResourceType::RUNIC_POWER:

            return _bot->GetMaxPower(POWER_RUNIC_POWER);
        case ResourceType::COMBO_POINTS:

            return 5; // Max combo points
        default:

            return 100;
    }
}

float CombatSpecializationBase::GetResourcePercent() const
{
    uint32 current = GetCurrentResource();
    uint32 max = GetMaxResource();

    return max > 0 ? (100.0f * current / max) : 0.0f;
}

void CombatSpecializationBase::RefreshExpiringBuffs()
{
    if (!_bot || !_bot->IsInWorld())
        return;

    uint32 currentTime = GameTime::GetGameTimeMS();

    // Get all positive auras on the bot that we might want to refresh
    Unit::AuraMap const& auras = _bot->GetOwnedAuras();

    for (auto const& [spellId, aura] : auras)
    {
        if (!aura || !aura->IsPositive())
            continue;

        // Skip passive/permanent auras
        int32 maxDuration = aura->GetMaxDuration();
        if (maxDuration == -1 || maxDuration == 0)
            continue;

        // Check remaining duration
        int32 remainingTime = aura->GetDuration();
        if (remainingTime <= 0 || remainingTime > static_cast<int32>(BUFF_REFRESH_THRESHOLD_MS))
            continue;

        // Only refresh buffs we can cast ourselves
        if (!_bot->HasSpell(spellId))
            continue;

        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, _bot->GetMap() ? _bot->GetMap()->GetDifficultyID() : DIFFICULTY_NONE);
        if (!spellInfo)
            continue;

        // Verify it's a self-buff (can target self)
        bool canTargetSelf = false;
        for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
        {
            SpellEffectInfo const& effect = spellInfo->GetEffect(SpellEffIndex(i));
            if (effect.TargetA.GetTarget() == TARGET_UNIT_CASTER ||
                effect.TargetB.GetTarget() == TARGET_UNIT_CASTER ||
                effect.TargetA.GetTarget() == TARGET_UNIT_TARGET_ALLY)
            {
                canTargetSelf = true;
                break;
            }
        }

        if (!canTargetSelf)
            continue;

        // Check if we're on GCD
        if (HasGlobalCooldown())
            break;  // Can't cast anything right now

        // Check spell cooldown
        if (_bot->GetSpellHistory()->HasCooldown(spellId))
            continue;

        // Check resource cost
        SpellPowerCost powerCost = spellInfo->CalcPowerCost(_bot, spellInfo->GetSchoolMask());
        Powers powerType = powerCost.empty() ? POWER_MANA : powerCost[0].Power;
        int32 cost = powerCost.empty() ? 0 : powerCost[0].Amount;

        if (cost > 0 && _bot->GetPower(powerType) < cost)
            continue;

        // Check if we're casting something else
        if (IsCasting() || IsChanneling())
            break;  // Don't interrupt current casts

        // Cast the refresh spell
        if (CastSpell(spellId, _bot))
        {
            // Update our tracking
            _buffExpirationTimes[spellId] = currentTime + static_cast<uint32>(maxDuration);

            TC_LOG_DEBUG("module.playerbot.buff",
                "CombatSpecializationBase: Bot {} refreshed buff {} ({} ms remaining)",
                _bot->GetName(), spellInfo->SpellName[0], remainingTime);

            // Only refresh one buff per update to avoid GCD conflicts
            break;
        }
    }

    // Clean up expired tracking entries
    std::erase_if(_buffExpirationTimes, [currentTime](const auto& pair) {
        return pair.second < currentTime;
    });
}

bool CombatSpecializationBase::HasBuff(uint32 spellId) const
{
    return _bot && _bot->HasAura(spellId);
}

void CombatSpecializationBase::ResetAllCooldowns()
{
    _cooldowns.clear();
    _globalCooldownEnd = 0;
}

bool CombatSpecializationBase::IsValidTarget(::Unit* target) const
{
    if (!target || !target->IsAlive())
        return false;

    if (!_bot->IsValidAttackTarget(target))
        return false;

    if (target->HasUnitFlag(UNIT_FLAG_NOT_SELECTABLE))
        return false;

    return true;
}

float CombatSpecializationBase::CalculateThreatLevel(::Unit* target) const
{
    if (!target)
        return 0.0f;

    auto it = _threatTable.find(target->GetGUID().GetRawValue());
    if (it != _threatTable.end())
        return it->second;

    return 0.0f;
}

void CombatSpecializationBase::UpdateThreatTable()
{
    // Clean old threat entries
    uint32 currentTime = GameTime::GetGameTimeMS();
    if (currentTime - _lastThreatUpdate < 1000)
        return;

    _lastThreatUpdate = currentTime;

    // Decay threat over time
    for (auto& [guid, threat] : _threatTable)
    {
        threat *= 0.95f; // 5% decay per second
    if (threat < 1.0f)

            threat = 0.0f;
    }

    // Remove zero threat entries
    std::erase_if(_threatTable, [](const auto& pair) {
        return pair.second == 0.0f;
    });
}

void CombatSpecializationBase::UseDefensiveCooldowns()
{
    // This would be overridden by specific classes
    // Base implementation could use items/racials
}

void CombatSpecializationBase::UseOffensiveCooldowns()
{
    // This would be overridden by specific classes
    // Base implementation could use items/racials
}

bool CombatSpecializationBase::ShouldUsePotions() const
{
    return _bot->GetHealthPct() < 40.0f || GetResourcePercent() < 20.0f;
}

void CombatSpecializationBase::UsePotions()
{
    // Implementation would check inventory for potions and use them
    // This is a placeholder for derived class implementation
}

bool CombatSpecializationBase::IsInEmergencyState() const
{
    return _bot->GetHealthPct() < EMERGENCY_HEALTH_PCT;
}

bool CombatSpecializationBase::HasGlobalCooldown() const
{
    return _globalCooldownEnd > GameTime::GetGameTimeMS();
}

bool CombatSpecializationBase::ShouldUseAoE() const
{
    return GetEnemiesInRange(10.0f) >= 3;
}

uint32 CombatSpecializationBase::GetEnemiesInRange(float range) const
{
    return GetNearbyEnemies(range).size();
}

bool CombatSpecializationBase::IsBehindTarget(::Unit* target) const
{
    if (!target)
        return false;

    return !target->HasInArc(static_cast<float>(M_PI), _bot);
}

bool CombatSpecializationBase::IsMoving() const
{
    return _bot && _bot->isMoving();
}

void CombatSpecializationBase::UpdatePositioning(::Unit* target)
{    if (!target || !_bot->IsAlive())
        return;

    uint32 currentTime = GameTime::GetGameTimeMS();

    // Throttle position updates
    if (currentTime - _lastPositionUpdate < 250) // 250ms minimum between updates
        return;

    _lastPositionUpdate = currentTime;

    if (!IsInOptimalPosition(target))
    {
        Position optimalPos = GetOptimalPosition(target);

        // PHASE 4 MIGRATION: Use Movement Arbiter with role-based priorities
        // All roles use ROLE_POSITIONING (170) - tank/healer/dps positioning priority
        // This is HIGH tier (150-199) which maps to MOTION_PRIORITY_NORMAL + MOTION_MODE_OVERRIDE
        PlayerBotMovementPriority priority = PlayerBotMovementPriority::ROLE_POSITIONING;  // Priority 170

        BotAI* botAI = dynamic_cast<BotAI*>(_bot->GetAI());
        if (botAI && botAI->GetUnifiedMovementCoordinator())
        {

            bool accepted = botAI->RequestPointMovement(
            priority,

                optimalPos,

                "Combat positioning - maintaining optimal range",

                "CombatSpecializationBase");


            if (accepted)

            {

                _metrics.positioningUpdates++;

                TC_LOG_DEBUG("playerbot.movement.arbiter",

                    "CombatSpecializationBase: Bot {} positioning requested (Role: {}, Priority: 170)",

                    _bot->GetName(), static_cast<uint32>(_role));

            }

            else

            {
                // Arbiter rejected - higher priority movement is active

                TC_LOG_TRACE("playerbot.movement.arbiter",

                    "CombatSpecializationBase: Positioning rejected for bot {} - higher priority active",

                    _bot->GetName());

            }
        }
        else
        {
            // FALLBACK: Direct MotionMaster call if arbiter not available

            TC_LOG_TRACE("playerbot.movement.arbiter",
                "CombatSpecializationBase: Movement Arbiter not available for bot {} - using fallback pathfinding",
                _bot->GetName());

            if (BotAI* ai = GetBotAI(_bot))
            {
                if (!ai->MoveTo(optimalPos, true))
                {
                    // Final fallback to legacy if validation fails
                    _bot->GetMotionMaster()->MovePoint(0, optimalPos);
                }
            }
            else
            {
                // Non-bot player - use standard movement
                _bot->GetMotionMaster()->MovePoint(0, optimalPos);
            }

            _metrics.positioningUpdates++;
        }
    }
}

bool CombatSpecializationBase::IsInOptimalPosition(::Unit* target) const
{
    if (!target)
        return false;

    float currentDistance = GetDistance(target);
    float optimalDistance = GetOptimalRange(target);

    // Allow 2 yard tolerance
    return std::abs(currentDistance - optimalDistance) <= 2.0f;
}

bool CombatSpecializationBase::ShouldReposition(::Unit* target) const
{
    return !IsInOptimalPosition(target);
}

Player* CombatSpecializationBase::GetGroupHealer() const
{
    // Similar to GetGroupTank but for healers
    if (_cachedHealer && GameTime::GetGameTimeMS() - _lastGroupUpdate < 5000)
        return _cachedHealer;

    // Find healer in group
    if (!IsInGroup())
        return nullptr;

    Group* group = _bot->GetGroup();
    for (GroupReference* itr : *group)
    {
        Player* member = itr->GetSource();
        if (!member || member == _bot)

            continue;

        // Simple healer detection based on class
    if (member->GetClass() == CLASS_PRIEST ||
        member->GetClass() == CLASS_DRUID ||
        member->GetClass() == CLASS_SHAMAN ||
        member->GetClass() == CLASS_PALADIN)
        {

            const_cast<CombatSpecializationBase*>(this)->_cachedHealer = member;

            return member;
        }
    }

    return nullptr;
}

} // namespace Playerbot