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

#ifndef PLAYERBOT_AI_COORDINATION_BG_SIEGE_ALTERACVALLEYSCRIPT_H
#define PLAYERBOT_AI_COORDINATION_BG_SIEGE_ALTERACVALLEYSCRIPT_H

#include "SiegeScriptBase.h"
#include "AlteracValleyData.h"

namespace Playerbot::Coordination::Battleground
{

/**
 * @class AlteracValleyScript
 * @brief Enterprise-grade Alterac Valley battleground script
 *
 * Alterac Valley is a classic 40v40 epic battleground featuring:
 * - Victory Conditions: Kill enemy boss (Vanndar/Drek'Thar) or deplete reinforcements to 0
 * - 8 Towers/Bunkers: 4 per faction, destroying enemy towers weakens their boss
 * - 7 Graveyards: Control spawn points throughout the map
 * - 2 Captains: Balinda (Alliance) and Galvangar (Horde) - killing provides bonuses
 * - 2 Mines: Irondeep and Coldtooth for resource gathering
 * - Reinforcement System: 600 starting, -1 per death, -75 per tower destroyed
 *
 * Key strategies:
 * - Rush: Bypass objectives and go straight to boss (risky but fast)
 * - Tower Burn: Systematically destroy towers to weaken boss (-1 warmaster per tower)
 * - Defense: Hold your towers and graveyards while counter-attacking
 * - Balanced: Split forces between offense and defense
 *
 * This script provides:
 * - 64 tower defense positions (8 per tower)
 * - 42 graveyard defense positions (6 per graveyard)
 * - 24 boss room raid positions (12 per boss)
 * - 10 map chokepoints
 * - 8 sniper/overlook positions
 * - 12 ambush positions (6 per faction)
 * - Phase-aware strategy (opening, tower burn, GY push, boss assault, defense)
 *
 * Map ID: 30
 */
class AlteracValleyScript : public SiegeScriptBase
{
public:
    AlteracValleyScript() = default;
    ~AlteracValleyScript() = default;

    // ========================================================================
    // IDENTIFICATION
    // ========================================================================

    uint32 GetMapId() const override { return AlteracValley::MAP_ID; }
    std::string GetName() const override { return AlteracValley::BG_NAME; }
    BGType GetBGType() const override { return BGType::ALTERAC_VALLEY; }
    uint32 GetMaxScore() const override { return AlteracValley::STARTING_REINFORCEMENTS; }
    uint32 GetMaxDuration() const override { return AlteracValley::MAX_DURATION; }
    uint8 GetTeamSize() const override { return AlteracValley::TEAM_SIZE; }

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
    std::vector<BGWorldState> GetInitialWorldStates() const override;

    // ========================================================================
    // WORLD STATE
    // ========================================================================

    bool InterpretWorldState(int32 stateId, int32 value,
        uint32& outObjectiveId, BGObjectiveState& outState) const override;

    void GetScoreFromWorldStates(const std::map<int32, int32>& states,
        uint32& allianceScore, uint32& hordeScore) const override;

    // ========================================================================
    // RUNTIME BEHAVIOR
    // ========================================================================

    bool ExecuteStrategy(::Player* player) override;

    // ========================================================================
    // STRATEGY & ROLE DISTRIBUTION
    // ========================================================================

    RoleDistribution GetRecommendedRoles(const StrategicDecision& decision,
        float scoreAdvantage, uint32 timeRemaining) const override;

    void AdjustStrategy(StrategicDecision& decision,
        float scoreAdvantage, uint32 controlledCount,
        uint32 totalObjectives, uint32 timeRemaining) const override;

    // ========================================================================
    // AV-SPECIFIC METHODS
    // ========================================================================

    /// Get current reinforcements for a faction
    uint32 GetReinforcements(uint32 faction) const;

    /// Check if a tower is still standing
    bool IsTowerStanding(uint32 towerId) const;

    /// Get count of standing towers for a faction
    uint32 GetStandingTowerCount(uint32 faction) const;

    /// Get count of destroyed enemy towers
    uint32 GetDestroyedEnemyTowerCount(uint32 faction) const;

    /// Check if captain is alive
    bool IsCaptainAlive(uint32 faction) const;

    /// Check if boss is attackable (enough towers burned)
    bool IsBossViable(uint32 targetFaction) const;

    /// Get tower burn priority order for attacking faction
    std::vector<uint32> GetTowerBurnOrder(uint32 attackingFaction) const;

    /// Get rush route for a faction
    std::vector<Position> GetRushRoute(uint32 faction) const;

    /// Get tower burn route for a faction
    std::vector<Position> GetTowerBurnRoute(uint32 faction) const;

    // ========================================================================
    // ENTERPRISE-GRADE POSITIONING
    // ========================================================================

    /// Get defense positions for a specific tower
    std::vector<Position> GetTowerDefensePositions(uint32 towerId) const;

    /// Get defense positions for a specific graveyard
    std::vector<Position> GetGraveyardDefensePositions(uint32 graveyardId) const;

    /// Get chokepoint positions
    std::vector<Position> GetChokepoints() const;

    /// Get sniper/overlook positions
    std::vector<Position> GetSniperPositions() const;

    /// Get ambush positions for a faction
    std::vector<Position> GetAmbushPositions(uint32 faction) const;

    /// Get boss raid positions for attacking a faction's boss
    std::vector<Position> GetBossRaidPositions(uint32 targetFaction) const;

    /// Get captain position
    Position GetCaptainPosition(uint32 faction) const;

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
        return AlteracValley::STARTING_REINFORCEMENTS;
    }

    uint32 GetReinforcementLossPerDeath() const override
    {
        return AlteracValley::REINF_LOSS_PER_DEATH;
    }

    uint32 GetReinforcementLossPerTower() const override
    {
        return AlteracValley::REINF_LOSS_PER_TOWER;
    }

    bool CanAttackBoss(uint32 faction) const override;

    // ========================================================================
    // PHASE MANAGEMENT
    // ========================================================================

    /// Game phase enumeration for strategy
    enum class AVPhase
    {
        OPENING,         // First 3 minutes - initial push
        TOWER_BURN,      // Burning enemy towers
        GRAVEYARD_PUSH,  // Taking forward graveyards
        BOSS_ASSAULT,    // All-in boss kill
        DEFENSE,         // Holding against enemy push
        DESPERATE        // Low reinforcements - must act fast
    };

    /// Determine current game phase
    AVPhase GetCurrentPhase() const;

    /// Get phase name for logging
    const char* GetPhaseName(AVPhase phase) const;

    /// Apply phase-specific strategy
    void ApplyPhaseStrategy(StrategicDecision& decision, AVPhase phase, uint32 faction) const;

    /// Apply opening phase strategy
    void ApplyOpeningStrategy(StrategicDecision& decision, uint32 faction) const;

    /// Apply tower burn strategy
    void ApplyTowerBurnStrategy(StrategicDecision& decision, uint32 faction) const;

    /// Apply graveyard push strategy
    void ApplyGraveyardPushStrategy(StrategicDecision& decision, uint32 faction) const;

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

    /// Check if we should burn towers before boss
    bool ShouldBurnTowers() const;

    /// Update tower state tracking
    void UpdateTowerStates();

    /// Update graveyard state tracking
    void UpdateGraveyardStates();

    /// Get objective data for captains
    BGObjectiveData GetCaptainData(uint32 faction) const;

    // ========================================================================
    // STATE TRACKING
    // ========================================================================

    uint32 m_matchStartTime = 0;
    uint32 m_lastStrategyUpdate = 0;
    uint32 m_lastTowerCheck = 0;

    uint32 m_allianceReinforcements = AlteracValley::STARTING_REINFORCEMENTS;
    uint32 m_hordeReinforcements = AlteracValley::STARTING_REINFORCEMENTS;

    // Tower states (true = standing)
    std::array<bool, AlteracValley::Towers::COUNT> m_towerStanding;

    // Graveyard control
    std::array<uint32, AlteracValley::Graveyards::COUNT> m_graveyardControl;  // 0=neutral, 1=ally, 2=horde

    // Captain/Boss status
    bool m_balindaAlive = true;
    bool m_galvangarAlive = true;
    bool m_vanndarAlive = true;
    bool m_drektharAlive = true;

    // Boss GUID cache (populated on main thread via OnUpdate)
    ObjectGuid m_vanndarGuid;
    ObjectGuid m_drektharGuid;
    bool m_bossGuidsResolved = false;

    /// Queue attack on boss NPC via deferred action system (thread-safe)
    void QueueBossAttack(::Player* bot, uint32 enemyFaction);
};

} // namespace Playerbot::Coordination::Battleground

#endif // PLAYERBOT_AI_COORDINATION_BG_SIEGE_ALTERACVALLEYSCRIPT_H
