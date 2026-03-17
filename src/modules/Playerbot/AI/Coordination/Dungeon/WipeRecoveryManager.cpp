/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license
 * Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "WipeRecoveryManager.h"
#include "DungeonCoordinator.h"
#include "AI/Coordination/Messaging/BotMessageBus.h"
#include "AI/Coordination/Messaging/BotMessage.h"
#include "Player.h"
#include "Group.h"
#include "ObjectAccessor.h"
#include "GameTime.h"
#include "Log.h"

namespace Playerbot {

WipeRecoveryManager::WipeRecoveryManager(DungeonCoordinator* coordinator)
    : _coordinator(coordinator)
{
}

void WipeRecoveryManager::Initialize()
{
    Reset();
    TC_LOG_DEBUG("playerbot", "WipeRecoveryManager::Initialize - Initialized");
}

void WipeRecoveryManager::Update(uint32 diff)
{
    if (_phase == RecoveryPhase::READY)
        return;

    _phaseTimer += diff;

    switch (_phase)
    {
        case RecoveryPhase::WAITING:
            // Wait for combat to fully end
            if (_phaseTimer >= WAITING_TIMEOUT_MS)
            {
                TransitionToPhase(RecoveryPhase::RELEASING);
            }
            break;

        case RecoveryPhase::RELEASING:
            // Wait for players to release spirit
            if (_phaseTimer >= RELEASING_TIMEOUT_MS)
            {
                TransitionToPhase(RecoveryPhase::RUNNING_BACK);
            }
            break;

        case RecoveryPhase::RUNNING_BACK:
            // Wait for players to run back
            if (_playersRunningBack.empty() || _phaseTimer >= RUNNING_BACK_TIMEOUT_MS)
            {
                TransitionToPhase(RecoveryPhase::REZZING);
            }
            break;

        case RecoveryPhase::REZZING:
            // Rez dead players
            if (AllPlayersAlive())
            {
                TransitionToPhase(RecoveryPhase::REBUFFING);
            }
            break;

        case RecoveryPhase::REBUFFING:
            // Wait for rebuffs
            if (AreBuffsComplete() || _phaseTimer >= REBUFFING_TIMEOUT_MS)
            {
                TransitionToPhase(RecoveryPhase::MANA_REGEN);
            }
            break;

        case RecoveryPhase::MANA_REGEN:
            // Wait for mana
            if (GetGroupManaPercent() >= _minManaThreshold || _phaseTimer >= MANA_REGEN_TIMEOUT_MS)
            {
                TransitionToPhase(RecoveryPhase::READY);
            }
            break;

        default:
            break;
    }
}

void WipeRecoveryManager::Reset()
{
    _phase = RecoveryPhase::READY;
    _recoveryStartTime = 0;
    _phaseTimer = 0;
    _deadPlayers.clear();
    _rezzedPlayers.clear();
    _playersRunningBack.clear();
    while (!_rezQueue.empty()) _rezQueue.pop();
    _playersNeedingBuffs.clear();
}

// ============================================================================
// WIPE HANDLING
// ============================================================================

void WipeRecoveryManager::OnGroupWipe()
{
    Reset();

    _phase = RecoveryPhase::WAITING;
    _recoveryStartTime = GameTime::GetGameTimeMS();
    _phaseTimer = 0;

    // Build list of dead players
    // Note: We get members through coordinator's interface
    // since we don't have direct group access
    _deadPlayers.clear();

    // Mark all players as needing buffs
    _playersNeedingBuffs.clear();

    TC_LOG_DEBUG("playerbot", "WipeRecoveryManager::OnGroupWipe - Wipe detected, starting recovery");

    // Broadcast wipe notification to group via BotMessageBus
    if (_coordinator && _coordinator->GetGroup())
    {
        Group* group = _coordinator->GetGroup();
        ObjectGuid groupGuid = group->GetGUID();
        ObjectGuid leaderGuid = group->GetLeaderGUID();
        BotMessage msg = BotMessage::CommandWipeRecovery(leaderGuid, groupGuid);
        sBotMessageBus->Publish(msg);
    }

    // Build rez queue
    BuildRezQueue();
}

void WipeRecoveryManager::OnCombatEnded()
{
    if (_phase == RecoveryPhase::WAITING)
    {
        TransitionToPhase(RecoveryPhase::RELEASING);
    }
}

float WipeRecoveryManager::GetRecoveryProgress() const
{
    switch (_phase)
    {
        case RecoveryPhase::READY:
            return 1.0f;
        case RecoveryPhase::WAITING:
            return 0.0f;
        case RecoveryPhase::RELEASING:
            return 0.1f;
        case RecoveryPhase::RUNNING_BACK:
            return 0.2f;
        case RecoveryPhase::REZZING:
            {
                if (_deadPlayers.empty())
                    return 0.5f;
                float progress = static_cast<float>(_rezzedPlayers.size()) /
                                (_deadPlayers.size() + _rezzedPlayers.size());
                return 0.3f + progress * 0.3f;
            }
        case RecoveryPhase::REBUFFING:
            return 0.7f;
        case RecoveryPhase::MANA_REGEN:
            return 0.9f;
        default:
            return 0.0f;
    }
}

// ============================================================================
// REZ MANAGEMENT
// ============================================================================

void WipeRecoveryManager::BuildRezQueue()
{
    while (!_rezQueue.empty()) _rezQueue.pop();

    for (const ObjectGuid& guid : _deadPlayers)
    {
        RezPriority entry;
        entry.playerGuid = guid;
        entry.priority = CalculateRezPriority(guid);
        entry.hasRezSickness = false;  // Rez sickness only applies to spirit healer resurrects
        entry.distanceToCorpse = 0;    // Distance calculated during corpse run phase

        // Determine role
        if (guid == _coordinator->GetMainTank() || guid == _coordinator->GetOffTank())
        {
            entry.isTank = true;
        }

        const auto& healers = _coordinator->GetHealers();
        entry.isHealer = ::std::find(healers.begin(), healers.end(), guid) != healers.end();
        entry.hasRezSpell = HasRezSpell(guid);

        _rezQueue.push(entry);
    }

    TC_LOG_DEBUG("playerbot", "WipeRecoveryManager::BuildRezQueue - Built queue with %zu entries",
                 _deadPlayers.size());
}

ObjectGuid WipeRecoveryManager::GetNextRezTarget() const
{
    if (_rezQueue.empty())
        return ObjectGuid::Empty;

    // Check if we have any alive players who can rez
    if (GetAliveRezzers().empty())
    {
        // No rezzers - everyone needs to run back
        return ObjectGuid::Empty;
    }

    return _rezQueue.top().playerGuid;
}

void WipeRecoveryManager::OnPlayerRezzed(ObjectGuid playerGuid)
{
    // Remove from dead list
    auto it = ::std::find(_deadPlayers.begin(), _deadPlayers.end(), playerGuid);
    if (it != _deadPlayers.end())
    {
        _deadPlayers.erase(it);
    }

    // Add to rezzed list
    _rezzedPlayers.push_back(playerGuid);

    // Add to needs buffs list
    _playersNeedingBuffs.push_back(playerGuid);

    // Rebuild queue if needed
    BuildRezQueue();

    TC_LOG_DEBUG("playerbot", "WipeRecoveryManager::OnPlayerRezzed - Player rezzed, %zu dead remaining",
                 _deadPlayers.size());
}

bool WipeRecoveryManager::AllPlayersAlive() const
{
    return _deadPlayers.empty();
}

// ============================================================================
// REZ PRIORITY
// ============================================================================

void WipeRecoveryManager::SetRezPriority(ObjectGuid player, uint8 priority)
{
    _customPriorities[player] = priority;
    BuildRezQueue();  // Rebuild to apply new priority
}

uint8 WipeRecoveryManager::GetRezPriority(ObjectGuid player) const
{
    auto it = _customPriorities.find(player);
    if (it != _customPriorities.end())
        return it->second;

    return CalculateRezPriority(player);
}

ObjectGuid WipeRecoveryManager::GetBestRezzer() const
{
    auto rezzers = GetAliveRezzers();
    if (rezzers.empty())
        return ObjectGuid::Empty;

    // Prefer healers
    for (const ObjectGuid& guid : rezzers)
    {
        const auto& healers = _coordinator->GetHealers();
        if (::std::find(healers.begin(), healers.end(), guid) != healers.end())
        {
            return guid;
        }
    }

    // Return first available
    return rezzers[0];
}

// ============================================================================
// RUN BACK
// ============================================================================

bool WipeRecoveryManager::ShouldRunBack() const
{
    // Run back if no one can rez
    return GetAliveRezzers().empty();
}

void WipeRecoveryManager::OnPlayerReachedCorpse(ObjectGuid playerGuid)
{
    auto it = ::std::find(_playersRunningBack.begin(), _playersRunningBack.end(), playerGuid);
    if (it != _playersRunningBack.end())
    {
        _playersRunningBack.erase(it);
    }

    TC_LOG_DEBUG("playerbot", "WipeRecoveryManager::OnPlayerReachedCorpse - Player reached corpse, %zu still running",
                 _playersRunningBack.size());
}

::std::vector<ObjectGuid> WipeRecoveryManager::GetPlayersRunningBack() const
{
    return _playersRunningBack;
}

// ============================================================================
// READY CHECK
// ============================================================================

bool WipeRecoveryManager::IsGroupReady() const
{
    if (!AllPlayersAlive())
        return false;

    if (GetGroupHealthPercent() < _minHealthThreshold)
        return false;

    if (GetGroupManaPercent() < _minManaThreshold)
        return false;

    // Buffs check is optional
    return true;
}

float WipeRecoveryManager::GetGroupManaPercent() const
{
    return _coordinator ? _coordinator->CalculateGroupMana() : 0.0f;
}

float WipeRecoveryManager::GetGroupHealthPercent() const
{
    return _coordinator ? _coordinator->CalculateGroupHealth() : 0.0f;
}

bool WipeRecoveryManager::AreBuffsComplete() const
{
    return _playersNeedingBuffs.empty();
}

// ============================================================================
// REBUFF TRACKING
// ============================================================================

bool WipeRecoveryManager::NeedsBuffs(ObjectGuid player) const
{
    return ::std::find(_playersNeedingBuffs.begin(), _playersNeedingBuffs.end(), player)
           != _playersNeedingBuffs.end();
}

::std::vector<ObjectGuid> WipeRecoveryManager::GetPlayersNeedingBuffs() const
{
    return _playersNeedingBuffs;
}

void WipeRecoveryManager::OnPlayerBuffed(ObjectGuid player)
{
    auto it = ::std::find(_playersNeedingBuffs.begin(), _playersNeedingBuffs.end(), player);
    if (it != _playersNeedingBuffs.end())
    {
        _playersNeedingBuffs.erase(it);
    }
}

// ============================================================================
// HELPERS
// ============================================================================

uint8 WipeRecoveryManager::CalculateRezPriority(ObjectGuid playerGuid) const
{
    // Priority: Lower = Higher Priority
    // 1. Healer with battle rez
    // 2. Other healers
    // 3. Tank
    // 4. DPS with battle rez
    // 5. Other DPS

    const auto& healers = _coordinator->GetHealers();
    bool isHealer = ::std::find(healers.begin(), healers.end(), playerGuid) != healers.end();
    bool isTank = (playerGuid == _coordinator->GetMainTank()) ||
                  (playerGuid == _coordinator->GetOffTank());
    bool hasRez = HasRezSpell(playerGuid);
    bool hasBRez = HasBattleRez(playerGuid);

    if (isHealer && hasBRez)
        return 1;
    if (isHealer)
        return 2;
    if (isTank)
        return 3;
    if (hasRez || hasBRez)
        return 4;

    return 5;
}

void WipeRecoveryManager::TransitionToPhase(RecoveryPhase newPhase)
{
    TC_LOG_DEBUG("playerbot", "WipeRecoveryManager: Phase transition %s -> %s",
                 RecoveryPhaseToString(_phase), RecoveryPhaseToString(newPhase));

    _phase = newPhase;
    _phaseTimer = 0;

    switch (newPhase)
    {
        case RecoveryPhase::RUNNING_BACK:
            // All dead players need to run back
            _playersRunningBack = _deadPlayers;
            break;

        case RecoveryPhase::REZZING:
            BuildRezQueue();
            break;

        default:
            break;
    }
}

bool WipeRecoveryManager::HasBattleRez(ObjectGuid player) const
{
    Player* p = ObjectAccessor::FindPlayer(player);
    if (!p)
        return false;

    // Battle rez spells
    static const ::std::vector<uint32> battleRezSpells = {
        20484,  // Rebirth (Druid)
        61999,  // Raise Ally (Death Knight)
        20707,  // Soulstone (Warlock)
        391054  // Intercession (Paladin)
    };

    for (uint32 spellId : battleRezSpells)
    {
        if (p->HasSpell(spellId))
            return true;
    }

    return false;
}

bool WipeRecoveryManager::HasRezSpell(ObjectGuid player) const
{
    Player* p = ObjectAccessor::FindPlayer(player);
    if (!p)
        return false;

    // Regular rez spells (out of combat only)
    static const ::std::vector<uint32> rezSpells = {
        2006,   // Resurrection (Priest)
        7328,   // Redemption (Paladin)
        2008,   // Ancestral Spirit (Shaman)
        50769,  // Revive (Druid)
        115178, // Resuscitate (Monk)
        361227  // Return (Evoker)
    };

    for (uint32 spellId : rezSpells)
    {
        if (p->HasSpell(spellId))
            return true;
    }

    // Also check battle rez
    return HasBattleRez(player);
}

::std::vector<ObjectGuid> WipeRecoveryManager::GetAliveHealers() const
{
    ::std::vector<ObjectGuid> result;
    const auto& healers = _coordinator->GetHealers();

    for (const ObjectGuid& guid : healers)
    {
        // Check if alive (not in dead list)
        if (::std::find(_deadPlayers.begin(), _deadPlayers.end(), guid) == _deadPlayers.end())
        {
            result.push_back(guid);
        }
    }

    return result;
}

::std::vector<ObjectGuid> WipeRecoveryManager::GetAliveRezzers() const
{
    ::std::vector<ObjectGuid> result;

    // Check all group members for rez capability
    const auto& healers = _coordinator->GetHealers();
    const auto& dps = _coordinator->GetDPS();
    ObjectGuid mainTank = _coordinator->GetMainTank();
    ObjectGuid offTank = _coordinator->GetOffTank();

    auto checkAndAdd = [&](const ObjectGuid& guid) {
        // Skip if dead
        if (::std::find(_deadPlayers.begin(), _deadPlayers.end(), guid) != _deadPlayers.end())
            return;

        // Check if has rez
        if (HasRezSpell(guid))
            result.push_back(guid);
    };

    for (const ObjectGuid& guid : healers)
        checkAndAdd(guid);
    for (const ObjectGuid& guid : dps)
        checkAndAdd(guid);
    if (!mainTank.IsEmpty())
        checkAndAdd(mainTank);
    if (!offTank.IsEmpty())
        checkAndAdd(offTank);

    return result;
}

} // namespace Playerbot
