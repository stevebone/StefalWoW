/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "ProcExpiryMonitor.h"
#include "Player.h"
#include "SpellAuras.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "Log.h"
#include "World.h"

#include <algorithm>
#include <sstream>

namespace Playerbot
{

// ============================================================================
// CONSTRUCTOR
// ============================================================================

ProcExpiryMonitor::ProcExpiryMonitor(Player* bot)
    : _bot(bot)
{
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void ProcExpiryMonitor::Initialize()
{
    if (!_bot)
        return;

    BuildProcDatabase();
    _initialized = true;

    TC_LOG_DEBUG("module.playerbot", "ProcExpiryMonitor [{}]: Initialized with {} tracked procs "
        "for class {}",
        _bot->GetName(), _trackedProcs.size(), static_cast<uint32>(_bot->GetClass()));
}

void ProcExpiryMonitor::OnSpecChanged()
{
    _trackedProcs.clear();
    _activeProcs.clear();
    _previouslyActive.clear();
    _initialized = false;
    Initialize();
}

// ============================================================================
// UPDATE
// ============================================================================

void ProcExpiryMonitor::Update(uint32 diff)
{
    if (!_initialized || !_bot || !_bot->IsInWorld() || !_bot->IsAlive())
        return;

    // Only scan during combat
    if (!_bot->IsInCombat())
    {
        if (!_activeProcs.empty())
            _activeProcs.clear();
        return;
    }

    _updateTimer += diff;
    if (_updateTimer < UPDATE_INTERVAL_MS)
        return;
    _updateTimer = 0;

    ScanActiveProcs();
}

// ============================================================================
// QUERIES
// ============================================================================

ActiveProcState const* ProcExpiryMonitor::GetHighestUrgencyProc() const
{
    if (_activeProcs.empty())
        return nullptr;

    // Active procs are sorted by urgency (highest first)
    ActiveProcState const& highest = _activeProcs.front();
    if (highest.urgencyLevel >= ProcUrgency::MODERATE)
        return &highest;

    return nullptr;
}

bool ProcExpiryMonitor::IsProcUrgent(uint32 procAuraId) const
{
    for (auto const& proc : _activeProcs)
    {
        if (proc.procAuraId == procAuraId)
            return proc.urgencyLevel >= ProcUrgency::HIGH;
    }
    return false;
}

ProcUrgency ProcExpiryMonitor::GetProcUrgency(uint32 procAuraId) const
{
    for (auto const& proc : _activeProcs)
    {
        if (proc.procAuraId == procAuraId)
            return proc.urgencyLevel;
    }
    return ProcUrgency::NONE;
}

uint32 ProcExpiryMonitor::GetConsumeSpellForProc(uint32 procAuraId) const
{
    for (auto const& proc : _activeProcs)
    {
        if (proc.procAuraId == procAuraId)
            return proc.consumeSpellId;
    }
    return 0;
}

bool ProcExpiryMonitor::HasUrgentProc() const
{
    for (auto const& proc : _activeProcs)
    {
        if (proc.urgencyLevel >= ProcUrgency::HIGH)
            return true;
    }
    return false;
}

// ============================================================================
// STATISTICS
// ============================================================================

::std::string ProcExpiryMonitor::FormatStatus() const
{
    ::std::ostringstream oss;
    oss << "=== Proc Expiry Monitor [" << _bot->GetName() << "] ===\n";
    oss << "  Tracked: " << _trackedProcs.size() << " procs\n";
    oss << "  Active:  " << _activeProcs.size() << " procs\n";
    oss << "  Stats: detected=" << _stats.procsDetected
        << " consumed=" << _stats.procsConsumedInTime
        << " expired=" << _stats.procsExpiredUnused
        << " warnings=" << _stats.urgentWarningsIssued << "\n";

    for (auto const& proc : _activeProcs)
    {
        oss << "  [" << proc.name << "] remaining="
            << proc.remainingMs << "ms urgency=";
        switch (proc.urgencyLevel)
        {
            case ProcUrgency::NONE:     oss << "NONE"; break;
            case ProcUrgency::LOW:      oss << "LOW"; break;
            case ProcUrgency::MODERATE: oss << "MODERATE"; break;
            case ProcUrgency::HIGH:     oss << "HIGH"; break;
            case ProcUrgency::CRITICAL: oss << "CRITICAL"; break;
        }
        if (proc.maxStacks > 1)
            oss << " stacks=" << static_cast<uint32>(proc.currentStacks)
                << "/" << static_cast<uint32>(proc.maxStacks);
        oss << "\n";
    }

    return oss.str();
}

// ============================================================================
// INTERNAL: BUILD PROC DATABASE
// ============================================================================

void ProcExpiryMonitor::BuildProcDatabase()
{
    _trackedProcs.clear();

    uint8 botClass = _bot->GetClass();

    // ========================================================================
    // WARRIOR PROCS
    // ========================================================================
    if (botClass == CLASS_WARRIOR)
    {
        // Sudden Death - free Execute
        _trackedProcs.push_back({280776, 163201, "Sudden Death", ProcCategory::EXECUTE_UNLOCK, 10000, 1, 0, 0.20f, CLASS_WARRIOR, 0});
        // Overpower proc (Tactician)
        _trackedProcs.push_back({386164, 7384, "Overpower Ready", ProcCategory::DAMAGE_BOOST, 15000, 1, 0, 0.25f, CLASS_WARRIOR, 0});
    }

    // ========================================================================
    // PALADIN PROCS
    // ========================================================================
    if (botClass == CLASS_PALADIN)
    {
        // Art of War - free Flash of Light / Verdict
        _trackedProcs.push_back({281178, 85256, "Art of War", ProcCategory::FREE_CAST, 15000, 1, 0, 0.20f, CLASS_PALADIN, 0});
        // Divine Purpose - free Holy Power spender
        _trackedProcs.push_back({408458, 0, "Divine Purpose", ProcCategory::FREE_CAST, 12000, 1, 0, 0.20f, CLASS_PALADIN, 0});
        // Infusion of Light - faster Holy Light
        _trackedProcs.push_back({54149, 82326, "Infusion of Light", ProcCategory::INSTANT_CAST, 15000, 2, 0, 0.20f, CLASS_PALADIN, 0});
    }

    // ========================================================================
    // HUNTER PROCS
    // ========================================================================
    if (botClass == CLASS_HUNTER)
    {
        // Lock and Load - free Aimed Shot
        _trackedProcs.push_back({194594, 19434, "Lock and Load", ProcCategory::FREE_CAST, 15000, 2, 0, 0.20f, CLASS_HUNTER, 0});
        // Precise Shots - empowered Arcane Shot / Multi-Shot
        _trackedProcs.push_back({260242, 185358, "Precise Shots", ProcCategory::DAMAGE_BOOST, 15000, 2, 0, 0.20f, CLASS_HUNTER, 0});
    }

    // ========================================================================
    // ROGUE PROCS
    // ========================================================================
    if (botClass == CLASS_ROGUE)
    {
        // Opportunity - free Sinister Strike
        _trackedProcs.push_back({195627, 185763, "Opportunity", ProcCategory::DAMAGE_BOOST, 10000, 1, 0, 0.20f, CLASS_ROGUE, 0});
    }

    // ========================================================================
    // PRIEST PROCS
    // ========================================================================
    if (botClass == CLASS_PRIEST)
    {
        // Surge of Light - free instant Flash Heal
        _trackedProcs.push_back({114255, 2061, "Surge of Light", ProcCategory::HEALING_BOOST, 20000, 2, 0, 0.20f, CLASS_PRIEST, 0});
        // Shadowy Insight - instant Mind Blast
        _trackedProcs.push_back({375981, 8092, "Shadowy Insight", ProcCategory::INSTANT_CAST, 12000, 1, 0, 0.20f, CLASS_PRIEST, 0});
    }

    // ========================================================================
    // DEATH KNIGHT PROCS
    // ========================================================================
    if (botClass == CLASS_DEATH_KNIGHT)
    {
        // Rime - free Howling Blast
        _trackedProcs.push_back({59052, 49184, "Rime", ProcCategory::FREE_CAST, 15000, 1, 0, 0.20f, CLASS_DEATH_KNIGHT, 0});
        // Killing Machine - guaranteed crit Obliterate
        _trackedProcs.push_back({51124, 49020, "Killing Machine", ProcCategory::DAMAGE_BOOST, 10000, 1, 0, 0.15f, CLASS_DEATH_KNIGHT, 0});
        // Sudden Doom - free Death Coil
        _trackedProcs.push_back({81340, 47541, "Sudden Doom", ProcCategory::FREE_CAST, 10000, 1, 0, 0.20f, CLASS_DEATH_KNIGHT, 0});
    }

    // ========================================================================
    // SHAMAN PROCS
    // ========================================================================
    if (botClass == CLASS_SHAMAN)
    {
        // Maelstrom Weapon - stacking, consume at 5+ stacks
        _trackedProcs.push_back({344179, 188196, "Maelstrom Weapon", ProcCategory::STACKING, 30000, 10, 5, 0.15f, CLASS_SHAMAN, 0});
        // Lava Surge - instant Lava Burst
        _trackedProcs.push_back({77762, 51505, "Lava Surge", ProcCategory::INSTANT_CAST, 10000, 1, 0, 0.20f, CLASS_SHAMAN, 0});
        // Tidal Waves - faster Healing Wave
        _trackedProcs.push_back({53390, 77472, "Tidal Waves", ProcCategory::INSTANT_CAST, 15000, 2, 0, 0.25f, CLASS_SHAMAN, 0});
    }

    // ========================================================================
    // MAGE PROCS
    // ========================================================================
    if (botClass == CLASS_MAGE)
    {
        // Hot Streak - instant Pyroblast
        _trackedProcs.push_back({48108, 11366, "Hot Streak", ProcCategory::INSTANT_CAST, 15000, 1, 0, 0.15f, CLASS_MAGE, 0});
        // Fingers of Frost - treat target as frozen
        _trackedProcs.push_back({44544, 30455, "Fingers of Frost", ProcCategory::DAMAGE_BOOST, 15000, 2, 0, 0.20f, CLASS_MAGE, 0});
        // Brain Freeze - free Flurry
        _trackedProcs.push_back({190446, 44614, "Brain Freeze", ProcCategory::FREE_CAST, 15000, 1, 0, 0.20f, CLASS_MAGE, 0});
        // Clearcasting (Arcane) - free Arcane Missiles
        _trackedProcs.push_back({263725, 5143, "Clearcasting", ProcCategory::FREE_CAST, 15000, 3, 0, 0.20f, CLASS_MAGE, 0});
    }

    // ========================================================================
    // WARLOCK PROCS
    // ========================================================================
    if (botClass == CLASS_WARLOCK)
    {
        // Nightfall - instant Shadow Bolt
        _trackedProcs.push_back({264571, 686, "Nightfall", ProcCategory::INSTANT_CAST, 12000, 1, 0, 0.20f, CLASS_WARLOCK, 0});
        // Backdraft - faster Incinerate/Chaos Bolt
        _trackedProcs.push_back({117828, 29722, "Backdraft", ProcCategory::INSTANT_CAST, 10000, 2, 0, 0.25f, CLASS_WARLOCK, 0});
        // Decimation - empowered Soul Fire
        _trackedProcs.push_back({457555, 6353, "Decimation", ProcCategory::DAMAGE_BOOST, 10000, 1, 0, 0.20f, CLASS_WARLOCK, 0});
    }

    // ========================================================================
    // MONK PROCS
    // ========================================================================
    if (botClass == CLASS_MONK)
    {
        // Teachings of the Monastery - extra Tiger Palm hits
        _trackedProcs.push_back({202090, 100780, "Teachings of the Monastery", ProcCategory::STACKING, 20000, 4, 3, 0.20f, CLASS_MONK, 0});
        // Vitality Conduit proc / Thunder Focus Tea (not really a proc but tracks)
    }

    // ========================================================================
    // DRUID PROCS
    // ========================================================================
    if (botClass == CLASS_DRUID)
    {
        // Clearcasting (Omen of Clarity) - free ability
        _trackedProcs.push_back({16870, 0, "Clearcasting", ProcCategory::FREE_CAST, 15000, 1, 0, 0.20f, CLASS_DRUID, 0});
        // Predatory Swiftness - instant cast nature spell
        _trackedProcs.push_back({69369, 8936, "Predatory Swiftness", ProcCategory::INSTANT_CAST, 12000, 1, 0, 0.20f, CLASS_DRUID, 0});
        // Eclipse (Solar) - empowered Wrath
        _trackedProcs.push_back({48517, 190984, "Eclipse (Solar)", ProcCategory::DAMAGE_BOOST, 15000, 1, 0, 0.20f, CLASS_DRUID, 0});
        // Eclipse (Lunar) - empowered Starfire
        _trackedProcs.push_back({48518, 194153, "Eclipse (Lunar)", ProcCategory::DAMAGE_BOOST, 15000, 1, 0, 0.20f, CLASS_DRUID, 0});
    }

    // ========================================================================
    // DEMON HUNTER PROCS
    // ========================================================================
    if (botClass == CLASS_DEMON_HUNTER)
    {
        // Chaos Theory - empowered Blade Dance
        _trackedProcs.push_back({390195, 188499, "Chaos Theory", ProcCategory::DAMAGE_BOOST, 8000, 1, 0, 0.20f, CLASS_DEMON_HUNTER, 0});
    }

    // ========================================================================
    // EVOKER PROCS
    // ========================================================================
    if (botClass == CLASS_EVOKER)
    {
        // Essence Burst - free Disintegrate/Living Flame
        _trackedProcs.push_back({359618, 356995, "Essence Burst", ProcCategory::FREE_CAST, 15000, 2, 0, 0.20f, CLASS_EVOKER, 0});
        // Burnout - empowered Living Flame
        _trackedProcs.push_back({375802, 361469, "Burnout", ProcCategory::DAMAGE_BOOST, 15000, 2, 0, 0.20f, CLASS_EVOKER, 0});
    }

    TC_LOG_DEBUG("module.playerbot", "ProcExpiryMonitor: Built proc database with {} entries "
        "for class {}",
        _trackedProcs.size(), static_cast<uint32>(botClass));
}

// ============================================================================
// INTERNAL: SCAN ACTIVE PROCS
// ============================================================================

void ProcExpiryMonitor::ScanActiveProcs()
{
    // Save previous state for expiry detection
    ::std::unordered_map<uint32, bool> currentlyActive;

    _activeProcs.clear();

    for (auto const& tracked : _trackedProcs)
    {
        Aura* aura = _bot->GetAura(tracked.procAuraId);
        if (!aura)
            continue;

        // Found an active proc
        currentlyActive[tracked.procAuraId] = true;

        ActiveProcState state;
        state.procAuraId = tracked.procAuraId;
        state.consumeSpellId = tracked.consumeSpellId;
        state.name = tracked.name;
        state.category = tracked.category;
        state.remainingMs = aura->GetDuration();
        state.totalDurationMs = aura->GetMaxDuration();
        state.currentStacks = aura->GetStackAmount();
        state.maxStacks = tracked.maxStacks;
        state.consumeAtStacks = tracked.consumeAtStacks;

        // Calculate urgency
        state.urgencyScore = CalculateUrgencyScore(
            state.remainingMs, state.totalDurationMs,
            state.currentStacks, state.consumeAtStacks,
            tracked.urgencyThreshold);
        state.urgencyLevel = ScoreToUrgency(state.urgencyScore);

        _activeProcs.push_back(::std::move(state));

        // Track new proc detection
        if (_previouslyActive.find(tracked.procAuraId) == _previouslyActive.end())
        {
            _stats.procsDetected++;
            TC_LOG_TRACE("module.playerbot", "ProcExpiryMonitor [{}]: Detected proc '{}' "
                "(duration: {}ms, stacks: {})",
                _bot->GetName(), tracked.name,
                aura->GetDuration(), aura->GetStackAmount());
        }
    }

    // Check for procs that expired (were previously active, now gone)
    for (auto const& [procId, wasActive] : _previouslyActive)
    {
        if (currentlyActive.find(procId) == currentlyActive.end())
        {
            // Proc expired or was consumed - we can't distinguish perfectly,
            // but if it was at HIGH urgency last tick, it likely expired
            _stats.procsConsumedInTime++;
        }
    }

    _previouslyActive = ::std::move(currentlyActive);

    // Sort by urgency (highest first)
    ::std::sort(_activeProcs.begin(), _activeProcs.end(),
        [](ActiveProcState const& a, ActiveProcState const& b)
        {
            return a.urgencyScore > b.urgencyScore;
        });

    // Issue warnings for urgent procs
    for (auto const& proc : _activeProcs)
    {
        if (proc.urgencyLevel == ProcUrgency::CRITICAL)
        {
            _stats.criticalWarningsIssued++;
            TC_LOG_DEBUG("module.playerbot", "ProcExpiryMonitor [{}]: CRITICAL - '{}' "
                "expiring in {}ms! Consume with spell {}",
                _bot->GetName(), proc.name, proc.remainingMs, proc.consumeSpellId);
        }
        else if (proc.urgencyLevel == ProcUrgency::HIGH)
        {
            _stats.urgentWarningsIssued++;
            TC_LOG_TRACE("module.playerbot", "ProcExpiryMonitor [{}]: HIGH urgency - '{}' "
                "has {}ms remaining",
                _bot->GetName(), proc.name, proc.remainingMs);
        }
    }
}

// ============================================================================
// INTERNAL: URGENCY CALCULATION
// ============================================================================

float ProcExpiryMonitor::CalculateUrgencyScore(int32 remainingMs, int32 totalDurationMs,
                                                 uint8 currentStacks, uint8 consumeAtStacks,
                                                 float /*urgencyThreshold*/) const
{
    // Guard against division by zero
    if (totalDurationMs <= 0)
        return 0.0f;

    // Base urgency from time remaining
    float timeRatio = static_cast<float>(remainingMs) / static_cast<float>(totalDurationMs);
    float timeUrgency = 1.0f - timeRatio;  // 0.0 = full duration, 1.0 = about to expire

    // Clamp
    timeUrgency = std::clamp(timeUrgency, 0.0f, 1.0f);

    // For stacking procs, boost urgency if at max stacks
    if (consumeAtStacks > 0 && currentStacks >= consumeAtStacks)
    {
        // At or above consume threshold - urgency should be higher
        // because the proc is ready to be consumed and wasting time
        timeUrgency = std::max(timeUrgency, 0.5f);
    }

    return timeUrgency;
}

ProcUrgency ProcExpiryMonitor::ScoreToUrgency(float score)
{
    if (score >= (1.0f - URGENCY_CRITICAL_THRESHOLD))
        return ProcUrgency::CRITICAL;
    if (score >= (1.0f - URGENCY_HIGH_THRESHOLD))
        return ProcUrgency::HIGH;
    if (score >= (1.0f - URGENCY_MODERATE_THRESHOLD))
        return ProcUrgency::MODERATE;
    if (score >= (1.0f - URGENCY_LOW_THRESHOLD))
        return ProcUrgency::LOW;
    return ProcUrgency::NONE;
}

} // namespace Playerbot
