/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "RaidTankCoordinator.h"
#include "RaidCoordinator.h"
#include "AI/Coordination/Messaging/BotMessageBus.h"
#include "AI/Coordination/Messaging/BotMessage.h"
#include "Player.h"
#include "Group.h"
#include "Log.h"

namespace Playerbot {

// ============================================================================
// CONSTRUCTOR
// ============================================================================

RaidTankCoordinator::RaidTankCoordinator(RaidCoordinator* coordinator)
    : _coordinator(coordinator)
{
}

// ============================================================================
// LIFECYCLE
// ============================================================================

void RaidTankCoordinator::Initialize()
{
    Reset();

    // Initialize tank info from coordinator's tank list
    for (ObjectGuid tankGuid : _coordinator->GetTanks())
    {
        RaidTankInfo info;
        info.guid = tankGuid;
        info.role = TankRole::OFF_TANK;
        _tanks.push_back(info);
    }

    // Assign first tank as main, second as off
    if (!_tanks.empty())
    {
        AssignMainTank(_tanks[0].guid);
    }
    if (_tanks.size() > 1)
    {
        AssignOffTank(_tanks[1].guid);
    }

    TC_LOG_DEBUG("playerbots.raid", "RaidTankCoordinator::Initialize - Initialized with %zu tanks",
        _tanks.size());
}

void RaidTankCoordinator::Update(uint32 diff)
{
    // Update tank info periodically
    UpdateTankInfo();

    // Check swap triggers
    _lastSwapCheckTime += diff;
    if (_lastSwapCheckTime >= _swapCheckInterval)
    {
        _lastSwapCheckTime = 0;
        CheckSwapTriggers();
    }

    // Update last swap time
    if (_lastSwapTime > 0 && _lastSwapTime < _swapCooldown)
        _lastSwapTime += diff;
}

void RaidTankCoordinator::Reset()
{
    _tanks.clear();
    _mainTank.Clear();
    _offTank.Clear();
    _activeTank.Clear();
    _debuffStacks.clear();
    _swapInProgress = false;
    _lastSwapTime = 0;
    _rotationIndex = 0;
}

// ============================================================================
// TANK ASSIGNMENT
// ============================================================================

void RaidTankCoordinator::AssignMainTank(ObjectGuid tank)
{
    _mainTank = tank;
    _activeTank = tank;

    RaidTankInfo* info = FindTank(tank);
    if (info)
    {
        info->isMainTank = true;
        info->role = TankRole::ACTIVE;
    }

    TC_LOG_DEBUG("playerbots.raid", "RaidTankCoordinator::AssignMainTank - Main tank assigned");
}

void RaidTankCoordinator::AssignOffTank(ObjectGuid tank)
{
    _offTank = tank;

    RaidTankInfo* info = FindTank(tank);
    if (info)
    {
        info->isMainTank = false;
        info->role = TankRole::SWAP_READY;
    }

    TC_LOG_DEBUG("playerbots.raid", "RaidTankCoordinator::AssignOffTank - Off tank assigned");
}

void RaidTankCoordinator::AssignTankToBoss(ObjectGuid tank)
{
    RaidTankInfo* info = FindTank(tank);
    if (info)
    {
        info->role = TankRole::ACTIVE;
        info->currentTarget = _coordinator->GetCurrentBossTarget();
    }
}

void RaidTankCoordinator::AssignTankToAdds(ObjectGuid tank, const ::std::vector<ObjectGuid>& adds)
{
    RaidTankInfo* info = FindTank(tank);
    if (info)
    {
        info->role = TankRole::ADD_DUTY;
        if (!adds.empty())
            info->currentTarget = adds.front();
    }
}

RaidTankInfo* RaidTankCoordinator::GetTankInfo(ObjectGuid tank)
{
    return FindTank(tank);
}

const RaidTankInfo* RaidTankCoordinator::GetTankInfo(ObjectGuid tank) const
{
    for (const auto& info : _tanks)
    {
        if (info.guid == tank)
            return &info;
    }
    return nullptr;
}

// ============================================================================
// TANK SWAP CONFIGURATION
// ============================================================================

void RaidTankCoordinator::ConfigureSwapTrigger(uint32 spellId, uint8 stackThreshold)
{
    TankSwapTrigger trigger;
    trigger.debuffSpellId = spellId;
    trigger.stackThreshold = stackThreshold;
    _swapTriggers.push_back(trigger);

    TC_LOG_DEBUG("playerbots.raid", "RaidTankCoordinator::ConfigureSwapTrigger - Added trigger: spell %u at %u stacks",
        spellId, stackThreshold);
}

void RaidTankCoordinator::ConfigureSwapTrigger(const TankSwapTrigger& trigger)
{
    _swapTriggers.push_back(trigger);
}

void RaidTankCoordinator::ClearSwapTriggers()
{
    _swapTriggers.clear();
    _debuffStacks.clear();
}

// ============================================================================
// TANK SWAP DETECTION
// ============================================================================

bool RaidTankCoordinator::NeedsTankSwap() const
{
    if (!_autoSwapEnabled)
        return false;

    if (_swapInProgress)
        return false;

    if (_lastSwapTime < _swapCooldown)
        return false;

    return ShouldSwapNow(_activeTank);
}

uint8 RaidTankCoordinator::GetSwapDebuffStacks(ObjectGuid tank) const
{
    auto tankIt = _debuffStacks.find(tank);
    if (tankIt == _debuffStacks.end())
        return 0;

    // Return highest stack count across all swap triggers
    uint8 maxStacks = 0;
    for (const auto& trigger : _swapTriggers)
    {
        auto spellIt = tankIt->second.find(trigger.debuffSpellId);
        if (spellIt != tankIt->second.end())
        {
            maxStacks = std::max(maxStacks, spellIt->second);
        }
    }

    return maxStacks;
}

uint8 RaidTankCoordinator::GetSwapStackThreshold() const
{
    if (_swapTriggers.empty())
        return 0;

    return _swapTriggers.front().stackThreshold;
}

bool RaidTankCoordinator::IsTankSwapImminent() const
{
    uint8 stacks = GetSwapDebuffStacks(_activeTank);
    uint8 threshold = GetSwapStackThreshold();

    return stacks >= threshold - 1;
}

uint32 RaidTankCoordinator::GetTimeUntilSwapNeeded() const
{
    // Estimate based on debuff duration and stacks
    // This is a rough approximation
    return 5000; // Default 5 seconds
}

// ============================================================================
// TANK SWAP EXECUTION
// ============================================================================

void RaidTankCoordinator::CallTankSwap()
{
    if (_swapInProgress)
        return;

    ObjectGuid newTank = FindBestSwapTarget();
    if (newTank.IsEmpty())
    {
        TC_LOG_WARN("playerbots.raid", "RaidTankCoordinator::CallTankSwap - No valid swap target found!");
        return;
    }

    ExecuteSwap(newTank);
}

void RaidTankCoordinator::SwapTanks(ObjectGuid newActiveTank)
{
    ExecuteSwap(newActiveTank);
}

void RaidTankCoordinator::OnTankSwapComplete()
{
    _swapInProgress = false;
    _lastSwapTime = 0;

    TC_LOG_DEBUG("playerbots.raid", "RaidTankCoordinator::OnTankSwapComplete - Swap complete");
}

// ============================================================================
// EVENT HANDLERS
// ============================================================================

void RaidTankCoordinator::OnSwapDebuffApplied(ObjectGuid tank, uint32 spellId, uint8 stacks)
{
    _debuffStacks[tank][spellId] = stacks;

    TC_LOG_DEBUG("playerbots.raid", "RaidTankCoordinator::OnSwapDebuffApplied - Tank has %u stacks of %u",
        stacks, spellId);

    // Check if swap needed
    if (_autoSwapEnabled && NeedsTankSwap())
    {
        CallTankSwap();
    }
}

void RaidTankCoordinator::OnSwapDebuffRemoved(ObjectGuid tank, uint32 spellId)
{
    auto tankIt = _debuffStacks.find(tank);
    if (tankIt != _debuffStacks.end())
    {
        tankIt->second.erase(spellId);
    }
}

void RaidTankCoordinator::OnTankDied(ObjectGuid tank)
{
    TC_LOG_DEBUG("playerbots.raid", "RaidTankCoordinator::OnTankDied - Tank died!");

    RaidTankInfo* info = FindTank(tank);
    if (info)
    {
        info->role = TankRole::RECOVERING;
    }

    // Emergency swap if active tank died
    if (tank == _activeTank)
    {
        ObjectGuid newTank = FindBestSwapTarget();
        if (!newTank.IsEmpty())
        {
            ExecuteSwap(newTank);
        }
    }
}

void RaidTankCoordinator::OnTauntSucceeded(ObjectGuid tank, ObjectGuid target)
{
    RaidTankInfo* info = FindTank(tank);
    if (info)
    {
        info->currentTarget = target;
    }

    // If this was part of a swap, complete it
    if (_swapInProgress && tank != _activeTank)
    {
        _activeTank = tank;

        // Update roles
        for (auto& tankInfo : _tanks)
        {
            if (tankInfo.guid == tank)
                tankInfo.role = TankRole::ACTIVE;
            else if (tankInfo.role == TankRole::ACTIVE)
                tankInfo.role = TankRole::RECOVERING;
        }

        OnTankSwapComplete();
    }
}

void RaidTankCoordinator::OnTauntFailed(ObjectGuid tank, ObjectGuid /*target*/)
{
    TC_LOG_WARN("playerbots.raid", "RaidTankCoordinator::OnTauntFailed - Taunt failed!");

    // May need to retry or use different tank
    if (_swapInProgress)
    {
        // Try next tank in rotation
        ObjectGuid nextTank = GetNextTankInRotation();
        if (!nextTank.IsEmpty() && nextTank != tank)
        {
            RequestTaunt(nextTank, _coordinator->GetCurrentBossTarget());
        }
    }
}

void RaidTankCoordinator::OnDamageEvent(const CombatEvent& event)
{
    // Track damage to tanks for threat estimation
    for (auto& tankInfo : _tanks)
    {
        if (tankInfo.guid == event.target)
        {
            // Tank is being hit - likely has aggro
        }
    }
}

void RaidTankCoordinator::OnAuraEvent(const CombatEvent& event)
{
    // Check for swap trigger debuffs
    for (const auto& trigger : _swapTriggers)
    {
        if (event.spellId == trigger.debuffSpellId)
        {
            if (IsTank(event.target))
            {
                if (event.type == CombatEventType::AURA_APPLIED)
                {
                    OnSwapDebuffApplied(event.target, event.spellId, 1);
                }
                else if (event.type == CombatEventType::AURA_REMOVED)
                {
                    OnSwapDebuffRemoved(event.target, event.spellId);
                }
            }
        }
    }
}

// ============================================================================
// THREAT MANAGEMENT
// ============================================================================

float RaidTankCoordinator::GetTankThreat(ObjectGuid tank) const
{
    const RaidTankInfo* info = GetTankInfo(tank);
    return info ? info->threatPercentage : 0.0f;
}

ObjectGuid RaidTankCoordinator::GetHighestThreatTank() const
{
    ObjectGuid highest;
    float highestThreat = 0.0f;

    for (const auto& tank : _tanks)
    {
        if (tank.threatPercentage > highestThreat)
        {
            highestThreat = tank.threatPercentage;
            highest = tank.guid;
        }
    }

    return highest;
}

bool RaidTankCoordinator::HasTankAggro(ObjectGuid tank) const
{
    const RaidTankInfo* info = GetTankInfo(tank);
    return info && info->role == TankRole::ACTIVE;
}

void RaidTankCoordinator::RequestTaunt(ObjectGuid tank, ObjectGuid target)
{
    TC_LOG_DEBUG("playerbots.raid", "RaidTankCoordinator::RequestTaunt - Requesting taunt");

    // Implementation would trigger bot to use taunt
    RaidTankInfo* info = FindTank(tank);
    if (info)
    {
        info->currentTarget = target;
    }
}

// ============================================================================
// TANK ROTATION
// ============================================================================

ObjectGuid RaidTankCoordinator::GetNextTankInRotation() const
{
    if (_tanks.empty())
        return ObjectGuid();

    uint32 nextIndex = (_rotationIndex + 1) % _tanks.size();

    // Find next alive tank
    for (uint32 i = 0; i < _tanks.size(); ++i)
    {
        uint32 index = (nextIndex + i) % _tanks.size();
        if (IsTankAlive(_tanks[index].guid) && _tanks[index].guid != _activeTank)
        {
            return _tanks[index].guid;
        }
    }

    return ObjectGuid();
}

void RaidTankCoordinator::AdvanceTankRotation()
{
    _rotationIndex = (_rotationIndex + 1) % _tanks.size();
}

// ============================================================================
// UTILITY
// ============================================================================

bool RaidTankCoordinator::IsTank(ObjectGuid player) const
{
    for (const auto& tank : _tanks)
    {
        if (tank.guid == player)
            return true;
    }
    return false;
}

bool RaidTankCoordinator::IsTankAlive(ObjectGuid tank) const
{
    Player* player = _coordinator->GetPlayer(tank);
    return player && player->IsAlive();
}

bool RaidTankCoordinator::CanTankTaunt(ObjectGuid tank) const
{
    const RaidTankInfo* info = GetTankInfo(tank);
    if (!info)
        return false;

    return info->tauntCooldown == 0 && IsTankAlive(tank);
}

uint32 RaidTankCoordinator::GetTauntCooldown(ObjectGuid tank) const
{
    const RaidTankInfo* info = GetTankInfo(tank);
    return info ? info->tauntCooldown : 0;
}

// ============================================================================
// INTERNAL METHODS (PRIVATE)
// ============================================================================

void RaidTankCoordinator::UpdateTankInfo()
{
    for (auto& tankInfo : _tanks)
    {
        Player* player = _coordinator->GetPlayer(tankInfo.guid);
        if (!player)
            continue;

        // Update from actual player state
        // Note: Threat APIs vary by expansion
    }
}

void RaidTankCoordinator::CheckSwapTriggers()
{
    if (!_autoSwapEnabled || _swapInProgress)
        return;

    if (NeedsTankSwap())
    {
        CallTankSwap();
    }
}

RaidTankInfo* RaidTankCoordinator::FindTank(ObjectGuid guid)
{
    for (auto& tank : _tanks)
    {
        if (tank.guid == guid)
            return &tank;
    }
    return nullptr;
}

ObjectGuid RaidTankCoordinator::FindBestSwapTarget() const
{
    // Prefer off-tank if available
    if (!_offTank.IsEmpty() && _offTank != _activeTank && IsTankAlive(_offTank) && CanTankTaunt(_offTank))
    {
        return _offTank;
    }

    // Otherwise find any available tank
    for (const auto& tank : _tanks)
    {
        if (tank.guid != _activeTank && IsTankAlive(tank.guid) && CanTankTaunt(tank.guid))
        {
            // Check debuff stacks - prefer tank with fewer stacks
            uint8 stacks = GetSwapDebuffStacks(tank.guid);
            if (stacks == 0)
                return tank.guid;
        }
    }

    // Last resort: any alive tank that can taunt
    for (const auto& tank : _tanks)
    {
        if (tank.guid != _activeTank && IsTankAlive(tank.guid))
        {
            return tank.guid;
        }
    }

    return ObjectGuid();
}

void RaidTankCoordinator::ExecuteSwap(ObjectGuid newTank)
{
    if (newTank.IsEmpty())
        return;

    _swapInProgress = true;

    TC_LOG_DEBUG("playerbots.raid", "RaidTankCoordinator::ExecuteSwap - Initiating tank swap");

    // Request new tank to taunt
    RequestTaunt(newTank, _coordinator->GetCurrentBossTarget());

    // Update roles
    RaidTankInfo* newTankInfo = FindTank(newTank);
    if (newTankInfo)
    {
        newTankInfo->role = TankRole::ACTIVE;
    }

    RaidTankInfo* oldTankInfo = FindTank(_activeTank);
    if (oldTankInfo)
    {
        oldTankInfo->role = TankRole::RECOVERING;
    }

    // Advance rotation
    AdvanceTankRotation();

    // Broadcast tank swap request via BotMessageBus
    if (_coordinator && !_coordinator->GetAllMembers().empty())
    {
        Player* leader = _coordinator->GetPlayer(_coordinator->GetAllMembers().front());
        if (leader && leader->GetGroup())
        {
            ObjectGuid groupGuid = leader->GetGroup()->GetGUID();
            ObjectGuid bossGuid = _coordinator->GetCurrentBossTarget();
            BotMessage msg = BotMessage::RequestTankSwap(_activeTank, groupGuid, bossGuid, 0);
            sBotMessageBus->Publish(msg);
        }
    }
}

bool RaidTankCoordinator::ShouldSwapNow(ObjectGuid currentTank) const
{
    if (currentTank.IsEmpty())
        return false;

    uint8 stacks = GetSwapDebuffStacks(currentTank);

    for (const auto& trigger : _swapTriggers)
    {
        auto tankIt = _debuffStacks.find(currentTank);
        if (tankIt != _debuffStacks.end())
        {
            auto spellIt = tankIt->second.find(trigger.debuffSpellId);
            if (spellIt != tankIt->second.end())
            {
                if (spellIt->second >= trigger.stackThreshold)
                    return true;
            }
        }

        if (trigger.swapOnCast)
        {
            // Check for cast-based triggers (would need spell tracking)
        }
    }

    return false;
}

} // namespace Playerbot
