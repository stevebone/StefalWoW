/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "CombatMetricsTracker.h"
#include "Log.h"
#include "Player.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "GameTime.h"
#include "World.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cmath>

namespace Playerbot
{

// ============================================================================
// CONSTRUCTOR
// ============================================================================

CombatMetricsTracker::CombatMetricsTracker(Player* bot)
    : _bot(bot)
{
    _eventBuffer.fill(CombatEvent{});
}

// ============================================================================
// EVENT RECORDING
// ============================================================================

void CombatMetricsTracker::RecordDamage(uint32 spellId, Unit* target, uint32 amount,
                                         uint32 overkill, bool isCrit, bool isPeriodic)
{
    CombatEvent event;
    event.timestamp = GetCurrentTimeMs();
    event.spellId = spellId;
    event.amount = amount;
    event.overhealOrOverkill = overkill;
    event.targetGuid = target ? target->GetGUID() : ObjectGuid::Empty;
    event.type = CombatEventType::DAMAGE_DONE;
    event.isCrit = isCrit;
    event.isPeriodic = isPeriodic;

    RecordEvent(event);
    UpdateSpellMetrics(spellId, amount, overkill, isCrit, isPeriodic, CombatEventType::DAMAGE_DONE);

    _encounterDamage += amount;
    _encounterOverkill += overkill;
    _sessionTotalDamage += amount;
    _encounterEvents++;
    if (isCrit)
        _encounterCrits++;
}

void CombatMetricsTracker::RecordHealing(uint32 spellId, Unit* target, uint32 amount,
                                          uint32 overheal, bool isCrit, bool isPeriodic)
{
    CombatEvent event;
    event.timestamp = GetCurrentTimeMs();
    event.spellId = spellId;
    event.amount = amount;
    event.overhealOrOverkill = overheal;
    event.targetGuid = target ? target->GetGUID() : ObjectGuid::Empty;
    event.type = CombatEventType::HEALING_DONE;
    event.isCrit = isCrit;
    event.isPeriodic = isPeriodic;

    RecordEvent(event);
    UpdateSpellMetrics(spellId, amount, overheal, isCrit, isPeriodic, CombatEventType::HEALING_DONE);

    _encounterHealing += amount;
    _encounterOverheal += overheal;
    _sessionTotalHealing += amount;
    _encounterEvents++;
    if (isCrit)
        _encounterCrits++;
}

void CombatMetricsTracker::RecordDamageTaken(uint32 spellId, Unit* attacker, uint32 amount,
                                              bool isAbsorbed)
{
    CombatEvent event;
    event.timestamp = GetCurrentTimeMs();
    event.spellId = spellId;
    event.amount = amount;
    event.targetGuid = attacker ? attacker->GetGUID() : ObjectGuid::Empty;
    event.type = CombatEventType::DAMAGE_TAKEN;
    event.isAbsorbed = isAbsorbed;

    RecordEvent(event);
    UpdateSpellMetrics(spellId, amount, 0, false, false, CombatEventType::DAMAGE_TAKEN);

    _encounterDamageTaken += amount;
    _sessionTotalDamageTaken += amount;
}

void CombatMetricsTracker::RecordHealingTaken(uint32 spellId, Unit* healer, uint32 amount,
                                               uint32 overheal)
{
    CombatEvent event;
    event.timestamp = GetCurrentTimeMs();
    event.spellId = spellId;
    event.amount = amount;
    event.overhealOrOverkill = overheal;
    event.targetGuid = healer ? healer->GetGUID() : ObjectGuid::Empty;
    event.type = CombatEventType::HEALING_TAKEN;

    RecordEvent(event);
}

void CombatMetricsTracker::RecordAbsorb(uint32 spellId, Unit* target, uint32 amount)
{
    CombatEvent event;
    event.timestamp = GetCurrentTimeMs();
    event.spellId = spellId;
    event.amount = amount;
    event.targetGuid = target ? target->GetGUID() : ObjectGuid::Empty;
    event.type = CombatEventType::ABSORB_DONE;

    RecordEvent(event);
}

// ============================================================================
// CURRENT METRICS (Rolling Window)
// ============================================================================

float CombatMetricsTracker::GetCurrentDPS(uint32 windowMs) const
{
    return CalculateRateInWindow(CombatEventType::DAMAGE_DONE, windowMs);
}

float CombatMetricsTracker::GetCurrentHPS(uint32 windowMs) const
{
    return CalculateRateInWindow(CombatEventType::HEALING_DONE, windowMs);
}

float CombatMetricsTracker::GetCurrentDTPS(uint32 windowMs) const
{
    return CalculateRateInWindow(CombatEventType::DAMAGE_TAKEN, windowMs);
}

float CombatMetricsTracker::GetOverallDPS() const
{
    if (!_inCombat || _combatStartTime == 0)
    {
        // Use last encounter if available
        if (_lastEncounter.durationMs > 0)
            return _lastEncounter.dps;
        return 0.0f;
    }

    uint32 duration = GetCurrentEncounterDuration();
    if (duration == 0)
        return 0.0f;

    float seconds = static_cast<float>(duration) / 1000.0f;
    return static_cast<float>(_encounterDamage) / seconds;
}

float CombatMetricsTracker::GetOverallHPS() const
{
    if (!_inCombat || _combatStartTime == 0)
    {
        if (_lastEncounter.durationMs > 0)
            return _lastEncounter.hps;
        return 0.0f;
    }

    uint32 duration = GetCurrentEncounterDuration();
    if (duration == 0)
        return 0.0f;

    float seconds = static_cast<float>(duration) / 1000.0f;
    return static_cast<float>(_encounterHealing) / seconds;
}

float CombatMetricsTracker::GetOverallDTPS() const
{
    if (!_inCombat || _combatStartTime == 0)
    {
        if (_lastEncounter.durationMs > 0)
            return _lastEncounter.dtps;
        return 0.0f;
    }

    uint32 duration = GetCurrentEncounterDuration();
    if (duration == 0)
        return 0.0f;

    float seconds = static_cast<float>(duration) / 1000.0f;
    return static_cast<float>(_encounterDamageTaken) / seconds;
}

// ============================================================================
// SPELL BREAKDOWN
// ============================================================================

::std::vector<SpellMetrics> CombatMetricsTracker::GetDamageBySpell() const
{
    return GetSortedSpellMetrics(_damageBySpell);
}

::std::vector<SpellMetrics> CombatMetricsTracker::GetHealingBySpell() const
{
    return GetSortedSpellMetrics(_healingBySpell);
}

::std::vector<SpellMetrics> CombatMetricsTracker::GetDamageTakenBySpell() const
{
    return GetSortedSpellMetrics(_damageTakenBySpell);
}

SpellMetrics CombatMetricsTracker::GetSpellDamageMetrics(uint32 spellId) const
{
    auto it = _damageBySpell.find(spellId);
    if (it != _damageBySpell.end())
        return it->second;
    return SpellMetrics{};
}

SpellMetrics CombatMetricsTracker::GetSpellHealingMetrics(uint32 spellId) const
{
    auto it = _healingBySpell.find(spellId);
    if (it != _healingBySpell.end())
        return it->second;
    return SpellMetrics{};
}

// ============================================================================
// ENCOUNTER TRACKING
// ============================================================================

void CombatMetricsTracker::OnCombatStart()
{
    if (_inCombat)
        return;

    _inCombat = true;
    _combatStartTime = GetCurrentTimeMs();
    _combatEndTime = 0;

    // Reset encounter-specific data
    ResetEncounter();

    TC_LOG_DEBUG("module.playerbot", "CombatMetricsTracker: Combat started for bot {}",
        _bot ? _bot->GetName() : "unknown");
}

void CombatMetricsTracker::OnCombatEnd()
{
    if (!_inCombat)
        return;

    _inCombat = false;
    _combatEndTime = GetCurrentTimeMs();

    // Build encounter summary
    BuildEncounterSummary();

    // Store in history
    if (_encounterHistory.size() >= MAX_ENCOUNTER_HISTORY)
        _encounterHistory.erase(_encounterHistory.begin());
    _encounterHistory.push_back(_lastEncounter);

    TC_LOG_DEBUG("module.playerbot", "CombatMetricsTracker: Combat ended for bot {} - DPS: {:.1f}, HPS: {:.1f}, Duration: {}",
        _bot ? _bot->GetName() : "unknown",
        _lastEncounter.dps, _lastEncounter.hps,
        FormatDuration(_lastEncounter.durationMs));
}

uint32 CombatMetricsTracker::GetCurrentEncounterDuration() const
{
    if (!_inCombat || _combatStartTime == 0)
        return 0;

    uint32 now = GetCurrentTimeMs();
    if (now < _combatStartTime)
        return 0;  // Timer wrap protection

    return now - _combatStartTime;
}

// ============================================================================
// FORMATTED REPORTS
// ============================================================================

::std::string CombatMetricsTracker::FormatDPSReport() const
{
    ::std::ostringstream ss;

    if (_inCombat)
    {
        uint32 duration = GetCurrentEncounterDuration();
        ss << "--- DPS Report (Active Combat: " << FormatDuration(duration) << ") ---\n";
        ss << "Overall DPS: " << FormatNumber(static_cast<uint64>(GetOverallDPS())) << "\n";
        ss << "Current DPS (5s): " << FormatNumber(static_cast<uint64>(GetCurrentDPS())) << "\n";
        ss << "Total Damage: " << FormatNumber(_encounterDamage) << "\n";
    }
    else if (_lastEncounter.durationMs > 0)
    {
        ss << "--- DPS Report (Last Fight: " << FormatDuration(_lastEncounter.durationMs) << ") ---\n";
        ss << "DPS: " << FormatNumber(static_cast<uint64>(_lastEncounter.dps)) << "\n";
        ss << "Total Damage: " << FormatNumber(_lastEncounter.totalDamage) << "\n";
        if (_lastEncounter.totalOverkill > 0)
            ss << "Overkill: " << FormatNumber(_lastEncounter.totalOverkill) << "\n";
    }
    else
    {
        ss << "--- DPS Report ---\n";
        ss << "No combat data available.\n";
        return ss.str();
    }

    // Spell breakdown (top 10)
    auto spells = GetDamageBySpell();
    uint64 totalDmg = _inCombat ? _encounterDamage : _lastEncounter.totalDamage;

    if (!spells.empty() && totalDmg > 0)
    {
        ss << "--- Spell Breakdown ---\n";
        uint32 shown = 0;
        for (auto const& spell : spells)
        {
            if (shown >= 10)
                break;

            float pct = totalDmg > 0 ? static_cast<float>(spell.totalAmount) / static_cast<float>(totalDmg) * 100.0f : 0.0f;
            ss << "  " << GetSpellName(spell.spellId) << ": "
               << FormatNumber(spell.totalAmount) << " (" << ::std::fixed << ::std::setprecision(1) << pct << "%)"
               << " | Hits: " << spell.hitCount
               << " | Crit: " << ::std::fixed << ::std::setprecision(1) << spell.GetCritRate() << "%"
               << " | Avg: " << FormatNumber(static_cast<uint64>(spell.GetAverageHit()))
               << "\n";
            ++shown;
        }
    }

    ss << "--- Crits: " << _encounterCrits << " | Events: " << _encounterEvents << " ---";
    return ss.str();
}

::std::string CombatMetricsTracker::FormatHPSReport() const
{
    ::std::ostringstream ss;

    if (_inCombat)
    {
        uint32 duration = GetCurrentEncounterDuration();
        ss << "--- HPS Report (Active Combat: " << FormatDuration(duration) << ") ---\n";
        ss << "Overall HPS: " << FormatNumber(static_cast<uint64>(GetOverallHPS())) << "\n";
        ss << "Current HPS (5s): " << FormatNumber(static_cast<uint64>(GetCurrentHPS())) << "\n";
        ss << "Total Healing: " << FormatNumber(_encounterHealing) << "\n";
        if (_encounterOverheal > 0)
        {
            float ohPct = static_cast<float>(_encounterOverheal) /
                          static_cast<float>(_encounterHealing + _encounterOverheal) * 100.0f;
            ss << "Overhealing: " << FormatNumber(_encounterOverheal)
               << " (" << ::std::fixed << ::std::setprecision(1) << ohPct << "%)\n";
        }
    }
    else if (_lastEncounter.durationMs > 0)
    {
        ss << "--- HPS Report (Last Fight: " << FormatDuration(_lastEncounter.durationMs) << ") ---\n";
        ss << "HPS: " << FormatNumber(static_cast<uint64>(_lastEncounter.hps)) << "\n";
        ss << "Total Healing: " << FormatNumber(_lastEncounter.totalHealing) << "\n";
        if (_lastEncounter.totalOverheal > 0)
        {
            float ohPct = static_cast<float>(_lastEncounter.totalOverheal) /
                          static_cast<float>(_lastEncounter.totalHealing + _lastEncounter.totalOverheal) * 100.0f;
            ss << "Overhealing: " << FormatNumber(_lastEncounter.totalOverheal)
               << " (" << ::std::fixed << ::std::setprecision(1) << ohPct << "%)\n";
        }
    }
    else
    {
        ss << "--- HPS Report ---\n";
        ss << "No healing data available.\n";
        return ss.str();
    }

    // Spell breakdown (top 10)
    auto spells = GetHealingBySpell();
    uint64 totalHeal = _inCombat ? _encounterHealing : _lastEncounter.totalHealing;

    if (!spells.empty() && totalHeal > 0)
    {
        ss << "--- Spell Breakdown ---\n";
        uint32 shown = 0;
        for (auto const& spell : spells)
        {
            if (shown >= 10)
                break;

            float pct = totalHeal > 0 ? static_cast<float>(spell.totalAmount) / static_cast<float>(totalHeal) * 100.0f : 0.0f;
            float effPct = spell.GetEfficiency();
            ss << "  " << GetSpellName(spell.spellId) << ": "
               << FormatNumber(spell.totalAmount) << " (" << ::std::fixed << ::std::setprecision(1) << pct << "%)"
               << " | Eff: " << ::std::fixed << ::std::setprecision(1) << effPct << "%"
               << " | Crit: " << ::std::fixed << ::std::setprecision(1) << spell.GetCritRate() << "%"
               << "\n";
            ++shown;
        }
    }

    return ss.str();
}

::std::string CombatMetricsTracker::FormatDTReport() const
{
    ::std::ostringstream ss;

    if (_inCombat)
    {
        uint32 duration = GetCurrentEncounterDuration();
        ss << "--- Damage Taken Report (Active Combat: " << FormatDuration(duration) << ") ---\n";
        ss << "DTPS: " << FormatNumber(static_cast<uint64>(GetOverallDTPS())) << "\n";
        ss << "Total Damage Taken: " << FormatNumber(_encounterDamageTaken) << "\n";
    }
    else if (_lastEncounter.durationMs > 0)
    {
        ss << "--- Damage Taken Report (Last Fight: " << FormatDuration(_lastEncounter.durationMs) << ") ---\n";
        ss << "DTPS: " << FormatNumber(static_cast<uint64>(_lastEncounter.dtps)) << "\n";
        ss << "Total Damage Taken: " << FormatNumber(_lastEncounter.totalDamageTaken) << "\n";
    }
    else
    {
        ss << "--- Damage Taken Report ---\n";
        ss << "No data available.\n";
        return ss.str();
    }

    // Spell breakdown
    auto spells = GetDamageTakenBySpell();
    uint64 totalDT = _inCombat ? _encounterDamageTaken : _lastEncounter.totalDamageTaken;

    if (!spells.empty() && totalDT > 0)
    {
        ss << "--- Incoming Breakdown ---\n";
        uint32 shown = 0;
        for (auto const& spell : spells)
        {
            if (shown >= 10)
                break;

            float pct = totalDT > 0 ? static_cast<float>(spell.totalAmount) / static_cast<float>(totalDT) * 100.0f : 0.0f;
            ss << "  " << GetSpellName(spell.spellId) << ": "
               << FormatNumber(spell.totalAmount) << " (" << ::std::fixed << ::std::setprecision(1) << pct << "%)"
               << " | Hits: " << spell.hitCount
               << "\n";
            ++shown;
        }
    }

    return ss.str();
}

::std::string CombatMetricsTracker::FormatEncounterSummary() const
{
    if (_lastEncounter.durationMs == 0)
        return "No encounter data available.";

    ::std::ostringstream ss;
    ss << "=== Encounter Summary (" << FormatDuration(_lastEncounter.durationMs) << ") ===\n";
    ss << "DPS: " << FormatNumber(static_cast<uint64>(_lastEncounter.dps))
       << " | HPS: " << FormatNumber(static_cast<uint64>(_lastEncounter.hps))
       << " | DTPS: " << FormatNumber(static_cast<uint64>(_lastEncounter.dtps)) << "\n";
    ss << "Total Damage: " << FormatNumber(_lastEncounter.totalDamage)
       << " | Healing: " << FormatNumber(_lastEncounter.totalHealing)
       << " | Taken: " << FormatNumber(_lastEncounter.totalDamageTaken) << "\n";

    if (_lastEncounter.totalOverkill > 0 || _lastEncounter.totalOverheal > 0)
    {
        ss << "Overkill: " << FormatNumber(_lastEncounter.totalOverkill)
           << " | Overheal: " << FormatNumber(_lastEncounter.totalOverheal) << "\n";
    }

    // Top damage spells
    if (_lastEncounter.topDamageCount > 0)
    {
        ss << "Top Damage: ";
        for (uint32 i = 0; i < _lastEncounter.topDamageCount && i < 5; ++i)
        {
            auto const& entry = _lastEncounter.topDamageSpells[i];
            if (i > 0) ss << ", ";
            ss << GetSpellName(entry.spellId) << " " << ::std::fixed << ::std::setprecision(1) << entry.percentage << "%";
        }
        ss << "\n";
    }

    // Top healing spells
    if (_lastEncounter.topHealingCount > 0)
    {
        ss << "Top Healing: ";
        for (uint32 i = 0; i < _lastEncounter.topHealingCount && i < 5; ++i)
        {
            auto const& entry = _lastEncounter.topHealingSpells[i];
            if (i > 0) ss << ", ";
            ss << GetSpellName(entry.spellId) << " " << ::std::fixed << ::std::setprecision(1) << entry.percentage << "%";
        }
        ss << "\n";
    }

    ss << "Crits: " << _lastEncounter.critCount << " | Spells Cast: " << _lastEncounter.spellsCast;
    return ss.str();
}

::std::string CombatMetricsTracker::FormatBriefDPS() const
{
    if (_inCombat)
    {
        return "DPS: " + FormatNumber(static_cast<uint64>(GetOverallDPS()))
             + " (5s: " + FormatNumber(static_cast<uint64>(GetCurrentDPS())) + ")";
    }
    else if (_lastEncounter.dps > 0.0f)
    {
        return "DPS: " + FormatNumber(static_cast<uint64>(_lastEncounter.dps))
             + " (" + FormatDuration(_lastEncounter.durationMs) + ")";
    }
    return "DPS: N/A";
}

::std::string CombatMetricsTracker::FormatBriefHPS() const
{
    if (_inCombat)
    {
        return "HPS: " + FormatNumber(static_cast<uint64>(GetOverallHPS()))
             + " (5s: " + FormatNumber(static_cast<uint64>(GetCurrentHPS())) + ")";
    }
    else if (_lastEncounter.hps > 0.0f)
    {
        return "HPS: " + FormatNumber(static_cast<uint64>(_lastEncounter.hps))
             + " (" + FormatDuration(_lastEncounter.durationMs) + ")";
    }
    return "HPS: N/A";
}

// ============================================================================
// LIFECYCLE
// ============================================================================

void CombatMetricsTracker::Reset()
{
    _eventBuffer.fill(CombatEvent{});
    _eventWriteIndex = 0;
    _eventCount = 0;

    _damageBySpell.clear();
    _healingBySpell.clear();
    _damageTakenBySpell.clear();

    _inCombat = false;
    _combatStartTime = 0;
    _combatEndTime = 0;

    _encounterDamage = 0;
    _encounterHealing = 0;
    _encounterDamageTaken = 0;
    _encounterOverheal = 0;
    _encounterOverkill = 0;
    _encounterCrits = 0;
    _encounterEvents = 0;

    _sessionTotalDamage = 0;
    _sessionTotalHealing = 0;
    _sessionTotalDamageTaken = 0;
    _sessionTotalOverheal = 0;

    _lastEncounter = EncounterSummary{};
    _encounterHistory.clear();
    _updateTimer = 0;
}

void CombatMetricsTracker::ResetEncounter()
{
    _encounterDamage = 0;
    _encounterHealing = 0;
    _encounterDamageTaken = 0;
    _encounterOverheal = 0;
    _encounterOverkill = 0;
    _encounterCrits = 0;
    _encounterEvents = 0;

    _damageBySpell.clear();
    _healingBySpell.clear();
    _damageTakenBySpell.clear();
}

void CombatMetricsTracker::Update(uint32 diff)
{
    _updateTimer += diff;
    if (_updateTimer < UPDATE_INTERVAL)
        return;
    _updateTimer -= UPDATE_INTERVAL;

    // Auto-detect combat start/end based on Player combat state
    if (_bot)
    {
        bool playerInCombat = _bot->IsInCombat();
        if (playerInCombat && !_inCombat)
            OnCombatStart();
        else if (!playerInCombat && _inCombat)
            OnCombatEnd();
    }
}

// ============================================================================
// INTERNAL HELPERS
// ============================================================================

void CombatMetricsTracker::RecordEvent(CombatEvent const& event)
{
    _eventBuffer[_eventWriteIndex] = event;
    _eventWriteIndex = (_eventWriteIndex + 1) % EVENT_BUFFER_SIZE;
    if (_eventCount < EVENT_BUFFER_SIZE)
        _eventCount++;
}

float CombatMetricsTracker::CalculateRateInWindow(CombatEventType type, uint32 windowMs) const
{
    if (_eventCount == 0)
        return 0.0f;

    uint32 now = GetCurrentTimeMs();
    uint32 windowStart = (now > windowMs) ? (now - windowMs) : 0;
    uint64 totalAmount = 0;
    uint32 oldestTimestamp = now;
    uint32 newestTimestamp = 0;
    bool foundAny = false;

    // Scan the circular buffer
    uint32 startIdx = (_eventCount >= EVENT_BUFFER_SIZE)
        ? _eventWriteIndex
        : 0;

    for (uint32 i = 0; i < _eventCount; ++i)
    {
        uint32 idx = (startIdx + i) % EVENT_BUFFER_SIZE;
        auto const& ev = _eventBuffer[idx];

        if (ev.type != type)
            continue;

        if (ev.timestamp < windowStart)
            continue;

        totalAmount += ev.amount;
        if (ev.timestamp < oldestTimestamp)
            oldestTimestamp = ev.timestamp;
        if (ev.timestamp > newestTimestamp)
            newestTimestamp = ev.timestamp;
        foundAny = true;
    }

    if (!foundAny)
        return 0.0f;

    // Use the actual time span of the window for calculation
    // But ensure at least 1 second to avoid spikes
    float seconds = static_cast<float>(windowMs) / 1000.0f;
    if (seconds < 1.0f)
        seconds = 1.0f;

    return static_cast<float>(totalAmount) / seconds;
}

void CombatMetricsTracker::BuildEncounterSummary()
{
    _lastEncounter = EncounterSummary{};
    _lastEncounter.startTime = _combatStartTime;
    _lastEncounter.endTime = _combatEndTime;

    if (_combatEndTime > _combatStartTime)
        _lastEncounter.durationMs = _combatEndTime - _combatStartTime;
    else
        _lastEncounter.durationMs = 0;

    _lastEncounter.totalDamage = _encounterDamage;
    _lastEncounter.totalHealing = _encounterHealing;
    _lastEncounter.totalDamageTaken = _encounterDamageTaken;
    _lastEncounter.totalOverheal = _encounterOverheal;
    _lastEncounter.totalOverkill = _encounterOverkill;
    _lastEncounter.spellsCast = _encounterEvents;
    _lastEncounter.critCount = _encounterCrits;

    if (_lastEncounter.durationMs > 0)
    {
        float seconds = static_cast<float>(_lastEncounter.durationMs) / 1000.0f;
        _lastEncounter.dps = static_cast<float>(_encounterDamage) / seconds;
        _lastEncounter.hps = static_cast<float>(_encounterHealing) / seconds;
        _lastEncounter.dtps = static_cast<float>(_encounterDamageTaken) / seconds;
    }

    // Build top damage spells
    auto dmgSpells = GetDamageBySpell();
    _lastEncounter.topDamageCount = ::std::min(static_cast<uint32>(dmgSpells.size()), 5u);
    for (uint32 i = 0; i < _lastEncounter.topDamageCount; ++i)
    {
        _lastEncounter.topDamageSpells[i].spellId = dmgSpells[i].spellId;
        _lastEncounter.topDamageSpells[i].amount = dmgSpells[i].totalAmount;
        _lastEncounter.topDamageSpells[i].percentage = _encounterDamage > 0
            ? static_cast<float>(dmgSpells[i].totalAmount) / static_cast<float>(_encounterDamage) * 100.0f
            : 0.0f;
    }

    // Build top healing spells
    auto healSpells = GetHealingBySpell();
    _lastEncounter.topHealingCount = ::std::min(static_cast<uint32>(healSpells.size()), 5u);
    for (uint32 i = 0; i < _lastEncounter.topHealingCount; ++i)
    {
        _lastEncounter.topHealingSpells[i].spellId = healSpells[i].spellId;
        _lastEncounter.topHealingSpells[i].amount = healSpells[i].totalAmount;
        _lastEncounter.topHealingSpells[i].percentage = _encounterHealing > 0
            ? static_cast<float>(healSpells[i].totalAmount) / static_cast<float>(_encounterHealing) * 100.0f
            : 0.0f;
    }
}

void CombatMetricsTracker::UpdateSpellMetrics(uint32 spellId, uint32 amount, uint32 overhealOverkill,
                                               bool isCrit, bool isPeriodic, CombatEventType type)
{
    ::std::unordered_map<uint32, SpellMetrics>* targetMap = nullptr;

    switch (type)
    {
        case CombatEventType::DAMAGE_DONE:
            targetMap = &_damageBySpell;
            break;
        case CombatEventType::HEALING_DONE:
            targetMap = &_healingBySpell;
            break;
        case CombatEventType::DAMAGE_TAKEN:
            targetMap = &_damageTakenBySpell;
            break;
        default:
            return;
    }

    auto& metrics = (*targetMap)[spellId];
    if (metrics.spellId == 0)
        metrics.spellId = spellId;

    metrics.totalAmount += amount;
    metrics.totalOverhealOverkill += overhealOverkill;
    metrics.hitCount++;

    if (isCrit)
    {
        metrics.critCount++;
        if (amount > metrics.maxCrit)
            metrics.maxCrit = amount;
    }
    else
    {
        if (amount > metrics.maxHit)
            metrics.maxHit = amount;
    }

    if (isPeriodic)
        metrics.periodicCount++;
}

::std::string CombatMetricsTracker::FormatNumber(uint64 number) const
{
    if (number >= 1000000000ULL)
    {
        ::std::ostringstream ss;
        ss << ::std::fixed << ::std::setprecision(2) << (static_cast<double>(number) / 1000000000.0) << "B";
        return ss.str();
    }
    if (number >= 1000000ULL)
    {
        ::std::ostringstream ss;
        ss << ::std::fixed << ::std::setprecision(2) << (static_cast<double>(number) / 1000000.0) << "M";
        return ss.str();
    }
    if (number >= 10000ULL)
    {
        ::std::ostringstream ss;
        ss << ::std::fixed << ::std::setprecision(1) << (static_cast<double>(number) / 1000.0) << "K";
        return ss.str();
    }
    return ::std::to_string(number);
}

::std::string CombatMetricsTracker::FormatDuration(uint32 ms) const
{
    uint32 totalSeconds = ms / 1000;
    uint32 minutes = totalSeconds / 60;
    uint32 seconds = totalSeconds % 60;

    ::std::ostringstream ss;
    if (minutes > 0)
        ss << minutes << "m " << seconds << "s";
    else
        ss << seconds << "s";
    return ss.str();
}

::std::string CombatMetricsTracker::GetSpellName(uint32 spellId) const
{
    if (spellId == 0)
        return "Auto Attack";

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId, DIFFICULTY_NONE);
    if (spellInfo && spellInfo->SpellName)
    {
        char const* name = (*spellInfo->SpellName)[sWorld->GetDefaultDbcLocale()];
        if (name && name[0] != '\0')
            return name;
    }

    // Fallback: just show the ID
    return "Spell#" + ::std::to_string(spellId);
}

uint32 CombatMetricsTracker::GetCurrentTimeMs() const
{
    return GameTime::GetGameTimeMS();
}

::std::vector<SpellMetrics> CombatMetricsTracker::GetSortedSpellMetrics(
    ::std::unordered_map<uint32, SpellMetrics> const& metrics) const
{
    ::std::vector<SpellMetrics> result;
    result.reserve(metrics.size());

    for (auto const& [id, spell] : metrics)
        result.push_back(spell);

    // Sort by total amount descending
    ::std::sort(result.begin(), result.end(),
        [](SpellMetrics const& a, SpellMetrics const& b)
        {
            return a.totalAmount > b.totalAmount;
        });

    return result;
}

} // namespace Playerbot
