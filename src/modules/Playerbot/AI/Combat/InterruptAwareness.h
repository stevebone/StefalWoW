/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "Define.h"
#include "Threading/LockHierarchy.h"
#include "ObjectGuid.h"
#include "Position.h"
#include "SharedDefines.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <atomic>
#include <mutex>
#include <shared_mutex>

class Unit;
class Player;
class Spell;
class WorldObject;

namespace Playerbot
{

class BotAI;
class InterruptCoordinatorFixed;

// Type alias for backward compatibility
using InterruptCoordinator = InterruptCoordinatorFixed;

// Spell detection configuration
struct SpellDetectionConfig
{
    float maxDetectionRange = 50.0f;        // Maximum range to detect spells
    uint32 detectionIntervalMs = 100;       // How often to scan for spells
    bool detectFriendlySpells = false;      // Whether to detect friendly casts
    bool detectChanneledSpells = true;      // Whether to detect channeled spells
    bool detectInstantCasts = false;        // Whether to detect instant casts
    uint32 minCastTime = 500;               // Minimum cast time to consider (ms)
    bool requireLineOfSight = true;         // Whether to require LOS to detect
};

// Information about a detected spell cast
struct DetectedSpellCast
{
    ObjectGuid casterGuid;
    uint32 spellId;
    ::std::chrono::steady_clock::time_point detectionTime;
    ::std::chrono::steady_clock::time_point estimatedCastEnd;
    Position casterPosition;
    uint32 castTimeMs;
    uint32 remainingTimeMs;
    bool isChanneled;
    bool isInterruptible;
    bool isHostile;
    uint32 schoolMask;
    float detectionRange;

    DetectedSpellCast() = default;

    bool IsExpired() const
    {
        return ::std::chrono::steady_clock::now() >= estimatedCastEnd;
    }

    bool IsValid() const
    {
        return !casterGuid.IsEmpty() && spellId != 0 && !IsExpired();
    }

    uint32 GetRemainingTime() const
    {
        auto now = ::std::chrono::steady_clock::now();
        if (now >= estimatedCastEnd)
            return 0;
        return static_cast<uint32>(::std::chrono::duration_cast<::std::chrono::milliseconds>(estimatedCastEnd - now).count());
    }

    float GetCastProgress() const
    {
        if (castTimeMs == 0)
            return 1.0f;

        auto now = ::std::chrono::steady_clock::now();
        auto elapsed = ::std::chrono::duration_cast<::std::chrono::milliseconds>(now - detectionTime);
        return ::std::min(1.0f, static_cast<float>(elapsed.count()) / castTimeMs);
    }
};

// Spell scanning result
struct SpellScanResult
{
    ::std::vector<DetectedSpellCast> newCasts;
    ::std::vector<ObjectGuid> completedCasts;
    ::std::vector<ObjectGuid> interruptedCasts;
    uint32 totalUnitsScanned = 0;
    uint32 totalSpellsDetected = 0;
    ::std::chrono::microseconds scanTime{0};
};

// Performance metrics for spell detection
struct DetectionMetrics
{
    ::std::atomic<uint32> totalScans{0};
    ::std::atomic<uint32> unitsScanned{0};
    ::std::atomic<uint32> spellsDetected{0};
    ::std::atomic<uint32> interruptibleSpells{0};
    ::std::atomic<uint32> hostileSpells{0};
    ::std::chrono::microseconds averageScanTime{0};
    ::std::chrono::microseconds maxScanTime{0};

    float GetDetectionRate() const
    {
        uint32 scans = totalScans.load();
        return scans > 0 ? static_cast<float>(spellsDetected.load()) / scans : 0.0f;
    }

    float GetInterruptibleRate() const
    {
        uint32 detected = spellsDetected.load();
        return detected > 0 ? static_cast<float>(interruptibleSpells.load()) / detected : 0.0f;
    }
};

/**
 * Advanced spell detection and awareness system for bot groups
 *
 * Monitors nearby units for spell casting activity and provides real-time
 * information to the InterruptCoordinator and other combat systems.
 *
 * Features:
 * - Real-time spell cast detection within configurable range
 * - Hostile vs friendly spell classification
 * - Interruptible spell identification
 * - Performance optimization for 5+ bot scanning
 * - Integration with interrupt coordination system
 * - Predictive casting pattern recognition
 */
class TC_GAME_API InterruptAwareness
{
public:
    explicit InterruptAwareness(Player* observer = nullptr);
    ~InterruptAwareness() = default;

    // === Core Detection Interface ===

    // Update detection system (called from bot AI update loop)
    SpellScanResult Update(uint32 diff);

    // Set the observing player (bot performing the detection)
    void SetObserver(Player* observer);
    Player* GetObserver() const { return _observer; }

    // Configuration
    void SetConfig(SpellDetectionConfig const& config) { _config = config; }
    SpellDetectionConfig const& GetConfig() const { return _config; }

    // === Spell Query Interface ===

    // Get all currently detected spell casts
    ::std::vector<DetectedSpellCast> GetActiveCasts() const;

    // Get spell casts from specific caster
    ::std::vector<DetectedSpellCast> GetCastsFromUnit(ObjectGuid casterGuid) const;

    // Get interruptible spell casts within range
    ::std::vector<DetectedSpellCast> GetInterruptibleCasts(float maxRange = 0.0f) const;

    // Get hostile spell casts targeting friendly units
    ::std::vector<DetectedSpellCast> GetHostileCasts() const;

    // Check if specific unit is currently casting
    bool IsUnitCasting(ObjectGuid unitGuid) const;

    // Get specific spell cast information
    DetectedSpellCast const* GetSpellCast(ObjectGuid casterGuid, uint32 spellId) const;

    // === Integration Interface ===

    // Set interrupt coordinator for automatic spell reporting
    void SetInterruptCoordinator(::std::shared_ptr<InterruptCoordinator> coordinator);

    // Register for spell cast notifications
    void RegisterSpellCastCallback(::std::function<void(DetectedSpellCast const&)> callback);

    // Register for spell completion notifications
    void RegisterSpellCompleteCallback(::std::function<void(ObjectGuid, uint32, bool)> callback);

    // === Status and Metrics ===

    // Current detection state
    bool IsActive() const { return _active.load(); }
    void SetActive(bool active) { _active.store(active); }

    size_t GetActiveCastCount() const;
    uint32 GetLastScanTimeMs() const { return _lastScanTimeMs; }

    // Performance metrics
    DetectionMetrics const& GetMetrics() const { return _metrics; }
    void ResetMetrics();

    // === Advanced Features ===

    // Pattern recognition for encounter-specific spell sequences
    void EnablePatternRecognition(bool enabled) { _enablePatterns = enabled; }
    void AddSpellPattern(uint32 npcId, ::std::vector<uint32> const& spellSequence);
    void ClearSpellPatterns();

    // Priority-based scanning (focus on specific unit types)
    void SetScanPriority(CreatureType creatureType, uint32 priority);
    void SetScanPriority(uint32 npcId, uint32 priority);

    // Predictive detection (anticipate spell casts based on patterns)
    ::std::vector<uint32> PredictUpcomingSpells(ObjectGuid casterGuid) const;

private:
    // === Internal Detection Methods ===

    // Core scanning functionality
    SpellScanResult ScanNearbyUnits();
    void ProcessUnit(Unit* unit, SpellScanResult& result);

    // Spell analysis
    DetectedSpellCast AnalyzeSpellCast(Unit* caster, Spell const* spell) const;
    bool ShouldDetectSpell(Unit* caster, Spell const* spell) const;
    bool IsSpellInterruptible(Spell const* spell) const;
    bool IsSpellHostile(Unit* caster, Spell const* spell) const;

    // Cast tracking and management
    void UpdateActiveCasts();
    void AddDetectedCast(DetectedSpellCast const& cast);
    void RemoveExpiredCasts();
    void ProcessCompletedCasts(SpellScanResult& result);

    // Unit filtering and prioritization
    ::std::vector<Unit*> GetNearbyUnits() const;
    bool ShouldScanUnit(Unit* unit) const;
    uint32 GetUnitScanPriority(Unit* unit) const;

    // Pattern recognition
    void UpdateSpellPatterns(DetectedSpellCast const& cast);
    bool MatchesKnownPattern(ObjectGuid casterGuid, uint32 spellId) const;

    // Performance optimization
    void OptimizeForPerformance();
    bool ShouldSkipScan() const;

    // Notification handling
    void NotifySpellDetected(DetectedSpellCast const& cast);
    void NotifySpellCompleted(ObjectGuid casterGuid, uint32 spellId, bool interrupted);

private:
    // Observer (bot performing the detection)
    Player* _observer = nullptr;
    ::std::atomic<bool> _active{true};

    // Configuration
    SpellDetectionConfig _config;

    // Active spell tracking
    ::std::unordered_map<ObjectGuid, ::std::vector<DetectedSpellCast>> _activeCasts; // casterGuid -> casts
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE> _castMutex;

    // Update tracking
    ::std::chrono::steady_clock::time_point _lastUpdate;
    uint32 _lastScanTimeMs = 0;
    uint32 _scanCount = 0;

    // Performance metrics
    mutable DetectionMetrics _metrics;
    // DEADLOCK FIX: Changed to recursive_mutex
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE> _metricsMutex;

    // Integration components
    ::std::weak_ptr<InterruptCoordinator> _coordinator;
    ::std::vector<::std::function<void(DetectedSpellCast const&)>> _spellCastCallbacks;
    ::std::vector<::std::function<void(ObjectGuid, uint32, bool)>> _spellCompleteCallbacks;
    // DEADLOCK FIX: Changed to recursive_mutex
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE> _callbackMutex;

    // Priority scanning
    ::std::unordered_map<CreatureType, uint32> _creatureTypePriorities;
    ::std::unordered_map<uint32, uint32> _npcPriorities; // npcId -> priority
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE> _priorityMutex;

    // Pattern recognition
    bool _enablePatterns = false;
    struct SpellPattern
    {
        uint32 npcId;
        ::std::vector<uint32> spellSequence;
        ::std::chrono::steady_clock::time_point lastMatch;
        uint32 currentIndex = 0;

        // Extended pattern tracking fields
        uint32 lastUpdateTime = 0;              // Time of last pattern update (getMSTime())
        float confidenceScore = 0.5f;           // Pattern confidence (0.0-1.0)
        ::std::vector<uint32> castTimings;      // Timing between casts for temporal analysis
    };
    ::std::unordered_map<uint32, SpellPattern> _spellPatterns; // npcId -> pattern
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE> _patternMutex;

    // Performance optimization
    static constexpr uint32 MAX_SCAN_UNITS = 50;        // Maximum units to scan per update
    static constexpr uint32 MAX_DETECTION_RANGE = 50;   // Maximum detection range in yards
    static constexpr uint32 SCAN_OPTIMIZATION_INTERVAL = 100; // Optimize every N scans
    static constexpr uint32 MAX_ACTIVE_CASTS = 100;     // Maximum tracked active casts

    // Thread safety
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE> _observerMutex;

    // Deleted operations
    InterruptAwareness(InterruptAwareness const&) = delete;
    InterruptAwareness& operator=(InterruptAwareness const&) = delete;
    InterruptAwareness(InterruptAwareness&&) = delete;
    InterruptAwareness& operator=(InterruptAwareness&&) = delete;
};

} // namespace Playerbot