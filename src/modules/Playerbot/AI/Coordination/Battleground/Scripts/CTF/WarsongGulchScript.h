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

#ifndef PLAYERBOT_AI_COORDINATION_BG_CTF_WARSONGGULCHSCRIPT_H
#define PLAYERBOT_AI_COORDINATION_BG_CTF_WARSONGGULCHSCRIPT_H

#include "CTFScriptBase.h"
#include "WarsongGulchData.h"

namespace Playerbot::Coordination::Battleground
{

/**
 * @brief Warsong Gulch battleground script
 *
 * Implements CTF mechanics for Warsong Gulch:
 * - 10v10 capture the flag
 * - First to 3 captures wins
 * - 25 minute time limit
 * - Focused/Brutal Assault debuffs after 10/15 minutes
 *
 * Map ID: 489 (original), 2106 (remake)
 */
class WarsongGulchScript : public CTFScriptBase
{
public:
    WarsongGulchScript() = default;
    ~WarsongGulchScript() = default;

    // ========================================================================
    // IDENTIFICATION
    // ========================================================================

    uint32 GetMapId() const override { return WarsongGulch::MAP_ID; }
    std::string GetName() const override { return WarsongGulch::BG_NAME; }
    BGType GetBGType() const override { return BGType::WARSONG_GULCH; }
    uint32 GetMaxScore() const override { return WarsongGulch::MAX_SCORE; }
    uint32 GetMaxDuration() const override { return WarsongGulch::MAX_DURATION; }
    uint8 GetTeamSize() const override { return WarsongGulch::TEAM_SIZE; }

    // ========================================================================
    // LIFECYCLE
    // ========================================================================

    void OnLoad(BattlegroundCoordinator* coordinator) override;

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
    // EVENTS
    // ========================================================================

    void OnEvent(const BGScriptEventData& event) override;

    // ========================================================================
    // RUNTIME BEHAVIOR
    // ========================================================================

    /**
     * @brief Dynamic behavior tree for WSG bot strategy
     * Evaluates game state each tick and selects the highest-priority action.
     * @param player The bot player to execute strategy for
     * @return true if the script handled the player's behavior
     */
    bool ExecuteStrategy(::Player* player) override;

    /**
     * @brief Get FC route waypoints using the standard kite path
     * WSG uses a single pre-calculated kite path per faction.
     */
    std::vector<Position> GetFCRouteWaypoints(uint32 faction,
        const std::vector<Position>& enemyPositions) const override;

    // ========================================================================
    // UTILITY
    // ========================================================================

    std::vector<Position> GetObjectivePath(
        uint32 fromObjective, uint32 toObjective) const override;

protected:
    // ========================================================================
    // CTF ABSTRACT IMPLEMENTATIONS
    // ========================================================================

    Position GetAllianceFlagPosition() const override
    {
        return WarsongGulch::GetAllianceFlagPos();
    }

    Position GetHordeFlagPosition() const override
    {
        return WarsongGulch::GetHordeFlagPos();
    }

    std::vector<Position> GetAllianceFlagRoomDefense() const override
    {
        return WarsongGulch::GetAllianceFlagRoomDefense();
    }

    std::vector<Position> GetHordeFlagRoomDefense() const override
    {
        return WarsongGulch::GetHordeFlagRoomDefense();
    }

    std::vector<Position> GetMiddleChokepoints() const override
    {
        return WarsongGulch::GetMiddleChokepoints();
    }

    std::vector<Position> GetSpeedBuffPositions() const override
    {
        return WarsongGulch::GetSpeedBuffPositions();
    }

    std::vector<Position> GetRestoreBuffPositions() const override
    {
        return WarsongGulch::GetRestoreBuffPositions();
    }

    std::vector<Position> GetBerserkBuffPositions() const override
    {
        return WarsongGulch::GetBerserkBuffPositions();
    }

private:
    // ========================================================================
    // WSG-SPECIFIC HELPERS
    // ========================================================================

    /**
     * @brief Get the optimal flag running route
     * @param faction The faction doing the run
     * @param useSpeedBuff Should route through speed buff
     */
    std::vector<Position> GetFlagRunRoute(uint32 faction, bool useSpeedBuff) const;

    /**
     * @brief Get tunnel ambush positions
     */
    std::vector<Position> GetTunnelAmbushPositions(uint32 faction) const;
};

} // namespace Playerbot::Coordination::Battleground

#endif // PLAYERBOT_AI_COORDINATION_BG_CTF_WARSONGGULCHSCRIPT_H
