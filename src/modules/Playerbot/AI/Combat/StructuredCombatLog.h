/*
 * Copyright (C) 2024+ TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * STRUCTURED COMBAT LOG
 *
 * Structured combat event logging for bot performance analysis. Records
 * damage, healing, threat, buff/debuff, death, and spell cast events with
 * timestamps and spell IDs. Enables post-fight analysis through DPS/HPS
 * summaries, death logs, and per-spell breakdowns.
 *
 * Architecture:
 *   - Thread-safe singleton (sStructuredCombatLog) managing per-bot log buffers
 *   - Each bot has its own ring buffer (fixed capacity, zero allocation in combat)
 *   - No cross-bot contention: each bot's buffer is accessed only from its own
 *     update thread, the singleton map is only modified on bot add/remove
 *   - Summary generation for DPS, HPS, and death analysis
 *
 * Event Types:
 *   DAMAGE_DEALT     - Spell/auto-attack damage to targets
 *   DAMAGE_TAKEN     - Damage received from sources
 *   HEALING_DONE     - Healing output to targets
 *   HEALING_RECEIVED - Healing input from sources
 *   BUFF_APPLIED     - Buff applied to bot or target
 *   BUFF_REMOVED     - Buff removed from bot or target
 *   DEBUFF_APPLIED   - Debuff applied to bot or target
 *   DEBUFF_REMOVED   - Debuff removed from bot or target
 *   DEATH            - Bot or target death event
 *   SPELL_CAST       - Spell cast attempt (success/fail via amount field)
 *
 * Usage:
 *   sStructuredCombatLog->LogDamageDealt(botGuid, targetGuid, spellId, 5000, SPELL_SCHOOL_MASK_FIRE, true);
 *   sStructuredCombatLog->LogHealingDone(botGuid, targetGuid, spellId, 3000, SPELL_SCHOOL_MASK_HOLY, false);
 *   auto dpsSummary = sStructuredCombatLog->GetDPSSummary(botGuid);
 *   auto deathLog = sStructuredCombatLog->GetDeathLog(botGuid, 10000); // last 10s
 *
 * Integration Points:
 *   - BotAI combat handlers: Log damage/healing events
 *   - CombatMetricsTracker: Can feed into structured log for unified storage
 *   - Chat commands: .bot combatlog dps / .bot combatlog hps / .bot combatlog deaths
 *   - Post-encounter analysis: Retrieve full event stream for replay
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include <array>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace Playerbot
{

// ============================================================================
// COMBAT LOG EVENT TYPES
// ============================================================================

enum class CombatLogEventType : uint8
{
    DAMAGE_DEALT        = 0,    // Bot dealt damage to a target
    DAMAGE_TAKEN        = 1,    // Bot received damage from a source
    HEALING_DONE        = 2,    // Bot healed a target
    HEALING_RECEIVED    = 3,    // Bot received healing from a source
    BUFF_APPLIED        = 4,    // Buff applied
    BUFF_REMOVED        = 5,    // Buff removed
    DEBUFF_APPLIED      = 6,    // Debuff applied
    DEBUFF_REMOVED      = 7,    // Debuff removed
    DEATH               = 8,    // Death event (bot or target)
    SPELL_CAST          = 9,    // Spell cast attempt (amount: 1=success, 0=fail)

    MAX_EVENT_TYPE
};

// ============================================================================
// COMBAT LOG ENTRY (Fixed-size for ring buffer storage)
// ============================================================================

struct CombatLogEntry
{
    uint32 timestampMs{0};              // GameTime in milliseconds
    CombatLogEventType eventType{CombatLogEventType::DAMAGE_DEALT};
    ObjectGuid sourceGuid;              // Who caused the event
    ObjectGuid targetGuid;              // Who was affected
    uint32 spellId{0};                  // Spell ID (0 = auto-attack / melee)
    int32 amount{0};                    // Damage/healing amount, or 1/0 for cast success/fail
    uint8 school{0};                    // Spell school mask (physical, fire, etc.)
    bool isCritical{false};             // Was this a critical strike
};

// ============================================================================
// DPS SUMMARY (Returned by GetDPSSummary)
// ============================================================================

struct DPSSummary
{
    uint64 totalDamage{0};              // Total damage dealt
    float dps{0.0f};                    // Damage per second
    uint32 durationMs{0};               // Combat duration used for calculation
    uint32 eventCount{0};               // Total damage events

    struct SpellBreakdown
    {
        uint32 spellId{0};
        uint64 totalDamage{0};
        uint32 hitCount{0};
        uint32 critCount{0};
        float percentage{0.0f};         // % of total damage
    };

    ::std::vector<SpellBreakdown> topSpells;   // Sorted by damage descending
};

// ============================================================================
// HPS SUMMARY (Returned by GetHPSSummary)
// ============================================================================

struct HPSSummary
{
    uint64 totalHealing{0};             // Total healing done
    float hps{0.0f};                    // Healing per second
    uint32 durationMs{0};               // Combat duration used for calculation
    uint32 eventCount{0};               // Total healing events

    struct SpellBreakdown
    {
        uint32 spellId{0};
        uint64 totalHealing{0};
        uint32 hitCount{0};
        uint32 critCount{0};
        float percentage{0.0f};         // % of total healing
    };

    ::std::vector<SpellBreakdown> topSpells;   // Sorted by healing descending
};

// ============================================================================
// DEATH LOG (Returned by GetDeathLog)
// ============================================================================

struct DeathLogEntry
{
    CombatLogEntry event;               // The raw event
    ::std::string spellName;            // Resolved spell name for display
    ::std::string sourceName;           // Resolved source name for display
    int32 healthRemaining{0};           // Approximate health after this event (0 = dead)
};

// ============================================================================
// PER-BOT COMBAT LOG BUFFER (Ring buffer with fixed capacity)
// ============================================================================

class BotCombatLogBuffer
{
public:
    static constexpr uint32 DEFAULT_CAPACITY = 1000;

    explicit BotCombatLogBuffer(uint32 capacity = DEFAULT_CAPACITY);
    ~BotCombatLogBuffer() = default;

    // Non-copyable, movable
    BotCombatLogBuffer(BotCombatLogBuffer const&) = delete;
    BotCombatLogBuffer& operator=(BotCombatLogBuffer const&) = delete;
    BotCombatLogBuffer(BotCombatLogBuffer&&) noexcept = default;
    BotCombatLogBuffer& operator=(BotCombatLogBuffer&&) noexcept = default;

    /// Push a new entry into the ring buffer (overwrites oldest if full)
    void Push(CombatLogEntry const& entry);

    /// Get the number of entries currently stored
    uint32 GetCount() const { return _count; }

    /// Get the buffer capacity
    uint32 GetCapacity() const { return static_cast<uint32>(_buffer.size()); }

    /// Check if the buffer is empty
    bool IsEmpty() const { return _count == 0; }

    /// Clear all entries
    void Clear();

    /// Get all entries in chronological order (oldest first)
    ::std::vector<CombatLogEntry> GetAllEntries() const;

    /// Get entries within a time window (timestampMs >= sinceMs)
    ::std::vector<CombatLogEntry> GetEntriesSince(uint32 sinceMs) const;

    /// Get entries of a specific type
    ::std::vector<CombatLogEntry> GetEntriesByType(CombatLogEventType type) const;

    /// Get entries within a time window of a specific type
    ::std::vector<CombatLogEntry> GetEntriesByTypeSince(CombatLogEventType type, uint32 sinceMs) const;

private:
    ::std::vector<CombatLogEntry> _buffer;
    uint32 _writeIndex{0};
    uint32 _count{0};
};

// ============================================================================
// STRUCTURED COMBAT LOG (Thread-safe singleton)
// ============================================================================

class TC_GAME_API StructuredCombatLog final
{
public:
    // Singleton
    static StructuredCombatLog* instance()
    {
        static StructuredCombatLog inst;
        return &inst;
    }

    // ========================================================================
    // LIFECYCLE
    // ========================================================================

    /// Initialize the combat log system
    void Initialize();

    /// Shutdown and release all buffers
    void Shutdown();

    /// Register a bot for combat logging (creates buffer if not exists)
    void RegisterBot(ObjectGuid botGuid);

    /// Unregister a bot and free its buffer
    void UnregisterBot(ObjectGuid botGuid);

    /// Clear all logged data for a bot (keeps registration)
    void ClearBotLog(ObjectGuid botGuid);

    /// Clear all logged data for all bots
    void ClearAll();

    // ========================================================================
    // EVENT LOGGING
    // ========================================================================

    /// Log damage dealt by a bot
    void LogDamageDealt(ObjectGuid botGuid, ObjectGuid targetGuid, uint32 spellId,
                        int32 amount, uint8 school, bool isCritical);

    /// Log damage taken by a bot
    void LogDamageTaken(ObjectGuid botGuid, ObjectGuid sourceGuid, uint32 spellId,
                        int32 amount, uint8 school, bool isCritical);

    /// Log healing done by a bot
    void LogHealingDone(ObjectGuid botGuid, ObjectGuid targetGuid, uint32 spellId,
                        int32 amount, uint8 school, bool isCritical);

    /// Log healing received by a bot
    void LogHealingReceived(ObjectGuid botGuid, ObjectGuid sourceGuid, uint32 spellId,
                            int32 amount, uint8 school, bool isCritical);

    /// Log a buff applied
    void LogBuffApplied(ObjectGuid botGuid, ObjectGuid sourceGuid, uint32 spellId);

    /// Log a buff removed
    void LogBuffRemoved(ObjectGuid botGuid, ObjectGuid sourceGuid, uint32 spellId);

    /// Log a debuff applied
    void LogDebuffApplied(ObjectGuid botGuid, ObjectGuid sourceGuid, uint32 spellId);

    /// Log a debuff removed
    void LogDebuffRemoved(ObjectGuid botGuid, ObjectGuid sourceGuid, uint32 spellId);

    /// Log a death event
    void LogDeath(ObjectGuid botGuid, ObjectGuid killerGuid, uint32 spellId);

    /// Log a spell cast attempt (amount: 1 = success, 0 = failure)
    void LogSpellCast(ObjectGuid botGuid, ObjectGuid targetGuid, uint32 spellId,
                      bool success);

    /// Log a raw event directly
    void LogEvent(ObjectGuid botGuid, CombatLogEntry const& entry);

    // ========================================================================
    // SUMMARY GENERATION
    // ========================================================================

    /// Get DPS summary for a bot (total damage, DPS, top spells by damage)
    /// Analyzes all entries currently in the ring buffer
    DPSSummary GetDPSSummary(ObjectGuid botGuid) const;

    /// Get HPS summary for a bot (total healing, HPS, top spells by healing)
    /// Analyzes all entries currently in the ring buffer
    HPSSummary GetHPSSummary(ObjectGuid botGuid) const;

    /// Get death log for a bot - events in the last N milliseconds before the most recent death
    /// Returns events in chronological order with resolved names
    ::std::vector<DeathLogEntry> GetDeathLog(ObjectGuid botGuid, uint32 windowMs = 10000) const;

    // ========================================================================
    // RAW ACCESS
    // ========================================================================

    /// Get all raw entries for a bot (chronological order)
    ::std::vector<CombatLogEntry> GetAllEntries(ObjectGuid botGuid) const;

    /// Get entries since a timestamp
    ::std::vector<CombatLogEntry> GetEntriesSince(ObjectGuid botGuid, uint32 sinceMs) const;

    /// Get entries of a specific type
    ::std::vector<CombatLogEntry> GetEntriesByType(ObjectGuid botGuid, CombatLogEventType type) const;

    /// Check if a bot is registered
    bool IsBotRegistered(ObjectGuid botGuid) const;

    /// Get the number of registered bots
    uint32 GetRegisteredBotCount() const;

    /// Get the total number of events across all bots
    uint64 GetTotalEventCount() const;

    // ========================================================================
    // FORMATTED OUTPUT
    // ========================================================================

    /// Format DPS summary as a multi-line string for chat output
    ::std::string FormatDPSSummary(ObjectGuid botGuid) const;

    /// Format HPS summary as a multi-line string for chat output
    ::std::string FormatHPSSummary(ObjectGuid botGuid) const;

    /// Format death log as a multi-line string for chat output
    ::std::string FormatDeathLog(ObjectGuid botGuid, uint32 windowMs = 10000) const;

private:
    StructuredCombatLog() = default;
    ~StructuredCombatLog() = default;

    // Non-copyable
    StructuredCombatLog(StructuredCombatLog const&) = delete;
    StructuredCombatLog& operator=(StructuredCombatLog const&) = delete;

    // ========================================================================
    // INTERNAL HELPERS
    // ========================================================================

    /// Get the buffer for a bot, returns nullptr if not registered
    BotCombatLogBuffer* GetBuffer(ObjectGuid botGuid);
    BotCombatLogBuffer const* GetBuffer(ObjectGuid botGuid) const;

    /// Ensure a buffer exists for a bot (creates if needed, returns pointer)
    BotCombatLogBuffer* EnsureBuffer(ObjectGuid botGuid);

    /// Record a generic event into a bot's buffer
    void RecordEvent(ObjectGuid botGuid, CombatLogEventType type,
                     ObjectGuid sourceGuid, ObjectGuid targetGuid,
                     uint32 spellId, int32 amount, uint8 school, bool isCritical);

    /// Get current game time in milliseconds
    uint32 GetCurrentTimeMs() const;

    /// Resolve a spell ID to its name
    ::std::string GetSpellName(uint32 spellId) const;

    /// Format a number with K/M/B suffixes
    static ::std::string FormatNumber(uint64 number);

    /// Format milliseconds as a duration string (e.g. "2m 15s")
    static ::std::string FormatDuration(uint32 ms);

    // ========================================================================
    // MEMBER DATA
    // ========================================================================

    /// Per-bot log buffers, keyed by bot ObjectGuid
    /// Protected by _mapMutex for add/remove operations.
    /// Individual buffers are lock-free: each bot's buffer is only accessed
    /// from its own update thread during combat logging.
    ::std::unordered_map<ObjectGuid, BotCombatLogBuffer> _botBuffers;

    /// Mutex protecting _botBuffers map structure (add/remove/lookup).
    /// NOT held during event recording into individual buffers.
    mutable ::std::mutex _mapMutex;

    /// Whether the system has been initialized
    bool _initialized{false};
};

} // namespace Playerbot

#define sStructuredCombatLog Playerbot::StructuredCombatLog::instance()
