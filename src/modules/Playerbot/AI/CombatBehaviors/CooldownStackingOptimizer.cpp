/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "CooldownStackingOptimizer.h"
#include "GameTime.h"
#include "AI/BotAI.h"
#include "Player.h"
#include "Unit.h"
#include "Creature.h"
#include "Spell.h"
#include "SpellAuras.h"
#include "SpellMgr.h"
#include "SpellInfo.h"
#include "SpellHistory.h"
#include "Group.h"
#include "ObjectAccessor.h"
#include "Log.h"
#include "Timer.h"
#include "UpdateFields.h"
#include "../../Group/GroupRoleEnums.h" // Centralized role detection utilities
#include "../../Core/Threading/SafeGridOperations.h"  // SEH-protected grid operations
#include <algorithm>
#include <cmath>

namespace Playerbot
{

namespace
{
    // Role Detection Helpers - Using centralized utilities from GroupRoleEnums.h
    enum BotRole : uint8 {
        BOT_ROLE_TANK = 0,
        BOT_ROLE_HEALER = 1,
        BOT_ROLE_DPS = 2
    };

    BotRole GetPlayerRole(Player const* player)
    {
        if (!player) return BOT_ROLE_DPS;

        // Use centralized role detection from GroupRoleEnums.h
        // This queries ChrSpecializationEntry for accurate role detection
        GroupRole role = GetPlayerSpecRole(player);

        switch (role)
        {
            case GroupRole::TANK:
                return BOT_ROLE_TANK;
            case GroupRole::HEALER:
                return BOT_ROLE_HEALER;
            default:
                return BOT_ROLE_DPS;
        }
    }
    bool IsTank(Player const* p) { return IsPlayerTank(p); }
    bool IsHealer(Player const* p) { return IsPlayerHealer(p); }
    bool IsDPS(Player const* p) { return IsPlayerDPS(p); }

    // Major cooldown spell IDs by class
    enum MajorCooldowns : uint32
    {
        // Warrior
        RECKLESSNESS = 1719,
        AVATAR = 107574,
        BLADESTORM = 46924,
        COLOSSUS_SMASH = 167105,

        // Paladin
        AVENGING_WRATH = 31884,
        CRUSADE = 231895,
        HOLY_AVENGER = 105809,
        SHIELD_OF_VENGEANCE = 184662,

        // Hunter
        BESTIAL_WRATH = 19574,
        ASPECT_OF_THE_WILD = 193530,
        COORDINATED_ASSAULT = 266779,
        TRUESHOT = 288613,

        // Rogue
        SHADOW_BLADES = 121471,
        VENDETTA = 79140,
        ADRENALINE_RUSH = 13750,
        KILLING_SPREE = 51690,

        // Priest
        POWER_INFUSION = 10060,
        SHADOW_FIEND = 34433,
        VOID_ERUPTION = 228260,
        DARK_ASCENSION = 391109,

        // Shaman
        FIRE_ELEMENTAL = 198067,
        STORM_ELEMENTAL = 192249,
        FERAL_SPIRIT = 51533,
        ASCENDANCE = 114050,

        // Mage
        ARCANE_POWER = 12042,
        ICY_VEINS = 12472,
        COMBUSTION = 190319,
        TIME_WARP = 80353,

        // Warlock
        SUMMON_INFERNAL = 1122,
        SUMMON_DOOMGUARD = 18540,
        DARK_SOUL_INSTABILITY = 113858,
        DARK_SOUL_MISERY = 113860,

        // Druid
        CELESTIAL_ALIGNMENT = 194223,
        INCARNATION_CHOSEN = 102560,
        INCARNATION_KING = 102543,
        BERSERK = 106951,

        // Death Knight
        ARMY_OF_THE_DEAD = 42650,
        UNHOLY_FRENZY = 207289,
        APOCALYPSE = 275699,
        DANCING_RUNE_WEAPON = 49028,

        // Bloodlust/Heroism
        BLOODLUST = 2825,
        HEROISM = 32182,
        TIME_WARP_BUFF = 80353,
        ANCIENT_HYSTERIA = 90355
    };
}

// Static member definitions moved to header with inline to fix DLL linkage (C2491)

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

CooldownStackingOptimizer::CooldownStackingOptimizer(BotAI* ai) :
    _ai(ai),
    _bot(ai ? ai->GetBot() : nullptr),
    _currentPhase(NORMAL),
    _lastPhase(NORMAL),
    _phaseStartTime(0),
    _lastPhaseUpdate(0),
    _lastBloodlustTime(0),
    _predictedBloodlustTime(0),
    _bloodlustUsed(false),
    _aggressiveUsage(false),
    _phaseLookaheadMs(15000),
    _alignWithBloodlust(true),
    _lastOptimizationCalc(0)
{
    if (!s_defaultsInitialized)
    {
        InitializeDefaults();
        s_defaultsInitialized = true;
    }

    InitializeClassCooldowns();
}

CooldownStackingOptimizer::~CooldownStackingOptimizer() = default;

// ============================================================================
// CORE UPDATE
// ============================================================================

void CooldownStackingOptimizer::Update(uint32 diff)
{
    if (!_bot || !_bot->IsAlive())
    {
        _currentPhase = NORMAL;
        return;
    }

    uint32 now = GameTime::GetGameTimeMS();

    // Update phase detection every 500ms
    if (now - _lastPhaseUpdate > 500)
    {
        UpdatePhaseDetection();
        _lastPhaseUpdate = now;
    }

    // Update cooldown availability
    for (auto& [spellId, data] : _cooldowns)
    {
        if (data.nextAvailable > 0 && now >= data.nextAvailable)
        {
            data.nextAvailable = 0;  // Cooldown is available
        }

        // Check actual cooldown from spell history
    if (_bot->GetSpellHistory()->HasCooldown(spellId))
        {
            SpellHistory::Duration remainingCooldown = _bot->GetSpellHistory()->GetRemainingCooldown(sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE));
            data.nextAvailable = now + remainingCooldown.count();
        }
    }

    // Clear expired reservations
    for (auto it = _phaseReservations.begin(); it != _phaseReservations.end();)
    {
        if (it->timeUntil > 0 && it->timeUntil <= diff)
        {
            // Phase has arrived, clear reservation
    for (uint32 spellId : it->cooldowns)
                _reservedCooldowns.erase(spellId);
            it = _phaseReservations.erase(it);
        }
        else
        {
            if (it->timeUntil > diff)
                it->timeUntil -= diff;
            ++it;
        }
    }

    // Update damage history for phase detection
    if (_bot->IsInCombat())
    {
        DamageSnapshot snapshot;
        snapshot.timestamp = now;
        snapshot.damageDealt = 0.0f;  // Would track actual damage
        snapshot.damageTaken = 0.0f;  // Would track actual damage

        _damageHistory.push(snapshot);
        while (_damageHistory.size() > MAX_DAMAGE_HISTORY)
            _damageHistory.pop();
    }
}

// ============================================================================
// BOSS PHASES
// ============================================================================

CooldownStackingOptimizer::BossPhase CooldownStackingOptimizer::DetectBossPhase(Unit* boss) const
{
    if (!boss)
        return NORMAL;

    float healthPct = boss->GetHealthPct();

    // Execute phase detection
    if (healthPct <= 20.0f)
        return EXECUTE;

    // Check for defensive auras on boss
    if (boss->HasAura(871) ||   // Shield Wall example
        boss->HasAura(31224))    // Cloak of Shadows example
        return DEFENSIVE;

    // Check for burn phase indicators
    if (boss->HasAura(32182) || // Heroism
        boss->HasAura(2825))     // Bloodlust
        return BURN;

    // Check for adds
    uint32 addCount = 0;
    ::std::list<Creature*> creatures;
    if (boss->GetTypeId() == TYPEID_UNIT)
    {
        // THREAD-SAFE: Use SafeGridOperations with SEH protection to catch access violations
        SafeGridOperations::GetCreatureListFromCreatureSafe(boss->ToCreature(), creatures, 0, 30.0f);  // 0 = any entry
        for (Creature* creature : creatures)
        {
            if (creature && creature->IsAlive() && creature != boss &&
                creature->IsHostileTo(_bot))
                ++addCount;
        }
    }

    if (addCount >= 3)
        return ADD;

    // Check health thresholds for phase transitions
    if ((healthPct <= 75.0f && healthPct > 74.0f) ||
        (healthPct <= 50.0f && healthPct > 49.0f) ||
        (healthPct <= 25.0f && healthPct > 24.0f))
        return TRANSITION;

    // Full burn phase detection with encounter integration
    // Analyzes multiple indicators for burn phase timing

    // Check for explicit burn phase buffs/debuffs on boss
    // These are common soft enrage indicators
    static constexpr uint32 BURN_PHASE_INDICATORS[] = {
        26662,   // Berserk (generic enrage timer)
        47008,   // Rage (raid boss soft enrage)
        8599,    // Enrage (physical damage increase)
        12880,   // Enrage (larger version)
        134488,  // Rage (MoP boss enrage)
        156598,  // Dark Devastation (WoD raid mechanic)
        260703,  // Bloodlust aura (Legion/BFA)
        386629,  // Soft Enrage (Dragonflight)
        396094,  // Frothing Venom (12.0 Nerub-ar Palace)
        439785   // Reactive Toxin (12.0 encounter)
    };

    for (uint32 auraId : BURN_PHASE_INDICATORS)
    {
        if (boss->HasAura(auraId))
            return BURN;
    }

    // Check for vulnerability debuffs on boss indicating burn window
    static constexpr uint32 VULNERABILITY_DEBUFFS[] = {
        1490,    // Curse of the Elements
        81326,   // Physical Vulnerability
        113746,  // Mystic Touch
        1079,    // Rip (bleed effect stacking)
        8680,    // Instant Poison
        51460    // Runic Corruption
    };

    uint32 vulnCount = 0;
    for (uint32 debuffId : VULNERABILITY_DEBUFFS)
    {
        if (boss->HasAura(debuffId))
            ++vulnCount;
    }

    // Multiple debuffs indicate coordinated burn phase
    if (vulnCount >= 3)
        return BURN;

    // Check if boss is casting a major ability (indicates phase change incoming)
    if (boss->IsNonMeleeSpellCast(false))
    {
        Spell const* currentSpell = boss->GetCurrentSpell(CURRENT_GENERIC_SPELL);
        if (currentSpell)
        {
            SpellInfo const* spellInfo = currentSpell->GetSpellInfo();
            if (spellInfo)
            {
                // Check cast time - long casts often indicate major phase abilities
                uint32 castTime = spellInfo->CalcCastTime();
                if (castTime >= 5000) // 5+ second cast
                {
                    // This could be a burn window during channel
                    return BURN;
                }
            }
        }
    }

    // Boss health-based burn phase detection with dynamic thresholds
    // Different encounter types have different burn thresholds
    if (boss->GetTypeId() == TYPEID_UNIT)
    {
        Creature* creature = boss->ToCreature();
        if (creature)
        {
            // Dungeon bosses: earlier burn phase
            if (creature->IsDungeonBoss())
            {
                if (healthPct <= 35.0f && healthPct > 20.0f)
                    return BURN;
            }
            // Raid bosses: tighter burn window
            else if (creature->isWorldBoss())
            {
                if (healthPct <= 25.0f && healthPct > 20.0f)
                    return BURN;
            }
        }
    }

    // Default burn phase threshold
    if (healthPct <= 30.0f && healthPct > 20.0f)
        return BURN;

    return NORMAL;
}

// ============================================================================
// COOLDOWN MANAGEMENT
// ============================================================================

void CooldownStackingOptimizer::RegisterCooldown(CooldownData const& data)
{
    _cooldowns[data.spellId] = data;
}

void CooldownStackingOptimizer::UpdateCooldownUsed(uint32 spellId)
{
    auto it = _cooldowns.find(spellId);
    if (it != _cooldowns.end())
    {
        uint32 now = GameTime::GetGameTimeMS();
        it->second.lastUsedTime = now;
        it->second.nextAvailable = now + it->second.cooldownMs;

        // Apply haste if applicable
    if (it->second.affectedByHaste && _bot)
        {
            // Get haste rating from combat rating system
            float hastePercent = _bot->GetRatingBonusValue(CR_HASTE_MELEE);
            float hasteMod = 1.0f + (hastePercent / 100.0f);
            if (hasteMod > 0)
                it->second.nextAvailable = now + uint32(it->second.cooldownMs / hasteMod);
        }
    }
}

// ============================================================================
// STACKING WINDOWS
// ============================================================================

CooldownStackingOptimizer::StackWindow CooldownStackingOptimizer::FindOptimalStackWindow(uint32 lookAheadMs) const
{
    // Cache optimization result
    uint32 now = GameTime::GetGameTimeMS();
    if (now - _lastOptimizationCalc < 2000)
        return _cachedOptimalWindow;

    StackWindow bestWindow;
    uint32 windowStart = now;

    // Find all available cooldowns in the window
    ::std::vector<uint32> availableCooldowns;
    for (auto const& [spellId, data] : _cooldowns)
    {
        if (data.category == MAJOR_DPS || data.category == BURST)
        {
            if (data.nextAvailable <= windowStart)
                availableCooldowns.push_back(spellId);
        }
    }

    if (availableCooldowns.empty())
        return bestWindow;

    // Test different combinations
    uint32 testIntervals = 5;  // Test 5 different timings
    for (uint32 i = 0; i < testIntervals; ++i)
    {
        StackWindow window;
        window.startTime = windowStart + (i * lookAheadMs / testIntervals);
        window.duration = 10000;  // 10 second window

        // Add cooldowns that would be available
    for (uint32 spellId : availableCooldowns)
        {
            auto it = _cooldowns.find(spellId);
            if (it != _cooldowns.end())
            {
                if (!IsCooldownReserved(spellId))
                {
                    window.cooldowns.push_back(spellId);
                }
            }
        }

        // Calculate window score
        window.totalMultiplier = CalculateStackedMultiplier(window.cooldowns);

        // Score based on multiplier and timing
        window.score = window.totalMultiplier;

        // Bonus for aligning with Bloodlust
    if (IsBloodlustActive() || (window.startTime >= _predictedBloodlustTime &&
            window.startTime <= _predictedBloodlustTime + 40000))
        {
            window.score *= 1.5f;
        }

        // Penalty for delaying too long
        float delay = float(window.startTime - now) / 1000.0f;
        if (delay > 10.0f)
            window.score *= (1.0f - delay / 60.0f);  // Lose value over time
    if (window.score > bestWindow.score)
            bestWindow = window;
    }

    const_cast<CooldownStackingOptimizer*>(this)->_cachedOptimalWindow = bestWindow;
    const_cast<CooldownStackingOptimizer*>(this)->_lastOptimizationCalc = now;

    return bestWindow;
}

float CooldownStackingOptimizer::CalculateStackedMultiplier(::std::vector<uint32> const& cooldowns) const
{
    if (cooldowns.empty())
        return 1.0f;

    float totalDamage = 1.0f;
    float totalHaste = 0.0f;
    float totalCrit = 0.0f;
    uint32 stackCount = 0;

    for (uint32 spellId : cooldowns)
    {
        auto it = _cooldowns.find(spellId);
        if (it == _cooldowns.end())
            continue;

        CooldownData const& data = it->second;

        if (data.stacksWithOthers)
        {
            totalDamage *= (1.0f + data.damageIncrease);
            totalHaste += data.hasteIncrease;
            totalCrit += data.critIncrease;
            ++stackCount;
        }
        else
        {
            // Non-stacking cooldown, take the best one
            totalDamage = ::std::max(totalDamage, 1.0f + data.damageIncrease);
        }
    }

    // Apply diminishing returns
    if (stackCount > 1)
    {
        totalDamage = ApplyDiminishingReturns(totalDamage, stackCount);
        totalHaste = ApplyDiminishingReturns(totalHaste, stackCount);
        totalCrit = ApplyDiminishingReturns(totalCrit, stackCount);
    }

    // Combine all modifiers
    float hasteMultiplier = 1.0f + totalHaste;
    float critMultiplier = 1.0f + (totalCrit * 0.5f);  // Crit is worth ~50% of its value

    return totalDamage * hasteMultiplier * critMultiplier;
}

// ============================================================================
// COOLDOWN DECISIONS
// ============================================================================

bool CooldownStackingOptimizer::ShouldUseMajorCooldown(Unit* target) const
{
    if (!target || !_bot)
        return false;

    // Check if target will live long enough
    if (!WillTargetSurvive(target, 10000))  // 10 second minimum
        return false;

    // Check phase appropriateness
    if (_currentPhase == TRANSITION || _currentPhase == DEFENSIVE)
        return false;

    // Strong yes for execute and burn phases
    if (_currentPhase == EXECUTE || _currentPhase == BURN)
        return true;

    // Check if we should wait for Bloodlust
    if (_alignWithBloodlust && !_bloodlustUsed)
    {
        uint32 predictedLust = PredictBloodlustTiming();
        if (predictedLust > 0 && predictedLust < 30000)  // Within 30 seconds
            return false;  // Wait for Bloodlust
    }

    // Check target importance
    if (target->GetTypeId() == TYPEID_UNIT)
    {
        Creature* creature = target->ToCreature();
        if (creature && (creature->IsDungeonBoss() || creature->isWorldBoss()))
            return true;

        // Use on elites if aggressive
    if (_aggressiveUsage && creature && creature->IsElite())
            return true;
    }

    // Default decision based on target health
    return target->GetHealthPct() > 50.0f || _aggressiveUsage;
}

bool CooldownStackingOptimizer::ShouldUseCooldownInPhase(uint32 spellId, BossPhase phase) const
{
    auto it = _cooldowns.find(spellId);
    if (it == _cooldowns.end())
        return false;

    CooldownData const& data = it->second;

    switch (phase)
    {
        case BURN:
            return data.category == MAJOR_DPS || data.category == BURST;

        case EXECUTE:
            return data.category != DEFENSIVE_CD;  // Use everything except defensive

        case DEFENSIVE:
            return data.category == DEFENSIVE_CD || data.category == UTILITY;

        case ADD:
            // Save major cooldowns for boss
            return data.category == MINOR_DPS || data.category == BURST;

        case TRANSITION:
            // Save everything for next phase
            return false;

        case NORMAL:
        default:
            // Use minor cooldowns freely
            return data.category == MINOR_DPS || data.category == BURST ||
                   (_aggressiveUsage && data.category == MAJOR_DPS);
    }
}

float CooldownStackingOptimizer::GetCooldownPriority(uint32 spellId) const
{
    if (!_bot || !_bot->GetVictim())
        return 0.0f;

    auto it = _cooldowns.find(spellId);
    if (it == _cooldowns.end())
        return 0.0f;

    CooldownData const& data = it->second;

    // Check if reserved
    if (IsCooldownReserved(spellId))
        return 0.0f;

    // Check if on cooldown
    if (data.nextAvailable > GameTime::GetGameTimeMS())
        return 0.0f;

    // Base priority on phase
    float priority = CalculatePhaseScore(_currentPhase);

    // Adjust for category
    switch (data.category)
    {
        case MAJOR_DPS:
            priority *= (_currentPhase == BURN || _currentPhase == EXECUTE) ? 1.5f : 0.8f;
            break;
        case BURST:
            priority *= 1.2f;  // Always good
            break;
        case DEFENSIVE_CD:
            priority *= (_currentPhase == DEFENSIVE) ? 2.0f : 0.3f;
            break;
        default:
            break;
    }

    // Adjust for Bloodlust
    if (IsBloodlustActive())
        priority *= 1.5f;

    // Adjust for target
    Unit* target = _bot->GetVictim();
    if (target)
    {
        if (!WillTargetSurvive(target, data.durationMs))
            priority *= 0.2f;  // Heavily reduce priority

        // Increase priority for low health targets in execute phase
    if (_currentPhase == EXECUTE && target->GetHealthPct() < 20.0f)
            priority *= 1.5f;
    }

    return ::std::min(1.0f, priority);
}

// ============================================================================
// PHASE RESERVATION
// ============================================================================

void CooldownStackingOptimizer::ReserveCooldownsForPhase(BossPhase phase, uint32 timeUntilMs)
{
    PhaseReservation reservation;
    reservation.phase = phase;
    reservation.timeUntil = timeUntilMs;

    // Determine which cooldowns to reserve
    for (auto const& [spellId, data] : _cooldowns)
    {
        bool shouldReserve = false;

        switch (phase)
        {
            case BURN:
            case EXECUTE:
                shouldReserve = (data.category == MAJOR_DPS);
                break;

            case DEFENSIVE:
                shouldReserve = (data.category == DEFENSIVE_CD);
                break;

            default:
                break;
        }

        if (shouldReserve)
        {
            reservation.cooldowns.push_back(spellId);
            _reservedCooldowns.insert(spellId);
        }
    }

    if (!reservation.cooldowns.empty())
        _phaseReservations.push_back(reservation);
}

bool CooldownStackingOptimizer::IsCooldownReserved(uint32 spellId) const
{
    return _reservedCooldowns.count(spellId) > 0;
}

void CooldownStackingOptimizer::ClearReservations()
{
    _reservedCooldowns.clear();
    _phaseReservations.clear();
}

// ============================================================================
// BLOODLUST/HEROISM
// ============================================================================

bool CooldownStackingOptimizer::IsBloodlustActive() const
{
    if (!_bot)
        return false;

    return _bot->HasAura(BLOODLUST) ||
           _bot->HasAura(HEROISM) ||
           _bot->HasAura(TIME_WARP_BUFF) ||
           _bot->HasAura(ANCIENT_HYSTERIA);
}

uint32 CooldownStackingOptimizer::PredictBloodlustTiming() const
{
    if (_bloodlustUsed)
        return 0;

    // Simple prediction based on boss health
    if (_bot && _bot->GetVictim())
    {
        Unit* target = _bot->GetVictim();
        float healthPct = target->GetHealthPct();

        // Common Bloodlust timings
    if (healthPct > 95.0f)
            return GameTime::GetGameTimeMS() + 5000;  // Start of fight
    if (healthPct <= 30.0f && healthPct > 20.0f)
            return GameTime::GetGameTimeMS() + 2000;  // Sub-30%
    if (healthPct <= 20.0f)
            return GameTime::GetGameTimeMS();  // Execute phase - immediate
    }

    return 0;
}

bool CooldownStackingOptimizer::ShouldAlignWithBloodlust(uint32 cooldownDuration) const
{
    if (!_alignWithBloodlust || _bloodlustUsed)
        return false;

    uint32 predictedTime = PredictBloodlustTiming();
    if (predictedTime == 0)
        return false;

    uint32 now = GameTime::GetGameTimeMS();
    uint32 timeUntilLust = (predictedTime > now) ? predictedTime - now : 0;

    // Align if Bloodlust is coming within the cooldown duration
    return timeUntilLust < cooldownDuration;
}

// ============================================================================
// OPTIMIZATION METRICS
// ============================================================================

float CooldownStackingOptimizer::CalculateDamageGain(uint32 spellId) const
{
    auto it = _cooldowns.find(spellId);
    if (it == _cooldowns.end())
        return 1.0f;

    CooldownData const& data = it->second;
    float baseGain = 1.0f + data.damageIncrease;

    // Add current modifiers
    baseGain *= GetCurrentDamageModifier();

    // Multiply by phase modifier
    baseGain *= CalculatePhaseScore(_currentPhase);

    return baseGain;
}

uint32 CooldownStackingOptimizer::GetTimeUntilBurnPhase() const
{
    if (_currentPhase == BURN)
        return 0;

    // Predict based on boss health
    if (_bot && _bot->GetVictim())
    {
        Unit* target = _bot->GetVictim();
        float healthPct = target->GetHealthPct();

        // Estimate time based on health thresholds
    if (healthPct > 30.0f)
        {
            uint32 timeToDie = EstimateTimeToDie(target);
            return uint32((healthPct - 30.0f) / 100.0f * timeToDie);
        }
    }

    return 60000;  // Default 60 seconds
}

bool CooldownStackingOptimizer::WillTargetSurvive(Unit* target, uint32 duration) const
{
    if (!target)
        return false;

    uint32 ttd = EstimateTimeToDie(target);
    return ttd > duration;
}

// ============================================================================
// CONFIGURATION
// ============================================================================

void CooldownStackingOptimizer::SetAggressiveUsage(bool aggressive)
{
    _aggressiveUsage = aggressive;
}

void CooldownStackingOptimizer::SetPhaseLookahead(uint32 ms)
{
    _phaseLookaheadMs = ms;
}

void CooldownStackingOptimizer::SetBloodlustAlignment(bool align)
{
    _alignWithBloodlust = align;
}

// ============================================================================
// INTERNAL HELPERS
// ============================================================================

void CooldownStackingOptimizer::InitializeClassCooldowns()
{
    if (!_bot)
        return;

    // Copy defaults for this class
    Classes botClass = static_cast<Classes>(_bot->GetClass());

    for (auto const& [spellId, data] : s_defaultCooldowns)
    {
        // Check if bot knows this spell
    if (_bot->HasSpell(spellId))
        {
            _cooldowns[spellId] = data;
        }
    }

    // Initialize class-specific cooldowns
    switch (botClass)
    {
        case CLASS_WARRIOR:     InitializeWarriorCooldowns(); break;
        case CLASS_PALADIN:     InitializePaladinCooldowns(); break;
        case CLASS_HUNTER:      InitializeHunterCooldowns(); break;
        case CLASS_ROGUE:       InitializeRogueCooldowns(); break;
        case CLASS_PRIEST:      InitializePriestCooldowns(); break;
        case CLASS_SHAMAN:      InitializeShamanCooldowns(); break;
        case CLASS_MAGE:        InitializeMageCooldowns(); break;
        case CLASS_WARLOCK:     InitializeWarlockCooldowns(); break;
        case CLASS_DRUID:       InitializeDruidCooldowns(); break;
        case CLASS_DEATH_KNIGHT: InitializeDeathKnightCooldowns(); break;
        case CLASS_MONK:        InitializeMonkCooldowns(); break;
        case CLASS_DEMON_HUNTER: InitializeDemonHunterCooldowns(); break;
        default: break;
    }
}

void CooldownStackingOptimizer::UpdatePhaseDetection()
{
    if (!_bot || !_bot->IsInCombat())
    {
        _currentPhase = NORMAL;
        return;
    }

    Unit* target = _bot->GetVictim();
    if (target)
    {
        BossPhase newPhase = DetectBossPhase(target);
        if (newPhase != _currentPhase)
        {
            _lastPhase = _currentPhase;
            _currentPhase = newPhase;
            _phaseStartTime = GameTime::GetGameTimeMS();

            // Track Bloodlust usage
    if (newPhase == BURN && IsBloodlustActive())
                _bloodlustUsed = true;
        }
    }
}

float CooldownStackingOptimizer::CalculatePhaseScore(BossPhase phase) const
{
    switch (phase)
    {
        case BURN:      return 1.5f;
        case EXECUTE:   return 1.8f;
        case NORMAL:    return 1.0f;
        case ADD:       return 0.7f;
        case DEFENSIVE: return 0.3f;
        case TRANSITION: return 0.1f;
        default:        return 1.0f;
    }
}

float CooldownStackingOptimizer::GetCurrentDamageModifier() const
{
    if (!_bot)
        return 1.0f;

    float modifier = 1.0f;

    // Check for damage increasing auras
    // This would check for actual aura effects in production
    // For now, return base modifier

    return modifier;
}

uint32 CooldownStackingOptimizer::EstimateTimeToDie(Unit* target) const
{
    if (!target)
        return 0;

    // Simple TTD estimation based on health
    float healthPct = target->GetHealthPct();
    uint32 maxHealth = target->GetMaxHealth();

    // Estimate DPS from damage history
    float dps = 1000.0f;  // Default 1000 DPS
    if (!_damageHistory.empty())
    {
        // Calculate average DPS from history
        float totalDamage = 0.0f;
        uint32 timeSpan = 0;

        if (_damageHistory.size() >= 2)
        {
            auto newest = _damageHistory.back();
            auto oldest = _damageHistory.front();
            timeSpan = newest.timestamp - oldest.timestamp;

            if (timeSpan > 0)
            {
                // Would calculate actual damage here
                dps = totalDamage / (timeSpan / 1000.0f);
            }
        }
    }

    uint32 remainingHealth = uint32(maxHealth * healthPct / 100.0f);
    return uint32(remainingHealth / dps * 1000);  // Return milliseconds
}

float CooldownStackingOptimizer::ApplyDiminishingReturns(float baseMultiplier, uint32 stackCount) const
{
    if (stackCount <= 1)
        return baseMultiplier;

    // Apply 10% reduction per additional stack
    float reduction = 1.0f - (0.1f * (stackCount - 1));
    reduction = ::std::max(0.5f, reduction);  // Minimum 50% effectiveness

    return baseMultiplier * reduction;
}

// ============================================================================
// STATIC INITIALIZATION
// ============================================================================

void CooldownStackingOptimizer::InitializeDefaults()
{
    // Initialize common cooldowns
    // These would be populated from DBC data in production

    // Example structure for major DPS cooldowns
    CooldownData recklessness;
    recklessness.spellId = RECKLESSNESS;
    recklessness.category = MAJOR_DPS;
    recklessness.cooldownMs = 90000;  // 90 seconds
    recklessness.durationMs = 10000;   // 10 seconds
    recklessness.damageIncrease = 0.20f;  // 20% damage
    recklessness.critIncrease = 0.20f;    // 20% crit
    recklessness.stacksWithOthers = true;
    s_defaultCooldowns[RECKLESSNESS] = recklessness;

    // More cooldowns would be initialized here...
}

// ============================================================================
// CLASS-SPECIFIC INITIALIZATION
// ============================================================================

void CooldownStackingOptimizer::InitializeWarriorCooldowns()
{
    // Warrior-specific cooldown data
    if (_bot->HasSpell(RECKLESSNESS))
    {
        CooldownData& reck = _cooldowns[RECKLESSNESS];
        reck.spellId = RECKLESSNESS;
        reck.category = MAJOR_DPS;
        reck.cooldownMs = 90000;
        reck.durationMs = 10000;
        reck.damageIncrease = 0.20f;
        reck.critIncrease = 0.20f;
        reck.stacksWithOthers = true;
    }

    if (_bot->HasSpell(AVATAR))
    {
        CooldownData& avatar = _cooldowns[AVATAR];
        avatar.spellId = AVATAR;
        avatar.category = MAJOR_DPS;
        avatar.cooldownMs = 90000;
        avatar.durationMs = 20000;
        avatar.damageIncrease = 0.20f;
        avatar.stacksWithOthers = true;
    }
}

void CooldownStackingOptimizer::InitializePaladinCooldowns()
{
    if (_bot->HasSpell(AVENGING_WRATH))
    {
        CooldownData& wings = _cooldowns[AVENGING_WRATH];
        wings.spellId = AVENGING_WRATH;
        wings.category = MAJOR_DPS;
        wings.cooldownMs = 120000;
        wings.durationMs = 20000;
        wings.damageIncrease = 0.35f;
        wings.stacksWithOthers = true;
    }
}

void CooldownStackingOptimizer::InitializeHunterCooldowns()
{
    if (_bot->HasSpell(BESTIAL_WRATH))
    {
        CooldownData& bw = _cooldowns[BESTIAL_WRATH];
        bw.spellId = BESTIAL_WRATH;
        bw.category = MAJOR_DPS;
        bw.cooldownMs = 90000;
        bw.durationMs = 15000;
        bw.damageIncrease = 0.25f;
        bw.stacksWithOthers = true;
    }
}

void CooldownStackingOptimizer::InitializeRogueCooldowns()
{
    if (_bot->HasSpell(SHADOW_BLADES))
    {
        CooldownData& sb = _cooldowns[SHADOW_BLADES];
        sb.spellId = SHADOW_BLADES;
        sb.category = MAJOR_DPS;
        sb.cooldownMs = 180000;
        sb.durationMs = 15000;
        sb.damageIncrease = 0.30f;
        sb.stacksWithOthers = true;
    }
}

void CooldownStackingOptimizer::InitializePriestCooldowns()
{
    if (_bot->HasSpell(POWER_INFUSION))
    {
        CooldownData& pi = _cooldowns[POWER_INFUSION];
        pi.spellId = POWER_INFUSION;
        pi.category = MAJOR_DPS;
        pi.cooldownMs = 120000;
        pi.durationMs = 15000;
        pi.hasteIncrease = 0.40f;
        pi.stacksWithOthers = true;
    }
}

void CooldownStackingOptimizer::InitializeShamanCooldowns()
{
    if (_bot->HasSpell(ASCENDANCE))
    {
        CooldownData& asc = _cooldowns[ASCENDANCE];
        asc.spellId = ASCENDANCE;
        asc.category = MAJOR_DPS;
        asc.cooldownMs = 180000;
        asc.durationMs = 15000;
        asc.damageIncrease = 0.30f;
        asc.stacksWithOthers = true;
    }
}

void CooldownStackingOptimizer::InitializeMageCooldowns()
{
    if (_bot->HasSpell(ARCANE_POWER))
    {
        CooldownData& ap = _cooldowns[ARCANE_POWER];
        ap.spellId = ARCANE_POWER;
        ap.category = MAJOR_DPS;
        ap.cooldownMs = 90000;
        ap.durationMs = 10000;
        ap.damageIncrease = 0.30f;
        ap.stacksWithOthers = true;
    }

    if (_bot->HasSpell(ICY_VEINS))
    {
        CooldownData& iv = _cooldowns[ICY_VEINS];
        iv.spellId = ICY_VEINS;
        iv.category = MAJOR_DPS;
        iv.cooldownMs = 180000;
        iv.durationMs = 20000;
        iv.hasteIncrease = 0.30f;
        iv.stacksWithOthers = true;
    }

    if (_bot->HasSpell(COMBUSTION))
    {
        CooldownData& comb = _cooldowns[COMBUSTION];
        comb.spellId = COMBUSTION;
        comb.category = MAJOR_DPS;
        comb.cooldownMs = 120000;
        comb.durationMs = 10000;
        comb.critIncrease = 1.00f;  // 100% crit
        comb.stacksWithOthers = true;
    }
}

void CooldownStackingOptimizer::InitializeWarlockCooldowns()
{
    if (_bot->HasSpell(DARK_SOUL_INSTABILITY))
    {
        CooldownData& ds = _cooldowns[DARK_SOUL_INSTABILITY];
        ds.spellId = DARK_SOUL_INSTABILITY;
        ds.category = MAJOR_DPS;
        ds.cooldownMs = 120000;
        ds.durationMs = 20000;
        ds.damageIncrease = 0.30f;
        ds.critIncrease = 0.30f;
        ds.stacksWithOthers = true;
    }
}

void CooldownStackingOptimizer::InitializeDruidCooldowns()
{
    if (_bot->HasSpell(CELESTIAL_ALIGNMENT))
    {
        CooldownData& ca = _cooldowns[CELESTIAL_ALIGNMENT];
        ca.spellId = CELESTIAL_ALIGNMENT;
        ca.category = MAJOR_DPS;
        ca.cooldownMs = 180000;
        ca.durationMs = 15000;
        ca.damageIncrease = 0.30f;
        ca.hasteIncrease = 0.15f;
        ca.stacksWithOthers = true;
    }

    if (_bot->HasSpell(BERSERK))
    {
        CooldownData& bers = _cooldowns[BERSERK];
        bers.spellId = BERSERK;
        bers.category = MAJOR_DPS;
        bers.cooldownMs = 180000;
        bers.durationMs = 15000;
        bers.damageIncrease = 0.20f;
        bers.stacksWithOthers = true;
    }
}

void CooldownStackingOptimizer::InitializeDeathKnightCooldowns()
{
    if (_bot->HasSpell(ARMY_OF_THE_DEAD))
    {
        CooldownData& army = _cooldowns[ARMY_OF_THE_DEAD];
        army.spellId = ARMY_OF_THE_DEAD;
        army.category = MAJOR_DPS;
        army.cooldownMs = 600000;  // 10 minutes
        army.durationMs = 40000;
        army.damageIncrease = 0.50f;  // Pet damage
        army.stacksWithOthers = true;
    }

    if (_bot->HasSpell(UNHOLY_FRENZY))
    {
        CooldownData& uf = _cooldowns[UNHOLY_FRENZY];
        uf.spellId = UNHOLY_FRENZY;
        uf.category = MAJOR_DPS;
        uf.cooldownMs = 75000;
        uf.durationMs = 12000;
        uf.hasteIncrease = 1.00f;  // 100% attack speed
        uf.stacksWithOthers = true;
    }
}

void CooldownStackingOptimizer::InitializeMonkCooldowns()
{
    // WoW 12.0 Monk cooldowns - full spec support

    // ========== BREWMASTER (Tank) ==========
    // Invoke Niuzao, the Black Ox - Major defensive/DPS cooldown
    if (_bot->HasSpell(132578))
    {
        CooldownData& niuzao = _cooldowns[132578];
        niuzao.spellId = 132578;
        niuzao.category = MAJOR_DPS;
        niuzao.cooldownMs = 180000; // 3 minutes
        niuzao.durationMs = 25000;  // 25 seconds
        niuzao.damageIncrease = 0.25f;
        niuzao.stacksWithOthers = true;
    }

    // Weapons of Order - Kyrian Covenant ability (still available)
    if (_bot->HasSpell(387184))
    {
        CooldownData& woo = _cooldowns[387184];
        woo.spellId = 387184;
        woo.category = MAJOR_DPS;
        woo.cooldownMs = 120000; // 2 minutes
        woo.durationMs = 30000;  // 30 seconds
        woo.damageIncrease = 0.10f;
        woo.masteryIncrease = 0.08f;
        woo.stacksWithOthers = true;
    }

    // Fortifying Brew - Major defensive
    if (_bot->HasSpell(115203))
    {
        CooldownData& fb = _cooldowns[115203];
        fb.spellId = 115203;
        fb.category = DEFENSIVE_CD;
        fb.cooldownMs = 180000; // 3 minutes (reduced by talents)
        fb.durationMs = 15000;  // 15 seconds
        fb.damageIncrease = 0.0f;
        fb.stacksWithOthers = false;
    }

    // ========== WINDWALKER (DPS) ==========
    // Storm, Earth, and Fire - Major DPS cooldown
    if (_bot->HasSpell(137639))
    {
        CooldownData& sef = _cooldowns[137639];
        sef.spellId = 137639;
        sef.category = MAJOR_DPS;
        sef.cooldownMs = 90000;  // 1.5 minutes (2 charges)
        sef.durationMs = 15000;  // 15 seconds
        sef.damageIncrease = 0.45f; // 45% more damage with clones
        sef.stacksWithOthers = true;
    }

    // Invoke Xuen, the White Tiger - Major burst
    if (_bot->HasSpell(123904))
    {
        CooldownData& xuen = _cooldowns[123904];
        xuen.spellId = 123904;
        xuen.category = MAJOR_DPS;
        xuen.cooldownMs = 120000; // 2 minutes
        xuen.durationMs = 20000;  // 20 seconds
        xuen.damageIncrease = 0.30f;
        xuen.stacksWithOthers = true;
    }

    // Touch of Death - Execute ability
    if (_bot->HasSpell(322109))
    {
        CooldownData& tod = _cooldowns[322109];
        tod.spellId = 322109;
        tod.category = BURST;
        tod.cooldownMs = 180000; // 3 minutes
        tod.durationMs = 0;      // Instant
        tod.damageIncrease = 0.0f; // Special damage
        tod.stacksWithOthers = true;
    }

    // Serenity - Alternative to SEF, burst window
    if (_bot->HasSpell(152173))
    {
        CooldownData& serenity = _cooldowns[152173];
        serenity.spellId = 152173;
        serenity.category = MAJOR_DPS;
        serenity.cooldownMs = 90000;  // 1.5 minutes
        serenity.durationMs = 12000;  // 12 seconds
        serenity.damageIncrease = 0.20f;
        serenity.stacksWithOthers = true;
    }

    // ========== MISTWEAVER (Healer) ==========
    // Invoke Yu'lon, the Jade Serpent - Major healing cooldown
    if (_bot->HasSpell(322118))
    {
        CooldownData& yulon = _cooldowns[322118];
        yulon.spellId = 322118;
        yulon.category = UTILITY;
        yulon.cooldownMs = 180000; // 3 minutes
        yulon.durationMs = 25000;  // 25 seconds
        yulon.healingIncrease = 0.25f;
        yulon.stacksWithOthers = true;
    }

    // Revival - Raid healing cooldown
    if (_bot->HasSpell(115310))
    {
        CooldownData& revival = _cooldowns[115310];
        revival.spellId = 115310;
        revival.category = UTILITY;
        revival.cooldownMs = 180000; // 3 minutes
        revival.durationMs = 0;      // Instant
        revival.healingIncrease = 0.0f; // Big burst heal
        revival.stacksWithOthers = true;
    }

    // Thunder Focus Tea - Short burst
    if (_bot->HasSpell(116680))
    {
        CooldownData& tft = _cooldowns[116680];
        tft.spellId = 116680;
        tft.category = MINOR_DPS;
        tft.cooldownMs = 30000;  // 30 seconds
        tft.durationMs = 0;      // Next spell empowerment
        tft.stacksWithOthers = true;
    }

    // ========== SHARED ==========
    // Touch of Karma - Damage absorption/reflection
    if (_bot->HasSpell(122470))
    {
        CooldownData& tok = _cooldowns[122470];
        tok.spellId = 122470;
        tok.category = DEFENSIVE_CD;
        tok.cooldownMs = 90000;  // 1.5 minutes
        tok.durationMs = 10000;  // 10 seconds
        tok.stacksWithOthers = true;
    }

    // Diffuse Magic - Magic damage reduction
    if (_bot->HasSpell(122783))
    {
        CooldownData& dm = _cooldowns[122783];
        dm.spellId = 122783;
        dm.category = DEFENSIVE_CD;
        dm.cooldownMs = 90000;  // 1.5 minutes
        dm.durationMs = 6000;   // 6 seconds
        dm.stacksWithOthers = true;
    }
}

void CooldownStackingOptimizer::InitializeDemonHunterCooldowns()
{
    // WoW 12.0 Demon Hunter cooldowns - full spec support

    // ========== HAVOC (DPS) ==========
    // Metamorphosis - Major DPS cooldown
    if (_bot->HasSpell(191427))
    {
        CooldownData& meta = _cooldowns[191427];
        meta.spellId = 191427;
        meta.category = MAJOR_DPS;
        meta.cooldownMs = 240000; // 4 minutes (reduced by talents)
        meta.durationMs = 30000;  // 30 seconds
        meta.damageIncrease = 0.25f;
        meta.hasteIncrease = 0.25f;
        meta.stacksWithOthers = true;
    }

    // Eye Beam - Major channeled damage (short CD)
    if (_bot->HasSpell(198013))
    {
        CooldownData& eb = _cooldowns[198013];
        eb.spellId = 198013;
        eb.category = BURST;
        eb.cooldownMs = 30000;   // 30 seconds
        eb.durationMs = 2000;    // 2 second channel
        eb.damageIncrease = 0.0f; // Direct damage
        eb.stacksWithOthers = true;
    }

    // Essence Break - Damage amplification window
    if (_bot->HasSpell(258860))
    {
        CooldownData& essbreak = _cooldowns[258860];
        essbreak.spellId = 258860;
        essbreak.category = BURST;
        essbreak.cooldownMs = 40000; // 40 seconds
        essbreak.durationMs = 4000;  // 4 second debuff
        essbreak.damageIncrease = 0.40f; // Chaos damage amplified
        essbreak.stacksWithOthers = true;
    }

    // Fel Barrage - Talent burst ability
    if (_bot->HasSpell(258925))
    {
        CooldownData& fb = _cooldowns[258925];
        fb.spellId = 258925;
        fb.category = BURST;
        fb.cooldownMs = 90000;  // 1.5 minutes
        fb.durationMs = 8000;   // 8 seconds
        fb.damageIncrease = 0.0f; // Direct damage
        fb.stacksWithOthers = true;
    }

    // The Hunt - Major damage + movement
    if (_bot->HasSpell(370965))
    {
        CooldownData& hunt = _cooldowns[370965];
        hunt.spellId = 370965;
        hunt.category = MAJOR_DPS;
        hunt.cooldownMs = 90000;  // 1.5 minutes
        hunt.durationMs = 0;      // Instant
        hunt.damageIncrease = 0.0f; // Big burst damage
        hunt.stacksWithOthers = true;
    }

    // ========== VENGEANCE (Tank) ==========
    // Metamorphosis (Vengeance version) - Defensive
    if (_bot->HasSpell(187827))
    {
        CooldownData& metaVeng = _cooldowns[187827];
        metaVeng.spellId = 187827;
        metaVeng.category = DEFENSIVE_CD;
        metaVeng.cooldownMs = 180000; // 3 minutes
        metaVeng.durationMs = 15000;  // 15 seconds
        metaVeng.damageIncrease = 0.20f;
        metaVeng.stacksWithOthers = true;
    }

    // Fiery Brand - Damage reduction on target
    if (_bot->HasSpell(204021))
    {
        CooldownData& brand = _cooldowns[204021];
        brand.spellId = 204021;
        brand.category = DEFENSIVE_CD;
        brand.cooldownMs = 60000;  // 1 minute
        brand.durationMs = 10000;  // 10 seconds
        brand.stacksWithOthers = true;
    }

    // Fel Devastation - Channeled damage + healing
    if (_bot->HasSpell(212084))
    {
        CooldownData& fd = _cooldowns[212084];
        fd.spellId = 212084;
        fd.category = MINOR_DPS;
        fd.cooldownMs = 60000;  // 1 minute (reduced by talents)
        fd.durationMs = 2000;   // 2 second channel
        fd.damageIncrease = 0.0f;
        fd.healingIncrease = 0.25f; // Self healing
        fd.stacksWithOthers = true;
    }

    // Spirit Bomb - AoE damage + self healing
    if (_bot->HasSpell(247454))
    {
        CooldownData& sb = _cooldowns[247454];
        sb.spellId = 247454;
        sb.category = BURST;
        sb.cooldownMs = 0;       // No cooldown, resource-gated
        sb.durationMs = 0;       // Instant
        sb.damageIncrease = 0.0f;
        sb.stacksWithOthers = true;
    }

    // ========== SHARED ==========
    // Darkness - Raid defensive
    if (_bot->HasSpell(196718))
    {
        CooldownData& darkness = _cooldowns[196718];
        darkness.spellId = 196718;
        darkness.category = UTILITY;
        darkness.cooldownMs = 300000; // 5 minutes
        darkness.durationMs = 8000;   // 8 seconds
        darkness.stacksWithOthers = true;
    }

    // Blur - Personal defensive
    if (_bot->HasSpell(198589))
    {
        CooldownData& blur = _cooldowns[198589];
        blur.spellId = 198589;
        blur.category = DEFENSIVE_CD;
        blur.cooldownMs = 60000;  // 1 minute
        blur.durationMs = 10000;  // 10 seconds
        blur.stacksWithOthers = false;
    }

    // Netherwalk - Immunity
    if (_bot->HasSpell(196555))
    {
        CooldownData& nw = _cooldowns[196555];
        nw.spellId = 196555;
        nw.category = DEFENSIVE_CD;
        nw.cooldownMs = 180000; // 3 minutes
        nw.durationMs = 6000;   // 6 seconds
        nw.stacksWithOthers = false;
    }

    // Chaos Nova - Stun + damage
    if (_bot->HasSpell(179057))
    {
        CooldownData& cn = _cooldowns[179057];
        cn.spellId = 179057;
        cn.category = UTILITY;
        cn.cooldownMs = 60000;  // 1 minute
        cn.durationMs = 2000;   // 2 second stun
        cn.stacksWithOthers = true;
    }

    // Immolation Aura - Regular damage ability
    if (_bot->HasSpell(258920))
    {
        CooldownData& immo = _cooldowns[258920];
        immo.spellId = 258920;
        immo.category = MINOR_DPS;
        immo.cooldownMs = 30000;  // 30 seconds
        immo.durationMs = 6000;   // 6 seconds
        immo.damageIncrease = 0.0f;
        immo.stacksWithOthers = true;
    }
}

} // namespace Playerbot