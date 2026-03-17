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
#include "SpellInfo.h"
#include "SharedDefines.h"
#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <chrono>
#include <functional>
#include <algorithm>
#include <mutex>

class Player;
class Unit;
class Aura;
class Group;

namespace Playerbot
{

class BotAI;

/**
 * @class DispelCoordinator
 * @brief Manages dispel and purge coordination among group bots
 *
 * This coordinator implements intelligent dispel/purge assignment based on:
 * - Debuff priority (death prevention > incapacitate > dangerous > moderate > minor)
 * - Dynamic priority adjustment based on target role and health
 * - Dispeller capabilities and availability
 * - Mana efficiency and cooldown management
 * - Group-wide coordination to prevent overlapping dispels
 * - Purge target prioritization for enemy buffs
 *
 * Performance: <0.012ms per update cycle
 * Memory: ~2KB per bot + ~5KB shared database
 */
class TC_GAME_API DispelCoordinator
{
public:
    explicit DispelCoordinator(BotAI* ai);
    ~DispelCoordinator();

    // ========================================================================
    // Core Priorities
    // ========================================================================

    enum DebuffPriority : uint8
    {
        PRIORITY_DEATH        = 6,  // Will cause death (Mortal Strike at low HP)
        PRIORITY_INCAPACITATE = 5,  // Complete loss of control (Fear, Polymorph)
        PRIORITY_DANGEROUS    = 4,  // High damage or severe impairment
        PRIORITY_MODERATE     = 3,  // Moderate impact (Curses, minor slows)
        PRIORITY_MINOR        = 2,  // Low impact debuffs
        PRIORITY_TRIVIAL      = 1   // Cosmetic or negligible
    };

    enum PurgePriority : uint8
    {
        PURGE_IMMUNITY     = 5,  // Ice Block, Divine Shield
        PURGE_MAJOR_BUFF   = 4,  // Bloodlust, Power Infusion
        PURGE_ENRAGE       = 3,  // Enrage effects
        PURGE_MODERATE_BUFF = 2, // Standard buffs
        PURGE_MINOR_BUFF   = 1   // Trivial buffs
    };

    // ========================================================================
    // Data Structures
    // ========================================================================

    struct DebuffData
    {
        uint32 auraId = 0;
        DispelType dispelType = DISPEL_NONE;
        DebuffPriority basePriority = PRIORITY_TRIVIAL;
        uint32 damagePerTick = 0;
        float slowPercent = 0.0f;
        bool preventsActions = false;
        bool preventsCasting = false;
        bool spreads = false;
        float spreadRadius = 0.0f;

        // Dynamic priority calculation
        float GetAdjustedPriority(Unit* target) const;
    };

    struct DispellerCapability
    {
        ObjectGuid botGuid;
        ::std::vector<DispelType> canDispel;
        uint32 dispelCooldown = 0;      // MS remaining
        uint32 lastDispelTime = 0;      // World time MS
        uint32 manaPercent = 100;
        bool inRange = false;
        uint32 globalCooldown = 0;      // GCD remaining
        Classes botClass = CLASS_NONE;

        bool CanDispelType(DispelType type) const
        {
            return ::std::find(canDispel.begin(), canDispel.end(), type) != canDispel.end();
        }
    };

    struct DispelAssignment
    {
        ObjectGuid dispeller;
        ObjectGuid target;
        uint32 auraId = 0;
        DebuffPriority priority = PRIORITY_TRIVIAL;
        uint32 assignedTime = 0;
        bool fulfilled = false;
        DispelType dispelType = DISPEL_NONE;
    };

    struct PurgeableBuff
    {
        uint32 auraId = 0;
        PurgePriority priority = PURGE_MINOR_BUFF;
        bool isEnrage = false;
        bool providesImmunity = false;
        bool increasesDamage = false;
        bool increasesHealing = false;
        float damageIncrease = 0.0f;
        float healingIncrease = 0.0f;
        float castSpeedIncrease = 0.0f;
    };

    struct DebuffTarget
    {
        ObjectGuid targetGuid;
        uint32 auraId = 0;
        DispelType dispelType = DISPEL_NONE;
        DebuffPriority priority = PRIORITY_TRIVIAL;
        float adjustedPriority = 0.0f;
        float targetHealthPct = 100.0f;
        bool isTank = false;
        bool isHealer = false;
        uint32 remainingDuration = 0;   // MS
        uint32 stackCount = 1;
    };

    struct PurgeTarget
    {
        ObjectGuid enemyGuid;
        uint32 auraId = 0;
        PurgePriority priority = PURGE_MINOR_BUFF;
        bool isEnrage = false;
        bool isImmunity = false;
        float threatLevel = 0.0f;
        float distance = 0.0f;
    };

    // ========================================================================
    // Core Interface
    // ========================================================================

    /**
     * Update dispel coordination state
     * @param diff Time elapsed since last update in milliseconds
     */
    void Update(uint32 diff);

    /**
     * Update dispel assignments for the group
     * Analyzes all debuffs and assigns appropriate dispellers
     */
    void UpdateDispelAssignments();

    /**
     * Register a new debuff for potential dispelling
     * @param target The unit with the debuff
     * @param auraId The aura ID of the debuff
     */
    void RegisterDebuff(ObjectGuid target, uint32 auraId);

    /**
     * Check if a specific aura should be dispelled
     * @param auraId The aura to check
     * @return true if aura should be dispelled
     */
    bool ShouldDispel(uint32 auraId) const;

    /**
     * Get current dispel assignment for this bot
     * @return Current assignment or empty if none
     */
    DispelAssignment GetDispelAssignment() const;

    /**
     * Check if an enemy buff should be purged
     * @param enemy The enemy unit
     * @param auraId The buff to check
     * @return true if buff should be purged
     */
    bool ShouldPurge(Unit* enemy, uint32 auraId) const;

    /**
     * Get the best purge target
     * @return Target info or empty if none
     */
    PurgeTarget GetPurgeTarget() const;

    // ========================================================================
    // Coordination Functions
    // ========================================================================

    /**
     * Find the best dispeller for a specific debuff
     * @param target The debuff target info
     * @return Best dispeller or empty GUID
     */
    ObjectGuid FindBestDispeller(const DebuffTarget& target) const;

    /**
     * Calculate dispeller effectiveness score
     * @param dispeller The dispeller capability
     * @param target The debuff target
     * @return Effectiveness score (0-100+)
     */
    float CalculateDispellerScore(const DispellerCapability& dispeller,
                                  const DebuffTarget& target) const;

    /**
     * Execute assigned dispel
     * @return true if dispel was executed
     */
    bool ExecuteDispel();

    /**
     * Execute purge on best target
     * @return true if purge was executed
     */
    bool ExecutePurge();

    // ========================================================================
    // Capability Management
    // ========================================================================

    /**
     * Update dispeller capabilities for all group members
     */
    void UpdateDispellerCapabilities();

    /**
     * Get dispel spell for current bot
     * @param dispelType Type of dispel needed
     * @return Spell ID or 0 if not available
     */
    uint32 GetDispelSpell(DispelType dispelType) const;

    /**
     * Get purge spell for current bot
     * @return Spell ID or 0 if not available
     */
    uint32 GetPurgeSpell() const;

    // ========================================================================
    // Group Coordination
    // ========================================================================

    /**
     * Check if another bot is already dispelling target
     * @param target Target GUID
     * @param auraId Aura to dispel
     * @return true if already being dispelled
     */
    bool IsBeingDispelled(ObjectGuid target, uint32 auraId) const;

    /**
     * Mark dispel as completed
     * @param assignment The completed assignment
     */
    void MarkDispelComplete(const DispelAssignment& assignment);

    /**
     * Clear expired assignments
     */
    void CleanupAssignments();

    // ========================================================================
    // Statistics & Debugging
    // ========================================================================

    struct Statistics
    {
        uint32 totalDebuffsDetected = 0;
        uint32 successfulDispels = 0;
        uint32 failedDispels = 0;
        uint32 successfulPurges = 0;
        uint32 failedPurges = 0;
        ::std::unordered_map<DispelType, uint32> dispelsByType;
        ::std::unordered_map<uint32, uint32> commonDebuffs;  // auraId -> count
        uint32 manaSpentOnDispels = 0;
        uint32 assignmentsCreated = 0;
        uint32 assignmentsExpired = 0;
    };

    const Statistics& GetStatistics() const { return m_statistics; }
    void ResetStatistics() { m_statistics = Statistics(); }

    // ========================================================================
    // Static Database
    // ========================================================================

    /**
     * Initialize global debuff and purge databases
     * Called once at startup
     */
    static void InitializeGlobalDatabase();

    /**
     * Check if databases are initialized
     */
    static bool IsDatabaseInitialized() { return s_databaseInitialized; }

    /**
     * Get debuff data by aura ID
     */
    static const DebuffData* GetDebuffData(uint32 auraId);

    /**
     * Get purgeable buff data by aura ID
     */
    static const PurgeableBuff* GetPurgeableBuffData(uint32 auraId);

private:
    // ========================================================================
    // Internal Methods
    // ========================================================================

    /**
     * Gather all debuffs on group members
     * @return Vector of debuff targets sorted by priority
     */
    ::std::vector<DebuffTarget> GatherGroupDebuffs() const;

    /**
     * Gather all purgeable buffs on enemies
     * @return Vector of purge targets sorted by priority
     */
    ::std::vector<PurgeTarget> GatherPurgeTargets() const;

    /**
     * Check if unit is tank role
     */
    bool IsTank(Unit* unit) const;

    /**
     * Check if unit is healer role
     */
    bool IsHealer(Unit* unit) const;

    /**
     * Get nearby allies count
     */
    uint32 GetNearbyAlliesCount(Unit* center, float radius) const;

    /**
     * Check if tank is taking damage
     */
    bool IsTankTakingDamage() const;

    /**
     * Evaluate purge cost/benefit
     */
    bool EvaluatePurgeBenefit(const PurgeableBuff& buff, Unit* enemy) const;

    /**
     * Mark dispeller as busy (GCD)
     */
    void MarkDispellerBusy(ObjectGuid dispeller, uint32 busyTimeMs);

    /**
     * Can dispeller handle this debuff
     */
    bool CanDispel(const DispellerCapability& dispeller, const DebuffTarget& target) const;

    /**
     * Get bot's class-specific dispel capabilities
     */
    ::std::vector<DispelType> GetClassDispelTypes(Classes botClass) const;

    // ========================================================================
    // Member Variables
    // ========================================================================

    BotAI* m_ai;                                        // Parent AI
    Player* m_bot;                                      // Bot player
    Group* m_group;                                     // Bot's group

    // Coordination state
    ::std::vector<DispellerCapability> m_dispellers;     // Group dispeller capabilities
    ::std::vector<DispelAssignment> m_assignments;       // Current assignments
    ::std::unordered_set<ObjectGuid> m_recentlyDispelled; // Recently dispelled (prevent spam)

    // Current bot state
    DispelAssignment m_currentAssignment;              // This bot's assignment
    uint32 m_lastDispelAttempt = 0;                    // Last dispel attempt time
    uint32 m_lastPurgeAttempt = 0;                     // Last purge attempt time
    uint32 m_globalCooldownUntil = 0;                  // GCD lockout

    // Caching
    uint32 m_lastCapabilityUpdate = 0;                 // Last capability refresh
    uint32 m_lastDebuffScan = 0;                       // Last debuff scan
    uint32 m_lastPurgeScan = 0;                        // Last purge scan

    // Statistics
    Statistics m_statistics;

    // Configuration
    struct Config
    {
        uint32 maxDispelRange = 40;                    // Maximum dispel range (yards)
        uint32 maxPurgeRange = 30;                     // Maximum purge range (yards)
        uint32 dispelGCD = 1500;                       // Global cooldown (ms)
        uint32 assignmentTimeout = 3000;               // Assignment expiry (ms)
        uint32 capabilityUpdateInterval = 500;         // Capability refresh (ms)
        uint32 debuffScanInterval = 200;               // Debuff scan interval (ms)
        uint32 purgeScanInterval = 300;                // Purge scan interval (ms)
        uint32 minManaPctForDispel = 20;               // Min mana % to dispel
        float priorityThreshold = 3.0f;                // Min priority to dispel
        uint32 maxDispelsPerSecond = 2;                // Rate limiting
        bool preferHealersForDispel = true;            // Healers prioritized
        bool smartPurging = true;                      // Intelligent purge selection
    } m_config;

    // ========================================================================
    // Static Database Members
    // ========================================================================

    static inline ::std::unordered_map<uint32, DebuffData> s_debuffDatabase;
    static inline ::std::unordered_map<uint32, PurgeableBuff> s_purgeDatabase;
    static inline bool s_databaseInitialized = false;
    static inline Playerbot::OrderedRecursiveMutex<Playerbot::LockOrder::BOT_AI_STATE> s_databaseMutex;
};

} // namespace Playerbot