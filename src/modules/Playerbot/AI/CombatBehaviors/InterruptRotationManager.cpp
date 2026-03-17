/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "InterruptRotationManager.h"
#include "GameTime.h"
#include "BotAI.h"
#include "Player.h"
#include "Unit.h"
#include "Spell.h"
#include "SpellMgr.h"
#include "ObjectAccessor.h"
#include "World.h"
#include "Log.h"
#include "SpellHistory.h"
#include "Timer.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "../../Spatial/SpatialGridQueryHelpers.h" // Thread-safe spatial grid queries
#include "../../Packets/SpellPacketBuilder.h"  // PHASE 0 WEEK 3: Packet-based spell casting
#include <algorithm>

namespace Playerbot
{

// Static member definitions moved to header with inline to fix DLL linkage (C2491)

// Interrupt spell IDs for all classes
enum InterruptSpells : uint32
{
    SPELL_KICK              = 1766,    // Rogue
    SPELL_PUMMEL            = 6552,    // Warrior
    SPELL_WIND_SHEAR        = 57994,   // Shaman
    SPELL_COUNTERSPELL      = 2139,    // Mage
    SPELL_SPELL_LOCK        = 19647,   // Warlock
    SPELL_MIND_FREEZE       = 47528,   // Death Knight
    SPELL_REBUKE            = 96231,   // Paladin
    SPELL_SKULL_BASH        = 106839,  // Druid
    SPELL_SPEAR_HAND_STRIKE = 116705,  // Monk
    SPELL_COUNTER_SHOT      = 147362,  // Hunter
    SPELL_SILENCE           = 15487,   // Priest
    SPELL_DISRUPT           = 183752,  // Demon Hunter
    SPELL_SOLAR_BEAM        = 78675    // Druid (area silence)
};

// Critical spells to interrupt
enum CriticalSpells : uint32
{
    // Heals (MANDATORY)
    SPELL_FLASH_HEAL        = 2061,
    SPELL_GREATER_HEAL      = 2060,
    SPELL_HOLY_LIGHT        = 635,
    SPELL_FLASH_OF_LIGHT    = 19750,
    SPELL_REGROWTH          = 8936,
    SPELL_HEALING_TOUCH     = 5185,
    SPELL_CHAIN_HEAL        = 1064,
    SPELL_HEALING_WAVE      = 331,
    SPELL_LESSER_HEALING    = 2050,

    // Crowd Control (MANDATORY)
    SPELL_POLYMORPH         = 118,
    SPELL_FEAR              = 5782,
    SPELL_PSYCHIC_SCREAM    = 8122,
    SPELL_MIND_CONTROL      = 605,
    SPELL_HEX               = 51514,
    SPELL_CYCLONE           = 33786,
    SPELL_ENTANGLING_ROOTS  = 339,
    SPELL_HIBERNATE         = 2637,
    SPELL_BANISH            = 710,

    // High damage (HIGH PRIORITY)
    SPELL_PYROBLAST         = 11366,
    SPELL_CHAOS_BOLT        = 116858,
    SPELL_GREATER_PYROBLAST = 33938,
    SPELL_AIMED_SHOT        = 19434,
    SPELL_SOUL_FIRE         = 6353,
    SPELL_MIND_BLAST        = 8092,
    SPELL_STARSURGE         = 78674,

    // Standard damage (MEDIUM)
    SPELL_FROSTBOLT         = 116,
    SPELL_FIREBALL          = 133,
    SPELL_SHADOW_BOLT       = 686,
    SPELL_LIGHTNING_BOLT    = 403,
    SPELL_WRATH             = 5176,
    SPELL_STARFIRE          = 2912,
    SPELL_HOLY_FIRE         = 14914,

    // Channels
    SPELL_EVOCATION         = 12051,
    SPELL_ARCANE_MISSILES   = 5143,
    SPELL_DRAIN_LIFE        = 689,
    SPELL_DRAIN_SOUL        = 1120,
    SPELL_MIND_FLAY         = 15407,
    SPELL_TRANQUILITY       = 740,
    SPELL_DIVINE_HYMN       = 64843
};

InterruptRotationManager::InterruptRotationManager(BotAI* ai)
    : _ai(ai)
    , _bot(ai ? ai->GetBot() : nullptr)
    , _lastCleanupTime(0)
    , _lastUpdateTime(0)
    , _unitCacheTime(0)
{
    // Initialize global database if not done
    if (!s_databaseInitialized)
    {
        InitializeGlobalDatabase();
    }

    // Set default configuration
    _config.reactionTimeMs = 200;
    _config.coordinationDelayMs = 100;
    _config.interruptRangeBuffer = 2.0f;
    _config.preferMeleeInterrupts = true;
    _config.useRotation = true;
    _config.maxInterruptsPerMinute = 20;

    // Clear tracking structures
    _interrupters.clear();
    _activeCasts.clear();
    _delayedInterrupts.clear();
    _unitCache.clear();

    // Initialize rotation queue
    while (!_rotationQueue.empty())
    {
        _rotationQueue.pop();
    }
}

InterruptRotationManager::~InterruptRotationManager()
{
    Reset();
}

void InterruptRotationManager::Update(uint32 diff)
{
    uint32 currentTime = GameTime::GetGameTimeMS();

    // Process delayed interrupts
    ProcessDelayedInterrupts();

    // Update interrupter cooldowns
    for (auto& interrupter : _interrupters)
    {
        if (interrupter.cooldownRemaining > 0)
        {
            if (interrupter.cooldownRemaining > diff)
                interrupter.cooldownRemaining -= diff;
            else
                interrupter.cooldownRemaining = 0;
        }
    }

    // Cleanup expired casts every 500ms
    if (currentTime - _lastCleanupTime > 500)
    {
        CleanupExpiredData();
        _lastCleanupTime = currentTime;
    }

    // Clear unit cache if expired
    if (currentTime - _unitCacheTime > UNIT_CACHE_DURATION)
    {
        _unitCache.clear();
        _unitCacheTime = currentTime;
    }

    _lastUpdateTime = currentTime;
}

void InterruptRotationManager::RegisterCast(Unit* caster, uint32 spellId, uint32 castTime)
{
    if (!caster || !spellId)
        return;

    // Check if spell should be tracked
    auto it = s_interruptDatabase.find(spellId);
    if (it == s_interruptDatabase.end())
    {
        // Check spell info for dangerous characteristics
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
        if (!spellInfo)
            return;

        // Skip instant casts and non-interruptible spells
    if (!spellInfo->CastTimeEntry || spellInfo->HasAttribute(SPELL_ATTR7_NO_UI_NOT_INTERRUPTIBLE))
            return;
    }

    // Check if we're already tracking this cast
    if (IsTrackingCast(caster->GetGUID(), spellId))
        return;

    uint32 currentTime = GameTime::GetGameTimeMS();

    // Calculate cast end time
    if (castTime == 0 && it != s_interruptDatabase.end())
    {
        castTime = it->second.castTimeMs;
    }
    else if (castTime == 0)
    {
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
        if (spellInfo && spellInfo->CastTimeEntry)
        {
            castTime = spellInfo->CastTimeEntry->Base;
        }
    }

    // Create active cast tracking
    ActiveCast cast;
    cast.casterGuid = caster->GetGUID();
    cast.spellId = spellId;
    cast.castStartTime = currentTime;
    cast.castEndTime = currentTime + castTime;
    cast.priority = (it != s_interruptDatabase.end()) ? it->second.priority : InterruptPriority::PRIORITY_LOW;
    cast.interrupted = false;
    cast.fallbackTriggered = false;

    _activeCasts.push_back(cast);
    _statistics.totalCastsDetected++;

    // Update range status for all interrupters
    UpdateRangeStatus(caster);
}

bool InterruptRotationManager::IsTrackingCast(ObjectGuid caster, uint32 spellId) const
{
    for (const auto& cast : _activeCasts)
    {
        if (cast.casterGuid == caster)
        {
            if (spellId == 0 || cast.spellId == spellId)
                return true;
        }
    }
    return false;
}

ObjectGuid InterruptRotationManager::SelectInterrupter(Unit* caster, uint32 spellId)
{
    if (!caster || !spellId)
        return ObjectGuid::Empty;

    // Find the active cast
    ActiveCast* activeCast = nullptr;
    for (auto& cast : _activeCasts)
    {
        if (cast.casterGuid == caster->GetGUID() && cast.spellId == spellId)
        {
            activeCast = &cast;
            break;
        }
    }

    if (!activeCast)
        return ObjectGuid::Empty;

    // Check if already assigned
    if (!activeCast->assignedInterrupter.IsEmpty())
        return activeCast->assignedInterrupter;

    // Find best interrupter
    ObjectGuid bestInterrupter = FindBestInterrupter(*activeCast);

    if (!bestInterrupter.IsEmpty())
    {
        activeCast->assignedInterrupter = bestInterrupter;

        // Mark interrupter as assigned
    for (auto& interrupter : _interrupters)
        {
            if (interrupter.botGuid == bestInterrupter)
            {
                interrupter.isAssigned = true;
                break;
            }
        }
    }

    return bestInterrupter;
}

ObjectGuid InterruptRotationManager::FindBestInterrupter(const ActiveCast& cast) const
{
    if (_interrupters.empty())
        return ObjectGuid::Empty;

    ObjectGuid bestBot;
    float bestScore = -1.0f;

    for (const auto& interrupter : _interrupters)
    {
        // Skip if already assigned or on cooldown
    if (interrupter.isAssigned || interrupter.cooldownRemaining > 0)
            continue;

        // Calculate score
        float score = CalculateInterrupterScore(interrupter, cast);

        if (score > bestScore)
        {
            bestScore = score;
            bestBot = interrupter.botGuid;
        }
    }

    // If using rotation, prefer next in queue if score is acceptable
    if (_config.useRotation && bestScore > 50.0f && !_rotationQueue.empty())
    {
        ObjectGuid nextInRotation = GetNextInRotation();

        // Check if next in rotation has acceptable score
    for (const auto& interrupter : _interrupters)
        {
            if (interrupter.botGuid == nextInRotation)
            {
                float rotationScore = CalculateInterrupterScore(interrupter, cast);
                if (rotationScore > 40.0f) // Slightly lower threshold for rotation
                {
                    return nextInRotation;
                }
                break;
            }
        }
    }

    return bestBot;
}

float InterruptRotationManager::CalculateInterrupterScore(const InterrupterBot& interrupter, const ActiveCast& cast) const
{
    float score = 100.0f;

    // Range check (most important)
    if (!interrupter.isInRange)
    {
        score -= 50.0f;

        // Check if can reach in time
        uint32 timeRemaining = GetTimeToComplete(cast);

        // PHASE 5B: Thread-safe spatial grid validation (replaces ObjectAccessor::GetUnit)
        Unit* caster = nullptr;
        if (_unitCache.count(cast.casterGuid))
        {
            caster = _unitCache.at(cast.casterGuid);
        }
        else
        {
            auto snapshot = SpatialGridQueryHelpers::FindCreatureByGuid(_bot, cast.casterGuid);
            if (snapshot)
                caster = ObjectAccessor::GetUnit(*_bot, cast.casterGuid);
        }

        if (caster && !CanReachInTime(interrupter, caster, timeRemaining))
            return 0.0f; // Cannot interrupt in time
    }

    // Priority bonus
    switch (cast.priority)
    {
        case InterruptPriority::PRIORITY_MANDATORY:
            score += 50.0f;
            break;
        case InterruptPriority::PRIORITY_HIGH:
            score += 30.0f;
            break;
        case InterruptPriority::PRIORITY_MEDIUM:
            score += 15.0f;
            break;
        default:
            break;
    }

    // Cooldown availability
    if (interrupter.cooldownRemaining == 0)
    {
        score += 20.0f;
    }
    else
    {
        score -= (interrupter.cooldownRemaining / 1000.0f) * 10.0f;
    }

    // Interrupt count (spread the load)
    score -= interrupter.interruptsPerformed * 2.0f;

    // Prefer melee interrupts (no travel time)
    if (_config.preferMeleeInterrupts && interrupter.range <= 10)
    {
        score += 10.0f;
    }

    // Alternative interrupt availability
    if (!interrupter.alternativeInterrupts.empty())
    {
        score += 5.0f;
    }

    return ::std::max(0.0f, score);
}

ObjectGuid InterruptRotationManager::GetNextInRotation() const
{
    if (_rotationQueue.empty())
        return ObjectGuid::Empty;

    // Return front of queue without modifying
    return _rotationQueue.front();
}

void InterruptRotationManager::MarkInterruptUsed(ObjectGuid bot, uint32 timeMs)
{
    uint32 currentTime = timeMs ? timeMs : GameTime::GetGameTimeMS();

    for (auto& interrupter : _interrupters)
    {
        if (interrupter.botGuid == bot)
        {
            interrupter.lastInterruptTime = currentTime;
            interrupter.interruptsPerformed++;
            interrupter.isAssigned = false;

            // Set cooldown based on spell (typical interrupt CDs)
    if (interrupter.interruptSpellId == SPELL_KICK)
                interrupter.cooldownRemaining = 15000; // 15 seconds
            else if (interrupter.interruptSpellId == SPELL_COUNTERSPELL)
                interrupter.cooldownRemaining = 24000; // 24 seconds
            else if (interrupter.interruptSpellId == SPELL_PUMMEL)
                interrupter.cooldownRemaining = 10000; // 10 seconds
            else if (interrupter.interruptSpellId == SPELL_WIND_SHEAR)
                interrupter.cooldownRemaining = 12000; // 12 seconds
            else
                interrupter.cooldownRemaining = 15000; // Default 15 seconds

            break;
        }
    }

    // Update rotation queue
    if (_config.useRotation && !_rotationQueue.empty())
    {
        if (_rotationQueue.front() == bot)
        {
            _rotationQueue.pop();
            _rotationQueue.push(bot); // Move to back
        }
    }

    // Update statistics
    _statistics.interruptsByBot[bot]++;
}

void InterruptRotationManager::RegisterInterrupter(ObjectGuid bot, uint32 interruptSpellId, uint32 range)
{
    // Check if already registered
    for (auto& interrupter : _interrupters)
    {
        if (interrupter.botGuid == bot)
        {
            // Update existing
            interrupter.interruptSpellId = interruptSpellId;
            interrupter.range = range;
            return;
        }
    }

    // Add new interrupter
    InterrupterBot newInterrupter;
    newInterrupter.botGuid = bot;
    newInterrupter.interruptSpellId = interruptSpellId;
    newInterrupter.range = range;
    newInterrupter.cooldownRemaining = 0;
    newInterrupter.isInRange = false;
    newInterrupter.lastInterruptTime = 0;
    newInterrupter.interruptsPerformed = 0;
    newInterrupter.isAssigned = false;

    _interrupters.push_back(newInterrupter);

    // Add to rotation queue
    if (_config.useRotation)
    {
        _rotationQueue.push(bot);
    }
}

void InterruptRotationManager::UpdateInterrupterStatus(ObjectGuid bot, bool available, uint32 cooldownMs)
{
    for (auto& interrupter : _interrupters)
    {
        if (interrupter.botGuid == bot)
        {
            interrupter.cooldownRemaining = cooldownMs;

            if (!available)
            {
                interrupter.isAssigned = false;
            }

            break;
        }
    }
}

void InterruptRotationManager::HandleFailedInterrupt(Unit* caster, uint32 spellId)
{
    if (!caster || !spellId)
        return;

    _statistics.failedInterrupts++;

    // Find the cast
    ActiveCast* activeCast = nullptr;
    for (auto& cast : _activeCasts)
    {
        if (cast.casterGuid == caster->GetGUID() && cast.spellId == spellId)
        {
            activeCast = &cast;
            break;
        }
    }

    if (!activeCast || activeCast->fallbackTriggered)
        return;

    // Determine fallback method
    FallbackMethod method = SelectFallbackMethod(spellId);

    if (method != FallbackMethod::FALLBACK_NONE)
    {
        if (ExecuteFallback(method, caster))
        {
            activeCast->fallbackTriggered = true;
            _statistics.fallbacksUsed++;
        }
    }
}

InterruptRotationManager::FallbackMethod InterruptRotationManager::SelectFallbackMethod(uint32 spellId) const
{
    auto it = s_interruptDatabase.find(spellId);

    if (it != s_interruptDatabase.end())
    {
        const InterruptableSpell& spell = it->second;

        // High priority spells need immediate action
    if (spell.priority >= InterruptPriority::PRIORITY_HIGH)
        {
            if (spell.isHeal)
                return FallbackMethod::FALLBACK_STUN; // Stun stops heals
            else if (spell.causesCC)
                return FallbackMethod::FALLBACK_SILENCE; // Silence for CC
            else if (spell.isAOE)
                return FallbackMethod::FALLBACK_RANGE; // Move out of AOE
            else
                return FallbackMethod::FALLBACK_STUN; // Default to stun
        }

        // Medium priority - try LOS
    if (spell.priority == InterruptPriority::PRIORITY_MEDIUM)
        {
            return FallbackMethod::FALLBACK_LOS;
        }
    }

    return FallbackMethod::FALLBACK_DEFENSIVE;
}

bool InterruptRotationManager::ExecuteFallback(FallbackMethod method, Unit* caster)
{
    if (!_bot || !caster)
        return false;

    switch (method)
    {
        case FallbackMethod::FALLBACK_STUN:
            return TryAlternativeInterrupt(caster);

        case FallbackMethod::FALLBACK_SILENCE:
            // Try silence abilities based on class
    if (_bot->GetClass() == CLASS_PRIEST)
            {
                if (!_bot->GetSpellHistory()->HasCooldown(SPELL_SILENCE))
                {
                    // Packet-based Silence fallback via SpellPacketBuilder (thread-safe)
                    SpellPacketBuilder::BuildOptions options;
                    options.skipGcdCheck = false;
                    options.skipResourceCheck = false;
                    options.skipTargetCheck = false;
                    options.skipStateCheck = false;
                    options.skipRangeCheck = false;
                    options.logFailures = true;

                    auto result = SpellPacketBuilder::BuildCastSpellPacket(_bot, SPELL_SILENCE, caster, options);
                    if (result.result == SpellPacketBuilder::ValidationResult::SUCCESS)
                    {
                        TC_LOG_DEBUG("playerbot.interrupt.fallback",
                                     "Bot {} queued SILENCE fallback (target: {})",
                                     _bot->GetName(), caster->GetName());
                        return true;
                    }
                }
            }
            else if (_bot->GetClass() == CLASS_DRUID)
            {
                if (!_bot->GetSpellHistory()->HasCooldown(SPELL_SOLAR_BEAM))
                {
                    // Packet-based Solar Beam fallback via SpellPacketBuilder (thread-safe)
                    SpellPacketBuilder::BuildOptions options;
                    options.skipGcdCheck = false;
                    options.skipResourceCheck = false;
                    options.skipTargetCheck = false;
                    options.skipStateCheck = false;
                    options.skipRangeCheck = false;
                    options.logFailures = true;

                    auto result = SpellPacketBuilder::BuildCastSpellPacket(_bot, SPELL_SOLAR_BEAM, caster, options);
                    if (result.result == SpellPacketBuilder::ValidationResult::SUCCESS)
                    {
                        TC_LOG_DEBUG("playerbot.interrupt.fallback",
                                     "Bot {} queued SOLAR_BEAM fallback (target: {})",
                                     _bot->GetName(), caster->GetName());
                        return true;
                    }
                }
            }
            break;

        case FallbackMethod::FALLBACK_LOS:
            // Request movement to break LOS
            // Movement system integration point
            return true;

        case FallbackMethod::FALLBACK_RANGE:
            // Request movement out of range
            // Movement system integration point
            return true;

        case FallbackMethod::FALLBACK_DEFENSIVE:
            // Use defensive cooldowns
            // Defensive system integration point
            return true;

        default:
            break;
    }

    return false;
}

bool InterruptRotationManager::TryAlternativeInterrupt(Unit* target)
{
    if (!_bot || !target)
        return false;

    // Find our interrupter data
    for (const auto& interrupter : _interrupters)
    {
        if (interrupter.botGuid == _bot->GetGUID())
        {
            // Try alternative interrupts
    for (uint32 spellId : interrupter.alternativeInterrupts)
            {
                if (!_bot->GetSpellHistory()->HasCooldown(spellId))
                {
                    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
                    if (spellInfo && _bot->IsWithinLOSInMap(target))
                    {
                        // Packet-based alternative interrupt via SpellPacketBuilder (thread-safe)
                        SpellPacketBuilder::BuildOptions options;
                        options.skipGcdCheck = false;
                        options.skipResourceCheck = false;
                        options.skipTargetCheck = false;
                        options.skipStateCheck = false;
                        options.skipRangeCheck = false;
                        options.logFailures = true;

                        auto result = SpellPacketBuilder::BuildCastSpellPacket(_bot, spellId, target, options);
                        if (result.result == SpellPacketBuilder::ValidationResult::SUCCESS)
                        {
                            TC_LOG_DEBUG("playerbot.interrupt.alternative",
                                         "Bot {} queued alternative interrupt {} (target: {})",
                                         _bot->GetName(), spellId, target->GetName());
                            return true;
                        }
                    }
                }
            }
            break;
        }
    }

    return false;
}

void InterruptRotationManager::ScheduleDelayedInterrupt(ObjectGuid bot, ObjectGuid target, uint32 spellId, uint32 delayMs)
{
    DelayedInterrupt delayed;
    delayed.interrupter = bot;
    delayed.target = target;
    delayed.spellId = spellId;
    delayed.executeTime = GameTime::GetGameTimeMS() + delayMs;

    _delayedInterrupts.push_back(delayed);
}

void InterruptRotationManager::ProcessDelayedInterrupts()
{
    uint32 currentTime = GameTime::GetGameTimeMS();

    auto it = _delayedInterrupts.begin();
    while (it != _delayedInterrupts.end())
    {
        if (it->executeTime <= currentTime)
        {
            // PHASE 5B: Thread-safe spatial grid validation (replaces ObjectAccessor::GetUnit)
            auto interrupterSnapshot = SpatialGridQueryHelpers::FindCreatureByGuid(_bot, it->interrupter);
            auto targetSnapshot = SpatialGridQueryHelpers::FindCreatureByGuid(_bot, it->target);

            // Get actual Unit* for spell casting
            Unit* interrupter = nullptr;
            Unit* target = nullptr;

            if (interrupterSnapshot)
                interrupter = ObjectAccessor::GetUnit(*_bot, it->interrupter);
            if (targetSnapshot)
                target = ObjectAccessor::GetUnit(*_bot, it->target);

            if (interrupter && target && target->IsNonMeleeSpellCast(false))
            {
                // Packet-based delayed interrupt via SpellPacketBuilder (thread-safe)
                SpellPacketBuilder::BuildOptions options;
                options.skipGcdCheck = false;
                options.skipResourceCheck = false;
                options.skipTargetCheck = false;
                options.skipStateCheck = false;
                options.skipRangeCheck = false;
                options.logFailures = true;

                auto result = SpellPacketBuilder::BuildCastSpellPacket(
                    dynamic_cast<Player*>(interrupter), it->spellId, target, options);
                if (result.result == SpellPacketBuilder::ValidationResult::SUCCESS)
                {
                    TC_LOG_DEBUG("playerbot.interrupt.delayed",
                                 "Bot {} queued delayed interrupt {} (target: {})",
                                 interrupter->GetName(), it->spellId, target->GetName());
                    MarkInterruptUsed(it->interrupter, currentTime);
                }
            }

            it = _delayedInterrupts.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void InterruptRotationManager::CoordinateGroupInterrupts(const ::std::vector<Unit*>& casters)
{
    if (casters.empty())
        return;

    // Sort casters by threat/priority
    ::std::vector<::std::pair<Unit*, float>> prioritizedCasters;

    for (Unit* caster : casters)
    {
        if (!caster || !caster->IsNonMeleeSpellCast(false))
            continue;

        Spell* spell = caster->GetCurrentSpell(CURRENT_GENERIC_SPELL);
        if (!spell)
            continue;

        uint32 spellId = spell->m_spellInfo->Id;
        float priority = static_cast<float>(GetSpellPriority(spellId));

        prioritizedCasters.push_back({caster, priority});
    }

    // Sort by priority
    ::std::sort(prioritizedCasters.begin(), prioritizedCasters.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });

    // Assign interrupters with coordination delay
    uint32 delay = 0;
    for (const auto& [caster, priority] : prioritizedCasters)
    {
        Spell* spell = caster->GetCurrentSpell(CURRENT_GENERIC_SPELL);
        if (!spell)
            continue;

        uint32 spellId = spell->m_spellInfo->Id;
        ObjectGuid interrupter = SelectInterrupter(caster, spellId);
        if (!interrupter.IsEmpty())
        {
            if (delay > 0)
            {
                ScheduleDelayedInterrupt(interrupter, caster->GetGUID(), spellId, delay);
            }

            delay += _config.coordinationDelayMs;
        }
    }
}

InterruptRotationManager::InterruptPriority InterruptRotationManager::GetSpellPriority(uint32 spellId) const
{
    auto it = s_interruptDatabase.find(spellId);
    if (it != s_interruptDatabase.end())
    {
        return it->second.priority;
    }

    return InterruptPriority::PRIORITY_LOW;
}

bool InterruptRotationManager::ShouldInterrupt(uint32 spellId) const
{
    auto it = s_interruptDatabase.find(spellId);
    if (it != s_interruptDatabase.end())
    {
        return it->second.priority >= InterruptPriority::PRIORITY_LOW;
    }

    // Check if spell has cast time and can be interrupted
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (spellInfo && spellInfo->CastTimeEntry && !spellInfo->HasAttribute(SPELL_ATTR7_NO_UI_NOT_INTERRUPTIBLE))
    {
        return true;
    }

    return false;
}

void InterruptRotationManager::RegisterInterruptableSpell(const InterruptableSpell& spell)
{
    s_interruptDatabase[spell.spellId] = spell;
}

void InterruptRotationManager::RecordInterruptAttempt(uint32 spellId, bool success)
{
    if (success)
    {
        _statistics.successfulInterrupts++;
        _statistics.interruptsBySpell[spellId]++;
    }
    else
    {
        _statistics.failedInterrupts++;
    }
}

uint32 InterruptRotationManager::GetTimeToComplete(const ActiveCast& cast) const
{
    uint32 currentTime = GameTime::GetGameTimeMS();

    if (cast.castEndTime > currentTime)
        return cast.castEndTime - currentTime;

    return 0;
}

void InterruptRotationManager::Reset()
{
    _interrupters.clear();
    _activeCasts.clear();
    _delayedInterrupts.clear();
    _unitCache.clear();

    while (!_rotationQueue.empty())
    {
        _rotationQueue.pop();
    }

    _statistics = InterruptStatistics();
}

void InterruptRotationManager::CleanupExpiredData()
{
    uint32 currentTime = GameTime::GetGameTimeMS();

    // Remove completed or expired casts
    _activeCasts.erase(
        ::std::remove_if(_activeCasts.begin(), _activeCasts.end(),
            [currentTime](const ActiveCast& cast) {
                return cast.castEndTime < currentTime || cast.interrupted;
            }),
        _activeCasts.end()
    );

    // Clear assignments for completed casts
    for (auto& interrupter : _interrupters)
    {
        bool hasActiveCast = false;
        for (const auto& cast : _activeCasts)
        {
            if (cast.assignedInterrupter == interrupter.botGuid)
            {
                hasActiveCast = true;
                break;
            }
        }

        if (!hasActiveCast)
        {
            interrupter.isAssigned = false;
        }
    }
}

bool InterruptRotationManager::CanReachInTime(const InterrupterBot& bot, Unit* target, uint32 timeAvailable) const
{
    if (!_bot || !target)
        return false;

    // PHASE 5B: Thread-safe spatial grid validation (replaces ObjectAccessor::GetUnit)
    auto snapshot = SpatialGridQueryHelpers::FindCreatureByGuid(_bot, bot.botGuid);
    if (!snapshot)
        return false;

    // Get actual Unit* for speed calculation
    Unit* botUnit = ObjectAccessor::GetUnit(*_bot, bot.botGuid);
    if (!botUnit)
        return false;

    float distance = ::std::sqrt(botUnit->GetExactDistSq(target)); // Calculate once from squared distance
    float range = bot.range + _config.interruptRangeBuffer;

    if (distance <= range)
        return true;

    // Check if can move into range
    float moveSpeed = botUnit->GetSpeed(MOVE_RUN);
    float timeToReach = (distance - range) / moveSpeed * 1000.0f; // Convert to ms

    // Add reaction time
    timeToReach += _config.reactionTimeMs;

    return timeToReach < timeAvailable;
}

void InterruptRotationManager::UpdateRangeStatus(Unit* target)
{
    if (!target)
        return;

    for (auto& interrupter : _interrupters)
    {
        // PHASE 5B: Thread-safe spatial grid validation (replaces ObjectAccessor::GetUnit)
        auto snapshot = SpatialGridQueryHelpers::FindCreatureByGuid(_bot, interrupter.botGuid);
        if (!snapshot)
            continue;

        // Use snapshot position for distance calculation (lock-free)
        float distance = ::std::sqrt(_bot->GetExactDistSq(snapshot->position)); // Calculate once from squared distance
        interrupter.isInRange = (distance <= interrupter.range + _config.interruptRangeBuffer);
    }
}

// Static method implementations
void InterruptRotationManager::InitializeGlobalDatabase()
{
    if (s_databaseInitialized)
        return;

    // MANDATORY - Heals
    s_interruptDatabase[SPELL_FLASH_HEAL] = {SPELL_FLASH_HEAL, InterruptPriority::PRIORITY_MANDATORY, 1500, false, false, 0, 0, false, true, 1000};
    s_interruptDatabase[SPELL_GREATER_HEAL] = {SPELL_GREATER_HEAL, InterruptPriority::PRIORITY_MANDATORY, 2500, false, false, 0, 0, false, true, 2000};
    s_interruptDatabase[SPELL_HOLY_LIGHT] = {SPELL_HOLY_LIGHT, InterruptPriority::PRIORITY_MANDATORY, 2500, false, false, 0, 0, false, true, 2000};
    s_interruptDatabase[SPELL_FLASH_OF_LIGHT] = {SPELL_FLASH_OF_LIGHT, InterruptPriority::PRIORITY_MANDATORY, 1500, false, false, 0, 0, false, true, 1000};
    s_interruptDatabase[SPELL_REGROWTH] = {SPELL_REGROWTH, InterruptPriority::PRIORITY_MANDATORY, 2000, false, false, 0, 0, false, true, 1500};
    s_interruptDatabase[SPELL_HEALING_TOUCH] = {SPELL_HEALING_TOUCH, InterruptPriority::PRIORITY_MANDATORY, 3000, false, false, 0, 0, false, true, 2500};
    s_interruptDatabase[SPELL_CHAIN_HEAL] = {SPELL_CHAIN_HEAL, InterruptPriority::PRIORITY_MANDATORY, 2500, false, false, 0, 0, false, true, 2000};
    s_interruptDatabase[SPELL_HEALING_WAVE] = {SPELL_HEALING_WAVE, InterruptPriority::PRIORITY_MANDATORY, 3000, false, false, 0, 0, false, true, 2500};
    s_interruptDatabase[SPELL_LESSER_HEALING] = {SPELL_LESSER_HEALING, InterruptPriority::PRIORITY_HIGH, 2500, false, false, 0, 0, false, true, 2000};

    // MANDATORY - Crowd Control
    s_interruptDatabase[SPELL_POLYMORPH] = {SPELL_POLYMORPH, InterruptPriority::PRIORITY_MANDATORY, 1500, false, false, 0, 0, true, false, 1200};
    s_interruptDatabase[SPELL_FEAR] = {SPELL_FEAR, InterruptPriority::PRIORITY_MANDATORY, 1500, false, false, 0, 0, true, false, 1200};
    s_interruptDatabase[SPELL_PSYCHIC_SCREAM] = {SPELL_PSYCHIC_SCREAM, InterruptPriority::PRIORITY_MANDATORY, 0, false, true, 8.0f, 0, true, false, 0};
    s_interruptDatabase[SPELL_MIND_CONTROL] = {SPELL_MIND_CONTROL, InterruptPriority::PRIORITY_MANDATORY, 3000, false, false, 0, 0, true, false, 2500};
    s_interruptDatabase[SPELL_HEX] = {SPELL_HEX, InterruptPriority::PRIORITY_MANDATORY, 1500, false, false, 0, 0, true, false, 1200};
    s_interruptDatabase[SPELL_CYCLONE] = {SPELL_CYCLONE, InterruptPriority::PRIORITY_MANDATORY, 1500, false, false, 0, 0, true, false, 1200};
    s_interruptDatabase[SPELL_ENTANGLING_ROOTS] = {SPELL_ENTANGLING_ROOTS, InterruptPriority::PRIORITY_HIGH, 1500, false, false, 0, 0, true, false, 1200};
    s_interruptDatabase[SPELL_HIBERNATE] = {SPELL_HIBERNATE, InterruptPriority::PRIORITY_HIGH, 1500, false, false, 0, 0, true, false, 1200};
    s_interruptDatabase[SPELL_BANISH] = {SPELL_BANISH, InterruptPriority::PRIORITY_HIGH, 1500, false, false, 0, 0, true, false, 1200};

    // HIGH - Major Damage
    s_interruptDatabase[SPELL_PYROBLAST] = {SPELL_PYROBLAST, InterruptPriority::PRIORITY_HIGH, 3500, false, false, 0, 5000, false, false, 3000};
    s_interruptDatabase[SPELL_CHAOS_BOLT] = {SPELL_CHAOS_BOLT, InterruptPriority::PRIORITY_HIGH, 3000, false, false, 0, 6000, false, false, 2500};
    s_interruptDatabase[SPELL_GREATER_PYROBLAST] = {SPELL_GREATER_PYROBLAST, InterruptPriority::PRIORITY_HIGH, 4500, false, false, 0, 8000, false, false, 4000};
    s_interruptDatabase[SPELL_AIMED_SHOT] = {SPELL_AIMED_SHOT, InterruptPriority::PRIORITY_HIGH, 3000, false, false, 0, 4000, false, false, 2500};
    s_interruptDatabase[SPELL_SOUL_FIRE] = {SPELL_SOUL_FIRE, InterruptPriority::PRIORITY_HIGH, 4000, false, false, 0, 7000, false, false, 3500};
    s_interruptDatabase[SPELL_MIND_BLAST] = {SPELL_MIND_BLAST, InterruptPriority::PRIORITY_HIGH, 1500, false, false, 0, 3000, false, false, 1200};
    s_interruptDatabase[SPELL_STARSURGE] = {SPELL_STARSURGE, InterruptPriority::PRIORITY_HIGH, 2000, false, false, 0, 4000, false, false, 1500};

    // MEDIUM - Standard Damage
    s_interruptDatabase[SPELL_FROSTBOLT] = {SPELL_FROSTBOLT, InterruptPriority::PRIORITY_MEDIUM, 2500, false, false, 0, 2000, false, false, 2000};
    s_interruptDatabase[SPELL_FIREBALL] = {SPELL_FIREBALL, InterruptPriority::PRIORITY_MEDIUM, 3000, false, false, 0, 2500, false, false, 2500};
    s_interruptDatabase[SPELL_SHADOW_BOLT] = {SPELL_SHADOW_BOLT, InterruptPriority::PRIORITY_MEDIUM, 2500, false, false, 0, 2000, false, false, 2000};
    s_interruptDatabase[SPELL_LIGHTNING_BOLT] = {SPELL_LIGHTNING_BOLT, InterruptPriority::PRIORITY_MEDIUM, 2500, false, false, 0, 1800, false, false, 2000};
    s_interruptDatabase[SPELL_WRATH] = {SPELL_WRATH, InterruptPriority::PRIORITY_MEDIUM, 2000, false, false, 0, 1500, false, false, 1500};
    s_interruptDatabase[SPELL_STARFIRE] = {SPELL_STARFIRE, InterruptPriority::PRIORITY_MEDIUM, 3500, false, false, 0, 3000, false, false, 3000};
    s_interruptDatabase[SPELL_HOLY_FIRE] = {SPELL_HOLY_FIRE, InterruptPriority::PRIORITY_MEDIUM, 3000, false, false, 0, 2000, false, false, 2500};

    // Channeled Spells
    s_interruptDatabase[SPELL_EVOCATION] = {SPELL_EVOCATION, InterruptPriority::PRIORITY_HIGH, 8000, true, false, 0, 0, false, false, 1000};
    s_interruptDatabase[SPELL_ARCANE_MISSILES] = {SPELL_ARCANE_MISSILES, InterruptPriority::PRIORITY_MEDIUM, 5000, true, false, 0, 2500, false, false, 1000};
    s_interruptDatabase[SPELL_DRAIN_LIFE] = {SPELL_DRAIN_LIFE, InterruptPriority::PRIORITY_MEDIUM, 5000, true, false, 0, 1500, false, true, 1000};
    s_interruptDatabase[SPELL_DRAIN_SOUL] = {SPELL_DRAIN_SOUL, InterruptPriority::PRIORITY_MEDIUM, 5000, true, false, 0, 2000, false, false, 1000};
    s_interruptDatabase[SPELL_MIND_FLAY] = {SPELL_MIND_FLAY, InterruptPriority::PRIORITY_MEDIUM, 3000, true, false, 0, 1800, false, false, 500};
    s_interruptDatabase[SPELL_TRANQUILITY] = {SPELL_TRANQUILITY, InterruptPriority::PRIORITY_MANDATORY, 8000, true, true, 40.0f, 0, false, true, 1000};
    s_interruptDatabase[SPELL_DIVINE_HYMN] = {SPELL_DIVINE_HYMN, InterruptPriority::PRIORITY_MANDATORY, 8000, true, true, 40.0f, 0, false, true, 1000};

    s_databaseInitialized = true;
}

::std::vector<uint32> InterruptRotationManager::GetClassInterrupts(uint8 classId)
{
    ::std::vector<uint32> interrupts;

    switch (classId)
    {
        case CLASS_WARRIOR:
            interrupts.push_back(SPELL_PUMMEL);
            break;

        case CLASS_PALADIN:
            interrupts.push_back(SPELL_REBUKE);
            break;

        case CLASS_HUNTER:
            interrupts.push_back(SPELL_COUNTER_SHOT);
            break;

        case CLASS_ROGUE:
            interrupts.push_back(SPELL_KICK);
            break;

        case CLASS_PRIEST:
            interrupts.push_back(SPELL_SILENCE);
            break;

        case CLASS_DEATH_KNIGHT:
            interrupts.push_back(SPELL_MIND_FREEZE);
            break;

        case CLASS_SHAMAN:
            interrupts.push_back(SPELL_WIND_SHEAR);
            break;

        case CLASS_MAGE:
            interrupts.push_back(SPELL_COUNTERSPELL);
            break;

        case CLASS_WARLOCK:
            interrupts.push_back(SPELL_SPELL_LOCK);
            break;

        case CLASS_MONK:
            interrupts.push_back(SPELL_SPEAR_HAND_STRIKE);
            break;

        case CLASS_DRUID:
            interrupts.push_back(SPELL_SKULL_BASH);
            interrupts.push_back(SPELL_SOLAR_BEAM);
            break;

        case CLASS_DEMON_HUNTER:
            interrupts.push_back(SPELL_DISRUPT);
            break;

        default:
            break;
    }

    return interrupts;
}

} // namespace Playerbot