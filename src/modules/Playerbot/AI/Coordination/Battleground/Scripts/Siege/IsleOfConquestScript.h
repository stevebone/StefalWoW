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

#ifndef PLAYERBOT_AI_COORDINATION_BG_SIEGE_ISLEOFCONQUESTSCRIPT_H
#define PLAYERBOT_AI_COORDINATION_BG_SIEGE_ISLEOFCONQUESTSCRIPT_H

#include "SiegeScriptBase.h"
#include "IsleOfConquestData.h"
#include <atomic>
#include <shared_mutex>

namespace Playerbot::Coordination::Battleground
{

/**
 * @class IsleOfConquestScript
 * @brief Enterprise-grade Isle of Conquest battleground script
 *
 * Isle of Conquest is a 40v40 epic battleground featuring:
 * - Victory Conditions: Kill enemy boss or deplete reinforcements to 0
 * - 5 Capturable Nodes: Docks, Hangar, Workshop, Quarry, Refinery
 * - 6 Gates: 3 per faction keep (Front, West, East)
 * - 2 Bosses: High Commander Halford Wyrmbane (Alliance), Overlord Agmar (Horde)
 * - Siege Vehicles: Demolishers (Workshop), Glaive Throwers, Catapults
 * - Gunship: Parachute assault from Hangar control
 * - Reinforcement System: 600 starting, lost per death and boss damage
 *
 * Key strategies:
 * - Workshop Rush: Capture Workshop for siege vehicles, break gates
 * - Hangar Control: Parachute directly into enemy keep
 * - Node Farming: Control Quarry/Refinery for reinforcement drain
 * - Balanced: Split forces between vehicles and node control
 *
 * This script provides:
 * - 50 node defense positions (10 per node)
 * - 48 gate approach positions (8 per gate)
 * - 24 boss room positions (12 per boss)
 * - 14 chokepoints
 * - 8 sniper positions
 * - 12 vehicle staging areas
 * - 12 ambush positions (6 per faction)
 * - Phase-aware strategy (opening, node capture, vehicle siege, gate assault, boss assault, defense)
 *
 * Map ID: 628
 */
class IsleOfConquestScript : public SiegeScriptBase
{
public:
    IsleOfConquestScript() = default;
    ~IsleOfConquestScript() = default;

    // ========================================================================
    // IDENTIFICATION
    // ========================================================================

    uint32 GetMapId() const override { return IsleOfConquest::MAP_ID; }
    std::string GetName() const override { return IsleOfConquest::BG_NAME; }
    BGType GetBGType() const override { return BGType::ISLE_OF_CONQUEST; }
    uint32 GetMaxScore() const override { return IsleOfConquest::STARTING_REINFORCEMENTS; }
    uint32 GetMaxDuration() const override { return IsleOfConquest::MAX_DURATION; }
    uint8 GetTeamSize() const override { return IsleOfConquest::TEAM_SIZE; }
    bool HasVehicles() const override { return true; }

    // ========================================================================
    // LIFECYCLE
    // ========================================================================

    void OnLoad(BattlegroundCoordinator* coordinator) override;
    void OnMatchStart() override;
    void OnMatchEnd(bool victory) override;
    void OnUpdate(uint32 diff) override;
    void OnEvent(const BGScriptEventData& event) override;

    // ========================================================================
    // DATA PROVIDERS
    // ========================================================================

    std::vector<BGObjectiveData> GetObjectiveData() const override;
    std::vector<BGPositionData> GetSpawnPositions(uint32 faction) const override;
    std::vector<BGPositionData> GetStrategicPositions() const override;
    std::vector<BGPositionData> GetGraveyardPositions(uint32 faction) const override;
    std::vector<BGVehicleData> GetVehicleData() const;
    std::vector<BGWorldState> GetInitialWorldStates() const override;

    // ========================================================================
    // WORLD STATE
    // ========================================================================

    bool InterpretWorldState(int32 stateId, int32 value,
        uint32& outObjectiveId, BGObjectiveState& outState) const override;

    void GetScoreFromWorldStates(const std::map<int32, int32>& states,
        uint32& allianceScore, uint32& hordeScore) const override;

    // ========================================================================
    // STRATEGY & ROLE DISTRIBUTION
    // ========================================================================

    RoleDistribution GetRecommendedRoles(const StrategicDecision& decision,
        float scoreAdvantage, uint32 timeRemaining) const override;

    void AdjustStrategy(StrategicDecision& decision,
        float scoreAdvantage, uint32 controlledCount,
        uint32 totalObjectives, uint32 timeRemaining) const override;

    // ========================================================================
    // RUNTIME BEHAVIOR
    // ========================================================================

    bool ExecuteStrategy(::Player* player) override;

    // ========================================================================
    // IOC-SPECIFIC METHODS
    // ========================================================================

    /// Check if a specific gate is destroyed
    bool IsGateDestroyed(uint32 gateId) const;

    /// Get count of intact gates for a faction
    uint32 GetIntactGateCount(uint32 faction) const;

    /// Check if a faction's keep can be accessed (at least one gate down)
    bool CanAccessKeep(uint32 targetFaction) const;

    /// Check if a node is controlled by a faction
    bool IsNodeControlled(uint32 nodeId, uint32 faction) const;

    /// Get list of available vehicles for a faction
    std::vector<uint32> GetAvailableVehicles(uint32 faction) const;

    /// Get current reinforcements for a faction
    uint32 GetReinforcements(uint32 faction) const;

    /// Check if workshop is controlled (enables siege vehicles)
    bool IsWorkshopControlled(uint32 faction) const;

    /// Check if hangar is controlled (enables airship assault)
    bool IsHangarControlled(uint32 faction) const;

    /// Check if boss is attackable (keep accessible)
    bool IsBossViable(uint32 targetFaction) const;

    /// Get priority order for node capture
    std::vector<uint32> GetNodePriorityOrder(uint32 attackingFaction) const;

    /// Get priority order for gate destruction
    std::vector<uint32> GetGatePriorityOrder(uint32 targetFaction) const;

    // ========================================================================
    // ENTERPRISE-GRADE POSITIONING
    // ========================================================================

    /// Get defense positions for a specific node
    std::vector<Position> GetNodeDefensePositions(uint32 nodeId) const;

    /// Get approach positions for a specific gate
    std::vector<Position> GetGateApproachPositions(uint32 gateId) const;

    /// Get chokepoint positions
    std::vector<Position> GetChokepoints() const;

    /// Get sniper/overlook positions
    std::vector<Position> GetSniperPositions() const;

    /// Get ambush positions for a faction
    std::vector<Position> GetAmbushPositions(uint32 faction) const;

    /// Get boss room assault positions for attacking a faction's boss
    std::vector<Position> GetBossRaidPositions(uint32 targetFaction) const;

    /// Get vehicle staging positions for a faction
    std::vector<Position> GetVehicleStagingPositions(uint32 faction) const;

    /// Get siege route for demolisher assault
    std::vector<Position> GetSiegeRoute(uint32 attackingFaction, uint32 targetGate) const;

    /// Get parachute drop zone positions (from Hangar gunship)
    std::vector<Position> GetParachuteDropPositions(uint32 targetFaction) const;

protected:
    // ========================================================================
    // SIEGE ABSTRACT IMPLEMENTATIONS
    // ========================================================================

    uint32 GetBossEntry(uint32 faction) const override;
    Position GetBossPosition(uint32 faction) const override;
    std::vector<BGObjectiveData> GetGateData() const override;
    std::vector<BGObjectiveData> GetTowerData() const override;
    std::vector<BGObjectiveData> GetGraveyardData() const override;

    uint32 GetStartingReinforcements() const override
    {
        return IsleOfConquest::STARTING_REINFORCEMENTS;
    }

    uint32 GetReinforcementLossPerDeath() const override
    {
        return IsleOfConquest::REINF_LOSS_PER_DEATH;
    }

    uint32 GetReinforcementLossPerTower() const override
    {
        return 0; // IOC doesn't have tower destruction reinforcement loss
    }

    bool CanAttackBoss(uint32 faction) const override;

    // ========================================================================
    // PHASE MANAGEMENT
    // ========================================================================

    /// Game phase enumeration for strategy
    enum class IOCPhase
    {
        OPENING,         // First 3 minutes - initial node rush
        NODE_CAPTURE,    // Capturing strategic nodes
        VEHICLE_SIEGE,   // Using vehicles to assault gates
        GATE_ASSAULT,    // Breaking into enemy keep
        BOSS_ASSAULT,    // All-in boss kill
        DEFENSE,         // Defending our keep from enemy assault
        DESPERATE        // Low reinforcements - must act fast
    };

    /// Determine current game phase
    IOCPhase GetCurrentPhase() const;

    /// Get phase name for logging
    const char* GetPhaseName(IOCPhase phase) const;

    /// Apply phase-specific strategy
    void ApplyPhaseStrategy(StrategicDecision& decision, IOCPhase phase, uint32 faction) const;

    /// Apply opening phase strategy
    void ApplyOpeningStrategy(StrategicDecision& decision, uint32 faction) const;

    /// Apply node capture strategy
    void ApplyNodeCaptureStrategy(StrategicDecision& decision, uint32 faction) const;

    /// Apply vehicle siege strategy
    void ApplyVehicleSiegeStrategy(StrategicDecision& decision, uint32 faction) const;

    /// Apply gate assault strategy
    void ApplyGateAssaultStrategy(StrategicDecision& decision, uint32 faction) const;

    /// Apply boss assault strategy
    void ApplyBossAssaultStrategy(StrategicDecision& decision, uint32 faction) const;

    /// Apply defensive strategy
    void ApplyDefensiveStrategy(StrategicDecision& decision, uint32 faction) const;

    /// Apply desperate strategy
    void ApplyDesperateStrategy(StrategicDecision& decision, uint32 faction) const;

private:
    // ========================================================================
    // HELPER METHODS
    // ========================================================================

    /// Get node objective data
    std::vector<BGObjectiveData> GetNodeData() const;

    /// Update node state tracking
    void UpdateNodeStates();

    /// Update gate state tracking
    void UpdateGateStates();

    /// Update vehicle availability
    void UpdateVehicleStates();

    /// Check if we should prioritize vehicle assault
    bool ShouldPrioritizeVehicles() const;

    /// Check if we should use hangar parachute assault
    bool ShouldUseParachuteAssault() const;

    /// Get best gate to target for assault
    uint32 GetBestGateTarget(uint32 attackingFaction) const;

    /// Queue boss NPC attack via BotActionMgr (deferred to main thread)
    void QueueBossAttack(::Player* bot, uint32 targetFaction);

    // ========================================================================
    // STATE TRACKING
    // ========================================================================

    // Thread-safety: OnUpdate/OnEvent writes (main thread), ExecuteStrategy reads (worker thread)
    std::atomic<uint32> m_matchStartTime{0};
    std::atomic<uint32> m_lastStrategyUpdate{0};
    std::atomic<uint32> m_lastNodeCheck{0};
    std::atomic<uint32> m_lastVehicleCheck{0};

    uint32 m_allianceReinforcements = IsleOfConquest::STARTING_REINFORCEMENTS;
    uint32 m_hordeReinforcements = IsleOfConquest::STARTING_REINFORCEMENTS;

    // Node states (faction control: 0=neutral, 1=alliance, 2=horde)
    // Small arrays — single writer (main thread), stale reads are benign
    std::array<uint32, IsleOfConquest::ObjectiveIds::NODE_COUNT> m_nodeControl;

    // Gate states (true = intact)
    std::array<bool, 6> m_gateIntact;

    // Node state map for quick lookup (protected by shared_mutex)
    std::map<uint32, BGObjectiveState> m_nodeStates;
    mutable std::shared_mutex m_nodeStateMutex;

    // Destroyed gates set (protected by shared_mutex)
    std::set<uint32> m_destroyedGates;
    mutable std::shared_mutex m_gateStateMutex;

    // Vehicle availability (protected by shared_mutex)
    std::map<uint32, uint32> m_vehicleAvailability; // vehicleEntry -> count
    mutable std::shared_mutex m_vehicleMutex;

    // Boss status
    std::atomic<bool> m_halfordAlive{true};
    std::atomic<bool> m_agmarAlive{true};

    // Cached boss GUIDs (resolved on main thread in OnUpdate)
    ObjectGuid m_halfordGuid;
    ObjectGuid m_agmarGuid;
    std::atomic<bool> m_bossGuidsResolved{false};
};

} // namespace Playerbot::Coordination::Battleground

#endif // PLAYERBOT_AI_COORDINATION_BG_SIEGE_ISLEOFCONQUESTSCRIPT_H
