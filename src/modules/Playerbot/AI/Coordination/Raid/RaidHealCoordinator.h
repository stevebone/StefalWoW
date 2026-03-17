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
 * @struct ExternalCDRequest
 * @brief Request for external cooldown usage
 */
struct ExternalCDRequest
{
    ObjectGuid targetGuid;
    uint8 urgency;              // 0-10, 10 = immediate
    uint32 requestTime;
    bool fulfilled;

    ExternalCDRequest()
        : urgency(5), requestTime(0), fulfilled(false) {}
};

/**
 * @class RaidHealCoordinator
 * @brief Manages healer assignments and external cooldown rotation
 *
 * Handles:
 * - Healer to tank assignments
 * - Healer to group assignments
 * - External cooldown coordination
 * - Dispel duty assignment
 * - Automatic healer rebalancing
 */
class RaidHealCoordinator
{
public:
    RaidHealCoordinator(RaidCoordinator* coordinator);

    void Initialize();
    void Update(uint32 diff);
    void Reset();

    // ========================================================================
    // HEALER ASSIGNMENT
    // ========================================================================

    void AssignHealerToTank(ObjectGuid healer, ObjectGuid tank);
    void AssignHealerToGroup(ObjectGuid healer, uint8 groupNum);
    void AssignHealerToDispel(ObjectGuid healer);
    void AssignHealerToRaid(ObjectGuid healer);
    void UnassignHealer(ObjectGuid healer);

    HealerAssignment GetHealerAssignment(ObjectGuid healer) const;
    ObjectGuid GetAssignedTarget(ObjectGuid healer) const;
    ::std::vector<ObjectGuid> GetHealersAssignedTo(ObjectGuid target) const;
    ::std::vector<ObjectGuid> GetHealersAssignedToGroup(uint8 groupNum) const;

    // ========================================================================
    // HEALER INFO
    // ========================================================================

    const ::std::vector<RaidHealerInfo>& GetAllHealers() const { return _healers; }
    RaidHealerInfo* GetHealerInfo(ObjectGuid healer);
    const RaidHealerInfo* GetHealerInfo(ObjectGuid healer) const;
    uint32 GetHealerCount() const { return static_cast<uint32>(_healers.size()); }

    // ========================================================================
    // AUTO ASSIGNMENT
    // ========================================================================

    void AutoAssignHealers();
    void RebalanceAssignments();
    bool NeedsRebalancing() const;

    // ========================================================================
    // EXTERNAL COOLDOWNS
    // ========================================================================

    void RequestExternalCooldown(ObjectGuid target, uint8 urgency);
    void OnExternalCooldownUsed(ObjectGuid healer, ObjectGuid target, uint32 spellId);
    ObjectGuid GetNextExternalProvider() const;
    bool HasExternalAvailable() const;
    uint32 GetExternalCooldownCount() const;
    ::std::vector<ObjectGuid> GetAvailableExternalProviders() const;

    // ========================================================================
    // DISPEL COORDINATION
    // ========================================================================

    void RequestDispel(ObjectGuid target, uint32 auraId);
    void OnDispelSucceeded(ObjectGuid healer, ObjectGuid target, uint32 auraId);
    ::std::vector<ObjectGuid> GetDispelHealers() const;

    // ========================================================================
    // MANA MANAGEMENT
    // ========================================================================

    float GetAverageHealerMana() const;
    float GetLowestHealerMana() const;
    ObjectGuid GetLowestManaHealer() const;
    bool AreHealersLowOnMana() const;
    void SignalManaBreak();

    // ========================================================================
    // EVENT HANDLERS
    // ========================================================================

    void OnHealingEvent(const CombatEvent& event);
    void OnHealerDied(ObjectGuid healer);
    void OnTankAssigned(ObjectGuid tank);
    void OnTankDied(ObjectGuid tank);

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    void SetAutoAssignEnabled(bool enabled) { _autoAssignEnabled = enabled; }
    bool IsAutoAssignEnabled() const { return _autoAssignEnabled; }
    void SetMinTankHealers(uint8 count) { _minTankHealers = count; }
    void SetLowManaThreshold(float percent) { _lowManaThreshold = percent; }

private:
    RaidCoordinator* _coordinator;

    // ========================================================================
    // HEALER STATE
    // ========================================================================

    ::std::vector<RaidHealerInfo> _healers;
    ::std::map<ObjectGuid, HealerAssignment> _assignments;
    ::std::map<ObjectGuid, ObjectGuid> _tankAssignments;     // healer -> tank
    ::std::map<ObjectGuid, uint8> _groupAssignments;         // healer -> group

    // ========================================================================
    // EXTERNAL CDS
    // ========================================================================

    ::std::vector<ExternalCDRequest> _externalRequests;
    ::std::map<ObjectGuid, uint32> _externalCooldowns;       // healer -> cooldown remaining
    uint32 _externalCDDuration = 60000;                      // 1 minute default

    // ========================================================================
    // DISPELS
    // ========================================================================

    ::std::vector<ObjectGuid> _dispelHealers;

    // ========================================================================
    // CONFIGURATION
    // ========================================================================

    bool _autoAssignEnabled = true;
    uint8 _minTankHealers = 1;
    float _lowManaThreshold = 30.0f;

    // ========================================================================
    // INTERNAL METHODS
    // ========================================================================

    void UpdateHealerInfo();
    void ProcessExternalRequests();
    RaidHealerInfo* FindHealer(ObjectGuid guid);
    ObjectGuid FindBestHealerForTank(ObjectGuid tank) const;
    ObjectGuid FindBestHealerForGroup(uint8 groupNum) const;
    float CalculateHealerSuitability(ObjectGuid healer, HealerAssignment assignment) const;
};

} // namespace Playerbot
