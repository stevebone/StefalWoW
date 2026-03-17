/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#pragma once

#include "RaidState.h"
#include "Core/Events/CombatEvent.h"
#include <vector>
#include <map>

namespace Playerbot {

class RaidCoordinator;

/**
 * @class RaidTankCoordinator
 * @brief Manages tank assignments and automatic tank swaps
 *
 * Handles:
 * - Main tank and off-tank assignment
 * - Automatic tank swap detection and execution
 * - Debuff stack tracking for swap triggers
 * - Tank assignment to boss and adds
 * - Taunt rotation management
 */
class RaidTankCoordinator
{
public:
    RaidTankCoordinator(RaidCoordinator* coordinator);

    void Initialize();
    void Update(uint32 diff);
    void Reset();

    // ========================================================================
    // TANK ASSIGNMENT
    // ========================================================================

    void AssignMainTank(ObjectGuid tank);
    void AssignOffTank(ObjectGuid tank);
    void AssignTankToBoss(ObjectGuid tank);
    void AssignTankToAdds(ObjectGuid tank, const ::std::vector<ObjectGuid>& adds);

    ObjectGuid GetMainTank() const { return _mainTank; }
    ObjectGuid GetOffTank() const { return _offTank; }
    ObjectGuid GetActiveTank() const { return _activeTank; }

    const ::std::vector<RaidTankInfo>& GetAllTanks() const { return _tanks; }
    RaidTankInfo* GetTankInfo(ObjectGuid tank);
    const RaidTankInfo* GetTankInfo(ObjectGuid tank) const;

    // ========================================================================
    // TANK SWAP CONFIGURATION
    // ========================================================================

    void ConfigureSwapTrigger(uint32 spellId, uint8 stackThreshold);
    void ConfigureSwapTrigger(const TankSwapTrigger& trigger);
    void ClearSwapTriggers();
    void SetSwapEnabled(bool enabled) { _autoSwapEnabled = enabled; }
    bool IsSwapEnabled() const { return _autoSwapEnabled; }

    // ========================================================================
    // TANK SWAP DETECTION
    // ========================================================================

    bool NeedsTankSwap() const;
    uint8 GetSwapDebuffStacks(ObjectGuid tank) const;
    uint8 GetSwapStackThreshold() const;
    bool IsTankSwapImminent() const;
    uint32 GetTimeUntilSwapNeeded() const;

    // ========================================================================
    // TANK SWAP EXECUTION
    // ========================================================================

    void CallTankSwap();
    void SwapTanks(ObjectGuid newActiveTank);
    void OnTankSwapComplete();
    bool IsTankSwapInProgress() const { return _swapInProgress; }

    // ========================================================================
    // EVENT HANDLERS
    // ========================================================================

    void OnSwapDebuffApplied(ObjectGuid tank, uint32 spellId, uint8 stacks);
    void OnSwapDebuffRemoved(ObjectGuid tank, uint32 spellId);
    void OnTankDied(ObjectGuid tank);
    void OnTauntSucceeded(ObjectGuid tank, ObjectGuid target);
    void OnTauntFailed(ObjectGuid tank, ObjectGuid target);

    void OnDamageEvent(const CombatEvent& event);
    void OnAuraEvent(const CombatEvent& event);

    // ========================================================================
    // THREAT MANAGEMENT
    // ========================================================================

    float GetTankThreat(ObjectGuid tank) const;
    ObjectGuid GetHighestThreatTank() const;
    bool HasTankAggro(ObjectGuid tank) const;
    void RequestTaunt(ObjectGuid tank, ObjectGuid target);

    // ========================================================================
    // TANK ROTATION
    // ========================================================================

    ObjectGuid GetNextTankInRotation() const;
    void AdvanceTankRotation();
    uint32 GetTankRotationIndex() const { return _rotationIndex; }

    // ========================================================================
    // UTILITY
    // ========================================================================

    bool IsTank(ObjectGuid player) const;
    bool IsTankAlive(ObjectGuid tank) const;
    bool CanTankTaunt(ObjectGuid tank) const;
    uint32 GetTauntCooldown(ObjectGuid tank) const;

private:
    RaidCoordinator* _coordinator;

    // ========================================================================
    // TANK STATE
    // ========================================================================

    ::std::vector<RaidTankInfo> _tanks;
    ObjectGuid _mainTank;
    ObjectGuid _offTank;
    ObjectGuid _activeTank;             // Currently tanking
    uint32 _rotationIndex = 0;

    // ========================================================================
    // SWAP TRIGGERS
    // ========================================================================

    ::std::vector<TankSwapTrigger> _swapTriggers;
    ::std::map<ObjectGuid, ::std::map<uint32, uint8>> _debuffStacks;  // tank -> spellId -> stacks
    bool _autoSwapEnabled = true;
    bool _swapInProgress = false;
    uint32 _lastSwapTime = 0;
    uint32 _swapCooldown = 3000;        // Minimum time between swaps

    // ========================================================================
    // TIMERS
    // ========================================================================

    uint32 _swapCheckInterval = 500;    // 500ms
    uint32 _lastSwapCheckTime = 0;

    // ========================================================================
    // INTERNAL METHODS
    // ========================================================================

    void UpdateTankInfo();
    void CheckSwapTriggers();
    RaidTankInfo* FindTank(ObjectGuid guid);
    ObjectGuid FindBestSwapTarget() const;
    void ExecuteSwap(ObjectGuid newTank);
    bool ShouldSwapNow(ObjectGuid currentTank) const;
};

} // namespace Playerbot
