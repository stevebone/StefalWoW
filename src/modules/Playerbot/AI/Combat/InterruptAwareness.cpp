/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 */

// Combat/ThreatManager.h removed - not used in this file
#include "InterruptAwareness.h"
#include <unordered_set>
#include "Player.h"
#include "Unit.h"
#include "Spell.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "World.h"
#include "Creature.h"
#include "CellImpl.h"
#include "GridNotifiersImpl.h"
#include "../BotAI.h"
#include "InterruptCoordinator.h"
#include "../../Spatial/SpatialGridManager.h"  // Lock-free spatial grid for deadlock fix
#include <algorithm>
#include <thread>
#include <chrono>

namespace Playerbot
{

InterruptAwareness::InterruptAwareness(Player* observer)
    : _observer(observer)
    , _lastUpdate(::std::chrono::steady_clock::now())
{
    // Set default configuration
    _config.maxDetectionRange = 40.0f;
    _config.detectionIntervalMs = 100;
    _config.detectFriendlySpells = false;
    _config.detectChanneledSpells = true;
    _config.detectInstantCasts = false;
    _config.minCastTime = 500;
    _config.requireLineOfSight = true;

    TC_LOG_DEBUG("playerbot", "InterruptAwareness: Initialized for observer %s",
                 observer ? observer->GetName().c_str() : "nullptr");
}

SpellScanResult InterruptAwareness::Update(uint32 /*diff*/)
{
    SpellScanResult result;

    if (!_active.load() || !_observer)
        return result;

    auto updateStart = ::std::chrono::steady_clock::now();

    // Check if it's time for a scan
    auto timeSinceLastUpdate = ::std::chrono::duration_cast<::std::chrono::milliseconds>(updateStart - _lastUpdate);
    if (timeSinceLastUpdate.count() < _config.detectionIntervalMs)
        return result;

    // Skip scan if performance optimization suggests it
    if (ShouldSkipScan())
        return result;

    // Update active casts (remove expired ones)
    UpdateActiveCasts();

    // Perform the main scan
    result = ScanNearbyUnits();

    // Process completed casts
    ProcessCompletedCasts(result);

    // Update performance metrics
    auto scanTime = ::std::chrono::duration_cast<::std::chrono::microseconds>(
        ::std::chrono::steady_clock::now() - updateStart);

    {
        ::std::lock_guard lock(_metricsMutex);
        _metrics.totalScans.fetch_add(1);
        _metrics.unitsScanned.fetch_add(result.totalUnitsScanned);
        _metrics.spellsDetected.fetch_add(result.totalSpellsDetected);

        // Update timing metrics
        result.scanTime = scanTime;
        if (scanTime > _metrics.maxScanTime)
            _metrics.maxScanTime = scanTime;

        // Rolling average calculation
        auto currentAvg = _metrics.averageScanTime;
        _metrics.averageScanTime = ::std::chrono::microseconds(
            (currentAvg.count() * 9 + scanTime.count()) / 10
        );
    }

    _lastUpdate = updateStart;
    _lastScanTimeMs = static_cast<uint32>(scanTime.count() / 1000);
    _scanCount++;

    // Periodic optimization
    if (_scanCount % SCAN_OPTIMIZATION_INTERVAL == 0)
    {
        OptimizeForPerformance();
    }

    return result;
}

void InterruptAwareness::SetObserver(Player* observer)
{
    ::std::lock_guard lock(_observerMutex);
    _observer = observer;
    TC_LOG_DEBUG("playerbot", "InterruptAwareness: Observer set to %s",
                 observer ? observer->GetName().c_str() : "nullptr");
}

::std::vector<DetectedSpellCast> InterruptAwareness::GetActiveCasts() const
{
    ::std::lock_guard lock(_castMutex);
    ::std::vector<DetectedSpellCast> allCasts;

    for (const auto& [casterGuid, casts] : _activeCasts)
    {
        for (const auto& cast : casts)
        {
            if (cast.IsValid())
                allCasts.push_back(cast);
        }
    }

    return allCasts;
}

::std::vector<DetectedSpellCast> InterruptAwareness::GetCastsFromUnit(ObjectGuid casterGuid) const
{
    ::std::lock_guard lock(_castMutex);
    auto it = _activeCasts.find(casterGuid);
    if (it != _activeCasts.end())
    {
        ::std::vector<DetectedSpellCast> validCasts;
        for (const auto& cast : it->second)
        {
            if (cast.IsValid())
                validCasts.push_back(cast);
        }
        return validCasts;
    }
    return {};
}

::std::vector<DetectedSpellCast> InterruptAwareness::GetInterruptibleCasts(float maxRange) const
{
    ::std::lock_guard lock(_castMutex);
    ::std::vector<DetectedSpellCast> interruptibleCasts;

    for (const auto& [casterGuid, casts] : _activeCasts)
    {
        for (const auto& cast : casts)
        {
            if (cast.IsValid() && cast.isInterruptible)
            {
                // Check range if specified
    if (maxRange > 0.0f && cast.detectionRange > maxRange)
                    continue;

                interruptibleCasts.push_back(cast);
            }
        }
    }

    // Sort by remaining cast time (most urgent first)
    ::std::sort(interruptibleCasts.begin(), interruptibleCasts.end(),
              [](const DetectedSpellCast& a, const DetectedSpellCast& b) {
                  return a.GetRemainingTime() < b.GetRemainingTime();
              });

    return interruptibleCasts;
}

::std::vector<DetectedSpellCast> InterruptAwareness::GetHostileCasts() const
{
    ::std::lock_guard lock(_castMutex);
    ::std::vector<DetectedSpellCast> hostileCasts;

    for (const auto& [casterGuid, casts] : _activeCasts)
    {
        for (const auto& cast : casts)
        {
            if (cast.IsValid() && cast.isHostile)
            {
                hostileCasts.push_back(cast);
            }
        }
    }

    return hostileCasts;
}

bool InterruptAwareness::IsUnitCasting(ObjectGuid unitGuid) const
{
    ::std::lock_guard lock(_castMutex);
    auto it = _activeCasts.find(unitGuid);
    if (it != _activeCasts.end())
    {
        for (const auto& cast : it->second)
        {
            if (cast.IsValid())
                return true;
        }
    }
    return false;
}

DetectedSpellCast const* InterruptAwareness::GetSpellCast(ObjectGuid casterGuid, uint32 spellId) const
{
    ::std::lock_guard lock(_castMutex);
    auto it = _activeCasts.find(casterGuid);
    if (it != _activeCasts.end())
    {
        for (const auto& cast : it->second)
        {
            if (cast.IsValid() && cast.spellId == spellId)
                return &cast;
        }
    }
    return nullptr;
}

void InterruptAwareness::SetInterruptCoordinator(::std::shared_ptr<InterruptCoordinator> coordinator)
{
    _coordinator = coordinator;
    TC_LOG_DEBUG("playerbot", "InterruptAwareness: Interrupt coordinator set");
}

void InterruptAwareness::RegisterSpellCastCallback(::std::function<void(DetectedSpellCast const&)> callback)
{
    ::std::lock_guard lock(_callbackMutex);
    _spellCastCallbacks.push_back(callback);
}

void InterruptAwareness::RegisterSpellCompleteCallback(::std::function<void(ObjectGuid, uint32, bool)> callback)
{
    ::std::lock_guard lock(_callbackMutex);
    _spellCompleteCallbacks.push_back(callback);
}

size_t InterruptAwareness::GetActiveCastCount() const
{
    ::std::lock_guard lock(_castMutex);
    size_t count = 0;
    for (const auto& [casterGuid, casts] : _activeCasts)
    {
        for (const auto& cast : casts)
        {
            if (cast.IsValid())
                count++;
        }
    }
    return count;
}

void InterruptAwareness::ResetMetrics()
{
    ::std::lock_guard lock(_metricsMutex);
    _metrics.totalScans.store(0);
    _metrics.unitsScanned.store(0);
    _metrics.spellsDetected.store(0);
    _metrics.interruptibleSpells.store(0);
    _metrics.hostileSpells.store(0);
    _metrics.averageScanTime = ::std::chrono::microseconds{0};
    _metrics.maxScanTime = ::std::chrono::microseconds{0};
}

void InterruptAwareness::AddSpellPattern(uint32 npcId, ::std::vector<uint32> const& spellSequence)
{
    if (spellSequence.empty())
        return;

    ::std::lock_guard lock(_patternMutex);
    SpellPattern pattern;
    pattern.npcId = npcId;
    pattern.spellSequence = spellSequence;
    pattern.lastMatch = ::std::chrono::steady_clock::now();
    pattern.currentIndex = 0;

    _spellPatterns[npcId] = pattern;

    TC_LOG_DEBUG("playerbot", "InterruptAwareness: Added spell pattern for NPC %u with %u spells",
                 npcId, static_cast<uint32>(spellSequence.size()));
}

void InterruptAwareness::ClearSpellPatterns()
{
    ::std::lock_guard lock(_patternMutex);
    _spellPatterns.clear();
}

void InterruptAwareness::SetScanPriority(CreatureType creatureType, uint32 priority)
{
    ::std::lock_guard lock(_priorityMutex);
    _creatureTypePriorities[creatureType] = priority;
}

void InterruptAwareness::SetScanPriority(uint32 npcId, uint32 priority)
{
    ::std::lock_guard lock(_priorityMutex);
    _npcPriorities[npcId] = priority;
}

::std::vector<uint32> InterruptAwareness::PredictUpcomingSpells(ObjectGuid casterGuid) const
{
    if (!_enablePatterns)
        return {};

    // This would implement predictive spell detection based on known patterns
    // For now, return empty vector
    return {};
}

SpellScanResult InterruptAwareness::ScanNearbyUnits()
{
    SpellScanResult result;

    if (!_observer)
        return result;

    // Get nearby units to scan
    ::std::vector<Unit*> units = GetNearbyUnits();
    result.totalUnitsScanned = static_cast<uint32>(units.size());

    // Limit scanning to prevent performance issues
    if (units.size() > MAX_SCAN_UNITS)
    {
        // Sort by priority and distance, then take the top ones
        ::std::sort(units.begin(), units.end(),
                 [this](Unit* a, Unit* b) {
                     uint32 priorityA = GetUnitScanPriority(a);
                     uint32 priorityB = GetUnitScanPriority(b);
                     if (priorityA != priorityB)
                         return priorityA > priorityB;

                     // If priorities are equal, prefer closer units (using squared distance for comparison)
                     float distSqA = _observer->GetExactDistSq(a);
                     float distSqB = _observer->GetExactDistSq(b);
                     return distSqA < distSqB;
                 });

        units.resize(MAX_SCAN_UNITS);
    }

    // Process each unit
    for (Unit* unit : units)
    {
        if (ShouldScanUnit(unit))
        {
            ProcessUnit(unit, result);
        }
    }

    return result;
}

void InterruptAwareness::ProcessUnit(Unit* unit, SpellScanResult& result)
{
    if (!unit)
        return;

    ObjectGuid unitGuid = unit->GetGUID();
    // Check for current spell casting
    if (unit->HasUnitState(UNIT_STATE_CASTING))
    {
        // Check generic spell
    if (Spell* currentSpell = unit->GetCurrentSpell(CURRENT_GENERIC_SPELL))
        {
            if (ShouldDetectSpell(unit, currentSpell))
            {
                DetectedSpellCast cast = AnalyzeSpellCast(unit, currentSpell);
                if (cast.IsValid())
                {
                    // Check if this is a new cast or an update
                    bool isNewCast = true;
                    {
                        ::std::lock_guard lock(_castMutex);
                        auto it = _activeCasts.find(unitGuid);
                        if (it != _activeCasts.end())
                        {
                            for (const auto& existingCast : it->second)
                            {
                                if (existingCast.spellId == cast.spellId)
                                {
                                    isNewCast = false;
                                    break;
                                }
                            }
                        }
                    }

                    if (isNewCast)
                    {
                        AddDetectedCast(cast);
                        result.newCasts.push_back(cast);
                        result.totalSpellsDetected++;

                        // Update metrics
    if (cast.isInterruptible)
                            _metrics.interruptibleSpells.fetch_add(1);
                        if (cast.isHostile)
                            _metrics.hostileSpells.fetch_add(1);

                        // Notify callbacks and coordinator
                        NotifySpellDetected(cast);
                    }
                }
            }
        }

        // Check channeled spell
    if (Spell* channeledSpell = unit->GetCurrentSpell(CURRENT_CHANNELED_SPELL))
        {
            if (_config.detectChanneledSpells && ShouldDetectSpell(unit, channeledSpell))
            {
                DetectedSpellCast cast = AnalyzeSpellCast(unit, channeledSpell);
                if (cast.IsValid())
                {
                    cast.isChanneled = true;

                    // Similar new cast check as above
                    bool isNewCast = true;
                    {
                        ::std::lock_guard lock(_castMutex);
                        auto it = _activeCasts.find(unitGuid);
                        if (it != _activeCasts.end())
                        {
                            for (const auto& existingCast : it->second)
                            {
                                if (existingCast.spellId == cast.spellId && existingCast.isChanneled)
                                {
                                    isNewCast = false;
                                    break;
                                }
                            }
                        }
                    }

                    if (isNewCast)
                    {
                        AddDetectedCast(cast);
                        result.newCasts.push_back(cast);
                        result.totalSpellsDetected++;

                        NotifySpellDetected(cast);
                    }
                }
            }
        }
    }

    // Update pattern recognition if enabled
    if (_enablePatterns && !result.newCasts.empty())
    {
        for (const auto& cast : result.newCasts)
        {
            if (cast.casterGuid == unitGuid)
            {
                UpdateSpellPatterns(cast);
            }
        }
    }
}

DetectedSpellCast InterruptAwareness::AnalyzeSpellCast(Unit* caster, Spell const* spell) const
{
    DetectedSpellCast cast;

    if (!caster || !spell)
        return cast;

    const SpellInfo* spellInfo = spell->GetSpellInfo();
    if (!spellInfo)
        return cast;

    cast.casterGuid = caster->GetGUID();
    cast.spellId = spellInfo->Id;
    cast.detectionTime = ::std::chrono::steady_clock::now();
    cast.casterPosition = Position(caster->GetPositionX(), caster->GetPositionY(), caster->GetPositionZ());
    cast.isChanneled = spellInfo->IsChanneled();
    cast.isInterruptible = IsSpellInterruptible(spell);
    cast.isHostile = IsSpellHostile(caster, spell);
    cast.schoolMask = spellInfo->SchoolMask;

    if (_observer)
        cast.detectionRange = ::std::sqrt(_observer->GetExactDistSq(caster)); // Calculate once from squared distance

    // Calculate cast timing
    if (cast.isChanneled)
    {
        cast.castTimeMs = spellInfo->GetDuration();
    }
    else
    {
        cast.castTimeMs = spellInfo->CastTimeEntry->Base;
    }

    // Calculate remaining time based on spell progress
    uint32 elapsedTime = spell->GetTimer(); // Time since cast started
    if (elapsedTime < cast.castTimeMs)
    {
        cast.remainingTimeMs = cast.castTimeMs - elapsedTime;
        cast.estimatedCastEnd = cast.detectionTime + ::std::chrono::milliseconds(cast.remainingTimeMs);
    }
    else
    {
        // Spell is nearly complete or we detected it very late
        cast.remainingTimeMs = 100; // Give a small buffer
        cast.estimatedCastEnd = cast.detectionTime + ::std::chrono::milliseconds(100);
    }

    return cast;
}

bool InterruptAwareness::ShouldDetectSpell(Unit* caster, Spell const* spell) const
{
    if (!caster || !spell)
        return false;

    const SpellInfo* spellInfo = spell->GetSpellInfo();
    if (!spellInfo)
        return false;

    // Check cast time minimum
    if (spellInfo->CastTimeEntry->Base < static_cast<int32>(_config.minCastTime) && !spellInfo->IsChanneled())
        return false;

    // Check if we should detect instant casts
    if (!_config.detectInstantCasts && spellInfo->CastTimeEntry->Base == 0 && !spellInfo->IsChanneled())
        return false;

    // Check range
    if (_observer)
    {
        float maxRangeSq = _config.maxDetectionRange * _config.maxDetectionRange;
        if (_observer->GetExactDistSq(caster) > maxRangeSq)
            return false;

        // Check line of sight if required
    if (_config.requireLineOfSight && !_observer->IsWithinLOSInMap(caster))
            return false;
    }

    // Check friendly vs hostile
    if (!_config.detectFriendlySpells && _observer)
    {
        if (caster->IsFriendlyTo(_observer))
            return false;
    }

    return true;
}

bool InterruptAwareness::IsSpellInterruptible(Spell const* spell) const
{
    if (!spell)
        return false;

    const SpellInfo* spellInfo = spell->GetSpellInfo();
    if (!spellInfo)
        return false;

    // Check for uninterruptible attributes
    // Note: SPELL_ATTR0_UNAFFECTED_BY_INVULNERABILITY not available in this TrinityCore version
    if (spellInfo->HasAttribute(SPELL_ATTR4_CANNOT_BE_STOLEN))
        return false;

    // Most spells are interruptible by default
    return true;
}

bool InterruptAwareness::IsSpellHostile(Unit* caster, Spell const* spell) const
{
    if (!caster || !spell || !_observer)
        return false;

    // Check faction relationship
    if (caster->IsHostileTo(_observer))
        return true;

    // Check spell effects for hostile nature
    const SpellInfo* spellInfo = spell->GetSpellInfo();
    if (!spellInfo)
        return false;

    // Check for damage or harmful effects
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        switch (spellInfo->GetEffects()[i].Effect)
        {
            case SPELL_EFFECT_SCHOOL_DAMAGE:
            case SPELL_EFFECT_WEAPON_DAMAGE:
            case SPELL_EFFECT_WEAPON_DAMAGE_NOSCHOOL:
            case SPELL_EFFECT_NORMALIZED_WEAPON_DMG:
            case SPELL_EFFECT_WEAPON_PERCENT_DAMAGE:
                return true;

            case SPELL_EFFECT_APPLY_AURA:
                switch (spellInfo->GetEffects()[i].ApplyAuraName)
                {
                    case SPELL_AURA_MOD_FEAR:
                    case SPELL_AURA_MOD_STUN:
                    case SPELL_AURA_MOD_CHARM:
                    case SPELL_AURA_TRANSFORM:
                    case SPELL_AURA_MOD_SILENCE:
                    case SPELL_AURA_PERIODIC_DAMAGE:
                        return true;
                    default:
                        break;
                }
                break;

            default:
                break;
        }
    }

    return false;
}

void InterruptAwareness::UpdateActiveCasts()
{
    RemoveExpiredCasts();

    // Update remaining times for active casts
    ::std::lock_guard lock(_castMutex);
    for (auto& [casterGuid, casts] : _activeCasts)
    {
        for (auto& cast : casts)
        {
            if (cast.IsValid())
            {
                cast.remainingTimeMs = cast.GetRemainingTime();
            }
        }
    }
}

void InterruptAwareness::AddDetectedCast(DetectedSpellCast const& cast)
{
    ::std::lock_guard lock(_castMutex);

    // Limit total active casts to prevent memory issues
    if (GetActiveCastCount() >= MAX_ACTIVE_CASTS)
    {
        // Remove oldest casts
        auto now = ::std::chrono::steady_clock::now();
        for (auto& [casterGuid, casts] : _activeCasts)
        {
            casts.erase(
                ::std::remove_if(casts.begin(), casts.end(),
                              [now](const DetectedSpellCast& c) {
                                  auto age = ::std::chrono::duration_cast<::std::chrono::seconds>(now - c.detectionTime);
                                  return age.count() > 30; // Remove casts older than 30 seconds
                              }),
                casts.end()
            );
        }
    }

    _activeCasts[cast.casterGuid].push_back(cast);
}

void InterruptAwareness::RemoveExpiredCasts()
{
    ::std::lock_guard lock(_castMutex);

    for (auto it = _activeCasts.begin(); it != _activeCasts.end();)
    {
        auto& casts = it->second;

        // Remove expired casts
        casts.erase(
            ::std::remove_if(casts.begin(), casts.end(),
                          [](const DetectedSpellCast& cast) {
                              return cast.IsExpired();
                          }),
            casts.end()
        );

        // Remove empty entries
    if (casts.empty())
            it = _activeCasts.erase(it);
        else
            ++it;
    }
}

void InterruptAwareness::ProcessCompletedCasts(SpellScanResult& result)
{
    // This would check for spells that were interrupted or completed
    // Implementation would depend on how TrinityCore reports spell completion
    // For now, we rely on the expiration system
}
::std::vector<Unit*> InterruptAwareness::GetNearbyUnits() const
{
    ::std::vector<Unit*> units;
    if (!_observer)
        return units;

    // DEADLOCK FIX: Use lock-free spatial grid instead of Cell::VisitGridObjects
    Map* map = _observer->GetMap();
    if (!map)
        return units;

    DoubleBufferedSpatialGrid* spatialGrid = sSpatialGridManager.GetGrid(map);
    if (!spatialGrid)
    {
        // Grid not yet created for this map - create it on demand
        sSpatialGridManager.CreateGrid(map);
        spatialGrid = sSpatialGridManager.GetGrid(map);
        if (!spatialGrid)
            return units;
    }

    // Query nearby creature GUIDs (lock-free!)
    ::std::vector<ObjectGuid> nearbyGuids = spatialGrid->QueryNearbyCreatureGuids(
        _observer->GetPosition(), _config.maxDetectionRange);

    // Resolve GUIDs to Unit pointers and filter appropriate units
    for (ObjectGuid guid : nearbyGuids)
    {
        // SPATIAL GRID MIGRATION COMPLETE (2025-11-26):
        // ObjectAccessor is intentionally retained - Live Unit* needed for:
        // 1. GetCurrentSpell() requires live unit state for spell detection
        // 2. HasUnitState(UNIT_STATE_CASTING) verification needs real-time state
        // The spatial grid provides pre-filtering but cannot provide live casting state.
        ::Unit* unit = ObjectAccessor::GetUnit(*_observer, guid);
        if (!unit || unit == _observer)
            continue;

        // Filter for casting or combat units to reduce processing load
    if (unit->HasUnitState(UNIT_STATE_CASTING) || unit->IsInCombat())
        {
            units.push_back(unit);
        }
        // Also include hostile units that might cast soon
        else if (unit->IsHostileTo(_observer) && unit->IsAlive())
        {
            units.push_back(unit);
        }
    }

    // Fallback: Add current target if grid search didn't find it
    // SPATIAL GRID MIGRATION COMPLETE (2025-11-26):
    // ObjectAccessor is intentionally retained - Live Unit* needed for:
    // 1. _observer->GetTarget() returns ObjectGuid requiring live Unit* lookup
    // 2. GetExactDistSq() requires current position from live object
    // The spatial grid handles bulk queries, fallback handles edge case.
    if (Unit* target = ObjectAccessor::GetUnit(*_observer, _observer->GetTarget()))
    {
        float maxRangeSq = _config.maxDetectionRange * _config.maxDetectionRange;
        if (_observer->GetExactDistSq(target) <= maxRangeSq)
        {
            // Check if target is already in the list
            auto it = ::std::find(units.begin(), units.end(), target);
            if (it == units.end())
            {
                units.push_back(target);
            }
        }
    }

    return units;
}

bool InterruptAwareness::ShouldScanUnit(Unit* unit) const
{
    if (!unit || !unit->IsAlive())
        return false;

    // Don't scan the observer
    if (unit == _observer)
        return false;

    // Check if unit is in casting state or might cast soon
    if (!unit->HasUnitState(UNIT_STATE_CASTING) && !unit->IsInCombat())
        return false;

    return true;
}

uint32 InterruptAwareness::GetUnitScanPriority(Unit* unit) const
{
    if (!unit)
        return 0;
    uint32 priority = 100; // Base priority

    // Higher priority for creatures in combat
    if (unit->IsInCombat())
        priority += 50;

    // Higher priority for creatures that are casting
    if (unit->HasUnitState(UNIT_STATE_CASTING))
        priority += 100;

    // Full implementation: Check creature type and dangerous caster patterns
    if (Creature const* creature = unit->ToCreature())
    {
        // Boss/Elite status - high priority interrupts
        if (creature->isWorldBoss() || creature->IsDungeonBoss())
            priority += 200;  // Boss spells are critical to interrupt
        else if (creature->IsElite())
            priority += 75;   // Elite mobs are dangerous

        // Check creature template for caster type
        CreatureTemplate const* creatureTemplate = creature->GetCreatureTemplate();
        if (creatureTemplate)
        {
            // Unit class check - casters get higher priority
            if (creatureTemplate->unit_class == UNIT_CLASS_MAGE ||
                creatureTemplate->unit_class == UNIT_CLASS_PALADIN ||
                creatureTemplate->unit_class == UNIT_CLASS_WARRIOR)
            {
                // Mage-type NPCs: highest caster priority
                if (creatureTemplate->unit_class == UNIT_CLASS_MAGE)
                    priority += 50;
            }

            // Check for known dangerous NPC entries (raid bosses, dungeon healers)
            uint32 entry = creature->GetEntry();
            // TWW 12.0 dungeon/raid healer entries that should be priority interrupted
            static const std::unordered_set<uint32> healerEntries = {
                // Nerub-ar Palace healers
                196662, 196704, 196810,
                // M+ dungeon healers
                199193, 199198, 199234, 199312
            };
            if (healerEntries.count(entry) > 0)
                priority += 150;  // Healers are critical interrupt targets
        }

        // Check current spell being cast for dangerous spell patterns
        if (Spell const* currentSpell = creature->GetCurrentSpell(CURRENT_GENERIC_SPELL))
        {
            SpellInfo const* spellInfo = currentSpell->GetSpellInfo();
            if (spellInfo)
            {
                // Check for healing spells
                if (spellInfo->HasEffect(SPELL_EFFECT_HEAL) ||
                    spellInfo->HasEffect(SPELL_EFFECT_HEAL_PCT) ||
                    spellInfo->HasAura(SPELL_AURA_PERIODIC_HEAL))
                {
                    priority += 125;  // Heal spells are high priority
                }

                // Check for crowd control spells
                if (spellInfo->HasAura(SPELL_AURA_MOD_STUN) ||
                    spellInfo->HasAura(SPELL_AURA_MOD_FEAR) ||
                    spellInfo->HasAura(SPELL_AURA_MOD_CHARM) ||
                    spellInfo->HasAura(SPELL_AURA_MOD_CONFUSE))
                {
                    priority += 100;  // CC spells are dangerous
                }

                // Check for damage reduction/immunity spells
                if (spellInfo->HasAura(SPELL_AURA_SCHOOL_IMMUNITY) ||
                    spellInfo->HasAura(SPELL_AURA_DAMAGE_IMMUNITY))
                {
                    priority += 75;  // Immunity spells should be interrupted
                }

                // Long cast time = more important to interrupt
                uint32 castTime = spellInfo->CalcCastTime();
                if (castTime > 2500)  // > 2.5s cast
                    priority += 30;
                else if (castTime > 1500)  // > 1.5s cast
                    priority += 15;
            }
        }
    }

    // Apply role-based multiplier if observer is a tank (tanks need to interrupt more)
    // Check if observer has tank specialization via talent tree analysis
    if (_observer)
    {
        // Get player's current specialization to check for tank role
        ChrSpecialization specId = _observer->GetPrimarySpecialization();
        // Tank spec IDs in TWW 12.0: Protection Warrior (73), Protection Paladin (66),
        // Blood DK (250), Guardian Druid (104), Brewmaster Monk (268), Vengeance DH (581)
        static const std::unordered_set<ChrSpecialization> tankSpecs = {
            ChrSpecialization(73), ChrSpecialization(66), ChrSpecialization(250),
            ChrSpecialization(104), ChrSpecialization(268), ChrSpecialization(581)
        };
        if (tankSpecs.count(specId) > 0)
            priority = static_cast<int32>(priority * 1.2f);
    }

    return priority;
}

void InterruptAwareness::UpdateSpellPatterns(DetectedSpellCast const& cast)
{
    if (!_enablePatterns)
        return;

    // Full implementation: Track and analyze spell patterns for prediction
    uint64 casterKey = cast.casterGuid.IsCreature() ? cast.casterGuid.GetEntry() : cast.casterGuid.GetCounter();

    // Get or create pattern for this caster
    auto& pattern = _spellPatterns[casterKey];

    // Update pattern with new spell
    if (pattern.spellSequence.empty())
    {
        // First spell from this caster - initialize pattern
        pattern.spellSequence.push_back(cast.spellId);
        pattern.currentIndex = 0;
        pattern.lastUpdateTime = getMSTime();
        pattern.confidenceScore = 0.5f;  // Start with moderate confidence
    }
    else
    {
        uint32 timeSinceLastCast = getMSTime() - pattern.lastUpdateTime;

        // Check if this matches expected next spell in sequence
        size_t nextIndex = (pattern.currentIndex + 1) % pattern.spellSequence.size();
        bool matchesPattern = false;

        if (nextIndex < pattern.spellSequence.size() &&
            pattern.spellSequence[nextIndex] == cast.spellId)
        {
            // Spell matches expected pattern - increase confidence
            matchesPattern = true;
            pattern.currentIndex = nextIndex;
            pattern.confidenceScore = std::min(1.0f, pattern.confidenceScore + 0.1f);

            TC_LOG_DEBUG("playerbot", "InterruptAwareness: Pattern match for caster %llu - spell %u at index %zu (confidence: %.2f)",
                casterKey, cast.spellId, nextIndex, pattern.confidenceScore);
        }
        else
        {
            // Spell doesn't match - either new pattern or variation
            // Check if we've seen this spell before in the sequence
            bool foundInSequence = false;
            for (size_t i = 0; i < pattern.spellSequence.size(); ++i)
            {
                if (pattern.spellSequence[i] == cast.spellId)
                {
                    foundInSequence = true;
                    pattern.currentIndex = i;
                    break;
                }
            }

            if (!foundInSequence)
            {
                // New spell - extend pattern if sequence is short enough
                if (pattern.spellSequence.size() < 10)  // Max pattern length
                {
                    pattern.spellSequence.push_back(cast.spellId);
                    pattern.currentIndex = pattern.spellSequence.size() - 1;
                }
                else
                {
                    // Pattern too long - might be random, reduce confidence
                    pattern.confidenceScore = std::max(0.1f, pattern.confidenceScore - 0.2f);
                }
            }
            else
            {
                // Spell found but out of order - minor confidence reduction
                pattern.confidenceScore = std::max(0.2f, pattern.confidenceScore - 0.05f);
            }
        }

        // Record timing for temporal analysis
        if (pattern.castTimings.size() < 20)
            pattern.castTimings.push_back(timeSinceLastCast);

        pattern.lastUpdateTime = getMSTime();
    }

    TC_LOG_DEBUG("playerbot", "InterruptAwareness: Updated pattern for caster %llu - "
        "sequence length %zu, current index %zu, confidence %.2f",
        casterKey, pattern.spellSequence.size(), pattern.currentIndex, pattern.confidenceScore);
}

bool InterruptAwareness::MatchesKnownPattern(ObjectGuid casterGuid, uint32 spellId) const
{
    if (!_enablePatterns)
        return false;

    // Full implementation: Check if spell matches known dangerous patterns
    uint64 casterKey = casterGuid.IsCreature() ? casterGuid.GetEntry() : casterGuid.GetCounter();

    auto it = _spellPatterns.find(casterKey);
    if (it == _spellPatterns.end())
        return false;

    SpellPattern const& pattern = it->second;

    // Only consider high-confidence patterns
    if (pattern.confidenceScore < 0.6f)
        return false;

    // Check if this spell is in the pattern and is a dangerous spell
    for (uint32 patternSpellId : pattern.spellSequence)
    {
        if (patternSpellId == spellId)
        {
            // Verify this is a dangerous spell worth tracking
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
            if (spellInfo)
            {
                // Consider dangerous if it's a heal, CC, or high damage spell
                if (spellInfo->HasEffect(SPELL_EFFECT_HEAL) ||
                    spellInfo->HasEffect(SPELL_EFFECT_HEAL_PCT) ||
                    spellInfo->HasAura(SPELL_AURA_MOD_STUN) ||
                    spellInfo->HasAura(SPELL_AURA_MOD_FEAR) ||
                    spellInfo->HasAura(SPELL_AURA_PERIODIC_DAMAGE) ||
                    spellInfo->HasAura(SPELL_AURA_SCHOOL_ABSORB))
                {
                    return true;
                }

                // Also check if cast time is long (worth interrupting)
                if (spellInfo->CalcCastTime() > 2000)
                    return true;
            }
        }
    }

    return false;
}

void InterruptAwareness::OptimizeForPerformance()
{
    // Clean up old data, optimize data structures
    RemoveExpiredCasts();

    // Reset metrics if they get too large
    {
        ::std::lock_guard lock(_metricsMutex);
        if (_metrics.totalScans.load() > 100000)
        {
            // Scale down metrics to prevent overflow
            _metrics.totalScans.store(_metrics.totalScans.load() / 2);
            _metrics.unitsScanned.store(_metrics.unitsScanned.load() / 2);
            _metrics.spellsDetected.store(_metrics.spellsDetected.load() / 2);
            _metrics.interruptibleSpells.store(_metrics.interruptibleSpells.load() / 2);
            _metrics.hostileSpells.store(_metrics.hostileSpells.load() / 2);
        }
    }

    TC_LOG_DEBUG("playerbot", "InterruptAwareness: Performance optimization complete - %u active casts, %u scans performed",
                 static_cast<uint32>(GetActiveCastCount()), _scanCount);
}

bool InterruptAwareness::ShouldSkipScan() const
{
    // Skip scan if we're not in combat and have no active casts
    if (!_observer || !_observer->IsInCombat())
    {
        if (GetActiveCastCount() == 0)
            return true;
    }

    // Skip scan if performance is degraded
    if (_lastScanTimeMs > 10) // If last scan took more than 10ms
        return true;

    return false;
}

void InterruptAwareness::NotifySpellDetected(DetectedSpellCast const& cast)
{
    // Notify interrupt coordinator
    if (auto coordinator = _coordinator.lock())
    {
        // Notify coordinator of the cast with cast time
        // SPATIAL GRID MIGRATION COMPLETE (2025-11-26):
        // ObjectAccessor is intentionally retained - Live Unit* needed for:
        // 1. OnEnemyCastStart(Unit*, ...) requires live Unit* for method signature
        // 2. Interrupt execution requires live target reference
        // The spatial grid provides detection, coordinator needs live objects.
        Unit* caster = ObjectAccessor::GetUnit(*_observer, cast.casterGuid);
        if (caster)
        {
            coordinator->OnEnemyCastStart(caster, cast.spellId, cast.castTimeMs);
        }
    }

    // Notify registered callbacks
    ::std::lock_guard lock(_callbackMutex);
    for (const auto& callback : _spellCastCallbacks)
    {
        try
        {
            callback(cast);
        }
        catch (...)
        {
            TC_LOG_ERROR("playerbot", "InterruptAwareness: Exception in spell cast callback");
        }
    }
}

void InterruptAwareness::NotifySpellCompleted(ObjectGuid casterGuid, uint32 spellId, bool interrupted)
{
    // Notify interrupt coordinator
    if (auto coordinator = _coordinator.lock())
    {
        if (interrupted)
            coordinator->OnEnemyCastInterrupted(casterGuid, spellId);
        else
            coordinator->OnEnemyCastComplete(casterGuid, spellId);
    }

    // Notify registered callbacks
    ::std::lock_guard lock(_callbackMutex);
    for (const auto& callback : _spellCompleteCallbacks)
    {
        try
        {
            callback(casterGuid, spellId, interrupted);
        }
        catch (...)
        {
            TC_LOG_ERROR("playerbot", "InterruptAwareness: Exception in spell complete callback");
        }
    }
}

} // namespace Playerbot