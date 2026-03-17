/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2021+ WarheadCore <https://github.com/AzerothCore/WarheadCore>
 * Copyright (C) 2025+ TrinityCore Playerbot Integration
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_AI_COORDINATION_BG_SIEGE_SIEGESCRIPTBASE_H
#define PLAYERBOT_AI_COORDINATION_BG_SIEGE_SIEGESCRIPTBASE_H

#include "BGScriptBase.h"
#include <set>

namespace Playerbot::Coordination::Battleground
{

/**
 * @brief Base class for Siege battlegrounds
 *
 * Provides common siege mechanics for:
 * - Alterac Valley (40v40, boss kill)
 * - Isle of Conquest (40v40, boss kill, vehicles)
 * - Strand of the Ancients (15v15, demolishers, rounds)
 *
 * Key Siege Mechanics:
 * - Gates that can be destroyed
 * - Vehicles for attacking structures
 * - Boss NPCs as win conditions
 * - Reinforcement systems
 * - Graveyards that can be captured
 * - Towers that provide buffs when controlled
 */
class SiegeScriptBase : public BGScriptBase
{
public:
    SiegeScriptBase() = default;
    virtual ~SiegeScriptBase() = default;

    // ========================================================================
    // Siege specific
    // ========================================================================

    virtual bool HasVehicles() const override { return true; }
    virtual bool IsEpic() const { return GetTeamSize() >= 40; }
    virtual uint8 GetTeamSize() const override = 0; // To be implemented by derived classes

    // ========================================================================
    // LIFECYCLE
    // ========================================================================

    void OnLoad(BattlegroundCoordinator* coordinator) override;
    void OnUpdate(uint32 diff) override;

    // ========================================================================
    // VEHICLE DATA
    // ========================================================================

    std::vector<BGVehicleData> GetVehicleData() const;

    // ========================================================================
    // STRATEGY - Siege overrides
    // ========================================================================

    RoleDistribution GetRecommendedRoles(
        const StrategicDecision& decision,
        float scoreAdvantage,
        uint32 timeRemaining) const override;

    void AdjustStrategy(StrategicDecision& decision,
        float scoreAdvantage, uint32 controlledCount,
        uint32 totalObjectives, uint32 timeRemaining) const override;

    float CalculateWinProbability(uint32 allianceScore, uint32 hordeScore,
        uint32 timeRemaining, uint32 objectivesControlled, uint32 faction) const override;

    // ========================================================================
    // SIEGE-SPECIFIC IMPLEMENTATIONS
    // ========================================================================

    uint8 GetGatePriority(uint32 gateId) const;
    bool ShouldUseVehicle(ObjectGuid botGuid, uint32 vehicleEntry) const;

    // ========================================================================
    // EVENT HANDLING
    // ========================================================================

    void OnEvent(const BGScriptEventData& event) override;
    void OnMatchStart() override;

protected:
    // ========================================================================
    // ABSTRACT - MUST BE IMPLEMENTED BY DERIVED CLASSES
    // ========================================================================

    /**
     * @brief Get the boss NPC entry for a faction
     */
    virtual uint32 GetBossEntry(uint32 faction) const = 0;

    /**
     * @brief Get boss position
     */
    virtual Position GetBossPosition(uint32 faction) const = 0;

    /**
     * @brief Get all gate data
     */
    virtual std::vector<BGObjectiveData> GetGateData() const = 0;

    /**
     * @brief Get all tower data
     */
    virtual std::vector<BGObjectiveData> GetTowerData() const = 0;

    /**
     * @brief Get all graveyard data
     */
    virtual std::vector<BGObjectiveData> GetGraveyardData() const = 0;

    /**
     * @brief Get starting reinforcements
     */
    virtual uint32 GetStartingReinforcements() const { return 0; }

    /**
     * @brief Get reinforcement loss per death
     */
    virtual uint32 GetReinforcementLossPerDeath() const { return 1; }

    /**
     * @brief Get reinforcement loss per tower destroyed
     */
    virtual uint32 GetReinforcementLossPerTower() const { return 0; }

    // ========================================================================
    // SIEGE HELPERS
    // ========================================================================

    /**
     * @brief Check if a gate is destroyed
     */
    bool IsGateDestroyed(uint32 gateId) const;

    /**
     * @brief Get number of standing towers for a faction
     */
    uint32 GetStandingTowerCount(uint32 faction) const;

    /**
     * @brief Get number of destroyed towers
     */
    uint32 GetDestroyedTowerCount(uint32 faction) const;

    /**
     * @brief Get current reinforcements
     */
    uint32 GetReinforcements(uint32 faction) const;

    /**
     * @brief Check if boss can be attacked (all required gates down)
     */
    virtual bool CanAttackBoss(uint32 faction) const;

    /**
     * @brief Get recommended vehicle for a bot
     */
    uint32 GetRecommendedVehicle(ObjectGuid botGuid) const;

    /**
     * @brief Check if we should rush boss
     */
    bool ShouldRushBoss() const;

    /**
     * @brief Try to board a nearby vehicle of the given creature entry
     *
     * Thread-safe: uses cached vehicle GUIDs (resolved on main thread).
     * Queues EnterVehicle action via BotActionMgr if a free vehicle is found.
     *
     * @param bot The bot player
     * @param vehicleEntry Creature entry ID of the vehicle (e.g., DEMOLISHER_ENTRY)
     * @param maxRange Maximum search range from bot position
     * @return true if a vehicle boarding action was queued
     */
    bool TryBoardNearbyVehicle(::Player* bot, uint32 vehicleEntry, float maxRange) const;

    /**
     * @brief Get gate destruction order for optimal attack
     */
    std::vector<uint32> GetGateDestructionOrder(uint32 attackingFaction) const;

    /**
     * @brief Get tower destruction priority
     */
    std::vector<uint32> GetTowerDestructionPriority(uint32 attackingFaction) const;

    /**
     * @brief Calculate effective boss HP
     * @param faction Faction of the boss
     * @return Effective HP considering buffs from towers
     */
    uint32 CalculateEffectiveBossHP(uint32 faction) const;

    // ========================================================================
    // SIEGE STATE
    // ========================================================================

    // Gate states
    std::set<uint32> m_destroyedGates;

    // Tower states
    std::map<uint32, BGObjectiveState> m_towerStates;
    std::set<uint32> m_destroyedTowers;

    // Graveyard control
    std::map<uint32, uint32> m_graveyardControl;  // gyId -> faction

    // Reinforcements
    uint32 m_allianceReinforcements = 0;
    uint32 m_hordeReinforcements = 0;

    // Boss state
    bool m_allianceBossEngaged = false;
    bool m_hordeBossEngaged = false;
    float m_allianceBossHealthPct = 100.0f;
    float m_hordeBossHealthPct = 100.0f;

    // Vehicle tracking
    std::map<ObjectGuid, uint32> m_vehicleAssignments;  // bot -> vehicle entry

    // Cached vehicle GUIDs (populated on main thread by OnUpdate, used from worker threads)
    struct CachedVehicle
    {
        ObjectGuid guid;
        uint32 entry;
        Position pos;
        bool hasPassengers;
    };
    std::vector<CachedVehicle> m_cachedVehicles;
    uint32 m_vehicleCacheTimer = 0;
    static constexpr uint32 VEHICLE_CACHE_INTERVAL = 3000;  // Refresh every 3s

    /// Resolve vehicle GUIDs on main thread
    void UpdateVehicleCache();

private:
    // Update timers
    uint32 m_siegeUpdateTimer = 0;
    static constexpr uint32 SIEGE_UPDATE_INTERVAL = 2000;

    void UpdateSiegeState();
};

// ============================================================================
// SIEGE CONSTANTS
// ============================================================================

namespace SiegeConstants
{
    // Vehicle priorities
    constexpr uint8 VEHICLE_PRIORITY_DEMOLISHER = 9;
    constexpr uint8 VEHICLE_PRIORITY_SIEGE_ENGINE = 8;
    constexpr uint8 VEHICLE_PRIORITY_CATAPULT = 7;

    // Gate attack priorities
    constexpr uint8 GATE_PRIORITY_INNER = 10;
    constexpr uint8 GATE_PRIORITY_OUTER = 8;

    // Boss rush threshold (health %)
    constexpr float BOSS_RUSH_THRESHOLD = 0.3f;

    // Reinforcement thresholds
    constexpr float REINF_CRITICAL = 0.1f;    // 10% remaining
    constexpr float REINF_DANGER = 0.25f;     // 25% remaining
    constexpr float REINF_CAUTION = 0.5f;     // 50% remaining
}

} // namespace Playerbot::Coordination::Battleground

#endif // PLAYERBOT_AI_COORDINATION_BG_SIEGE_SIEGESCRIPTBASE_H
