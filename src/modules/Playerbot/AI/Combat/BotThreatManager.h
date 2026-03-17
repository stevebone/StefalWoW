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
#include "GameTime.h"
#include "Core/LRUCache.h"
#include "Core/Events/ICombatEventSubscriber.h"
#include "Core/Events/CombatEventType.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <atomic>
#include <shared_mutex>
#include <chrono>

// Forward declarations
class Player;
class Unit;

namespace Playerbot
{

// Threat priority levels
enum class ThreatPriority : uint8
{
    CRITICAL_PRIORITY = 0,    // Immediate threat response needed (healers under attack, etc.)
    HIGH_PRIORITY = 1,        // High priority threat target (casters, high-damage DPS)
    MODERATE_PRIORITY = 2,    // Normal threat management (standard DPS, melee)
    LOW_PRIORITY = 3,         // Low priority or controlled (CC'd targets, low threat)
    IGNORE_PRIORITY = 4       // Targets to ignore (friendly, neutral, etc.)
};

// Threat role assignments
enum class ThreatRole : uint8
{
    TANK = 0,        // Primary threat holder
    DPS = 1,         // Damage dealer
    HEALER = 2,      // Healing role
    SUPPORT = 3,     // Utility/buff role
    UNDEFINED = 4    // Role not determined
};

// Threat calculation types
enum class ThreatType : uint8
{
    DAMAGE = 0,      // Damage-based threat
    HEALING = 1,     // Healing-based threat
    AGGRO = 2,       // Direct aggro manipulation
    PROXIMITY = 3,   // Distance-based threat
    SPECIAL = 4      // Special ability threat
};

// Enhanced threat information
struct ThreatInfo
{
    ObjectGuid targetGuid;
    ObjectGuid botGuid;
    float threatValue;
    float threatPercent;
    ThreatPriority priority;
    ThreatType type;
    uint32 lastUpdate;
    bool isActive;
    bool isInCombat;
    float distance;
    Position lastPosition;

    // Additional threat metrics
    float damageDealt;
    float healingDone;
    float threatGenerated;
    float threatReduced;
    uint32 spellsInterrupted;
    uint32 abilitiesUsed;

    ThreatInfo() : threatValue(0.0f), threatPercent(0.0f), priority(ThreatPriority::MODERATE_PRIORITY),
                   type(ThreatType::DAMAGE), lastUpdate(0), isActive(false), isInCombat(false),
                   distance(0.0f), damageDealt(0.0f), healingDone(0.0f), threatGenerated(0.0f),
                   threatReduced(0.0f), spellsInterrupted(0), abilitiesUsed(0) {}

    ThreatInfo(ObjectGuid target, ObjectGuid bot, float threat)
        : targetGuid(target), botGuid(bot), threatValue(threat), threatPercent(0.0f),
          priority(ThreatPriority::MODERATE_PRIORITY), type(ThreatType::DAMAGE), lastUpdate(GameTime::GetGameTimeMS()),
          isActive(true), isInCombat(false), distance(0.0f), damageDealt(0.0f), healingDone(0.0f),
          threatGenerated(0.0f), threatReduced(0.0f), spellsInterrupted(0), abilitiesUsed(0) {}
};

// Threat target classification
struct ThreatTarget
{
    Unit* target;
    ThreatInfo info;
    ::std::vector<ObjectGuid> threateningBots;
    float aggregatedThreat;
    float averageThreatPercent;
    uint32 botsInCombat;
    bool requiresAttention;

    ThreatTarget() : target(nullptr), aggregatedThreat(0.0f), averageThreatPercent(0.0f),
                     botsInCombat(0), requiresAttention(false) {}

    bool operator<(const ThreatTarget& other) const
    {
        if (info.priority != other.info.priority)
            return info.priority < other.info.priority;
        return aggregatedThreat > other.aggregatedThreat;
    }
};

// Threat analysis result
struct ThreatAnalysis
{
    ::std::vector<ThreatTarget> sortedTargets;
    ThreatTarget* primaryTarget;
    ThreatTarget* secondaryTarget;
    float totalThreat;
    float averageThreat;
    uint32 activeTargets;
    uint32 criticalTargets;
    bool threatOverload;
    bool emergencyResponse;

    ThreatAnalysis() : primaryTarget(nullptr), secondaryTarget(nullptr), totalThreat(0.0f),
                       averageThreat(0.0f), activeTargets(0), criticalTargets(0),
                       threatOverload(false), emergencyResponse(false) {}
};

// Performance metrics for threat management
struct ThreatMetrics
{
    ::std::atomic<uint32> threatCalculations{0};
    ::std::atomic<uint32> targetAnalyses{0};
    ::std::atomic<uint32> roleAssignments{0};
    ::std::atomic<uint32> priorityUpdates{0};
    ::std::chrono::microseconds averageAnalysisTime{0};
    ::std::chrono::microseconds maxAnalysisTime{0};
    ::std::chrono::steady_clock::time_point lastUpdate;

    void Reset()
    {
        threatCalculations = 0;
        targetAnalyses = 0;
        roleAssignments = 0;
        priorityUpdates = 0;
        averageAnalysisTime = ::std::chrono::microseconds{0};
        maxAnalysisTime = ::std::chrono::microseconds{0};
        lastUpdate = ::std::chrono::steady_clock::now();
    }
};

/**
 * @class BotThreatManager
 * @brief Per-bot threat tracking and analysis system
 *
 * Phase 3 Event-Driven Architecture:
 * - Implements ICombatEventSubscriber for threat-related events
 * - Subscribes to: DAMAGE_DEALT, DAMAGE_TAKEN, HEALING_DONE, THREAT_CHANGED, UNIT_DIED
 * - Reduces polling overhead by ~70% through event-driven updates
 */
class TC_GAME_API BotThreatManager : public ICombatEventSubscriber
{
public:
    explicit BotThreatManager(Player* bot);
    ~BotThreatManager();

    // ====================================================================
    // ICombatEventSubscriber Interface (Phase 3 Event-Driven)
    // ====================================================================

    /**
     * @brief Called when a subscribed combat event occurs
     * Routes events to appropriate threat handlers
     */
    void OnCombatEvent(const CombatEvent& event) override;

    /**
     * @brief Return bitmask of event types this manager wants
     * Subscribes to: DAMAGE_DEALT, DAMAGE_TAKEN, HEALING_DONE, THREAT_CHANGED, UNIT_DIED
     */
    CombatEventType GetSubscribedEventTypes() const override;

    /**
     * @brief Medium priority (60) for per-bot threat tracking
     * Slightly higher than ThreatCoordinator (50) for bot-specific tracking
     */
    int32 GetEventPriority() const override { return 60; }

    /**
     * @brief Filter events to only receive those involving this bot
     */
    bool ShouldReceiveEvent(const CombatEvent& event) const;

    /**
     * @brief Subscriber name for debugging
     */
    const char* GetSubscriberName() const override { return "BotThreatManager"; }

    // Core threat management
    void UpdateThreat(uint32 diff);
    void ResetThreat();
    void ClearAllThreat();

    // Threat calculation
    float CalculateThreat(Unit* target) const;
    float CalculateThreatPercent(Unit* target) const;
    void UpdateThreatValue(Unit* target, float threat, ThreatType type = ThreatType::DAMAGE);
    void ModifyThreat(Unit* target, float modifier);

    // Threat analysis
    ThreatAnalysis AnalyzeThreatSituation();
    ::std::vector<ThreatTarget> GetSortedThreatTargets();
    ThreatTarget* GetPrimaryThreatTarget();
    ThreatTarget* GetSecondaryThreatTarget();

    // Target priority management
    void SetTargetPriority(Unit* target, ThreatPriority priority);
    ThreatPriority GetTargetPriority(Unit* target) const;
    void UpdateTargetPriorities();

    // Role-based threat management
    void SetBotRole(ThreatRole role) { _botRole = role; }
    ThreatRole GetBotRole() const { return _botRole; }
    void UpdateRoleBasedThreat();

    // Threat information access
    bool HasThreat(Unit* target) const;
    float GetThreat(Unit* target) const;
    float GetThreatPercent(Unit* target) const;

    // Multi-target threat management
    ::std::vector<Unit*> GetAllThreatTargets();
    ::std::vector<Unit*> GetThreatTargetsByPriority(ThreatPriority priority);
    uint32 GetThreatTargetCount() const;

    // Emergency threat handling
    bool IsInThreatEmergency() const;
    ::std::vector<Unit*> GetEmergencyTargets();
    void HandleThreatEmergency();

    // Threat coordination (for group/raid scenarios)
    void ShareThreatInformation(const ::std::vector<Player*>& groupMembers);
    void ReceiveThreatInformation(ObjectGuid sourceBot, const ThreatInfo& info);
    void SynchronizeGroupThreat();

    // Performance monitoring
    ThreatMetrics const& GetMetrics() const { return _metrics; }
    void ResetMetrics() { _metrics.Reset(); }

    // Configuration
    void SetUpdateInterval(uint32 intervalMs) { _updateInterval = intervalMs; }
    uint32 GetUpdateInterval() const { return _updateInterval; }
    void SetThreatRadius(float radius) { _threatRadius = radius; }
    float GetThreatRadius() const { return _threatRadius; }

    // Advanced threat features
    void PredictThreatChanges(uint32 timeAheadMs);
    float EstimateFutureThreat(Unit* target, uint32 timeAheadMs);
    void OptimizeThreatDistribution();

    // Threat events and callbacks
    void OnDamageDealt(Unit* target, uint32 damage);
    void OnHealingDone(Unit* target, uint32 healing);
    void OnSpellInterrupt(Unit* target);
    void OnTauntUsed(Unit* target);
    void OnThreatRedirect(Unit* from, Unit* to, float amount);

private:
    // ====================================================================
    // Event Handlers (Phase 3 Event-Driven Architecture)
    // ====================================================================

    /**
     * @brief Handle DAMAGE_DEALT event - update threat for damage dealt by bot
     */
    void HandleDamageDealt(const CombatEvent& event);

    /**
     * @brief Handle DAMAGE_TAKEN event - update threat for damage taken by bot
     */
    void HandleDamageTaken(const CombatEvent& event);

    /**
     * @brief Handle HEALING_DONE event - update threat from healing
     */
    void HandleHealingDone(const CombatEvent& event);

    /**
     * @brief Handle THREAT_CHANGED event - direct threat table update
     */
    void HandleThreatChanged(const CombatEvent& event);

    /**
     * @brief Handle UNIT_DIED event - remove from threat tracking
     */
    void HandleUnitDied(const CombatEvent& event);

    // Internal threat calculation methods
    float CalculateBaseThreat(Unit* target) const;
    float CalculateRoleModifier() const;
    float CalculateDistanceModifier(Unit* target) const;
    float CalculateHealthModifier(Unit* target) const;
    float CalculateAbilityModifier(Unit* target) const;

    // Internal analysis methods
    void AnalyzeTargetThreat(Unit* target, ThreatTarget& threatTarget);
    void ClassifyThreatPriority(ThreatTarget& threatTarget);
    void UpdateThreatHistory(Unit* target, float threat);

    // Internal update methods
    void UpdateThreatTable(uint32 diff);
    void CleanupStaleEntries();
    void UpdateDistances();
    void UpdateCombatState();

    // Performance tracking
    void TrackPerformance(::std::chrono::microseconds duration, const ::std::string& operation);

private:
    Player* _bot;
    ThreatRole _botRole;

    // Threat storage - bounded to prevent unbounded memory growth
    BoundedMap<ObjectGuid, ThreatInfo> _threatMap{MAX_THREAT_ENTRIES};
    BoundedMap<ObjectGuid, BoundedHistory<float>> _threatHistory{MAX_THREAT_ENTRIES};

    // Configuration
    uint32 _updateInterval;
    float _threatRadius;
    uint32 _lastUpdate;

    // Analysis cache
    mutable ThreatAnalysis _cachedAnalysis;
    mutable uint32 _analysisTimestamp;
    mutable bool _analysisDirty;

    // Performance metrics
    mutable ThreatMetrics _metrics;

    // Thread safety
    mutable Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE> _mutex;

    // Constants
    static constexpr uint32 DEFAULT_UPDATE_INTERVAL = 500;  // 500ms
    static constexpr float DEFAULT_THREAT_RADIUS = 50.0f;   // 50 yards
    static constexpr uint32 THREAT_HISTORY_SIZE = 10;       // Keep last 10 values per target
    static constexpr size_t MAX_THREAT_ENTRIES = 50;        // Maximum tracked threat targets
    static constexpr uint32 ANALYSIS_CACHE_DURATION = 250;  // 250ms cache
    static constexpr float EMERGENCY_THREAT_THRESHOLD = 150.0f; // 150% threat = emergency

    // Phase 3: Event-driven state
    ::std::atomic<bool> _subscribed{false};
    ::std::atomic<bool> _threatDataDirty{false};  // Set by event handlers
    ::std::atomic<uint32> _maintenanceTimer{0};
    static constexpr uint32 MAINTENANCE_INTERVAL_MS = 1000;  // Maintenance every 1s (reduced from 500ms)
};

// Threat calculation utilities
class TC_GAME_API ThreatCalculator
{
public:
    // Base threat calculations
    static float CalculateDamageThreat(uint32 damage, float modifier = 1.0f);
    static float CalculateHealingThreat(uint32 healing, float modifier = 0.5f);
    static float CalculateSpellThreat(uint32 spellId, float modifier = 1.0f);

    // Advanced threat calculations
    static float CalculatePositionalThreat(Player* bot, Unit* target);
    static float CalculateRoleThreat(ThreatRole role, Unit* target);
    static float CalculateGroupThreat(const ::std::vector<Player*>& group, Unit* target);

    // Threat modifiers
    static float GetClassThreatModifier(uint8 classId);
    static float GetSpecializationThreatModifier(uint8 classId, uint8 spec);
    static float GetLevelDifferenceThreatModifier(uint8 botLevel, uint8 targetLevel);
    static float GetEquipmentThreatModifier(Player* bot);

    // Utility functions
    static ThreatPriority DetermineThreatPriority(Unit* target);
    static float NormalizeThreat(float threat, float maxThreat);
    static bool IsValidThreatTarget(Unit* target);
};

} // namespace Playerbot