/*
 * Copyright (C) 2024 TrinityCore <https://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef TRINITYCORE_TARGETSCANNER_H
#define TRINITYCORE_TARGETSCANNER_H

#include "Define.h"
#include "ObjectGuid.h"
#include "../../Spatial/DoubleBufferedSpatialGrid.h"  // For CreatureSnapshot type
#include <memory>
#include <vector>
#include <unordered_set>

class Player;
class Unit;
class Creature;

namespace Playerbot
{

    struct ScanResult
    {
        Unit* target = nullptr;
        float distance = 0.0f;
        float threat = 0.0f;
        uint8 priority = 0;

        bool operator<(const ScanResult& other) const
        {
            // Higher priority first, then closer distance
            if (priority != other.priority)
                return priority > other.priority;
            return distance < other.distance;
        }
    };

    enum class ScanMode
    {
        AGGRESSIVE,     // Attack anything hostile
        DEFENSIVE,      // Only attack if threatened
        PASSIVE,        // Never auto-attack
        ASSIST          // Only attack group's target
    };

    enum TargetPriority
    {
        PRIORITY_CRITICAL = 10,  // Attacking bot or allies
        PRIORITY_CASTER = 8,     // Casters and healers
        PRIORITY_ELITE = 6,      // Elite mobs
        PRIORITY_NORMAL = 4,     // Normal mobs
        PRIORITY_TRIVIAL = 2,    // Grey-level mobs
        PRIORITY_AVOID = 0       // Should not engage
    };

    class TargetScanner
    {
    public:
        explicit TargetScanner(Player* bot);
        ~TargetScanner();

        // Main scanning functions
        // CRITICAL DEADLOCK FIX: Return GUIDs instead of Unit* pointers
        // Worker threads can call these methods safely without accessing Map
        // Main thread converts GUID → Unit* and queues actions
        ObjectGuid FindNearestHostile(float range = 0.0f);
        ObjectGuid FindBestTarget(float range = 0.0f);
        ::std::vector<ObjectGuid> FindAllHostiles(float range = 0.0f);

        // Target validation
        bool IsValidTarget(Unit* target) const;
        bool ShouldEngage(Unit* target) const;
        bool CanReachTarget(Unit* target) const;
        bool IsValidTargetSnapshot(DoubleBufferedSpatialGrid::CreatureSnapshot const& creature) const;

        // Target prioritization
        uint8 GetTargetPriority(Unit* target) const;
        float GetThreatValue(Unit* target) const;

        // Scanning configuration
        float GetScanRadius() const;
        float GetMaxEngageRange() const;
        void SetScanMode(ScanMode mode) { m_scanMode = mode; }
        ScanMode GetScanMode() const { return m_scanMode; }

        // Performance optimization
        bool ShouldScan(uint32 currentTime) const;
        void UpdateScanTime(uint32 currentTime);

        // Special target checks
        bool IsAttackingGroup(Unit* target) const;
        bool IsCaster(Unit* target) const;
        bool IsHealer(Unit* target) const;
        bool IsDangerous(Unit* target) const;

        // Blacklist management
        void AddToBlacklist(ObjectGuid guid, uint32 duration = 30000);
        void RemoveFromBlacklist(ObjectGuid guid);
        bool IsBlacklisted(ObjectGuid guid) const;
        void UpdateBlacklist(uint32 currentTime);

    private:
        // Internal helper functions
        float CalculateEngageDistance(Unit* target) const;
        bool CheckLineOfSight(Unit* target) const;
        bool IsTargetInCombatWithOthers(Unit* target) const;
        float GetClassBasedRange() const;

        // Grid search implementation
        template<class T>
        void VisitNearbyGridObjects(float range);

    private:
        Player* m_bot;
        ScanMode m_scanMode;
        uint32 m_lastScanTime;
        uint32 m_scanInterval;

        // Blacklist for temporarily ignored targets
        struct BlacklistEntry
        {
            ObjectGuid guid;
            uint32 expireTime;
        };
        ::std::vector<BlacklistEntry> m_blacklist;

        // Performance optimization
        mutable ::std::vector<ScanResult> m_lastScanResults;
        uint32 m_lastResultsTime;

        // Configuration based on class/spec
        float m_baseRange;
        float m_maxRange;
        bool m_preferRanged;
        bool m_avoidElites;

        // Scan intervals (milliseconds)
        static constexpr uint32 SCAN_INTERVAL_COMBAT = 500;
        static constexpr uint32 SCAN_INTERVAL_NORMAL = 1000;
        static constexpr uint32 SCAN_INTERVAL_IDLE = 2000;
        static constexpr uint32 SCAN_RESULTS_CACHE = 250;
    };
}

#endif // TRINITYCORE_TARGETSCANNER_H