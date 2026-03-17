/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "StructuredCombatLog.h"
#include "Log.h"
#include "GameTime.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "World.h"
#include <algorithm>
#include <iomanip>
#include <sstream>

namespace Playerbot
{

// ============================================================================
// BOT COMBAT LOG BUFFER
// ============================================================================

BotCombatLogBuffer::BotCombatLogBuffer(uint32 capacity)
    : _buffer(capacity), _writeIndex(0), _count(0)
{
}

void BotCombatLogBuffer::Push(CombatLogEntry const& entry)
{
    _buffer[_writeIndex] = entry;
    _writeIndex = (_writeIndex + 1) % static_cast<uint32>(_buffer.size());
    if (_count < static_cast<uint32>(_buffer.size()))
        ++_count;
}

void BotCombatLogBuffer::Clear()
{
    _writeIndex = 0;
    _count = 0;
    // No need to zero the buffer; count tracks valid entries
}

::std::vector<CombatLogEntry> BotCombatLogBuffer::GetAllEntries() const
{
    ::std::vector<CombatLogEntry> result;
    if (_count == 0)
        return result;

    result.reserve(_count);

    // If buffer is not yet full, entries are [0, _count)
    // If buffer is full, oldest entry is at _writeIndex (it was just overwritten)
    uint32 startIdx = (_count >= static_cast<uint32>(_buffer.size()))
        ? _writeIndex
        : 0;

    for (uint32 i = 0; i < _count; ++i)
    {
        uint32 idx = (startIdx + i) % static_cast<uint32>(_buffer.size());
        result.push_back(_buffer[idx]);
    }

    return result;
}

::std::vector<CombatLogEntry> BotCombatLogBuffer::GetEntriesSince(uint32 sinceMs) const
{
    ::std::vector<CombatLogEntry> result;
    if (_count == 0)
        return result;

    uint32 startIdx = (_count >= static_cast<uint32>(_buffer.size()))
        ? _writeIndex
        : 0;

    for (uint32 i = 0; i < _count; ++i)
    {
        uint32 idx = (startIdx + i) % static_cast<uint32>(_buffer.size());
        if (_buffer[idx].timestampMs >= sinceMs)
            result.push_back(_buffer[idx]);
    }

    return result;
}

::std::vector<CombatLogEntry> BotCombatLogBuffer::GetEntriesByType(CombatLogEventType type) const
{
    ::std::vector<CombatLogEntry> result;
    if (_count == 0)
        return result;

    uint32 startIdx = (_count >= static_cast<uint32>(_buffer.size()))
        ? _writeIndex
        : 0;

    for (uint32 i = 0; i < _count; ++i)
    {
        uint32 idx = (startIdx + i) % static_cast<uint32>(_buffer.size());
        if (_buffer[idx].eventType == type)
            result.push_back(_buffer[idx]);
    }

    return result;
}

::std::vector<CombatLogEntry> BotCombatLogBuffer::GetEntriesByTypeSince(CombatLogEventType type, uint32 sinceMs) const
{
    ::std::vector<CombatLogEntry> result;
    if (_count == 0)
        return result;

    uint32 startIdx = (_count >= static_cast<uint32>(_buffer.size()))
        ? _writeIndex
        : 0;

    for (uint32 i = 0; i < _count; ++i)
    {
        uint32 idx = (startIdx + i) % static_cast<uint32>(_buffer.size());
        if (_buffer[idx].eventType == type && _buffer[idx].timestampMs >= sinceMs)
            result.push_back(_buffer[idx]);
    }

    return result;
}

// ============================================================================
// STRUCTURED COMBAT LOG - LIFECYCLE
// ============================================================================

void StructuredCombatLog::Initialize()
{
    ::std::lock_guard<::std::mutex> lock(_mapMutex);

    if (_initialized)
    {
        TC_LOG_DEBUG("module.playerbot", "StructuredCombatLog: Already initialized");
        return;
    }

    _botBuffers.clear();
    _initialized = true;

    TC_LOG_DEBUG("module.playerbot", "StructuredCombatLog: Initialized");
}

void StructuredCombatLog::Shutdown()
{
    ::std::lock_guard<::std::mutex> lock(_mapMutex);

    uint32 botCount = static_cast<uint32>(_botBuffers.size());
    _botBuffers.clear();
    _initialized = false;

    TC_LOG_DEBUG("module.playerbot", "StructuredCombatLog: Shutdown, released {} bot buffers", botCount);
}

void StructuredCombatLog::RegisterBot(ObjectGuid botGuid)
{
    ::std::lock_guard<::std::mutex> lock(_mapMutex);

    auto [it, inserted] = _botBuffers.try_emplace(botGuid, BotCombatLogBuffer::DEFAULT_CAPACITY);
    if (inserted)
    {
        TC_LOG_DEBUG("module.playerbot", "StructuredCombatLog: Registered bot {}",
            botGuid.ToString());
    }
}

void StructuredCombatLog::UnregisterBot(ObjectGuid botGuid)
{
    ::std::lock_guard<::std::mutex> lock(_mapMutex);

    auto it = _botBuffers.find(botGuid);
    if (it != _botBuffers.end())
    {
        _botBuffers.erase(it);
        TC_LOG_DEBUG("module.playerbot", "StructuredCombatLog: Unregistered bot {}",
            botGuid.ToString());
    }
}

void StructuredCombatLog::ClearBotLog(ObjectGuid botGuid)
{
    ::std::lock_guard<::std::mutex> lock(_mapMutex);

    auto it = _botBuffers.find(botGuid);
    if (it != _botBuffers.end())
        it->second.Clear();
}

void StructuredCombatLog::ClearAll()
{
    ::std::lock_guard<::std::mutex> lock(_mapMutex);

    for (auto& [guid, buffer] : _botBuffers)
        buffer.Clear();
}

// ============================================================================
// EVENT LOGGING
// ============================================================================

void StructuredCombatLog::LogDamageDealt(ObjectGuid botGuid, ObjectGuid targetGuid, uint32 spellId,
                                          int32 amount, uint8 school, bool isCritical)
{
    RecordEvent(botGuid, CombatLogEventType::DAMAGE_DEALT,
                botGuid, targetGuid, spellId, amount, school, isCritical);
}

void StructuredCombatLog::LogDamageTaken(ObjectGuid botGuid, ObjectGuid sourceGuid, uint32 spellId,
                                          int32 amount, uint8 school, bool isCritical)
{
    RecordEvent(botGuid, CombatLogEventType::DAMAGE_TAKEN,
                sourceGuid, botGuid, spellId, amount, school, isCritical);
}

void StructuredCombatLog::LogHealingDone(ObjectGuid botGuid, ObjectGuid targetGuid, uint32 spellId,
                                          int32 amount, uint8 school, bool isCritical)
{
    RecordEvent(botGuid, CombatLogEventType::HEALING_DONE,
                botGuid, targetGuid, spellId, amount, school, isCritical);
}

void StructuredCombatLog::LogHealingReceived(ObjectGuid botGuid, ObjectGuid sourceGuid, uint32 spellId,
                                              int32 amount, uint8 school, bool isCritical)
{
    RecordEvent(botGuid, CombatLogEventType::HEALING_RECEIVED,
                sourceGuid, botGuid, spellId, amount, school, isCritical);
}

void StructuredCombatLog::LogBuffApplied(ObjectGuid botGuid, ObjectGuid sourceGuid, uint32 spellId)
{
    RecordEvent(botGuid, CombatLogEventType::BUFF_APPLIED,
                sourceGuid, botGuid, spellId, 0, 0, false);
}

void StructuredCombatLog::LogBuffRemoved(ObjectGuid botGuid, ObjectGuid sourceGuid, uint32 spellId)
{
    RecordEvent(botGuid, CombatLogEventType::BUFF_REMOVED,
                sourceGuid, botGuid, spellId, 0, 0, false);
}

void StructuredCombatLog::LogDebuffApplied(ObjectGuid botGuid, ObjectGuid sourceGuid, uint32 spellId)
{
    RecordEvent(botGuid, CombatLogEventType::DEBUFF_APPLIED,
                sourceGuid, botGuid, spellId, 0, 0, false);
}

void StructuredCombatLog::LogDebuffRemoved(ObjectGuid botGuid, ObjectGuid sourceGuid, uint32 spellId)
{
    RecordEvent(botGuid, CombatLogEventType::DEBUFF_REMOVED,
                sourceGuid, botGuid, spellId, 0, 0, false);
}

void StructuredCombatLog::LogDeath(ObjectGuid botGuid, ObjectGuid killerGuid, uint32 spellId)
{
    RecordEvent(botGuid, CombatLogEventType::DEATH,
                killerGuid, botGuid, spellId, 0, 0, false);

    TC_LOG_DEBUG("module.playerbot", "StructuredCombatLog: Death recorded for bot {} (killer: {}, spell: {})",
        botGuid.ToString(), killerGuid.ToString(), spellId);
}

void StructuredCombatLog::LogSpellCast(ObjectGuid botGuid, ObjectGuid targetGuid, uint32 spellId,
                                        bool success)
{
    RecordEvent(botGuid, CombatLogEventType::SPELL_CAST,
                botGuid, targetGuid, spellId, success ? 1 : 0, 0, false);
}

void StructuredCombatLog::LogEvent(ObjectGuid botGuid, CombatLogEntry const& entry)
{
    BotCombatLogBuffer* buffer = EnsureBuffer(botGuid);
    if (buffer)
        buffer->Push(entry);
}

// ============================================================================
// SUMMARY GENERATION
// ============================================================================

DPSSummary StructuredCombatLog::GetDPSSummary(ObjectGuid botGuid) const
{
    DPSSummary summary;

    ::std::lock_guard<::std::mutex> lock(_mapMutex);

    BotCombatLogBuffer const* buffer = GetBuffer(botGuid);
    if (!buffer || buffer->IsEmpty())
        return summary;

    // Collect all damage events
    auto damageEvents = buffer->GetEntriesByType(CombatLogEventType::DAMAGE_DEALT);
    if (damageEvents.empty())
        return summary;

    // Aggregate per-spell data
    struct SpellAgg
    {
        uint64 totalDamage{0};
        uint32 hitCount{0};
        uint32 critCount{0};
    };
    ::std::unordered_map<uint32, SpellAgg> spellAggs;

    uint32 earliestTimestamp = UINT32_MAX;
    uint32 latestTimestamp = 0;

    for (auto const& event : damageEvents)
    {
        summary.totalDamage += static_cast<uint64>(event.amount > 0 ? event.amount : 0);
        ++summary.eventCount;

        auto& agg = spellAggs[event.spellId];
        agg.totalDamage += static_cast<uint64>(event.amount > 0 ? event.amount : 0);
        ++agg.hitCount;
        if (event.isCritical)
            ++agg.critCount;

        if (event.timestampMs < earliestTimestamp)
            earliestTimestamp = event.timestampMs;
        if (event.timestampMs > latestTimestamp)
            latestTimestamp = event.timestampMs;
    }

    // Calculate DPS
    if (latestTimestamp > earliestTimestamp)
    {
        summary.durationMs = latestTimestamp - earliestTimestamp;
        float seconds = static_cast<float>(summary.durationMs) / 1000.0f;
        if (seconds > 0.0f)
            summary.dps = static_cast<float>(summary.totalDamage) / seconds;
    }
    else if (summary.eventCount > 0)
    {
        // Single event, assume 1 second
        summary.durationMs = 1000;
        summary.dps = static_cast<float>(summary.totalDamage);
    }

    // Build top spells list sorted by total damage descending
    summary.topSpells.reserve(spellAggs.size());
    for (auto const& [spellId, agg] : spellAggs)
    {
        DPSSummary::SpellBreakdown breakdown;
        breakdown.spellId = spellId;
        breakdown.totalDamage = agg.totalDamage;
        breakdown.hitCount = agg.hitCount;
        breakdown.critCount = agg.critCount;
        breakdown.percentage = summary.totalDamage > 0
            ? static_cast<float>(agg.totalDamage) / static_cast<float>(summary.totalDamage) * 100.0f
            : 0.0f;
        summary.topSpells.push_back(breakdown);
    }

    ::std::sort(summary.topSpells.begin(), summary.topSpells.end(),
        [](DPSSummary::SpellBreakdown const& a, DPSSummary::SpellBreakdown const& b)
        {
            return a.totalDamage > b.totalDamage;
        });

    return summary;
}

HPSSummary StructuredCombatLog::GetHPSSummary(ObjectGuid botGuid) const
{
    HPSSummary summary;

    ::std::lock_guard<::std::mutex> lock(_mapMutex);

    BotCombatLogBuffer const* buffer = GetBuffer(botGuid);
    if (!buffer || buffer->IsEmpty())
        return summary;

    // Collect all healing events
    auto healingEvents = buffer->GetEntriesByType(CombatLogEventType::HEALING_DONE);
    if (healingEvents.empty())
        return summary;

    // Aggregate per-spell data
    struct SpellAgg
    {
        uint64 totalHealing{0};
        uint32 hitCount{0};
        uint32 critCount{0};
    };
    ::std::unordered_map<uint32, SpellAgg> spellAggs;

    uint32 earliestTimestamp = UINT32_MAX;
    uint32 latestTimestamp = 0;

    for (auto const& event : healingEvents)
    {
        summary.totalHealing += static_cast<uint64>(event.amount > 0 ? event.amount : 0);
        ++summary.eventCount;

        auto& agg = spellAggs[event.spellId];
        agg.totalHealing += static_cast<uint64>(event.amount > 0 ? event.amount : 0);
        ++agg.hitCount;
        if (event.isCritical)
            ++agg.critCount;

        if (event.timestampMs < earliestTimestamp)
            earliestTimestamp = event.timestampMs;
        if (event.timestampMs > latestTimestamp)
            latestTimestamp = event.timestampMs;
    }

    // Calculate HPS
    if (latestTimestamp > earliestTimestamp)
    {
        summary.durationMs = latestTimestamp - earliestTimestamp;
        float seconds = static_cast<float>(summary.durationMs) / 1000.0f;
        if (seconds > 0.0f)
            summary.hps = static_cast<float>(summary.totalHealing) / seconds;
    }
    else if (summary.eventCount > 0)
    {
        summary.durationMs = 1000;
        summary.hps = static_cast<float>(summary.totalHealing);
    }

    // Build top spells list sorted by total healing descending
    summary.topSpells.reserve(spellAggs.size());
    for (auto const& [spellId, agg] : spellAggs)
    {
        HPSSummary::SpellBreakdown breakdown;
        breakdown.spellId = spellId;
        breakdown.totalHealing = agg.totalHealing;
        breakdown.hitCount = agg.hitCount;
        breakdown.critCount = agg.critCount;
        breakdown.percentage = summary.totalHealing > 0
            ? static_cast<float>(agg.totalHealing) / static_cast<float>(summary.totalHealing) * 100.0f
            : 0.0f;
        summary.topSpells.push_back(breakdown);
    }

    ::std::sort(summary.topSpells.begin(), summary.topSpells.end(),
        [](HPSSummary::SpellBreakdown const& a, HPSSummary::SpellBreakdown const& b)
        {
            return a.totalHealing > b.totalHealing;
        });

    return summary;
}

::std::vector<DeathLogEntry> StructuredCombatLog::GetDeathLog(ObjectGuid botGuid, uint32 windowMs) const
{
    ::std::vector<DeathLogEntry> result;

    ::std::lock_guard<::std::mutex> lock(_mapMutex);

    BotCombatLogBuffer const* buffer = GetBuffer(botGuid);
    if (!buffer || buffer->IsEmpty())
        return result;

    // Find the most recent death event for this bot
    auto allEntries = buffer->GetAllEntries();
    uint32 deathTimestamp = 0;

    // Scan backwards for the most recent death
    for (auto it = allEntries.rbegin(); it != allEntries.rend(); ++it)
    {
        if (it->eventType == CombatLogEventType::DEATH && it->targetGuid == botGuid)
        {
            deathTimestamp = it->timestampMs;
            break;
        }
    }

    if (deathTimestamp == 0)
        return result;  // No death found

    // Collect all events in the window before death
    uint32 windowStart = (deathTimestamp > windowMs) ? (deathTimestamp - windowMs) : 0;

    // Track approximate health changes (we only track deltas, not absolute health)
    // We reconstruct relative health changes: positive for healing, negative for damage
    int32 cumulativeDelta = 0;

    for (auto const& entry : allEntries)
    {
        if (entry.timestampMs < windowStart)
            continue;
        if (entry.timestampMs > deathTimestamp)
            break;

        DeathLogEntry deathEntry;
        deathEntry.event = entry;
        deathEntry.spellName = GetSpellName(entry.spellId);

        // We do not have access to the source Player/Unit here for name resolution,
        // so we store the GUID string as the source name. Callers with access to
        // ObjectAccessor can resolve names as needed.
        deathEntry.sourceName = entry.sourceGuid.ToString();

        // Track cumulative health delta from damage/healing
        switch (entry.eventType)
        {
            case CombatLogEventType::DAMAGE_TAKEN:
                cumulativeDelta -= entry.amount;
                break;
            case CombatLogEventType::HEALING_RECEIVED:
                cumulativeDelta += entry.amount;
                break;
            default:
                break;
        }

        // healthRemaining is relative: 0 means dead, negative means overkill
        // At the death event itself, health is 0
        if (entry.eventType == CombatLogEventType::DEATH)
            deathEntry.healthRemaining = 0;
        else
            deathEntry.healthRemaining = cumulativeDelta;

        result.push_back(::std::move(deathEntry));
    }

    return result;
}

// ============================================================================
// RAW ACCESS
// ============================================================================

::std::vector<CombatLogEntry> StructuredCombatLog::GetAllEntries(ObjectGuid botGuid) const
{
    ::std::lock_guard<::std::mutex> lock(_mapMutex);

    BotCombatLogBuffer const* buffer = GetBuffer(botGuid);
    if (!buffer)
        return {};

    return buffer->GetAllEntries();
}

::std::vector<CombatLogEntry> StructuredCombatLog::GetEntriesSince(ObjectGuid botGuid, uint32 sinceMs) const
{
    ::std::lock_guard<::std::mutex> lock(_mapMutex);

    BotCombatLogBuffer const* buffer = GetBuffer(botGuid);
    if (!buffer)
        return {};

    return buffer->GetEntriesSince(sinceMs);
}

::std::vector<CombatLogEntry> StructuredCombatLog::GetEntriesByType(ObjectGuid botGuid, CombatLogEventType type) const
{
    ::std::lock_guard<::std::mutex> lock(_mapMutex);

    BotCombatLogBuffer const* buffer = GetBuffer(botGuid);
    if (!buffer)
        return {};

    return buffer->GetEntriesByType(type);
}

bool StructuredCombatLog::IsBotRegistered(ObjectGuid botGuid) const
{
    ::std::lock_guard<::std::mutex> lock(_mapMutex);
    return _botBuffers.find(botGuid) != _botBuffers.end();
}

uint32 StructuredCombatLog::GetRegisteredBotCount() const
{
    ::std::lock_guard<::std::mutex> lock(_mapMutex);
    return static_cast<uint32>(_botBuffers.size());
}

uint64 StructuredCombatLog::GetTotalEventCount() const
{
    ::std::lock_guard<::std::mutex> lock(_mapMutex);

    uint64 total = 0;
    for (auto const& [guid, buffer] : _botBuffers)
        total += buffer.GetCount();
    return total;
}

// ============================================================================
// FORMATTED OUTPUT
// ============================================================================

::std::string StructuredCombatLog::FormatDPSSummary(ObjectGuid botGuid) const
{
    DPSSummary summary = GetDPSSummary(botGuid);

    ::std::ostringstream ss;
    if (summary.eventCount == 0)
    {
        ss << "--- Structured DPS Log ---\n";
        ss << "No damage data recorded.\n";
        return ss.str();
    }

    ss << "--- Structured DPS Log (" << FormatDuration(summary.durationMs) << ") ---\n";
    ss << "Total Damage: " << FormatNumber(summary.totalDamage)
       << " | DPS: " << FormatNumber(static_cast<uint64>(summary.dps))
       << " | Events: " << summary.eventCount << "\n";

    if (!summary.topSpells.empty())
    {
        ss << "--- Spell Breakdown ---\n";
        uint32 shown = 0;
        for (auto const& spell : summary.topSpells)
        {
            if (shown >= 10)
                break;

            float critRate = spell.hitCount > 0
                ? static_cast<float>(spell.critCount) / static_cast<float>(spell.hitCount) * 100.0f
                : 0.0f;

            ss << "  " << GetSpellName(spell.spellId) << ": "
               << FormatNumber(spell.totalDamage)
               << " (" << ::std::fixed << ::std::setprecision(1) << spell.percentage << "%)"
               << " | Hits: " << spell.hitCount
               << " | Crit: " << ::std::fixed << ::std::setprecision(1) << critRate << "%"
               << "\n";
            ++shown;
        }
    }

    return ss.str();
}

::std::string StructuredCombatLog::FormatHPSSummary(ObjectGuid botGuid) const
{
    HPSSummary summary = GetHPSSummary(botGuid);

    ::std::ostringstream ss;
    if (summary.eventCount == 0)
    {
        ss << "--- Structured HPS Log ---\n";
        ss << "No healing data recorded.\n";
        return ss.str();
    }

    ss << "--- Structured HPS Log (" << FormatDuration(summary.durationMs) << ") ---\n";
    ss << "Total Healing: " << FormatNumber(summary.totalHealing)
       << " | HPS: " << FormatNumber(static_cast<uint64>(summary.hps))
       << " | Events: " << summary.eventCount << "\n";

    if (!summary.topSpells.empty())
    {
        ss << "--- Spell Breakdown ---\n";
        uint32 shown = 0;
        for (auto const& spell : summary.topSpells)
        {
            if (shown >= 10)
                break;

            float critRate = spell.hitCount > 0
                ? static_cast<float>(spell.critCount) / static_cast<float>(spell.hitCount) * 100.0f
                : 0.0f;

            ss << "  " << GetSpellName(spell.spellId) << ": "
               << FormatNumber(spell.totalHealing)
               << " (" << ::std::fixed << ::std::setprecision(1) << spell.percentage << "%)"
               << " | Hits: " << spell.hitCount
               << " | Crit: " << ::std::fixed << ::std::setprecision(1) << critRate << "%"
               << "\n";
            ++shown;
        }
    }

    return ss.str();
}

::std::string StructuredCombatLog::FormatDeathLog(ObjectGuid botGuid, uint32 windowMs) const
{
    auto deathLog = GetDeathLog(botGuid, windowMs);

    ::std::ostringstream ss;
    if (deathLog.empty())
    {
        ss << "--- Death Log ---\n";
        ss << "No death events recorded.\n";
        return ss.str();
    }

    // Find the death event timestamp for relative timing
    uint32 deathTime = 0;
    for (auto const& entry : deathLog)
    {
        if (entry.event.eventType == CombatLogEventType::DEATH)
        {
            deathTime = entry.event.timestampMs;
            break;
        }
    }

    ss << "--- Death Log (last " << FormatDuration(windowMs) << " before death) ---\n";

    for (auto const& entry : deathLog)
    {
        // Calculate time relative to death (negative = before death)
        int32 relativeMs = static_cast<int32>(entry.event.timestampMs) - static_cast<int32>(deathTime);
        float relativeSeconds = static_cast<float>(relativeMs) / 1000.0f;

        ss << "[" << ::std::fixed << ::std::setprecision(1) << ::std::showpos << relativeSeconds << "s] ";

        switch (entry.event.eventType)
        {
            case CombatLogEventType::DAMAGE_TAKEN:
                ss << "TOOK " << entry.event.amount << " from " << entry.spellName;
                if (entry.event.isCritical)
                    ss << " (CRIT)";
                break;
            case CombatLogEventType::HEALING_RECEIVED:
                ss << "HEALED " << entry.event.amount << " by " << entry.spellName;
                if (entry.event.isCritical)
                    ss << " (CRIT)";
                break;
            case CombatLogEventType::DAMAGE_DEALT:
                ss << "DEALT " << entry.event.amount << " with " << entry.spellName;
                break;
            case CombatLogEventType::HEALING_DONE:
                ss << "HEALED-OUT " << entry.event.amount << " with " << entry.spellName;
                break;
            case CombatLogEventType::BUFF_APPLIED:
                ss << "BUFF +" << entry.spellName;
                break;
            case CombatLogEventType::BUFF_REMOVED:
                ss << "BUFF -" << entry.spellName;
                break;
            case CombatLogEventType::DEBUFF_APPLIED:
                ss << "DEBUFF +" << entry.spellName;
                break;
            case CombatLogEventType::DEBUFF_REMOVED:
                ss << "DEBUFF -" << entry.spellName;
                break;
            case CombatLogEventType::DEATH:
                ss << "DIED";
                if (entry.event.spellId != 0)
                    ss << " (from " << entry.spellName << ")";
                break;
            case CombatLogEventType::SPELL_CAST:
                ss << "CAST " << entry.spellName << (entry.event.amount > 0 ? " (SUCCESS)" : " (FAILED)");
                break;
            default:
                ss << "EVENT " << static_cast<uint32>(entry.event.eventType);
                break;
        }

        ss << "\n";
    }

    return ss.str();
}

// ============================================================================
// INTERNAL HELPERS
// ============================================================================

BotCombatLogBuffer* StructuredCombatLog::GetBuffer(ObjectGuid botGuid)
{
    // Caller must hold _mapMutex or guarantee single-threaded access
    auto it = _botBuffers.find(botGuid);
    if (it != _botBuffers.end())
        return &it->second;
    return nullptr;
}

BotCombatLogBuffer const* StructuredCombatLog::GetBuffer(ObjectGuid botGuid) const
{
    // Caller must hold _mapMutex or guarantee single-threaded access
    auto it = _botBuffers.find(botGuid);
    if (it != _botBuffers.end())
        return &it->second;
    return nullptr;
}

BotCombatLogBuffer* StructuredCombatLog::EnsureBuffer(ObjectGuid botGuid)
{
    ::std::lock_guard<::std::mutex> lock(_mapMutex);

    auto [it, inserted] = _botBuffers.try_emplace(botGuid, BotCombatLogBuffer::DEFAULT_CAPACITY);
    return &it->second;
}

void StructuredCombatLog::RecordEvent(ObjectGuid botGuid, CombatLogEventType type,
                                       ObjectGuid sourceGuid, ObjectGuid targetGuid,
                                       uint32 spellId, int32 amount, uint8 school, bool isCritical)
{
    CombatLogEntry entry;
    entry.timestampMs = GetCurrentTimeMs();
    entry.eventType = type;
    entry.sourceGuid = sourceGuid;
    entry.targetGuid = targetGuid;
    entry.spellId = spellId;
    entry.amount = amount;
    entry.school = school;
    entry.isCritical = isCritical;

    BotCombatLogBuffer* buffer = EnsureBuffer(botGuid);
    if (buffer)
        buffer->Push(entry);
}

uint32 StructuredCombatLog::GetCurrentTimeMs() const
{
    return GameTime::GetGameTimeMS();
}

::std::string StructuredCombatLog::GetSpellName(uint32 spellId) const
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

    return "Spell#" + ::std::to_string(spellId);
}

::std::string StructuredCombatLog::FormatNumber(uint64 number)
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

::std::string StructuredCombatLog::FormatDuration(uint32 ms)
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

} // namespace Playerbot
